#include "shutdown.h"
#include "level.h"
#include "orders.h"
#include "types.h"
#include "units.h"
#include "util.h"
#include "log.h"

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
#include <deque>

#define TURN_LENGTH 1000


using namespace sf;
using namespace std;

namespace sum {

//////////////////////////////////////////////////////////////////////
// Global app-state variables
//////////////////////////////////////////////////////////////////////

RenderWindow *l_r_window;
View *level_view;
float view_rel_x_to_y;

// Managers
IMGuiManager* l_gui_manager;
IMCursorManager* l_cursor_manager;
SFML_TextureManager* l_texture_manager;
SFML_WindowEventManager* l_event_manager;

int turn, turn_progress;

//////////////////////////////////////////////////////////////////////
// Some definitions
//////////////////////////////////////////////////////////////////////

struct Building
{

};

struct Effect
{

};

//////////////////////////////////////////////////////////////////////
// Level Data
//////////////////////////////////////////////////////////////////////

int level_init = 0;

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
Building** building_grid;
Unit** unit_grid;
// Lists
deque<Unit*> unit_list;
deque<Building*> building_list;
deque<Effect*> effects_list;

//////////////////////////////////////////////////////////////////////
// UI Data
//////////////////////////////////////////////////////////////////////

int left_mouse_down = 0;

//////////////////////////////////////////////////////////////////////
// Utility
//////////////////////////////////////////////////////////////////////

#define GRID_AT(GRID,X,Y) (GRID[((X) + ((Y) * level_dim_x))])

Vector2f coordsWindowToView( int window_x, int window_y )
{
   // (win_size / view_size) + view_base
   float view_x = (((float)(level_view->getSize().x) / (float)(l_r_window->getSize().x))*window_x) + ((level_view->getCenter().x) - (level_view->getSize().x / 2.0));
   float view_y = (((float)(level_view->getSize().y) / (float)(l_r_window->getSize().y))*window_y) + ((level_view->getCenter().y) - (level_view->getSize().y / 2.0));

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
//////////////////////////////////////////////////////////////////////

int setView( float x_size, Vector2f center )
{
   float y_size = x_size * view_rel_x_to_y;

   float x_base = center.x - (x_size / 2),
         y_base = center.y - (y_size / 2),
         x_top = center.x + (x_size / 2),
         y_top = center.y + (y_size / 2);

   if (x_base < 0 || y_base < 0 || x_top >= level_dim_x || y_top >= level_dim_y)
   {
      log("setView error: parameters describe view outside of level");
      return -1;
   }

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
// Targetting
//////////////////////////////////////////////////////////////////////

Unit* getEnemy( int x, int y, float range, Direction dir, int selector)
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
         max_x = x;
         break;
      case SOUTH:
         min_x = x;
         break;
      case WEST:
         max_y = y;
         break;
      case EAST:
         min_y = y;
         break;
   }
   if (min_x < 0) min_x = 0;
   if (min_x >= level_dim_x) min_x = level_dim_x - 1;
   if (min_y < 0) min_y = 0;
   if (min_y >= level_dim_y) min_y = level_dim_y - 1;

   float range_squared = range * range;

   Unit *result = NULL;
   float result_r_squared = 0;
   for (int i = min_x; i <= max_x; ++i) {
      for (int j = min_y; j <= max_y; ++j) {
         Unit *u = GRID_AT(unit_grid,i,j);
         if (u) {
            // Is it really in range?
            float u_x = u->x_real - x, u_y = u->y_real - y;
            float u_squared = (u_x * u_x) + (u_y * u_y);
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
// Loading
//////////////////////////////////////////////////////////////////////

void initTextures()
{
   // Setup terrain
   terrain_sprites = new Sprite*[NUM_TERRAINS];

   terrain_sprites[TER_NONE] = NULL;
   terrain_sprites[TER_TREE1] = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "BasicTree1.png" )));
   terrain_sprites[TER_TREE2] = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "BasicTree2.png" )));
         
   normalizeTo1x1( terrain_sprites[TER_TREE1] );
   normalizeTo1x1( terrain_sprites[TER_TREE2] );
   
   base_grass_sprite = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "GreenGrass.png" )));
   base_mountain_sprite = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "GrayRock.png" )));
   base_underground_sprite = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "BrownRock.png" )));

}

void init()
{
   if (level_init) return;

   l_r_window = SFML_GlobalRenderWindow::get();
   l_gui_manager = &IMGuiManager::getSingleton();
   l_cursor_manager = &IMCursorManager::getSingleton();
   l_texture_manager = &SFML_TextureManager::getSingleton();
   l_event_manager = &SFML_WindowEventManager::getSingleton();

   level_view = new View();
   view_rel_x_to_y = ((float)l_r_window->getSize().y) / ((float)l_r_window->getSize().x);

   initTextures();

   level_init = true;
}

void clearGrids()
{
   if (terrain_grid)
      delete terrain_grid;
   if (unit_grid)
      delete unit_grid;
   if (building_grid)
      delete building_grid; 
}

int initGrids(int x, int y)
{
   clearGrids();

   level_dim_x = x;
   level_dim_y = y;

   int dim = x * y;

   terrain_grid = new Terrain[dim];
   for (int i = 0; i < dim; ++i) terrain_grid[i] = TER_NONE;

   building_grid = new Building*[dim];
   unit_grid = new Unit*[dim];

   log("initGrids succeeded");

   return 0;
}

int loadLevel( int level_id )
{
   if (!level_init)
      init();

   // Currently only loads test level
   if (level_id == -1 || true)
   {
      base_terrain = BASE_TER_GRASS;
      initGrids(20,15);
      GRID_AT(terrain_grid,6,5) = TER_TREE1;
      GRID_AT(terrain_grid,2,2) = TER_TREE2;
      GRID_AT(terrain_grid,11,11) = TER_TREE2;
      setView( 11.9, Vector2f( 6.0, 6.0 ) );

      Unit *magician = new Magician( 4, 4, SOUTH );
      unit_list.push_front( magician );
      //Order o( MOVE_FORWARD );
      //magician->addOrder( o );
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////
// Update
//////////////////////////////////////////////////////////////////////

int startTurnAll( )
{
   for (deque<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
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
   for (deque<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit) {
         unit->update( dt, dtf );
      }
   }

   return 0;
}

int completeTurnAll( )
{
   for (deque<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
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

      completeTurnAll();
      turn++;
      startTurnAll();

      updateAll( turn_progress );
   } else {
      updateAll( dt );
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////
// Draw
//////////////////////////////////////////////////////////////////////

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
   l_r_window->draw( *s_ter );
}

void drawTerrain()
{
   int x, y;
   for (x = 0; x < level_dim_x; ++x) {
      for (y = 0; y < level_dim_y; ++y) {

         Sprite *s_ter = terrain_sprites[GRID_AT(terrain_grid,x,y)];
      
         if (s_ter) {
            s_ter->setPosition( x, y );
            l_r_window->draw( *s_ter );
         }

      }
   }
}

void drawUnits()
{
   for (deque<Unit*>::iterator it=unit_list.begin(); it != unit_list.end(); ++it)
   {
      Unit* unit = (*it);
      if (unit) {
         unit->draw();
      }
   }
}

int drawLevel()
{
   l_r_window->setView(*level_view);
   // Level
   drawBaseTerrain();
   drawTerrain();
   drawUnits();

   // Gui

   return 0;
} 

//////////////////////////////////////////////////////////////////////
// Event listener
//////////////////////////////////////////////////////////////////////

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
         unit_list.front()->addOrder( Order( TURN_NORTH, TRUE, 1 ) );
         unit_list.front()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == sf::Keyboard::A) {
         log("Pressed A");
         unit_list.front()->addOrder( Order( TURN_WEST, TRUE, 1 ) );
         unit_list.front()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == sf::Keyboard::R) {
         log("Pressed R");
         unit_list.front()->addOrder( Order( TURN_SOUTH, TRUE, 1 ) );
         unit_list.front()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == sf::Keyboard::S) {
         log("Pressed S");
         unit_list.front()->addOrder( Order( TURN_EAST, TRUE, 1 ) );
         unit_list.front()->addOrder( Order( MOVE_FORWARD, TRUE, 1 ) ); 
      }
      if (key_press.code == sf::Keyboard::Space) {
         unit_list.front()->activate();
      }
      if (key_press.code == sf::Keyboard::D) {
         unit_list.front()->clearOrders();
         unit_list.front()->active = 0;
      }
      if (key_press.code == sf::Keyboard::P) {
         unit_list.front()->logOrders();
      }
      if (key_press.code == sf::Keyboard::LBracket) {
         log("Pressed LBracket");
         unit_list.front()->addOrder( Order( START_BLOCK, TRUE, 2 ) );
      }
      if (key_press.code == sf::Keyboard::RBracket) {
         log("Pressed RBracket");
         unit_list.front()->addOrder( Order( END_BLOCK ) );
      }
      if (key_press.code == sf::Keyboard::O) {
         log("Pressed O");
         unit_list.front()->addOrder( Order( REPEAT, TRUE, -1 ) );
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
      if (mbp.button == Mouse::Left)
         left_mouse_down = 1;

      return true;
   }

   virtual bool mouseButtonReleased( const sf::Event::MouseButtonEvent &mbr )
   {
      if (mbr.button == Mouse::Left)
         left_mouse_down = 0;

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
      event_manager.removeAllMouseListeners();
      event_manager.removeAllKeyListeners();
      event_manager.addMouseListener( &level_listener, "level" );
      event_manager.addKeyListener( &level_listener, "level" );
   } else {
      event_manager.removeMouseListener( &level_listener );
      event_manager.removeKeyListener( &level_listener );
   }
}


};
