#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>  

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Ticker.h>

#define BTN_PIN         4
#define LED_PIN         5
#define NUMPIXELS       1

const char CONFIG_FILE[] = "config.json";

//define your default values here, if there are different values in config.json, they are overwritten.
char customUrl[128];

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266HTTPUpdateServer httpUpdater;
Ticker mBlink;
Ticker mBlinkOnce;
uint32_t mLedColor;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
    Serial.println("Should save config");
    shouldSaveConfig = true;
}

void factoryReset(WiFiManager &wifiManager) {
    wifiManager.resetSettings();
    SPIFFS.remove(CONFIG_FILE);
}

void loadConfig() {
    SPIFFS.begin();
    if (!SPIFFS.exists(CONFIG_FILE)) { 
        Serial.println("Could not find config file");
        return;
    }

    File configFile = SPIFFS.open("/config.json", "r");
    size_t size = configFile.size();
    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    configFile.readBytes(buf.get(), size);
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(buf.get());
    json.printTo(Serial);

    if (json.success()) {
        Serial.println("\nparsed json");
        strcpy(customUrl, json["url"]);
        Serial.println("Parsed config");
    }
}

void setLed(int state) {
    if (state) {
        pixels.setPixelColor(0, mLedColor);
    } else {
        pixels.setPixelColor(0, pixels.Color(0,0,0));
    }
    pixels.show();
}

void blinkHandler() {
    setLed(1);
    mBlinkOnce.once_ms(25, setLed, 0);
}

void setup() {
    pinMode(BTN_PIN, OUTPUT);
    digitalWrite(BTN_PIN, HIGH);
    Serial.begin(115200);
    Serial.println("\r\nStart AButton");
    loadConfig();

    WiFiManager wifiManager;

    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setAPStaticIPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter customUrlParam("url", "custom url", customUrl, 128, "");
    wifiManager.addParameter(&customUrlParam);

    pixels.begin();
    mLedColor = pixels.Color(255, 0, 0);
    mBlink.attach(1, blinkHandler);
    wifiManager.autoConnect("AButton");
    mLedColor = pixels.Color(0, 0, 255);

    if (shouldSaveConfig) {
        strcpy(customUrl, customUrlParam.getValue());

        Serial.println("saving config");
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.createObject();
        json["url"] = customUrl;

        File configFile = SPIFFS.open(CONFIG_FILE, "w");
        json.printTo(Serial);
        Serial.println("");
        json.printTo(configFile);
        configFile.close();
    }

    ESP8266WebServer server(80);
    httpUpdater.setup(&server);

    Serial.print("local ip: ");
    Serial.println(WiFi.localIP());

    HTTPClient http;
    http.begin(customUrl);
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println(payload);
            mLedColor = pixels.Color(0, 255, 0);
        }
        http.end();
        // turn off
        setLed(0);
        digitalWrite(BTN_PIN, LOW);
        delay(2000);
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        http.end();
    }

    // should not go here
    factoryReset(wifiManager);
    wifiManager.autoConnect("AButton");
}

void loop() {
    Serial.println("loop");
    delay(1000);
}
