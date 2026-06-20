#include "screensaver.hpp"

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include "LGFX.hpp"
#include "donut.hpp"

extern LGFX tft;
extern LGFX_Sprite sprite;
extern DonutScreensaver donut;

static bool spriteCreated = false;
static bool preferPsram = false;

bool createScreensaverSprite() {
  if (spriteCreated) return true;

  sprite.setPsram(preferPsram);
  if (sprite.createSprite(300, 300) == nullptr) {
    Serial.println("Internal screensaver sprite allocation failed; trying PSRAM");

    preferPsram = true;
    sprite.setPsram(true);

    if (sprite.createSprite(300, 300) == nullptr) {
      Serial.println("PSRAM screensaver sprite allocation failed");
      return false;
    }
  }

  sprite.fillScreen(TFT_BLACK);
  donut.begin(tft, sprite);
  spriteCreated = true;
  return true;
}

void deleteScreensaverSprite() {
  if (!spriteCreated) return;

  sprite.deleteSprite();
  spriteCreated = false;
}

void runScreensaverFrame(uint32_t frameDelayMs) {
  if (!spriteCreated) {
    delay(frameDelayMs);
    return;
  }

  donut.update();
  donut.draw();
  delay(frameDelayMs);
}

void runScreensaverUntil(unsigned long startMs, uint32_t durationMs, uint32_t frameDelayMs) {
  while (millis() - startMs < durationMs) {
    runScreensaverFrame(frameDelayMs);
  }
}
