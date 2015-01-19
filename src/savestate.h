#ifndef SAVESTATE_H__
#define SAVESTATE_H__

// LevelRecord
#define LR_EASY 0x1
#define LR_EASY_RT 0x2
#define LR_EASY_MM 0x4
#define LR_HARD 0x10
#define LR_HARD_RT 0x20
#define LR_HARD_MM 0x40

#define NUM_LEVELS 20

namespace sum
{
   typedef int LevelRecord;

   void setRecord( int level, LevelRecord record );
   void addRecord( int level, LevelRecord record );
   LevelRecord getRecord( int level );

   int loadSaveGame();
   int saveSaveGame();
};

#endif
