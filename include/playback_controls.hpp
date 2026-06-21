#ifndef PLAYBACK_CONTROLS_HPP
#define PLAYBACK_CONTROLS_HPP

#include <Arduino.h>

void beginPlaybackControls();
void drawPlaybackControl(bool isPlaying, uint32_t background);
bool playbackControlPressed();

#endif
