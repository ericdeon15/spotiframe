#ifndef APP_MODE_HPP
#define APP_MODE_HPP

#include <stdint.h>

enum class AppMode {
  StartupMenu,
  Spotify,
  Donut,
};

void beginAppModeMenu();
void updateAppModeMenu();
void drawAppModeBackButton(uint32_t background = 0x000000);
bool appModeBackPressed();
AppMode getAppMode();

#endif
