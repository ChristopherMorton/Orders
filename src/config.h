#ifndef CONFIG_H__
#define CONFIG_H__

#define W_FULLSCREEN_FLAG = 0x1
#define W_SHOW_MENU_FLAG = 0x2

namespace config
{
   int width();
   int height();
   int flags();

   int load();
   int save();

   int setWindow( int w, int h, int f );
};

#endif
