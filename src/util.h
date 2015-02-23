#ifndef __UTIL_H
#define __UTIL_H

#include "types.h"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/System/String.hpp>

namespace sf { class Sprite; }

void normalizeTo1x1( sf::Sprite *s );

sum::Direction getDirection( int x, int y, int to_x, int to_y );
int addDirection( sum::Direction d, int &x, int &y );
int addDirectionF( sum::Direction d, float &x, float &y, float add = 1.0 );
sum::Direction reverseDirection( sum::Direction d );

sf::String keybindTargetToString( sum::KeybindTarget target );
sf::String keyToString( sf::Keyboard::Key key );

#endif
