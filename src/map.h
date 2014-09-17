#ifndef MAP_H__
#define MAP_H__

#include "menustate.h"

namespace sum
{
   int initMap();
   int drawMap( int dt );

   void setMapListener( bool set );
};

#endif
