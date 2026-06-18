# Spotiframe

![ASCII Banner](assets/ascii-blur-spotiframe.png)

A digital music frame powered by an ESP32-S3 that displays the currently playing Spotify track, album artwork, artist information, and dynamically generated color themes.

Spotiframe connects to Spotify through a lightweight Flask backend, retrieves playback information in real time, and renders a custom touchscreen interface on a 7-inch display.

---

## Features

- Real-time Spotify playback information
- Album artwork display
- Dynamic UI colors extracted from album art
- Wi-Fi connected updates
- Animated ASCII donut screensaver
- Built on ESP32-S3 hardware

---

## ASCII Donut Screensaver 🍩

One of my favorite features of Spotiframe is a rotating ASCII-art donut screensaver inspired by Andy Sloane's and a1k0n's classic terminal donuts.

The donut is rendered in real time on the ESP32-S3 using:

- Parametric torus geometry
- 3D rotation matrices
- Perspective projection
- Per-character depth buffering (z-buffer)
- ASCII brightness shading

Each frame is generated from thousands of sampled points on a torus, projected into screen space, and mapped to ASCII characters based on simulated lighting.

FYI: Strawberry frosted donuts are the best ones!

![ASCII Donut](assets/donut_spin.gif)

---

## Hardware

- Elecrow 7" ESP32-S3 HMI Display (800×480)
- ESP32-S3-WROOM
- Capacitive touchscreen

---

## Software Stack

### Device

- C++
- Arduino Framework
- PlatformIO
- LovyanGFX
- ArduinoJson
- PNGdec

### Backend

- Python
- Flask
- Spotify Web API

### Deployment

- Render

---

## PlatformIO Setup

The ESP32 firmware is built with PlatformIO using the environment in `platformio.ini`:

```ini
[env:rymcu-esp32-s3-devkitc-1]
platform = espressif32
board = rymcu-esp32-s3-devkitc-1
framework = arduino
```

Required device libraries are installed by PlatformIO from `lib_deps`:

- `lovyan03/LovyanGFX`
- `bblanchon/ArduinoJson`
- `bitbank2/PNGdec`

Before building, create `include/secrets.hpp`. This file is ignored by git and should contain Wi-Fi credentials plus the backend host. Use `example-secrets.hpp` as the template.

## Useful Commands

Run these from the project root.

Build the firmware:

```sh
pio run
```

Upload to the connected ESP32-S3:

```sh
pio run --target upload
```

Open the serial monitor:

```sh
pio device monitor
```

Build and upload in one command:

```sh
pio run --target upload && pio device monitor
```

Clean build artifacts:

```sh
pio run --target clean
```

If upload fails because PlatformIO picked the wrong port, list connected devices:

```sh
pio device list
```

Then upload with an explicit port:

```sh
pio run --target upload --upload-port /dev/cu.usbmodemXXXX
```

---

## How It Works

1. Spotify playback data is retrieved through the Spotify Web API.
2. A Flask backend processes track metadata and album artwork.
3. Dominant colors are extracted from album art.
4. The ESP32 periodically requests playback updates.
5. Track information, artwork, and UI elements are rendered on the display.

---

## Gallery

### Now Playing Screen

<img src="assets/now_playing_small.png" width="500">

### Screensaver

![ASCII Donut](assets/donut_spin.gif)

---

## Future Improvements

- Additional screensavers
- Multiple visual themes
- Touchscreen support

---

## License

GNU General Public License v3.0
