#ifndef LEVEL_H__
#define LEVEL_H__

namespace sum
{
   void setLevelListener( bool set = true );

   int loadLevel( int level_id );
   int updateLevel( int dt );
   int drawLevel();
};

#endif
