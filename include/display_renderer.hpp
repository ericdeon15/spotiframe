#ifndef DISPLAY_RENDERER_HPP
#define DISPLAY_RENDERER_HPP

#include <Arduino.h>
#include <PNGdec.h>

void printHeap(const char* label);
void uiClear();
void uiStatus(const char* msg);
void uiStatusBottomLeft(const char* msg);
void uiStatusSameLine(const char* msg);
uint32_t hexToColor(String hex);
uint16_t toRGB565(uint32_t color888);
void fadeBackground(uint32_t fromColor, uint32_t toColor);
bool drawScreensaverLogo();
void drawNowPlaying(const String& title, const String& artist, uint32_t background);
int drawAlbum(PNGDRAW* pDraw);

#endif
