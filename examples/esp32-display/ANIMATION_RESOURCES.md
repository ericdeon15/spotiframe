# Elecrow 7" Display Resources

While building Spotiframe, one of the most frustrating parts was getting the Elecrow 7" ESP32 display configured correctly with LovyanGFX. It was surprisingly difficult to find a resource that showed a complete setup that actually worked for my hardware and software configuration.

In particular, figuring out how to properly construct the custom `LGFX` class—including the RGB bus configuration, display timing parameters, backlight setup, and GT911 touch controller initialization—took a fair amount of experimentation and debugging before everything was working reliably.

The resources below were especially helpful during development and may save future developers some time.

## Helpful Resources

### Elecrow 7" ESP32 Touchscreen Repository

The `CollisionCircles` example from the Elecrow community repository was invaluable for understanding how the display hardware is initialized and configured.

Repository:
https://github.com/getis/elecrow-7in-esp32-touchscreen

### Bytes and Bits Tutorial

A great walkthrough covering setup, configuration, and usage of the Elecrow 7" display.

Tutorial:
https://bytesnbits.co.uk/esp32-7-inch-lcd-elecrow

## Recommended Test

If you're trying to verify that your display configuration is working correctly, the `CollisionCircles` example is a great first test. It exercises the display heavily and provides a quick way to confirm that:

- The RGB panel is configured correctly
- Timing values are valid
- Touch input is functioning
- Rendering performance is reasonable

## Attribution

The original example code is not redistributed in this repository. Please refer to the original sources above for the code itself, licensing information, and usage instructions.

This note exists simply to document the resources that helped me get the display working and may be useful to anyone trying to do something similar.
