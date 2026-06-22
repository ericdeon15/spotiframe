#include "app_mode.hpp"

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include "LGFX.hpp"
#include "display_renderer.hpp"

extern LGFX tft;

namespace {

struct ModeOption {
  AppMode mode;
  const char* label;
};

constexpr ModeOption MODE_OPTIONS[] = {
    {AppMode::Spotify, "Spotify"},
    {AppMode::Donut, "Donut"},
};

constexpr size_t MODE_OPTION_COUNT =
    sizeof(MODE_OPTIONS) / sizeof(MODE_OPTIONS[0]);
constexpr int MAX_COLUMNS = 2;
constexpr int BUTTON_WIDTH = 220;
constexpr int BUTTON_HEIGHT = 90;
constexpr int BUTTON_GAP = 40;
constexpr int BUTTON_AREA_TOP = 275;

LGFX_Button modeButtons[MODE_OPTION_COUNT];
LGFX_Button backButton;
AppMode currentMode = AppMode::StartupMenu;
uint16_t backBackground = TFT_BLACK;
uint16_t backForeground = TFT_WHITE;

void drawBackArrow(
    LovyanGFX* gfx,
    int32_t x,
    int32_t y,
    int32_t w,
    int32_t h,
    bool inverted,
    const char*
) {
  const uint16_t background =
      inverted ? backForeground : backBackground;
  const uint16_t foreground =
      inverted ? backBackground : backForeground;
  const int32_t centerX = x + w / 2;
  const int32_t centerY = y + h / 2;
  const int32_t radius = min(w, h) / 2 - 2;

  gfx->fillRect(x, y, w, h, backBackground);
  gfx->fillCircle(centerX, centerY, radius, background);
  gfx->drawCircle(centerX, centerY, radius, backForeground);
  gfx->fillRect(centerX - 7, centerY - 2, 17, 5, foreground);
  gfx->fillTriangle(
      centerX - 12,
      centerY,
      centerX - 3,
      centerY - 9,
      centerX - 3,
      centerY + 9,
      foreground
  );
}

void initializeButton(size_t index) {
  const int rows =
      (static_cast<int>(MODE_OPTION_COUNT) + MAX_COLUMNS - 1) / MAX_COLUMNS;
  const int row = index / MAX_COLUMNS;
  const int column = index % MAX_COLUMNS;
  const int rowItemCount =
      min(MAX_COLUMNS, static_cast<int>(MODE_OPTION_COUNT) - row * MAX_COLUMNS);
  const int rowWidth =
      rowItemCount * BUTTON_WIDTH + (rowItemCount - 1) * BUTTON_GAP;
  const int x =
      (tft.width() - rowWidth) / 2 + column * (BUTTON_WIDTH + BUTTON_GAP);
  const int areaHeight = rows * BUTTON_HEIGHT + (rows - 1) * BUTTON_GAP;
  const int y = BUTTON_AREA_TOP + (tft.height() - BUTTON_AREA_TOP - areaHeight) / 2
      + row * (BUTTON_HEIGHT + BUTTON_GAP);
  const uint16_t outlineColor = TFT_WHITE;
  const uint16_t fillColor = TFT_BLACK;
  const uint16_t textColor = TFT_WHITE;

  modeButtons[index].initButton(
      &tft,
      x + BUTTON_WIDTH / 2,
      y + BUTTON_HEIGHT / 2,
      BUTTON_WIDTH,
      BUTTON_HEIGHT,
      outlineColor,
      fillColor,
      textColor,
      MODE_OPTIONS[index].label,
      2
  );
}

}  // namespace

void beginAppModeMenu() {
  currentMode = AppMode::StartupMenu;
  tft.fillScreen(TFT_BLACK);
  drawScreensaverLogo();
  tft.unloadFont();
  tft.setFont(&fonts::Font0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.drawString("Choose a mode", tft.width() / 2, 230);

  for (size_t i = 0; i < MODE_OPTION_COUNT; ++i) {
    initializeButton(i);
    modeButtons[i].drawButton();
  }
}

void updateAppModeMenu() {
  if (currentMode != AppMode::StartupMenu) return;

  uint16_t x = 0;
  uint16_t y = 0;
  const bool touched = tft.getTouch(&x, &y);

  for (size_t i = 0; i < MODE_OPTION_COUNT; ++i) {
    modeButtons[i].press(touched && modeButtons[i].contains(x, y));

    if (modeButtons[i].justPressed()) {
      modeButtons[i].drawButton(true);
      currentMode = MODE_OPTIONS[i].mode;
      return;
    }

    if (modeButtons[i].justReleased()) {
      modeButtons[i].drawButton();
    }
  }
}

void drawAppModeBackButton(uint32_t background) {
  const uint8_t r = (background >> 16) & 0xFF;
  const uint8_t g = (background >> 8) & 0xFF;
  const uint8_t b = background & 0xFF;
  const float brightness = 0.299f * r + 0.587f * g + 0.114f * b;

  backBackground = tft.color565(r, g, b);
  backForeground = brightness > 212 ? TFT_BLACK : TFT_WHITE;
  backButton.initButton(
      &tft,
      32,
      32,
      48,
      48,
      backForeground,
      backBackground,
      backForeground,
      "",
      1
  );
  backButton.setDrawCb(drawBackArrow);
  backButton.drawButton();
}

bool appModeBackPressed() {
  uint16_t x = 0;
  uint16_t y = 0;
  const bool touched = tft.getTouch(&x, &y);

  backButton.press(touched && backButton.contains(x, y));
  if (!backButton.justPressed()) return false;

  backButton.drawButton(true);
  return true;
}

AppMode getAppMode() {
  return currentMode;
}
