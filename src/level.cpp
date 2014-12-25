#include "shutdown.h"
#include "level.h"
#include "orders.h"
#include "types.h"
#include "units.h"
#include "util.h"
#include "log.h"
#include "menustate.h"
#include "config.h"
#include "clock.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_WindowEventManager.hpp"
#include "SFML_TextureManager.hpp"
#include "IMGuiManager.hpp"
#include "IMCursorManager.hpp"
#include "IMImageButton.hpp"
#include "IMTextButton.hpp"
#include "IMEdgeButton.hpp"

#include "stdio.h"
#include "math.h"
#include <sstream>
#include <fstream>
#include <list>
#include <set>

#define TURN_LENGTH 1000

#define MOUSE_DOWN_SELECT_TIME 150

using namespace sf;
using namespace std;

namespace sum {

//////////////////////////////////////////////////////////////////////
// Global app-state variables ---

View *level_view;
float view_rel_x_to_y;

int turn, turn_progress;
int pause_state = 0;
#define FULLY_PAUSED 15

//////////////////////////////////////////////////////////////////////
// Some definitions ---

const float c_min_zoom_x = 10;

//////////////////////////////////////////////////////////////////////
// Level Data ---

int level_init = 0;

bool between_turns = false;

int level_num = -1;
int level_dim_x = -1;
int level_dim_y = -1;
float level_fog_dim = 0;

Sprite **terrain_sprites;

BaseTerrain base_terrain;
Sprite *base_grass_sprite;
Sprite *base_mountain_sprite;
Sprite *base_underground_sprite;
// Grids - row-major order, size = dim_x X dim_y
Terrain* terrain_grid;
Unit** unit_grid;
// Lists
list<Unit*> unit_list;
list<Effect*> effect_list;

// Vision
Vision* vision_grid;
bool vision_enabled;
Vision* ai_vision_grid;

list<Unit*> listening_units;
Unit *player = NULL;

Unit *selected_unit;

//////////////////////////////////////////////////////////////////////
// UI Data ---

int left_mouse_down = 0;
int left_mouse_down_time = 0;

int right_mouse_down = 0;
int right_mouse_down_time = 0;

int key_shift_down = 0;

//////////////////////////////////////////////////////////////////////
// Utility ---

#define GRID_AT(GRID,X,Y) (GRID[((X) + ((Y) * level_dim_x))])

Vector2f coordsWindowToView( int window_x, int window_y )
{
   RenderWindow *r_window = SFML_GlobalRenderWindow::get();
   float view_x = (((float)window_x / (float)(r_window->getSize().x))* (float)(level_view->getSize().x)) 
                + ((level_view->getCenter().x) - (level_view->getSize().x / 2.0));
   float view_y = (((float)window_y / (float)(r_window->getSize().y))* (float)(level_view->getSize().y)) 
                + ((level_view->getCenter().y) - (level_view->getSize().y / 2.0));

   return Vector2f( view_x, view_y );
}

Vector2i coordsWindowToLevel( int window_x, int window_y )
{
   Vector2f view_coords = coordsWindowToView( window_x, window_y );

   int x = (view_coords.x < 0)?((int)view_coords.x - 1):(int)view_coords.x,
       y = (view_coords.y < 0)?((int)view_coords.y - 1):(int)view_coords.y;
   return Vector2i(x, y);
}

Vector2u coordsViewToWindow( float view_x, float view_y )
{
   RenderWindow *r_window = SFML_GlobalRenderWindow::get();
   int window_x = (int)(((view_x - ((level_view->getCenter().x) - (level_view->getSize().x / 2.0)))
                         / (level_view->getSize().x)) * (r_window->getSize().x));
   int window_y = (int)(((view_y - ((level_view->getCenter().y) - (level_view->getSize().y / 2.0)))
                         / (level_view->getSize().y)) * (r_window->getSize().y));
   return Vector2u( window_x, window_y );
}

void drawSquare( int x, int y, int size, Color line_color )
{
   RectangleShape rect;
   rect.setSize( Vector2f( size, size ) );
   rect.setPosition( x, y );
   rect.setFillColor( Color::Transparent );
   rect.setOutlineColor( line_color );
   rect.setOutlineThickness( 1.0 );
   SFML_GlobalRenderWindow::get()->draw( rect );
}

//////////////////////////////////////////////////////////////////////
// View management ---

int setView( float x_size, Vector2f center )
{
   float y_size = x_size * view_rel_x_to_y;

   float x_add = 0, y_add = 0;
   float x_base = center.x - (x_size / 2),
         y_base = center.y - (y_size / 2),
         x_top = center.x + (x_size / 2),
         y_top = center.y + (y_size / 2);

   // Don't need to check for: top AND bottom overlap -> failure
   if (x_base < -level_fog_dim)
      x_add = -(x_base + level_fog_dim);

   if (x_top > (level_dim_x + level_fog_dim))
      x_add = (level_dim_x + level_fog_dim) - x_top;

   if (y_base < -level_fog_dim)
      y_add = -(y_base + level_fog_dim);

   if (y_top > (level_dim_y + level_fog_dim))
      y_add = (level_dim_y + level_fog_dim) - y_top;

   center.x += x_add;
   center.y += y_add;

   level_fog_dim = (y_size / 4);
   level_view->setSize( x_size, y_size );
   level_view->setCenter( center.x, center.y );

   return 0;
}

// Shifts in terms of view coords
int shiftView( float x_shift, float y_shift )
{
   Vector2f old_center = level_view->getCenter(),
            old_size = level_view->getSize();
   Vector2f new_center (old_center.x + x_shift, old_center.y + y_shift);

   return setView( old_size.x, new_center );
}

// Positive is zoom in negative is zoom out
int zoomView( int ticks, Vector2f zoom_around )
{
   float zoom_val = pow(1.5, -ticks);
   Vector2f old_center = level_view->getCenter(),
            old_size = level_view->getSize();
   Vector2f new_center ((old_center.x + zoom_around.x) / 2.0, (old_center.y + zoom_around.y) / 2.0);
   Vector2f new_size (old_size*zoom_val);

   // Fit map
   if (new_size.x > (level_dim_x + (2*level_fog_dim))) {
      float x_fit = (level_dim_x + (2*level_fog_dim)) / new_size.x;
      new_size *= x_fit;
   }
   if (new_size.y > (level_dim_y + (2*level_fog_dim))) {
      float y_fit = (level_dim_y + (2*level_fog_dim)) / new_size.y;
      new_size *= y_fit;
   }

   // Zoom limits
   if (new_size.x < c_min_zoom_x) {
      float min_fit = c_min_zoom_x / new_size.x;
      new_size *= min_fit;
   }

   return setView( new_size.x, new_center );
}

//////////////////////////////////////////////////////////////////////
// Adding stuff ---

int removeUnit (list<Unit*>::iterator it)
{
   Unit *u = (*it);
   if (u) {
      unit_list.erase( it );

      if (GRID_AT(unit_grid, u->x_grid, u->y_grid) == u)
         GRID_AT(unit_grid, u->x_grid, u->y_grid) = NULL;

      return 0;
   }
   return -1;
}

int addPlayer()
{
   log( "Added Player" );
   if (player) {
      int cx = player->x_grid, cy = player->y_grid;
      if (GRID_AT(unit_grid, cx, cy) != NULL) {
         log("Can't add a unit where one already is");
         return -2;
      }

      // Otherwise, we should be good
      GRID_AT(unit_grid, cx, cy) = player;

      return 0;
   }
   return -1;
}

int addUnit( Unit *u )
{
   if (u) {
      int cx = u->x_grid, cy = u->y_grid;
      if (GRID_AT(unit_grid, cx, cy) != NULL) {
         log("Can't add a unit where one already is");
         return -2;
      }

      // Otherwise, we should be good
      GRID_AT(unit_grid, cx, cy) = u;
      unit_list.push_front( u );

      return 0;
   }
   return -1;
}

int moveUnit( Unit *u, int new_x, int new_y )
{
   if (u && between_turns && canMove( u->x_grid, u->y_grid, new_x, new_y)) {

      if (GRID_AT(unit_grid, u->x_grid, u->y_grid) == u)
         GRID_AT(unit_grid, u->x_grid, u->y_grid) = NULL;

      GRID_AT(unit_grid, new_x, new_y) = u;
      return 0;
   }
   return -1;
}

int removeEffect( Effect *p )
{
   if (p) {
      list<Effect*>::iterator it= find( effect_list.begin(), effect_list.end(), p );
      effect_list.erase( it );
      return 0;
   }
   return -1;
}

int addProjectile( Effect_Type t, int team, float x, float y, float speed, float range, Unit* target )
{
   Effect *p = genProjectile( t, team, x, y, speed, range, target );

   if (p) {
      effect_list.push_back(p);
      return 0;
   }
   return -1;
}

int addEffect( Effect_Type t, float dur, float x, float y )
{
   Effect *e = genEffect( t, dur, x, y );

   if (e) {
      effect_list.push_back(e);
      return 0;
   }
   return -1;
}

//////////////////////////////////////////////////////////////////////
// Vision ---

bool isVisible( int x, int y )
{
   return (GRID_AT(vision_grid,x,y) == VIS_VISIBLE);
}

bool blocksVision( int x, int y, int from_x, int from_y, Direction ew, Direction ns, int flags )
{
   Terrain t = GRID_AT(terrain_grid,x,y);

   if (t >= TER_ROCK_1 && t <= TER_TREE_LAST)
      return true;

   // Cliffs
   if ((t == CLIFF_SOUTH || t == CLIFF_SOUTH_EAST_EDGE || t == CLIFF_SOUTH_WEST_EDGE)
      && ns == NORTH && (y < from_y))
      return true;
   if ((t == CLIFF_NORTH || t == CLIFF_NORTH_EAST_EDGE || t == CLIFF_NORTH_WEST_EDGE)
      && ns == SOUTH && (y > from_y))
      return true;
   if ((t == CLIFF_EAST || t == CLIFF_EAST_NORTH_EDGE || t == CLIFF_EAST_SOUTH_EDGE)
      && ew == WEST && (x < from_x))
      return true;
   if ((t == CLIFF_WEST || t == CLIFF_WEST_SOUTH_EDGE || t == CLIFF_WEST_NORTH_EDGE)
      && ew == EAST && (x > from_x))
      return true;
   // Cliff corners
   if ((t == CLIFF_CORNER_SOUTHEAST_90 || t == CLIFF_CORNER_SOUTHEAST_270)
         && ns != SOUTH && ew != EAST)
      return true;
   if ((t == CLIFF_CORNER_SOUTHWEST_90 || t == CLIFF_CORNER_SOUTHWEST_270)
         && ns != SOUTH && ew != WEST)
      return true;
   if ((t == CLIFF_CORNER_NORTHWEST_90 || t == CLIFF_CORNER_NORTHWEST_270)
         && ns != NORTH && ew != WEST)
      return true;
   if ((t == CLIFF_CORNER_NORTHEAST_90 || t == CLIFF_CORNER_NORTHEAST_270)
         && ns != NORTH && ew != EAST)
      return true;


   // TODO: everything else
   return false;
}

int calculatePointVision( int start_x, int start_y, int end_x, int end_y, int flags, Vision *v_grid )
{
   int previous_x = start_x, previous_y = start_y;

   float dydx;
   if (start_x == end_x)
      dydx = 100000000;
   else
      dydx = ((float)(start_y - end_y)) / ((float)(start_x - end_x));

   int x, y, dx = 1, dy = 1;
   if (end_x < start_x) dx = -1;
   if (end_y < start_y) dy = -1;
   float calculator = 0;

   Direction dir1 = EAST, dir2 = SOUTH;
   if (dx == -1) dir1 = WEST;
   if (dy == -1) dir2 = NORTH;
   
   if (dydx >= 1 || dydx <= -1) { 
      // steep slope - go up/down 1 each move, calculate x
      float f_dx = ((float)dy)/dydx; 
      x = start_x;
      for (y = start_y; y != end_y; y += dy) {
         calculator += f_dx;
         if (calculator >= 1.05) {
            x += dx;
            calculator -= 1.0;
         }
         else if (calculator <= -1.05) {
            x += dx;
            calculator += 1.0;
         }

         // Check if this square obstructs vision
         if (y != start_y && blocksVision(x, y, previous_x, previous_y, dir1, dir2, flags)) {
            GRID_AT(v_grid,x,y) = VIS_VISIBLE;
            return -1; // Vision obstructed beyond here
         }

         // Otherwise, it's visible, move on
         GRID_AT(v_grid,x,y) = VIS_VISIBLE;
         previous_x = x;
         previous_y = y;
      }
   }
   else
   {
      // shallow slope, go left/right 1 each move, calculate y
      float f_dy = dydx*((float)dx);
      y = start_y;
      for (x = start_x; x != end_x; x += dx) {
         calculator += f_dy;
         if (calculator >= 1.05) {
            y += dy;
            calculator -= 1.0;
         }
         else if (calculator <= -1.05) {
            y += dy;
            calculator += 1.0;
         }

         // Check if this square obstructs vision
         if (x != start_x && blocksVision(x, y, previous_x, previous_y, dir1, dir2, flags)) {
            GRID_AT(v_grid,x,y) = VIS_VISIBLE;
            return -1; // Vision obstructed beyond here
         }

         // Otherwise, it's visible, move on
         GRID_AT(v_grid,x,y) = VIS_VISIBLE;
         previous_x = x;
         previous_y = y;
      }
   }
   GRID_AT(v_grid,end_x,end_y) = VIS_VISIBLE;
   return 0;
}

int calculateVerticalLineVision( int x, int start_y, int end_y, int flags, Vision *v_grid )
{
   int dy = 1;
   if (start_y > end_y) dy = -1;

   Direction dir = SOUTH; 
   if (dy == -1) dir = NORTH;

   for ( int y = start_y + dy; y != end_y + dy; y += dy) {
      // Check possible vision-obstructors
      if (blocksVision(x, y, x, y - dy, ALL_DIR, dir, flags)) {
         GRID_AT(v_grid,x,y) = VIS_VISIBLE;
         return -1; // Vision obstructed beyond here
      }

      // Otherwise, it's visible, move on
      GRID_AT(v_grid,x,y) = VIS_VISIBLE;
   }
   return 0;
}

int calculateHorizintalLineVision( int start_x, int end_x, int y, int flags, Vision *v_grid )
{
   int dx = 1;
   if (start_x > end_x) dx = -1;

   Direction dir = EAST;
   if (dx == -1) dir = WEST;

   for ( int x = start_x + dx; x != end_x + dx; x += dx) {
      // Check possible vision-obstructors
      if (blocksVision(x, y, x - dx, y, dir, ALL_DIR, flags)) {
         GRID_AT(v_grid,x,y) = VIS_VISIBLE;
         return -1; // Vision obstructed beyond here
      }

      // Otherwise, it's visible, move on
      GRID_AT(v_grid,x,y) = VIS_VISIBLE;
   }
   return 0;
}

int calculateLineVision( int start_x, int start_y, int end_x, int end_y, float range_squared, int flags, Vision *v_grid )
{
   // First things first, if it's out of range it's not visible
   int dx = (end_x - start_x), dy = (end_y - start_y);
   if (range_squared < ((dx*dx)+(dy*dy)))
      return -2;

   if (start_x == end_x)
      return calculateVerticalLineVision( start_x, start_y, end_y, flags, v_grid );
   if (start_y == end_y)
      return calculateHorizintalLineVision( start_x, end_x, start_y, flags, v_grid );
   
   int ret = calculatePointVision( start_x, start_y, end_x, end_y, flags, v_grid ); 
   if (ret == 0) // We must have seen it to get this result
      GRID_AT(v_grid,end_x,end_y) = VIS_VISIBLE;

   return ret;
}

int calculateUnitVision( Unit *unit, bool ai=false )
{
   int x, y;
   if (unit) {
      
      Vision *v_grid = vision_grid;
      if (unit->team != 0 || ai)
         v_grid = ai_vision_grid;

      float vision_range_squared = unit->vision_range * unit->vision_range;

      // Strategy: go around in boxes, skipping already visible squares,
      // and from each square make a line back to the middle,
      // and determine visibility along that line for all squares in the line
      // GOAL: be generous in what is visible, to avoid intuitive problems
      GRID_AT(v_grid,unit->x_grid,unit->y_grid) = VIS_VISIBLE;

      for (int radius = unit->vision_range; radius > 0; --radius)
      {
         if (unit->y_grid - radius >= 0)
            for (x = unit->x_grid - radius; x <= unit->x_grid + radius; ++x) {
               if (x < 0 || x >= level_dim_x) continue;
               if (GRID_AT(v_grid, x, unit->y_grid - radius) == VIS_VISIBLE) continue;
               calculateLineVision( unit->x_grid, unit->y_grid, x, (unit->y_grid - radius), vision_range_squared, 0, v_grid );
            }

         if (unit->y_grid + radius < level_dim_y)
            for (x = unit->x_grid - radius; x <= unit->x_grid + radius; ++x) {
               if (x < 0 || x >= level_dim_x) continue;
               if (GRID_AT(v_grid, x, unit->y_grid + radius) == VIS_VISIBLE) continue;
               calculateLineVision( unit->x_grid, unit->y_grid, x, (unit->y_grid + radius), vision_range_squared, 0, v_grid );
            }

         if (unit->x_grid - radius >= 0)
            for (y = unit->y_grid - radius; y <= unit->y_grid + radius; ++y) {
               if (y < 0 || y >= level_dim_y) continue;
               if (GRID_AT(v_grid, unit->x_grid - radius, y) == VIS_VISIBLE) continue;
               calculateLineVision( unit->x_grid, unit->y_grid, (unit->x_grid - radius), y, vision_range_squared, 0, v_grid );
            }

         if (unit->x_grid + radius < level_dim_x)
            for (y = unit->y_grid - radius; y <= unit->y_grid + radius; ++y) {
               if (y < 0 || y >= level_dim_y) continue;
               if (GRID_AT(v_grid, unit->x_grid + radius, y) == VIS_VISIBLE) continue;
               calculateLineVision( unit->x_grid, unit->y_grid, (unit->x_grid + radius), y, vision_range_squared, 0, v_grid );
            }

      }
   }

   return 0;
}

int calculateVision()
{
   if (false == vision_enabled) {
      int x, y;
      for (x = 0; x < level_dim_x; ++x) {
         for (y = 0; y < level_dim_y; ++y) {
            GRID_AT(vision_grid,x,y) = VIS_VISIBLE;
         }
      }

      return -1;
   }

   int x, y;
   for (x = 0; x < level_dim_x; ++x) {
      for (y = 0; y < level_dim_y; ++y) {
         if (GRID_AT(vision_grid,x,y) >= VIS_SEEN_BEFORE)
            GRID_AT(vision_grid,x,y) = VIS_SEEN_BEFORE;
         else
            GRID_AT(vision_grid,x,y) = VIS_NEVER_SEEN;
      }
   }

   for (x = 0; x < level_dim_x; ++x) {
      for (y = 0; y < level_dim_y; ++y) {
         if (GRID_AT(terrain_grid,x,y) >= TER_ATELIER) {
            // Give vision in 1 radius
            for (int i = x - 1; i <= x + 1 && i <= level_dim_x; ++i) {
               if (i < 0) continue;
               for (int j = y - 1; j <= y + 1 && j <= level_dim_y; ++j) {
                  if (j < 0) continue;
                  GRID_AT(vision_grid,i,j) = VIS_VISIBLE;
               }
            }
         }
      }
   }
   
   if (player)
      calculateUnitVision( player );
 
   for (list<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit->team == 0) // Allied unit
         calculateUnitVision( unit );
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////
// Targetting ---

int selectUnit( Vector2f coords )
{
   int cx = (int)coords.x,
       cy = (int)coords.y;

   std::stringstream ls;
   ls << "Selecting a unit at x=" << cx << ", y=" << cy;

   if (GRID_AT(vision_grid,cx,cy) != VIS_VISIBLE) {
      ls << " - coordinates not visible";
      log(ls.str());
      selected_unit = NULL;
      return -2;
   }

   Unit *u = GRID_AT(unit_grid, (int)coords.x, (int)coords.y);

   if (u) {
      ls << " - Selected: " << u->descriptor();
      log(ls.str());
      selected_unit = u;
      return 0;
   }

   ls << " - None found";
   log(ls.str());

   selected_unit = NULL;
   return -1;
}

Unit* getEnemy( int x, int y, float range, Direction dir, Unit *source, int selector)
{
   int team;
   if (source == NULL) team = -1;
   else {
      team = source->team;
      calculateUnitVision( source, true );
   }
   // First, get search box
   int int_range = (int) range + 1;
   int min_x, max_x, min_y, max_y;
   min_x = x - int_range;
   max_x = x + int_range;
   min_y = y - int_range;
   max_y = y + int_range;
   switch (dir) {
      case NORTH:
         max_y = y;
         break;
      case SOUTH:
         min_y = y;
         break;
      case WEST:
         max_x = x;
         break;
      case EAST:
         min_x = x;
         break;
      default:
         break;
   }
   if (min_x < 0) min_x = 0;
   if (max_x >= level_dim_x) max_x = level_dim_x - 1;
   if (min_y < 0) min_y = 0;
   if (max_y >= level_dim_y) max_y = level_dim_y - 1;

   float range_squared = range * range;

   Unit *result = NULL;
   float result_r_squared = 0;
   for (int j = min_y; j <= max_y; ++j) {
      for (int i = min_x; i <= max_x; ++i) {
         Unit *u = GRID_AT(unit_grid,i,j);
         if (u && u->team != team && u->alive) {
            // Can I see it?
            if ((team == -1) || (GRID_AT(ai_vision_grid,i,j) == VIS_VISIBLE)) {
               float u_x = u->x_grid - x, u_y = u->y_grid - y;
               float u_squared = (u_x * u_x) + (u_y * u_y);
               // Is it really in range?
               if (u_squared <= range_squared) {
                  // Compare based on selector
                  if (NULL == result) {
                     result = u;
                     result_r_squared = u_squared;
                  } else if (selector == SELECT_CLOSEST) {
                     if (u_squared < result_r_squared) {
                        result = u;
                        result_r_squared = u_squared;
                     }
                  } else if (selector == SELECT_FARTHEST) {
                     if (u_squared > result_r_squared) {
                        result = u;
                        result_r_squared = u_squared;
                     }
                  } else if (selector == SELECT_SMALLEST) {
                     if (u->health < result->health) {
                        result = u;
                        result_r_squared = u_squared;
                     }
                  } else if (selector == SELECT_BIGGEST) {
                     if (u->health > result->health) {
                        result = u;
                        result_r_squared = u_squared;
                     }
                  }
               }
            }
         }
      }
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
// Movement interface ---

bool canMove( int x, int y, int from_x, int from_y )
{
   if (x < 0 || y < 0 || x >= level_dim_x || y >= level_dim_y)
      return false;

   // TODO: building collision
   Terrain t = GRID_AT(terrain_grid,x,y);
   if ( (t >= CLIFF_SOUTH && t <= CLIFF_CORNER_NORTHWEST_270)
     || (t >= TER_ROCK_1 && t <= TER_ROCK_LAST))
      return false; // cliffs impassable

   return true;
}

set<Unit*> unit_collision_set;

Unit* unitIncoming( int to_x, int to_y, int from_x, int from_y )
{
   if (to_x < 0 || to_x >= level_dim_x || to_y < 0 || to_y >= level_dim_y
         || from_x < 0 || from_x >= level_dim_x || from_y < 0 || from_y >= level_dim_y)
      return NULL;

   Unit *u = GRID_AT(unit_grid,from_x,from_y);
   if (NULL != u && u->active == 1)
   {
      Order o = u->this_turn_order;
      if ((o.action == MOVE_FORWARD || o.action == MOVE_BACK)
            && (u->x_next == to_x && u->y_next == to_y))
         return u;
   }
   return NULL;
}

bool canMoveUnit( int x, int y, int from_x, int from_y, Unit *u )
{
   // Ignores terrain/buildings - do that stuff first
   //
   // This only figures out:
   //
   // 1) Is there a unit in the square you're going to?
   // 1.5) If so, will it move out in another direction?
   // 
   // 2) Are there any other units planning to move into this square?
   // 2.5) If so, are they bigger than you?

   // 1)
   Unit *u_next = GRID_AT(unit_grid,x,y);
   if (NULL != u_next) {
      // Check for loops
      set<Unit*>::iterator it = unit_collision_set.find( u_next );
      if (it != unit_collision_set.end()) { // found it
         // We have a collision loop. Nobody's moving today.
         log("Collision loop");
         return false;
      }
         
      // There's a unit. Let's see if it's moving?
      if (u_next->active != 1) // He aint moving
         return false;

      Order o = u_next->this_turn_order;
      if (o.action == MOVE_FORWARD || o.action == MOVE_BACK) {
         // Is he moving to THIS square?
         if (u_next->x_next == x && u_next->y_next == y) {
            // Well fuck it, we're both gonna bump
            return false;
         }
         // Move bitch get out the WAAAAY
         
         // Oh wait what if he can't move b/c of other units?
         unit_collision_set.insert( u_next );
         bool r = testUnitCanMove( u_next );
         unit_collision_set.erase( u_next );

         if (r == false) // He aint moving
            return false;

      }
      else
      {
         // He aint moving
         return false;
      }
   }
   
   // Still here? Then this location is available to move into,
   // but you might not be the only one trying

   // TODO
   // 2)
   Unit *u_biggest = u;

   u_next = unitIncoming( x, y, x - 1, y );
   if (u_next && u_next != u) {
      if (u_next->max_health > u_biggest->max_health) {
         u_biggest = u_next;
      } else {
         u_next->this_turn_order = Order(BUMP);
         if (u_next->max_health == u_biggest->max_health)
            u_biggest->this_turn_order = Order(BUMP);
      }
   }

   u_next = unitIncoming( x, y, x + 1, y );
   if (u_next && u_next != u) {
      if (u_next->max_health > u_biggest->max_health) {
         u_biggest = u_next;
      } else {
         u_next->this_turn_order = Order(BUMP);
         if (u_next->max_health == u_biggest->max_health)
            u_biggest->this_turn_order = Order(BUMP);
      }
   }

   u_next = unitIncoming( x, y, x, y - 1 );
   if (u_next && u_next != u) {
      if (u_next->max_health > u_biggest->max_health) {
         u_biggest = u_next;
      } else {
         u_next->this_turn_order = Order(BUMP);
         if (u_next->max_health == u_biggest->max_health)
            u_biggest->this_turn_order = Order(BUMP);
      }
   }

   u_next = unitIncoming( x, y, x, y + 1 );
   if (u_next && u_next != u) {
      if (u_next->max_health > u_biggest->max_health) {
         u_biggest = u_next;
      } else {
         u_next->this_turn_order = Order(BUMP);
         if (u_next->max_health == u_biggest->max_health)
            u_biggest->this_turn_order = Order(BUMP);
      }
   }

   if (u_biggest != u)
      return false;

   return true;
}

//////////////////////////////////////////////////////////////////////
// Player interface ---

// Choosing player orders

int order_prep_count = 1;
Order_Conditional order_prep_cond = TRUE;

#define ORDER_PREP_LAST_COUNT 0x1
#define ORDER_PREP_LAST_COND 0x2
#define ORDER_PREP_LAST_ORDER 0x4
#define ORDER_PREP_MAX_COUNT 10000
int order_prep_last_input = 0;

int playerAddCount( int digit )
{
   if (order_prep_last_input == ORDER_PREP_LAST_COUNT) {
      // Add another digit to the count
      order_prep_count = ((order_prep_count * 10) + digit) % ORDER_PREP_MAX_COUNT;
   }
   else
      order_prep_count = digit;

   order_prep_last_input = ORDER_PREP_LAST_COUNT;
   return 0;
}
      
int playerSetCount( int c )
{
   order_prep_count = c;
   order_prep_last_input = 0;

   return 0;
}

int selectConditionButton( Order_Conditional c );

int playerAddConditional( Order_Conditional c )
{
   switch (c)
   {
      case TRUE:
         order_prep_cond = TRUE;
         break;
      case ENEMY_ADJACENT:
      case ENEMY_AHEAD:
      case ENEMY_IN_RANGE:
      case ALLY_ADJACENT:
      case ALLY_AHEAD:
      case ALLY_IN_RANGE:
         if (order_prep_cond == c) order_prep_cond = (Order_Conditional)((int)c + 1);
         else if (order_prep_cond == (Order_Conditional)((int)c + 1)) order_prep_cond = TRUE;
         else order_prep_cond = c;
         break;
      default:
         order_prep_cond = c;
   }

   selectConditionButton( order_prep_cond );
   order_prep_last_input = ORDER_PREP_LAST_COND;
   return 0;
}

int playerAddOrder( Order_Action a )
{
   Order o;
   o.initOrder( a, order_prep_cond, order_prep_count );

   player->addOrder( o );

   order_prep_count = 1;
   order_prep_cond = TRUE;
   order_prep_last_input = ORDER_PREP_LAST_ORDER;
   selectConditionButton( TRUE );
   return 0;
}

// Summons
SummonMarker* summonMarker = NULL;

int startSummon( Order o )
{
   log( "Starting a summon" );
   int x = o.count,
       y = o.iteration;

   if (GRID_AT(unit_grid, x, y) != NULL) // Summon fails, obvi
      return -1;
   if (!canMove( x, y, x, y ))
      return -1;

   summonMarker = SummonMarker::get( x, y );
   GRID_AT(unit_grid, x, y) = summonMarker;

   return 0;
}

int completeSummon( Order o )
{
   int x = o.count,
       y = o.iteration;

   GRID_AT(unit_grid, x, y) = 0;
   summonMarker = NULL;

   Unit *u;

   if (o.action == SUMMON_MONSTER)
      u = new Monster( x, y, SOUTH );
   if (o.action == SUMMON_SOLDIER)
      u = new Soldier( x, y, SOUTH );
   if (o.action == SUMMON_WORM)
      u = new Worm( x, y, SOUTH );
   if (o.action == SUMMON_BIRD)
      u = new Bird( x, y, SOUTH );
   if (o.action == SUMMON_BUG)
      u = new Bug( x, y, SOUTH );

   if (NULL != u)
   {
      addUnit( u );
      effect_list.push_back( new StaticEffect( SE_SUMMON_CLOUD, 0.5, x + 0.5, y + 0.5 ) );
   }

   return 0;
}

int alert( Unit *u )
{
   listening_units.push_back(u);

   u->active = 0;
   u->clearOrders();

   return 0;
}

int unalert( Unit *u )
{
   // Yeah this does nothing

   return 0;
}

int alertUnits( Order o )
{
   if (NULL == player)
      return -1;

   // Un-alert current listeners
   for (list<Unit*>::iterator it=listening_units.begin(); it != listening_units.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit) {
         unalert(unit);
      }
   }
   listening_units.clear();

   // Check all areas that are in the Atelier
   for (int x = 0; x < level_dim_x; ++x) {
      for (int y = 0; y < level_dim_y; ++y) {
         if (GRID_AT(terrain_grid,x,y) >= TER_ATELIER) {
            Unit *u = GRID_AT(unit_grid,x,y);
            if (u == player) continue;

            if (u && u->team == 0) { // On my team

               if (o.action == PL_ALERT_ALL) {
                  alert(u);
               } else if (o.action == PL_ALERT_MONSTERS && u->type == MONSTER_T) {
                  alert(u);
               } else if (o.action == PL_ALERT_SOLDIERS && u->type == SOLDIER_T) {
                  alert(u);
               } else if (o.action == PL_ALERT_WORMS && u->type == WORM_T) {
                  alert(u);
               } else if (o.action == PL_ALERT_BIRDS && u->type == BIRD_T) {
                  alert(u);
               } else if (o.action == PL_ALERT_BUGS && u->type == BUG_T) {
                  alert(u);
               } else if (o.action == PL_ALERT_TEAM && u->team == o.count) {
                  alert(u);
               }

            }
         }
      }
   }

   return 0;
}
         
int activateUnits( Order o )
{
   for (int x = 0; x < level_dim_x; ++x) {
      for (int y = 0; y < level_dim_y; ++y) {
         if (GRID_AT(terrain_grid,x,y) >= TER_ATELIER) {
            Unit *u = GRID_AT(unit_grid,x,y);
            if (u == player) continue;

            if (u && u->team == 0) { // On my team

               if (o.action == PL_CMD_GO_ALL) {
                  u->activate();
               } else if (o.action == PL_CMD_GO_MONSTERS && u->type == MONSTER_T) {
                  u->activate();
               } else if (o.action == PL_CMD_GO_SOLDIERS && u->type == SOLDIER_T) {
                  u->activate();
               } else if (o.action == PL_CMD_GO_WORMS && u->type == WORM_T) {
                  u->activate();
               } else if (o.action == PL_CMD_GO_BIRDS && u->type == BIRD_T) {
                  u->activate();
               } else if (o.action == PL_CMD_GO_BUGS && u->type == BUG_T) {
                  u->activate();
               } else if (o.action == PL_CMD_GO_TEAM && u->team == o.count) {
                  u->activate();
               }
            
            }
         }
      }
   }

   return 0;
}

int activateAlert()
{
   log("Activating alert units");

   for (list<Unit*>::iterator it=listening_units.begin(); it != listening_units.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit) {
         unit->activate();
      }
   }
   
   return 0;
}

int broadcastOrder( Order o )
{
   log("Broadcasting order");
   if (NULL == player)
      return -1;

   for (list<Unit*>::iterator it=listening_units.begin(); it != listening_units.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit) {
         unit->addOrder( o );
      }
   }

   return 0;
}

int startPlayerCommand( Order o )
{
   if (NULL == player)
      return -1;

   switch( o.action ) {
      case PL_ALERT_ALL:
      case PL_ALERT_TEAM:
      case PL_ALERT_MONSTERS:
      case PL_ALERT_SOLDIERS:
      case PL_ALERT_WORMS:
      case PL_ALERT_BIRDS:
      case PL_ALERT_BUGS:
         alertUnits( o );
         break;
      case SUMMON_MONSTER:
      case SUMMON_SOLDIER:
      case SUMMON_WORM:
      case SUMMON_BIRD:
      case SUMMON_BUG:
         return startSummon( o );
         break;
      case PL_CAST_HEAL:
      case PL_CAST_LIGHTNING:
      case PL_CAST_TIMELOCK:
      case PL_CAST_SCRY:
      case PL_CAST_CONFUSION:
         log("startPlayerCommand: Spells not implemented");
         // TODO
         break;
      default:
         break;
   }

   return 0;
}

int completePlayerCommand( Order o )
{
   switch( o.action ) {
      case PL_CMD_GO_ALL:
      case PL_CMD_GO_MONSTERS:
      case PL_CMD_GO_SOLDIERS:
      case PL_CMD_GO_WORMS:
      case PL_CMD_GO_BIRDS:
      case PL_CMD_GO_BUGS:
      case PL_CMD_GO_TEAM:
         activateUnits( o );
         break;
      case PL_CMD_GO:
         activateAlert();
         break;
      case SUMMON_MONSTER:
      case SUMMON_SOLDIER:
      case SUMMON_WORM:
      case SUMMON_BIRD:
      case SUMMON_BUG:
         completeSummon( o );
         break;
      default:
         break;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////
// Loading ---

void initTextures()
{
   int i;
   SFML_TextureManager &t_manager = SFML_TextureManager::getSingleton();
   // Setup terrain
   terrain_sprites = new Sprite*[NUM_TERRAINS];
   for (i = 0; i < NUM_TERRAINS; ++i)
      terrain_sprites[i] = NULL;

   terrain_sprites[TER_NONE] = NULL;
   // Trees
   terrain_sprites[TER_TREE_1] = new Sprite( *(t_manager.getTexture( "BasicTree1.png" )));
   normalizeTo1x1( terrain_sprites[TER_TREE_1] );
   for (i = TER_TREE_2; i <= TER_TREE_LAST; ++i) { 
      terrain_sprites[i] = new Sprite( *(t_manager.getTexture( "BasicTree2.png" )));
      normalizeTo1x1( terrain_sprites[i] );
   }

   // Rocks
   for (i = TER_ROCK_1; i <= TER_ROCK_LAST; ++i) { 
      terrain_sprites[i] = new Sprite( *(t_manager.getTexture( "Rock1.png" )));
      normalizeTo1x1( terrain_sprites[i] );
   }

   // CLIFF
   // straight
   Vector2u dim = t_manager.getTexture( "CliffStraight.png" )->getSize();
   terrain_sprites[CLIFF_SOUTH] = new Sprite( *(t_manager.getTexture( "CliffStraight.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_SOUTH] );
   terrain_sprites[CLIFF_WEST] = new Sprite( *(t_manager.getTexture( "CliffStraight.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_WEST] );
   terrain_sprites[CLIFF_WEST]->setRotation( 90 );
   terrain_sprites[CLIFF_WEST]->setOrigin( 0, dim.y );
   terrain_sprites[CLIFF_NORTH] = new Sprite( *(t_manager.getTexture( "CliffStraight.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_NORTH] );
   terrain_sprites[CLIFF_NORTH]->setRotation( 180 );
   terrain_sprites[CLIFF_NORTH]->setOrigin( dim.x, dim.y );
   terrain_sprites[CLIFF_EAST] = new Sprite( *(t_manager.getTexture( "CliffStraight.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_EAST] );
   terrain_sprites[CLIFF_EAST]->setRotation( 270 );
   terrain_sprites[CLIFF_EAST]->setOrigin( dim.x, 0 );

   // edge
   terrain_sprites[CLIFF_SOUTH_WEST_EDGE] = new Sprite( *(t_manager.getTexture( "CliffEndLeft.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_SOUTH_WEST_EDGE] );
   terrain_sprites[CLIFF_SOUTH_EAST_EDGE] = new Sprite( *(t_manager.getTexture( "CliffEndRight.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_SOUTH_EAST_EDGE] );

   terrain_sprites[CLIFF_WEST_NORTH_EDGE] = new Sprite( *(t_manager.getTexture( "CliffEndLeft.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_WEST_NORTH_EDGE] );
   terrain_sprites[CLIFF_WEST_SOUTH_EDGE] = new Sprite( *(t_manager.getTexture( "CliffEndRight.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_WEST_SOUTH_EDGE] );
   terrain_sprites[CLIFF_WEST_NORTH_EDGE]->setRotation( 90 );
   terrain_sprites[CLIFF_WEST_NORTH_EDGE]->setOrigin( 0, dim.y );
   terrain_sprites[CLIFF_WEST_SOUTH_EDGE]->setRotation( 90 );
   terrain_sprites[CLIFF_WEST_SOUTH_EDGE]->setOrigin( 0, dim.y );

   terrain_sprites[CLIFF_NORTH_EAST_EDGE] = new Sprite( *(t_manager.getTexture( "CliffEndLeft.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_NORTH_EAST_EDGE] );
   terrain_sprites[CLIFF_NORTH_WEST_EDGE] = new Sprite( *(t_manager.getTexture( "CliffEndRight.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_NORTH_WEST_EDGE] );
   terrain_sprites[CLIFF_NORTH_EAST_EDGE]->setRotation( 180 );
   terrain_sprites[CLIFF_NORTH_EAST_EDGE]->setOrigin( dim.x, dim.y );
   terrain_sprites[CLIFF_NORTH_WEST_EDGE]->setRotation( 180 );
   terrain_sprites[CLIFF_NORTH_WEST_EDGE]->setOrigin( dim.x, dim.y );

   terrain_sprites[CLIFF_EAST_SOUTH_EDGE] = new Sprite( *(t_manager.getTexture( "CliffEndLeft.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_EAST_SOUTH_EDGE] );
   terrain_sprites[CLIFF_EAST_NORTH_EDGE] = new Sprite( *(t_manager.getTexture( "CliffEndRight.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_EAST_NORTH_EDGE] );
   terrain_sprites[CLIFF_EAST_SOUTH_EDGE]->setRotation( 270 );
   terrain_sprites[CLIFF_EAST_SOUTH_EDGE]->setOrigin( dim.x, 0 );
   terrain_sprites[CLIFF_EAST_NORTH_EDGE]->setRotation( 270 );
   terrain_sprites[CLIFF_EAST_NORTH_EDGE]->setOrigin( dim.x, 0 );

   // corner
   terrain_sprites[CLIFF_CORNER_SOUTHEAST_90] = new Sprite( *(t_manager.getTexture( "CliffCorner90.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_CORNER_SOUTHEAST_90] );
   terrain_sprites[CLIFF_CORNER_SOUTHWEST_90] = new Sprite( *(t_manager.getTexture( "CliffCorner90.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_CORNER_SOUTHWEST_90] );
   terrain_sprites[CLIFF_CORNER_SOUTHWEST_90]->setRotation( 90 );
   terrain_sprites[CLIFF_CORNER_SOUTHWEST_90]->setOrigin( 0, dim.y );
   terrain_sprites[CLIFF_CORNER_NORTHWEST_90] = new Sprite( *(t_manager.getTexture( "CliffCorner90.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_CORNER_NORTHWEST_90] );
   terrain_sprites[CLIFF_CORNER_NORTHWEST_90]->setRotation( 180 ); 
   terrain_sprites[CLIFF_CORNER_NORTHWEST_90]->setOrigin( dim.x, dim.y );
   terrain_sprites[CLIFF_CORNER_NORTHEAST_90] = new Sprite( *(t_manager.getTexture( "CliffCorner90.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_CORNER_NORTHEAST_90] );
   terrain_sprites[CLIFF_CORNER_NORTHEAST_90]->setRotation( 270 );
   terrain_sprites[CLIFF_CORNER_NORTHEAST_90]->setOrigin( dim.x, 0 );

   terrain_sprites[CLIFF_CORNER_SOUTHEAST_270] = new Sprite( *(t_manager.getTexture( "CliffCorner270.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_CORNER_SOUTHEAST_270] );
   terrain_sprites[CLIFF_CORNER_SOUTHWEST_270] = new Sprite( *(t_manager.getTexture( "CliffCorner270.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_CORNER_SOUTHWEST_270] );
   terrain_sprites[CLIFF_CORNER_SOUTHWEST_270]->setRotation( 90 );
   terrain_sprites[CLIFF_CORNER_SOUTHWEST_270]->setOrigin( 0, dim.y );
   terrain_sprites[CLIFF_CORNER_NORTHWEST_270] = new Sprite( *(t_manager.getTexture( "CliffCorner270.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_CORNER_NORTHWEST_270] );
   terrain_sprites[CLIFF_CORNER_NORTHWEST_270]->setRotation( 180 ); 
   terrain_sprites[CLIFF_CORNER_NORTHWEST_270]->setOrigin( dim.x, dim.y );
   terrain_sprites[CLIFF_CORNER_NORTHEAST_270] = new Sprite( *(t_manager.getTexture( "CliffCorner270.png" )));
   normalizeTo1x1( terrain_sprites[CLIFF_CORNER_NORTHEAST_270] );
   terrain_sprites[CLIFF_CORNER_NORTHEAST_270]->setRotation( 270 );
   terrain_sprites[CLIFF_CORNER_NORTHEAST_270]->setOrigin( dim.x, 0 );

   // Atelier

   terrain_sprites[TER_ATELIER] = new Sprite( *(t_manager.getTexture( "AtelierCenter.png" )));
   normalizeTo1x1( terrain_sprites[TER_ATELIER] );

   // edge
   dim = t_manager.getTexture( "AtelierEdge.png" )->getSize();
   terrain_sprites[TER_ATELIER_SOUTH_EDGE] = new Sprite( *(t_manager.getTexture( "AtelierEdge.png" )));
   normalizeTo1x1( terrain_sprites[TER_ATELIER_SOUTH_EDGE] );
   terrain_sprites[TER_ATELIER_WEST_EDGE] = new Sprite( *(t_manager.getTexture( "AtelierEdge.png" )));
   normalizeTo1x1( terrain_sprites[TER_ATELIER_WEST_EDGE] );
   terrain_sprites[TER_ATELIER_WEST_EDGE]->setRotation( 90 );
   terrain_sprites[TER_ATELIER_WEST_EDGE]->setOrigin( 0, dim.y );
   terrain_sprites[TER_ATELIER_NORTH_EDGE] = new Sprite( *(t_manager.getTexture( "AtelierEdge.png" )));
   normalizeTo1x1( terrain_sprites[TER_ATELIER_NORTH_EDGE] );
   terrain_sprites[TER_ATELIER_NORTH_EDGE]->setRotation( 180 );
   terrain_sprites[TER_ATELIER_NORTH_EDGE]->setOrigin( dim.x, dim.y );
   terrain_sprites[TER_ATELIER_EAST_EDGE] = new Sprite( *(t_manager.getTexture( "AtelierEdge.png" )));
   normalizeTo1x1( terrain_sprites[TER_ATELIER_EAST_EDGE] );
   terrain_sprites[TER_ATELIER_EAST_EDGE]->setRotation( 270 );
   terrain_sprites[TER_ATELIER_EAST_EDGE]->setOrigin( dim.x, 0 );
   
   // corner
   terrain_sprites[TER_ATELIER_CORNER_SOUTHWEST] = new Sprite( *(t_manager.getTexture( "AtelierCorner.png" )));
   normalizeTo1x1( terrain_sprites[TER_ATELIER_CORNER_SOUTHWEST] );
   terrain_sprites[TER_ATELIER_CORNER_NORTHWEST] = new Sprite( *(t_manager.getTexture( "AtelierCorner.png" )));
   normalizeTo1x1( terrain_sprites[TER_ATELIER_CORNER_NORTHWEST] );
   terrain_sprites[TER_ATELIER_CORNER_NORTHWEST]->setRotation( 90 );
   terrain_sprites[TER_ATELIER_CORNER_NORTHWEST]->setOrigin( 0, dim.y );
   terrain_sprites[TER_ATELIER_CORNER_NORTHEAST] = new Sprite( *(t_manager.getTexture( "AtelierCorner.png" )));
   normalizeTo1x1( terrain_sprites[TER_ATELIER_CORNER_NORTHEAST] );
   terrain_sprites[TER_ATELIER_CORNER_NORTHEAST]->setRotation( 180 ); 
   terrain_sprites[TER_ATELIER_CORNER_NORTHEAST]->setOrigin( dim.x, dim.y );
   terrain_sprites[TER_ATELIER_CORNER_SOUTHEAST] = new Sprite( *(t_manager.getTexture( "AtelierCorner.png" )));
   normalizeTo1x1( terrain_sprites[TER_ATELIER_CORNER_SOUTHEAST] );
   terrain_sprites[TER_ATELIER_CORNER_SOUTHEAST]->setRotation( 270 );
   terrain_sprites[TER_ATELIER_CORNER_SOUTHEAST]->setOrigin( dim.x, 0 );

   base_grass_sprite = new Sprite( *(t_manager.getTexture( "GreenGrass.png" )));
   base_mountain_sprite = new Sprite( *(t_manager.getTexture( "GrayRock.png" )));
   base_underground_sprite = new Sprite( *(t_manager.getTexture( "BrownRock.png" )));

}

void init()
{
   if (level_init) return;

   level_view = new View();
   view_rel_x_to_y = ((float)config::height()) / ((float)config::width());

   initTextures();

   level_init = true;
}

void clearGrids()
{
   if (terrain_grid)
      delete terrain_grid;
   if (unit_grid)
      delete unit_grid;
   if (vision_grid)
      delete vision_grid;
   if (ai_vision_grid)
      delete ai_vision_grid;
}

void clearAll()
{
   clearGrids();
   if (player) delete player;

   unit_list.clear();
   effect_list.clear();
   listening_units.clear();
}

int initGrids(int x, int y)
{
   clearGrids();

   level_dim_x = x;
   level_dim_y = y;
   level_fog_dim = 3;

   int dim = x * y;

   terrain_grid = new Terrain[dim];
   for (int i = 0; i < dim; ++i) terrain_grid[i] = TER_NONE;

   unit_grid = new Unit*[dim];
   for (int i = 0; i < dim; ++i) unit_grid[i] = NULL;

   vision_grid = new Vision[dim];
   for (int i = 0; i < dim; ++i) vision_grid[i] = VIS_NEVER_SEEN;

   ai_vision_grid = new Vision[dim];
   for (int i = 0; i < dim; ++i) ai_vision_grid[i] = VIS_NEVER_SEEN;

   log("initGrids succeeded");

   return 0;
}

//////////////////////////////////////////////////////////////////////
// Loading Level --- 

Terrain parseTerrain( char c )
{
   return (Terrain)(int)c;
}

// Mirrors parseTerrain
int terrainToCharData( Terrain t, char&c )
{
   c = (char)(int)t;
   return 0;
}

int parseAndCreateUnit( char c1, char c2, char x, char y )
{
   Direction facing;
   switch ((c2 & 0x3))
   {
      case 0x0:
         facing = EAST;
         break;
      case 0x1:
         facing = SOUTH;
         break;
      case 0x2:
         facing = WEST;
         break;
      case 0x3:
         facing = NORTH;
         break;
   }

   switch ((UnitType)(int)c1)
   {
      case PLAYER_T:
         if (NULL != player) return -1;
         player = Player::initPlayer( (int)x, (int)y, facing );
         addPlayer();
         break;
      case TARGETPRACTICE_T:
         addUnit( new TargetPractice( (int)x, (int)y, facing ) );
         break;
      default:
         break;
   }

   return 0;
}

// Mirrors parseAndCreateUnit
int unitToCharData( Unit *u, char &c1, char &c2, char &x, char &y )
{
   if (NULL == u)
      return -1;

   c2 = 0; // clear
   switch (u->facing) {
      case EAST:
         c2 |= 0x0;
         break;
      case SOUTH:
         c2 |= 0x1;
         break;
      case WEST:
         c2 |= 0x2;
         break;
      case NORTH:
         c2 |= 0x3;
         break;
      default:
         break;
   }

   c1 = (char)(int)u->type;

   x = u->x_grid;
   y = u->y_grid;

   return 0;
}

int createLevelFromFile( string filename )
{
   ifstream level_file( filename.c_str(), ios::in | ios::binary | ios::ate );
   ifstream::pos_type fileSize;
   char* fileContents;
   if(level_file.is_open())
   {
      fileSize = level_file.tellg();
      fileContents = new char[fileSize];
      level_file.seekg(0, ios::beg);
      if(!level_file.read(fileContents, fileSize))
      {
         log("Failed to read level file");
         level_file.close();
         return -1;
      }
      else
      {
         int x, y, i;
         int counter = 0;

         int dim_x = (int)fileContents[counter++];
         int dim_y = (int)fileContents[counter++];
         initGrids(dim_x,dim_y);

         float view_size_x = (float)(int)fileContents[counter++];
         float view_pos_x = (float)(int)fileContents[counter++];
         float view_pos_y = (float)(int)fileContents[counter++];
         setView( view_size_x, Vector2f( view_pos_x, view_pos_y ) );

         for (x = 0; x < dim_x; ++x) {
            for (y = 0; y < dim_y; ++y) {
               GRID_AT(terrain_grid,x,y) = parseTerrain( fileContents[counter] );
               counter++;
            }
         }

         int num_units = (int)fileContents[counter++];

         for (i = 0; i < num_units; ++i) {
            char c1 = fileContents[counter++],
                 c2 = fileContents[counter++],
                 ux = fileContents[counter++],
                 uy = fileContents[counter++];
            parseAndCreateUnit( c1, c2, ux, uy );
         }

      }
      level_file.close();
   }
   return 0;
}

// Mirrors createLevelFromFile
int writeLevelToFile( string filename )
{
   ofstream level_file( filename.c_str(), ios::out | ios::binary | ios::ate );
   int unitcount = unit_list.size() + 1; // 1 = player
   int writesize = 2 // dimensions
                 + 3 // view
                 // Could be some other metadata here
                 + (level_dim_x * level_dim_y) // terrain data
                 + 1 // count of units
                 + (4 * unitcount) // unit descriptions
                 // Should be something here for buildings
                 ;
   char *fileContents = new char[writesize];
   if(level_file.is_open())
   {
      int i = 0, x, y;
      // Dimensions
      fileContents[i++] = (char)level_dim_x;
      fileContents[i++] = (char)level_dim_y;

      // View
      fileContents[i++] = (char)(int)level_view->getSize().x;
      fileContents[i++] = (char)(int)level_view->getCenter().x;
      fileContents[i++] = (char)(int)level_view->getCenter().y;

      for (x = 0; x < level_dim_x; ++x) {
         for (y = 0; y < level_dim_y; ++y) {
            terrainToCharData( GRID_AT(terrain_grid,x,y), fileContents[i] );
            i++;
         }
      }

      // Units
      fileContents[i++] = unitcount + 1;

      if (player)
      {
         char c1, c2, ux, uy;
         unitToCharData( player, c1, c2, ux, uy );
         fileContents[i++] = c1;
         fileContents[i++] = c2;
         fileContents[i++] = ux;
         fileContents[i++] = uy;
      }

      for (list<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
      {
         Unit* unit = (*it);
         if (unit) {
            char c1, c2, ux, uy;
            unitToCharData( unit, c1, c2, ux, uy );
            fileContents[i++] = c1;
            fileContents[i++] = c2;
            fileContents[i++] = ux;
            fileContents[i++] = uy;
         }
      }

      level_file.write(fileContents, writesize);

      log("Wrote current level to file");
      delete fileContents;
      return 0;
   }

   log("Write level to file error: file failed to open");
   delete fileContents;
   return -1;
}

int loadLevel( int level_id )
{
   if (!level_init)
      init();

   // Currently only loads test level
   if (level_id == 0 || true)
   {
      base_terrain = BASE_TER_GRASS;
      if (createLevelFromFile( "res/testlevel.txt" ) == -1)
         return -1;

      //player->x_grid = 1;
      //player->y_grid = 4;
      addUnit( new RangedUnit( R_HUMAN_ARCHER_T, 2, 2, EAST, 2 ) );

      //writeLevelToFile( "res/testlevel.txt" );
   }

   menu_state = MENU_MAIN | MENU_PRI_INGAME;
   setLevelListener(true);

   vision_enabled = true;
   calculateVision();

   return 0;
}


//////////////////////////////////////////////////////////////////////
// Pause ---

void pause()
{
   pause_state = -1;
}

void unpause()
{
   pause_state = FULLY_PAUSED - 1;
}

void togglePause()
{
   if (pause_state == 0)
      pause();
   else if (pause_state == FULLY_PAUSED)
      unpause();
}

void drawPause()
{
   if (pause_state == 0) return;

   int transparency = (128 * abs(pause_state)) / FULLY_PAUSED;
   Color c( 192, 192, 192, transparency );
   RectangleShape r( Vector2f(config::width(), config::height()) );
   r.setFillColor( c );
   r.setPosition( 0, 0 );
   SFML_GlobalRenderWindow::get()->draw( r );
}

int updatePause( int dt )
{
   if (pause_state == FULLY_PAUSED)
      return 2;
   else if (pause_state < 0) {
      float factor = (float)(FULLY_PAUSED + pause_state) / (float)FULLY_PAUSED;
      int d_pause = dt / 10;
      dt *= factor;
      pause_state -= d_pause;
      if (pause_state <= -FULLY_PAUSED)
         pause_state = FULLY_PAUSED;
   }
   else if (pause_state > 0) {
      float factor = (float)(FULLY_PAUSED + pause_state) / (float)FULLY_PAUSED;
      int d_pause = dt / 10;
      dt *= factor;
      pause_state -= d_pause;
      if (pause_state <= 0)
         pause_state = 0;
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////
// Update ---

int prepareTurnAll()
{
   if (NULL != player)
      player->prepareTurn();

   for (list<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit) {
         unit->prepareTurn();
      }
   }

   return 0;
}

int startTurnAll( )
{
   if (NULL != player)
      player->startTurn();

   for (list<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit) {
         unit->startTurn();
      }
   }

   return 0;
}

int updateAll( int dt )
{
   float dtf = (float)dt / (float)TURN_LENGTH;
   int result;

   if (NULL != player)
      player->update( dtf );

   if (NULL != summonMarker)
      summonMarker->update( dtf );

   for (list<Unit*>::iterator it=unit_list.begin(); it != unit_list.end();)
   {
      Unit* unit = (*it);
      if (unit) {
         result = unit->update( dtf );
         if (result == 1) {
            // Kill the unit
            list<Unit*>::iterator to_delete = it;
            ++it;
            removeUnit( to_delete );
            continue;
         }
      }
      ++it;
   }
   for (list<Effect*>::iterator it=effect_list.begin(); it != effect_list.end(); ++it)
   {
      Effect* effect = (*it);
      if (effect) {
         result = effect->update( dtf );
         if (result == 1) {
            // Delete the effect
            delete effect;
            (*it) = NULL;
         }
      }
   }

   return 0;
}

int completeTurnAll( )
{
   if (NULL != player)
      player->completeTurn();

   for (list<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit) {
         unit->completeTurn();
      }
   }

   return 0;
}

int updateLevel( int dt )
{
   if (updatePause( dt ) == 2)
      return 2;

   int til_end = TURN_LENGTH - turn_progress;
   turn_progress += dt;

   if (turn_progress > TURN_LENGTH) {
      turn_progress -= TURN_LENGTH;

      updateAll( til_end );

      between_turns = true;
      completeTurnAll();

      turn++;
      calculateVision();

      prepareTurnAll();
      startTurnAll();
      between_turns = false;

      updateAll( turn_progress );
   } else {
      updateAll( dt );
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////
// GUI ---

//////////////////////////////////////////////////////////////////////
// Right Click Menu ---

int cast_menu_x_grid, cast_menu_y_grid; // the affected grid square
int cast_menu_grid_left, cast_menu_grid_top, cast_menu_grid_size;

int cast_menu_box = 1; // 0 not expanded, 1 above, 2 right, 3 below, 4 left
Vector2u cast_menu_top_left, cast_menu_size;
int cast_menu_button_size;

// Deprecated
int cast_menu_top_x_left, cast_menu_top_y_bottom; // Top box
int cast_menu_bottom_x_left, cast_menu_bottom_y_top; // Bottom box
float cast_menu_button_size_x, cast_menu_button_size_y;

int cast_menu_selection = 0; // 1 == closest to source

bool cast_menu_open = false;
bool cast_menu_solid = false;
bool cast_menu_summons_allowed = true;

void initRightClickMenu()
{
   cast_menu_open = false;
   cast_menu_solid = false;
}

int castMenuCreate( Vector2i grid )
{
   if (grid.x < 0 || grid.x >= level_dim_x || grid.y < 0 || grid.y >= level_dim_y)
      return -1;
   
   // TODO: do something so that the menu is always visible

   cast_menu_x_grid = grid.x;
   cast_menu_y_grid = grid.y;

   cast_menu_top_left = coordsViewToWindow( grid.x, grid.y );
   Vector2u bottom_right = coordsViewToWindow( grid.x + 1, grid.y + 1 );
   cast_menu_grid_size = cast_menu_size.x = cast_menu_size.y = bottom_right.y - cast_menu_top_left.y;

   cast_menu_box = 0;
   cast_menu_grid_left = cast_menu_top_left.x;
   cast_menu_grid_top = cast_menu_top_left.y;

   cast_menu_selection = 0;

   // Calculate how big to make the buttons
   float x_dim = config::width(), y_dim = config::height();
   cast_menu_button_size_x = x_dim / 6;
   cast_menu_button_size_y = y_dim / 30;
   cast_menu_button_size = x_dim / 20;

   cast_menu_open = true;
   cast_menu_solid = false;

   if (GRID_AT(terrain_grid,grid.x,grid.y) >= TER_ATELIER)
      cast_menu_summons_allowed = true;
   else
      cast_menu_summons_allowed = false;

   log("Right click menu created");

   return 0;
}

int castMenuSelect( unsigned int x, unsigned int y )
{
   if (cast_menu_box == 0)
   {
      if (x < cast_menu_top_left.x)
         cast_menu_box = 4;
      else if (x > cast_menu_top_left.x + cast_menu_size.x)
         cast_menu_box = 2;
      else if (y < cast_menu_top_left.y && cast_menu_summons_allowed)
         cast_menu_box = 1;
      else if (y > cast_menu_top_left.y + cast_menu_size.y && cast_menu_summons_allowed)
         cast_menu_box = 3;
   }

   if (cast_menu_box == 1 || cast_menu_box == 3) // Vertical - summons
   {
      int dy;
      if (cast_menu_box == 1)
         dy = cast_menu_top_left.y - y;
      else
         dy = y - (cast_menu_top_left.y + cast_menu_size.y);

      if (dy > (5*cast_menu_button_size))
         cast_menu_selection = 0;
      else if (dy < 0) {
         cast_menu_selection = 0;
         cast_menu_box = 0;
      }
      else
         cast_menu_selection = (dy / cast_menu_button_size) + 1;
      /*
      if (x < cast_menu_top_x_left || 
          x > (cast_menu_top_x_left + cast_menu_button_size_x) ||
          !( (y <= cast_menu_top_y_bottom && 
              y >= cast_menu_top_y_bottom - (cast_menu_button_size_y * 5))
             ||
             (y >= cast_menu_bottom_y_top && 
              y <= cast_menu_bottom_y_top + (cast_menu_button_size_y * 5))
           )
         )
      {
         // Not in a selection area
         cast_menu_selection = 0;
      } 
      else if (y <= cast_menu_top_y_bottom && 
          y >= cast_menu_top_y_bottom - (cast_menu_button_size_y * 5))
      {
         // Top selection area
         cast_menu_selection = ((int) ((y - cast_menu_top_y_bottom) / cast_menu_button_size_y)) - 1;
      }
      else
      {
         // Bottom selection area
         cast_menu_selection = (int) ((y - cast_menu_bottom_y_top) / cast_menu_button_size_y);
      }
      */
   }

   if (cast_menu_box == 2 || cast_menu_box == 4) // Horizontal - spells
   {
      int dx;
      if (cast_menu_box == 4)
         dx = cast_menu_top_left.x - x;
      else
         dx = x - (cast_menu_top_left.x + cast_menu_size.x);

      if (dx > (5*cast_menu_button_size))
         cast_menu_selection = 0;
      else if (dx < 0) {
         cast_menu_selection = 0;
         cast_menu_box = 0;
      }
      else
         cast_menu_selection = (dx / cast_menu_button_size) + 1;
   }


   return cast_menu_selection;
}

void castMenuSolidify()
{ 
   cast_menu_solid = true;
}

void castMenuClose()
{
   cast_menu_open = false;
}

void castMenuChoose()
{
   Order o( SKIP, TRUE, cast_menu_x_grid );
   o.iteration = cast_menu_y_grid;

   if (cast_menu_box == 1 || cast_menu_box == 3)
   {
      switch (cast_menu_selection) {
         case 1:
            o.action = SUMMON_MONSTER;
            player->addOrder( o ); 
            break;
         case 2:
            o.action = SUMMON_SOLDIER;
            player->addOrder( o ); 
            break;
         case 3:
            o.action = SUMMON_WORM;
            player->addOrder( o ); 
            break;
         case 4:
            o.action = SUMMON_BIRD;
            player->addOrder( o ); 
            break;
         case 5:
            o.action = SUMMON_BUG;
            player->addOrder( o ); 
            break;
      }
   }
   else
   {
      switch (cast_menu_selection) {
         case 1:
            o.action = PL_CAST_HEAL;
            player->addOrder( o ); 
            break;
         case 2:
            o.action = PL_CAST_LIGHTNING;
            player->addOrder( o ); 
            break;
         case 3:
            o.action = PL_CAST_TIMELOCK;
            player->addOrder( o ); 
            break;
         case 4:
            o.action = PL_CAST_SCRY;
            player->addOrder( o ); 
            break;
         case 5:
            o.action = PL_CAST_CONFUSION;
            player->addOrder( o ); 
            break;
      }

   }

   castMenuClose();
}

void drawRightClickMenu()
{
   if (cast_menu_open == false) return;
   RenderWindow *gui_window = SFML_GlobalRenderWindow::get();

   if (cast_menu_box == 0)
   {
      // Draw 'summon' vertically and 'cast' horizontally
      Text text;
      text.setFont( *menu_font );
      text.setColor( Color::Black );
      text.setCharacterSize( 16 );

      if (cast_menu_summons_allowed) {
         text.setString( String("Summon") );
         text.setPosition( cast_menu_top_left.x, cast_menu_top_left.y );
         text.setRotation( 270 );
         gui_window->draw( text );

         text.setPosition( cast_menu_top_left.x + cast_menu_size.x, cast_menu_top_left.y + cast_menu_size.y );
         text.setRotation( 90 );
         gui_window->draw( text );
      }

      text.setString( String("Cast") );
      text.setPosition( cast_menu_top_left.x, cast_menu_top_left.y + cast_menu_size.y );
      text.setRotation( 180 );
      gui_window->draw( text );

      text.setPosition( cast_menu_top_left.x + cast_menu_size.x, cast_menu_top_left.y );
      text.setRotation( 0 );
      gui_window->draw( text );

   }
   else if (cast_menu_box == 1 || cast_menu_box == 3)
   {
      int x, y, dy, dim;
      dim = cast_menu_button_size;
      x = cast_menu_grid_left + (cast_menu_grid_size / 2) - (dim / 2);
      if (cast_menu_box == 1) {
         y = cast_menu_grid_top - dim;
         dy = -dim;
      } else {
         y = cast_menu_grid_top + cast_menu_grid_size;
         dy = dim;
      }

      drawOrder( Order( SUMMON_MONSTER ), x, y, dim );
      if (cast_menu_selection == 1) drawSquare( x, y, dim, Color::White );
      y += dy;
      drawOrder( Order( SUMMON_SOLDIER ), x, y, dim );
      if (cast_menu_selection == 2) drawSquare( x, y, dim, Color::White );
      y += dy;
      drawOrder( Order( SUMMON_WORM ), x, y, dim );
      if (cast_menu_selection == 3) drawSquare( x, y, dim, Color::White );
      y += dy;
      drawOrder( Order( SUMMON_BIRD ), x, y, dim );
      if (cast_menu_selection == 4) drawSquare( x, y, dim, Color::White );
      y += dy;
      drawOrder( Order( SUMMON_BUG ), x, y, dim );
      if (cast_menu_selection == 5) drawSquare( x, y, dim, Color::White );

   }
   else if (cast_menu_box == 2 || cast_menu_box == 4)
   {
      int x, y, dx, dim;
      dim = cast_menu_button_size;
      y = cast_menu_grid_top + (cast_menu_grid_size / 2) - (dim / 2);
      if (cast_menu_box == 4) {
         x = cast_menu_grid_left - dim;
         dx = -dim;
      } else {
         x = cast_menu_grid_left + cast_menu_grid_size;
         dx = dim;
      }

      drawOrder( Order( PL_CAST_HEAL ), x, y, dim );
      if (cast_menu_selection == 1) drawSquare( x, y, dim, Color::White );
      x += dx;
      drawOrder( Order( PL_CAST_LIGHTNING ), x, y, dim );
      if (cast_menu_selection == 2) drawSquare( x, y, dim, Color::White );
      x += dx;
      drawOrder( Order( PL_CAST_TIMELOCK ), x, y, dim );
      if (cast_menu_selection == 3) drawSquare( x, y, dim, Color::White );
      x += dx;
      drawOrder( Order( PL_CAST_SCRY ), x, y, dim );
      if (cast_menu_selection == 4) drawSquare( x, y, dim, Color::White );
      x += dx;
      drawOrder( Order( PL_CAST_CONFUSION ), x, y, dim );
      if (cast_menu_selection == 5) drawSquare( x, y, dim, Color::White );

   }

   drawSquare( cast_menu_top_left.x, cast_menu_top_left.y, cast_menu_size.x, Color::White );
}

//////////////////////////////////////////////////////////////////////
// Order Buttons ---

int selection_box_width = 240, selection_box_height = 50;

bool init_level_gui = false;
IMImageButton *b_con_enemy_adjacent,
              *b_con_enemy_ahead,
              *b_con_enemy_in_range,
              *b_con_ally_adjacent,
              *b_con_ally_ahead,
              *b_con_ally_in_range,
              *b_con_clear,
              *b_o_move_forward,
              *b_o_move_backward,
              *b_o_turn_north,
              *b_o_turn_east,
              *b_o_turn_south,
              *b_o_turn_west,
              *b_o_turn_nearest_enemy,
              *b_o_attack_smallest,
              *b_o_attack_biggest,
              *b_o_attack_closest,
              *b_o_attack_farthest,
              *b_o_start_block,
              *b_o_end_block,
              *b_o_repeat,
              *b_o_wait,
              *b_pl_alert_all,
              *b_pl_cmd_go,
              *b_pl_cmd_go_all,
              *b_pl_alert_monster,
              *b_pl_cmd_go_monster,
              *b_o_monster_guard,
              *b_o_monster_burst,
              *b_pl_alert_soldier,
              *b_pl_cmd_go_soldier,
              *b_o_soldier_switch_axe,
              *b_o_soldier_switch_spear,
              *b_o_soldier_switch_bow,
              *b_pl_alert_worm,
              *b_pl_cmd_go_worm,
              *b_o_worm_sprint,
              *b_o_worm_trail_on,
              *b_o_worm_trail_off,
              *b_pl_alert_bird,
              *b_pl_cmd_go_bird,
              *b_o_bird_fly,
              *b_cmd_bird_shout,
              *b_cmd_bird_quiet,
              *b_pl_alert_bug,
              *b_pl_cmd_go_bug,
              *b_o_bug_meditate,
              *b_o_bug_fireball,
              *b_o_bug_sunder,
              *b_o_bug_heal,
              *b_o_bug_open_wormhole,
              *b_o_bug_close_wormhole,
              *b_monster_image,
              *b_soldier_image,
              *b_worm_image,
              *b_bird_image,
              *b_bug_image;
IMTextButton *b_count_1,
             *b_count_2,
             *b_count_3,
             *b_count_4,
             *b_count_5,
             *b_count_6,
             *b_count_7,
             *b_count_8,
             *b_count_9,
             *b_count_0,
             *b_count_infinite,
             *b_count_reset;
Text *count_text;
string s_1 = "1", s_2 = "2", s_3 = "3", s_4 = "4", s_5 = "5", s_6 = "6", s_7 = "7", s_8 = "8", s_9 = "9", s_0 = "0", s_infinite = "inf", s_reset = "X";
// Gui Boxes
IMEdgeButton *b_numpad_area,
             *b_conditional_area,
             *b_movement_area,
             *b_attack_area,
             *b_control_area,
             *b_pl_cmd_area,
             *b_monster_area,
             *b_soldier_area,
             *b_worm_area,
             *b_bird_area,
             *b_bug_area;

Vector2f b_turn_north_count_pos,
         b_turn_east_count_pos,
         b_turn_south_count_pos,
         b_turn_west_count_pos,
         b_turn_nearest_enemy_count_pos,
         b_guard_count_pos,
         b_switch_axe_count_pos,
         b_switch_spear_count_pos,
         b_switch_bow_count_pos,
         b_trail_on_count_pos,
         b_trail_off_count_pos,
         b_fly_count_pos,
         b_meditate_count_pos;

const int border = 3;
const int spacer = 2;
int button_size = 0;

bool isMouseOverGui( int x, int y )
{
   int width = config::width(),
       height = config::height();

   if (y > (height - (border + (spacer * 4) + (button_size * 3))))
      return true; // All button areas are this tall

   if (y > (height - ((border * 2) + (spacer * 7) + (button_size * 5)))
         && x < (border + (spacer * 4) + (button_size * 3)))
      return true; // Player command area is taller

   if (selected_unit && (y < selection_box_height) && (x > width - selection_box_width))
      return true; // Over the selected unit area

   return false;
}

void fitGui_Level()
{
   int width = config::width(),
       height = config::height();

   float button_size_f = ((float)width - (32.0 * spacer) - (9.0 * border)) / 22.0;
   button_size = (int) button_size_f;
   float adjustment = 0, adjust_ratio = (button_size_f - button_size);

   int text_size = floor( button_size / 2);

   int sec_start_numpad = 0,
       sec_start_conditionals = sec_start_numpad + border + (spacer * 4) + (button_size * 3),
       sec_start_control = sec_start_conditionals + border + (spacer * 3) + (button_size * 2),
       sec_start_movement = sec_start_control + border + (spacer * 2) + (button_size * 1),
       sec_start_attack = sec_start_movement + border + (spacer * 4) + (button_size * 3),
       sec_start_monster = sec_start_attack + border + (spacer * 3) + (button_size * 2),
       sec_start_soldier = sec_start_monster + border + (spacer * 3) + (button_size * 2),
       sec_start_worm = sec_start_soldier + border + (spacer * 3) + (button_size * 2),
       sec_start_bird = sec_start_worm + border + (spacer * 3) + (button_size * 2),
       sec_start_bug = sec_start_bird + border + (spacer * 3) + (button_size * 2);

   adjustment += (3 * adjust_ratio);
   sec_start_conditionals += (int) adjustment;
   adjustment += (1 * adjust_ratio);
   sec_start_control += (int) adjustment;
   adjustment += (1 * adjust_ratio);
   sec_start_movement += (int) adjustment;
   adjustment += (3 * adjust_ratio);
   sec_start_attack += (int) adjustment;
   adjustment += (2 * adjust_ratio);
   sec_start_monster += (int) adjustment;
   adjustment += (2 * adjust_ratio);
   sec_start_soldier += (int) adjustment;
   adjustment += (2 * adjust_ratio);
   sec_start_worm += (int) adjustment;
   adjustment += (2 * adjust_ratio);
   sec_start_bird += (int) adjustment;
   adjustment += (2 * adjust_ratio);
   sec_start_bug += (int) adjustment;

   // Numpad

   b_numpad_area->setSize( (border * 3) + (spacer * 4) + (button_size * 3),
                           (border * 2) + (spacer * 5) + (button_size * 4) );
   b_numpad_area->setPosition( sec_start_numpad - (2 * border),
                               height - (border + (spacer * 5) + (button_size * 4)) );
   
   b_count_0->setSize( button_size, button_size );
   b_count_0->setPosition( sec_start_numpad + (spacer * 2) + button_size,
                           height - (spacer + button_size));
   b_count_0->setTextSize( text_size );
   b_count_0->centerText();
   
   b_count_1->setSize( button_size, button_size );
   b_count_1->setPosition( sec_start_numpad + spacer,
                           height - ((spacer * 4) + (button_size * 4)) );
   b_count_1->setTextSize( text_size );
   b_count_1->centerText();

   b_count_2->setSize( button_size, button_size );
   b_count_2->setPosition( sec_start_numpad + (spacer * 2) + button_size,
                           height - ((spacer * 4) + (button_size * 4)) );
   b_count_2->setTextSize( text_size );
   b_count_2->centerText();

   b_count_3->setSize( button_size, button_size );
   b_count_3->setPosition( sec_start_numpad + (spacer * 3) + (button_size * 2),
                           height - ((spacer * 4) + (button_size * 4)) );
   b_count_3->setTextSize( text_size );
   b_count_3->centerText();

   b_count_4->setSize( button_size, button_size );
   b_count_4->setPosition( sec_start_numpad + spacer,
                           height - ((spacer * 3) + (button_size * 3)) );
   b_count_4->setTextSize( text_size );
   b_count_4->centerText();

   b_count_5->setSize( button_size, button_size );
   b_count_5->setPosition( sec_start_numpad + (spacer * 2) + button_size,
                           height - ((spacer * 3) + (button_size * 3)) );
   b_count_5->setTextSize( text_size );
   b_count_5->centerText();

   b_count_6->setSize( button_size, button_size );
   b_count_6->setPosition( sec_start_numpad + (spacer * 3) + (button_size * 2),
                           height - ((spacer * 3) + (button_size * 3)) );
   b_count_6->setTextSize( text_size );
   b_count_6->centerText();

   b_count_7->setSize( button_size, button_size );
   b_count_7->setPosition( sec_start_numpad + spacer,
                           height - ((spacer * 2) + (button_size * 2)) );
   b_count_7->setTextSize( text_size );
   b_count_7->centerText();

   b_count_8->setSize( button_size, button_size );
   b_count_8->setPosition( sec_start_numpad + (spacer * 2) + button_size,
                           height - ((spacer * 2) + (button_size * 2)) );
   b_count_8->setTextSize( text_size );
   b_count_8->centerText();

   b_count_9->setSize( button_size, button_size );
   b_count_9->setPosition( sec_start_numpad + (spacer * 3) + (button_size * 2),
                           height - ((spacer * 2) + (button_size * 2)) );
   b_count_9->setTextSize( text_size );
   b_count_9->centerText();

   b_count_infinite->setSize( button_size, button_size );
   b_count_infinite->setPosition( sec_start_numpad + (spacer * 3) + (button_size * 2),
                                  height - (spacer + button_size) );
   b_count_infinite->setTextSize( text_size );
   b_count_infinite->centerText();

   b_count_reset->setSize( button_size, button_size );
   b_count_reset->setPosition( sec_start_numpad + spacer,
                               height - (spacer + button_size) );
   b_count_reset->setTextSize( text_size );
   b_count_reset->centerText();

   count_text->setPosition( sec_start_conditionals + spacer,
                            height - (border + (button_size * 4) + (spacer * 6) + text_size));
   count_text->setCharacterSize( text_size );

   // Player commands

   b_pl_cmd_area->setSize( (border * 3) + (spacer * 4) + (button_size * 3),
                           (border * 2) + (spacer * 3) + (button_size * 1));
   b_pl_cmd_area->setPosition( sec_start_numpad - (2 * border),
                               height - ((border * 2) + (spacer * 7) + (button_size * 5)));

   b_pl_alert_all->setSize( button_size, button_size );
   b_pl_alert_all->setPosition( sec_start_numpad + spacer,
                                height - ((border * 1) + (spacer * 6) + (button_size * 5)));

   b_pl_cmd_go->setSize( button_size, button_size );
   b_pl_cmd_go->setPosition( sec_start_numpad + (spacer * 2) + button_size,
                             height - ((border * 1) + (spacer * 6) + (button_size * 5)));

   b_pl_cmd_go_all->setSize( button_size, button_size );
   b_pl_cmd_go_all->setPosition( sec_start_numpad + (spacer * 3) + (button_size * 2),
                                 height - ((border * 1) + (spacer * 6) + (button_size * 5)));

   // Conditionals

   b_conditional_area->setSize( (border * 3) + (spacer * 3) + (button_size * 2),
                                (border * 2) + (spacer * 5) + (button_size * 4));
   b_conditional_area->setPosition( sec_start_conditionals - (2 * border),
                                    height - (border + (spacer * 5) + (button_size * 4)));

   b_con_enemy_adjacent->setSize( button_size, button_size );
   b_con_enemy_adjacent->setImageSize( button_size, button_size );
   b_con_enemy_adjacent->setPosition( sec_start_conditionals + spacer,
                                      height - ((spacer * 3) + (button_size * 3)));

   b_con_enemy_ahead->setSize( button_size, button_size );
   b_con_enemy_ahead->setImageSize( button_size, button_size );
   b_con_enemy_ahead->setPosition( sec_start_conditionals + spacer,
                                      height - ((spacer * 2) + (button_size * 2)));

   b_con_enemy_in_range->setSize( button_size, button_size );
   b_con_enemy_in_range->setImageSize( button_size, button_size );
   b_con_enemy_in_range->setPosition( sec_start_conditionals + spacer,
                                      height - ((spacer * 1) + (button_size * 1)));

   b_con_ally_adjacent->setSize( button_size, button_size );
   b_con_ally_adjacent->setImageSize( button_size, button_size );
   b_con_ally_adjacent->setPosition( sec_start_conditionals + (spacer * 2) + button_size,
                                      height - ((spacer * 3) + (button_size * 3)));

   b_con_ally_ahead->setSize( button_size, button_size );
   b_con_ally_ahead->setImageSize( button_size, button_size );
   b_con_ally_ahead->setPosition( sec_start_conditionals + (spacer * 2) + button_size,
                                      height - ((spacer * 2) + (button_size * 2)));

   b_con_ally_in_range->setSize( button_size, button_size );
   b_con_ally_in_range->setImageSize( button_size, button_size );
   b_con_ally_in_range->setPosition( sec_start_conditionals + (spacer * 2) + button_size,
                                      height - ((spacer * 1) + (button_size * 1)));

   b_con_clear->setSize( button_size, button_size );
   b_con_clear->setImageSize( button_size, button_size );
   b_con_clear->setPosition( sec_start_conditionals + spacer,
                                      height - ((spacer * 4) + (button_size * 4)));

   // Control

   b_control_area->setSize( (border * 3) + (spacer * 2) + (button_size * 1),
                            (border * 2) + (spacer * 4) + (button_size * 3));
   b_control_area->setPosition( sec_start_control - (2 * border),
                                height - (border + (spacer * 4) + (button_size * 3)));

   b_o_start_block->setSize( button_size, button_size );
   b_o_start_block->setImageSize( button_size, button_size );
   b_o_start_block->setPosition( sec_start_control + spacer,
                                 height - ((spacer * 3) + (button_size * 3)));

   b_o_end_block->setSize( button_size, button_size );
   b_o_end_block->setImageSize( button_size, button_size );
   b_o_end_block->setPosition( sec_start_control + spacer,
                               height - ((spacer * 2) + (button_size * 2)));

   b_o_repeat->setSize( button_size, button_size );
   b_o_repeat->setImageSize( button_size, button_size );
   b_o_repeat->setPosition( sec_start_control + spacer,
                            height - ((spacer * 1) + (button_size * 1)));

   // Movement

   b_movement_area->setSize( (border * 3) + (spacer * 4) + (button_size * 3),
                             (border * 2) + (spacer * 4) + (button_size * 3));
   b_movement_area->setPosition( sec_start_movement - (2 * border),
                                 height - (border + (spacer * 4) + (button_size * 3)));

   b_o_move_forward->setSize( button_size, button_size );
   b_o_move_forward->setImageSize( button_size, button_size );
   b_o_move_forward->setPosition( sec_start_movement + (spacer * 3) + (button_size * 2),
                                  height - ((spacer * 3) + (button_size * 3)));

   b_o_move_backward->setSize( button_size, button_size );
   b_o_move_backward->setImageSize( button_size, button_size );
   b_o_move_backward->setPosition( sec_start_movement + spacer,
                                   height - ((spacer * 3) + (button_size * 3)));

   b_o_wait->setSize( button_size, button_size );
   b_o_wait->setImageSize( button_size, button_size );
   b_o_wait->setPosition( sec_start_movement + (spacer * 2) + button_size,
                          height - ((spacer * 3) + (button_size * 3)));

   b_o_turn_north->setSize( button_size, button_size );
   b_o_turn_north->setImageSize( button_size, button_size );
   b_turn_north_count_pos = Vector2f( sec_start_movement + (spacer * 2) + button_size,
                                      height - ((spacer * 2) + (button_size * 2)));
   b_o_turn_north->setPosition( b_turn_north_count_pos.x, b_turn_north_count_pos.y );

   b_o_turn_east->setSize( button_size, button_size );
   b_o_turn_east->setImageSize( button_size, button_size );
   b_turn_east_count_pos = Vector2f( sec_start_movement + (spacer * 3) + (button_size * 2),
                                     height - (spacer + (button_size * 1)));
   b_o_turn_east->setPosition( b_turn_east_count_pos.x, b_turn_east_count_pos.y );

   b_o_turn_south->setSize( button_size, button_size );
   b_o_turn_south->setImageSize( button_size, button_size );
   b_turn_south_count_pos = Vector2f( sec_start_movement + (spacer * 2) + button_size,
                                      height - (spacer + (button_size * 1)));
   b_o_turn_south->setPosition( b_turn_south_count_pos.x, b_turn_south_count_pos. y );

   b_o_turn_west->setSize( button_size, button_size );
   b_o_turn_west->setImageSize( button_size, button_size );
   b_turn_west_count_pos = Vector2f( sec_start_movement + spacer,
                                     height - (spacer + (button_size * 1)));
   b_o_turn_west->setPosition( b_turn_west_count_pos.x, b_turn_west_count_pos.y );

   b_o_turn_nearest_enemy->setSize( button_size, button_size );
   b_o_turn_nearest_enemy->setImageSize( button_size, button_size );
   b_turn_nearest_enemy_count_pos = Vector2f( sec_start_movement + spacer,
                                     height - ((spacer * 2) + (button_size * 2)));
   b_o_turn_nearest_enemy->setPosition( b_turn_nearest_enemy_count_pos.x, b_turn_nearest_enemy_count_pos.y );

   // Attack

   b_attack_area->setSize( (border * 3) + (spacer * 3) + (button_size * 2),
                           (border * 2) + (spacer * 4) + (button_size * 3));
   b_attack_area->setPosition( sec_start_attack - (2 * border),
                               height - (border + (spacer * 4) + (button_size * 3)));

   b_o_attack_smallest->setSize( button_size, button_size );
   b_o_attack_smallest->setImageSize( button_size, button_size );
   b_o_attack_smallest->setPosition( sec_start_attack + spacer,
                                     height - ((spacer * 2) + (button_size * 2)));

   b_o_attack_biggest->setSize( button_size, button_size );
   b_o_attack_biggest->setImageSize( button_size, button_size );
   b_o_attack_biggest->setPosition( sec_start_attack + (spacer * 2) + button_size,
                                    height - ((spacer * 2) + (button_size * 2)));

   b_o_attack_closest->setSize( button_size, button_size );
   b_o_attack_closest->setImageSize( button_size, button_size );
   b_o_attack_closest->setPosition( sec_start_attack + spacer,
                                    height - (spacer + (button_size * 1)));

   b_o_attack_farthest->setSize( button_size, button_size );
   b_o_attack_farthest->setImageSize( button_size, button_size );
   b_o_attack_farthest->setPosition( sec_start_attack + (spacer * 2) + button_size,
                                     height - (spacer + (button_size * 1)));

   // Units

   // Monster
   b_monster_area->setSize( (border * 3) + (spacer * 3) + (button_size * 2),
                            (border * 2) + (spacer * 4) + (button_size * 3));
   b_monster_area->setPosition( sec_start_monster - (2 * border),
                                height - (border + (spacer * 4) + (button_size * 3)));

   b_pl_alert_monster->setSize( button_size, button_size );
   b_pl_alert_monster->setImageSize( button_size, button_size );
   b_pl_alert_monster->setPosition( sec_start_monster + spacer,
                                    height - ((spacer * 3) + (button_size * 3)) );

   b_pl_cmd_go_monster->setSize( button_size, button_size );
   b_pl_cmd_go_monster->setImageSize( button_size, button_size );
   b_pl_cmd_go_monster->setPosition( sec_start_monster + (spacer * 2) + button_size,
                                     height - ((spacer * 3) + (button_size * 3)) );

   b_o_monster_guard->setSize( button_size, button_size );
   b_o_monster_guard->setImageSize( button_size, button_size );
   b_guard_count_pos = Vector2f( sec_start_monster + spacer,
                                 height - ((spacer * 2) + (button_size * 2)) );
   b_o_monster_guard->setPosition( b_guard_count_pos.x, b_guard_count_pos.y );

   b_o_monster_burst->setSize( button_size, button_size );
   b_o_monster_burst->setImageSize( button_size, button_size );
   b_o_monster_burst->setPosition( sec_start_monster + spacer,
                                   height - (spacer + button_size) );

   b_monster_image->setSize( button_size, button_size );
   b_monster_image->setImageSize( button_size, button_size );
   b_monster_image->setPosition( sec_start_monster + (spacer * 3) + button_size,
                                 height - (button_size) );

   // Soldier
   b_soldier_area->setSize( (border * 3) + (spacer * 3) + (button_size * 2),
                            (border * 2) + (spacer * 4) + (button_size * 3));
   b_soldier_area->setPosition( sec_start_soldier - (2 * border),
                                height - (border + (spacer * 4) + (button_size * 3)));

   b_pl_alert_soldier->setSize( button_size, button_size );
   b_pl_alert_soldier->setImageSize( button_size, button_size );
   b_pl_alert_soldier->setPosition( sec_start_soldier + spacer,
                                    height - ((spacer * 3) + (button_size * 3)) );

   b_pl_cmd_go_soldier->setSize( button_size, button_size );
   b_pl_cmd_go_soldier->setImageSize( button_size, button_size );
   b_pl_cmd_go_soldier->setPosition( sec_start_soldier + (spacer * 2) + button_size,
                                     height - ((spacer * 3) + (button_size * 3)) );

   b_o_soldier_switch_axe->setSize( button_size, button_size );
   b_o_soldier_switch_axe->setImageSize( button_size, button_size );
   b_switch_axe_count_pos = Vector2f( sec_start_soldier + spacer,
                                      height - ((spacer * 2) + (button_size * 2)) );
   b_o_soldier_switch_axe->setPosition( b_switch_axe_count_pos.x, b_switch_axe_count_pos.y );

   b_o_soldier_switch_spear->setSize( button_size, button_size );
   b_o_soldier_switch_spear->setImageSize( button_size, button_size );
   b_switch_spear_count_pos = Vector2f( sec_start_soldier + (spacer * 2) + button_size,
                                        height - ((spacer * 2) + (button_size * 2)) );
   b_o_soldier_switch_spear->setPosition( b_switch_spear_count_pos.x, b_switch_spear_count_pos.y );
   
   b_o_soldier_switch_bow->setSize( button_size, button_size );
   b_o_soldier_switch_bow->setImageSize( button_size, button_size );
   b_switch_bow_count_pos = Vector2f( sec_start_soldier + spacer,
                                      height - (spacer + button_size ) );
   b_o_soldier_switch_bow->setPosition( b_switch_bow_count_pos.x, b_switch_bow_count_pos.y );

   b_soldier_image->setSize( button_size, button_size );
   b_soldier_image->setImageSize( button_size, button_size );
   b_soldier_image->setPosition( sec_start_soldier + (spacer * 3) + button_size,
                                 height - (button_size) );

   // Worm
   b_worm_area->setSize( (border * 3) + (spacer * 3) + (button_size * 2),
                         (border * 2) + (spacer * 4) + (button_size * 3));
   b_worm_area->setPosition( sec_start_worm - (2 * border),
                             height - (border + (spacer * 4) + (button_size * 3)));

   b_pl_alert_worm->setSize( button_size, button_size );
   b_pl_alert_worm->setImageSize( button_size, button_size );
   b_pl_alert_worm->setPosition( sec_start_worm + spacer,
                                 height - ((spacer * 3) + (button_size * 3)) );

   b_pl_cmd_go_worm->setSize( button_size, button_size );
   b_pl_cmd_go_worm->setImageSize( button_size, button_size );
   b_pl_cmd_go_worm->setPosition( sec_start_worm + (spacer * 2) + button_size,
                                  height - ((spacer * 3) + (button_size * 3)) );

   b_o_worm_trail_on->setSize( button_size, button_size );
   b_o_worm_trail_on->setImageSize( button_size, button_size );
   b_trail_on_count_pos = Vector2f( sec_start_worm + spacer,
                                    height - ((spacer * 2) + (button_size * 2)) );
   b_o_worm_trail_on->setPosition( b_trail_on_count_pos.x, b_trail_on_count_pos.y );

   b_o_worm_trail_off->setSize( button_size, button_size );
   b_o_worm_trail_off->setImageSize( button_size, button_size );
   b_trail_off_count_pos = Vector2f( sec_start_worm + (spacer * 2) + button_size,
                                     height - ((spacer * 2) + (button_size * 2)) );
   b_o_worm_trail_off->setPosition( b_trail_off_count_pos.x, b_trail_off_count_pos.y );

   b_o_worm_sprint->setSize( button_size, button_size );
   b_o_worm_sprint->setImageSize( button_size, button_size );
   b_o_worm_sprint->setPosition( sec_start_worm + spacer,
                                 height - (spacer + button_size ) );

   b_worm_image->setSize( button_size, button_size );
   b_worm_image->setImageSize( button_size, button_size );
   b_worm_image->setPosition( sec_start_worm + (spacer * 3) + button_size,
                              height - (button_size) );

   // Bird
   b_bird_area->setSize( (border * 3) + (spacer * 3) + (button_size * 2),
                         (border * 2) + (spacer * 4) + (button_size * 3));
   b_bird_area->setPosition( sec_start_bird - (2 * border),
                             height - (border + (spacer * 4) + (button_size * 3)));

   b_pl_alert_bird->setSize( button_size, button_size );
   b_pl_alert_bird->setImageSize( button_size, button_size );
   b_pl_alert_bird->setPosition( sec_start_bird + spacer,
                                 height - ((spacer * 3) + (button_size * 3)) );

   b_pl_cmd_go_bird->setSize( button_size, button_size );
   b_pl_cmd_go_bird->setImageSize( button_size, button_size );
   b_pl_cmd_go_bird->setPosition( sec_start_bird + (spacer * 2) + button_size,
                                  height - ((spacer * 3) + (button_size * 3)) );

   b_cmd_bird_shout->setSize( button_size, button_size );
   b_cmd_bird_shout->setImageSize( button_size, button_size );
   b_cmd_bird_shout->setPosition( sec_start_bird + spacer,
                                height - ((spacer * 2) + (button_size * 2)) );

   b_cmd_bird_quiet->setSize( button_size, button_size );
   b_cmd_bird_quiet->setImageSize( button_size, button_size );
   b_cmd_bird_quiet->setPosition( sec_start_bird + (spacer * 2) + button_size,
                                height - ((spacer * 2) + (button_size * 2)) );

   b_o_bird_fly->setSize( button_size, button_size );
   b_o_bird_fly->setImageSize( button_size, button_size );
   b_fly_count_pos = Vector2f( sec_start_bird + spacer,
                               height - (spacer + button_size) );
   b_o_bird_fly->setPosition( b_fly_count_pos.x, b_fly_count_pos.y );

   b_bird_image->setSize( button_size, button_size );
   b_bird_image->setImageSize( button_size, button_size );
   b_bird_image->setPosition( sec_start_bird + (spacer * 3) + button_size,
                              height - (button_size) );

   // Bug
   b_bug_area->setSize( (border * 3) + (spacer * 6) + (button_size * 3),
                        (border * 2) + (spacer * 4) + (button_size * 3));
   b_bug_area->setPosition( sec_start_bug - (2 * border),
                            height - (border + (spacer * 4) + (button_size * 3)));

   b_pl_alert_bug->setSize( button_size, button_size );
   b_pl_alert_bug->setImageSize( button_size, button_size );
   b_pl_alert_bug->setPosition( sec_start_bug + spacer,
                                height - ((spacer * 3) + (button_size * 3)) );

   b_pl_cmd_go_bug->setSize( button_size, button_size );
   b_pl_cmd_go_bug->setImageSize( button_size, button_size );
   b_pl_cmd_go_bug->setPosition( sec_start_bug + (spacer * 2) + button_size,
                                 height - ((spacer * 3) + (button_size * 3)) );

   b_o_bug_fireball->setSize( button_size, button_size );
   b_o_bug_fireball->setImageSize( button_size, button_size );
   b_o_bug_fireball->setPosition( sec_start_bug + spacer,
                                  height - ((spacer * 2) + (button_size * 2)) );

   b_o_bug_sunder->setSize( button_size, button_size );
   b_o_bug_sunder->setImageSize( button_size, button_size );
   b_o_bug_sunder->setPosition( sec_start_bug + (spacer * 2) + button_size,
                                height - ((spacer * 2) + (button_size * 2)) );

   b_o_bug_heal->setSize( button_size, button_size );
   b_o_bug_heal->setImageSize( button_size, button_size );
   b_o_bug_heal->setPosition( sec_start_bug + (spacer * 3) + (button_size * 2),
                              height - ((spacer * 2) + (button_size * 2)) );

   b_o_bug_open_wormhole->setSize( button_size, button_size );
   b_o_bug_open_wormhole->setImageSize( button_size, button_size );
   b_o_bug_open_wormhole->setPosition( sec_start_bug + spacer,
                                        height - (spacer + button_size ) );

   b_o_bug_close_wormhole->setSize( button_size, button_size );
   b_o_bug_close_wormhole->setImageSize( button_size, button_size );
   b_o_bug_close_wormhole->setPosition( sec_start_bug + (spacer * 2) + button_size,
                                        height - (spacer + button_size ) );

   b_o_bug_meditate->setSize( button_size, button_size );
   b_o_bug_meditate->setImageSize( button_size, button_size );
   b_meditate_count_pos = Vector2f( sec_start_bug + (spacer * 3) + (button_size * 2),
                                    height - ((spacer * 3) + (button_size * 3) ) );
   b_o_bug_meditate->setPosition( b_meditate_count_pos.x, b_meditate_count_pos.y );

   b_bug_image->setSize( button_size, button_size );
   b_bug_image->setImageSize( button_size, button_size );
   b_bug_image->setPosition( width - button_size, height - button_size );

   view_rel_x_to_y = ((float)height) / ((float)width);
}

int initLevelGui()
{
   SFML_TextureManager &t_manager = SFML_TextureManager::getSingleton();
   IMGuiManager &gui_manager = IMGuiManager::getSingleton();

   // These should be ImageButtons with the border changing around the central image

   b_con_enemy_adjacent = new IMImageButton();
   b_con_enemy_adjacent->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_enemy_adjacent->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_enemy_adjacent->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );
   b_con_enemy_adjacent->setImage( t_manager.getTexture( "ConditionalEnemyAdjacent.png" ) );
   b_con_enemy_adjacent->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Condition: enemy adjacent", b_con_enemy_adjacent);

   b_con_enemy_ahead = new IMImageButton();
   b_con_enemy_ahead->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_enemy_ahead->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_enemy_ahead->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );
   b_con_enemy_ahead->setImage( t_manager.getTexture( "ConditionalEnemyAhead.png" ) );
   b_con_enemy_ahead->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Condition: enemy ahead", b_con_enemy_ahead);

   b_con_enemy_in_range = new IMImageButton();
   b_con_enemy_in_range->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_enemy_in_range->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_enemy_in_range->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );
   b_con_enemy_in_range->setImage( t_manager.getTexture( "ConditionalEnemyInRange.png" ) );
   b_con_enemy_in_range->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Condition: enemy in range", b_con_enemy_in_range);

   b_con_ally_adjacent = new IMImageButton();
   b_con_ally_adjacent->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_ally_adjacent->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_ally_adjacent->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );
   b_con_ally_adjacent->setImage( t_manager.getTexture( "ConditionalAllyAdjacent.png" ) );
   b_con_ally_adjacent->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Condition: ally adjacent", b_con_ally_adjacent);

   b_con_ally_ahead = new IMImageButton();
   b_con_ally_ahead->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_ally_ahead->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_ally_ahead->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );
   b_con_ally_ahead->setImage( t_manager.getTexture( "ConditionalAllyAhead.png" ) );
   b_con_ally_ahead->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Condition: ally ahead", b_con_ally_ahead);

   b_con_ally_in_range = new IMImageButton();
   b_con_ally_in_range->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_ally_in_range->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_ally_in_range->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );
   b_con_ally_in_range->setImage( t_manager.getTexture( "ConditionalAllyInRange.png" ) );
   b_con_ally_in_range->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Condition: ally in range", b_con_ally_in_range);

   b_con_clear = new IMImageButton();
   b_con_clear->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_clear->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_clear->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );
   b_con_clear->setImage( NULL );
   b_con_clear->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Clear conditionals", b_con_clear);

   b_o_start_block = new IMImageButton();
   b_o_start_block->setNormalTexture( t_manager.getTexture( "ControlButtonBase.png" ) );
   b_o_start_block->setPressedTexture( t_manager.getTexture( "ControlButtonPressed.png" ) );
   b_o_start_block->setHoverTexture( t_manager.getTexture( "ControlButtonHover.png" ) );
   b_o_start_block->setImage(  t_manager.getTexture( "ControlStartBlock.png" ) );
   b_o_start_block->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Control: start block", b_o_start_block);

   b_o_end_block = new IMImageButton();
   b_o_end_block->setNormalTexture( t_manager.getTexture( "ControlButtonBase.png" ) );
   b_o_end_block->setPressedTexture( t_manager.getTexture( "ControlButtonPressed.png" ) );
   b_o_end_block->setHoverTexture( t_manager.getTexture( "ControlButtonHover.png" ) );
   b_o_end_block->setImage(  t_manager.getTexture( "ControlEndBlock.png" ) );
   b_o_end_block->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Control: end block", b_o_end_block);

   b_o_repeat = new IMImageButton();
   b_o_repeat->setNormalTexture( t_manager.getTexture( "ControlButtonBase.png" ) );
   b_o_repeat->setPressedTexture( t_manager.getTexture( "ControlButtonPressed.png" ) );
   b_o_repeat->setHoverTexture( t_manager.getTexture( "ControlButtonHover.png" ) );
   b_o_repeat->setImage(  t_manager.getTexture( "ControlRepeat.png" ) );
   b_o_repeat->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Control: repeat", b_o_repeat);

   b_o_wait = new IMImageButton();
   b_o_wait->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_wait->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_wait->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_wait->setImage(  t_manager.getTexture( "OrderWait.png" ) );
   b_o_wait->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: wait", b_o_wait);

   b_o_move_forward = new IMImageButton();
   b_o_move_forward->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_move_forward->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_move_forward->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_move_forward->setImage( t_manager.getTexture( "OrderMoveForward.png" ) );
   b_o_move_forward->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: move forward", b_o_move_forward);

   b_o_move_backward = new IMImageButton();
   b_o_move_backward->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_move_backward->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_move_backward->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_move_backward->setImage( t_manager.getTexture( "OrderMoveBackward.png" )  );
   b_o_move_backward->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: move backward", b_o_move_backward);

   b_o_turn_north = new IMImageButton();
   b_o_turn_north->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_turn_north->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_turn_north->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_turn_north->setImage( t_manager.getTexture( "OrderTurnNorth.png" )  );
   b_o_turn_north->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: turn north", b_o_turn_north);

   b_o_turn_east = new IMImageButton();
   b_o_turn_east->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_turn_east->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_turn_east->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_turn_east->setImage(  t_manager.getTexture( "OrderTurnEast.png" ) );
   b_o_turn_east->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: turn east", b_o_turn_east);

   b_o_turn_south = new IMImageButton();
   b_o_turn_south->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_turn_south->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_turn_south->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_turn_south->setImage(  t_manager.getTexture( "OrderTurnSouth.png" ) );
   b_o_turn_south->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: turn south", b_o_turn_south);

   b_o_turn_west = new IMImageButton();
   b_o_turn_west->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_turn_west->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_turn_west->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_turn_west->setImage(  t_manager.getTexture( "OrderTurnWest.png" ) );
   b_o_turn_west->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: turn west", b_o_turn_west);

   b_o_turn_nearest_enemy = new IMImageButton();
   b_o_turn_nearest_enemy->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_turn_nearest_enemy->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_turn_nearest_enemy->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_turn_nearest_enemy->setImage(  t_manager.getTexture( "OrderTurnNearestEnemy.png" ) );
   b_o_turn_nearest_enemy->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: turn nearest enemy", b_o_turn_nearest_enemy);

   b_o_attack_smallest = new IMImageButton();
   b_o_attack_smallest->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_attack_smallest->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_attack_smallest->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_attack_smallest->setImage(  t_manager.getTexture( "OrderAttackSmallest.png" ) );
   b_o_attack_smallest->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: attack smallest", b_o_attack_smallest);

   b_o_attack_biggest = new IMImageButton();
   b_o_attack_biggest->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_attack_biggest->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_attack_biggest->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_attack_biggest->setImage(  t_manager.getTexture( "OrderAttackBiggest.png" ) );
   b_o_attack_biggest->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: attack biggest", b_o_attack_biggest);

   b_o_attack_closest = new IMImageButton();
   b_o_attack_closest->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_attack_closest->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_attack_closest->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_attack_closest->setImage(  t_manager.getTexture( "OrderAttackClosest.png" ) );
   b_o_attack_closest->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: attack closest", b_o_attack_closest);

   b_o_attack_farthest = new IMImageButton();
   b_o_attack_farthest->setNormalTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_attack_farthest->setPressedTexture( t_manager.getTexture( "OrderButtonPressed.png" ) );
   b_o_attack_farthest->setHoverTexture( t_manager.getTexture( "OrderButtonHover.png" ) );
   b_o_attack_farthest->setImage(  t_manager.getTexture( "OrderAttackFarthest.png" ) );
   b_o_attack_farthest->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Order: attack farthest", b_o_attack_farthest);

   b_pl_alert_all = new IMImageButton();
   b_pl_alert_all->setAllTextures( t_manager.getTexture( "PlayerAlert.png" ) );
   b_pl_alert_all->setImage( NULL );
   b_pl_alert_all->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Player: alert all", b_pl_alert_all);

   b_pl_cmd_go = new IMImageButton();
   b_pl_cmd_go->setAllTextures( t_manager.getTexture( "PlayerGoButton.png" ) );
   b_pl_cmd_go->setImage( NULL );
   b_pl_cmd_go->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Player: command go", b_pl_cmd_go);

   b_pl_cmd_go_all = new IMImageButton();
   b_pl_cmd_go_all->setAllTextures( t_manager.getTexture( "PlayerGoAllButton.png" ) );
   b_pl_cmd_go_all->setImage( NULL );
   b_pl_cmd_go_all->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Player: command go all", b_pl_cmd_go_all);

   b_pl_alert_monster = new IMImageButton();
   b_pl_alert_monster->setAllTextures( t_manager.getTexture( "MonsterAlertButton.png" ) );
   b_pl_alert_monster->setImage( NULL );
   b_pl_alert_monster->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Alert Monster", b_pl_alert_monster);

   b_pl_cmd_go_monster = new IMImageButton();
   b_pl_cmd_go_monster->setAllTextures( t_manager.getTexture( "MonsterGoButton.png" ) );
   b_pl_cmd_go_monster->setImage( NULL );
   b_pl_cmd_go_monster->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Go Monster", b_pl_cmd_go_monster);

   b_o_monster_guard = new IMImageButton();
   b_o_monster_guard->setNormalTexture( t_manager.getTexture( "MonsterOrderButtonBase.png" ) );
   b_o_monster_guard->setPressedTexture( t_manager.getTexture( "MonsterButtonPressed.png" ) );
   b_o_monster_guard->setHoverTexture( t_manager.getTexture( "MonsterButtonHover.png" ) );
   b_o_monster_guard->setImage( t_manager.getTexture( "MonsterOrderGuard.png" ) );
   b_o_monster_guard->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Monster: Guard", b_o_monster_guard);

   b_o_monster_burst = new IMImageButton();
   b_o_monster_burst->setNormalTexture( t_manager.getTexture( "MonsterOrderButtonBase.png" ) );
   b_o_monster_burst->setPressedTexture( t_manager.getTexture( "MonsterButtonPressed.png" ) );
   b_o_monster_burst->setHoverTexture( t_manager.getTexture( "MonsterButtonHover.png" ) );
   b_o_monster_burst->setImage( t_manager.getTexture( "MonsterOrderBurst.png" ) );
   b_o_monster_burst->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Monster: Burst", b_o_monster_burst);


   b_pl_alert_soldier = new IMImageButton();
   b_pl_alert_soldier->setAllTextures( t_manager.getTexture( "SoldierAlertButton.png" ) );
   b_pl_alert_soldier->setImage( NULL );
   b_pl_alert_soldier->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Alert Soldier", b_pl_alert_soldier);

   b_pl_cmd_go_soldier = new IMImageButton();
   b_pl_cmd_go_soldier->setAllTextures( t_manager.getTexture( "SoldierGoButton.png" ) );
   b_pl_cmd_go_soldier->setImage( NULL );
   b_pl_cmd_go_soldier->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Go Soldier", b_pl_cmd_go_soldier);

   b_o_soldier_switch_axe = new IMImageButton();
   b_o_soldier_switch_axe->setNormalTexture( t_manager.getTexture( "SoldierOrderButtonBase.png" ) );
   b_o_soldier_switch_axe->setPressedTexture( t_manager.getTexture( "SoldierButtonPressed.png" ) );
   b_o_soldier_switch_axe->setHoverTexture( t_manager.getTexture( "SoldierButtonHover.png" ) );
   b_o_soldier_switch_axe->setImage( t_manager.getTexture( "SoldierOrderSwitchAxe.png" ) );
   b_o_soldier_switch_axe->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Soldier: Switch Axe", b_o_soldier_switch_axe);

   b_o_soldier_switch_spear = new IMImageButton();
   b_o_soldier_switch_spear->setNormalTexture( t_manager.getTexture( "SoldierOrderButtonBase.png" ) );
   b_o_soldier_switch_spear->setPressedTexture( t_manager.getTexture( "SoldierButtonPressed.png" ) );
   b_o_soldier_switch_spear->setHoverTexture( t_manager.getTexture( "SoldierButtonHover.png" ) );
   b_o_soldier_switch_spear->setImage( t_manager.getTexture( "SoldierOrderSwitchSpear.png" ) );
   b_o_soldier_switch_spear->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Soldier: Switch Spear", b_o_soldier_switch_spear);

   b_o_soldier_switch_bow = new IMImageButton();
   b_o_soldier_switch_bow->setNormalTexture( t_manager.getTexture( "SoldierOrderButtonBase.png" ) );
   b_o_soldier_switch_bow->setPressedTexture( t_manager.getTexture( "SoldierButtonPressed.png" ) );
   b_o_soldier_switch_bow->setHoverTexture( t_manager.getTexture( "SoldierButtonHover.png" ) );
   b_o_soldier_switch_bow->setImage( t_manager.getTexture( "SoldierOrderSwitchBow.png" ) );
   b_o_soldier_switch_bow->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Soldier: Switch Bow", b_o_soldier_switch_bow);

   b_pl_alert_worm = new IMImageButton();
   b_pl_alert_worm->setAllTextures( t_manager.getTexture( "WormAlertButton.png" ) );
   b_pl_alert_worm->setImage( NULL );
   b_pl_alert_worm->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Alert Worm", b_pl_alert_worm);

   b_pl_cmd_go_worm = new IMImageButton();
   b_pl_cmd_go_worm->setAllTextures( t_manager.getTexture( "WormGoButton.png" ) );
   b_pl_cmd_go_worm->setImage( NULL );
   b_pl_cmd_go_worm->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Go Worm", b_pl_cmd_go_worm);

   b_o_worm_sprint = new IMImageButton();
   b_o_worm_sprint->setNormalTexture( t_manager.getTexture( "WormOrderButtonBase.png" ) );
   b_o_worm_sprint->setPressedTexture( t_manager.getTexture( "WormButtonPressed.png" ) );
   b_o_worm_sprint->setHoverTexture( t_manager.getTexture( "WormButtonHover.png" ) );
   b_o_worm_sprint->setImage( t_manager.getTexture( "WormOrderSprint.png" ) );
   b_o_worm_sprint->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Worm: Sprint", b_o_worm_sprint);

   b_o_worm_trail_on = new IMImageButton();
   b_o_worm_trail_on->setNormalTexture( t_manager.getTexture( "WormOrderButtonBase.png" ) );
   b_o_worm_trail_on->setPressedTexture( t_manager.getTexture( "WormButtonPressed.png" ) );
   b_o_worm_trail_on->setHoverTexture( t_manager.getTexture( "WormButtonHover.png" ) );
   b_o_worm_trail_on->setImage( t_manager.getTexture( "WormOrderTrailOn.png" ) );
   b_o_worm_trail_on->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Worm: Trail On", b_o_worm_trail_on);

   b_o_worm_trail_off = new IMImageButton();
   b_o_worm_trail_off->setNormalTexture( t_manager.getTexture( "WormOrderButtonBase.png" ) );
   b_o_worm_trail_off->setPressedTexture( t_manager.getTexture( "WormButtonPressed.png" ) );
   b_o_worm_trail_off->setHoverTexture( t_manager.getTexture( "WormButtonHover.png" ) );
   b_o_worm_trail_off->setImage( t_manager.getTexture( "WormOrderTrailOff.png" ) );
   b_o_worm_trail_off->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Worm: Trail Off", b_o_worm_trail_off);

   b_pl_alert_bird = new IMImageButton();
   b_pl_alert_bird->setAllTextures( t_manager.getTexture( "BirdAlertButton.png" ) );
   b_pl_alert_bird->setImage( NULL );
   b_pl_alert_bird->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Alert Bird", b_pl_alert_bird);

   b_pl_cmd_go_bird = new IMImageButton();
   b_pl_cmd_go_bird->setAllTextures( t_manager.getTexture( "BirdGoButton.png" ) );
   b_pl_cmd_go_bird->setImage( NULL );
   b_pl_cmd_go_bird->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Go Bird", b_pl_cmd_go_bird);

   b_o_bird_fly = new IMImageButton();
   b_o_bird_fly->setNormalTexture( t_manager.getTexture( "BirdOrderButtonBase.png" ) );
   b_o_bird_fly->setPressedTexture( t_manager.getTexture( "BirdButtonPressed.png" ) );
   b_o_bird_fly->setHoverTexture( t_manager.getTexture( "BirdButtonHover.png" ) );
   b_o_bird_fly->setImage( t_manager.getTexture( "BirdOrderFly.png" ) );
   b_o_bird_fly->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bird: Fly", b_o_bird_fly);

   b_cmd_bird_shout = new IMImageButton();
   b_cmd_bird_shout->setNormalTexture( t_manager.getTexture( "BirdControlButtonBase.png" ) );
   b_cmd_bird_shout->setPressedTexture( t_manager.getTexture( "BirdControlButtonPressed.png" ) );
   b_cmd_bird_shout->setHoverTexture( t_manager.getTexture( "BirdControlButtonHover.png" ) );
   b_cmd_bird_shout->setImage( t_manager.getTexture( "BirdControlShout.png" ) );
   b_cmd_bird_shout->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bird: Shout", b_cmd_bird_shout);

   b_cmd_bird_quiet = new IMImageButton();
   b_cmd_bird_quiet->setNormalTexture( t_manager.getTexture( "BirdControlButtonBase.png" ) );
   b_cmd_bird_quiet->setPressedTexture( t_manager.getTexture( "BirdControlButtonPressed.png" ) );
   b_cmd_bird_quiet->setHoverTexture( t_manager.getTexture( "BirdControlButtonHover.png" ) );
   b_cmd_bird_quiet->setImage( t_manager.getTexture( "BirdControlQuiet.png" ) );
   b_cmd_bird_quiet->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bird: Quiet", b_cmd_bird_quiet);

   b_pl_alert_bug = new IMImageButton();
   b_pl_alert_bug->setAllTextures( t_manager.getTexture( "BugAlertButton.png" ) );
   b_pl_alert_bug->setImage( NULL );
   b_pl_alert_bug->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Alert Bug", b_pl_alert_bug);

   b_pl_cmd_go_bug = new IMImageButton();
   b_pl_cmd_go_bug->setAllTextures( t_manager.getTexture( "BugGoButton.png" ) );
   b_pl_cmd_go_bug->setImage( NULL );
   b_pl_cmd_go_bug->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Go Bug", b_pl_cmd_go_bug);

   b_o_bug_meditate = new IMImageButton();
   b_o_bug_meditate->setNormalTexture( t_manager.getTexture( "BugOrderButtonBase.png" ) );
   b_o_bug_meditate->setPressedTexture( t_manager.getTexture( "BugButtonPressed.png" ) );
   b_o_bug_meditate->setHoverTexture( t_manager.getTexture( "BugButtonHover.png" ) );
   b_o_bug_meditate->setImage( t_manager.getTexture( "BugOrderMeditate.png" ) );
   b_o_bug_meditate->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bug: Meditate", b_o_bug_meditate);

   b_o_bug_fireball = new IMImageButton();
   b_o_bug_fireball->setNormalTexture( t_manager.getTexture( "BugOrderButtonBase.png" ) );
   b_o_bug_fireball->setPressedTexture( t_manager.getTexture( "BugButtonPressed.png" ) );
   b_o_bug_fireball->setHoverTexture( t_manager.getTexture( "BugButtonHover.png" ) );
   b_o_bug_fireball->setImage( t_manager.getTexture( "BugOrderFireball.png" ) );
   b_o_bug_fireball->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bug: Fireball", b_o_bug_fireball);

   b_o_bug_sunder = new IMImageButton();
   b_o_bug_sunder->setNormalTexture( t_manager.getTexture( "BugOrderButtonBase.png" ) );
   b_o_bug_sunder->setPressedTexture( t_manager.getTexture( "BugButtonPressed.png" ) );
   b_o_bug_sunder->setHoverTexture( t_manager.getTexture( "BugButtonHover.png" ) );
   b_o_bug_sunder->setImage( t_manager.getTexture( "BugOrderSunder.png" ) );
   b_o_bug_sunder->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bug: Sunder", b_o_bug_sunder);

   b_o_bug_heal = new IMImageButton();
   b_o_bug_heal->setNormalTexture( t_manager.getTexture( "BugOrderButtonBase.png" ) );
   b_o_bug_heal->setPressedTexture( t_manager.getTexture( "BugButtonPressed.png" ) );
   b_o_bug_heal->setHoverTexture( t_manager.getTexture( "BugButtonHover.png" ) );
   b_o_bug_heal->setImage( t_manager.getTexture( "BugOrderHeal.png" ) );
   b_o_bug_heal->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bug: Heal", b_o_bug_heal);

   b_o_bug_open_wormhole = new IMImageButton();
   b_o_bug_open_wormhole->setNormalTexture( t_manager.getTexture( "BugOrderButtonBase.png" ) );
   b_o_bug_open_wormhole->setPressedTexture( t_manager.getTexture( "BugButtonPressed.png" ) );
   b_o_bug_open_wormhole->setHoverTexture( t_manager.getTexture( "BugButtonHover.png" ) );
   b_o_bug_open_wormhole->setImage( t_manager.getTexture( "BugOrderOpenWormhole.png" ) );
   b_o_bug_open_wormhole->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bug: Open Wormhole", b_o_bug_open_wormhole);

   b_o_bug_close_wormhole = new IMImageButton();
   b_o_bug_close_wormhole->setNormalTexture( t_manager.getTexture( "BugOrderButtonBase.png" ) );
   b_o_bug_close_wormhole->setPressedTexture( t_manager.getTexture( "BugButtonPressed.png" ) );
   b_o_bug_close_wormhole->setHoverTexture( t_manager.getTexture( "BugButtonHover.png" ) );
   b_o_bug_close_wormhole->setImage( t_manager.getTexture( "BugOrderCloseWormhole.png" ) );
   b_o_bug_close_wormhole->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bug: Close Wormhole", b_o_bug_close_wormhole);

   b_monster_image = new IMImageButton();
   b_monster_image->setAllTextures( t_manager.getTexture( "MonsterButtonImage.png" ) );
   b_monster_image->setImage( NULL );
   b_monster_image->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Monster Image", b_monster_image);

   b_soldier_image = new IMImageButton();
   b_soldier_image->setAllTextures( t_manager.getTexture( "SoldierButtonImage.png" ) );
   b_soldier_image->setImage( NULL );
   b_soldier_image->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Soldier Image", b_soldier_image);

   b_worm_image = new IMImageButton();
   b_worm_image->setAllTextures( t_manager.getTexture( "WormButtonImage.png" ) );
   b_worm_image->setImage( NULL );
   b_worm_image->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Worm Image", b_worm_image);

   b_bird_image = new IMImageButton();
   b_bird_image->setAllTextures( t_manager.getTexture( "BirdButtonImage.png" ) );
   b_bird_image->setImage( NULL );
   b_bird_image->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bird Image", b_bird_image);

   b_bug_image = new IMImageButton();
   b_bug_image->setAllTextures( t_manager.getTexture( "BugButtonImage.png" ) );
   b_bug_image->setImage( NULL );
   b_bug_image->setImageOffset( 0, 0 );
   gui_manager.registerWidget( "Bug Image", b_bug_image);

   b_count_1 = new IMTextButton();
   b_count_1->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_1->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_1->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_1->setText( &s_1 );
   b_count_1->setTextColor( Color::Black );
   b_count_1->setFont( menu_font );
   gui_manager.registerWidget( "Count: 1", b_count_1);

   b_count_2 = new IMTextButton();
   b_count_2->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_2->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_2->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_2->setText( &s_2 );
   b_count_2->setTextColor( Color::Black );
   b_count_2->setFont( menu_font );
   gui_manager.registerWidget( "Count: 2", b_count_2);

   b_count_3 = new IMTextButton();
   b_count_3->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_3->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_3->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_3->setText( &s_3 );
   b_count_3->setTextColor( Color::Black );
   b_count_3->setFont( menu_font );
   gui_manager.registerWidget( "Count: 3", b_count_3);

   b_count_4 = new IMTextButton();
   b_count_4->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_4->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_4->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_4->setText( &s_4 );
   b_count_4->setTextColor( Color::Black );
   b_count_4->setFont( menu_font );
   gui_manager.registerWidget( "Count: 4", b_count_4);

   b_count_5 = new IMTextButton();
   b_count_5->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_5->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_5->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_5->setText( &s_5 );
   b_count_5->setTextColor( Color::Black );
   b_count_5->setFont( menu_font );
   gui_manager.registerWidget( "Count: 5", b_count_5);

   b_count_6 = new IMTextButton();
   b_count_6->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_6->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_6->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_6->setText( &s_6 );
   b_count_6->setTextColor( Color::Black );
   b_count_6->setFont( menu_font );
   gui_manager.registerWidget( "Count: 6", b_count_6);

   b_count_7 = new IMTextButton();
   b_count_7->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_7->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_7->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_7->setText( &s_7 );
   b_count_7->setTextColor( Color::Black );
   b_count_7->setFont( menu_font );
   gui_manager.registerWidget( "Count: 7", b_count_7);

   b_count_8 = new IMTextButton();
   b_count_8->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_8->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_8->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_8->setText( &s_8 );
   b_count_8->setTextColor( Color::Black );
   b_count_8->setFont( menu_font );
   gui_manager.registerWidget( "Count: 8", b_count_8);

   b_count_9 = new IMTextButton();
   b_count_9->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_9->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_9->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_9->setText( &s_9 );
   b_count_9->setTextColor( Color::Black );
   b_count_9->setFont( menu_font );
   gui_manager.registerWidget( "Count: 9", b_count_9);

   b_count_0 = new IMTextButton();
   b_count_0->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_0->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_0->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_0->setText( &s_0 );
   b_count_0->setTextColor( Color::Black );
   b_count_0->setFont( menu_font );
   gui_manager.registerWidget( "Count: 0", b_count_0);

   b_count_infinite = new IMTextButton();
   b_count_infinite->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_infinite->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_infinite->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_infinite->setText( &s_infinite );
   b_count_infinite->setTextColor( Color::Black );
   b_count_infinite->setFont( menu_font );
   gui_manager.registerWidget( "Count: infinite", b_count_infinite);

   b_count_reset = new IMTextButton();
   b_count_reset->setNormalTexture( t_manager.getTexture( "CountButtonBase.png" ) );
   b_count_reset->setPressedTexture( t_manager.getTexture( "CountButtonPressed.png" ) );
   b_count_reset->setHoverTexture( t_manager.getTexture( "CountButtonHover.png" ) );
   b_count_reset->setText( &s_reset );
   b_count_reset->setTextColor( Color::Black );
   b_count_reset->setFont( menu_font );
   gui_manager.registerWidget( "Count: reset", b_count_reset);

   count_text = new Text();
   count_text->setFont( *menu_font );
   count_text->setColor( Color::Black );

   b_numpad_area = new IMEdgeButton();
   b_numpad_area->setAllTextures( t_manager.getTexture( "UICenterBrown.png" ) );
   b_numpad_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_numpad_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_numpad_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Numpad area", b_numpad_area);

   b_conditional_area = new IMEdgeButton();
   b_conditional_area->setAllTextures( t_manager.getTexture( "UICenterBrown.png" ) );
   b_conditional_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_conditional_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_conditional_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Conditional area", b_conditional_area);

   b_movement_area = new IMEdgeButton();
   b_movement_area->setAllTextures( t_manager.getTexture( "UICenterBrown.png" ) );
   b_movement_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_movement_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_movement_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Movement area", b_movement_area);

   b_attack_area = new IMEdgeButton();
   b_attack_area->setAllTextures( t_manager.getTexture( "UICenterBrown.png" ) );
   b_attack_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_attack_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_attack_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Attack area", b_attack_area);

   b_control_area = new IMEdgeButton();
   b_control_area->setAllTextures( t_manager.getTexture( "UICenterBrown.png" ) );
   b_control_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_control_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_control_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Control area", b_control_area);

   b_pl_cmd_area = new IMEdgeButton();
   b_pl_cmd_area->setAllTextures( t_manager.getTexture( "UICenterBrown.png" ) );
   b_pl_cmd_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_pl_cmd_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_pl_cmd_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Player command area", b_pl_cmd_area);

   b_monster_area = new IMEdgeButton();
   b_monster_area->setAllTextures( t_manager.getTexture( "MonsterColorSoft.png" ) );
   b_monster_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_monster_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_monster_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Monster area", b_monster_area);

   b_soldier_area = new IMEdgeButton();
   b_soldier_area->setAllTextures( t_manager.getTexture( "SoldierColorSoft.png" ) );
   b_soldier_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_soldier_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_soldier_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Soldier area", b_soldier_area);

   b_worm_area = new IMEdgeButton();
   b_worm_area->setAllTextures( t_manager.getTexture( "WormColorSoft.png" ) );
   b_worm_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_worm_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_worm_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Worm area", b_worm_area);

   b_bird_area = new IMEdgeButton();
   b_bird_area->setAllTextures( t_manager.getTexture( "BirdColorSoft.png" ) );
   b_bird_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_bird_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_bird_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Bird area", b_bird_area);

   b_bug_area = new IMEdgeButton();
   b_bug_area->setAllTextures( t_manager.getTexture( "BugColorSoft.png" ) );
   b_bug_area->setCornerAllTextures( t_manager.getTexture( "UICornerBrown3px.png" ) );
   b_bug_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeBrown3px.png" ) );
   b_bug_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Bug area", b_bug_area);

   fitGui_Level();
   init_level_gui = true;
   return 0;
}

int selectConditionButton( Order_Conditional c )
{
   SFML_TextureManager &t_manager = SFML_TextureManager::getSingleton();

   b_con_enemy_adjacent->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_enemy_adjacent->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_enemy_adjacent->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );

   b_con_enemy_ahead->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_enemy_ahead->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_enemy_ahead->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );

   b_con_enemy_in_range->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_enemy_in_range->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_enemy_in_range->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );

   b_con_ally_adjacent->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_ally_adjacent->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_ally_adjacent->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );

   b_con_ally_ahead->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_ally_ahead->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_ally_ahead->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );

   b_con_ally_in_range->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_ally_in_range->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_ally_in_range->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );

   b_con_clear->setNormalTexture( t_manager.getTexture( "ConditionalButtonBase.png" ) );
   b_con_clear->setHoverTexture( t_manager.getTexture( "ConditionalButtonHover.png" ) );
   b_con_clear->setPressedTexture( t_manager.getTexture( "ConditionalButtonPressed.png" ) );

   switch (c)
   {
      case ENEMY_ADJACENT:
         b_con_enemy_adjacent->setAllTextures(t_manager.getTexture("ConditionalSelectedGreen.png"));
         break;
      case ENEMY_NOT_ADJACENT:
         b_con_enemy_adjacent->setAllTextures(t_manager.getTexture("ConditionalSelectedRed.png"));
         break;
      case ENEMY_AHEAD:
         b_con_enemy_ahead->setAllTextures(t_manager.getTexture("ConditionalSelectedGreen.png"));
         break;
      case ENEMY_NOT_AHEAD:
         b_con_enemy_ahead->setAllTextures(t_manager.getTexture("ConditionalSelectedRed.png"));
         break;
      case ENEMY_IN_RANGE:
         b_con_enemy_in_range->setAllTextures(t_manager.getTexture("ConditionalSelectedGreen.png"));
         break;
      case ENEMY_NOT_IN_RANGE:
         b_con_enemy_in_range->setAllTextures(t_manager.getTexture("ConditionalSelectedRed.png"));
         break;
      case ALLY_ADJACENT:
         b_con_ally_adjacent->setAllTextures(t_manager.getTexture("ConditionalSelectedGreen.png"));
         break;
      case ALLY_NOT_ADJACENT:
         b_con_ally_adjacent->setAllTextures(t_manager.getTexture("ConditionalSelectedRed.png"));
         break;
      case ALLY_AHEAD:
         b_con_ally_ahead->setAllTextures(t_manager.getTexture("ConditionalSelectedGreen.png"));
         break;
      case ALLY_NOT_AHEAD:
         b_con_ally_ahead->setAllTextures(t_manager.getTexture("ConditionalSelectedRed.png"));
         break;
      case ALLY_IN_RANGE:
         b_con_ally_in_range->setAllTextures(t_manager.getTexture("ConditionalSelectedGreen.png"));
         break;
      case ALLY_NOT_IN_RANGE:
         b_con_ally_in_range->setAllTextures(t_manager.getTexture("ConditionalSelectedRed.png"));
         break;
      default:
         break;
   }

   return true;
}

int drawOrderButtons()
{
   if (init_level_gui == false)
      initLevelGui();

   // TODO Keybinding markers

   // Count boxes
   int count_text_size = 12;
   if (button_size < 40) count_text_size = 10;
   drawCount( 0, b_turn_north_count_pos.x, b_turn_north_count_pos.y, button_size, false, count_text_size );
   drawCount( 0, b_turn_east_count_pos.x, b_turn_east_count_pos.y, button_size, false, count_text_size );
   drawCount( 0, b_turn_south_count_pos.x, b_turn_south_count_pos.y, button_size, false, count_text_size );
   drawCount( 0, b_turn_west_count_pos.x, b_turn_west_count_pos.y, button_size, false, count_text_size );
   drawCount( 0, b_turn_nearest_enemy_count_pos.x, b_turn_nearest_enemy_count_pos.y, button_size, false, count_text_size );
   drawCount( 1, b_guard_count_pos.x, b_guard_count_pos.y, button_size, true, count_text_size );
   drawCount( 0, b_switch_axe_count_pos.x, b_switch_axe_count_pos.y, button_size, false, count_text_size );
   drawCount( 0, b_switch_spear_count_pos.x, b_switch_spear_count_pos.y, button_size, false, count_text_size );
   drawCount( 0, b_switch_bow_count_pos.x, b_switch_bow_count_pos.y, button_size, false, count_text_size );
   drawCount( 0, b_trail_on_count_pos.x, b_trail_on_count_pos.y, button_size, false, count_text_size );
   drawCount( 0, b_trail_off_count_pos.x, b_trail_off_count_pos.y, button_size, false, count_text_size );
   drawCount( 2, b_fly_count_pos.x, b_fly_count_pos.y, button_size, true, count_text_size );
   drawCount( 10, b_meditate_count_pos.x, b_meditate_count_pos.y, button_size, false, count_text_size );

   if (b_con_enemy_adjacent->doWidget())
      playerAddConditional( ENEMY_ADJACENT );

   if (b_con_enemy_ahead->doWidget())
      playerAddConditional( ENEMY_AHEAD );

   if (b_con_enemy_in_range->doWidget())
      playerAddConditional( ENEMY_IN_RANGE );

   if (b_con_ally_adjacent->doWidget())
      playerAddConditional( ALLY_ADJACENT );

   if (b_con_ally_ahead->doWidget())
      playerAddConditional( ALLY_AHEAD );

   if (b_con_ally_in_range->doWidget())
      playerAddConditional( ALLY_IN_RANGE );

   if (b_con_clear->doWidget())
      playerAddConditional( TRUE );

   if (b_o_move_forward->doWidget())
      playerAddOrder( MOVE_FORWARD );

   if (b_o_move_backward->doWidget())
      playerAddOrder( MOVE_BACK );
      
   if (b_o_turn_north->doWidget())
      playerAddOrder( TURN_NORTH );
      
   if (b_o_turn_east->doWidget())
      playerAddOrder( TURN_EAST );
      
   if (b_o_turn_south->doWidget())
      playerAddOrder( TURN_SOUTH );
      
   if (b_o_turn_west->doWidget())
      playerAddOrder( TURN_WEST );
      
   if (b_o_turn_nearest_enemy->doWidget())
      playerAddOrder( TURN_NEAREST_ENEMY );
      
   if (b_o_attack_smallest->doWidget())
      playerAddOrder( ATTACK_SMALLEST );
      
   if (b_o_attack_biggest->doWidget())
      playerAddOrder( ATTACK_BIGGEST );
      
   if (b_o_attack_closest->doWidget())
      playerAddOrder( ATTACK_CLOSEST );
      
   if (b_o_attack_farthest->doWidget())
      playerAddOrder( ATTACK_FARTHEST );
      
   if (b_o_start_block->doWidget())
      playerAddOrder( START_BLOCK );
      
   if (b_o_end_block->doWidget())
      playerAddOrder( END_BLOCK );
      
   if (b_o_repeat->doWidget())
      playerAddOrder( REPEAT );

   if (b_o_wait->doWidget())
      playerAddOrder( WAIT );

   if (b_pl_alert_all->doWidget())
      playerAddOrder( PL_ALERT_ALL );

   if (b_pl_cmd_go->doWidget())
      playerAddOrder( PL_CMD_GO );

   if (b_pl_cmd_go_all->doWidget())
      playerAddOrder( PL_CMD_GO_ALL );

   if (b_pl_alert_monster->doWidget())
      playerAddOrder( PL_ALERT_MONSTERS );

   if (b_pl_cmd_go_monster->doWidget())
      playerAddOrder( PL_CMD_GO_MONSTERS );

   if (b_o_monster_guard->doWidget())
      playerAddOrder( MONSTER_GUARD );

   if (b_o_monster_burst->doWidget())
      playerAddOrder( MONSTER_BURST );

   if (b_pl_alert_soldier->doWidget())
      playerAddOrder( PL_ALERT_SOLDIERS );

   if (b_pl_cmd_go_soldier->doWidget())
      playerAddOrder( PL_CMD_GO_SOLDIERS );

   if (b_o_soldier_switch_axe->doWidget())
      playerAddOrder( SOLDIER_SWITCH_AXE );

   if (b_o_soldier_switch_spear->doWidget())
      playerAddOrder( SOLDIER_SWITCH_SPEAR );

   if (b_o_soldier_switch_bow->doWidget())
      playerAddOrder( SOLDIER_SWITCH_BOW );

   if (b_pl_alert_worm->doWidget())
      playerAddOrder( PL_ALERT_WORMS );

   if (b_pl_cmd_go_worm->doWidget())
      playerAddOrder( PL_CMD_GO_WORMS );

   if (b_o_worm_sprint->doWidget())
      playerAddOrder( WORM_SPRINT );

   if (b_o_worm_trail_on->doWidget())
      playerAddOrder( WORM_TRAIL_START );

   if (b_o_worm_trail_off->doWidget())
      playerAddOrder( WORM_TRAIL_END );

   if (b_pl_alert_bird->doWidget())
      playerAddOrder( PL_ALERT_BIRDS );

   if (b_pl_cmd_go_bird->doWidget())
      playerAddOrder( PL_CMD_GO_BIRDS );

   if (b_o_bird_fly->doWidget())
      playerAddOrder( BIRD_FLY );

   if (b_cmd_bird_shout->doWidget())
      playerAddOrder( BIRD_CMD_SHOUT );

   if (b_cmd_bird_quiet->doWidget())
      playerAddOrder( BIRD_CMD_QUIET );

   if (b_pl_alert_bug->doWidget())
      playerAddOrder( PL_ALERT_BUGS );

   if (b_pl_cmd_go_bug->doWidget())
      playerAddOrder( PL_CMD_GO_BUGS );

   if (b_o_bug_meditate->doWidget())
      playerAddOrder( BUG_MEDITATE );

   if (b_o_bug_fireball->doWidget())
      playerAddOrder( BUG_CAST_FIREBALL );

   if (b_o_bug_sunder->doWidget())
      playerAddOrder( BUG_CAST_SUNDER );

   if (b_o_bug_heal->doWidget())
      playerAddOrder( BUG_CAST_HEAL );

   if (b_o_bug_open_wormhole->doWidget())
      playerAddOrder( BUG_OPEN_WORMHOLE );

   if (b_o_bug_close_wormhole->doWidget())
      playerAddOrder( BUG_CLOSE_WORMHOLE );


   if (b_count_0->doWidget())
      playerAddCount( 0 );

   if (b_count_1->doWidget())
      playerAddCount( 1 );

   if (b_count_2->doWidget())
      playerAddCount( 2 );

   if (b_count_3->doWidget())
      playerAddCount( 3 );

   if (b_count_4->doWidget())
      playerAddCount( 4 );

   if (b_count_5->doWidget())
      playerAddCount( 5 );

   if (b_count_6->doWidget())
      playerAddCount( 6 );

   if (b_count_7->doWidget())
      playerAddCount( 7 );

   if (b_count_8->doWidget())
      playerAddCount( 8 );

   if (b_count_9->doWidget())
      playerAddCount( 9 );

   if (b_count_infinite->doWidget())
      playerSetCount( -1 );

   if (b_count_reset->doWidget())
      playerSetCount( 1 );

   b_monster_image->doWidget();
   b_soldier_image->doWidget();
   b_worm_image->doWidget();
   b_bird_image->doWidget();
   b_bug_image->doWidget();

   b_numpad_area->doWidget();
   b_pl_cmd_area->doWidget();
   b_conditional_area->doWidget();
   b_control_area->doWidget();
   b_movement_area->doWidget();
   b_attack_area->doWidget();
   b_monster_area->doWidget();
   b_soldier_area->doWidget();
   b_worm_area->doWidget();
   b_bird_area->doWidget();
   b_bug_area->doWidget();

   // Draw current count
   if (order_prep_count != 1) {
      stringstream ss;
      if (order_prep_count == -1)
         ss << "indefinitely";
      else
         ss << order_prep_count;
      count_text->setString( String(ss.str()) );

      SFML_GlobalRenderWindow::get()->draw( *count_text );
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////
// Rest of the Gui ---

Sprite *s_clock_face,
       *s_clock_half_red,
       *s_clock_half_white;
bool init_tick_clock = false;

void initTickClock()
{
   s_clock_face = new Sprite( *SFML_TextureManager::getSingleton().getTexture( "ClockFace.png" ) );
   normalizeTo1x1( s_clock_face );
   s_clock_face->scale( 32, 32 );
   s_clock_face->setPosition( 2, 2 );

   s_clock_half_red = new Sprite( *SFML_TextureManager::getSingleton().getTexture( "ClockHalfRed.png" ) );
   normalizeTo1x1( s_clock_half_red );
   s_clock_half_red->scale( 32, 32 );
   s_clock_half_red->setOrigin( 64, 64 );
   s_clock_half_red->setPosition( 18, 18 );

   s_clock_half_white = new Sprite( *SFML_TextureManager::getSingleton().getTexture( "ClockHalfWhite.png" ) );
   normalizeTo1x1( s_clock_half_white );
   s_clock_half_white->setOrigin( 64, 64 );
   s_clock_half_white->scale( 32, 32 );
   s_clock_half_white->setPosition( 18, 18 );

   init_tick_clock = true;
}

int drawClock()
{
   RenderWindow *gui_window = SFML_GlobalRenderWindow::get();

   if (!init_tick_clock)
      initTickClock();

   Sprite *s_1, *s_2;
   if (turn % 2 == 0) {
      s_1 = s_clock_half_red;
      s_2 = s_clock_half_white;
   } else {
      s_1 = s_clock_half_white;
      s_2 = s_clock_half_red;
   }

   s_1->setRotation( 0 );
   gui_window->draw( *s_1 );
   s_2->setRotation( 180 );
   gui_window->draw( *s_2 );
   if (turn_progress < (TURN_LENGTH / 2)) {
      s_2->setRotation( 360 * ( (float)turn_progress / (float)TURN_LENGTH ) );
      gui_window->draw( *s_2 );
   } else {
      s_1->setRotation( 360 * ( ((float)turn_progress / (float)TURN_LENGTH ) - 0.5) );
      gui_window->draw( *s_1 );
   }

   gui_window->draw( *s_clock_face );

   return 0;
}

int drawOrderQueue()
{
   if (player) {
      int draw_x = 38, draw_y = 2, x_edge = config::width();
      if (selected_unit != NULL) x_edge -= selection_box_width;

      for (int i = player->current_order; i != player->final_order; ++i) {
         if (i == player->max_orders) i = 0;

         Order &o = player->order_queue[i];
         if ( o.action != SKIP ) {
            drawOrder( player->order_queue[i], draw_x, draw_y, 32 );
            draw_x += 36;

            if (draw_x + 32 >= x_edge) {
               if (pause_state == FULLY_PAUSED) {
                  draw_x = 2;
                  draw_y += 36;
               } else {
                  break;
               }
            }
         }
      }
   }
   return 0;
}

int drawSelectedUnit()
{
   RenderWindow *gui_window = SFML_GlobalRenderWindow::get();

   if (selected_unit != NULL) {
      if (selected_unit->alive == false) {
         selected_unit = NULL;
         return -1;
      }

      Texture *t = selected_unit->getTexture();
      if (t) { 
         float window_edge = config::width();
         RectangleShape select_rect, health_bound_rect, health_rect;

         select_rect.setSize( Vector2f( selection_box_width, selection_box_height ) );
         select_rect.setPosition( window_edge - selection_box_width, 0 );
         select_rect.setFillColor( Color::White );
         select_rect.setOutlineColor( Color::Black );
         select_rect.setOutlineThickness( 2.0 );
         gui_window->draw( select_rect );

         // The image
         int picture_size = selection_box_height - 10;
         Sprite unit_image( *t);
         normalizeTo1x1( &unit_image );
         unit_image.scale( picture_size, picture_size );
         unit_image.setPosition( window_edge - selection_box_width + 5, 5 );

         gui_window->draw( unit_image );

         // Health box
         int health_box_start_x = window_edge - selection_box_width + selection_box_height + 5,
             health_box_width = window_edge - health_box_start_x - 5,
             health_box_start_y = selection_box_height - 18,
             health_box_height = 14;
         health_bound_rect.setSize( Vector2f( health_box_width, health_box_height ) );
         health_bound_rect.setPosition( health_box_start_x, health_box_start_y );
         health_bound_rect.setFillColor( Color::White );
         health_bound_rect.setOutlineColor( Color::Black );
         health_bound_rect.setOutlineThickness( 2.0 );
         gui_window->draw( health_bound_rect );

         health_rect.setSize( Vector2f( (health_box_width - 4) * (selected_unit->health / selected_unit->max_health), 10 ) );
         health_rect.setPosition( health_box_start_x + 2, health_box_start_y + 2 );
         health_rect.setFillColor( Color::Red );
         gui_window->draw( health_rect );

         // Descriptive text
         Text selected_text;
         selected_text.setString(String(selected_unit->descriptor()));
         selected_text.setFont( *menu_font );
         selected_text.setColor( (selected_unit->team == 0)?Color::Black:Color::Red);
         selected_text.setCharacterSize( 24 );
         FloatRect text_size = selected_text.getGlobalBounds();
         float text_x = ((health_box_width - text_size.width) / 2) + health_box_start_x;
         selected_text.setPosition( text_x, 0 );
         gui_window->draw( selected_text );

         if (selected_unit != player && selected_unit->team == 0)
         {
            // Draw selected unit's order queue
            int draw_x = window_edge - selection_box_width + 1,
                draw_y = selection_box_height + 2,
                dx = selection_box_width / 6;
            for (int i = 0; i < selected_unit->order_count; ++i) {
               if (i == selected_unit->max_orders) i = 0;

               Order &o = selected_unit->order_queue[i];
               if ( o.action != SKIP ) {
                  drawOrder( o, draw_x, draw_y, (dx - 2) );
                  draw_x += dx;
                  if (draw_x >= window_edge) {
                     draw_x = window_edge - selection_box_width + 1;
                     draw_y += dx;
                  }
               }
            }
         }

      }
   }

   return 0;
}

int drawGui()
{
   RenderWindow *gui_window = SFML_GlobalRenderWindow::get();
   gui_window->setView(gui_window->getDefaultView());

   drawPause();

   drawClock();
   drawOrderQueue();
   drawSelectedUnit();
   drawOrderButtons();

   drawRightClickMenu();

   return 0;
}

//////////////////////////////////////////////////////////////////////
// Level Editor ---

Vector2u l_e_selection;
bool l_e_selected;

int levelEditorSelectGrid( Vector2f coords )
{
   int cx = (int)coords.x,
       cy = (int)coords.y;

   if (cx < 0 || cx >= level_dim_x || cy < 0 || cy >= level_dim_y) {
      l_e_selected = false;
      return -1;
   }

   l_e_selection.x = cx;
   l_e_selection.y = cy;
   l_e_selected = true;

   return 0;
}

int levelEditorChangeTerrain( int change )
{
   if (!l_e_selected) {
      return -1;
   }

   int cur_t = GRID_AT(terrain_grid,l_e_selection.x,l_e_selection.y);
   int new_t = cur_t + change;

   if (new_t < 0)
      new_t = 0;
   if (new_t >= NUM_TERRAINS)
      new_t = NUM_TERRAINS - 1;

   GRID_AT(terrain_grid,l_e_selection.x,l_e_selection.y) = (Terrain)new_t;

   return 0;
}

int loadLevelEditor( int level )
{
   loadLevel( level );

   menu_state = MENU_MAIN | MENU_PRI_LEVEL_EDITOR;
   setLevelListener(false);
   setLevelEditorListener(true);

   vision_enabled = false;
   calculateVision();
   
   return 0;
}

int levelEditorWriteToFile()
{
   string level_name_out = "level_editor_output.lvl";

   return writeLevelToFile( level_name_out );
}

//////////////////////////////////////////////////////////////////////
// Draw ---

void drawBaseTerrain()
{
   Sprite *s_ter;
   if (base_terrain == BASE_TER_GRASS)
         s_ter = base_grass_sprite;
   if (base_terrain == BASE_TER_MOUNTAIN)
         s_ter = base_mountain_sprite;
   if (base_terrain == BASE_TER_UNDERGROUND)
         s_ter = base_underground_sprite;
   
   s_ter->setPosition( 0, 0 );
   s_ter->setScale( level_dim_x, level_dim_y ); 
   SFML_GlobalRenderWindow::get()->draw( *s_ter );
}

void drawTerrain()
{
   RenderWindow *r_window = SFML_GlobalRenderWindow::get();
   int x, y;
   for (x = 0; x < level_dim_x; ++x) {
      for (y = 0; y < level_dim_y; ++y) {

         Sprite *s_ter = terrain_sprites[GRID_AT(terrain_grid,x,y)];
      
         if (NULL != s_ter) {
            s_ter->setPosition( x, y );
            r_window->draw( *s_ter );
         }

      }
   }
}

void drawUnits()
{
   if (NULL != player)
      player->draw();

   if (NULL != summonMarker)
      summonMarker->draw();

   for (list<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit && GRID_AT(vision_grid,unit->x_grid,unit->y_grid) == VIS_VISIBLE) {
         unit->draw();
      }
   }
}

void drawEffects()
{
   for (list<Effect*>::iterator it=effect_list.begin(); it != effect_list.end(); ++it)
   {
      Effect* effect = (*it);
      if (effect) {
         effect->draw();
      }
   }
}

int drawFog()
{
   SFML_TextureManager &t_manager = SFML_TextureManager::getSingleton();
   RenderWindow *r_window = SFML_GlobalRenderWindow::get();

   Sprite s_unseen( *(t_manager.getTexture( "FogCloudSolid.png" )) ),
          s_dark( *(t_manager.getTexture( "FogCloudDark.png" )) ),
          s_fog_base( *(t_manager.getTexture( "Fog.png" )) ),
          s_fog_left( *(t_manager.getTexture( "FogCloudTransparentLeft.png" )) ),
          s_fog_right( *(t_manager.getTexture( "FogCloudTransparentRight.png" )) ),
          s_fog_top( *(t_manager.getTexture( "FogCloudTransparentTop.png" )) ),
          s_fog_bottom( *(t_manager.getTexture( "FogCloudTransparentBottom.png" )) );
   normalizeTo1x1( &s_unseen );
   s_unseen.scale( 1.3, 1.3 );
   normalizeTo1x1( &s_dark );
   s_dark.scale( 1.3, 1.3 );
   normalizeTo1x1( &s_fog_base );
   normalizeTo1x1( &s_fog_left );
   normalizeTo1x1( &s_fog_right );
   normalizeTo1x1( &s_fog_top );
   normalizeTo1x1( &s_fog_bottom );
   int x, y;
   for (x = 0; x < level_dim_x; ++x) {
      for (y = 0; y < level_dim_y; ++y) {
         Vision v = GRID_AT(vision_grid,x,y);
      
         if (v == VIS_NONE || v == VIS_OFFMAP || v == VIS_NEVER_SEEN) {
            s_unseen.setPosition( x - 0.17, y - 0.17 );
            r_window->draw( s_unseen );
         }
         else if (v == VIS_SEEN_BEFORE) {
            s_fog_base.setPosition( x, y );
            r_window->draw( s_fog_base );
         }
         else
         {
            // Draw fog edges for adjacent semi-fogged areas
            if (x != 0 && GRID_AT(vision_grid,(x-1),y) == VIS_SEEN_BEFORE) {
               s_fog_right.setPosition( x, y );
               r_window->draw( s_fog_right );
            }
            if (y != 0 && GRID_AT(vision_grid,x,(y-1)) == VIS_SEEN_BEFORE) {
               s_fog_bottom.setPosition( x, y );
               r_window->draw( s_fog_bottom );
            }
            if (x != level_dim_x - 1 && GRID_AT(vision_grid,(x+1),y) == VIS_SEEN_BEFORE) {
               s_fog_left.setPosition( x, y );
               r_window->draw( s_fog_left );
            }
            if (y != level_dim_y - 1 && GRID_AT(vision_grid,x,(y+1)) == VIS_SEEN_BEFORE) {
               s_fog_top.setPosition( x, y );
               r_window->draw( s_fog_top );
            }
         }
      }
   }

   // TODO: Draw something to delineate the level boundaries
   for (x = -1; x <= level_dim_x; ++x) {
      s_dark.setPosition( x - 0.17, -1 );
      r_window->draw( s_dark );
      s_dark.setPosition( x - 0.17, level_dim_y - 0.17 );
      r_window->draw( s_dark );
   }
   for (y = 0; y < level_dim_y; ++y) {
      s_dark.setPosition( -1.17, y - 0.17 );
      r_window->draw( s_dark );
      s_dark.setPosition( level_dim_x - 0.17, y - 0.17 );
      r_window->draw( s_dark );
   }

   return 0;
}

int drawLevel()
{
   SFML_GlobalRenderWindow::get()->setView(*level_view);
   SFML_GlobalRenderWindow::get()->clear( Color( 64, 64, 64 ) );
   // Level
   drawBaseTerrain();
   drawTerrain();
   drawUnits();
   drawEffects();
   drawFog();

   // Gui
   drawGui();

   return 0;
} 

//////////////////////////////////////////////////////////////////////
// Event listener ---

struct LevelEventHandler : public My_SFML_MouseListener, public My_SFML_KeyListener
{
   virtual bool keyPressed( const Event::KeyEvent &key_press )
   {
      if (key_press.code == Keyboard::Q)
         shutdown(1,1);

      if (key_press.code == Keyboard::LShift || key_press.code == Keyboard::RShift)
         key_shift_down = 1;

      // View movement
      if (key_press.code == Keyboard::Right)
         shiftView( 2, 0 );
      if (key_press.code == Keyboard::Left)
         shiftView( -2, 0 );
      if (key_press.code == Keyboard::Down)
         shiftView( 0, 2 );
      if (key_press.code == Keyboard::Up)
         shiftView( 0, -2 );
      if (key_press.code == Keyboard::Add)
         zoomView( 1 , level_view->getCenter());
      if (key_press.code == Keyboard::Subtract)
         zoomView( -1 , level_view->getCenter());

      // Pause
      if (key_press.code == Keyboard::Space)
         togglePause();

      // TODO: Give orders via the keyboard
      if (key_press.code == Keyboard::Num0)
         playerAddCount( 0 );
      if (key_press.code == Keyboard::Num1)
         playerAddCount( 1 );
      if (key_press.code == Keyboard::Num2)
         playerAddCount( 2 );
      if (key_press.code == Keyboard::Num3)
         playerAddCount( 3 );
      if (key_press.code == Keyboard::Num4)
         playerAddCount( 4 );
      if (key_press.code == Keyboard::Num5)
         playerAddCount( 5 );
      if (key_press.code == Keyboard::Num6)
         playerAddCount( 6 );
      if (key_press.code == Keyboard::Num7)
         playerAddCount( 7 );
      if (key_press.code == Keyboard::Num8)
         playerAddCount( 8 );
      if (key_press.code == Keyboard::Num9)
         playerAddCount( 9 );

      // TODO: Make key-bindings something you can change/save

      return true;
   }
    
   virtual bool keyReleased( const Event::KeyEvent &key_release )
   {
      if (key_release.code == Keyboard::LShift || key_release.code == Keyboard::RShift)
         key_shift_down = 0;

      return true;
   }

   virtual bool mouseMoved( const Event::MouseMoveEvent &mouse_move )
   {
      static int old_mouse_x, old_mouse_y;

      if (left_mouse_down) {
         Vector2f old_spot = coordsWindowToView( old_mouse_x, old_mouse_y );
         Vector2f new_spot = coordsWindowToView( mouse_move.x, mouse_move.y );
         shiftView( old_spot.x - new_spot.x, old_spot.y - new_spot.y );
      }

      if ((right_mouse_down && cast_menu_open) || cast_menu_solid) {
         castMenuSelect( mouse_move.x, mouse_move.y );
      }

      old_mouse_x = mouse_move.x;
      old_mouse_y = mouse_move.y;

      return true;
   }

   virtual bool mouseButtonPressed( const Event::MouseButtonEvent &mbp )
   {
      if (isMouseOverGui( mbp.x, mbp.y ))
         return false;

      if (mbp.button == Mouse::Left && key_shift_down == 0) {
         left_mouse_down = 1;
         left_mouse_down_time = game_clock->getElapsedTime().asMilliseconds();

         if (cast_menu_solid) {
            castMenuSelect( mbp.x, mbp.y );
            castMenuChoose();
         }
      }

      if (mbp.button == Mouse::Right ||
         (mbp.button == Mouse::Left && key_shift_down == 1)) {
         right_mouse_down = 1;
         right_mouse_down_time = game_clock->getElapsedTime().asMilliseconds();
         castMenuCreate( coordsWindowToLevel( mbp.x, mbp.y ) );
      }


      return true;
   }

   virtual bool mouseButtonReleased( const Event::MouseButtonEvent &mbr )
   {
      if (left_mouse_down == 1) {
         left_mouse_down = 0;
         int left_mouse_up_time = game_clock->getElapsedTime().asMilliseconds();

         if (left_mouse_up_time - left_mouse_down_time < MOUSE_DOWN_SELECT_TIME
               && !isMouseOverGui( mbr.x, mbr.y ))
            selectUnit( coordsWindowToView( mbr.x, mbr.y ) );

      }
      if (right_mouse_down == 1) {
         right_mouse_down = 0;
         int right_mouse_up_time = game_clock->getElapsedTime().asMilliseconds();

         if (right_mouse_up_time - right_mouse_down_time < MOUSE_DOWN_SELECT_TIME)
            castMenuSolidify();
         else {
            castMenuSelect( mbr.x, mbr.y );
            castMenuChoose();
         }
      }

      return true;
   }

   virtual bool mouseWheelMoved( const Event::MouseWheelEvent &mwm )
   {
      zoomView( mwm.delta, coordsWindowToView( mwm.x, mwm.y ) );
      return true;
   }
};

struct LevelEditorEventHandler : public My_SFML_MouseListener, public My_SFML_KeyListener
{
   virtual bool keyPressed( const Event::KeyEvent &key_press )
   {
      if (key_press.code == Keyboard::Q)
         shutdown(1,1);

      if (key_press.code == Keyboard::LShift || key_press.code == Keyboard::RShift)
         key_shift_down = 1;

      // View movement
      if (key_press.code == Keyboard::Right)
         levelEditorChangeTerrain( 10 );
      if (key_press.code == Keyboard::Left)
         levelEditorChangeTerrain( -10 );
      if (key_press.code == Keyboard::Down)
         levelEditorChangeTerrain( 1 );
      if (key_press.code == Keyboard::Up)
         levelEditorChangeTerrain( -1 );
      if (key_press.code == Keyboard::Add)
         zoomView( 1 , level_view->getCenter());
      if (key_press.code == Keyboard::Subtract)
         zoomView( -1 , level_view->getCenter());

      if (key_press.code == Keyboard::W && (menu_state & MENU_PRI_LEVEL_EDITOR))
      {
         log("Writing new level data from level editor");
         levelEditorWriteToFile();
      }

      return true;
   }
    
   virtual bool keyReleased( const Event::KeyEvent &key_release )
   {
      if (key_release.code == Keyboard::LShift || key_release.code == Keyboard::RShift)
         key_shift_down = 0;

      return true;
   }

   virtual bool mouseMoved( const Event::MouseMoveEvent &mouse_move )
   {
      static int old_mouse_x, old_mouse_y;

      if (left_mouse_down) {
         Vector2f old_spot = coordsWindowToView( old_mouse_x, old_mouse_y );
         Vector2f new_spot = coordsWindowToView( mouse_move.x, mouse_move.y );
         shiftView( old_spot.x - new_spot.x, old_spot.y - new_spot.y );
      }

      if (right_mouse_down && cast_menu_open) {
         //castMenuSelect( mouse_move.x, mouse_move.y );
      }

      old_mouse_x = mouse_move.x;
      old_mouse_y = mouse_move.y;

      return true;
   }

   virtual bool mouseButtonPressed( const Event::MouseButtonEvent &mbp )
   {
      if (mbp.button == Mouse::Left && key_shift_down == 0) {
         left_mouse_down = 1;
         left_mouse_down_time = game_clock->getElapsedTime().asMilliseconds();
      }

      if (mbp.button == Mouse::Right ||
         (mbp.button == Mouse::Left && key_shift_down == 1)) {
         right_mouse_down = 1;
         right_mouse_down_time = game_clock->getElapsedTime().asMilliseconds();
      }


      return true;
   }

   virtual bool mouseButtonReleased( const Event::MouseButtonEvent &mbr )
   {
      if (mbr.button == Mouse::Left && key_shift_down == 0) {
         left_mouse_down = 0;
         int left_mouse_up_time = game_clock->getElapsedTime().asMilliseconds();

         if (left_mouse_up_time - left_mouse_down_time < MOUSE_DOWN_SELECT_TIME
               && !isMouseOverGui( mbr.x, mbr.y ))
            levelEditorSelectGrid( coordsWindowToView( mbr.x, mbr.y ) );

      }
      if (mbr.button == Mouse::Right ||
         (mbr.button == Mouse::Left && key_shift_down == 1)) {
         right_mouse_down = 0;
      }

      return true;
   }

   virtual bool mouseWheelMoved( const Event::MouseWheelEvent &mwm )
   {
      zoomView( mwm.delta, coordsWindowToView( mwm.x, mwm.y ) );
      return true;
   }
};

void setLevelListener( bool set )
{
   static LevelEventHandler level_listener;
   SFML_WindowEventManager& event_manager = SFML_WindowEventManager::getSingleton();
   if (set) {
      event_manager.addMouseListener( &level_listener, "level mouse" );
      event_manager.addKeyListener( &level_listener, "level key" );
   } else {
      event_manager.removeMouseListener( &level_listener );
      event_manager.removeKeyListener( &level_listener );
   }
}

void setLevelEditorListener( bool set )
{
   static LevelEditorEventHandler level_editor_listener;
   SFML_WindowEventManager& event_manager = SFML_WindowEventManager::getSingleton();
   if (set) {
      event_manager.addMouseListener( &level_editor_listener, "level editor mouse" );
      event_manager.addKeyListener( &level_editor_listener, "level editor key" );
   } else {
      event_manager.removeMouseListener( &level_editor_listener );
      event_manager.removeKeyListener( &level_editor_listener );
   }
}


};
