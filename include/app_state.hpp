#ifndef APP_STATE_HPP
#define APP_STATE_HPP

#include <Arduino.h>

struct AppState {
  String currentID = "";
  uint32_t prevColor = 0x000000;
  bool hasPrevColor = false;
  unsigned long donutStart = 0;
  unsigned long lastSongChangeAt = 0;
  bool screensaverActive = false;
  bool playbackScreenActive = false;
  bool isPlaying = false;
  uint32_t backgroundColor = 0x000000;
};

extern AppState appState;

#endif
