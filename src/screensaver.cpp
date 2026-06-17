#include "screensaver.hpp"

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include "LGFX.hpp"
#include "donut.hpp"

extern LGFX tft;
extern LGFX_Sprite sprite;
extern DonutScreensaver donut;

void createScreensaverSprite() {
  sprite.setPsram(false);
  sprite.createSprite(300, 300);
  sprite.fillScreen(TFT_BLACK);
  donut.begin(tft, sprite);
}

void deleteScreensaverSprite() {
  sprite.deleteSprite();
}

void runScreensaverFrame(uint32_t frameDelayMs) {
  donut.update();
  donut.draw();
  delay(frameDelayMs);
}

void runScreensaverUntil(unsigned long startMs, uint32_t durationMs, uint32_t frameDelayMs) {
  while (millis() - startMs < durationMs) {
    runScreensaverFrame(frameDelayMs);
  }
}
