#include "gui.h"
#include "types.h"
#include "units.h"
#include "orders.h"
#include "util.h"
#include "log.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include "IMGuiManager.hpp"
#include "IMButton.hpp"

#include <SFML/Graphics.hpp>

using namespace std;

namespace sum
{
   RenderWindow *gui_window;

   Unit *selected_unit;

   int initLevelGui()
   {
      log( "Initialized level Gui" );
      gui_window = SFML_GlobalRenderWindow::get();
      return 0;
   }

   int drawOrderQueue()
   {
      return 0;
   }

   int drawSelectedUnit()
   {
      if (selected_unit != NULL) {
         if (selected_unit->alive == false) {
            selected_unit = NULL;
            return -1;
         }

         Texture *t = selected_unit->getTexture();
         if (t) { 
            float window_edge = gui_window->getSize().x;
            RectangleShape select_rect, health_bound_rect, health_rect;

            select_rect.setSize( Vector2f( 120, 50 ) );
            select_rect.setPosition( window_edge - 120, 0 );
            select_rect.setFillColor( Color::White );
            select_rect.setOutlineColor( Color::Black );
            select_rect.setOutlineThickness( 2.0 );
            gui_window->draw( select_rect );

            health_bound_rect.setSize( Vector2f( 64, 14 ) );
            health_bound_rect.setPosition( window_edge - 67, 8 );
            health_bound_rect.setFillColor( Color::White );
            health_bound_rect.setOutlineColor( Color::Black );
            health_bound_rect.setOutlineThickness( 2.0 );
            gui_window->draw( health_bound_rect );

            health_rect.setSize( Vector2f( 60 * (selected_unit->health / selected_unit->max_health), 10 ) );
            health_rect.setPosition( window_edge - 65, 10 );
            health_rect.setFillColor( Color::Red );
            gui_window->draw( health_rect );

            Sprite unit_image( *t);
            float x = t->getSize().x;
            float y = t->getSize().y;
            float scale_x = 40 / x;
            float scale_y = 40 / y;
            unit_image.setScale( scale_x, scale_y );
            unit_image.setPosition( window_edge - 115, 5 );

            gui_window->draw( unit_image );

         }
      }

      return 0;
   }

   int drawOrderButtons()
   {
      return 0;
   }

   int drawGui()
   {
      gui_window->setView(gui_window->getDefaultView());

      drawOrderQueue();
      drawSelectedUnit();
      drawOrderButtons();

      return 0;
   }
};
