#include "units.h"
#include "focus.h"
#include "util.h"
#include "log.h"

#include "SFML_GlobalRenderWindow.hpp"
#include "SFML_TextureManager.hpp"

#include <SFML/Graphics.hpp>

//////////////////////////////////////////////////////////////////////
// Definitions
//////////////////////////////////////////////////////////////////////

#define MAGICIAN_BASE_HEALTH 100

using namespace sf;
using namespace std;

namespace sum
{

//////////////////////////////////////////////////////////////////////
// Base Unit
//////////////////////////////////////////////////////////////////////

int Unit::TurnTo( Direction face )
{
   facing = face;

   if (face == NORTH) {
      x_next = x_grid;
      y_next = y_grid - 1;
   } else if (face == SOUTH) {
      x_next = x_grid;
      y_next = y_grid + 1;
   } else if (face == EAST) {
      x_next = x_grid + 1;
      y_next = y_grid;
   } else if (face == WEST) {
      x_next = x_grid - 1;
      y_next = y_grid;
   } 

   return 0;
}

Unit::~Unit()
{

}

//////////////////////////////////////////////////////////////////////
// Magician
//////////////////////////////////////////////////////////////////////

// *tors
Magician::Magician()
{
   log("Creating empty Magician");
   x_grid = -1;
   y_grid = -1;
}

Magician::Magician( int x, int y, Direction face )
{
   log("Creating Magician");
   x_grid = x_real = x;
   y_grid = y_real = y;
   TurnTo(face);

   health = max_health = MAGICIAN_BASE_HEALTH * ( 1.0 + ( focus_toughness * 0.02 ) );

   team = 0;
}

Magician::~Magician()
{

}

// Virtual methods

int Magician::addOrder( Order o )
{

   return 0;
}

int Magician::completeTurn()
{

   return 0;
}

int Magician::update()
{

   return 0;
}

int Magician::draw()
{
   Sprite *mag = new Sprite( *(SFML_TextureManager::getSingleton().getTexture( "Magician1.png" )));

   normalizeTo1x1( mag );
   mag->setPosition( x_real, y_real );
   SFML_GlobalRenderWindow::get()->draw( *mag );

   return 0;
}

};
