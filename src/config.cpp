#include "log.h"
#define INCONFIGCPP 1
#include "config.h"

#include <fstream>
#include <sstream>
#include <string>

using namespace std;

namespace config
{

///////////////////////////////////////////////////////////////////////////////
// Data

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

void setDefaults()
{
   setWindow( 800, 600, 0 );
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
