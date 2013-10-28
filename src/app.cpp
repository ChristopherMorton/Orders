// Summoner includes
#include "shutdown.h"
#include "config.h"
#include "savestate.h" 
#include "level.h"
#include "listeners.h"

// SFML includes
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

// lib includes
#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_WindowEventManager.hpp"

// C includes
#include <stdio.h>

using namespace sf;

namespace sum
{

// Global app-state variables

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

int runApp()
{
   shutdown(1,0);
   sf::RenderWindow r_window(sf::VideoMode(800, 600), "Summoner");
   SFML_GlobalRenderWindow::set( &r_window );
   SFML_WindowEventManager& event_manager = SFML_WindowEventManager::getSingleton();
   MainWindowListener w_listener;
   MainMouseListener m_listener;
   MainKeyListener k_listener;
   event_manager.addWindowListener( &w_listener, "main" );
   event_manager.addMouseListener( &m_listener, "main" );
   event_manager.addKeyListener( &k_listener, "main" );

//////////////////////////////////////////////////////////////////////
// Main Loop
//////////////////////////////////////////////////////////////////////
   while (shutdown() == 0)
   {
      event_manager.handleEvents();
      r_window.clear(sf::Color::Black);
      r_window.display();
   }
   
   r_window.close();

   return 0;
}

};

int main(int argc, char* argv[])
{
   return sum::runApp();
}
