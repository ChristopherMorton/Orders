#include <fstream>
#include <sstream>

using namespace std;

int main(int argc, char* argv[])
{
   ofstream level_file( "res/testlevel.txt", ios::out | ios::binary | ios::ate );
   char fileContents[248];
   if(level_file.is_open())
   {
      int i;
      fileContents[0] = (char)15;
      fileContents[1] = (char)15;

      for (i = 2; i <= 226; ++i)
         fileContents[i] = ' ';

      fileContents[116] = 't';
      fileContents[117] = 't';
      fileContents[118] = 't';
      fileContents[131] = 't';
      fileContents[146] = 't';

      fileContents[227] = (char)5;
      // Player
      fileContents[228] = 'p';
      fileContents[229] = (char)0;
      fileContents[230] = (char)1;
      fileContents[231] = (char)4;
      // TargetPractices
      int y = 1;
      for (i = 232; i < 248; i += 4) {
         fileContents[i] = 't';
         fileContents[i+1] = (char)1;
         fileContents[i+2] = (char)9;
         fileContents[i+3] = (char)y;
         y += 2;
      }

      level_file.write(fileContents, 248);
   }

   return 0;
}
