// Spotiframe: ESP32-S3 + Elecrow 7" display
// Shows current Spotify track title/artist + album art from a Flask server.
// Hardware: Elecrow ESP32-S3 HMI 7" (800x480, RGB panel)
// Display lib: LovyanGFX
// PNG decode: bitbank2/PNGdec
// Wi-Fi: WPA2-Enterprise or WPA2-PSK (configure in secrets.h)

#include <Arduino.h>

#define LGFX_USE_V1

#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include "Free_Sans.hpp"
#include "LGFX.hpp"

#include "donut.hpp"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PNGdec.h>
#include <vector>

#include "esp_wifi.h"
#include "esp_wpa2.h"
#include "secrets.hpp"

#include <HardwareSerial.h>

#include "app_state.hpp"
#include "display_renderer.hpp"
#include "screensaver.hpp"
#include "spotify_response.hpp"

// HardwareSerial Serial0(0);

static constexpr uint16_t HTTPS_PORT = 443;
static constexpr uint32_t SOCKET_TIMEOUT_MS = 10000;
static constexpr uint32_t UPDATE_INTERVAL_MS = 2500;

static const char* SERVER_HOST = SPOTIFRAME_HOST;

static const char* TITLE_FONT = "Latin_Hiragana_24";

LGFX tft;
LGFX_Sprite sprite(&tft);
DonutScreensaver donut;
PNG png;

// ============================ Wi-Fi ============================

static void wifiConnect() {
  WiFi.disconnect(true, true);
  delay(200);
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

  appState.donutStart = millis();

  for (int attempts = 0; attempts < 30 && WiFi.status() != WL_CONNECTED; ++attempts) {
    for (int i = 0; i < 100; i++) {
      runScreensaverFrame(10);

      if (WiFi.status() == WL_CONNECTED) break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    runScreensaverUntil(appState.donutStart, 3000, 5);

    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_GREEN);
    uiStatus("Connected!");
    tft.setTextColor(TFT_WHITE);
    delay(300);
    tft.fillScreen(TFT_BLACK);
  } else {
    tft.setTextColor(TFT_RED);
    uiStatus("WiFi connection failed");
    tft.setTextColor(TFT_WHITE);
  }
}

// ============================ Setup / Loop ============================

void setup() {
  Serial.begin(115200);
  Serial0.begin(115200);
  delay(200);

  tft.init();
  tft.setRotation(0);
  tft.setBrightness(255);

  tft.loadFont(TITLE_FONT);
  uiClear();
  uiStatus("Spotiframe booting...");

  printHeap("before donut sprite");

  createScreensaverSprite();

  printHeap("after donut sprite");

  wifiConnect();

  // CRITICAL: free donut sprite before HTTPS and PNG decoding.
  deleteScreensaverSprite();

  printHeap("after sprite delete");

  tft.fillScreen(TFT_BLACK);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    uiStatus("Reconnecting WiFi...");

    createScreensaverSprite();

    wifiConnect();

    deleteScreensaverSprite();

    delay(1000);
    return;
  }

  printHeap("before /current");

  WiFiClientSecure client;
  client.setInsecure();
  client.setTimeout(SOCKET_TIMEOUT_MS);

  if (!client.connect(SERVER_HOST, HTTPS_PORT)) {
    Serial.println("Server connect failed");
    delay(500);
    return;
  }

  client.println("GET /current HTTP/1.1");
  client.printf("Host: %s\r\n", SERVER_HOST);
  client.println("Connection: close\r\n");

  readHttpHeaders(client);
  const String jsonPayload = readBodyDechunked(client);
  client.stop();

  SpotifyCurrent current;

  if (!parseSpotifyCurrent(jsonPayload, current)) {
    delay(500);
    return;
  }

  if (current.status == "stopped") {
    delay(500);
    return;
  }

  if (current.id == appState.currentID) {
    delay(UPDATE_INTERVAL_MS);
    return;
  }

  appState.currentID = current.id;

  uint32_t background = hexToColor(current.color);

  if (appState.hasPrevColor) {
    fadeBackground(appState.prevColor, background);
  } else {
    tft.fillScreen(toRGB565(background));
    appState.hasPrevColor = true;
  }

  appState.prevColor = background;

  printHeap("before /album");

  WiFiClientSecure img;
  img.setInsecure();
  img.setTimeout(SOCKET_TIMEOUT_MS);

  if (img.connect(SERVER_HOST, HTTPS_PORT)) {
    img.println("GET /album HTTP/1.1");
    img.printf("Host: %s\r\n", SERVER_HOST);
    img.println("Connection: close\r\n");

    readHttpHeaders(img);

    std::vector<uint8_t> pngData;
    pngData.reserve(120000);

    readBodyBinary(img, pngData);
    img.stop();

    Serial.printf("Album bytes: %d\n", pngData.size());

    Serial.print("First bytes: ");
    for (int i = 0; i < min(16, (int)pngData.size()); i++) {
      Serial.printf("%02X ", pngData[i]);
    }
    Serial.println();

    const int openRC = png.openRAM(pngData.data(), (int)pngData.size(), drawAlbum);

    Serial.printf("PNG openRC: %d\n", openRC);

    if (openRC == PNG_SUCCESS) {
      tft.startWrite();
      int decodeRC = png.decode(nullptr, 0);
      tft.endWrite();

      Serial.printf("PNG decodeRC: %d\n", decodeRC);

      png.close();
    } else {
      Serial.println("PNG open failed");
    }

    pngData.clear();
    pngData.shrink_to_fit();
  } else {
    Serial.println("Album connect failed");
  }

  printHeap("after /album");

  drawNowPlaying(current.title, current.artist, background);

  delay(UPDATE_INTERVAL_MS);
}
