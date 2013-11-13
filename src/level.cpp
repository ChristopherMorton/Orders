#include "shutdown.h"
#include "level.h"
#include "building.h"
#include "unit.h"
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

#define TILE_SIZE 8

using namespace sf;

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

//////////////////////////////////////////////////////////////////////
// Level Data
//////////////////////////////////////////////////////////////////////

int level_init = 0;

int level_num = -1;
int level_dim_x = -1;
int level_dim_y = -1;

typedef enum {
   TER_NONE,
   TER_GRASS,
   TER_ROCK,
   NUM_TERRAINS
} Terrain;
Sprite **terrain_sprites;

// Grids - row-major order, size = dim_x X dim_y
Terrain* terrain_grid;
Building** building_grid;
Unit** unit_grid;

//////////////////////////////////////////////////////////////////////
// Utility
//////////////////////////////////////////////////////////////////////

#define GRID_AT(GRID,X,Y) (GRID[((X) + ((Y) * level_dim_x))])

Vector2f coordsWindowToView( int window_x, int window_y )
{
   // (win_size / view_size) + view_base
   float view_x = ((float)(l_r_window->getSize().x) / (float)(level_view->getSize().x)) + ((level_view->getCenter().x) - (level_view->getSize().x / 2.0));
   float view_y = ((float)(l_r_window->getSize().y) / (float)(level_view->getSize().y)) + ((level_view->getCenter().y) - (level_view->getSize().y / 2.0));

   return Vector2f( view_x, view_y );
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

   x_size *= TILE_SIZE;
   y_size *= TILE_SIZE;
   center.x *= TILE_SIZE;
   center.y *= TILE_SIZE;
   
   level_view->setSize( x_size, y_size );
   level_view->setCenter( center.x, center.y );

   return 0;
}

// Positive is zoom ***, negative is zoom **
int zoomView( int ticks, Vector2f zoom_around )
{
   float zoom_val = pow(1.5, -ticks);

   level_view->zoom( zoom_val );
   
   return 0;
}

//////////////////////////////////////////////////////////////////////
// Loading
//////////////////////////////////////////////////////////////////////

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

   // Setup terrain
   terrain_sprites = new Sprite*[NUM_TERRAINS];

   terrain_sprites[TER_GRASS] = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "GreenGrass.png" )));
   terrain_sprites[TER_ROCK] = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "BrownRock.png" )));
         

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

int initGrids(int x, int y, Terrain ter_default)
{
   clearGrids();

   level_dim_x = x;
   level_dim_y = y;

   int dim = x * y;

   terrain_grid = new Terrain[dim];
   for (int i = 0; i < dim; ++i) terrain_grid[i] = ter_default;

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
      initGrids(10,5,TER_GRASS);
      GRID_AT(terrain_grid,4,3) = TER_ROCK;
      setView( 3.0, Vector2f( 2.0, 2.0 ) );
   }

   return 0;
}

int updateLevel( int dt )
{
   return 0;
}

int drawLevel()
{
   l_r_window->setView(*level_view);
   // Level
   int x, y;
   for (x = 0; x < level_dim_x; ++x) {
      for (y = 0; y < level_dim_y; ++y) {

         Sprite *s_ter = terrain_sprites[GRID_AT(terrain_grid,x,y)];
      
         if (s_ter) {
         s_ter->setPosition( x * TILE_SIZE, y * TILE_SIZE );
         s_ter->setScale( 1, 1 );
         l_r_window->draw( *s_ter );
         }

      }
   }

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
      return true;
   }
    
   virtual bool keyReleased( const sf::Event::KeyEvent &key_release )
   {
      return true;
   }

   virtual bool mouseMoved( const sf::Event::MouseMoveEvent &mouse_move )
   {
      return true;
   }

   virtual bool mouseButtonPressed( const sf::Event::MouseButtonEvent &mouse_button_press )
   {
      return true;
   }

   virtual bool mouseButtonReleased( const sf::Event::MouseButtonEvent &mouse_button_release )
   {
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
