// Summoner includes
#include "shutdown.h"
#include "config.h"
#include "savestate.h" 
#include "level.h"
#include "projectile.h"
#include "savestate.h"
#include "menustate.h"
#include "map.h"
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
#include "IMEdgeTextButton.hpp"

// C includes
#include <stdio.h>

// C++ includes
#include <deque>
#include <fstream>
#include <sstream>

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

void refitGuis(); // Predeclared

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

   config::save();

   refitGuis();
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
IMEdgeTextButton *b_splash_to_map = NULL; 
string s_splash_to_map = "Play!";
IMButton *b_open_options = NULL;
IMButton *b_splashToTestLevel = NULL;
Sprite* splashScreen = NULL;

void fitGui_Splash()
{
   if (!initSplashGui) return;

   b_open_options->setPosition( 0, 0 );
   b_open_options->setSize( 40, 40 );

   //b_splashToTestLevel->setPosition( config::height() - 50, config::width() - 50 );
   b_splashToTestLevel->setPosition( 40, 40 );
   b_splashToTestLevel->setSize( 50, 50 );

   b_splash_to_map->setPosition( 500, 300 );
   b_splash_to_map->setSize( 200, 100 );
   b_splash_to_map->centerText();
}

int initSplashMenuGui()
{
   splashScreen = new Sprite( *(texture_manager->getTexture("SplashScreen0.png") ));

   b_open_options = new IMButton();
   b_open_options->setAllTextures( texture_manager->getTexture( "GearIcon.png" ) );
   gui_manager->registerWidget( "Splash menu - open options", b_open_options);

   b_splashToTestLevel = new IMButton();
   b_splashToTestLevel->setAllTextures( texture_manager->getTexture( "OrderButtonBase.png" ) );
   gui_manager->registerWidget( "splashToTestLevel", b_splashToTestLevel);

   b_splash_to_map = new IMEdgeTextButton();
   b_splash_to_map->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_splash_to_map->setCornerAllTextures( texture_manager->getTexture( "UICorner3px.png" ) );
   b_splash_to_map->setEdgeAllTextures( texture_manager->getTexture( "UIEdge3px.png" ) );
   b_splash_to_map->setEdgeWidth( 3 );
   b_splash_to_map->setText( &s_splash_to_map );
   b_splash_to_map->setFont( menu_font );
   b_splash_to_map->setTextSize( 48 );
   b_splash_to_map->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Splash menu - go to map", b_splash_to_map);

   initSplashGui = true;
   fitGui_Splash();

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

      if (b_splashToTestLevel->doWidget())
         loadLevel(0);

      if (b_open_options->doWidget())
         openOptionsMenu();

      if (b_splash_to_map->doWidget())
         openMap();
   }
}


// OPTIONS
bool initOptionsMenu = false;
IMButton* b_exit_options_menu = NULL;
IMTextButton* b_av_options = NULL,
            * b_input_options = NULL;

string s_av_options = "AV Options";
string s_input_options = "Input Options";

void fitGui_Options()
{
   if (!initOptionsMenu) return;

   b_exit_options_menu->setPosition( 10, 10 );
   b_exit_options_menu->setSize( 40, 40 );

   b_av_options->setPosition( 300, 400 );
   b_av_options->setSize( 80, 80 );
   b_av_options->setTextSize( 16 );
   b_av_options->centerText();

   b_input_options->setPosition( 500, 400 );
   b_input_options->setSize( 80, 80 );
   b_input_options->setTextSize( 16 );
   b_input_options->centerText();
}

int initOptionsMenuGui()
{
   b_exit_options_menu = new IMButton();
   b_exit_options_menu->setAllTextures( texture_manager->getTexture( "GuiExitX.png" ) );
   gui_manager->registerWidget( "Close Options Menu", b_exit_options_menu);

   b_av_options = new IMTextButton();
   b_av_options->setAllTextures( texture_manager->getTexture( "BasicTree1.png" ) );
   b_av_options->setText( &s_av_options );
   b_av_options->setFont( menu_font );
   b_av_options->setTextColor( sf::Color::White );
   gui_manager->registerWidget( "Open AV Option", b_av_options);

   b_input_options = new IMTextButton();
   b_input_options->setAllTextures( texture_manager->getTexture( "BasicTree2.png" ) );
   b_input_options->setText( &s_input_options );
   b_input_options->setFont( menu_font );
   b_input_options->setTextColor( sf::Color::White );
   gui_manager->registerWidget( "Open Input Options", b_input_options);

   initOptionsMenu = true;
   fitGui_Options();

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
IMEdgeTextButton* b_800x600 = NULL;
IMEdgeTextButton* b_1200x900 = NULL;
IMEdgeTextButton* b_av_apply = NULL;

string s_800x600 = "800 x 600";
string s_1200x900 = "1200 x 900";
string s_apply_av = "Apply changes";

// Selections are:
// 0 - 800x600
// 1 - 1200x900

void selectResolution( int selection )
{
   av_selected_resolution = selection;
   if (av_selected_resolution == 0) {
      b_800x600->setAllTextures( texture_manager->getTexture( "UICenterGold.png" ) );
      b_1200x900->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   } else if (av_selected_resolution == 1) {
      b_800x600->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
      b_1200x900->setAllTextures( texture_manager->getTexture( "UICenterGold.png" ) );
   }
}

void fitGui_AVOptions()
{
   if (!initAVOptionsMenu) return;

   int width = config::width(),
       height = config::height();

   float gui_tick_x = (float)width / 80.0,
         gui_tick_y = (float)height / 60.0;
   int gui_text_size = width / 50;

   b_exit_av_options_menu->setPosition( gui_tick_x, gui_tick_y );
   b_exit_av_options_menu->setSize( gui_tick_x * 4, gui_tick_y * 4 );

   b_800x600->setPosition( 25 * gui_tick_x, 30 * gui_tick_y );
   b_800x600->setSize( 30 * gui_tick_x, 4 * gui_tick_y );
   b_800x600->setTextSize( gui_text_size );
   b_800x600->centerText();

   b_1200x900->setPosition( 25 * gui_tick_x, 35 * gui_tick_y );
   b_1200x900->setSize( 30 * gui_tick_x, 4 * gui_tick_y );
   b_1200x900->setTextSize( gui_text_size );
   b_1200x900->centerText();

   b_av_apply->setPosition( 30 * gui_tick_x, 40 * gui_tick_y );
   b_av_apply->setSize( 20 * gui_tick_x, 4 * gui_tick_y );
   b_av_apply->setTextSize( gui_text_size );
   b_av_apply->centerText();
}

int initAVOptionsMenuGui()
{
   b_exit_av_options_menu = new IMButton();
   b_exit_av_options_menu->setAllTextures( texture_manager->getTexture( "GuiExitX.png" ) );
   gui_manager->registerWidget( "Close AV Options Menu", b_exit_av_options_menu);

   b_800x600 = new IMEdgeTextButton();
   b_800x600->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_800x600->setCornerAllTextures( texture_manager->getTexture( "UICorner3px.png" ) );
   b_800x600->setEdgeAllTextures( texture_manager->getTexture( "UIEdge3px.png" ) );
   b_800x600->setEdgeWidth( 3 );
   b_800x600->setText( &s_800x600 );
   b_800x600->setFont( menu_font );
   b_800x600->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Resolution 800 x 600", b_800x600);

   b_1200x900 = new IMEdgeTextButton();
   b_1200x900->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_1200x900->setCornerAllTextures( texture_manager->getTexture( "UICorner3px.png" ) );
   b_1200x900->setEdgeAllTextures( texture_manager->getTexture( "UIEdge3px.png" ) );
   b_1200x900->setEdgeWidth( 3 );
   b_1200x900->setText( &s_1200x900 );
   b_1200x900->setFont( menu_font );
   b_1200x900->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Resolution 1200 x 900", b_1200x900);

   b_av_apply = new IMEdgeTextButton();
   b_av_apply->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_av_apply->setCornerAllTextures( texture_manager->getTexture( "UICorner3px.png" ) );
   b_av_apply->setEdgeAllTextures( texture_manager->getTexture( "UIEdge3px.png" ) );
   b_av_apply->setEdgeWidth( 3 );
   b_av_apply->setText( &s_apply_av );
   b_av_apply->setFont( menu_font );
   b_av_apply->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Apply AV Settings", b_av_apply);

   if (config::width() == 1200 && config::height() == 900)
      selectResolution( 1 );
   else
      selectResolution( 0 );

   initAVOptionsMenu = true;
   fitGui_AVOptions();

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
         selectResolution( 0 );
      if (b_1200x900->doWidget())
         selectResolution( 1 );
      if (b_av_apply->doWidget())
         applyAVOptions();
   }

}

int progressiveInitMenus()
{
   static int count = 0;
   log("Progressive init - Menus");

   if (count == 0) { 
      menu_font = new Font();
      if (menu_font->loadFromFile("/usr/share/fonts/TTF/LiberationSans-Regular.ttf"))
         log("Successfully loaded font");
      count = 1;
   } else if (count == 1) {
      initSplashMenuGui();
      count = 2;
   } else if (count == 2) {
      initOptionsMenuGui();
      count = 3;
   } else if (count == 3) {
      initAVOptionsMenuGui();
      count = 4;
   } else if (count == 4) {
      //initInputOptionsMenuGui();

      count = 5;
   } else if (count == 5) {
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
   string type, path, line;
   ifstream alist_in;
   alist_in.open("res/AssetList.txt");

   while (true) {
      // Get next line
      getline( alist_in, line );
      stringstream s_line(line);
      s_line >> type;

      if (alist_in.eof())
         break;

      if (alist_in.bad()) {
         log("Error in AssetList loading - alist_in is 'bad' - INVESTIGATE");
         break;
      }

      if (type == "//") // comment
         continue;

      if (type == "IMAGE") {
         s_line >> path;
         asset_list.push_back( Asset( ASSET_TYPE_TEXTURE, path ) );
      }
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
      case 2: // Initialize a variety of things
         if (progressiveInit() == 0)
            asset_segment = 3;
         return -1;
      case 3: // Load save game
         loadSaveGame();
         saveSaveGame();
         asset_segment = 4;
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
// Propogation

void refitGuis()
{
   fitGui_Splash(); 

   fitGui_Options();
   fitGui_AVOptions();
   //fitGui_InputOptions();

   fitGui_Map();

   fitGui_Level();
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
   IMCursorManager::getSingleton().setCursor( IMCursorManager::CLICKING );
   return true;
}

bool MainMouseListener::mouseButtonReleased( const Event::MouseButtonEvent &mouse_button_release )
{
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
