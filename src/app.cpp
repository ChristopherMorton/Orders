// Summoner includes
#include "shutdown.h"
#include "config.h"
#include "savestate.h" 
#include "level.h"
#include "listeners.h"
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

// C includes
#include <stdio.h>

using namespace sf;

namespace sum
{

// Global app-state variables

// Managers
IMGuiManager* gui_manager;
IMCursorManager* cursor_manager;
SFML_TextureManager* texture_manager;
SFML_WindowEventManager* event_manager;

// Config
Config config;

// Player data - i.e. save state
String player_name;
LevelRecord *level_scores;

// Menu state - i.e. which parts of the app are active
typedef unsigned int MenuState;
MenuState menu_state;
// MenuState masks:
// startup:
#define MENU_PRELOAD 0x1
#define MENU_LOADING 0x2
#define MENU_POSTLOAD 0x4
#define MENU_MAIN 0x8
// where in the app are we?
#define MENU_PRI_SPLASH 0x100
#define MENU_PRI_MAP 0x200
#define MENU_PRI_INGAME 0x400
// which secondary windows are up?
#define MENU_SEC_GAME_OPTIONS 0x10000
#define MENU_SEC_AV_OPTIONS 0x20000
#define MENU_SEC_INPUT_OPTIONS 0x40000
#define MENU_ERROR_DIALOG 0x100000

// Specific data for the menu sections
// Map view:
float mv_x_base, mv_y_base, mv_x_extent, mv_y_extent;


///////////////////////////////////////////////////////////////////////////////
// Loading
///////////////////////////////////////////////////////////////////////////////

#define PROGESS_CAP 100

#define ASSET_TYPE_END 0
#define ASSET_TYPE_TEXTURE 1

struct Asset
{
   int type;
   string path;
};

Asset* asset_list;

int loadAssetList()
{
   asset_list = new Asset[2];
   
   asset_list[0].type = ASSET_TYPE_TEXTURE;
   asset_list[0].path = "FingerCursor.png";

   asset_list[1].type = ASSET_TYPE_END;

   return 0;
}

int loadAsset( Asset* asset )
{
   if (NULL == asset)
      return -1;

   if (asset->type == ASSET_TYPE_TEXTURE)
      texture_manager->getTexture( asset->path );

   return 0;
}

// preload gets everything required by the loading animation
int preload()
{
   log("Preload");

   menu_state = MENU_LOADING;
   return 0;
}

// progressiveLoad works through the assets a bit at a time
int progressiveLoad()
{
   static int asset_segment = 0;
   static Asset* next = 0;

   int progress = 0;

   if (asset_segment == 0)
   {
      // Need to load asset list first
      loadAssetList();
      asset_segment = 1;
      next = &asset_list[0];
      return -1;
   }

   while (next->type != ASSET_TYPE_END)
   {
      loadAsset( next );
      next++;
      if (progress++ > PROGESS_CAP)
         return -1;
   }

   menu_state = MENU_POSTLOAD;
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
   sf::RenderWindow* r_window = SFML_GlobalRenderWindow::get();
   r_window->clear(sf::Color::Black);
   r_window->display();
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Execution starts here
///////////////////////////////////////////////////////////////////////////////
int runApp()
{
   // Setup the window
   shutdown(1,0);
   sf::RenderWindow r_window(sf::VideoMode(800, 600, 32), "Summoner");

   // Setup various resource managers
   gui_manager = &IMGuiManager::getSingleton();
   cursor_manager = &IMCursorManager::getSingleton();
   texture_manager = &SFML_TextureManager::getSingleton();
   event_manager = &SFML_WindowEventManager::getSingleton();

   SFML_GlobalRenderWindow::set( &r_window );
   gui_manager->setRenderWindow( &r_window );

   texture_manager->addSearchDirectory( "res/" ); 

   cursor_manager->createCursor( IMCursorManager::DEFAULT, texture_manager->getTexture( "FingerCursor.png" ), 0, 0, 40, 60);
   cursor_manager->createCursor( IMCursorManager::CLICKING, texture_manager->getTexture( "FingerCursorClick.png" ), 0, 0, 40, 60);

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
   sf::Clock clock;
   unsigned int dt;

//////////////////////////////////////////////////////////////////////
// Main Loop
//////////////////////////////////////////////////////////////////////
   log("Entering main loop");
   while (shutdown() == 0)
   {
      dt = clock.getElapsedTime().asMilliseconds();
      switch (menu_state)
      {
      case MENU_PRELOAD:
         preload();
      case MENU_LOADING:
         loadingAnimation(dt);
         if(progressiveLoad())
            break; // not complete
      case MENU_POSTLOAD:
         postload();
      default:
         if (menu_state & MENU_MAIN) { 

            event_manager->handleEvents();

            r_window.clear(sf::Color::Yellow);

            gui_manager->begin();

            gui_manager->end();

            if (sf::Mouse::isButtonPressed( sf::Mouse::Left ))
               IMCursorManager::getSingleton().setCursor( IMCursorManager::CLICKING );

            cursor_manager->drawCursor();

            r_window.display();
         }
      }
   }
   log("End main loop");
   
   r_window.close();

   return 0;
}

};

int main(int argc, char* argv[])
{
   return sum::runApp();
}
