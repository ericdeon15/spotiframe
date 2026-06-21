#include "playback_controls.hpp"

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include "LGFX.hpp"

extern LGFX tft;

static LGFX_Button playPauseButton;
static bool initialized = false;
static bool showingPause = true;
static uint16_t controlBackground = TFT_BLACK;
static uint16_t controlForeground = TFT_WHITE;

static void drawPlayPauseIcon(
    LovyanGFX* gfx,
    int32_t x,
    int32_t y,
    int32_t w,
    int32_t h,
    bool inverted,
    const char*
) {
  const int32_t centerX = x + w / 2;
  const int32_t centerY = y + h / 2;
  const int32_t radius = min(w, h) / 2 - 2;
  const uint16_t fill = inverted ? controlForeground : controlBackground;
  const uint16_t icon = inverted ? controlBackground : controlForeground;

  gfx->fillRect(x, y, w, h, controlBackground);
  gfx->fillCircle(centerX, centerY, radius, fill);
  gfx->drawCircle(centerX, centerY, radius, controlForeground);

  if (showingPause) {
    gfx->fillRect(centerX - 8, centerY - 11, 5, 22, icon);
    gfx->fillRect(centerX + 3, centerY - 11, 5, 22, icon);
  } else {
    gfx->fillTriangle(
        centerX - 6,
        centerY - 12,
        centerX - 6,
        centerY + 12,
        centerX + 12,
        centerY,
        icon
    );
  }
}

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
      420,
      64,
      64,
      TFT_WHITE,
      TFT_BLACK,
      TFT_WHITE,
      "",
      1
  );
  playPauseButton.setDrawCb(drawPlayPauseIcon);
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

  showingPause = isPlaying;
  controlBackground = background565;
  controlForeground = textColor;
  playPauseButton.drawButton();
}

bool playbackControlPressed() {
  beginPlaybackControls();

  uint16_t x = 0;
  uint16_t y = 0;
  const bool touched = tft.getTouch(&x, &y);

  playPauseButton.press(touched && playPauseButton.contains(x, y));
  const bool pressed = playPauseButton.justPressed();
  if (pressed) playPauseButton.drawButton(true);
  return pressed;
}
