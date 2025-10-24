#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

class LGFX : public lgfx::LGFX_Device {
public:
  lgfx::Bus_RGB _bus_instance;
  lgfx::Panel_RGB _panel_instance;
  lgfx::Light_PWM _light_instance;
  lgfx::Touch_GT911 _touch_instance;

  LGFX(void) {
    // ----- Panel -----
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width  = 800;
      cfg.memory_height = 480;
      cfg.panel_width   = 800;
      cfg.panel_height  = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      _panel_instance.config(cfg);
    }

    // ----- RGB Bus -----
    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;

      // Blue pins
      cfg.pin_d0 = 15;  cfg.pin_d1 = 7;  cfg.pin_d2 = 6;
      cfg.pin_d3 = 5;   cfg.pin_d4 = 4;

      // Green pins
      cfg.pin_d5 = 9;   cfg.pin_d6 = 46; cfg.pin_d7 = 3;
      cfg.pin_d8 = 8;   cfg.pin_d9 = 16; cfg.pin_d10 = 1;

      // Red pins
      cfg.pin_d11 = 14; cfg.pin_d12 = 21;
      cfg.pin_d13 = 47; cfg.pin_d14 = 48; cfg.pin_d15 = 45;

      cfg.pin_henable = 41;  // DE
      cfg.pin_vsync   = 40;
      cfg.pin_hsync   = 39;
      cfg.pin_pclk    = 0;
      cfg.freq_write  = 12000000;

      cfg.hsync_polarity = 0;
      cfg.hsync_front_porch = 40;
      cfg.hsync_pulse_width = 48;
      cfg.hsync_back_porch  = 40;

      cfg.vsync_polarity = 0;
      cfg.vsync_front_porch = 1;
      cfg.vsync_pulse_width = 31;
      cfg.vsync_back_porch  = 13;

      cfg.pclk_active_neg = 1;
      cfg.de_idle_high    = 0;
      cfg.pclk_idle_high  = 0;

      _bus_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);

    // ----- Backlight -----
    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = 2;   // change to 38 if dark
      _light_instance.config(cfg);
    }
    _panel_instance.light(&_light_instance);

    // ----- Touch -----
    {
      auto cfg = _touch_instance.config();
      cfg.x_min = 0;  cfg.x_max = 799;
      cfg.y_min = 0;  cfg.y_max = 479;
      cfg.pin_int = -1;  cfg.pin_rst = -1;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      cfg.i2c_port = 1;
      cfg.pin_sda = 19;  cfg.pin_scl = 20;
      cfg.freq = 400000; cfg.i2c_addr = 0x14;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};

LGFX tft;

void setup() {
  Serial.begin(115200);
  Serial.println("Init Elecrow 7\" HMI...");
  tft.init();
  tft.setBrightness(255);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(3);
  tft.drawString("Hello Elecrow 7-inch!", 100, 100);
}

void loop() {
  static int c = 0;
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setCursor(120, 220);
  tft.printf("Count: %d", c++);
  delay(500);
}
