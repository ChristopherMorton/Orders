#include "savestate.h"
#include <cstddef>

namespace sum
{

LevelRecord *level_scores = NULL;

LevelRecord getRecord( int level )
{

}

void initRecords()
{
   if (level_scores == NULL)
      level_scores = new LevelRecord[NUM_LEVELS];
}

int loadSaveGame()
{
   initRecords();

   //string type, value;
   //ifstream save_in;
   //save_in.open("res/.save");

   return 0;
}

int saveSaveGame()
{
   
   return 0;
}

};
