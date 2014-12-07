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
#define FULLY_PAUSED 20

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
list<Projectile*> projectile_list;

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

int removeProjectile( Projectile *p )
{
   if (p) {
      list<Projectile*>::iterator it= find( projectile_list.begin(), projectile_list.end(), p );
      projectile_list.erase( it );
      return 0;
   }
   return -1;
}

int addProjectile( Projectile_Type t, int team, float x, float y, float speed, Unit* target )
{
   Projectile *p = genProjectile( t, team, x, y, speed, target );

   if (p) {
      projectile_list.push_back(p);
      return 0;
   }
   return -1;
}

//////////////////////////////////////////////////////////////////////
// Vision ---

bool blocksVision( int from_x, int from_y, int to_x, int to_y, Direction ew, Direction ns, int flags )
{
   Terrain t = GRID_AT(terrain_grid,from_x,from_y);

   if (t == TER_TREE1 || t == TER_TREE2)
      return true;

   // Cliffs
   if ((t == CLIFF_SOUTH || t == CLIFF_SOUTH_EAST_EDGE || t == CLIFF_SOUTH_WEST_EDGE)
      && ns == NORTH && (to_y < from_y))
      return true;
   if ((t == CLIFF_NORTH || t == CLIFF_NORTH_EAST_EDGE || t == CLIFF_NORTH_WEST_EDGE)
      && ns == SOUTH && (to_y > from_y))
      return true;
   if ((t == CLIFF_EAST || t == CLIFF_EAST_NORTH_EDGE || t == CLIFF_EAST_SOUTH_EDGE)
      && ew == WEST && (to_x < from_x))
      return true;
   if ((t == CLIFF_WEST || t == CLIFF_WEST_SOUTH_EDGE || t == CLIFF_WEST_NORTH_EDGE)
      && ew == EAST && (to_x > from_x))
      return true;
   // Cliff corners
   if ((t == CLIFF_CORNER_SOUTHEAST_90 || t == CLIFF_CORNER_SOUTHEAST_270)
         && ns == NORTH && ew == WEST)
      return true;
   if ((t == CLIFF_CORNER_SOUTHWEST_90 || t == CLIFF_CORNER_SOUTHWEST_270)
         && ns == NORTH && ew == EAST)
      return true;
   if ((t == CLIFF_CORNER_NORTHWEST_90 || t == CLIFF_CORNER_NORTHWEST_270)
         && ns == SOUTH && ew == EAST)
      return true;
   if ((t == CLIFF_CORNER_NORTHEAST_90 || t == CLIFF_CORNER_NORTHEAST_270)
         && ns == SOUTH && ew == WEST)
      return true;


   // TODO: everything else
   return false;
}


int calculatePointVision( int start_x, int start_y, int end_x, int end_y, int flags, Vision *v_grid )
{
   /* Eight quadrants:
    * \2|3/
    * 1\|/4
    * --s-- 
    * 5/|\8
    * /6|7\
    */

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

   int previous_x = start_x, previous_y = start_y;
   
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

         // Check if vision to this square is obstructed by the last square
         if (blocksVision(previous_x, previous_y, x, y, dir1, dir2, flags)) {
            return -1; // Vision obstructed
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

         // Check possible vision-obstructors
         if (blocksVision(previous_x, previous_y, x, y, dir1, dir2, flags)) {
            return -1; // Vision obstructed beyond here
         }

         // Otherwise, it's visible, move on
         GRID_AT(v_grid,x,y) = VIS_VISIBLE;
         previous_x = x;
         previous_y = y;
      }
   }
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
      if (blocksVision(x, y - dy, x, y, ALL_DIR, dir, flags)) {
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
      if (blocksVision(x - dx, y, x, y, dir, ALL_DIR, flags)) {
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

   float dydx;
   if (start_x == end_x)
      dydx = 100000000;
   else
      dydx = ((float)(start_y - end_y)) / ((float)(start_x - end_x));

   int quadrant;
   if (end_x < start_x) {
      if (dydx >= 1 || dydx <= -1)
         quadrant = 2;
      else
         quadrant = 1;
   } else {
      if (dydx >= 1 || dydx <= -1)
         quadrant = 3;
      else
         quadrant = 4;
   }
   if (end_y > start_y)
      quadrant += 4;

   if (quadrant == 1 || quadrant == 2) 
      return calculatePointVision( start_x-1, start_y-1, end_x-1, end_y-1, flags, v_grid );
   if (quadrant == 3 || quadrant == 4) 
      return calculatePointVision( start_x+1, start_y-1, end_x+1, end_y-1, flags, v_grid );
   if (quadrant == 5 || quadrant == 6) 
      return calculatePointVision( start_x-1, start_y+1, end_x-1, end_y+1, flags, v_grid );
   if (quadrant == 7 || quadrant == 8) 
      return calculatePointVision( start_x+1, start_y+1, end_x+1, end_y+1, flags, v_grid );

   // We must have seen it to get this result
   GRID_AT(v_grid,end_x,end_y) = VIS_VISIBLE;
   return 0;
}

int calculateUnitVision( Unit *unit )
{
   int x, y;
   if (unit) {
      
      Vision *v_grid = vision_grid;
      if (unit->team != 0)
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
   int x, y;
   for (x = 0; x < level_dim_x; ++x) {
      for (y = 0; y < level_dim_y; ++y) {
         if (GRID_AT(vision_grid,x,y) >= VIS_SEEN_BEFORE)
            GRID_AT(vision_grid,x,y) = VIS_SEEN_BEFORE;
         else
            GRID_AT(vision_grid,x,y) = VIS_NEVER_SEEN;
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

Unit* getEnemy( int x, int y, float range, Direction dir, int my_team, int selector)
{
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
         if (u && u->team != my_team && u->alive) {
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
   if (t >= CLIFF_SOUTH && t <= CLIFF_CORNER_NORTHWEST_270)
      return false; // cliffs impassable

   return true;
}

set<Unit*> unit_collision_set;

Unit* unitIncoming( int to_x, int to_y, int from_x, int from_y )
{
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

int playerAddConditional( Order_Conditional c )
{
   order_prep_cond = c;

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

   addUnit( u );

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

   // Get search box
   int x = player->x_grid, y = player->y_grid;
   int shout_range = (int)player->attack_range + 1;
   int min_x, max_x, min_y, max_y;
   min_x = x - shout_range;
   max_x = x + shout_range;
   min_y = y - shout_range;
   max_y = y + shout_range;
   if (min_x < 0) min_x = 0;
   if (max_x >= level_dim_x) max_x = level_dim_x - 1;
   if (min_y < 0) min_y = 0;
   if (max_y >= level_dim_y) max_y = level_dim_y - 1;

   float range_squared = player->attack_range * player->attack_range;

   for (int j = min_y; j <= max_y; ++j) {
      for (int i = min_x; i <= max_x; ++i) {
         Unit *u = GRID_AT(unit_grid,i,j);
         if (u == player) continue;

         if (u && u->team == 0) { // On my team
            float u_x = u->x_grid - x, u_y = u->y_grid - y;
            float u_squared = (u_x * u_x) + (u_y * u_y);
            // Is it really in range?
            if (u_squared <= range_squared) {

               // Okay so it's in shout range, now what?
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
   // Get search box
   int x = player->x_grid, y = player->y_grid;
   int shout_range = (int)player->attack_range + 1;
   int min_x, max_x, min_y, max_y;
   min_x = x - shout_range;
   max_x = x + shout_range;
   min_y = y - shout_range;
   max_y = y + shout_range;
   if (min_x < 0) min_x = 0;
   if (max_x >= level_dim_x) max_x = level_dim_x - 1;
   if (min_y < 0) min_y = 0;
   if (max_y >= level_dim_y) max_y = level_dim_y - 1;

   float range_squared = player->attack_range * player->attack_range;

   for (int j = min_y; j <= max_y; ++j) {
      for (int i = min_x; i <= max_x; ++i) {
         Unit *u = GRID_AT(unit_grid,i,j);
         if (u == player) continue;

         if (u && u->team == 0) { // On my team
            float u_x = u->x_grid - x, u_y = u->y_grid - y;
            float u_squared = (u_x * u_x) + (u_y * u_y);
            // Is it really in range?
            if (u_squared <= range_squared) {

               // Okay so it's in shout range, now what?
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
         log("UNIT");
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
      case PL_CAST_QUAKE:
      case PL_CAST_TIMELOCK:
      case PL_CAST_SCRY:
      case PL_CAST_TELEPATHY:
         log("Spells not implemented");
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
   SFML_TextureManager &t_manager = SFML_TextureManager::getSingleton();
   // Setup terrain
   terrain_sprites = new Sprite*[NUM_TERRAINS];

   terrain_sprites[TER_NONE] = NULL;
   terrain_sprites[TER_TREE1] = new Sprite( *(t_manager.getTexture( "BasicTree1.png" )));
   normalizeTo1x1( terrain_sprites[TER_TREE1] );
   terrain_sprites[TER_TREE2] = new Sprite( *(t_manager.getTexture( "BasicTree2.png" )));
   normalizeTo1x1( terrain_sprites[TER_TREE2] );

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
   projectile_list.clear();
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

      //GRID_AT(terrain_grid,0,0) = TER_NONE;

 //     player->x_grid = 1;
//      player->y_grid = 4;

  //    writeLevelToFile( "res/testlevel.txt" );
   }

   menu_state = MENU_MAIN | MENU_PRI_INGAME;
   setLevelListener(true);
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
   else
      unpause();
}

void drawPause()
{
   if (pause_state == 0) return;

   int transparency = (128 * abs(pause_state)) / FULLY_PAUSED;
   Color c( 128, 128, 128, transparency );
   RectangleShape r( Vector2f(config::width(), config::height()) );
   r.setFillColor( c );
   r.setPosition( 0, 0 );
   SFML_GlobalRenderWindow::get()->draw( r );
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
   for (list<Projectile*>::iterator it=projectile_list.begin(); it != projectile_list.end(); ++it)
   {
      Projectile* proj = (*it);
      if (proj) {
         result = proj->update( dtf );
         if (result == 1) {
            // Delete the projectile
            delete proj;
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

// Right Click Menu

int r_click_menu_x_grid, r_click_menu_y_grid;
int r_click_menu_top_x_left, r_click_menu_top_y_bottom; // Top box
int r_click_menu_bottom_x_left, r_click_menu_bottom_y_top; // Bottom box
float r_click_menu_button_size_x, r_click_menu_button_size_y;

int r_click_menu_selection = 0; // -num == top box, +num == bottom box

bool r_click_menu_open = false;
bool r_click_menu_solid = false;

void initRightClickMenu()
{
   r_click_menu_open = false;
   r_click_menu_solid = false;
}

int rightClickMenuCreate( int x, int y )
{
   Vector2i grid = coordsWindowToLevel( x, y );

   if (grid.x < 0 || grid.x >= level_dim_x || grid.y < 0 || grid.y >= level_dim_y)
      return -1;
   
   // TODO: do something so that the menu is always visible

   r_click_menu_x_grid = grid.x;
   r_click_menu_y_grid = grid.y;

   Vector2u top_left = coordsViewToWindow( grid.x, grid.y );
   Vector2u bottom_left = coordsViewToWindow( grid.x, grid.y + 1 );

   r_click_menu_top_x_left = top_left.x;
   r_click_menu_top_y_bottom = top_left.y;

   r_click_menu_bottom_x_left = bottom_left.x;
   r_click_menu_bottom_y_top = bottom_left.y;

   r_click_menu_selection = 0;

   // Calculate how big to make the buttons
   float x_dim = config::width(), y_dim = config::height();
   r_click_menu_button_size_x = x_dim / 6;
   r_click_menu_button_size_y = y_dim / 30;

   r_click_menu_open = true;
   r_click_menu_solid = false;

   log("Right click menu created");

   return 0;
}

int rightClickMenuSelect( int x, int y )
{
   if (x < r_click_menu_top_x_left || 
       x > (r_click_menu_top_x_left + r_click_menu_button_size_x) ||
       !( (y <= r_click_menu_top_y_bottom && 
           y >= r_click_menu_top_y_bottom - (r_click_menu_button_size_y * 5))
          ||
          (y >= r_click_menu_bottom_y_top && 
           y <= r_click_menu_bottom_y_top + (r_click_menu_button_size_y * 5))
        )
      )
   {
      // Not in a selection area
      r_click_menu_selection = 0;
   } 
   else if (y <= r_click_menu_top_y_bottom && 
       y >= r_click_menu_top_y_bottom - (r_click_menu_button_size_y * 5))
   {
      // Top selection area
      r_click_menu_selection = ((int) ((y - r_click_menu_top_y_bottom) / r_click_menu_button_size_y)) - 1;
   }
   else
   {
      // Bottom selection area
      r_click_menu_selection = (int) ((y - r_click_menu_bottom_y_top) / r_click_menu_button_size_y);
   }
   return r_click_menu_selection;
}

void rightClickMenuSolidify()
{ 
   r_click_menu_solid = true;
}

void rightClickMenuClose()
{
   r_click_menu_open = false;
}

void rightClickMenuChoose()
{
   Order o( SKIP, TRUE, r_click_menu_x_grid );
   o.iteration = r_click_menu_y_grid;

   switch (r_click_menu_selection) {
      case -1:
         o.action = SUMMON_MONSTER;
         player->addOrder( o ); 
         break;
      case -2:
         o.action = SUMMON_SOLDIER;
         player->addOrder( o ); 
         break;
      case -3:
         o.action = SUMMON_WORM;
         player->addOrder( o ); 
         break;
      case -4:
         o.action = SUMMON_BIRD;
         player->addOrder( o ); 
         break;
      case -5:
         o.action = SUMMON_BUG;
         player->addOrder( o ); 
         break;
   }

   rightClickMenuClose();
}

void drawRightClickMenu()
{
   if (r_click_menu_open == false) return;
   RenderWindow *gui_window = SFML_GlobalRenderWindow::get();

   Text text;
   text.setFont( *menu_font );
   text.setColor( Color::Black );
   text.setCharacterSize( 16 );

   text.setString( String("Summon Monster") );
   text.setPosition( r_click_menu_top_x_left, r_click_menu_top_y_bottom - (r_click_menu_button_size_y) );
   gui_window->draw( text );
   text.setString( String("Summon Soldier") );
   text.setPosition( r_click_menu_top_x_left, r_click_menu_top_y_bottom - (2*r_click_menu_button_size_y) );
   gui_window->draw( text );
   text.setString( String("Summon Worm") );
   text.setPosition( r_click_menu_top_x_left, r_click_menu_top_y_bottom - (3*r_click_menu_button_size_y) );
   gui_window->draw( text );
   text.setString( String("Summon Bird") );
   text.setPosition( r_click_menu_top_x_left, r_click_menu_top_y_bottom - (4*r_click_menu_button_size_y) );
   gui_window->draw( text );
   text.setString( String("Summon Bug") );
   text.setPosition( r_click_menu_top_x_left, r_click_menu_top_y_bottom - (5*r_click_menu_button_size_y) );
   gui_window->draw( text );

   text.setString( String("Cast Scry") );
   text.setPosition( r_click_menu_bottom_x_left, r_click_menu_bottom_y_top);
   gui_window->draw( text );
   text.setString( String("Cast Lightning") );
   text.setPosition( r_click_menu_bottom_x_left, r_click_menu_bottom_y_top + (1*r_click_menu_button_size_y) );
   gui_window->draw( text );
   text.setString( String("Cast Quake") );
   text.setPosition( r_click_menu_bottom_x_left, r_click_menu_bottom_y_top + (2*r_click_menu_button_size_y) );
   gui_window->draw( text );
   text.setString( String("Cast Heal") );
   text.setPosition( r_click_menu_bottom_x_left, r_click_menu_bottom_y_top + (3*r_click_menu_button_size_y) );
   gui_window->draw( text );
   text.setString( String("Cast Timelock") );
   text.setPosition( r_click_menu_bottom_x_left, r_click_menu_bottom_y_top + (4*r_click_menu_button_size_y) );
   gui_window->draw( text );
   text.setString( String("Cast Telepathy") );
   text.setPosition( r_click_menu_bottom_x_left, r_click_menu_bottom_y_top + (5*r_click_menu_button_size_y) );
   gui_window->draw( text );

   RectangleShape grid_select_rect;
   float rect_size = r_click_menu_bottom_y_top - r_click_menu_top_y_bottom;
   grid_select_rect.setSize( Vector2f( rect_size, rect_size ) );
   grid_select_rect.setPosition( r_click_menu_top_x_left, r_click_menu_top_y_bottom );
   grid_select_rect.setFillColor( Color::Transparent );
   grid_select_rect.setOutlineColor( Color::White );
   grid_select_rect.setOutlineThickness( 1.0 );
   gui_window->draw( grid_select_rect );
}

// The order buttons part of the gui

int selection_box_width = 240, selection_box_height = 50;

bool init_level_gui = false;
IMImageButton *b_o_move_forward,
              *b_o_move_backward,
              *b_o_turn_north,
              *b_o_turn_east,
              *b_o_turn_south,
              *b_o_turn_west,
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
              *b_pl_cmd_go_all;
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

bool isMouseOverGui( int x, int y )
{
   int width = config::width(),
       height = config::height();

   int sec_buffer = 5;
   float button_size = (((float)width) - (20.0 * sec_buffer)) / 20.0;

   if (y > (height - ((sec_buffer * 2) + (button_size * 3))))
      return true; // All button areas are this tall

   if (y > (height - ((sec_buffer * 2) + (button_size * 4)))
         && (x > width / 2))
      return true; // Unit specialty areas are taller

   if (y > (height - ((sec_buffer * 6) + (button_size * 4)))
         && x < ((sec_buffer * 2) + (button_size * 5)))
      return true; // Player command area is tallest of all

   if (selected_unit && (y < selection_box_height) && (x > width - selection_box_width))
      return true; // Over the selected unit area

   return false;
}

void fitGui_Level()
{
   int width = config::width(),
       height = config::height();

   int sec_buffer = 5;
   float button_size = (((float)width) - (20.0 * sec_buffer)) / 20.0;

   int text_size = floor( button_size / 2);

   float sec_start_numpad = 0,
       sec_start_conditionals = sec_start_numpad + (sec_buffer * 2) + (button_size * 3),
       sec_start_movement = sec_start_conditionals + (sec_buffer * 2) + (button_size * 1),
       sec_start_attack = sec_start_movement + (sec_buffer * 2) + (button_size * 3),
       sec_start_control = sec_start_attack + (sec_buffer * 2) + (button_size * 2),
       sec_start_monster = sec_start_control + (sec_buffer * 2) + (button_size * 1),
       sec_start_soldier = sec_start_monster + (sec_buffer * 2) + (button_size * 1),
       sec_start_worm = sec_start_soldier + (sec_buffer * 2) + (button_size * 2),
       sec_start_bird = sec_start_worm + (sec_buffer * 2) + (button_size * 2),
       sec_start_bug = sec_start_bird + (sec_buffer * 2) + (button_size * 2);

   // Numpad

   b_numpad_area->setSize( (sec_buffer * 3) + (button_size * 3),
                           (sec_buffer * 3) + (button_size * 4));
   b_numpad_area->setPosition( sec_start_numpad - sec_buffer,
                               height - ((sec_buffer * 2) + (button_size * 4)));
   
   b_count_0->setSize( button_size, button_size );
   b_count_0->setPosition( sec_start_numpad + sec_buffer + button_size,
                           height - (sec_buffer + button_size));
   b_count_0->setTextSize( text_size );
   b_count_0->centerText();
   
   b_count_1->setSize( button_size, button_size );
   b_count_1->setPosition( sec_start_numpad + sec_buffer,
                           height - (sec_buffer + button_size * 4) );
   b_count_1->setTextSize( text_size );
   b_count_1->centerText();

   b_count_2->setSize( button_size, button_size );
   b_count_2->setPosition( sec_start_numpad + sec_buffer + button_size,
                           height - (sec_buffer + button_size * 4) );
   b_count_2->setTextSize( text_size );
   b_count_2->centerText();

   b_count_3->setSize( button_size, button_size );
   b_count_3->setPosition( sec_start_numpad + sec_buffer + (button_size * 2),
                           height - (sec_buffer + button_size * 4) );
   b_count_3->setTextSize( text_size );
   b_count_3->centerText();

   b_count_4->setSize( button_size, button_size );
   b_count_4->setPosition( sec_start_numpad + sec_buffer,
                           height - (sec_buffer + button_size * 3) );
   b_count_4->setTextSize( text_size );
   b_count_4->centerText();

   b_count_5->setSize( button_size, button_size );
   b_count_5->setPosition( sec_start_numpad + sec_buffer + button_size,
                           height - (sec_buffer + button_size * 3) );
   b_count_5->setTextSize( text_size );
   b_count_5->centerText();

   b_count_6->setSize( button_size, button_size );
   b_count_6->setPosition( sec_start_numpad + sec_buffer + (button_size * 2),
                           height - (sec_buffer + button_size * 3) );
   b_count_6->setTextSize( text_size );
   b_count_6->centerText();

   b_count_7->setSize( button_size, button_size );
   b_count_7->setPosition( sec_start_numpad + sec_buffer,
                           height - (sec_buffer + button_size * 2) );
   b_count_7->setTextSize( text_size );
   b_count_7->centerText();

   b_count_8->setSize( button_size, button_size );
   b_count_8->setPosition( sec_start_numpad + sec_buffer + button_size,
                           height - (sec_buffer + button_size * 2) );
   b_count_8->setTextSize( text_size );
   b_count_8->centerText();

   b_count_9->setSize( button_size, button_size );
   b_count_9->setPosition( sec_start_numpad + sec_buffer + (button_size * 2),
                           height - (sec_buffer + button_size * 2) );
   b_count_9->setTextSize( text_size );
   b_count_9->centerText();

   b_count_infinite->setSize( button_size, button_size );
   b_count_infinite->setPosition( sec_start_numpad + sec_buffer + (button_size * 2),
                                  height - (sec_buffer + button_size) );
   b_count_infinite->setTextSize( text_size );
   b_count_infinite->centerText();

   b_count_reset->setSize( button_size, button_size );
   b_count_reset->setPosition( sec_start_numpad + sec_buffer,
                                  height - (sec_buffer + button_size) );
   b_count_reset->setTextSize( text_size );
   b_count_reset->centerText();

   count_text->setPosition( sec_start_conditionals + sec_buffer,
                            height - ((sec_buffer * 3) + (button_size * 4)));
   count_text->setCharacterSize( text_size );

   // Player commands

   b_pl_cmd_area->setSize( (sec_buffer * 3) + (button_size * 3),
                           (sec_buffer * 3) + (button_size * 1));
   b_pl_cmd_area->setPosition( sec_start_numpad - sec_buffer,
                               height - ((sec_buffer * 4) + (button_size * 5)));

   b_pl_alert_all->setSize( button_size, button_size );
   b_pl_alert_all->setPosition( sec_start_numpad + sec_buffer,
                                height - ((sec_buffer * 3) + (button_size * 5)) );

   b_pl_cmd_go->setSize( button_size, button_size );
   b_pl_cmd_go->setPosition( sec_start_numpad + sec_buffer + button_size,
                             height - ((sec_buffer * 3) + (button_size * 5)) );

   b_pl_cmd_go_all->setSize( button_size, button_size );
   b_pl_cmd_go_all->setPosition( sec_start_numpad + sec_buffer + (button_size * 2),
                                 height - ((sec_buffer * 3) + (button_size * 5)) );

   // Conditionals

   b_conditional_area->setSize( (sec_buffer * 3) + (button_size * 1),
                                (sec_buffer * 3) + (button_size * 3));
   b_conditional_area->setPosition( sec_start_conditionals - sec_buffer,
                                    height - ((sec_buffer * 2) + (button_size * 3)));

   // Movement

   b_movement_area->setSize( (sec_buffer * 3) + (button_size * 3),
                             (sec_buffer * 3) + (button_size * 3));
   b_movement_area->setPosition( sec_start_movement - sec_buffer,
                                 height - ((sec_buffer * 2) + (button_size * 3)));

   b_o_move_forward->setSize( button_size, button_size );
   b_o_move_forward->setPosition( sec_start_movement + sec_buffer + (button_size * 2),
                                  height - (sec_buffer + (button_size * 3)));

   b_o_move_backward->setSize( button_size, button_size );
   b_o_move_backward->setPosition( sec_start_movement + sec_buffer,
                                   height - (sec_buffer + (button_size * 3)));

   b_o_wait->setSize( button_size, button_size );
   b_o_wait->setPosition( sec_start_movement + sec_buffer + button_size,
                          height - (sec_buffer + (button_size * 3)));

   b_o_turn_north->setSize( button_size, button_size );
   b_o_turn_north->setPosition( sec_start_movement + sec_buffer + button_size,
                                height - (sec_buffer + (button_size * 2)));

   b_o_turn_east->setSize( button_size, button_size );
   b_o_turn_east->setPosition( sec_start_movement + sec_buffer + (button_size * 2),
                               height - (sec_buffer + (button_size * 1)));

   b_o_turn_south->setSize( button_size, button_size );
   b_o_turn_south->setPosition( sec_start_movement + sec_buffer + button_size,
                                height - (sec_buffer + (button_size * 1)));

   b_o_turn_west->setSize( button_size, button_size );
   b_o_turn_west->setPosition( sec_start_movement + sec_buffer,
                               height - (sec_buffer + (button_size * 1)));

   // Attack

   b_attack_area->setSize( (sec_buffer * 3) + (button_size * 2),
                           (sec_buffer * 3) + (button_size * 3));
   b_attack_area->setPosition( sec_start_attack - sec_buffer,
                               height - ((sec_buffer * 2) + (button_size * 3)));

   b_o_attack_smallest->setSize( button_size, button_size );
   b_o_attack_smallest->setPosition( sec_start_attack + sec_buffer,
                                     height - (sec_buffer + (button_size * 2)));

   b_o_attack_biggest->setSize( button_size, button_size );
   b_o_attack_biggest->setPosition( sec_start_attack + sec_buffer + button_size,
                                    height - (sec_buffer + (button_size * 2)));

   b_o_attack_closest->setSize( button_size, button_size );
   b_o_attack_closest->setPosition( sec_start_attack + sec_buffer,
                                    height - (sec_buffer + (button_size * 1)));

   b_o_attack_farthest->setSize( button_size, button_size );
   b_o_attack_farthest->setPosition( sec_start_attack + sec_buffer + button_size,
                                     height - (sec_buffer + (button_size * 1)));

   // Control

   b_control_area->setSize( (sec_buffer * 3) + (button_size * 1),
                           (sec_buffer * 3) + (button_size * 3));
   b_control_area->setPosition( sec_start_control - sec_buffer,
                                height - ((sec_buffer * 2) + (button_size * 3)));

   b_o_start_block->setSize( button_size, button_size );
   b_o_start_block->setPosition( sec_start_control + sec_buffer,
                                 height - (sec_buffer + (button_size * 3)));

   b_o_end_block->setSize( button_size, button_size );
   b_o_end_block->setPosition( sec_start_control + sec_buffer,
                               height - (sec_buffer + (button_size * 2)));

   b_o_repeat->setSize( button_size, button_size );
   b_o_repeat->setPosition( sec_start_control + sec_buffer,
                            height - (sec_buffer + (button_size * 1)));

   // Units

   // Monster
   b_monster_area->setSize( (sec_buffer * 3) + (button_size * 1),
                           (sec_buffer * 3) + (button_size * 4));
   b_monster_area->setPosition( sec_start_monster,
                                height - ((sec_buffer * 2) + (button_size * 4)));

   // Soldier
   b_soldier_area->setSize( (sec_buffer * 3) + (button_size * 2),
                           (sec_buffer * 3) + (button_size * 4));
   b_soldier_area->setPosition( sec_start_soldier,
                                height - ((sec_buffer * 2) + (button_size * 4)));

   // Worm
   b_worm_area->setSize( (sec_buffer * 3) + (button_size * 2),
                           (sec_buffer * 3) + (button_size * 4));
   b_worm_area->setPosition( sec_start_worm,
                                height - ((sec_buffer * 2) + (button_size * 4)));

   // Bird
   b_bird_area->setSize( (sec_buffer * 3) + (button_size * 2),
                           (sec_buffer * 3) + (button_size * 4));
   b_bird_area->setPosition( sec_start_bird,
                                height - ((sec_buffer * 2) + (button_size * 4)));

   // Bug
   b_bug_area->setSize( (sec_buffer * 3) + (button_size * 3),
                           (sec_buffer * 3) + (button_size * 4));
   b_bug_area->setPosition( sec_start_bug,
                                height - ((sec_buffer * 2) + (button_size * 4)));


   view_rel_x_to_y = ((float)height) / ((float)width);
}

int initLevelGui()
{
   SFML_TextureManager &t_manager = SFML_TextureManager::getSingleton();
   IMGuiManager &gui_manager = IMGuiManager::getSingleton();

   // These should be ImageButtons with the border changing around the central image
   // but for now they can be unchanging regular buttons.

   b_o_move_forward = new IMImageButton();
   b_o_move_forward->setAllTextures( t_manager.getTexture( "OrderMoveForward.png" ) );
   b_o_move_forward->setPressedTexture( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_o_move_forward->setImage( NULL );
   gui_manager.registerWidget( "Order: move forward", b_o_move_forward);

   b_o_move_backward = new IMImageButton();
   b_o_move_backward->setAllTextures( t_manager.getTexture( "OrderMoveBackward.png" ) );
   b_o_move_backward->setImage( NULL );
   gui_manager.registerWidget( "Order: move backward", b_o_move_backward);

   b_o_turn_north = new IMImageButton();
   b_o_turn_north->setAllTextures( t_manager.getTexture( "OrderTurnNorth.png" ) );
   b_o_turn_north->setImage( NULL );
   gui_manager.registerWidget( "Order: turn north", b_o_turn_north);

   b_o_turn_east = new IMImageButton();
   b_o_turn_east->setAllTextures( t_manager.getTexture( "OrderTurnEast.png" ) );
   b_o_turn_east->setImage( NULL );
   gui_manager.registerWidget( "Order: turn east", b_o_turn_east);

   b_o_turn_south = new IMImageButton();
   b_o_turn_south->setAllTextures( t_manager.getTexture( "OrderTurnSouth.png" ) );
   b_o_turn_south->setImage( NULL );
   gui_manager.registerWidget( "Order: turn south", b_o_turn_south);

   b_o_turn_west = new IMImageButton();
   b_o_turn_west->setAllTextures( t_manager.getTexture( "OrderTurnWest.png" ) );
   b_o_turn_west->setImage( NULL );
   gui_manager.registerWidget( "Order: turn west", b_o_turn_west);

   b_o_attack_smallest = new IMImageButton();
   b_o_attack_smallest->setAllTextures( t_manager.getTexture( "OrderAttackSmallest.png" ) );
   b_o_attack_smallest->setImage( NULL );
   gui_manager.registerWidget( "Order: attack smallest", b_o_attack_smallest);

   b_o_attack_biggest = new IMImageButton();
   b_o_attack_biggest->setAllTextures( t_manager.getTexture( "OrderAttackBiggest.png" ) );
   b_o_attack_biggest->setImage( NULL );
   gui_manager.registerWidget( "Order: attack biggest", b_o_attack_biggest);

   b_o_attack_closest = new IMImageButton();
   b_o_attack_closest->setAllTextures( t_manager.getTexture( "OrderAttackClosest.png" ) );
   b_o_attack_closest->setImage( NULL );
   gui_manager.registerWidget( "Order: attack closest", b_o_attack_closest);

   b_o_attack_farthest = new IMImageButton();
   b_o_attack_farthest->setAllTextures( t_manager.getTexture( "OrderAttackFarthest.png" ) );
   b_o_attack_farthest->setImage( NULL );
   gui_manager.registerWidget( "Order: attack farthest", b_o_attack_farthest);

   b_o_start_block = new IMImageButton();
   b_o_start_block->setAllTextures( t_manager.getTexture( "ControlStartBlock.png" ) );
   b_o_start_block->setImage( NULL );
   gui_manager.registerWidget( "Control: start block", b_o_start_block);

   b_o_end_block = new IMImageButton();
   b_o_end_block->setAllTextures( t_manager.getTexture( "ControlEndBlock.png" ) );
   b_o_end_block->setImage( NULL );
   gui_manager.registerWidget( "Control: end block", b_o_end_block);

   b_o_repeat = new IMImageButton();
   b_o_repeat->setAllTextures( t_manager.getTexture( "ControlRepeat.png" ) );
   b_o_repeat->setImage( NULL );
   gui_manager.registerWidget( "Control: repeat", b_o_repeat);

   b_o_wait = new IMImageButton();
   b_o_wait->setAllTextures( t_manager.getTexture( "OrderWait.png" ) );
   b_o_wait->setImage( NULL );
   gui_manager.registerWidget( "Order: wait", b_o_wait);

   b_pl_alert_all = new IMImageButton();
   b_pl_alert_all->setAllTextures( t_manager.getTexture( "PlayerAlert.png" ) );
   b_pl_alert_all->setImage( NULL );
   gui_manager.registerWidget( "Player: alert all", b_pl_alert_all);

   b_pl_cmd_go = new IMImageButton();
   b_pl_cmd_go->setAllTextures( t_manager.getTexture( "PlayerGoButton.png" ) );
   b_pl_cmd_go->setImage( NULL );
   gui_manager.registerWidget( "Player: command go", b_pl_cmd_go);

   b_pl_cmd_go_all = new IMImageButton();
   b_pl_cmd_go_all->setAllTextures( t_manager.getTexture( "PlayerGoAllButton.png" ) );
   b_pl_cmd_go_all->setImage( NULL );
   gui_manager.registerWidget( "Player: command go all", b_pl_cmd_go_all);

   b_count_1 = new IMTextButton();
   b_count_1->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_1->setText( &s_1 );
   b_count_1->setTextColor( Color::Black );
   b_count_1->setFont( menu_font );
   gui_manager.registerWidget( "Count: 1", b_count_1);

   b_count_2 = new IMTextButton();
   b_count_2->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_2->setText( &s_2 );
   b_count_2->setTextColor( Color::Black );
   b_count_2->setFont( menu_font );
   gui_manager.registerWidget( "Count: 2", b_count_2);

   b_count_3 = new IMTextButton();
   b_count_3->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_3->setText( &s_3 );
   b_count_3->setTextColor( Color::Black );
   b_count_3->setFont( menu_font );
   gui_manager.registerWidget( "Count: 3", b_count_3);

   b_count_4 = new IMTextButton();
   b_count_4->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_4->setText( &s_4 );
   b_count_4->setTextColor( Color::Black );
   b_count_4->setFont( menu_font );
   gui_manager.registerWidget( "Count: 4", b_count_4);

   b_count_5 = new IMTextButton();
   b_count_5->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_5->setText( &s_5 );
   b_count_5->setTextColor( Color::Black );
   b_count_5->setFont( menu_font );
   gui_manager.registerWidget( "Count: 5", b_count_5);

   b_count_6 = new IMTextButton();
   b_count_6->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_6->setText( &s_6 );
   b_count_6->setTextColor( Color::Black );
   b_count_6->setFont( menu_font );
   gui_manager.registerWidget( "Count: 6", b_count_6);

   b_count_7 = new IMTextButton();
   b_count_7->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_7->setText( &s_7 );
   b_count_7->setTextColor( Color::Black );
   b_count_7->setFont( menu_font );
   gui_manager.registerWidget( "Count: 7", b_count_7);

   b_count_8 = new IMTextButton();
   b_count_8->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_8->setText( &s_8 );
   b_count_8->setTextColor( Color::Black );
   b_count_8->setFont( menu_font );
   gui_manager.registerWidget( "Count: 8", b_count_8);

   b_count_9 = new IMTextButton();
   b_count_9->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_9->setText( &s_9 );
   b_count_9->setTextColor( Color::Black );
   b_count_9->setFont( menu_font );
   gui_manager.registerWidget( "Count: 9", b_count_9);

   b_count_0 = new IMTextButton();
   b_count_0->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_0->setText( &s_0 );
   b_count_0->setTextColor( Color::Black );
   b_count_0->setFont( menu_font );
   gui_manager.registerWidget( "Count: 0", b_count_0);

   b_count_infinite = new IMTextButton();
   b_count_infinite->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
   b_count_infinite->setText( &s_infinite );
   b_count_infinite->setTextColor( Color::Black );
   b_count_infinite->setFont( menu_font );
   gui_manager.registerWidget( "Count: infinite", b_count_infinite);

   b_count_reset = new IMTextButton();
   b_count_reset->setAllTextures( t_manager.getTexture( "OrderButtonBase.png" ) );
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
   b_monster_area->setAllTextures( t_manager.getTexture( "UICenterGrey.png" ) );
   b_monster_area->setCornerAllTextures( t_manager.getTexture( "UICornerGrey3px.png" ) );
   b_monster_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeGrey3px.png" ) );
   b_monster_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Monster area", b_monster_area);

   b_soldier_area = new IMEdgeButton();
   b_soldier_area->setAllTextures( t_manager.getTexture( "UICenterGrey.png" ) );
   b_soldier_area->setCornerAllTextures( t_manager.getTexture( "UICornerGrey3px.png" ) );
   b_soldier_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeGrey3px.png" ) );
   b_soldier_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Soldier area", b_soldier_area);

   b_worm_area = new IMEdgeButton();
   b_worm_area->setAllTextures( t_manager.getTexture( "UICenterGrey.png" ) );
   b_worm_area->setCornerAllTextures( t_manager.getTexture( "UICornerGrey3px.png" ) );
   b_worm_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeGrey3px.png" ) );
   b_worm_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Worm area", b_worm_area);

   b_bird_area = new IMEdgeButton();
   b_bird_area->setAllTextures( t_manager.getTexture( "UICenterGrey.png" ) );
   b_bird_area->setCornerAllTextures( t_manager.getTexture( "UICornerGrey3px.png" ) );
   b_bird_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeGrey3px.png" ) );
   b_bird_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Bird area", b_bird_area);

   b_bug_area = new IMEdgeButton();
   b_bug_area->setAllTextures( t_manager.getTexture( "UICenterGrey.png" ) );
   b_bug_area->setCornerAllTextures( t_manager.getTexture( "UICornerGrey3px.png" ) );
   b_bug_area->setEdgeAllTextures( t_manager.getTexture( "UIEdgeGrey3px.png" ) );
   b_bug_area->setEdgeWidth( 3 );
   gui_manager.registerWidget( "Bug area", b_bug_area);

   fitGui_Level();
   init_level_gui = true;
   return 0;
}

int drawOrderButtons()
{
   if (init_level_gui == false)
      initLevelGui();

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

   b_numpad_area->doWidget();
   b_pl_cmd_area->doWidget();
   b_conditional_area->doWidget();
   b_movement_area->doWidget();
   b_attack_area->doWidget();
   b_monster_area->doWidget();
   b_soldier_area->doWidget();
   b_worm_area->doWidget();
   b_bird_area->doWidget();
   b_bug_area->doWidget();
   b_control_area->doWidget(); // at the end for overlap reasons

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
      int draw_x = 38;
      for (int i = player->current_order; i != player->final_order; ++i) {
         if (i == player->max_orders) i = 0;

         Order &o = player->order_queue[i];
         if ( o.action != SKIP ) {
            drawOrder( player->order_queue[i], draw_x, 2, 32 );
            draw_x += 36;
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
      
         if (s_ter) {
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
      if (unit) {
         unit->draw();
      }
   }
}

void drawProjectiles()
{
   for (list<Projectile*>::iterator it=projectile_list.begin(); it != projectile_list.end(); ++it)
   {
      Projectile* proj = (*it);
      if (proj) {
         proj->draw();
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
   drawProjectiles();
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

      // For testing purposes
      if (key_press.code == Keyboard::W) {
         log("Pressed W");
         unit_list.back()->addOrder( Order( TURN_NORTH, TRUE, 1 ) );
         unit_list.back()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == Keyboard::A) {
         log("Pressed A");
         unit_list.back()->addOrder( Order( TURN_WEST, TRUE, 1 ) );
         unit_list.back()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == Keyboard::R) {
         log("Pressed R");
         unit_list.back()->addOrder( Order( TURN_SOUTH, TRUE, 1 ) );
         unit_list.back()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == Keyboard::S) {
         log("Pressed S");
         unit_list.back()->addOrder( Order( TURN_EAST, TRUE, 1 ) );
         unit_list.back()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == Keyboard::Space) {
         unit_list.back()->activate();
      }
      if (key_press.code == Keyboard::D) {
         unit_list.back()->clearOrders();
         unit_list.back()->active = 0;
      }
      if (key_press.code == Keyboard::P) {
         unit_list.back()->logOrders();
      }

      if (key_press.code == Keyboard::LBracket) {
         log("Pressed LBracket");
         unit_list.back()->addOrder( Order( START_BLOCK, TRUE, 2 ) );
      }
      if (key_press.code == Keyboard::RBracket) {
         log("Pressed RBracket");
         unit_list.back()->addOrder( Order( END_BLOCK ) );
      }
      if (key_press.code == Keyboard::O) {
         log("Pressed O");
         unit_list.back()->addOrder( Order( REPEAT, TRUE, -1 ) );
      }
      if (key_press.code == Keyboard::M) {
         log("Pressed M");
         unit_list.back()->addOrder( Order( ATTACK_CLOSEST, TRUE, 1 ) );
      }
      if (key_press.code == Keyboard::N) {
         log("Pressed N");
         Order o( SUMMON_BUG, TRUE, 6);
         o.iteration = 6;
         player->addOrder( o );
      }
      if (key_press.code == Keyboard::E) {
         log("Pressed E");
         player->activate();
      }

      return true;
   }
    
   virtual bool keyReleased( const Event::KeyEvent &key_release )
   {
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

      if (right_mouse_down && r_click_menu_open) {
         //rightClickMenuSelect( mouse_move.x, mouse_move.y );
      }

      old_mouse_x = mouse_move.x;
      old_mouse_y = mouse_move.y;

      return true;
   }

   virtual bool mouseButtonPressed( const Event::MouseButtonEvent &mbp )
   {
      if (mbp.button == Mouse::Left) {
         left_mouse_down = 1;
         left_mouse_down_time = game_clock->getElapsedTime().asMilliseconds();

         if (r_click_menu_solid) {
            rightClickMenuSelect( mbp.x, mbp.y );
            rightClickMenuChoose();
         }
      }

      if (mbp.button == Mouse::Right) {
         right_mouse_down = 1;
         right_mouse_down_time = game_clock->getElapsedTime().asMilliseconds();
         if (!isMouseOverGui( mbp.x, mbp.y ))
            rightClickMenuCreate( mbp.x, mbp.y );
      }


      return true;
   }

   virtual bool mouseButtonReleased( const Event::MouseButtonEvent &mbr )
   {
      if (mbr.button == Mouse::Left) {
         left_mouse_down = 0;
         int left_mouse_up_time = game_clock->getElapsedTime().asMilliseconds();

         if (left_mouse_up_time - left_mouse_down_time < MOUSE_DOWN_SELECT_TIME
               && !isMouseOverGui( mbr.x, mbr.y ))
            selectUnit( coordsWindowToView( mbr.x, mbr.y ) );

      }
      if (mbr.button == Mouse::Right) {
         right_mouse_down = 0;
         int right_mouse_up_time = game_clock->getElapsedTime().asMilliseconds();

         if (right_mouse_up_time - right_mouse_down_time < MOUSE_DOWN_SELECT_TIME)
            rightClickMenuSolidify();
         else {
            rightClickMenuSelect( mbr.x, mbr.y );
            rightClickMenuChoose();
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


};
