#include "map.h"
#include "util.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace sum
{

int updateMap( int dt )
{

   return 0;
}

sf::Sprite *s_map;
sf::View *map_view;

int initMap()
{
   s_map = new Sprite( *SFML_TextureManager::getSingleton().getTexture( "MapScratch.png" ));
   normalizeTo1x1( s_map );

   map_view = new View();
   map_view->setSize( 1, 1 );
   map_view->setCenter( 0.5, 0.5 );

   return 0;
}

int drawMap()
{
   RenderWindow *r_window = SFML_GlobalRenderWindow::get();
   r_window->setView(*map_view);
   r_window->draw( *s_map );

   return 0;
}

};
