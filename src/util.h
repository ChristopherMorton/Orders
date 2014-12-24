#ifndef __UTIL_H
#define __UTIL_H

#include "types.h"

namespace sf { class Sprite; }

void normalizeTo1x1( sf::Sprite *s );
sum::Direction getDirection( int x, int y, int to_x, int to_y );

#endif
