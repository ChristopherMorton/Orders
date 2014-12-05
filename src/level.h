#ifndef LEVEL_H__
#define LEVEL_H__

#include "types.h"
#include "units.h"
#include "projectile.h"

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

   bool canMove( int x, int y, int from_x, int from_y );
   bool canMoveUnit( int x, int y, int from_x, int from_y, Unit* u );

   int moveUnit( Unit *u, int new_x, int new_y );
   int addProjectile( Projectile_Type t, int team, float x, float y, float speed, Unit* target );

   int broadcastOrder( Order o );
   int startPlayerCommand( Order o );
   int completePlayerCommand( Order o );

   void fitGui_Level();

   // Gui related
   extern Unit *selected_unit;

   int initLevelGui();
};

#endif
