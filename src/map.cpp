#include "map.h"
#include "util.h"
#include "shutdown.h"
#include "clock.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"
#include "SFML_WindowEventManager.hpp"

#include "IMButton.hpp"
#include "IMGuiManager.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

using namespace sf;

namespace sum
{

Sprite *s_map = NULL;
View *map_view = NULL;

IMButton *b_start_test_level = NULL;

const int map_dimension = 100;
const int view_size = 30;

///////////////////////////////////////////////////////////////////////////////
// View

Vector2f coordsWindowToMapView( int window_x, int window_y )
{
   RenderWindow *r_window = SFML_GlobalRenderWindow::get();
   float view_x = ( ((float)window_x / (float)(r_window->getSize().x)) *(float)view_size ) 
                  + ((map_view->getCenter().x) - (view_size / 2.0));
   float view_y = ( ((float)window_y / (float)(r_window->getSize().y)) *(float)view_size ) 
                  + ((map_view->getCenter().y) - (view_size / 2.0));

   return Vector2f( view_x, view_y );
}

int setMapView( Vector2f center )
{ 
   if (center.x < (view_size / 2)) center.x = (view_size / 2);
   if (center.y < (view_size / 2)) center.y = (view_size / 2);
   if (center.x > (map_dimension - (view_size / 2))) center.x = (map_dimension - (view_size / 2));
   if (center.y > (map_dimension - (view_size / 2))) center.y = (map_dimension - (view_size / 2));

   map_view->setCenter( center.x, center.y );

   return 0;
}

// Shifts in terms of view coords
int shiftMapView( float x_shift, float y_shift )
{
   Vector2f old_center = map_view->getCenter();
   Vector2f new_center (old_center.x + x_shift, old_center.y + y_shift);

   setMapView( new_center );

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// API 

int initMap()
{
   SFML_TextureManager *texture_manager = &SFML_TextureManager::getSingleton();
   s_map = new Sprite( *texture_manager->getTexture( "MapScratch.png" ));
   normalizeTo1x1( s_map );
   s_map->scale( map_dimension, map_dimension );

   map_view = new View();
   map_view->setSize( view_size, view_size );
   setMapView( Vector2f( 50, 50 ) );

   b_start_test_level = new IMButton();
   b_start_test_level->setPosition( 0, 0 );
   b_start_test_level->setSize( 30, 30 );
   b_start_test_level->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_start_test_level->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_start_test_level->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   IMGuiManager::getSingleton().registerWidget( "Start Test Level", b_start_test_level);

   return 0;
}

int drawMap( int dt )
{
   RenderWindow *r_window = SFML_GlobalRenderWindow::get();
   r_window->setView(*map_view);
   r_window->draw( *s_map );

   if (b_start_test_level->doWidget()) {
      return -1;
   }

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Listener

extern bool left_mouse_down;
extern int left_mouse_down_time;

struct MapEventHandler : public My_SFML_MouseListener, public My_SFML_KeyListener
{
   virtual bool keyPressed( const Event::KeyEvent &key_press )
   {
      if (key_press.code == Keyboard::Q)
         shutdown(1,1);
      if (key_press.code == Keyboard::Right)
         shiftMapView( 15, 0 );
      if (key_press.code == Keyboard::Left)
         shiftMapView( -15, 0 );
      if (key_press.code == Keyboard::Down)
         shiftMapView( 0, 15 );
      if (key_press.code == Keyboard::Up)
         shiftMapView( 0, -15 );

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
         Vector2f old_spot = coordsWindowToMapView( old_mouse_x, old_mouse_y );
         Vector2f new_spot = coordsWindowToMapView( mouse_move.x, mouse_move.y );
         shiftMapView( old_spot.x - new_spot.x, old_spot.y - new_spot.y );
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
      }

      return true;
   }

   virtual bool mouseButtonReleased( const Event::MouseButtonEvent &mbr )
   {
      if (mbr.button == Mouse::Left) {
         left_mouse_down = 0;
         //int left_mouse_up_time = game_clock->getElapsedTime().asMilliseconds();
      }

      return true;
   }

   virtual bool mouseWheelMoved( const Event::MouseWheelEvent &mwm )
   {
      //zoomView( mwm.delta, coordsWindowToView( mwm.x, mwm.y ) );
      return true;
   }
};

void setMapListener( bool set )
{
   static MapEventHandler map_listener;
   SFML_WindowEventManager& event_manager = SFML_WindowEventManager::getSingleton();
   if (set) {
      event_manager.addMouseListener( &map_listener, "map mouse" );
      event_manager.addKeyListener( &map_listener, "map key" );
   } else {
      event_manager.removeMouseListener( &map_listener );
      event_manager.removeKeyListener( &map_listener );
   }
}

};
