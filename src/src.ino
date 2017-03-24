#include "Arduino.h"
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>  

#include <Adafruit_NeoPixel.h>

#define colorSaturation 128

#define LED_PIN         5
#define NUMPIXELS       1

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiManager wifiManager;

void setup() {
    Serial.begin(115200);
    wifiManager.autoConnect("AButton");
    pixels.begin();
}

void loop() {
    Serial.println("loop");
    pixels.setPixelColor(0, pixels.Color(255,0,0));
    pixels.show();
    delay(1000);
}
