#include "display_renderer.hpp"

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include "Free_Sans.hpp"
#include "LGFX.hpp"

extern LGFX tft;
extern PNG png;

static constexpr uint16_t PNG_MARGIN = 20;

static constexpr float TITLE_SIZE = 3;
static const char* TITLE_FONT = "Latin_Hiragana_24";

static constexpr float ARTIST_SIZE = 2;
static const char* ARTIST_FONT = "Latin_Hiragana_24";

void printHeap(const char* label) {
  Serial0.printf("[%s] free heap=%u, largest block=%u, psram=%u\n",
                label,
                ESP.getFreeHeap(),
                ESP.getMaxAllocHeap(),
                ESP.getFreePsram());
}

void uiClear() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
}

void uiStatus(const char* msg) {
  tft.unloadFont();
  tft.setFont(&fonts::Font0);
  tft.setTextSize(2);
  tft.println(msg);
  tft.loadFont(Free_Sans);
}

void uiStatusSameLine(const char* msg) {
  tft.unloadFont();
  tft.setFont(&fonts::Font0);
  tft.setTextSize(2);
  tft.print(msg);
  tft.loadFont(Free_Sans);
}

uint32_t hexToColor(String hex) {
  hex.trim();
  if (hex.startsWith("#")) hex.remove(0, 1);
  return (uint32_t) strtol(hex.c_str(), NULL, 16);
}

uint16_t toRGB565(uint32_t color888) {
  uint8_t r = (color888 >> 16) & 0xFF;
  uint8_t g = (color888 >> 8) & 0xFF;
  uint8_t b = color888 & 0xFF;
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

static uint16_t blend888to565(uint32_t c1, uint32_t c2, float t) {
  uint8_t r1 = (c1 >> 16) & 0xFF;
  uint8_t g1 = (c1 >> 8) & 0xFF;
  uint8_t b1 = c1 & 0xFF;

  uint8_t r2 = (c2 >> 16) & 0xFF;
  uint8_t g2 = (c2 >> 8) & 0xFF;
  uint8_t b2 = c2 & 0xFF;

  uint8_t r = r1 + (r2 - r1) * t;
  uint8_t g = g1 + (g2 - g1) * t;
  uint8_t b = b1 + (b2 - b1) * t;

  return toRGB565((r << 16) | (g << 8) | b);
}

void fadeBackground(uint32_t fromColor, uint32_t toColor) {
  const int STEPS = 40;
  const int FRAME_DELAY_MS = 15;
  uint32_t nextFrame = millis();

  for (int i = 0; i <= STEPS; i++) {
    float t = (float)i / STEPS;
    uint16_t blended = blend888to565(fromColor, toColor, t);
    tft.fillRect(0, 0, tft.width(), tft.height(), blended);

    while (millis() < nextFrame) delay(1);
    nextFrame += FRAME_DELAY_MS;
  }
}

static void drawAdaptiveText(
    const String& text,
    int centerX,
    int baseY,
    int maxWidth,
    uint32_t background,
    int baseSize
) {

  uint8_t r = (background >> 16) & 0xFF;
  uint8_t g = (background >> 8) & 0xFF;
  uint8_t b = background & 0xFF;

  float brightness = 0.299f * r + 0.587f * g + 0.114f * b;
  uint16_t textColor = (brightness > 212) ? TFT_BLACK : TFT_WHITE;

  tft.setTextColor(textColor);
  tft.setTextSize(baseSize);

  String line1 = text;
  String line2 = "";

  if (tft.textWidth(text) > maxWidth) {
    int splitIndex = text.lastIndexOf(' ', text.length() / 2);
    if (splitIndex == -1) splitIndex = text.length() / 2;

    line1 = text.substring(0, splitIndex + 1);
    line2 = text.substring(splitIndex + 1);
    line1.trim();
    line2.trim();
  }

  int currentSize = baseSize;
  while ((tft.textWidth(line1) > maxWidth ||
          (line2.length() > 0 && tft.textWidth(line2) > maxWidth))
         && currentSize > 1) {
    currentSize -= 1;
    tft.setTextSize(currentSize);
  }

  int lineSpacing = tft.fontHeight() + 10;
  int totalHeight = (line2.length() > 0) ? lineSpacing : 0;
  int startY = baseY - totalHeight / 2;

  if (line2.length() > 0) {
    tft.drawString(line1, centerX, startY);
    tft.drawString(line2, centerX, startY + lineSpacing);
  } else {
    tft.drawString(line1, centerX, baseY);
  }
}

void drawNowPlaying(const String& title, const String& artist, uint32_t background) {
  int xOff = png.getWidth() == 0 ? tft.width() : tft.width() - png.getWidth() - PNG_MARGIN;
  int textCenterX = xOff / 2;
  int textY = tft.height() / 2;
  int maxWidth = xOff - 20;

  tft.setTextDatum(MC_DATUM);

  drawAdaptiveText(title, textCenterX, textY - 65, maxWidth, background, TITLE_SIZE);
  drawAdaptiveText(artist, textCenterX, textY + 65, maxWidth, background, ARTIST_SIZE);
}

int drawAlbum(PNGDRAW* pDraw) {
  static int xOff = 0;
  static int yOff = 0;

  if (pDraw->y == 0) {
    const int imgW = png.getWidth();
    const int imgH = png.getHeight();

    xOff = (tft.width() - imgW) - PNG_MARGIN;
    yOff = (tft.height() - imgH) / 2;

    if (xOff < 0) xOff = 0;
    if (yOff < 0) yOff = 0;
  }

  static uint16_t lineBuf[800];

  png.getLineAsRGB565(pDraw, lineBuf, PNG_RGB565_BIG_ENDIAN, 0xFFFFFFFF);
  tft.pushImage(xOff, yOff + pDraw->y, pDraw->iWidth, 1, lineBuf);

  return 1;
}
