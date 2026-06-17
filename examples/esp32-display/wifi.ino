/**
 * Wi-Fi connection setup using WPA2 Enterprise
 *
 * Adapted from Jeroen Beemster's ESP32-WPA2-Enterprise example:
 * https://github.com/JeroenBeemster/ESP32-WPA2-enterprise
 *
 * Licensed under the GNU General Public License v3.0
 * See LICENSE file or https://www.gnu.org/licenses/gpl-3.0.html for details.
 *
 * Modifications by Eric D'Eon (2025)
 */

#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wpa2.h"

#include "secrets.h"

// ==== MODE SELECT ====
#define USE_ENTERPRISE true

// ==== Elecrow 7" Display ====
class LGFX : public lgfx::LGFX_Device {
public:
  lgfx::Bus_RGB _bus_instance;
  lgfx::Panel_RGB _panel_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_GT911 _touch_instance;

  LGFX(void) {
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width  = 800;
      cfg.memory_height = 480;
      cfg.panel_width   = 800;
      cfg.panel_height  = 480;
      _panel_instance.config(cfg);
    }
    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;
      cfg.pin_d0 = 15; cfg.pin_d1 = 7; cfg.pin_d2 = 6; cfg.pin_d3 = 5; cfg.pin_d4 = 4;
      cfg.pin_d5 = 9; cfg.pin_d6 = 46; cfg.pin_d7 = 3; cfg.pin_d8 = 8; cfg.pin_d9 = 16; cfg.pin_d10 = 1;
      cfg.pin_d11 = 14; cfg.pin_d12 = 21; cfg.pin_d13 = 47; cfg.pin_d14 = 48; cfg.pin_d15 = 45;
      cfg.pin_henable = 41; cfg.pin_vsync = 40; cfg.pin_hsync = 39; cfg.pin_pclk = 0;
      cfg.freq_write = 12000000;
      cfg.hsync_polarity = 0; cfg.hsync_front_porch = 40; cfg.hsync_pulse_width = 48; cfg.hsync_back_porch = 40;
      cfg.vsync_polarity = 0; cfg.vsync_front_porch = 1; cfg.vsync_pulse_width = 31; cfg.vsync_back_porch = 13;
      cfg.pclk_active_neg = 1; cfg.de_idle_high = 0; cfg.pclk_idle_high = 0;
      _bus_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 2;
      _light_instance.config(cfg);
    }
    _panel_instance.light(&_light_instance);
    {
      auto cfg = _touch_instance.config();
      cfg.x_min = 0; cfg.x_max = 799; cfg.y_min = 0; cfg.y_max = 479;
      cfg.pin_int = -1; cfg.pin_rst = -1;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port = 1;
      cfg.pin_sda = 19; cfg.pin_scl = 20;
      cfg.freq = 400000; cfg.i2c_addr = 0x14;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }
    setPanel(&_panel_instance);
  }
};

LGFX tft;

void connectWiFi() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 20);

  if (USE_ENTERPRISE) {
    tft.println("Connecting to WPA2-Enterprise");
    Serial.println("WPA2-Enterprise...");

    // WPA2-Enterprise setup
    esp_wifi_sta_wpa2_ent_clear_identity();
    esp_wifi_sta_wpa2_ent_clear_username();
    esp_wifi_sta_wpa2_ent_clear_password();

    esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY));
    esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME));
    esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD));

    esp_wifi_sta_wpa2_ent_enable();

    WiFi.begin(ssid_ent);
  } else {
    tft.println("Connecting to WPA2-PSK");
    Serial.println("WPA2-PSK...");
    WiFi.begin(SSID_PSK, PSK_PASSWORD);
  }

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    tft.print(".");
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    tft.println("\n\nConnected!");
    Serial.println("\nonnected!");
    tft.print("IP: ");
    tft.println(WiFi.localIP());
  } else {
    tft.setTextColor(TFT_RED);
    tft.println("\nWiFi connection failed.");
    Serial.println("\nWiFi connection failed.");
  }
}

void setup() {
  Serial.begin(115200);
  tft.init();
  tft.setBrightness(255);
  tft.setRotation(0);
  connectWiFi();
}

void loop() {}