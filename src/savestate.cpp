#include "savestate.h"
#include "log.h"

#include <fstream>
#include <sstream>
#include <string>

using namespace std;

namespace sum
{

LevelRecord *level_scores = NULL;

LevelRecord getRecord( int level )
{
   if (level >= 0 && level < NUM_LEVELS)
      return (level_scores[level]);

   return 0;
}

void setRecord( int level, LevelRecord record )
{
   if (level >= 0 && level < NUM_LEVELS)
      level_scores[level] = record;
}

void initRecords()
{
   if (level_scores == NULL)
      level_scores = new LevelRecord[NUM_LEVELS];

   for (int i = 0; i < NUM_LEVELS; ++i)
      level_scores[i] = 0;
}

int loadSaveGame()
{
   initRecords();

   int num, record;
   ifstream save_in;
   save_in.open("res/.save");

   if (save_in.is_open()) {
      while (true) {
         // Get next line
         save_in >> num >> record;
         if (save_in.eof())
            break;

         if (save_in.bad()) {
            log("Error in reading save file - save_in is 'bad' - INVESTIGATE");
            break;
         }

         setRecord( num, record );
      }

      save_in.close();
   }

   return 0;
}

int saveSaveGame()
{
   
   return 0;
}

};
