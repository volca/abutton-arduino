#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>  

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Ticker.h>

#define LED_PIN         5
#define NUMPIXELS       1

const char CONFIG_FILE[] = "config.json";

//define your default values here, if there are different values in config.json, they are overwritten.
char customUrl[128];

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266HTTPUpdateServer httpUpdater;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
	Serial.println("Should save config");
	shouldSaveConfig = true;
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

void setup() {
    Serial.begin(115200);
    Serial.println("Start AButton");
    Serial.println("Loading config");
    loadConfig();

    WiFiManager wifiManager;

	//set config save notify callback
	wifiManager.setSaveConfigCallback(saveConfigCallback);
    wifiManager.setAPStaticIPConfig(IPAddress(192,168,1,1), IPAddress(192,168,1,1), IPAddress(255,255,255,0));

    //reset settings - for testing
    //wifiManager.resetSettings();

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter customUrlParam("url", "custom url", customUrl, 128, "");
    wifiManager.addParameter(&customUrlParam);

    pixels.begin();
    wifiManager.autoConnect("AButton");

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
}

void loop() {
    Serial.println("loop");
    pixels.setPixelColor(0, pixels.Color(255,0,0));
    pixels.show();
    delay(1000);
}
