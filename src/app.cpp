// Summoner includes
#include "shutdown.h"
#include "config.h"
#include "savestate.h" 
#include "level.h"
#include "projectile.h"
#include "savestate.h"
#include "menustate.h"
#include "map.h"
#include "gui.h"
#include "log.h"

// SFML includes
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

// lib includes
#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_WindowEventManager.hpp"
#include "SFML_TextureManager.hpp"
#include "IMGuiManager.hpp"
#include "IMCursorManager.hpp"
#include "IMButton.hpp"
#include "IMTextButton.hpp"

// C includes
#include <stdio.h>

// C++ includes
#include <deque>
#include <fstream>

using namespace sf;

///////////////////////////////////////////////////////////////////////////////
// Data

Font *menu_font;

MenuState menu_state;

namespace sum
{

// Global app-state variables

RenderWindow *r_window = NULL;

// Managers
IMGuiManager* gui_manager;
IMCursorManager* cursor_manager;
SFML_TextureManager* texture_manager;
SFML_WindowEventManager* event_manager;

// Clock
Clock *game_clock = NULL;

void resetView()
{
   r_window->setView( r_window->getDefaultView() );
}

void resetWindow()
{ 
   if (r_window != NULL)
      r_window->create(VideoMode(config::width(), config::height(), 32), "Summoner", Style::None);
   else 
      r_window = new RenderWindow(VideoMode(config::width(), config::height(), 32), "Summoner", Style::None);
}

///////////////////////////////////////////////////////////////////////////////
// Definitions


///////////////////////////////////////////////////////////////////////////////
// Menus

// Here's what the various menu buttons can DO

void openMap()
{
   menu_state = MENU_MAIN | MENU_PRI_MAP;

   setMapListener( true );
}

void closeMap()
{
   setMapListener( false );
}

// MAP

void startLevel( int level )
{
   menu_state = MENU_MAIN | MENU_PRI_INGAME;

   loadLevel( level );
   setLevelListener(true);
}

// OPTIONS MENUS

int av_selected_resolution;

void openOptionsMenu()
{
   menu_state = (menu_state | MENU_SEC_OPTIONS) & (~(MENU_SEC_AV_OPTIONS | MENU_SEC_INPUT_OPTIONS));
}

// AV
void openAVOptions()
{
   menu_state = menu_state | MENU_SEC_OPTIONS | MENU_SEC_AV_OPTIONS;
}

int applyAVOptions()
{
   log("Applying AV Options");
   int w, h, f;
   f = 0;
   if (av_selected_resolution == 0) {
      w = 800;
      h = 600;
   } else if (av_selected_resolution == 1) {
      w = 1200;
      h = 900;
   }

   config::setWindow( w, h, f );

   resetWindow();

   return 0;
}

void closeAVOptions()
{
   applyAVOptions();
   openOptionsMenu();
}

// Input
void openInputOptions()
{
   menu_state = menu_state | MENU_SEC_OPTIONS | MENU_SEC_INPUT_OPTIONS;
}

int applyInputOptions()
{
   return 0;
}

void closeInputOptions()
{
   applyInputOptions();
   openOptionsMenu();
}

void closeOptionsMenu()
{
   if (menu_state & MENU_SEC_AV_OPTIONS)
      applyAVOptions();
   if (menu_state & MENU_SEC_INPUT_OPTIONS)
      applyInputOptions();
   menu_state = menu_state & (~(MENU_SEC_OPTIONS | MENU_SEC_AV_OPTIONS | MENU_SEC_INPUT_OPTIONS));
}


// Here's the actual buttons and shit


// SPLASH
bool initSplashGui = false;
IMButton* splashToTestLevel = NULL;
IMButton* b_splash_to_map = NULL; 
IMButton* b_open_options = NULL;
Sprite* splashScreen = NULL;

int initSplashMenuGui()
{
   splashScreen = new Sprite( *(texture_manager->getTexture("SplashScreen0.png") ));

   b_open_options = new IMButton();
   b_open_options->setPosition( 300, 200 );
   b_open_options->setSize( 50, 50 );
   b_open_options->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_open_options->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_open_options->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   gui_manager->registerWidget( "Splash menu - open options", b_open_options);

   splashToTestLevel = new IMButton();
   splashToTestLevel->setPosition( 10, 10 );
   splashToTestLevel->setSize( 40, 40 );
   splashToTestLevel->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   splashToTestLevel->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   splashToTestLevel->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   gui_manager->registerWidget( "splashToTestLevel", splashToTestLevel);

   b_splash_to_map = new IMButton();
   b_splash_to_map->setPosition( 500, 300 );
   b_splash_to_map->setSize( 200, 100 );
   b_splash_to_map->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_splash_to_map->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_splash_to_map->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   gui_manager->registerWidget( "Splash menu - go to map", b_splash_to_map);

   initSplashGui = true;
   return 0;
}

void splashMenu()
{
   if (!initSplashGui) {
      initSplashMenuGui();
   }
   else
   {
      // Draw
      SFML_GlobalRenderWindow::get()->draw(*splashScreen);

      if (splashToTestLevel->doWidget())
         startLevel( -1 );

      if (b_open_options->doWidget())
         openOptionsMenu();

      if (b_splash_to_map->doWidget())
         openMap();
   }
}


// OPTIONS
bool initOptionsMenu = false;
IMButton* b_exit_options_menu = NULL,
        * b_av_options = NULL,
        * b_input_options = NULL;

int initOptionsMenuGui()
{
   b_exit_options_menu = new IMButton();
   b_exit_options_menu->setPosition( 10, 10 );
   b_exit_options_menu->setSize( 40, 40 );
   b_exit_options_menu->setNormalTexture( texture_manager->getTexture( "GuiExitX.png" ) );
   b_exit_options_menu->setHoverTexture( texture_manager->getTexture( "GuiExitX.png" ) );
   b_exit_options_menu->setPressedTexture( texture_manager->getTexture( "GuiExitX.png" ) );
   gui_manager->registerWidget( "Close Options Menu", b_exit_options_menu);

   b_av_options = new IMButton();
   b_av_options->setPosition( 300, 400 );
   b_av_options->setSize( 80, 80 );
   b_av_options->setNormalTexture( texture_manager->getTexture( "BasicTree1.png" ) );
   b_av_options->setHoverTexture( texture_manager->getTexture( "BasicTree1.png" ) );
   b_av_options->setPressedTexture( texture_manager->getTexture( "BasicTree1.png" ) );
   gui_manager->registerWidget( "Open AV Option", b_av_options);

   b_input_options = new IMButton();
   b_input_options->setPosition( 500, 400 );
   b_input_options->setSize( 80, 80 );
   b_input_options->setNormalTexture( texture_manager->getTexture( "BasicTree2.png" ) );
   b_input_options->setHoverTexture( texture_manager->getTexture( "BasicTree2.png" ) );
   b_input_options->setPressedTexture( texture_manager->getTexture( "BasicTree2.png" ) );
   gui_manager->registerWidget( "Open Input Options", b_input_options);

   initOptionsMenu = true;

   return 0;
}

void optionsMenu()
{
   if (!initOptionsMenu) {
      initOptionsMenuGui();
   }
   else
   {
      RenderWindow* r_wind = SFML_GlobalRenderWindow::get();
      r_wind->clear(Color::Red);

      if (b_exit_options_menu->doWidget())
         closeOptionsMenu();
      if (b_av_options->doWidget())
         openAVOptions();
      if (b_input_options->doWidget())
         openInputOptions();
   }
}


// AV OPTIONS
bool initAVOptionsMenu = false;
IMButton* b_exit_av_options_menu = NULL;
IMTextButton* b_800x600 = NULL;
IMTextButton* b_1200x900 = NULL;
IMTextButton* b_av_apply = NULL;

string s_800x600 = "800 x 600";
string s_1200x900 = "1200 x 900";
string s_apply_av = "Apply changes";

// Selections are:
// 0 - 800x600
// 1 - 1200x900

int initAVOptionsMenuGui()
{
   b_exit_av_options_menu = new IMButton();
   b_exit_av_options_menu->setPosition( 10, 10 );
   b_exit_av_options_menu->setSize( 40, 40 );
   b_exit_av_options_menu->setNormalTexture( texture_manager->getTexture( "GuiExitX.png" ) );
   b_exit_av_options_menu->setHoverTexture( texture_manager->getTexture( "GuiExitX.png" ) );
   b_exit_av_options_menu->setPressedTexture( texture_manager->getTexture( "GuiExitX.png" ) );
   gui_manager->registerWidget( "Close AV Options Menu", b_exit_av_options_menu);

   b_800x600 = new IMTextButton();
   b_800x600->setPosition( 250, 300 );
   b_800x600->setSize( 300, 40 );
   b_800x600->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_800x600->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_800x600->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_800x600->setText( &s_800x600 );
   b_800x600->setFont( menu_font );
   b_800x600->setTextSize( 16 );
   b_800x600->setTextColor( sf::Color::Black );
   b_800x600->centerText();
   gui_manager->registerWidget( "Resolution 800 x 600", b_800x600);

   b_1200x900 = new IMTextButton();
   b_1200x900->setPosition( 250, 350 );
   b_1200x900->setSize( 300, 40 );
   b_1200x900->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_1200x900->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_1200x900->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_1200x900->setText( &s_1200x900 );
   b_1200x900->setFont( menu_font );
   b_1200x900->setTextSize( 16 );
   b_1200x900->setTextColor( sf::Color::Black );
   b_1200x900->centerText();
   gui_manager->registerWidget( "Resolution 1200 x 900", b_1200x900);

   b_av_apply = new IMTextButton();
   b_av_apply->setPosition( 300, 400 );
   b_av_apply->setSize( 200, 40 );
   b_av_apply->setNormalTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_av_apply->setHoverTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_av_apply->setPressedTexture( texture_manager->getTexture( "OrderButtonBase.png" ) );
   b_av_apply->setText( &s_apply_av );
   b_av_apply->setFont( menu_font );
   b_av_apply->setTextSize( 16 );
   b_av_apply->setTextColor( sf::Color::Black );
   b_av_apply->centerText();
   gui_manager->registerWidget( "Apply AV Settings", b_av_apply);

   if (config::width() == 1200 && config::height() == 900)
      av_selected_resolution = 1;
   else
      av_selected_resolution = 0;

   initAVOptionsMenu = true;

   return 0;
}

void AVOptionsMenu()
{
   if (!initAVOptionsMenu) {
      initAVOptionsMenuGui();
   }
   else
   {
      RenderWindow* r_wind = SFML_GlobalRenderWindow::get();
      r_wind->clear(Color::Red);

      if (b_exit_av_options_menu->doWidget())
         closeAVOptions();
      if (b_800x600->doWidget())
         av_selected_resolution = 0;
      if (b_1200x900->doWidget())
         av_selected_resolution = 1;
      if (b_av_apply->doWidget())
         applyAVOptions();
   }

}

int progressiveInitMenus()
{
   static int count = 0;

   menu_font = new Font();
   if (menu_font->loadFromFile("/usr/share/fonts/TTF/LiberationMono-Regular.ttf"))
      log("Successfully loaded font");

   if (count == 0) { 
      initSplashMenuGui();
      count = 1;
   } else if (count == 1) {
      initOptionsMenuGui();
      count = 2;
   } else if (count == 2) {
      initAVOptionsMenuGui();
      count = 3;
   } else if (count == 3) {
      //initInputOptionsMenuGui();
      return 0; // done
   }

   return -1; // repeat
}

///////////////////////////////////////////////////////////////////////////////
// Loading

#define PROGRESS_CAP 100

#define ASSET_TYPE_END 0
#define ASSET_TYPE_TEXTURE 1
#define ASSET_TYPE_SOUND 2

struct Asset
{
   int type;
   string path;

   Asset(int t, string& s) { type=t; path=s; }
};

deque<Asset> asset_list;

int loadAssetList()
{
   /* Actually read AssetList.txt 
    */ 
   string type, path;
   ifstream alist_in;
   alist_in.open("res/AssetList.txt");

   while (true) {
      // Get next line
      alist_in >> type >> path;
      if (alist_in.eof())
         break;

      if (alist_in.bad()) {
         log("Error in AssetList loading - alist_in is 'bad' - INVESTIGATE");
         break;
      }

      if (type == "IMAGE")
         asset_list.push_back( Asset( ASSET_TYPE_TEXTURE, path ) );
   }

   return 0;
}

int loadAsset( Asset& asset )
{
   if (asset.type == ASSET_TYPE_TEXTURE)
      texture_manager->addTexture( asset.path );

   return 0;
}

// preload gets everything required by the loading animation
int preload()
{
   log("Preload");

   menu_state = MENU_LOADING;
   return 0;
}

int progressiveInit()
{
   static int progress = 0;
   log ("Progressive init");

   switch (progress) {
      case 0:
         if (progressiveInitMenus() == 0)
            progress = 1;
         break;
      case 1:
         if (initLevelGui() == 0)
            progress = 2;
         break;
      case 2:
         if (initProjectiles() == 0)
            progress = 3;
         break;
      case 3:
         if (initMap() == 0)
            progress = 4;
         break;

      default:
         return 0;
   }

   return 1;
}

// progressiveLoad works through the assets a bit at a time
int progressiveLoad()
{
   static int asset_segment = 0;
   int progress = 0;

   switch (asset_segment) {
      case 0: // Load asset list
         loadAssetList();
         asset_segment = 1;
         return -1;
      case 1: // Load up to PROGRESS_CAP assets
         while (!asset_list.empty())
         {
            loadAsset( asset_list.front() );
            asset_list.pop_front();
            if (progress++ > PROGRESS_CAP)
               return -1;
         }
         asset_segment = 2;
         return -1;
      case 2:
         if (progressiveInit() == 0)
            asset_segment = 3;
         return -1;
      default:
         menu_state = MENU_POSTLOAD;
   }

   return 0;
}

// postload clears the loading structures and drops us in the main menu
int postload()
{
   log("Postload");
   //delete(asset_list);

   menu_state = MENU_MAIN | MENU_PRI_SPLASH;
   return 0;
}

int loadingAnimation(int dt)
{
   r_window->clear(Color::Black);
   r_window->display();
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Main Listeners

struct MainWindowListener : public My_SFML_WindowListener
{
   virtual bool windowClosed( );
   virtual bool windowResized( const Event::SizeEvent &resized );
   virtual bool windowLostFocus( );
   virtual bool windowGainedFocus( );
};

struct MainMouseListener : public My_SFML_MouseListener
{
   virtual bool mouseMoved( const Event::MouseMoveEvent &mouse_move );
   virtual bool mouseButtonPressed( const Event::MouseButtonEvent &mouse_button_press );
   virtual bool mouseButtonReleased( const Event::MouseButtonEvent &mouse_button_release );
   virtual bool mouseWheelMoved( const Event::MouseWheelEvent &mouse_wheel_move );
};

struct MainKeyListener : public My_SFML_KeyListener
{
   virtual bool keyPressed( const Event::KeyEvent &key_press );
   virtual bool keyReleased( const Event::KeyEvent &key_release );
};

// Window
bool MainWindowListener::windowClosed( )
{
   shutdown(1,1);
   return true;
}

bool MainWindowListener::windowResized( const Event::SizeEvent &resized )
{

   return true;
}

bool MainWindowListener::windowLostFocus( )
{

   return true;
}

bool MainWindowListener::windowGainedFocus( )
{

   return true;
}

// Mouse
bool MainMouseListener::mouseMoved( const Event::MouseMoveEvent &mouse_move )
{
   return true;
}

bool MainMouseListener::mouseButtonPressed( const Event::MouseButtonEvent &mouse_button_press )
{
   log("Clicked");
   IMCursorManager::getSingleton().setCursor( IMCursorManager::CLICKING );
   return true;
}

bool MainMouseListener::mouseButtonReleased( const Event::MouseButtonEvent &mouse_button_release )
{
   log("Un-Clicked");
   IMCursorManager::getSingleton().setCursor( IMCursorManager::DEFAULT );
   return true;
}

bool MainMouseListener::mouseWheelMoved( const Event::MouseWheelEvent &mouse_wheel_move )
{
   return true;
}

// Key
bool MainKeyListener::keyPressed( const Event::KeyEvent &key_press )
{
   if (key_press.code == Keyboard::Q)
      shutdown(1,1);

   if (key_press.code == Keyboard::Escape) {
      if (menu_state & MENU_SEC_OPTIONS)
         closeOptionsMenu();
      else
         openOptionsMenu();
   }

   return true;
}

bool MainKeyListener::keyReleased( const Event::KeyEvent &key_release )
{
   return true;
}


///////////////////////////////////////////////////////////////////////////////
// Main loop

int mainLoop( int dt )
{
   event_manager->handleEvents();

   r_window->clear(Color::Yellow);

   gui_manager->begin();

   if (menu_state & MENU_SEC_AV_OPTIONS) {
      AVOptionsMenu();
   } else if (menu_state & MENU_SEC_INPUT_OPTIONS) {
      //InputOptionsMenu();
   } else if (menu_state & MENU_SEC_OPTIONS) {
      optionsMenu();
   } else if (menu_state & MENU_PRI_SPLASH) {
      splashMenu();

   } else if (menu_state & MENU_PRI_MAP) {
      int map_result = drawMap(dt);
      if (map_result == -2) {
         // Various map options etc
      } else if (map_result == -1 || map_result >= 1) {
         startLevel( map_result );
      }

   } else if (menu_state & MENU_PRI_INGAME) {
      updateLevel(dt);
      drawLevel();

   } else if (menu_state & MENU_PRI_ANIMATION) {

   } 
   
   gui_manager->end();

   resetView();

   cursor_manager->drawCursor();

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Execution starts here

int runApp()
{
   // Read the config files
   config::load();
   config::save();

   // Setup the window
   shutdown(1,0);
   resetWindow();

   // Setup various resource managers
   gui_manager = &IMGuiManager::getSingleton();
   cursor_manager = &IMCursorManager::getSingleton();
   texture_manager = &SFML_TextureManager::getSingleton();
   event_manager = &SFML_WindowEventManager::getSingleton();

   SFML_GlobalRenderWindow::set( r_window );
   gui_manager->setRenderWindow( r_window );

   texture_manager->addSearchDirectory( "res/" ); 

   // Setup event listeners
   MainWindowListener w_listener;
   MainMouseListener m_listener;
   MainKeyListener k_listener;
   event_manager->addWindowListener( &w_listener, "main" );
   event_manager->addMouseListener( &m_listener, "main" );
   event_manager->addKeyListener( &k_listener, "main" );

   // We need to load a loading screen
   menu_state = MENU_PRELOAD;

   // Timing!
   game_clock = new Clock();
   unsigned int old_time, new_time, dt;
   old_time = 0;
   new_time = 0;

   // Loading
   preload();
   while (progressiveLoad())
      loadingAnimation(game_clock->getElapsedTime().asMilliseconds());
   postload();

   // Now setup some things using our new resources
   cursor_manager->createCursor( IMCursorManager::DEFAULT, texture_manager->getTexture( "FingerCursor.png" ), 0, 0, 40, 60);
   cursor_manager->createCursor( IMCursorManager::CLICKING, texture_manager->getTexture( "FingerCursorClick.png" ), 0, 0, 40, 60);

//////////////////////////////////////////////////////////////////////
// Main Loop
//////////////////////////////////////////////////////////////////////
   log("Entering main loop");
   while (shutdown() == 0)
   {
      if (menu_state & MENU_MAIN) { 

         // Get dt
         new_time = game_clock->getElapsedTime().asMilliseconds();
         dt = new_time - old_time;
         old_time = new_time;

         mainLoop( dt );

         r_window->display();
      }
   }
   log("End main loop");
   
   r_window->close();

   return 0;
}

};

int main(int argc, char* argv[])
{
   return sum::runApp();
}
