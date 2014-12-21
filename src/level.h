#ifndef LEVEL_H__
#define LEVEL_H__

#include "types.h"
#include "units.h"
#include "effects.h"

namespace sum
{
   void setLevelListener( bool set = true );
   void setLevelEditorListener( bool set = true );

   int loadLevel( int level_id );
   int updateLevel( int dt );
   int drawLevel();

   int loadLevelEditor( int level );

#define SELECT_CLOSEST 1
#define SELECT_FARTHEST 2
#define SELECT_BIGGEST 3
#define SELECT_SMALLEST 4
   class Unit;
   Unit* getEnemy( int x, int y, float range, Direction dir, Unit *source, int selector);

   bool isVisible( int x, int y );

   bool canMove( int x, int y, int from_x, int from_y );
   bool canMoveUnit( int x, int y, int from_x, int from_y, Unit* u );

   int moveUnit( Unit *u, int new_x, int new_y );
   int addProjectile( Effect_Type t, int team, float x, float y, float speed, float range, Unit* target );
   int addEffect( Effect_Type t, float dur, float x, float y );

   int broadcastOrder( Order o );
   int startPlayerCommand( Order o );
   int completePlayerCommand( Order o );

   void fitGui_Level();

   // Gui related
   extern Unit *selected_unit;

   int initLevelGui();
};

#endif
