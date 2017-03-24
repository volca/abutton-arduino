#include "Arduino.h"
#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>  

#include <Adafruit_NeoPixel.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <Ticker.h>

#define LED_PIN         5
#define NUMPIXELS       1

const char CONFIG_FILE[] = "config.json";

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
WiFiManager wifiManager;



void loadConfig() {
    if (!SPIFFS.exists(CONFIG_FILE)) { 
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Loading config");
    loadConfig();

    //reset settings - for testing
    wifiManager.resetSettings();

    WiFiManagerParameter customUrl("url", "custom url", "", 40, "");
    wifiManager.addParameter(&customUrl);

    wifiManager.setAPStaticIPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));
    wifiManager.autoConnect("AButton");

    pixels.begin();
}

void loop() {
    Serial.println("loop");
    pixels.setPixelColor(0, pixels.Color(255,0,0));
    pixels.show();
    delay(1000);
}
