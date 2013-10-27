#include "config.h"
#include "savestate.h"
#include <SFML/Window.hpp>
#include <SFML/System.hpp>

using namespace sf;

namespace sum
{
   // Global app-state variables

   // Config
   Config config;

   // Player data - i.e. save state
   String player_name;
   LevelRecord *level_scores;



   int runApp()
   {
    sf::Window window(sf::VideoMode(800, 600), "Summoner");

    // run the program as long as the window is open
    while (window.isOpen())
    {
        // check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();
        }
    }

    return 0;
   }
};

int main(int argc, char* argv[])
{
   return sum::runApp();
}
