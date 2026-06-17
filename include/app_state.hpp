#ifndef APP_STATE_HPP
#define APP_STATE_HPP

#include <Arduino.h>

struct AppState {
  String currentID = "";
  uint32_t prevColor = 0x000000;
  bool hasPrevColor = false;
  unsigned long donutStart = 0;
};

extern AppState appState;

#endif
