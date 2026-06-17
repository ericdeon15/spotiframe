#pragma once
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

// This LGFX class configures the 800x480 Elecrow RGB panel
class LGFX : public lgfx::LGFX_Device {
public:
  lgfx::Bus_RGB      _bus;
  lgfx::Panel_RGB    _panel;
  lgfx::Light_PWM    _backlight;
  lgfx::Touch_GT911  _touch;

  LGFX() {
    { 
      // Panel geometry
      auto cfg = _panel.config();
      cfg.memory_width  = 800;
      cfg.memory_height = 480;
      cfg.panel_width   = 800;
      cfg.panel_height  = 480;
      _panel.config(cfg);
    }

    { 
      // RGB bus (Elecrow pinout)
      auto cfg = _bus.config();
      cfg.panel = &_panel;

      // B0..B4
      cfg.pin_d0 = 15; cfg.pin_d1 = 7;  cfg.pin_d2 = 6;  cfg.pin_d3 = 5;  cfg.pin_d4 = 4;
      // G0..G5
      cfg.pin_d5 = 9;  cfg.pin_d6 = 46; cfg.pin_d7 = 3;  cfg.pin_d8 = 8;  cfg.pin_d9 = 16; cfg.pin_d10 = 1;
      // R0..R4
      cfg.pin_d11 = 14; cfg.pin_d12 = 21; cfg.pin_d13 = 47; cfg.pin_d14 = 48; cfg.pin_d15 = 45;

      // Timing pins
      cfg.pin_henable = 41;
      cfg.pin_vsync   = 40;
      cfg.pin_hsync   = 39;
      cfg.pin_pclk    = 0;

      cfg.freq_write  = 12000000;

      // Porch and sync
      cfg.hsync_polarity = 0;
      cfg.hsync_front_porch = 40;
      cfg.hsync_pulse_width = 48;
      cfg.hsync_back_porch  = 40;

      cfg.vsync_polarity = 0;
      cfg.vsync_front_porch = 1;
      cfg.vsync_pulse_width = 31;
      cfg.vsync_back_porch  = 13;

      cfg.pclk_active_neg = 1;
      cfg.de_idle_high = 0;
      cfg.pclk_idle_high = 0;

      _bus.config(cfg);
    }

    _panel.setBus(&_bus);

    { 
      // Backlight
      auto cfg = _backlight.config();
      cfg.pin_bl = 2;
      _backlight.config(cfg);
    }
    _panel.light(&_backlight);

    { 
      // Touch panel (GT911)
      auto cfg = _touch.config();
      cfg.x_min = 0; 
      cfg.x_max = 799; 
      cfg.y_min = 0; 
      cfg.y_max = 479;

      cfg.pin_int = -1;
      cfg.pin_rst = -1;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;

      cfg.i2c_port = 1;
      cfg.pin_sda = 19;
      cfg.pin_scl = 20;
      cfg.freq = 400000;
      cfg.i2c_addr = 0x14;

      _touch.config(cfg);
      _panel.setTouch(&_touch);
    }

    // Register the panel with LovyanGFX
    setPanel(&_panel);
  }
};
