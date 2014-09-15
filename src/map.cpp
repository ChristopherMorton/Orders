#include "map.h"
#include "util.h"
#include "shutdown.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include "IMButton.hpp"
#include "IMGuiManager.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace sum
{

sf::Sprite *s_map = NULL;
sf::View *map_view = NULL;

IMButton *b_start_test_level = NULL;

int initMap()
{
   SFML_TextureManager *texture_manager = &SFML_TextureManager::getSingleton();
   s_map = new Sprite( *texture_manager->getTexture( "MapScratch.png" ));
   normalizeTo1x1( s_map );

   map_view = new View();
   map_view->setSize( 1, 1 );
   map_view->setCenter( 0.5, 0.5 );

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

};
