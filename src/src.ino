#include "Arduino.h"
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>  

#include <NeoPixelBus.h>

#define colorSaturation 128

const uint8_t PIN_LED = 5;

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

HslColor hslRed(red);

WiFiManager wifiManager;
NeoPixelBus<NeoGrbFeature, NeoEsp8266BitBang800KbpsMethod> strip(1, PIN_LED);

void setup() {
    Serial.begin(115200);
    wifiManager.autoConnect("AButton");
    strip.Begin();
}

void loop() {
    Serial.println("loop");
    strip.SetPixelColor(0, hslRed);
    strip.Show();
    delay(1000);
}
