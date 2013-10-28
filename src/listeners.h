#ifndef LISTENERS_H__
#define LISTENERS_H__

#include "SFML_Listeners.hpp"

namespace sum
{
   struct MainWindowListener : public My_SFML_WindowListener
   {
      virtual bool windowClosed( );
      virtual bool windowResized( const sf::Event::SizeEvent &resized );
      virtual bool windowLostFocus( );
      virtual bool windowGainedFocus( );
   };

   struct MainMouseListener : public My_SFML_MouseListener
   {
      virtual bool mouseMoved( const sf::Event::MouseMoveEvent &mouse_move );
      virtual bool mouseButtonPressed( const sf::Event::MouseButtonEvent &mouse_button_press );
      virtual bool mouseButtonReleased( const sf::Event::MouseButtonEvent &mouse_button_release );
      virtual bool mouseWheelMoved( const sf::Event::MouseWheelEvent &mouse_wheel_move );
   };

   struct MainKeyListener : public My_SFML_KeyListener
   {
      virtual bool keyPressed( const sf::Event::KeyEvent &key_press );
      virtual bool keyReleased( const sf::Event::KeyEvent &key_release );
   };
}

#endif

