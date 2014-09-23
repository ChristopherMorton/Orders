#include "map.h"
#include "menustate.h"
#include "config.h"
#include "util.h"
#include "shutdown.h"
#include "clock.h"
#include "log.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"
#include "SFML_WindowEventManager.hpp"

#include "IMButton.hpp"
#include "IMTextButton.hpp"
#include "IMGuiManager.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include <sstream>

using namespace sf;

namespace sum
{

///////////////////////////////////////////////////////////////////////////////
// Data

Sprite *s_map = NULL;
View *map_view = NULL;

IMButton *b_start_test_level = NULL;
IMButton *b_map_to_splash = NULL;
IMTextButton *b_map_to_options = NULL;
IMTextButton *b_map_to_focus = NULL;
IMTextButton *b_map_to_presets = NULL;

bool init_map_gui = false;

string s_map_to_options = "Options";
string s_map_to_focus = "Focus";
string s_map_to_presets = "Order Sets";

int selected_level = 0;

const int c_map_dimension = 100;
const int c_view_size = 30;

const int c_map_gui_y_dim = 6;

///////////////////////////////////////////////////////////////////////////////
// View

Vector2f coordsWindowToMapView( int window_x, int window_y )
{
   RenderWindow *r_window = SFML_GlobalRenderWindow::get();
   float view_x = ( ((float)window_x / (float)(r_window->getSize().x)) *(float)c_view_size ) 
                  + ((map_view->getCenter().x) - (c_view_size / 2.0));
   float view_y = ( ((float)window_y / (float)(r_window->getSize().y)) *(float)c_view_size ) 
                  + ((map_view->getCenter().y) - (c_view_size / 2.0));

   return Vector2f( view_x, view_y );
}

int setMapView( Vector2f center )
{ 
   if (center.x < (c_view_size / 2)) center.x = (c_view_size / 2);
   if (center.y < (c_view_size / 2)) center.y = (c_view_size / 2);
   if (center.x > (c_map_dimension - (c_view_size / 2))) center.x = (c_map_dimension - (c_view_size / 2));
   if (center.y > (c_map_dimension - (c_view_size / 2))) center.y = (c_map_dimension - (c_view_size / 2));

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

int selectMapObject( Vector2f coords )
{
   int cx = (int)coords.x,
       cy = (int)coords.y;

   std::stringstream ls;
   ls << "Selecting a map object (level) at x=" << cx << ", y=" << cy;
   log(ls.str());

   // TODO: is there a level at this location? -data structure??

   if (cx >= 40 && cx <= 50 && cy >= 40 && cy <= 50) {
      log("Selected a map object at 40-50 x 40-50");
      selected_level = -1;
      return 0;
   }

   selected_level = 0;
   return -1;
}

///////////////////////////////////////////////////////////////////////////////
// API 

void initMapLevels()
{

}

void initMapGui()
{
   SFML_TextureManager *texture_manager = &SFML_TextureManager::getSingleton();
   IMGuiManager *gui_manager = &IMGuiManager::getSingleton();

   b_start_test_level = new IMButton();
   b_start_test_level->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_start_test_level->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_start_test_level->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   gui_manager->registerWidget( "Start Test Level", b_start_test_level);

   b_map_to_splash = new IMButton();
   b_map_to_splash->setNormalTexture( texture_manager->getTexture( "GoBackButtonScratch.png" ) );
   b_map_to_splash->setHoverTexture( texture_manager->getTexture( "GoBackButtonScratch.png" ) );
   b_map_to_splash->setPressedTexture( texture_manager->getTexture( "GoBackButtonScratch.png" ) );
   gui_manager->registerWidget( "Map to Splash", b_map_to_splash);

   b_map_to_options = new IMTextButton();
   b_map_to_options->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_map_to_options->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_map_to_options->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_map_to_options->setText( &s_map_to_options );
   b_map_to_options->setFont( menu_font );
   b_map_to_options->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Map to Options", b_map_to_options);

   b_map_to_focus = new IMTextButton();
   b_map_to_focus->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_map_to_focus->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_map_to_focus->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_map_to_focus->setText( &s_map_to_focus );
   b_map_to_focus->setFont( menu_font );
   b_map_to_focus->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Map to Focus", b_map_to_focus);

   b_map_to_presets = new IMTextButton();
   b_map_to_presets->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_map_to_presets->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_map_to_presets->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_map_to_presets->setText( &s_map_to_presets );
   b_map_to_presets->setFont( menu_font );
   b_map_to_presets->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Map to Presets", b_map_to_presets);

   init_map_gui = true;
}

void fitGui_Map()
{
   if (!init_map_gui)
      initMapGui();

   int width = config::width(),
       height = config::height();

   int bar_height = height / 15;

   b_start_test_level->setPosition( 100, 100 );
   b_start_test_level->setSize( 20, 20 );

   b_map_to_splash->setPosition( 0, 0 );
   b_map_to_splash->setSize( bar_height, bar_height );

   b_map_to_options->setPosition( 60, 0 );
   b_map_to_options->setSize( 200, bar_height );
   b_map_to_options->setTextSize( 24 );
   b_map_to_options->centerText();

   b_map_to_focus->setPosition( 280, 0 );
   b_map_to_focus->setSize( 200, bar_height );
   b_map_to_focus->setTextSize( 24 );
   b_map_to_focus->centerText();

   b_map_to_presets->setPosition( 500, 0 );
   b_map_to_presets->setSize( 200, bar_height );
   b_map_to_presets->setTextSize( 24 );
   b_map_to_presets->centerText();
}

int initMap()
{
   static int progress = 0;
   if (progress == 0) {
      initMapLevels();
      progress = 1;
      return -1;
   } else if (progress == 1) {
      initMapGui();
      fitGui_Map();
      progress = 2;
      return -1;
   } else {
      SFML_TextureManager *texture_manager = &SFML_TextureManager::getSingleton();
      s_map = new Sprite( *texture_manager->getTexture( "MapScratch.png" ));
      normalizeTo1x1( s_map );
      s_map->scale( c_map_dimension, c_map_dimension );

      map_view = new View();
      map_view->setSize( c_view_size, c_view_size );
      setMapView( Vector2f( 50, 50 ) );

      return 0;
   }
}

int drawMap( int dt )
{
   int retval = 0;
   RenderWindow *r_window = SFML_GlobalRenderWindow::get();
   r_window->setView(*map_view);
   r_window->draw( *s_map );
   r_window->setView( r_window->getDefaultView() );

   int bar_height = (config::height() / 15) + 10;
   RectangleShape gui_bar( Vector2f( config::width(), bar_height ) );
   gui_bar.setFillColor( Color::White );
   gui_bar.setOutlineThickness( 0 );

   r_window->draw( gui_bar );

   if (b_start_test_level->doWidget()) {
      retval = 1;
   }
   if (b_map_to_splash->doWidget()) {
      menu_state = MENU_MAIN | MENU_PRI_SPLASH;
   }
   if (b_map_to_options->doWidget()) {
      menu_state = MENU_MAIN | MENU_PRI_MAP | MENU_SEC_OPTIONS;
   }
   if (b_map_to_focus->doWidget()) {
      menu_state = MENU_MAIN | MENU_PRI_MAP | MENU_MAP_FOCUS;
   }
   if (b_map_to_presets->doWidget()) {
      menu_state = MENU_MAIN | MENU_PRI_MAP | MENU_MAP_PRESETS;
   }

   return retval;
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
         int left_mouse_up_time = game_clock->getElapsedTime().asMilliseconds();

         if (left_mouse_up_time - left_mouse_down_time < 300) {

            // FIRST: is the mouse in the gui?
            if (mbr.y <= c_map_gui_y_dim)
               return true;

            selectMapObject( coordsWindowToMapView( mbr.x, mbr.y ) );
         }
      }

      return true;
   }

   virtual bool mouseWheelMoved( const Event::MouseWheelEvent &mwm )
   {
      //zoomView( mwm.delta, coordsWindowToMapView( mwm.x, mwm.y ) );
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
