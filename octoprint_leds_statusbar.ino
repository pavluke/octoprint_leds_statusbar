/**
 * @file octoled.ino
 * @brief 3D-printer status indicator on an addressable LED strip (WS2812B).
 *
 * Polls the OctoPrint REST API every api_mtbs milliseconds and renders the
 * current print state on the strip: color depends on the status (printing/
 * paused/done/idle/error), the length of the lit section reflects progress,
 * and blinking indicates whether the status needs attention.
 *
 * Configuration (Wi-Fi, OctoPrint IP/port/API key) lives in secrets.h —
 * fill in the fields there with your own values. errors.h checks that all
 * fields are filled in and stops the build with a clear error if not.
 */

#include <OctoPrintAPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <FastLED.h>
#include "defines.h"
#include "secrets.h"
#include "errors.h"
#include "logger.h"

/** Number of LEDs in the strip. */
#define NUM_LEDS 23

/** Data pin the strip is connected to. */
#define LED_PIN 2

/** Brightness (0-255) for statuses that don't need attention (Idle, Offline). */
const byte LOW_BRIGHTNESS = 20;

/** Pixel buffer for the strip, used by FastLED to push frames. */
CRGB leds[NUM_LEDS];

/** TCP client for HTTP requests to OctoPrint. */
WiFiClient client;

/** OctoPrint REST API client, configured with values from secrets.h. */
OctoprintApi api(client, OCTOPRINT_IP, OCTOPRINT_HTTP_PORT, OCTOPRINT_APIKEY);

/** Interval between OctoPrint API polls, ms. */
const unsigned long api_mtbs = 20000;

/** Full brightness (used for statuses that need attention). */
const byte FULL_BRIGHTNESS = 255;

/** Placeholder "percent" for statuses without real progress — lights the whole strip. */
const byte FULL_LEDSTRIP = 100;

/** Red hue value in the CHSV palette (FastLED). */
const byte RED = 0;

/** Green hue value in the CHSV palette (FastLED). */
const byte GREEN = 85;

/** Purple hue value in the CHSV palette (FastLED, unused for now, kept for future statuses). */
const byte PURPLE = 213;

/**
 * @brief Visual style for rendering one printer status on the strip.
 */
struct LedStyle {
  byte color;       ///< Hue for CHSV (see RED/GREEN/PURPLE above).
  byte brightness;  ///< Brightness of the lit section (0-255).
  bool blink;       ///< Whether to blink (true for statuses that need attention).
  bool borders;     ///< Whether to highlight the edge LEDs with an accent color.
};

/**
 * Style table indexed by status. The index is the PrinterStatus enum value
 * (see defines.h); the order here must match the order in the enum.
 */
const LedStyle STYLES[] = {
  /* PRINTING            */ {GREEN, FULL_BRIGHTNESS, false, true},
  /* PAUSED              */ {GREEN, FULL_BRIGHTNESS, true,  true},
  /* COMPLETED           */ {GREEN, FULL_BRIGHTNESS, true,  false},
  /* IDLE                */ {GREEN, LOW_BRIGHTNESS,  false, false},
  /* OFFLINE_AFTER_ERROR */ {RED,   FULL_BRIGHTNESS, true,  false},
  /* OFFLINE             */ {RED,   LOW_BRIGHTNESS,  false, false},
  /* NOT_CONNECTED       */ {RED,   LOW_BRIGHTNESS,  true,  false},
};

/**
 * @brief Device initialization: LED strip, Wi-Fi connection.
 *
 * While connecting to Wi-Fi, blinks blue outward from the center of the
 * strip — two LEDs per second of waiting. If the connection doesn't succeed
 * within NUM_LEDS seconds, reboots the ESP (ESP.reset()).
 */
void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);

  FastLED.clear();
  FastLED.show();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  LOG_INFO(String("Connecting to ") + WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    static byte connection_retry = 0;
    delay(1000);
    Serial.print(".");  // visual waiting ticker, not a log message
    leds[(NUM_LEDS / 2) - connection_retry] = CRGB(0, 0, 255);
    leds[(NUM_LEDS / 2) + connection_retry] = CRGB(0, 0, 255);
    FastLED.show();
    if (connection_retry == NUM_LEDS) {
      LOG_ERROR("WiFi failed to connect within the allotted time, rebooting ESP");
      ESP.reset();
    }
    else connection_retry++;
  }

  Serial.println();
  LOG_INFO(String("WiFi connected, IP address: ") + WiFi.localIP().toString());
  fill_solid(leds, NUM_LEDS, 0);
  FastLED.show();
}

/**
 * @brief Main loop: poll the printer status and update the strip.
 *
 * delay(50) caps the iteration rate — without it loop() would spin tens of
 * thousands of times per second for no benefit (the OctoPrint poll only
 * happens once every api_mtbs anyway), and on the ESP8266 a bare busy-loop
 * also gets in the way of the background Wi-Fi stack processing.
 */
void loop() {
  float completion;
  PrinterStatus status = getPrinterStatus(completion);
  LEDsController(status, completion);
  delay(50);
}

/**
 * @brief Polls OctoPrint and returns the current printer status.
 *
 * The actual API request happens no more than once every api_mtbs
 * milliseconds — between polls, the function simply returns the last known
 * status. On failure, retries up to 3 times (with a 300ms pause between
 * attempts) before treating the printer as unreachable
 * (PrinterStatus::NOT_CONNECTED) — this guards against false positives from
 * a one-off network hiccup.
 *
 * @param[out] completion Current print job progress (0-100). Not
 *                         meaningful for statuses without an active print
 *                         job (Idle, Offline, etc.).
 * @return The current printer status.
 */
PrinterStatus getPrinterStatus(float& completion) {
  static PrinterStatus status = PrinterStatus::NOT_CONNECTED;
  static float lastCompletion = 0;
  static unsigned long api_lasttime = 0;
  static byte octoErrors = 0;

  if (millis() - api_lasttime > api_mtbs || api_lasttime == 0) {
    if (WiFi.status() == WL_CONNECTED) {
      const byte MAX_RETRIES = 3;
      bool success = false;

      for (byte attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        if (api.getPrintJob()) {
          success = true;
          break;
        }
        LOG_WARNING(String("getPrintJob() failed, attempt ") + attempt + "/" + MAX_RETRIES);
        if (attempt < MAX_RETRIES) delay(300);
      }

      if (success) {
        lastCompletion = float(api.printJob.progressCompletion);
        status = parsePrinterStatus(api.printJob.printerState, lastCompletion);
        octoErrors = 0;

        if (status == PrinterStatus::PRINTING || status == PrinterStatus::PAUSED) {
          LOG_INFO(String(printerStatusToString(status)) + ": " + String(lastCompletion) + "%");
        } else {
          LOG_DEBUG(printerStatusToString(status));
        }
      } else {
        octoErrors++;
        status = PrinterStatus::NOT_CONNECTED;
        LOG_ERROR(String("getPrintJob() failed after ") + MAX_RETRIES + " attempts, octoErrors=" + octoErrors);
      }
      api_lasttime = millis();
    }
  }

  completion = lastCompletion;
  return status;
}

/**
 * @brief Picks the display style for a status and triggers the rendering.
 *
 * For statuses without a meaningful percentage (Completed, Idle, Offline,
 * Offline after error, Not connected), substitutes FULL_LEDSTRIP so the
 * whole strip is used for the indication instead of just part of it.
 *
 * @param status     The current printer status.
 * @param completion Print job progress (only used for Printing/Paused;
 *                    ignored and replaced with FULL_LEDSTRIP for all other
 *                    statuses).
 */
void LEDsController(PrinterStatus status, float completion) {
  const LedStyle& style = STYLES[static_cast<int>(status)];

  float displayCompletion = (status == PrinterStatus::COMPLETED || status == PrinterStatus::IDLE
                              || status == PrinterStatus::OFFLINE_AFTER_ERROR || status == PrinterStatus::OFFLINE
                              || status == PrinterStatus::NOT_CONNECTED)
                             ? FULL_LEDSTRIP
                             : completion;

  solid(displayCompletion, style.color, style.brightness, style.blink, style.borders);
}

/**
 * @brief Draws a progress bar on the strip with the given color/brightness.
 *
 * The length of the lit section is proportional to completion (with a
 * smooth transition on the boundary LED via the fractional part of the
 * percentage — sub_progress simulates "sub-pixel" precision for a single
 * LED).
 *
 * If blink=true, alternates between a fully empty frame (or just the
 * borders) and a fully lit one, with phase duration DELAY (1000ms for
 * dimmed statuses, 500ms for full-brightness statuses).
 *
 * @param completion Progress percentage (0-100), determines the bar length.
 * @param color      Hue for the CHSV color of the lit section.
 * @param brightness Brightness of the lit section (0-255).
 * @param blink      Whether to blink (true) or stay solid (false).
 * @param borders    Whether to highlight the edge LEDs with an accent color
 *                    (hue color+40, full brightness).
 */
void solid(float completion, byte color, byte brightness, bool blink, bool borders) {

  static uint32_t timer;
  const int DELAY = (brightness < 255) ? 1000 : 500;
  const int PROGRESS = NUM_LEDS * (completion - 2) / 100;
  int sub_progress = (int(completion) - completion) * 100;
  if (sub_progress != 0) {
    if (sub_progress < 50) sub_progress = -sub_progress;
    else completion--;
    sub_progress = sub_progress * 255 / 100;
  }

  if (!blink) {
    fill_solid(leds, PROGRESS, CHSV(color, 255, brightness));
    if (completion < 100) leds[PROGRESS] = CHSV(color, 255, sub_progress);
    if (borders) {
      leds[0] = getBorderColor(color);
      leds[NUM_LEDS - 1] = leds[0];
    }
    FastLED.show();
  } else {
    if (millis() - timer < DELAY) {
      fill_solid(leds, NUM_LEDS, 0);
      if (borders) {
        leds[0] = getBorderColor(color);
        leds[NUM_LEDS - 1] = leds[0];
      }
    } else {
      fill_solid(leds, PROGRESS, CHSV(color, 255, brightness));
      leds[PROGRESS] = CHSV(color, 255, sub_progress);
      if (borders) {
        leds[0] = getBorderColor(color);
        leds[NUM_LEDS - 1] = leds[0];
      }
    }
    FastLED.show();
    if (millis() - timer > DELAY * 2) timer = millis();
  }
}

/**
 * @brief Computes the accent border color for a given base color.
 *
 * Shifts the hue by +40 relative to the base color — used to highlight the
 * edge LEDs of the strip (border) at full brightness.
 *
 * @param color Hue of the status's base color.
 * @return CHSV border color (same hue+40, full saturation and brightness).
 */
CHSV getBorderColor(byte color) {
  return CHSV(color + 40, 255, 255);
}
