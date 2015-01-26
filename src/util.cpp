#include "util.h"
#include "level.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "stdlib.h"

using namespace sum;

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

Direction getDirection( int x, int y, int to_x, int to_y )
{
   int dx = to_x - x,
       dy = to_y - y;

   if (abs(dx) > abs(dy)) {
      if (dx > 0)
         return EAST;
      else
         return WEST;
   } else {
      if (dy > 0)
         return SOUTH;
      else
         return NORTH;
   }
   return ALL_DIR;
}

int addDirection( Direction d, int &x, int &y )
{
   if (d == NORTH)
      --y;
   else if (d == SOUTH)
      ++y;
   else if (d == EAST)
      ++x;
   else if (d == WEST)
      --x;

   if (x < 0 || y < 0 || x >= level_dim_x || y >= level_dim_y)
      return -1;

   return 0;
}

Direction reverseDirection( Direction d )
{
   if (d == NORTH)
      return SOUTH;
   if (d == SOUTH)
      return NORTH;
   if (d == EAST)
      return WEST;
   if (d == WEST)
      return EAST;
   
   return ALL_DIR;
}
