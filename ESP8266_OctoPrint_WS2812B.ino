#include <OctoPrintAPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <FastLED.h>
#include "defines.h"

#define LED_PIN 2                  
#define NUM_LEDS 23               
CRGB leds[NUM_LEDS];

const char* SSID = "RT-GPON-2580";          
const char* PASSWORD = "#10032023";
WiFiClient client;

const int OCTOPRINT_HTTP_PORT = 5000;
const String OCTOPRINT_APIKEY = "D3DAA0102D0748D8BEE59EBFD2432B2D";
IPAddress ip(192, 168, 0, 123);
OctoprintApi api(client, ip, OCTOPRINT_HTTP_PORT, OCTOPRINT_APIKEY); 

const unsigned long api_mtbs = 10000;

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  
  FastLED.clear();
  FastLED.show();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    static byte connection_retry = 0;
    delay(1000);
    Serial.print(".");
    leds[(NUM_LEDS / 2) - connection_retry] = CRGB(0,0,255);
    leds[(NUM_LEDS / 2) + connection_retry] = CRGB(0,0,255);
    FastLED.show();
    if (connection_retry == NUM_LEDS) ESP.reset();
    else connection_retry++;
  }
 
  Serial.println();
  Serial.print("WiFi connected \n IP address: ");
  Serial.println(WiFi.localIP());
  fill_solid(leds, NUM_LEDS, 0);
  FastLED.show();
}

void loop() {
  LEDsController(opState());
}

String opState() {
  static String state;
  static String completion;
  static unsigned long api_lasttime = 0;
  static byte octoErrors;
  if (millis() - api_lasttime > api_mtbs || api_lasttime == 0) {
    if (WiFi.status() == WL_CONNECTED) {
      if (api.getPrintJob()) {
        state = api.printJob.printerState;
        completion = String(float(api.printJob.progressCompletion));
        octoErrors = 0;
        if (api.printJob.printerState == PRINTING)    Serial.println("Printing: " + completion + "%");
        else if (api.printJob.printerState == PAUSED) Serial.println("Paused: " + completion + "%");
        else if (api.printJob.progressCompletion == 100 && api.printJob.printerState == OPERATIONAL) {
          Serial.println(COMPLITED);
          state =  COMPLITED;
        }
        else if (api.printJob.progressCompletion == 0 && api.printJob.printerState == OPERATIONAL) {
          Serial.println(IDLE);
          state =  IDLE;
        }
        else Serial.println(state);
      }
      else octoErrors++;
      api_lasttime = millis();
    }
  }
  return state + " " + completion;
}

void LEDsController(String opState) {
  const byte FULL_BRIGHTNESS = 255;
  const byte FULL_LEDSTRIP = 100;
  const byte RED = 0;
  const byte GREEN = 85;
  const byte PURPLE = 213;
  const int INDEX = opState.indexOf(' ');
  const String STATE = opState.substring(0,INDEX);
  const float COMPLETION = opState.substring(INDEX).toFloat();

  if      (STATE == PRINTING)             solid(STATE, COMPLETION, GREEN, FULL_BRIGHTNESS, false, true);
  else if (STATE == PAUSED)               solid(STATE, COMPLETION, GREEN, FULL_BRIGHTNESS, true, true);
  else if (STATE == COMPLITED)            solid(STATE, FULL_LEDSTRIP, GREEN, FULL_BRIGHTNESS, true, false);
  else if (STATE == IDLE)                 solid(STATE, FULL_LEDSTRIP, GREEN, 20, false, false);
  else if (STATE == OFFLINE_AFTER_ERROR)  solid(STATE, FULL_LEDSTRIP, RED, FULL_BRIGHTNESS, true, false);
  else if (STATE == OFFLINE)              solid(STATE, FULL_LEDSTRIP, RED, 20, false, false);
}

void solid(String state, float completion, byte color, byte brightness, bool blink, bool borders) {
  // Яркость оставшейся части статус-бара
  const byte SECONDARY_BRIGHTNESS = 20;

  static uint32_t timer;
  const int DELAY = (brightness < 255) ? 1000 : 500;
  const int PROGRESS = NUM_LEDS * (completion - 2) / 100;
  int sub_progress = (int(completion) - completion) * 100;
  if (sub_progress != 0){
    if(sub_progress < 50) sub_progress = -sub_progress;
    else completion--;
    sub_progress = sub_progress * 255 / 100;
  }

  if (!blink){
    fill_solid(leds, PROGRESS, CHSV(color, 255, brightness));
    if(completion < 100) leds[PROGRESS] = CHSV(color, 255, sub_progress);
    if(borders){
      leds[0] = CHSV(color + 40, 255, 255);
      leds[NUM_LEDS - 1] = leds[0];
    }
    FastLED.show();
  }
  else{
    if (millis() - timer < DELAY){
      fill_solid(leds, NUM_LEDS, 0);
      leds[0] = CHSV(color + 40, 255, 255);
      leds[NUM_LEDS - 1] = leds[0];
      if(borders){
        leds[0] = CHSV(color + 40, 255, 255);
        leds[NUM_LEDS - 1] = leds[0];
      }
    }
    else{
      fill_solid(leds, PROGRESS, CHSV(color, 255, 255));
      leds[PROGRESS] = CHSV(color, 255, sub_progress);
      if(borders){
        leds[0] = CHSV(color + 40, 255, 255);
        leds[NUM_LEDS - 1] = leds[0];
      }
    }
    FastLED.show();
    if (millis() - timer > DELAY*2) timer = millis();
  }
}