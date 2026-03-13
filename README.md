# ESP32 Matter LED Matrix Controller

This project provides firmware for ESP32 chips to control a WS2812B LED Matrix (e.g., 8x8 WS2812B-64) over the Matter protocol, exposing it as an Extended Color Light device and allowing integration into smart home ecosystems like Apple HomeKit.

## Features

- **Matter Protocol Support**: Implements the standard Matter Extended Color Light device type.
- **HomeKit Compatibility**: Includes explicit HomeKit/HSL fixes to ensure full color, saturation, and temperature control within the Apple Home interface.
- **WS2812B Control**: Uses the ESP32 hardware RMT peripheral for efficient, flicker-free control of addressable LEDs (default 64 LEDs, adjustable in code).
- **Color Temperature & Hue/Saturation**: Support for CIE 1931 XY colors, Hue/Saturation, and Color Temperature (Mireds).

## Hardware Setup

1. **ESP32**: Standard ESP32, ESP32-C3, or similar.
2. **WS2812B Matrix**: Connect the data line of the WS2812B strip/matrix to the configured GPIO pin (default: `GPIO_NUM_8` on ESP32-C3, check `app_driver.cpp`). Be sure to use a level shifter if necessary, and power the matrix appropriately.

## Prerequisites

- Standard [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) (Check esp-matter for compatible version)
- [ESP-Matter SDK](https://github.com/espressif/esp-matter) properly installed and environment exported (`export.sh`).

## Build and Flash
1. Set ESP-IDF and ESP-Matter export paths (adjust to your installation paths):
   ```bash
   source ~/esp/esp-idf/export.sh
   source ~/esp/esp-matter/export.sh
   ```

2. Configure the project for your target (e.g., esp32c3):
   ```bash
   idf.py set-target esp32c3
   ```
3. Build the project:
   ```bash
   idf.py build
   ```
4. Flash and monitor output to find the commissioning QR code/pairing code:
   ```bash
   idf.py flash monitor
   ```

## Commissioning

Once the device is flashing and running, open your Matter controller app (e.g., Apple Home, Google Home) and add a new accessory. The pairing code and QR code URL will be printed in the serial terminal output on the first boot.

## Licensing

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
