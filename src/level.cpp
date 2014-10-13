#include "shutdown.h"
#include "level.h"
#include "orders.h"
#include "types.h"
#include "units.h"
#include "gui.h"
#include "util.h"
#include "log.h"
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

#include "stdio.h"
#include "math.h"
#include <sstream>
#include <list>

#define TURN_LENGTH 1000

#define MOUSE_DOWN_SELECT_TIME 200

using namespace sf;
using namespace std;

namespace sum {

//////////////////////////////////////////////////////////////////////
// Global app-state variables

View *level_view;
float view_rel_x_to_y;

int turn, turn_progress;

//////////////////////////////////////////////////////////////////////
// Some definitions

const float c_min_zoom_x = 10;

//////////////////////////////////////////////////////////////////////
// Level Data

int level_init = 0;

bool between_turns = false;

int level_num = -1;
int level_dim_x = -1;
int level_dim_y = -1;

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

list<Unit*> listening_units;
Unit *player = NULL;

//////////////////////////////////////////////////////////////////////
// UI Data

int left_mouse_down = 0;
int left_mouse_down_time = 0;

//////////////////////////////////////////////////////////////////////
// Utility

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

Vector2u coordsWindowToLevel( int window_x, int window_y )
{
   Vector2f view_coords = coordsWindowToView( window_x, window_y );

   int x = (int)view_coords.x,
       y = (int)view_coords.y;
   return Vector2u(x, y);
}

//////////////////////////////////////////////////////////////////////
// View management

int setView( float x_size, Vector2f center )
{
   float y_size = x_size * view_rel_x_to_y;

   float x_add = 0, y_add = 0;
   float x_base = center.x - (x_size / 2),
         y_base = center.y - (y_size / 2),
         x_top = center.x + (x_size / 2),
         y_top = center.y + (y_size / 2);

   // Don't need to check for: top AND bottom overlap -> failure
   if (x_base < 0)
      x_add = -x_base;

   if (x_top > (level_dim_x ))
      x_add = (level_dim_x ) - x_top;

   if (y_base < 0)
      y_add = -y_base;

   if (y_top > (level_dim_y ))
      y_add = (level_dim_y ) - y_top;

   center.x += x_add;
   center.y += y_add;

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

   float x_add = 0, y_add = 0;
   float x_base = new_center.x - (old_size.x / 2.0);
   float y_base = new_center.y - (old_size.y / 2.0);
   float x_top = new_center.x + (old_size.x / 2.0);
   float y_top = new_center.y + (old_size.y / 2.0);

   // Don't need to check for: top AND bottom overlap -> failure
   if (x_base < 0)
      x_add = -x_base;

   if (x_top > (level_dim_x ))
      x_add = (level_dim_x ) - x_top;

   if (y_base < 0)
      y_add = -y_base;

   if (y_top > (level_dim_y ))
      y_add = (level_dim_y ) - y_top;

   new_center.x += x_add;
   new_center.y += y_add;
   level_view->setCenter( new_center );

   return 0;
}

// Positive is zoom ***, negative is zoom **
int zoomView( int ticks, Vector2f zoom_around )
{
   float zoom_val = pow(1.5, -ticks);
   Vector2f old_center = level_view->getCenter(),
            old_size = level_view->getSize();
   Vector2f new_center ((old_center.x + zoom_around.x) / 2.0, (old_center.y + zoom_around.y) / 2.0);
   Vector2f new_size (old_size*zoom_val);

   // Fit map
   if (new_size.x > level_dim_x) {
      float x_fit = level_dim_x / new_size.x;
      new_size *= x_fit;
   }
   if (new_size.y > level_dim_y) {
      float y_fit = level_dim_y / new_size.y;
      new_size *= y_fit;
   }

   // Zoom limits
   if (new_size.x < c_min_zoom_x) {
      float min_fit = c_min_zoom_x / new_size.x;
      new_size *= min_fit;
   }

   return setView( new_size.x, new_center );

   float x_add = 0, y_add = 0;
   float x_base = new_center.x - (zoom_val * old_size.x / 2.0);
   float y_base = new_center.y - (zoom_val * old_size.y / 2.0);
   float x_top = new_center.x + (zoom_val * old_size.x / 2.0);
   float y_top = new_center.y + (zoom_val * old_size.y / 2.0);

   // Check each edge
   if (x_base < 0) {
      x_add = -x_base;
      x_base = 0;
      x_top -= x_base;
   }

   if (x_top > (level_dim_x )) {
      if (x_add != 0) // In other words, if can't zoom out that much
         return -1;
      else {
         x_add = (level_dim_x ) - x_top;
         x_top = (level_dim_x );
         x_base += x_add;
         if (x_base < 0)
            return -1;
      }
   }

   if (y_base < 0) {
      y_add = -y_base;
      y_base = 0;
      y_top -= y_base;
   }

   if (y_top > (level_dim_y )) {
      if (y_add != 0) // In other words, if can't zoom out that much
         return -1;
      else {
         y_add = (level_dim_y ) - y_top;
         y_top = (level_dim_y );
         y_base += y_add;
         if (y_base < 0)
            return -1;
      }
   }

   level_view->setCenter( new_center.x + x_add, new_center.y + y_add ); 

   level_view->setSize( x_top - x_base, y_top - y_base );
   //level_view->zoom( zoom_val );
   
   return 0;
}

//////////////////////////////////////////////////////////////////////
// Adding stuff

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

      Order o( SUMMON_BUG, TRUE, 1);
      o.iteration = 1;
      player->addOrder( o );
      player->activate();

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
   if (between_turns && u) {
      if (GRID_AT(unit_grid, u->x_grid, u->y_grid) == u)
         GRID_AT(unit_grid, u->x_grid, u->y_grid) = NULL;

      GRID_AT(unit_grid, new_x, new_y) = u;
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
// Targetting

int selectUnit( Vector2f coords )
{
   int cx = (int)coords.x,
       cy = (int)coords.y;

   std::stringstream ls;
   ls << "Selecting a unit at x=" << cx << ", y=" << cy;
   log(ls.str());
   Unit *u = GRID_AT(unit_grid, (int)coords.x, (int)coords.y);

   if (u) {
      log("Selected a unit");
      selected_unit = u;
      return 0;
   }

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
   if (min_x >= level_dim_x) min_x = level_dim_x - 1;
   if (min_y < 0) min_y = 0;
   if (min_y >= level_dim_y) min_y = level_dim_y - 1;

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
               } else if (selector == SELECT_CLOSEST) {
                  if (u_squared < result_r_squared)
                     result = u;
               } else if (selector == SELECT_FARTHEST) {
                  if (u_squared > result_r_squared)
                     result = u;
               } else if (selector == SELECT_SMALLEST) {
                  if (u->health < result->health)
                     result = u;
               } else if (selector == SELECT_BIGGEST) {
                  if (u->health > result->health)
                     result = u;
               }
            }
         }
      }
   }

   return result;
}

//////////////////////////////////////////////////////////////////////
// Player interface

SummonMarker* summonMarker = NULL;

int startSummon( Order o )
{
   log( "Starting a summon" );
   int x = o.count,
       y = o.iteration;

   if (GRID_AT(unit_grid, x, y) != NULL) // Summon fails, obvi
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
   switch( o.action ) {
      case SUMMON_TANK:
         //u = new Tank();
         //break;
      case SUMMON_WARRIOR:
         //u = new Warrior();
         //break;
      case SUMMON_WORM:
         //u = new Worm();
         //break;
      case SUMMON_BIRD:
         //u = new Bird();
         //break;
      case SUMMON_BUG:
         u = new Bug( x, y, SOUTH );
         break;
   }

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
   if (min_x >= level_dim_x) min_x = level_dim_x - 1;
   if (min_y < 0) min_y = 0;
   if (min_y >= level_dim_y) min_y = level_dim_y - 1;

   float range_squared = player->attack_range * player->attack_range;

   for (int j = min_y; j <= max_y; ++j) {
      for (int i = min_x; i <= max_x; ++i) {
         Unit *u = GRID_AT(unit_grid,i,j);
         if (u && u->team == 0) { // On my team
            float u_x = u->x_grid - x, u_y = u->y_grid - y;
            float u_squared = (u_x * u_x) + (u_y * u_y);
            // Is it really in range?
            if (u_squared <= range_squared) {

               // Okay so it's in shout range, now what?
               if (o.action == PL_ALERT_ALL) {
                  alert(u);
               } else if (o.action == PL_ALERT_TANKS && u->type == TANK_T) {
                  alert(u);
               } else if (o.action == PL_ALERT_WARRIORS && u->type == WARRIOR_T) {
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

int broadcastOrder( Order o )
{
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
      case PL_ALERT_TANKS:
      case PL_ALERT_WARRIORS:
      case PL_ALERT_WORMS:
      case PL_ALERT_BIRDS:
      case PL_ALERT_BUGS:
         alertUnits( o );
         break;
      case SUMMON_TANK:
      case SUMMON_WARRIOR:
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
      case SUMMON_TANK:
      case SUMMON_WARRIOR:
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
// Loading

void fitGui_Level()
{
   view_rel_x_to_y = ((float)config::height()) / ((float)config::width());
}

void initTextures()
{
   SFML_TextureManager &t_manager = SFML_TextureManager::getSingleton();
   // Setup terrain
   terrain_sprites = new Sprite*[NUM_TERRAINS];

   terrain_sprites[TER_NONE] = NULL;
   terrain_sprites[TER_TREE1] = new Sprite( *(t_manager.getTexture( "BasicTree1.png" )));
   terrain_sprites[TER_TREE2] = new Sprite( *(t_manager.getTexture( "BasicTree2.png" )));
         
   normalizeTo1x1( terrain_sprites[TER_TREE1] );
   normalizeTo1x1( terrain_sprites[TER_TREE2] );
   
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
}

int initGrids(int x, int y)
{
   clearGrids();

   level_dim_x = x;
   level_dim_y = y;

   int dim = x * y;

   terrain_grid = new Terrain[dim];
   for (int i = 0; i < dim; ++i) terrain_grid[i] = TER_NONE;

   unit_grid = new Unit*[dim];
   for (int i = 0; i < dim; ++i) unit_grid[i] = NULL;

   log("initGrids succeeded");

   return 0;
}

int loadLevel( int level_id )
{
   if (!level_init)
      init();

   // Currently only loads test level
   if (level_id == 0 || true)
   {
      base_terrain = BASE_TER_GRASS;
      initGrids(15,15);
      GRID_AT(terrain_grid,6,5) = TER_TREE1;
      GRID_AT(terrain_grid,2,2) = TER_TREE2;
      GRID_AT(terrain_grid,11,11) = TER_TREE2;
      setView( 11.9, Vector2f( 6.0, 6.0 ) );

      Unit *bug = new Bug( 4, 4, SOUTH );
      addUnit( bug );

      Unit *targetpractice = new TargetPractice( 4, 7, SOUTH );
      addUnit( targetpractice );

      player = Player::initPlayer( 5, 2, SOUTH );
      addPlayer();
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////
// Update

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
   int til_end = TURN_LENGTH - turn_progress;
   turn_progress += dt;

   if (turn_progress > TURN_LENGTH) {
      turn_progress -= TURN_LENGTH;

      updateAll( til_end );

      between_turns = true;
      completeTurnAll();
      turn++;
      startTurnAll();
      between_turns = false;

      updateAll( turn_progress );
   } else {
      updateAll( dt );
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////
// Draw

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

int drawLevel()
{
   SFML_GlobalRenderWindow::get()->setView(*level_view);
   // Level
   drawBaseTerrain();
   drawTerrain();
   drawUnits();
   drawProjectiles();

   // Gui
   drawGui();

   return 0;
} 

//////////////////////////////////////////////////////////////////////
// Event listener

struct LevelEventHandler : public My_SFML_MouseListener, public My_SFML_KeyListener
{
   virtual bool keyPressed( const sf::Event::KeyEvent &key_press )
   {
      if (key_press.code == sf::Keyboard::Q)
         shutdown(1,1);
      if (key_press.code == sf::Keyboard::Right)
         shiftView( 2, 0 );
      if (key_press.code == sf::Keyboard::Left)
         shiftView( -2, 0 );
      if (key_press.code == sf::Keyboard::Down)
         shiftView( 0, 2 );
      if (key_press.code == sf::Keyboard::Up)
         shiftView( 0, -2 );
      if (key_press.code == sf::Keyboard::Add)
         zoomView( 1 , level_view->getCenter());
      if (key_press.code == sf::Keyboard::Subtract)
         zoomView( -1 , level_view->getCenter());

      if (key_press.code == sf::Keyboard::W) {
         log("Pressed W");
         unit_list.back()->addOrder( Order( TURN_NORTH, TRUE, 1 ) );
         unit_list.back()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == sf::Keyboard::A) {
         log("Pressed A");
         unit_list.back()->addOrder( Order( TURN_WEST, TRUE, 1 ) );
         unit_list.back()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == sf::Keyboard::R) {
         log("Pressed R");
         unit_list.back()->addOrder( Order( TURN_SOUTH, TRUE, 1 ) );
         unit_list.back()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == sf::Keyboard::S) {
         log("Pressed S");
         unit_list.back()->addOrder( Order( TURN_EAST, TRUE, 1 ) );
         unit_list.back()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == sf::Keyboard::Space) {
         unit_list.back()->activate();
      }
      if (key_press.code == sf::Keyboard::D) {
         unit_list.back()->clearOrders();
         unit_list.back()->active = 0;
      }
      if (key_press.code == sf::Keyboard::P) {
         unit_list.back()->logOrders();
      }

      if (key_press.code == sf::Keyboard::LBracket) {
         log("Pressed LBracket");
         unit_list.back()->addOrder( Order( START_BLOCK, TRUE, 2 ) );
      }
      if (key_press.code == sf::Keyboard::RBracket) {
         log("Pressed RBracket");
         unit_list.back()->addOrder( Order( END_BLOCK ) );
      }
      if (key_press.code == sf::Keyboard::O) {
         log("Pressed O");
         unit_list.back()->addOrder( Order( REPEAT, TRUE, -1 ) );
      }
      if (key_press.code == sf::Keyboard::M) {
         log("Pressed M");
         unit_list.back()->addOrder( Order( ATTACK_CLOSEST, TRUE, 1 ) );
      }
      if (key_press.code == sf::Keyboard::N) {
         log("Pressed N");
         Order o( SUMMON_BUG, TRUE, 1);
         o.iteration = 1;
         player->addOrder( o );
      }
      if (key_press.code == sf::Keyboard::E) {
         log("Pressed E");
         player->activate();
      }

      return true;
   }
    
   virtual bool keyReleased( const sf::Event::KeyEvent &key_release )
   {
      return true;
   }

   virtual bool mouseMoved( const sf::Event::MouseMoveEvent &mouse_move )
   {
      static int old_mouse_x, old_mouse_y;

      if (left_mouse_down) {
         Vector2f old_spot = coordsWindowToView( old_mouse_x, old_mouse_y );
         Vector2f new_spot = coordsWindowToView( mouse_move.x, mouse_move.y );
         shiftView( old_spot.x - new_spot.x, old_spot.y - new_spot.y );
      }

      old_mouse_x = mouse_move.x;
      old_mouse_y = mouse_move.y;

      return true;
   }

   virtual bool mouseButtonPressed( const sf::Event::MouseButtonEvent &mbp )
   {
      if (mbp.button == Mouse::Left) {
         left_mouse_down = 1;
         left_mouse_down_time = game_clock->getElapsedTime().asMilliseconds();
      }

      return true;
   }

   virtual bool mouseButtonReleased( const sf::Event::MouseButtonEvent &mbr )
   {
      if (mbr.button == Mouse::Left) {
         left_mouse_down = 0;
         int left_mouse_up_time = game_clock->getElapsedTime().asMilliseconds();

         if (left_mouse_up_time - left_mouse_down_time < MOUSE_DOWN_SELECT_TIME)
            selectUnit( coordsWindowToView( mbr.x, mbr.y ) );

      }

      return true;
   }

   virtual bool mouseWheelMoved( const sf::Event::MouseWheelEvent &mwm )
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
