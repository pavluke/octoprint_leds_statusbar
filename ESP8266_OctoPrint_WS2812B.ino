#include <OctoPrintAPI.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <FastLED.h>

#define LED_PIN 2                  
#define NUM_LEDS 23               
CRGB leds[NUM_LEDS];

const char* ssid = "Pavluke";          
const char* password = "AL0ngPassw0rd";
WiFiClient client;

IPAddress ip(192, 168, 0, 123);
//char* octoprint_host = "octopi.example.com";  // Or your hostname. Comment out one or the other.
const int octoprint_httpPort = 5000;  //If you are connecting through a router this will work, but you need a random port forwarded to the OctoPrint server from your router. Enter that port here if you are external
String octoprint_apikey = "D3DAA0102D0748D8BEE59EBFD2432B2D"; //See top of file or GIT Readme about getting API key

unsigned long api_mtbs = 10000;
unsigned long api_lasttime = 0;
byte connection_retry = 0;
byte point = 0;
byte octoErrors = 0;

long printed_timeout = 3600000; //60 mins in milliseconds - timeout after printing completed, to clear strip
long printed_timeout_timer = printed_timeout;

// Use one of the following:
//OctoprintApi api; //Be sure to call init in setup. 
OctoprintApi api(client, ip, octoprint_httpPort, octoprint_apikey);               //If using IP address
//OctoprintApi api(client, octoprint_host, octoprint_httpPort, octoprint_apikey); //If using hostname. Comment out one or the other.

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
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    leds[(NUM_LEDS/2) - connection_retry] = CRGB(0,0,255);
    leds[(NUM_LEDS/2) + connection_retry] = CRGB(0,0,255);
    //delay(100);
    FastLED.show();
    if (connection_retry == NUM_LEDS) {
      ESP.reset();
    }
    else{
      connection_retry++;
    }
  }
 
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  fill_solid(leds, NUM_LEDS, 0);
  FastLED.show();
}

void loop() {
  LEDsController(opState());
}

String opState(){
  static String state;
  static String completion;
  if (millis() - api_lasttime > api_mtbs || api_lasttime == 0) {
    if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
      if (api.getPrintJob()) { //Get the print job API endpoint
        state = api.printJob.printerState;
        completion = String(float(api.printJob.progressCompletion));
        octoErrors = 0;
        if (api.printJob.printerState == "Printing") {
          Serial.println("Printing: " + completion + "%");
        }
        else if (api.printJob.printerState == "Paused"){
          Serial.println("Paused: " + completion + "%");
        }
        else if (api.printJob.progressCompletion == 100 && api.printJob.printerState == "Operational")
        {
          Serial.println("Complited");
          state =  "Complited";
        }
        else if (api.printJob.progressCompletion == 0 && api.printJob.printerState == "Operational"){
          Serial.println("Idle");
          state =  "Idle";
        }
        else if (api.printJob.printerState == "Offline after error"){
          Serial.println(state);
        }
        else if (api.printJob.printerState == "Offline"){
          Serial.println(state);
        }
      }
      else{
        octoErrors++;
        if (octoErrors == 2){
        }
      }
      api_lasttime = millis();  //Set api_lasttime to current milliseconds run
    }
  }
  return state + ' ' + completion;
}

void LEDsController(String opState){
  const byte FULL_BRIGHTNESS = 255;
  const byte FULL_LEDSTRIP = 100;
  const byte RED = 0;
  const byte GREEN = 85;
  const byte PURPLE = 213;
  int index = opState.indexOf(' ');
  String state = opState.substring(0,index);
  float completion = opState.substring(index).toFloat();

  if (state == "Printing")                  solid(state, completion, GREEN, FULL_BRIGHTNESS, false, true);
  else if (state == "Paused")               solid(state, completion, GREEN, FULL_BRIGHTNESS, true, true);
  else if (state == "Complited")            solid(state, FULL_LEDSTRIP, GREEN, FULL_BRIGHTNESS, true, false);
  else if (state == "Idle")                 solid(state, FULL_LEDSTRIP, GREEN, 20, false, false);
  else if (state == "Offline after error")  solid(state, FULL_LEDSTRIP, RED, FULL_BRIGHTNESS, true, false);
  else if (state == "Offline")              solid(state, FULL_LEDSTRIP, RED, 20, false, false);
}

void solid(String state, float completion, byte color, byte brightness, bool blink, bool borders){
  // Яркость оставшейся части статус-бара
  byte SECONDARY_BRIGHTNESS = 20;

  static uint32_t timer;
  int DELAY = (brightness < 255) ? 1000 : 500;
  int progress = NUM_LEDS * (completion-2) / 100;
  int sub_progress = (int(completion) - completion) * 100;
  if (sub_progress!=0){
    if(sub_progress<50) sub_progress = -sub_progress;
    else completion-=1;
    sub_progress = sub_progress*255/100;
  }

  if (!blink){
    //fill_solid(leds, NUM_LEDS, CHSV(color, 255, SECONDARY_BRIGHTNESS));
    fill_solid(leds, progress, CHSV(color, 255, brightness));
    if(completion < 100) leds[progress] = CHSV(color, 255, sub_progress);
    if(borders){
      leds[0] = CHSV(color+40, 255, 255);
      leds[NUM_LEDS-1] = leds[0];
    }
    FastLED.show();
  }
  else{
    if (millis() - timer < DELAY){
      fill_solid(leds, NUM_LEDS, 0);
      leds[0] = CHSV(color+40, 255, 255);
      leds[NUM_LEDS-1] = leds[0];
      if(borders){
        leds[0] = CHSV(color+40, 255, 255);
        leds[NUM_LEDS-1] = leds[0];
      }
    }
    else{
      //fill_solid(leds+progress, NUM_LEDS, CHSV(color, 255, SECONDARY_BRIGHTNESS));
      fill_solid(leds, progress, CHSV(color, 255, 255));
      leds[progress] = CHSV(color, 255, sub_progress);
      if(borders){
        leds[0] = CHSV(color+40, 255, 255);
        leds[NUM_LEDS-1] = leds[0];
      }
    }
    FastLED.show();
    if (millis() - timer > DELAY*2) timer = millis();
  }
}