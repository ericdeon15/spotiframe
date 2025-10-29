// Spotiframe: ESP32-S3 + Elecrow 7" display
// Shows current Spotify track title/artist + album art from a Flask server.
// Hardware: Elecrow ESP32-S3 HMI 7" (800x480, RGB panel)
// Display lib: LovyanGFX
// PNG decode: bitbank2/PNGdec
// Wi-Fi: WPA2-Enterprise or WPA2-PSK (configure in secrets.h)

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <PNGdec.h>
#include <vector>

#include "esp_wifi.h"
#include "esp_wpa2.h"

// ---- Project configuration ----
#include "secrets.h" 

// ---- Networking constants ----
static constexpr uint16_t HTTPS_PORT        = 443;
static constexpr uint32_t SOCKET_TIMEOUT_MS = 10000;
static constexpr uint32_t UPDATE_INTERVAL_MS = 2500;

// ---- Flask/Render host ----
static const char* SERVER_HOST = SPOTIFRAME_HOST; // from secrets.h

// ---- UI constants ----
static constexpr uint16_t PNG_MARGIN = 20;

static constexpr float TITLE_SIZE = 3;
static const lgfx::IFont* TITLE_FONT = &fonts::lgfxJapanGothicP_16;

static constexpr float ARTIST_SIZE = 2;
static const lgfx::IFont* ARTIST_FONT = &fonts::lgfxJapanGothicP_16;

// ---- TRACK STATE ----
String currentID = "";
uint32_t prevColor = 0x000000;  // previous background (for fade)
bool hasPrevColor = false;

// ====================== Display wiring (Elecrow ESP32-S3 7") ======================
class LGFX : public lgfx::LGFX_Device {
public:
  lgfx::Bus_RGB   _bus;
  lgfx::Panel_RGB _panel;
  lgfx::Light_PWM _backlight;
  lgfx::Touch_GT911 _touch;

  LGFX() {
    { // Panel geometry
      auto cfg = _panel.config();
      cfg.memory_width  = 800;
      cfg.memory_height = 480;
      cfg.panel_width   = 800;
      cfg.panel_height  = 480;
      _panel.config(cfg);
    }
    { // RGB bus (Elecrow pinout)
      auto cfg = _bus.config();
      cfg.panel = &_panel;
      // B0..B4
      cfg.pin_d0 = 15; cfg.pin_d1 = 7;  cfg.pin_d2 = 6;  cfg.pin_d3 = 5;  cfg.pin_d4 = 4;
      // G0..G5
      cfg.pin_d5 = 9;  cfg.pin_d6 = 46; cfg.pin_d7 = 3;  cfg.pin_d8 = 8;  cfg.pin_d9 = 16; cfg.pin_d10 = 1;
      // R0..R4
      cfg.pin_d11 = 14; cfg.pin_d12 = 21; cfg.pin_d13 = 47; cfg.pin_d14 = 48; cfg.pin_d15 = 45;

      cfg.pin_henable = 41;
      cfg.pin_vsync   = 40;
      cfg.pin_hsync   = 39;
      cfg.pin_pclk    = 0;
      cfg.freq_write  = 12000000;

      cfg.hsync_polarity = 0; cfg.hsync_front_porch = 40; cfg.hsync_pulse_width = 48; cfg.hsync_back_porch = 40;
      cfg.vsync_polarity = 0; cfg.vsync_front_porch = 1;  cfg.vsync_pulse_width = 31; cfg.vsync_back_porch = 13;

      cfg.pclk_active_neg = 1; cfg.de_idle_high = 0; cfg.pclk_idle_high = 0;
      _bus.config(cfg);
    }
    _panel.setBus(&_bus);

    { // Backlight
      auto cfg = _backlight.config();
      cfg.pin_bl = 2;
      _backlight.config(cfg);
    }
    _panel.light(&_backlight);

    { // Touch (GT911)
      auto cfg = _touch.config();
      cfg.x_min = 0; cfg.x_max = 799; cfg.y_min = 0; cfg.y_max = 479;
      cfg.pin_int = -1; cfg.pin_rst = -1;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port = 1;
      cfg.pin_sda = 19; cfg.pin_scl = 20;
      cfg.freq = 400000; cfg.i2c_addr = 0x14;
      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }

    setPanel(&_panel);
  }
};

LGFX tft;
PNG  png;

// ============================ UI helpers ============================
static void uiClear() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 20);
}

static void uiStatus(const char* msg) {
  tft.println(msg);
}

// ---- Color utilities ----
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

// ---- Smooth color blending (RGB888 → RGB565) ----
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

/*
Performs a short background fade between the previous and new dominant color.
Text and album art DO NOT visible during the transition.
*/
static void fadeBackground(uint32_t fromColor, uint32_t toColor) {
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

// ---- Text rendering ----
static void drawAdaptiveText(
    const String& text,
    int centerX,
    int baseY,
    int maxWidth,
    uint32_t background,
    int baseSize,
    const lgfx::IFont* font
) {
  uint8_t r = (background >> 16) & 0xFF;
  uint8_t g = (background >> 8) & 0xFF;
  uint8_t b = background & 0xFF;

  float brightness = 0.299f * r + 0.587f * g + 0.114f * b;
  uint16_t textColor = (brightness > 212) ? TFT_BLACK : TFT_WHITE;

  tft.setTextColor(textColor);
  tft.setTextSize(baseSize);
  tft.setFont(font);

  String line1 = text;
  String line2 = "";

  if (tft.textWidth(text) > maxWidth) {
    int splitIndex = text.lastIndexOf(' ', text.length() / 2);
    if (splitIndex == -1) splitIndex = text.length() / 2;
    line1 = text.substring(0, splitIndex);
    line2 = text.substring(splitIndex + 1);
  }

  int currentSize = baseSize;
  while ((tft.textWidth(line1) > maxWidth ||
          (line2.length() > 0 && tft.textWidth(line2) > maxWidth))
         && currentSize > 1) {
    currentSize -= 0.25;
    tft.setTextSize(currentSize);
  }

  int lineSpacing = tft.fontHeight() + 4;
  int totalHeight = (line2.length() > 0) ? lineSpacing : 0;
  int startY = baseY - totalHeight / 2;

  if (line2.length() > 0) {
    tft.drawString(line1, centerX, startY);
    tft.drawString(line2, centerX, startY + lineSpacing);
  } else {
    tft.drawString(line1, centerX, baseY);
  }
}

static void drawNowPlaying(const String& title, const String& artist, uint32_t background) {
  int xOff = png.getWidth() == 0 ? tft.width() : tft.width() - png.getWidth() - PNG_MARGIN;
  int textCenterX = xOff / 2;
  int textY = tft.height() / 2; 
  int maxWidth = xOff - 20;

  tft.setTextDatum(MC_DATUM);

  drawAdaptiveText(title, textCenterX, textY - 30, maxWidth, background, TITLE_SIZE, TITLE_FONT);
  drawAdaptiveText(artist, textCenterX, textY + 30, maxWidth, background, ARTIST_SIZE, ARTIST_FONT);
}

// ============================ WPA2 Enterprise ============================
static void wifiConnect() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);

  uiClear();
  if (USE_ENTERPRISE) {
    uiStatus("Connecting (WPA2-Enterprise)...");
    esp_wifi_sta_wpa2_ent_clear_identity();
    esp_wifi_sta_wpa2_ent_clear_username();
    esp_wifi_sta_wpa2_ent_clear_password();
    esp_wifi_sta_wpa2_ent_set_identity((uint8_t*)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t*)EAP_USERNAME, strlen(EAP_USERNAME));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t*)EAP_PASSWORD, strlen(EAP_PASSWORD));
    esp_wifi_sta_wpa2_ent_enable();
    WiFi.begin(SSID_ENT);
  } else {
    uiStatus("Connecting (WPA2-PSK)...");
    WiFi.begin(SSID_PSK, PSK_PASSWORD);
  }

  for (int attempts = 0; attempts < 30 && WiFi.status() != WL_CONNECTED; ++attempts) {
    delay(1000);
    tft.print(".");
  }

  tft.println();
  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(TFT_GREEN); tft.println("Connected!");
    tft.setTextColor(TFT_WHITE);
    tft.print("IP: "); tft.println(WiFi.localIP().toString());
  } else {
    tft.setTextColor(TFT_RED); tft.println("WiFi connection failed");
    tft.setTextColor(TFT_WHITE);
  }
}

// ============================ HTTP helpers ============================
static void readHttpHeaders(WiFiClient& c) {
  while (c.connected()) {
    String line = c.readStringUntil('\n');
    if (line == "\r" || line == "") break;
  }
}

static String readBodyDechunked(WiFiClient& c) {
  String body;
  while (c.connected() || c.available()) {
    body += c.readString();
    delay(10);
  }

  int start = body.indexOf('{');
  int end   = body.lastIndexOf('}');
  if (start == -1 || end == -1 || end <= start) {
    Serial.println("No valid JSON object found in response.");
    Serial.println(body);
    return "";
  }

  String jsonPayload = body.substring(start, end + 1);
  Serial.println("Cleaned JSON payload:");
  Serial.println(jsonPayload);
  Serial.println("========================");
  return jsonPayload;
}

static void readBodyBinary(WiFiClient& c, std::vector<uint8_t>& out) {
  out.clear();
  c.setTimeout(SOCKET_TIMEOUT_MS);
  while (c.connected() || c.available()) {
    while (c.available()) {
      out.push_back(c.read());
    }
    delay(10);
  }
}

// ============================ PNG drawing callback ============================
int drawAlbum(PNGDRAW* pDraw) {
  static int xOff = 0, yOff = 0;
  if (pDraw->y == 0) {
    const int imgW = png.getWidth();
    const int imgH = png.getHeight();
    xOff = (tft.width()  - imgW) - PNG_MARGIN;
    yOff = (tft.height() - imgH) / 2;
    if (xOff < 0) xOff = 0;
    if (yOff < 0) yOff = 0;
  }

  static uint16_t lineBuf[800];
  png.getLineAsRGB565(pDraw, lineBuf, PNG_RGB565_BIG_ENDIAN, 0xFFFFFFFF);
  tft.pushImage(xOff, yOff + pDraw->y, pDraw->iWidth, 1, lineBuf);
  return 1;
}

// ============================ Setup / Loop ============================
void setup() {
  Serial.begin(115200);
  Serial0.begin(115200);
  delay(200);
  tft.init();
  tft.setRotation(0);
  tft.setBrightness(255);
  uiClear();
  uiStatus("Spotiframe booting...");
  wifiConnect();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    uiStatus("Reconnecting WiFi...");
    wifiConnect();
    delay(5000);
    return;
  }

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(SOCKET_TIMEOUT_MS);

  if (!client.connect(SERVER_HOST, HTTPS_PORT)) {
    uiStatus("Server connect failed");
    delay(5000);
    return;
  }

  client.println("GET /current HTTP/1.1");
  client.printf("Host: %s\r\n", SERVER_HOST);
  client.println("Connection: close\r\n");

  readHttpHeaders(client);
  const String jsonPayload = readBodyDechunked(client);
  client.stop();

  StaticJsonDocument<1024> doc;
  if (deserializeJson(doc, jsonPayload)) {
    uiStatus("JSON parse error");
    delay(2000);
    return;
  }

  String status = doc["status"].as<String>();
  String title = doc["title"].as<String>();
  String artist = doc["artist"].as<String>();
  String id = doc["id"].as<String>();
  String colorStr = doc["color"].as<String>();

  if (status == "stopped") {
    delay(5000);
    return;
  }

  if (id == currentID) {
    delay(UPDATE_INTERVAL_MS);
    return;
  }
  currentID = id;

  uint32_t background = hexToColor(colorStr);
  if (hasPrevColor) {
    fadeBackground(prevColor, background);
  } else {
    tft.fillScreen(toRGB565(background));
    hasPrevColor = true;
  }
  prevColor = background;

  // ---- Fetch PNG (album art) ----
  WiFiClientSecure img;
  img.setInsecure();
  img.setTimeout(SOCKET_TIMEOUT_MS);

  if (img.connect(SERVER_HOST, HTTPS_PORT)) {
    img.println("GET /album HTTP/1.1");
    img.printf("Host: %s\r\n", SERVER_HOST);
    img.println("Connection: close\r\n");

    readHttpHeaders(img);
    std::vector<uint8_t> pngData;
    readBodyBinary(img, pngData);
    img.stop();

    const int openRC = png.openRAM(pngData.data(), (int)pngData.size(), drawAlbum);
    if (openRC == PNG_SUCCESS) {
      tft.startWrite();
      (void)png.decode(nullptr, 0);
      tft.endWrite();
      png.close();
    } else {
      uiStatus("PNG open failed");
    }
  } else {
    uiStatus("Album connect failed");
  }

  drawNowPlaying(title, artist, background);
  delay(UPDATE_INTERVAL_MS);
}