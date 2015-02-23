#ifndef CONFIG_H__
#define CONFIG_H__

#define W_FULLSCREEN_FLAG = 0x1
#define W_SHOW_MENU_FLAG = 0x2

#include "types.h"
#include <SFML/Window/Keyboard.hpp>
#include <SFML/System/String.hpp>

namespace config
{
   int width();
   int height();
   int flags();

   int load();
   int save();

   int setWindow( int w, int h, int f );

   sf::Keyboard::Key getBoundKey( sum::KeybindTarget target );
   sum::KeybindTarget getBoundTarget( sf::Keyboard::Key key );

   // Returns old bound key
   sf::Keyboard::Key bindKey( sum::KeybindTarget target, sf::Keyboard::Key key );

   extern bool show_keybindings;
};

#endif
