#include "playback_controls.hpp"

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include "Free_Sans.hpp"
#include "LGFX.hpp"

extern LGFX tft;

static LGFX_Button playPauseButton;
static bool initialized = false;

static uint16_t controlTextColor(uint32_t background) {
  const uint8_t r = (background >> 16) & 0xFF;
  const uint8_t g = (background >> 8) & 0xFF;
  const uint8_t b = background & 0xFF;
  const float brightness = 0.299f * r + 0.587f * g + 0.114f * b;
  return brightness > 212 ? TFT_BLACK : TFT_WHITE;
}

void beginPlaybackControls() {
  if (initialized) return;

  playPauseButton.initButton(
      &tft,
      180,
      425,
      120,
      50,
      TFT_WHITE,
      TFT_BLACK,
      TFT_WHITE,
      "Pause",
      2
  );
  initialized = true;
}

void drawPlaybackControl(bool isPlaying, uint32_t background) {
  beginPlaybackControls();

  const uint16_t background565 = tft.color565(
      (background >> 16) & 0xFF,
      (background >> 8) & 0xFF,
      background & 0xFF
  );
  const uint16_t textColor = controlTextColor(background);

  playPauseButton.setOutlineColor(textColor);
  playPauseButton.setFillColor(background565);
  playPauseButton.setTextColor(textColor);
  playPauseButton.setLabelText(isPlaying ? "Pause" : "Play");

  tft.unloadFont();
  tft.setFont(&fonts::Font0);
  playPauseButton.drawButton();
  tft.loadFont(Free_Sans);
}

bool playbackControlPressed() {
  beginPlaybackControls();

  uint16_t x = 0;
  uint16_t y = 0;
  const bool touched = tft.getTouch(&x, &y);

  playPauseButton.press(touched && playPauseButton.contains(x, y));
  return playPauseButton.justReleased();
}
