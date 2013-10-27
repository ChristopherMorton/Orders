#ifndef CONFIG_H__
#define CONFIG_H__

#define W_FULLSCREEN_FLAG = 0x1
#define W_SHOW_MENU_FLAG = 0x2

namespace sum
{
   struct Config
   {
      int w_height;
      int w_width;
      int w_flags;
   };

   extern Config config;
};

#endif
