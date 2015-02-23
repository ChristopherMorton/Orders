#include "log.h"
#define INCONFIGCPP 1
#include "config.h"

#include <fstream>
#include <sstream>
#include <string>

using namespace std;
using namespace sf;
using namespace sum;

namespace config
{

///////////////////////////////////////////////////////////////////////////////
// Window

int w_width;
int w_height;
int w_flags;

int width()
{
   return w_width;
}

int height()
{
   return w_height;
}

int flags()
{
   return w_flags;
}

int validateWindow( int w, int h, int f )
{
   if (w == 0 || h == 0)
      return 0;
   else return 1;
}

int setWindow( int w, int h, int f )
{
   if (validateWindow( w, h, f ))
   {
      w_width = w;
      w_height = h;
      w_flags = f;
   }
   else
   {
      w_width = 800;
      w_height = 600;
      w_flags = 0;
   }
   return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Key Binding

KeybindTarget keyToTarget[(int)Keyboard::KeyCount];
Keyboard::Key targetToKey[(int)KB_COUNT];

void clearKeybinds()
{
   int i;
   for ( i = 0; i < (int)Keyboard::KeyCount; ++i )
      keyToTarget[i] = KB_NOTHING;
   for ( i = 0; i < (int)KB_COUNT; ++i )
      targetToKey[i] = Keyboard::Unknown;
}

void setDefaultKeybindings()
{
   clearKeybinds();
   bindKey( KB_MOVE_CAMERA_DOWN, Keyboard::Down );
   bindKey( KB_MOVE_CAMERA_UP, Keyboard::Up );
   bindKey( KB_MOVE_CAMERA_RIGHT, Keyboard::Right );
   bindKey( KB_MOVE_CAMERA_LEFT, Keyboard::Left );
   bindKey( KB_ZOOM_OUT_CAMERA, Keyboard::Subtract );
   bindKey( KB_ZOOM_IN_CAMERA, Keyboard::Add );

   bindKey( KB_BTN_COUNT_0, Keyboard::Num0 );
   bindKey( KB_BTN_COUNT_1, Keyboard::Num1 );
   bindKey( KB_BTN_COUNT_2, Keyboard::Num2 );
   bindKey( KB_BTN_COUNT_3, Keyboard::Num3 );
   bindKey( KB_BTN_COUNT_4, Keyboard::Num4 );
   bindKey( KB_BTN_COUNT_5, Keyboard::Num5 );
   bindKey( KB_BTN_COUNT_6, Keyboard::Num6 );
   bindKey( KB_BTN_COUNT_7, Keyboard::Num7 );
   bindKey( KB_BTN_COUNT_8, Keyboard::Num8 );
   bindKey( KB_BTN_COUNT_9, Keyboard::Num9 );
   bindKey( KB_BTN_COUNT_CLEAR, Keyboard::Dash );
   bindKey( KB_BTN_COUNT_INFINITE, Keyboard::Equal );

   bindKey( KB_PAUSE, Keyboard::Space );

   bindKey( KB_TOGGLE_OPTIONS_MENU, Keyboard::Escape );

   bindKey( KB_SHOW_KEYBINDINGS, Keyboard::K );

   bindKey( KB_DEBUG_TOGGLE_FOG, Keyboard::V );
   bindKey( KB_DEBUG_TOGGLE_FRAMERATE, Keyboard::F );

   bindKey( KB_FORCE_QUIT, Keyboard::Q );
}

sf::Keyboard::Key getBoundKey( KeybindTarget target )
{
   return targetToKey[(int)target];
}

KeybindTarget getBoundTarget( sf::Keyboard::Key key )
{
   return keyToTarget[(int)key];
}

Keyboard::Key bindKey( KeybindTarget target, sf::Keyboard::Key key )
{
   Keyboard::Key ret = getBoundKey( target );

   keyToTarget[(int)key] = target;
   targetToKey[(int)target] = key;

   keyToTarget[(int)ret] = KB_NOTHING;

   return ret;
}

bool show_keybindings = false;

void setDefaults()
{
   setWindow( 800, 600, 0 );
   setDefaultKeybindings();
}

int load()
{
   setDefaults();

   string type, value;
   ifstream config_in;
   config_in.open("res/config.txt"); 

   if (config_in.is_open()) {
      while (true) {
         // Get next line
         config_in >> type;
         if (config_in.eof())
            break;

         if (config_in.bad()) {
            log("Error in reading config - config_in is 'bad' - INVESTIGATE");
            break;
         }

         if (type == "WINDOW") {
            int w = 0, h = 0, f = 0;
            config_in >> w >> h >> f;
            setWindow( w, h, f );
         }
      }

      config_in.close();
   }

   return 0;
}

int save()
{
   ofstream config_out;
   config_out.open("res/config.txt"); 

   config_out << "WINDOW " << w_width << " " << w_height << " " << w_flags;

   config_out.close();

   return 0;
}

};
