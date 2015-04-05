// Summoner includes
#include "shutdown.h"
#include "config.h"
#include "savestate.h" 
#include "level.h"
#include "effects.h"
#include "savestate.h"
#include "menustate.h"
#include "util.h"
#include "map.h"
#include "focus.h"
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
#include "IMImageButton.hpp"

// C includes
#include <stdio.h>

// C++ includes
#include <deque>
#include <fstream>
#include <sstream>

using namespace sf;

///////////////////////////////////////////////////////////////////////////////
// Data ---

Font *menu_font;
Font *ingame_font;

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
   ContextSettings settings( 0, 0, 10, 2, 0 );
   if (r_window != NULL)
      r_window->create(VideoMode(config::width(), config::height(), 32), "Summoner", Style::None, settings);
   else 
      r_window = new RenderWindow(VideoMode(config::width(), config::height(), 32), "Summoner", Style::None, settings);
}

///////////////////////////////////////////////////////////////////////////////
// Definitions ---

void refitGuis(); // Predeclared

///////////////////////////////////////////////////////////////////////////////
// Menus ---

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
KeybindTarget input_selected_kb;

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
   int w = 0, h = 0, f = 0;
   if (av_selected_resolution == 0) {
      w = 800;
      h = 600;
   } else if (av_selected_resolution == 1) {
      w = 1200;
      h = 900;
   } else if (av_selected_resolution == 2) {
      w = 1280;
      h = 1024;
   }

   if (w == config::width() && h == config::height())
      return -1; // No change

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
   config::save();
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
   config::save();
}

// Here's the actual buttons and shit


// SPLASH --
bool initSplashGui = false;
IMEdgeTextButton *b_splash_to_map = NULL; 
string s_splash_to_map = "Play!";
IMButton *b_open_options = NULL;
IMButton *b_splashToTestLevel = NULL;
IMButton *b_splashToLevelEditor = NULL;
Sprite* splashScreen = NULL;

void fitGui_Splash()
{
   if (!initSplashGui) return;

   b_open_options->setPosition( 0, 0 );
   b_open_options->setSize( 40, 40 );

   b_splashToTestLevel->setPosition( 40, 40 );
   b_splashToTestLevel->setSize( 50, 50 );

   b_splashToLevelEditor->setPosition( 40, 100 );
   b_splashToLevelEditor->setSize( 50, 50 );

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

   b_splashToLevelEditor = new IMButton();
   b_splashToLevelEditor->setAllTextures( texture_manager->getTexture( "StarFull.png" ) );
   gui_manager->registerWidget( "splashToTestLevel", b_splashToLevelEditor);

   b_splash_to_map = new IMEdgeTextButton();
   b_splash_to_map->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_splash_to_map->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_splash_to_map->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
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

      if (b_splashToLevelEditor->doWidget())
         loadLevelEditor(0);

      if (b_splashToTestLevel->doWidget())
         loadLevel(-1);

      if (b_open_options->doWidget())
         openOptionsMenu();

      if (b_splash_to_map->doWidget())
         openMap();

   }
}


// OPTIONS --
bool initOptionsMenu = false;
IMButton* b_exit_options_menu = NULL;
IMEdgeTextButton* b_av_options = NULL,
                * b_input_options = NULL;
IMImageButton* b_option_display_damage = NULL;
Vector2f v_option_display_damage_txt;
int optionsMenuFontSize = 18;

string s_av_options = "AV Options";
string s_input_options = "Input Options";

void fitGui_Options()
{
   if (!initOptionsMenu) return;

   int width = config::width(),
       height = config::height();

   float gui_tick_x = (float)width / 80.0,
         gui_tick_y = (float)height / 60.0;
   optionsMenuFontSize = width / 45;

   b_exit_options_menu->setPosition( 10, 10 );
   b_exit_options_menu->setSize( 40, 40 );

   b_av_options->setPosition( gui_tick_x * 20, gui_tick_y * 42 );
   b_av_options->setSize( gui_tick_x * 16, gui_tick_y * 10 );
   b_av_options->setTextSize( optionsMenuFontSize );
   b_av_options->centerText();

   b_input_options->setPosition( gui_tick_x * 44, gui_tick_y * 42 );
   b_input_options->setSize( gui_tick_x * 16, gui_tick_y * 10 );
   b_input_options->setTextSize( optionsMenuFontSize );
   b_input_options->centerText();

   b_option_display_damage->setPosition( gui_tick_x * 18, gui_tick_y * 10 );
   b_option_display_damage->setSize( gui_tick_x * 3, gui_tick_y * 3 );
   b_option_display_damage->setImageSize( gui_tick_x * 3, gui_tick_y * 3 );
   v_option_display_damage_txt = Vector2f( gui_tick_x * 22, gui_tick_y * 10 );
   if (config::display_damage == true)
      b_option_display_damage->setImage( texture_manager->getTexture( "GuiExitX.png" ) );
   else
      b_option_display_damage->setImage( NULL );
}

int initOptionsMenuGui()
{
   b_exit_options_menu = new IMButton();
   b_exit_options_menu->setAllTextures( texture_manager->getTexture( "GuiExitX.png" ) );
   gui_manager->registerWidget( "Close Options Menu", b_exit_options_menu);

   b_av_options = new IMEdgeTextButton();
   b_av_options->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_av_options->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_av_options->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_av_options->setEdgeWidth( 3 );
   b_av_options->setText( &s_av_options );
   b_av_options->setFont( menu_font );
   b_av_options->setTextColor( Color::Black );
   gui_manager->registerWidget( "Open AV Option", b_av_options);

   b_input_options = new IMEdgeTextButton();
   b_input_options->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_input_options->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_input_options->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_input_options->setEdgeWidth( 3 );
   b_input_options->setText( &s_input_options );
   b_input_options->setFont( menu_font );
   b_input_options->setTextColor( Color::Black );
   gui_manager->registerWidget( "Open Input Options", b_input_options);

   b_option_display_damage = new IMImageButton();
   b_option_display_damage->setNormalTexture( texture_manager->getTexture( "CountButtonBase.png" ) );
   b_option_display_damage->setHoverTexture( texture_manager->getTexture( "CountButtonHover.png" ) );
   b_option_display_damage->setPressedTexture( texture_manager->getTexture( "CountButtonPressed.png" ) );
   b_option_display_damage->setImage( texture_manager->getTexture( "GuiExitX.png" ) );
   b_option_display_damage->setImageOffset( 0, 0 );
   gui_manager->registerWidget( "Toggle Display Damage", b_option_display_damage);

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
      r_wind->clear( Color( 155, 155, 155, 255 ) );

      if (b_exit_options_menu->doWidget())
         closeOptionsMenu();
      if (b_av_options->doWidget())
         openAVOptions();
      if (b_input_options->doWidget())
         openInputOptions();
      if (b_option_display_damage->doWidget()) {
         config::display_damage = !config::display_damage;
         if (config::display_damage == true)
            b_option_display_damage->setImage( texture_manager->getTexture( "GuiExitX.png" ) );
         else
            b_option_display_damage->setImage( NULL );
      }

      Text txt;
      txt.setFont( *menu_font );
      txt.setColor( Color::Black );
      txt.setCharacterSize( optionsMenuFontSize );

      txt.setString( String( "Display Damage Numbers" ) ); 
      txt.setPosition( v_option_display_damage_txt );
      r_wind->draw( txt );
   }
}


// AV OPTIONS --
bool initAVOptionsMenu = false;
IMEdgeTextButton* b_800x600 = NULL;
IMEdgeTextButton* b_1200x900 = NULL;
IMEdgeTextButton* b_1280x1024 = NULL;
IMEdgeTextButton* b_av_apply = NULL;

string s_800x600 = "800 x 600";
string s_1200x900 = "1200 x 900";
string s_1280x1024 = "1280 x 1024";
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
      b_1280x1024->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   } else if (av_selected_resolution == 1) {
      b_800x600->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
      b_1200x900->setAllTextures( texture_manager->getTexture( "UICenterGold.png" ) );
      b_1280x1024->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   } else if (av_selected_resolution == 2) {
      b_800x600->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
      b_1200x900->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
      b_1280x1024->setAllTextures( texture_manager->getTexture( "UICenterGold.png" ) );
   }
}

void fitGui_AVOptions()
{
   if (!initAVOptionsMenu) return;

   int width = config::width(),
       height = config::height();

   float gui_tick_x = (float)width / 80.0,
         gui_tick_y = (float)height / 60.0;

   b_800x600->setPosition( 25 * gui_tick_x, 30 * gui_tick_y );
   b_800x600->setSize( 30 * gui_tick_x, 4 * gui_tick_y );
   b_800x600->setTextSize( optionsMenuFontSize );
   b_800x600->centerText();

   b_1200x900->setPosition( 25 * gui_tick_x, 35 * gui_tick_y );
   b_1200x900->setSize( 30 * gui_tick_x, 4 * gui_tick_y );
   b_1200x900->setTextSize( optionsMenuFontSize );
   b_1200x900->centerText();

   b_1280x1024->setPosition( 25 * gui_tick_x, 40 * gui_tick_y );
   b_1280x1024->setSize( 30 * gui_tick_x, 4 * gui_tick_y );
   b_1280x1024->setTextSize( optionsMenuFontSize );
   b_1280x1024->centerText();

   b_av_apply->setPosition( 30 * gui_tick_x, 45 * gui_tick_y );
   b_av_apply->setSize( 20 * gui_tick_x, 4 * gui_tick_y );
   b_av_apply->setTextSize( optionsMenuFontSize );
   b_av_apply->centerText();
}

int initAVOptionsMenuGui()
{
   b_800x600 = new IMEdgeTextButton();
   b_800x600->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_800x600->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_800x600->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_800x600->setEdgeWidth( 3 );
   b_800x600->setText( &s_800x600 );
   b_800x600->setFont( menu_font );
   b_800x600->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Resolution 800 x 600", b_800x600);

   b_1200x900 = new IMEdgeTextButton();
   b_1200x900->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_1200x900->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_1200x900->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_1200x900->setEdgeWidth( 3 );
   b_1200x900->setText( &s_1200x900 );
   b_1200x900->setFont( menu_font );
   b_1200x900->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Resolution 1200 x 900", b_1200x900);

   b_1280x1024 = new IMEdgeTextButton();
   b_1280x1024->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_1280x1024->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_1280x1024->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_1280x1024->setEdgeWidth( 3 );
   b_1280x1024->setText( &s_1280x1024 );
   b_1280x1024->setFont( menu_font );
   b_1280x1024->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Resolution 1280 x 1024", b_1280x1024);

   b_av_apply = new IMEdgeTextButton();
   b_av_apply->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_av_apply->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_av_apply->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_av_apply->setEdgeWidth( 3 );
   b_av_apply->setText( &s_apply_av );
   b_av_apply->setFont( menu_font );
   b_av_apply->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Apply AV Settings", b_av_apply);

   if (config::width() == 1200 && config::height() == 900)
      selectResolution( 1 );
   else if (config::width() == 1280 && config::height() == 1024)
      selectResolution( 2 );
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
      r_wind->clear( Color( 155, 155, 155, 255 ) );

      if (b_exit_options_menu->doWidget())
         closeAVOptions();
      if (b_800x600->doWidget())
         selectResolution( 0 );
      if (b_1200x900->doWidget())
         selectResolution( 1 );
      if (b_1280x1024->doWidget())
         selectResolution( 2 );
      if (b_av_apply->doWidget())
         applyAVOptions();
   }

}

// INPUT OPTIONS --
bool initInputOptionsMenu = false;
IMEdgeTextButton *b_io_bind_key = NULL,
                 *b_io_clear_bind = NULL,
                 *b_io_pause = NULL,
                 *b_io_show_keybinds = NULL;
IMButton *b_io_camera_left = NULL,
         *b_io_camera_up = NULL,
         *b_io_camera_right = NULL,
         *b_io_camera_down = NULL,
         *b_io_zoom_in = NULL,
         *b_io_zoom_out = NULL;
                 
IMButton* b_overlay = NULL;
float io_x_1 = 300, io_x_2 = 800, io_x_3 = 1000, io_x_4 = 200, io_x_5 = 900,
      io_x_4_diff = 50,
      io_y_1 = 150, io_y_2 = 200, io_y_3 = 300, io_y_4 = 400,
      io_y_3_diff = 50, io_y_4_diff = 25,
      io_button_size = 40, io_spacer = 10;

string s_io_bind_key = "Bind";
string s_io_clear_bind = "Clear";
string s_io_pause = "Pause";
string s_io_show_keybinds = "Show Keybinds";

void fitGui_InputOptions()
{
   if (!initInputOptionsMenu) return;

   int width = config::width(),
       height = config::height();

   float gui_tick_x = (float)width / 80.0,
         gui_tick_y = (float)height / 60.0;

   io_button_size = 4 * gui_tick_x;
   io_spacer = gui_tick_x;

   io_x_1 = (20 * gui_tick_x);
   io_x_2 = (52 * gui_tick_x);
   io_x_3 = (63 * gui_tick_x);
   io_x_4 = (12 * gui_tick_x);
   io_x_5 = (57 * gui_tick_x);
   io_x_4_diff = io_button_size + io_spacer;

   io_y_1 = (10 * gui_tick_y);
   io_y_2 = io_y_1 + io_button_size + io_spacer;
   io_y_3 = io_y_2 + (io_button_size * 2) + io_spacer;
   io_y_4 = io_y_3 + io_button_size + io_spacer;
   io_y_3_diff = io_button_size + io_spacer;
   io_y_4_diff = (io_button_size + io_spacer) / 2;

   b_io_bind_key->setSize( (io_x_3 - io_x_2) - io_spacer, (io_button_size * 2) - (io_spacer * 2) );
   b_io_bind_key->setPosition( io_x_2, io_y_1 + io_spacer );
   b_io_bind_key->setTextSize( optionsMenuFontSize );
   b_io_bind_key->centerText();

   b_io_clear_bind->setSize( (io_x_3 - io_x_2) - io_spacer, (io_button_size * 2) - (io_spacer * 2) );
   b_io_clear_bind->setPosition( io_x_3, io_y_1 + io_spacer );
   b_io_clear_bind->setTextSize( optionsMenuFontSize );
   b_io_clear_bind->centerText();

   b_io_pause->setSize( (width - io_x_5) - io_button_size, io_button_size );
   b_io_pause->setPosition( io_x_5, io_y_3 );
   b_io_pause->setTextSize( optionsMenuFontSize );
   b_io_pause->centerText();

   b_io_show_keybinds->setSize( (width - io_x_5) - io_button_size, io_button_size );
   b_io_show_keybinds->setPosition( io_x_5, io_y_3 + io_y_3_diff );
   b_io_show_keybinds->setTextSize( optionsMenuFontSize );
   b_io_show_keybinds->centerText();

   b_io_camera_left->setSize( io_button_size, io_button_size );
   b_io_camera_left->setPosition( io_x_4, io_y_4 + io_y_4_diff );

   b_io_camera_right->setSize( io_button_size, io_button_size );
   b_io_camera_right->setPosition( io_x_4 + (2 * io_x_4_diff), io_y_4 + io_y_4_diff );

   b_io_camera_up->setSize( io_button_size, io_button_size );
   b_io_camera_up->setPosition( io_x_4 + io_x_4_diff, io_y_4 );

   b_io_camera_down->setSize( io_button_size, io_button_size );
   b_io_camera_down->setPosition( io_x_4 + io_x_4_diff, io_y_4 + (2 * io_y_4_diff) );

   b_io_zoom_in->setSize( io_button_size, io_button_size );
   b_io_zoom_in->setPosition( io_x_4 + (4 * io_x_4_diff), io_y_4 );

   b_io_zoom_out->setSize( io_button_size, io_button_size );
   b_io_zoom_out->setPosition( io_x_4 + (4 * io_x_4_diff), io_y_4 + (2 * io_y_4_diff) );

   b_overlay->setSize( width, height );
   b_overlay->setPosition( 0, 0 );
}

int initInputOptionsMenuGui()
{
   b_io_bind_key = new IMEdgeTextButton();
   b_io_bind_key->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_io_bind_key->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_io_bind_key->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_io_bind_key->setEdgeWidth( 3 );
   b_io_bind_key->setText( &s_io_bind_key );
   b_io_bind_key->setFont( menu_font );
   b_io_bind_key->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Bind Key", b_io_bind_key);

   b_io_clear_bind = new IMEdgeTextButton();
   b_io_clear_bind->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_io_clear_bind->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_io_clear_bind->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_io_clear_bind->setEdgeWidth( 3 );
   b_io_clear_bind->setText( &s_io_clear_bind );
   b_io_clear_bind->setFont( menu_font );
   b_io_clear_bind->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Clear Bind", b_io_clear_bind);

   b_io_pause = new IMEdgeTextButton();
   b_io_pause->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_io_pause->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_io_pause->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_io_pause->setEdgeWidth( 3 );
   b_io_pause->setText( &s_io_pause );
   b_io_pause->setFont( menu_font );
   b_io_pause->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Keybind: Pause", b_io_pause);

   b_io_show_keybinds = new IMEdgeTextButton();
   b_io_show_keybinds->setAllTextures( texture_manager->getTexture( "UICenterBrown.png" ) );
   b_io_show_keybinds->setCornerAllTextures( texture_manager->getTexture( "UICornerBrown3px.png" ) );
   b_io_show_keybinds->setEdgeAllTextures( texture_manager->getTexture( "UIEdgeBrown3px.png" ) );
   b_io_show_keybinds->setEdgeWidth( 3 );
   b_io_show_keybinds->setText( &s_io_show_keybinds );
   b_io_show_keybinds->setFont( menu_font );
   b_io_show_keybinds->setTextColor( sf::Color::Black );
   gui_manager->registerWidget( "Keybind: Show Keybinds", b_io_show_keybinds);

   b_io_camera_left = new IMButton();
   b_io_camera_left->setAllTextures( texture_manager->getTexture( "OrderTurnWest.png" ) );
   gui_manager->registerWidget( "Keybind: Camera Left", b_io_camera_left);

   b_io_camera_right = new IMButton();
   b_io_camera_right->setAllTextures( texture_manager->getTexture( "OrderTurnEast.png" ) );
   gui_manager->registerWidget( "Keybind: Camera Right", b_io_camera_right);

   b_io_camera_up = new IMButton();
   b_io_camera_up->setAllTextures( texture_manager->getTexture( "OrderTurnNorth.png" ) );
   gui_manager->registerWidget( "Keybind: Camera Up", b_io_camera_up);

   b_io_camera_down = new IMButton();
   b_io_camera_down->setAllTextures( texture_manager->getTexture( "OrderTurnSouth.png" ) );
   gui_manager->registerWidget( "Keybind: Camera Down", b_io_camera_down);

   b_io_zoom_in = new IMButton();
   b_io_zoom_in->setAllTextures( texture_manager->getTexture( "PlusSign.png" ) );
   gui_manager->registerWidget( "Keybind: Zoom In", b_io_zoom_in);

   b_io_zoom_out = new IMButton();
   b_io_zoom_out->setAllTextures( texture_manager->getTexture( "MinusSign.png" ) );
   gui_manager->registerWidget( "Keybind: Zoom Out", b_io_zoom_out);

   Image gray_image;
   gray_image.create( 1, 1, Color( 85, 85, 85, 115 ) );
   Texture *gray_overlay = new Texture();
   gray_overlay->create( 1, 1 );
   gray_overlay->update( gray_image );
   texture_manager->addTexture( "gray_overlay", gray_overlay );

   b_overlay = new IMButton();
   b_overlay->setAllTextures( gray_overlay );
   gui_manager->registerWidget( "Gray Overlay", b_overlay);

   initInputOptionsMenu = true;
   fitGui_InputOptions();

   return 0;
}

bool inputOptionsBindNextKey = false;

void inputOptionsMenu()
{
   if (!initInputOptionsMenu) {
      initInputOptionsMenuGui();
   }
   else
   {
      RenderWindow* r_wind = SFML_GlobalRenderWindow::get();
      r_wind->clear( Color( 165, 165, 165, 255 ) );

      if (b_io_bind_key->doWidget())
         inputOptionsBindNextKey = !inputOptionsBindNextKey;

      if (b_io_clear_bind->doWidget()) {
         inputOptionsBindNextKey = false;
         config::bindKey( input_selected_kb, Keyboard::Unknown );
      }

      if (inputOptionsBindNextKey)
         b_overlay->doWidget();

      if (b_exit_options_menu->doWidget() && inputOptionsBindNextKey == false)
         closeInputOptions();

      KeybindTarget kb = drawKeybindButtons();

      drawKeybind( KB_PAUSE, io_x_5, io_y_3, io_button_size, 14 );
      if (b_io_pause->doWidget() && inputOptionsBindNextKey == false)
         kb = KB_PAUSE;

      drawKeybind( KB_SHOW_KEYBINDINGS, io_x_5, io_y_3 + io_y_3_diff, io_button_size, 14 );
      if (b_io_show_keybinds->doWidget() && inputOptionsBindNextKey == false)
         kb = KB_SHOW_KEYBINDINGS;

      drawKeybind( KB_MOVE_CAMERA_LEFT, io_x_4, io_y_4 + io_y_4_diff, io_button_size, 14 );
      if (b_io_camera_left->doWidget() && inputOptionsBindNextKey == false)
         kb = KB_MOVE_CAMERA_LEFT;

      drawKeybind( KB_MOVE_CAMERA_UP, io_x_4 + io_x_4_diff, io_y_4 , io_button_size, 14 );
      if (b_io_camera_up->doWidget() && inputOptionsBindNextKey == false)
         kb = KB_MOVE_CAMERA_UP;

      drawKeybind( KB_MOVE_CAMERA_RIGHT, io_x_4 + (2 * io_x_4_diff), io_y_4 + io_y_4_diff, io_button_size, 14 );
      if (b_io_camera_right->doWidget() && inputOptionsBindNextKey == false)
         kb = KB_MOVE_CAMERA_RIGHT;

      drawKeybind( KB_MOVE_CAMERA_DOWN, io_x_4 + io_x_4_diff, io_y_4 + (2 * io_y_4_diff), io_button_size, 14 );
      if (b_io_camera_down->doWidget() && inputOptionsBindNextKey == false)
         kb = KB_MOVE_CAMERA_DOWN;

      drawKeybind( KB_ZOOM_IN_CAMERA, io_x_4 + (4 * io_x_4_diff), io_y_4, io_button_size, 14 );
      if (b_io_zoom_in->doWidget() && inputOptionsBindNextKey == false)
         kb = KB_ZOOM_IN_CAMERA;

      drawKeybind( KB_ZOOM_OUT_CAMERA, io_x_4 + (4 * io_x_4_diff), io_y_4 + (2 * io_y_4_diff), io_button_size, 14 );
      if (b_io_zoom_out->doWidget() && inputOptionsBindNextKey == false)
         kb = KB_ZOOM_OUT_CAMERA;

      if (kb != KB_NOTHING && inputOptionsBindNextKey == false)
         input_selected_kb = kb;

      Text txt;
      txt.setFont( *menu_font );
      txt.setColor( Color::Black );
      txt.setCharacterSize( optionsMenuFontSize );

      txt.setString( String( "Bound Action:" ) ); 
      FloatRect fr = txt.getGlobalBounds();
      txt.setPosition( io_x_1 - io_spacer - fr.width, io_y_1 );
      r_wind->draw( txt );

      txt.setString( String( "Bound Key:" ) ); 
      fr = txt.getGlobalBounds();
      txt.setPosition( io_x_1 - io_spacer - fr.width, io_y_2 );
      r_wind->draw( txt );

      RectangleShape rect;
      rect.setSize( Vector2f( (io_x_2 - io_x_1) - io_spacer, io_button_size - io_spacer ) );
      rect.setFillColor( Color( 215, 215, 215, 255 ) );
      rect.setOutlineColor( Color::Black );
      rect.setOutlineThickness( 2 );

      rect.setPosition( io_x_1, io_y_1 );
      r_wind->draw( rect );
      txt.setString( keybindTargetToString( input_selected_kb ));
      txt.setPosition( io_x_1 + 4, io_y_1 + 4 );
      r_wind->draw( txt );

      rect.setPosition( io_x_1, io_y_2 );
      r_wind->draw( rect );
      txt.setString( keyToString( config::getBoundKey( input_selected_kb )));
      txt.setPosition( io_x_1 + 4, io_y_2 + 4 );
      r_wind->draw( txt );

      // Text headers
      txt.setCharacterSize( optionsMenuFontSize * 2 );

      txt.setString( String( "Keybindings" ) );
      fr = txt.getGlobalBounds();
      txt.setPosition( (config::width() - fr.width) / 2, io_spacer * 2 );
      r_wind->draw( txt );
      
      txt.setString( String( "Camera Controls" ) );
      txt.setPosition( io_x_4, io_y_3 - (io_spacer * 2) );
      r_wind->draw( txt );
   }

}

// Init --

int progressiveInitMenus()
{
   static int count = 0;
   log("Progressive init - Menus");

   if (count == 0) { 
      menu_font = new Font();
      if (menu_font->loadFromFile("./res/LiberationSans-Regular.ttf"))
         log("Successfully loaded menu font");
      ingame_font = new Font();
      if (ingame_font->loadFromFile("./res/FreeSansBold.ttf"))
         log("Successfully loaded ingame font");
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
      initInputOptionsMenuGui();

      count = 5;
   } else if (count == 5) {
      return 0; // done
   }

   return -1; // repeat
}

///////////////////////////////////////////////////////////////////////////////
// Loading ---

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
         if (initUnits() == 0)
            progress = 3;
         break;
      case 3:
         if (initEffects() == 0)
            progress = 4;
         break;
      case 4:
         if (initMap() == 0)
            progress = 5;
         break;
      case 5:
         if (initLevelEditorGui() == 0)
            progress = 6;
         break;
      case 6:
         if (initFocusMenuGui() == 0)
            progress = 7;
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
   sf::Sprite sp_hgg( *texture_manager->getTexture( "HGGLoadingScreenLogo128.png" ) );
   int x = (config::width() - 128) / 2;
   int y = (config::height() - 128) / 2;
   sp_hgg.setPosition( x, y );

   r_window->clear(Color::Black);
   r_window->draw( sp_hgg );
   r_window->display();
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Propogation ---

void refitGuis()
{
   fitGui_Splash(); 

   fitGui_Options();
   fitGui_AVOptions();
   fitGui_InputOptions();

   fitGui_Map();
   fitGui_FocusMenu();

   fitGui_Level();
   fitGui_LevelEditor();
}

///////////////////////////////////////////////////////////////////////////////
// Main Listeners ---

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
   if (inputOptionsBindNextKey == true) {
      config::bindKey( input_selected_kb, key_press.code );
      inputOptionsBindNextKey = false;
      return true;
   }

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
// Main loop ---

int mainLoop( int dt )
{
   event_manager->handleEvents();

   r_window->clear(Color::Yellow);

   gui_manager->begin();

   if (menu_state & MENU_SEC_AV_OPTIONS) {
      AVOptionsMenu();
   } else if (menu_state & MENU_SEC_INPUT_OPTIONS) {
      inputOptionsMenu();
   } else if (menu_state & MENU_SEC_OPTIONS) {
      optionsMenu();
   } else if (menu_state & MENU_PRI_SPLASH) {
      splashMenu();

   } else if (menu_state & MENU_MAP_FOCUS) {
      focusMenu();
   } else if (menu_state & MENU_PRI_MAP) {
      int map_result = drawMap(dt);
      if (map_result == -2) {
         // Various map options etc
      }

   } else if (menu_state & MENU_PRI_INGAME) {
      updateLevel(dt);
      int result = drawLevel();
      if (result == -3) openMap();

   } else if (menu_state & MENU_PRI_LEVEL_EDITOR) {
      drawLevel();

   } else if (menu_state & MENU_PRI_ANIMATION) {

   } 
   
   gui_manager->end();

   resetView();

   cursor_manager->drawCursor();

   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Execution starts here ---

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

   texture_manager->addSearchDirectory( "./res/" ); 

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
