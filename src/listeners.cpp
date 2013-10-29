#include "listeners.h"
#include "shutdown.h"
#include "log.h"

#include "IMCursorManager.hpp"

namespace sum
{

// Window
bool MainWindowListener::windowClosed( )
{
   shutdown(1,1);
   return true;
}

bool MainWindowListener::windowResized( const sf::Event::SizeEvent &resized )
{

   return true;
}

bool MainWindowListener::windowLostFocus( )
{

   return true;
}

bool MainWindowListener::windowGainedFocus( )
{

   return true;
}

// Mouse
bool MainMouseListener::mouseMoved( const sf::Event::MouseMoveEvent &mouse_move )
{
   return true;
}

bool MainMouseListener::mouseButtonPressed( const sf::Event::MouseButtonEvent &mouse_button_press )
{
   log("Clicked");
   return true;
}

bool MainMouseListener::mouseButtonReleased( const sf::Event::MouseButtonEvent &mouse_button_release )
{
   log("Un-Clicked");
   return true;
}

bool MainMouseListener::mouseWheelMoved( const sf::Event::MouseWheelEvent &mouse_wheel_move )
{
   return true;
}

// Key
bool MainKeyListener::keyPressed( const sf::Event::KeyEvent &key_press )
{
   if (key_press.code == sf::Keyboard::Q)
      shutdown(1,1);

   return true;
}

bool MainKeyListener::keyReleased( const sf::Event::KeyEvent &key_release )
{
   return true;
}

};
