#include <fstream>
#include <sstream>

using namespace std;

int main(int argc, char* argv[])
{
   ofstream level_file( "res/testlevel.txt", ios::out | ios::binary | ios::ate );
   char fileContents[251];
   if(level_file.is_open())
   {
      int i;
      // Dimensions
      fileContents[0] = (char)15;
      fileContents[1] = (char)15;

      // View
      fileContents[2] = (char)12;
      fileContents[3] = (char)6;
      fileContents[4] = (char)5;

      for (i = 5; i <= 229; ++i)
         fileContents[i] = ' ';

      fileContents[116] = 't';
      fileContents[117] = 't';
      fileContents[118] = 't';
      fileContents[131] = 't';
      fileContents[146] = 't';

      fileContents[230] = (char)5;
      // Player
      fileContents[231] = 'p';
      fileContents[232] = (char)0;
      fileContents[233] = (char)1;
      fileContents[234] = (char)4;
      // TargetPractices
      int y = 1;
      for (i = 235; i < 251; i += 4) {
         fileContents[i] = 't';
         fileContents[i+1] = (char)1;
         fileContents[i+2] = (char)9;
         fileContents[i+3] = (char)y;
         y += 2;
      }

      level_file.write(fileContents, 251);
   }

   return 0;
}
