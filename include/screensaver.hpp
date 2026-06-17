#ifndef SCREENSAVER_HPP
#define SCREENSAVER_HPP

#include <Arduino.h>

void createScreensaverSprite();
void deleteScreensaverSprite();
void runScreensaverFrame(uint32_t frameDelayMs);
void runScreensaverUntil(unsigned long startMs, uint32_t durationMs, uint32_t frameDelayMs);

#endif
