#include "util.h"
#include "level.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include "stdlib.h"

using namespace sum;
using namespace sf;

void normalizeTo1x1( Sprite *s )
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

int addDirectionF( Direction d, float &x, float &y, float add )
{
   if (d == NORTH)
      y -= add;
   else if (d == SOUTH)
      y += add;
   else if (d == EAST)
      x += add;
   else if (d == WEST)
      x -= add;

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

// ToString methods --

String keybindTargetToString( sum::KeybindTarget target )
{
   switch (target) {
      case KB_NOTHING:
         return String( "N/A" );
      case KB_FORCE_QUIT:
         return String( "Force Quit" );
      case KB_PAUSE:
         return String( "Pause" );
      case KB_MOVE_CAMERA_LEFT:
         return String( "Camera: Move Left" );
      case KB_MOVE_CAMERA_UP:
         return String( "Camera: Move Up" );
      case KB_MOVE_CAMERA_RIGHT:
         return String( "Camera: Move Right" );
      case KB_MOVE_CAMERA_DOWN:
         return String( "Camera: Move Down" );
      case KB_ZOOM_OUT_CAMERA:
         return String( "Camera: Zoom Out" );
      case KB_ZOOM_IN_CAMERA:
         return String( "Camera: Zoom In" );
      case KB_TOGGLE_OPTIONS_MENU:
         return String( "Options Menu" );
      case KB_SHOW_KEYBINDINGS:
         return String( "Show Keybindings" );
      case KB_DEBUG_TOGGLE_FOG:
         return String( "DEBUG: Toggle Fog" );
      case KB_DEBUG_TOGGLE_FRAMERATE:
         return String( "DEBUG: Toggle Framerate" );
      case KB_BTN_COUNT_0:
         return String( "Count: 0" );
      case KB_BTN_COUNT_1:
         return String( "Count: 1" );
      case KB_BTN_COUNT_2:
         return String( "Count: 2" );
      case KB_BTN_COUNT_3:
         return String( "Count: 3" );
      case KB_BTN_COUNT_4:
         return String( "Count: 4" );
      case KB_BTN_COUNT_5:
         return String( "Count: 5" );
      case KB_BTN_COUNT_6:
         return String( "Count: 6" );
      case KB_BTN_COUNT_7:
         return String( "Count: 7" );
      case KB_BTN_COUNT_8:
         return String( "Count: 8" );
      case KB_BTN_COUNT_9:
         return String( "Count: 9" );
      case KB_BTN_COUNT_CLEAR:
         return String( "Clear Count" );
      case KB_BTN_COUNT_INFINITE:
         return String( "Count: Indefinitely" );
      case KB_BTN_COND_ENEMY_ADJACENT:
         return String( "Condition: Enemy Adjacent" );
      case KB_BTN_COND_ENEMY_AHEAD:
         return String( "Condition: Enemy Ahead" );
      case KB_BTN_COND_ENEMY_IN_RANGE:
         return String( "Condition: Enemy In Range" );
      case KB_BTN_COND_HEALTH_UNDER_50:
         return String( "Condition: Health < 50%" );
      case KB_BTN_COND_HEALTH_UNDER_20:
         return String( "Condition: Health < 20%" );
      case KB_BTN_COND_BLOCKED_AHEAD:
         return String( "Condition: Path Blocked" );
      case KB_BTN_COND_CLEAR:
         return String( "Clear Condition" );
      case KB_BTN_MOVE_FORWARD:
         return String( "Order: Move Forward" );
      case KB_BTN_MOVE_BACK:
         return String( "Order: Retreat" );
      case KB_BTN_TURN_NORTH:
         return String( "Order: Turn North" );
      case KB_BTN_TURN_EAST:
         return String( "Order: Turn East" );
      case KB_BTN_TURN_SOUTH:
         return String( "Order: Turn South" );
      case KB_BTN_TURN_WEST:
         return String( "Order: Turn West" );
      case KB_BTN_TURN_NEAREST_ENEMY:
         return String( "Order: Turn to Nearest Enemy" );
      case KB_BTN_FOLLOW_PATH:
         return String( "Order: Follow Path" );
      case KB_BTN_WAIT:
         return String( "Order: Wait" );
      case KB_BTN_ATTACK_CLOSEST:
         return String( "Order: Attack Nearest" );
      case KB_BTN_ATTACK_FARTHEST:
         return String( "Order: Attack Farthest" );
      case KB_BTN_ATTACK_BIGGEST:
         return String( "Order: Attack Strongest" );
      case KB_BTN_ATTACK_SMALLEST:
         return String( "Order: Attack Weakest" );
      case KB_BTN_ATTACK_MOST_ARMORED:
         return String( "Order: Attack Most Armored" );
      case KB_BTN_ATTACK_LEAST_ARMORED:
         return String( "Order: Attack Least Armored" );
      case KB_BTN_START_BLOCK:
         return String( "Control: Block Begin" );
      case KB_BTN_END_BLOCK:
         return String( "Control: Block End" );
      case KB_BTN_REPEAT:
         return String( "Control: Repeat" );
      case KB_BTN_MONSTER_GUARD:
         return String( "Order: Guard" );
      case KB_BTN_MONSTER_BURST:
         return String( "Order: Burst" );
      case KB_BTN_SOLDIER_SWITCH_AXE:
         return String( "Order: Switch to Axe" );
      case KB_BTN_SOLDIER_SWITCH_SPEAR:
         return String( "Order: Switch to Spear" );
      case KB_BTN_SOLDIER_SWITCH_BOW:
         return String( "Order: Switch to Bow" );
      case KB_BTN_WORM_HIDE:
         return String( "Order: Hide" );
      case KB_BTN_WORM_TRAIL_START:
         return String( "Order: Trail On" );
      case KB_BTN_WORM_TRAIL_END:
         return String( "Order: Trail Off" );
      case KB_BTN_BIRD_CMD_SHOUT:
         return String( "Control: Shout Block Begin" );
      case KB_BTN_BIRD_CMD_QUIET:
         return String( "Control: Shout Block End" );
      case KB_BTN_BIRD_FLY:
         return String( "Order: Fly" );
      case KB_BTN_BUG_CAST_FIREBALL:
         return String( "Order: Cast Fireball" );
      case KB_BTN_BUG_CAST_SUNDER:
         return String( "Order: Cast Sunder" );
      case KB_BTN_BUG_CAST_HEAL:
         return String( "Order: Cast Heal" );
      case KB_BTN_BUG_OPEN_WORMHOLE:
         return String( "Order: Open Wormhole" );
      case KB_BTN_BUG_CLOSE_WORMHOLE:
         return String( "Order: Close Wormhole" );
      case KB_BTN_BUG_MEDITATE:
         return String( "Order: Meditate" );
      case KB_BTN_PL_ALERT_ALL:
         return String( "Player: Alert All" );
      case KB_BTN_PL_ALERT_MONSTERS:
         return String( "Player: Alert Monsters" );
      case KB_BTN_PL_ALERT_SOLDIERS:
         return String( "Player: Alert Soldiers" );
      case KB_BTN_PL_ALERT_WORMS:
         return String( "Player: Alert Worms" );
      case KB_BTN_PL_ALERT_BIRDS:
         return String( "Player: Alert Birds" );
      case KB_BTN_PL_ALERT_BUGS:
         return String( "Player: Alert Bugs" );
      case KB_BTN_PL_GO:
         return String( "Player: Go" );
      case KB_BTN_PL_GO_ALL:
         return String( "Player: Go All" );
      case KB_BTN_PL_GO_MONSTERS:
         return String( "Player: Go Monsters" );
      case KB_BTN_PL_GO_SOLDIERS:
         return String( "Player: Go Soldiers" );
      case KB_BTN_PL_GO_WORMS:
         return String( "Player: Go Worms" );
      case KB_BTN_PL_GO_BIRDS:
         return String( "Player: Go Birds" );
      case KB_BTN_PL_GO_BUGS:
         return String( "Player: Go Bugs" );
      case KB_BTN_PL_SET_GROUP:
         return String( "Player: Set Group" );
      case KB_BTN_PL_DELAY:
         return String( "Player: Delay" );
      default:
         return String( "Keybind has no string representation" );
   }
}

String keyToString( Keyboard::Key key )
{
   switch (key) {
      case Keyboard::A:
         return String( "A" );
      case Keyboard::B:
         return String( "B" );
      case Keyboard::C:
         return String( "C" );
      case Keyboard::D:
         return String( "D" );
      case Keyboard::E:
         return String( "E" );
      case Keyboard::F:
         return String( "F" );
      case Keyboard::G:
         return String( "G" );
      case Keyboard::H:
         return String( "H" );
      case Keyboard::I:
         return String( "I" );
      case Keyboard::J:
         return String( "J" );
      case Keyboard::K:
         return String( "K" );
      case Keyboard::L:
         return String( "L" );
      case Keyboard::M:
         return String( "M" );
      case Keyboard::N:
         return String( "N" );
      case Keyboard::O:
         return String( "O" );
      case Keyboard::P:
         return String( "P" );
      case Keyboard::Q:
         return String( "Q" );
      case Keyboard::R:
         return String( "R" );
      case Keyboard::S:
         return String( "S" );
      case Keyboard::T:
         return String( "T" );
      case Keyboard::U:
         return String( "U" );
      case Keyboard::V:
         return String( "V" );
      case Keyboard::W:
         return String( "W" );
      case Keyboard::X:
         return String( "X" );
      case Keyboard::Y:
         return String( "Y" );
      case Keyboard::Z:
         return String( "Z" );
      case Keyboard::Num0:
         return String( "0" );
      case Keyboard::Num1:
         return String( "1" );
      case Keyboard::Num2:
         return String( "2" );
      case Keyboard::Num3:
         return String( "3" );
      case Keyboard::Num4:
         return String( "4" );
      case Keyboard::Num5:
         return String( "5" );
      case Keyboard::Num6:
         return String( "6" );
      case Keyboard::Num7:
         return String( "7" );
      case Keyboard::Num8:
         return String( "8" );
      case Keyboard::Num9:
         return String( "9" );
      case Keyboard::Escape:
         return String( "Esc" );
      case Keyboard::LControl:
         return String( "LCtrl" );
      case Keyboard::LShift:
         return String( "LShift" );
      case Keyboard::LAlt:
         return String( "LAlt" );
      case Keyboard::LSystem:
         return String( "LSys" );
      case Keyboard::RControl:
         return String( "RCtrl" );
      case Keyboard::RShift:
         return String( "RShift" );
      case Keyboard::RAlt:
         return String( "RAlt" );
      case Keyboard::RSystem:
         return String( "RSys" );
      case Keyboard::Menu:
         return String( "Menu" );
      case Keyboard::LBracket:
         return String( "[" );
      case Keyboard::RBracket:
         return String( "]" );
      case Keyboard::SemiColon:
         return String( ";" );
      case Keyboard::Comma:
         return String( "," );
      case Keyboard::Period:
         return String( "." );
      case Keyboard::Quote:
         return String( "'" );
      case Keyboard::Slash:
         return String( "/" );
      case Keyboard::BackSlash:
         return String( "\\" );
      case Keyboard::Tilde:
         return String( "~" );
      case Keyboard::Equal:
         return String( "=" );
      case Keyboard::Dash:
         return String( "-" );
      case Keyboard::Space:
         return String( "Space" );
      case Keyboard::Return:
         return String( "Ret" );
      case Keyboard::BackSpace:
         return String( "BSp" );
      case Keyboard::Tab:
         return String( "Tab" );
      case Keyboard::PageUp:
         return String( "PUp" );
      case Keyboard::PageDown:
         return String( "PDown" );
      case Keyboard::End:
         return String( "End" );
      case Keyboard::Home:
         return String( "Home" );
      case Keyboard::Insert:
         return String( "Ins" );
      case Keyboard::Delete:
         return String( "Del" );
      case Keyboard::Add:
         return String( "Num+" );
      case Keyboard::Subtract:
         return String( "Num-" );
      case Keyboard::Multiply:
         return String( "Num*" );
      case Keyboard::Divide:
         return String( "Num/" );
      case Keyboard::Left:
         return String( "Left" );
      case Keyboard::Right:
         return String( "Right" );
      case Keyboard::Up:
         return String( "Up" );
      case Keyboard::Down:
         return String( "Down" );
      case Keyboard::Numpad0:
         return String( "Num0" );
      case Keyboard::Numpad1:
         return String( "Num1" );
      case Keyboard::Numpad2:
         return String( "Num2" );
      case Keyboard::Numpad3:
         return String( "Num3" );
      case Keyboard::Numpad4:
         return String( "Num4" );
      case Keyboard::Numpad5:
         return String( "Num5" );
      case Keyboard::Numpad6:
         return String( "Num6" );
      case Keyboard::Numpad7:
         return String( "Num7" );
      case Keyboard::Numpad8:
         return String( "Num8" );
      case Keyboard::Numpad9:
         return String( "Num9" );
      case Keyboard::F1:
         return String( "F1" );
      case Keyboard::F2:
         return String( "F2" );
      case Keyboard::F3:
         return String( "F3" );
      case Keyboard::F4:
         return String( "F4" );
      case Keyboard::F5:
         return String( "F5" );
      case Keyboard::F6:
         return String( "F6" );
      case Keyboard::F7:
         return String( "F7" );
      case Keyboard::F8:
         return String( "F8" );
      case Keyboard::F9:
         return String( "F9" );
      case Keyboard::F10:
         return String( "F10" );
      case Keyboard::F11:
         return String( "F11" );
      case Keyboard::F12:
         return String( "F12" );
      case Keyboard::F13:
         return String( "F13" );
      case Keyboard::F14:
         return String( "F14" );
      case Keyboard::F15:
         return String( "F15" );
      case Keyboard::Pause:
         return String( "Pause" );
      default:
         return String( "N/A" );
   }
}
