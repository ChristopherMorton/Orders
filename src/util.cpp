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
