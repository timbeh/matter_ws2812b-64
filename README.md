# ESP32 Matter WS2812B Controller

This project provides production-ready firmware for ESP32 chips to control a WS2812B LED Matrix (e.g., 8x8 WS2812B-64) over the Matter protocol, exposing it as an Extended Color Light device perfectly integrated into Apple HomeKit and smart home ecosystems.

## Core Features

- **Matter Protocol Support**: Full-featured Matter Extended Color Light, correctly syncing states locally and externally.
- **HomeKit Optimization**: Seamless color, saturation, brightness, and color temperature (Mireds) control directly from the Apple Home app, including White-Balance calibrations tailored for generic GRB/RGB WS2812B nodes.
- **Asynchronous RTOS Renderer Pipeline**: Thread-safe architecture decoupling Matter protocol operations from physical RMT interactions. Updates are coalesced inside a FreeRTOS `Queue` targeting `50ms` debouncing latency to eliminate HomeKit control flickering or Matter stack blocking out.
- **State Persistence**: Attributes (Hue, X, Y, Mireds, Brightness) are stored directly inside the `NVS` partition upon modification and correctly snap-restored prior to Matter initializing up to prevent "offline" or "mismatched" status feedback upon boot. 
- **Hardware Protection Pipeline**: 
  - **High-Fidelity Dynamic Power Model**: Our renderer implements a physics-based power model for WS2812B. It accounts for the static quiescent current of the ICs (~1mA/LED) and calculates precise per-channel draw (R/G/B). If the limit is reached, it dynamically scales variable LED intensity while preserving the baseline logic power, ensuring maximum brightness for single-color modes while protecting your PSU during full-white modes.
  - Explicit GPIO UART Multiplex-Overriding, shielding your matrix from receiving junk data during standard framework boot sequences.

## Architecture

The previous monolithic implementation has been restructured into standard modules under `/main`:
- `/state` -> Thread-safe `LedState` abstraction and mutexes.
- `/renderer` -> Asynchronous `renderer_task` evaluating physics, debouncing, and drawing `final_r`, `final_g`, `final_b` targets.
- `/matter` -> Abstraction isolating only Matter initialization and protocol mappings. 
- `/storage` -> Non-Volatile Storage (NVS) implementations.
- `/led_driver` -> Low-level ESP-IDF `led_strip` and `rmt` interface logic.

## Hardware Setup

1. **ESP32 Target**: Standard ESP32, ESP32-C3, or similar.
2. **WS2812B Matrix**: Connect the data line of the WS2812B matrix. Be sure to use a 5V level-shifter if driving heavy distances. Be highly mindful of power constraints!

### Configuration
You no longer need to edit source C++ code to target your hardware. Run:
```bash
idf.py menuconfig
```
Navigate to **WS2812B Configuration** to tweak:
- `LED Strip GPIO Pin` (Default: `8`)
- `Maximum number of LEDs` (Default: `64`)
- `Maximum Current Limit (mA)` (Default: `1500`)
- `Global Brightness Cap` (Default: `255`)

## Prerequisites

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html) -> Environment exported. 
- [ESP-Matter SDK](https://github.com/espressif/esp-matter) -> Environment exported.

## Build and Flash

1. Configure your target environment (e.g. ESP32-C3):
   ```bash
   idf.py set-target esp32c3
   ```
2. Set your custom pins/power inside CMake's `menuconfig`
3. Hit Build, Flash, & Monitor:
   ```bash
   idf.py build flash monitor
   ```

## Commissioning

Open your Matter controller app (Apple Home, Google Home) and add a new accessory. The pairing code and QR code URL will be printed extensively in the serial terminal output upon your first boot.

## Licensing
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
