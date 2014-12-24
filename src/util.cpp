#include "util.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

void normalizeTo1x1( sf::Sprite *s )
{
   if (s) {
      float x = s->getTexture()->getSize().x;
      float y = s->getTexture()->getSize().y;
      float scale_x = 1.0 / x;
      float scale_y = 1.0 / y;
      s->setScale( scale_x, scale_y );
   }
}

sum::Direction getDirection( int x, int y, int to_x, int to_y )
{
   int dx = to_x - x,
       dy = to_y - y;

   if (abs(dx) > abs(dy)) {
      if (dx > 0)
         return sum::EAST;
      else
         return sum::WEST;
   } else {
      if (dy > 0)
         return sum::SOUTH;
      else
         return sum::NORTH;
   }
   return sum::ALL_DIR;
}
