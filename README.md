# OctoPrint LEDs Statusbar

EN | [RU](README_RU.md)

A 3D-printer status indicator on an addressable LED strip (WS2812B), driven by an ESP8266. Polls the [OctoPrint](https://octoprint.org/) REST API and shows the current print state through color, progress-bar length, and blinking.

## What it looks like

| Status                                | Color | Behavior                                              |
| ------------------------------------- | ----- | ----------------------------------------------------- |
| Printing                              | Green | Growing progress bar, full brightness, edge highlight |
| Paused                                | Green | Progress bar, blinking, edge highlight                |
| Completed                             | Green | Full strip, blinking                                  |
| Idle                                  | Green | Full strip, dimmed, no blinking                       |
| Offline after error                   | Red   | Full strip, full brightness, blinking                 |
| Offline                               | Red   | Full strip, dimmed, no blinking                       |
| Not connected (OctoPrint unreachable) | Red   | Full strip, dimmed, blinking                          |

On boot, while connecting to Wi-Fi, the strip blinks blue outward from the center.

## Hardware

- ESP8266 (NodeMCU, Wemos D1 mini, etc.)
- Addressable WS2812B LED strip, 23 LEDs by default (see `NUM_LEDS` in `octoled.ino`)
- Strip data line connected to `D4`/`GPIO2` (see `LED_PIN` in `octoled.ino`)

## Project structure

| File          | Purpose                                                                                        |
| ------------- | ---------------------------------------------------------------------------------------------- |
| `octoled.ino` | Main logic: polling OctoPrint, rendering status on the strip                                   |
| `defines.h`   | Printer status enum and OctoPrint API response parsing                                         |
| `secrets.h`   | Your personal connection details (Wi-Fi, OctoPrint) — **not committed**                        |
| `errors.h`    | Validates that all `secrets.h` fields are filled in; stops the build with a clear error if not |
| `logger.h`    | Leveled logging (DEBUG/INFO/WARNING/ERROR)                                                     |

## Setup

1. Open `secrets.h` and fill in all the fields with your own values:

   - `SECRET_WIFI_SSID`, `SECRET_WIFI_PASSWORD` — your Wi-Fi network credentials.
   - `SECRET_OCTOPRINT_IP` — IP address of your OctoPrint server on the local network (4 comma-separated numbers).
   - `SECRET_OCTOPRINT_PORT` — OctoPrint port (usually `5000` or `80`).
   - `SECRET_OCTOPRINT_APIKEY` — Global API Key from OctoPrint: _Settings → API → Global API Key_.

2. If you forget to fill in a field, the build will stop with a clear compile error pointing to the specific field — that's `errors.h` doing its job.

3. Install dependencies in Arduino IDE / PlatformIO:

   - [FastLED](https://github.com/FastLED/FastLED)
   - [OctoPrintAPI](https://github.com/stefanboehm/OctoPrintAPI)
   - [ESP8266 board support](https://github.com/esp8266/Arduino)

4. Flash `octoled.ino` to the device.

## Logging

By default, `INFO`-level messages and above are printed to the Serial port (115200 baud). To change the level — for debugging, for example — add this before `#include "logger.h"` in `octoled.ino`:

```cpp
#define LOG_LEVEL LOG_LEVEL_DEBUG  // maximum detail
```

Available levels: `LOG_LEVEL_DEBUG`, `LOG_LEVEL_INFO`, `LOG_LEVEL_WARNING`, `LOG_LEVEL_ERROR`, `LOG_LEVEL_NONE` (disable entirely). Messages below the configured level don't end up in the compiled binary at all — it's a compile-time exclusion, not a runtime filter.

## Security

`secrets.h` contains your real Wi-Fi password and OctoPrint API key — don't publish it and don't commit it to git (it's already in `.gitignore`). If you share this project, send `secrets.h` separately from the repository.

## License

[MIT](LICENSE)
