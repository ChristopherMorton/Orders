#ifndef LEVEL_H__
#define LEVEL_H__

#include "types.h"

namespace sum
{
   void setLevelListener( bool set = true );

   int loadLevel( int level_id );
   int updateLevel( int dt );
   int drawLevel();

#define SELECT_CLOSEST 1
#define SELECT_FARTHEST 2
#define SELECT_BIGGEST 3
#define SELECT_SMALLEST 4
   class Unit;
   Unit* getEnemy( int x, int y, float range, Direction dir, int team, int selector);
};

#endif
