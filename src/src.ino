#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <PubSubClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <WiFiManager.h>  

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>
#include <Ticker.h>

#define BUTTON_PIN      4
#define LED_PIN         5
#define NUMPIXELS       1

#define LOG_SERIAL  Serial

#define debug_println(x)            LOG_SERIAL.println(x)
#define debug_printf(x, ...)        LOG_SERIAL.printf((x), __VA_ARGS__)
#define debug_print(x)              LOG_SERIAL.print(x)

const char CONFIG_FILE[] = "config.json";

//define your default values here, if there are different values in config.json, they are overwritten.
uint32_t mLedColor;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266HTTPUpdateServer httpUpdater;
ESP8266WebServer server(80);

Ticker mBlink;
Ticker mBlinkOnce;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

char mqttServer[64];
char mqttPort[5];
char mqttUser[16];
char mqttPass[16];

WiFiManagerParameter customMqttServer("server", "custom MQTT server", mqttServer, 64, "");
WiFiManagerParameter customMqttPort("port", "custom MQTT port", mqttPort, 5, "");
WiFiManagerParameter customMqttUser("user", "custom MQTT user", mqttUser, 16, "");
WiFiManagerParameter customMqttPass("pass", "custom MQTT password", mqttPass, 16, "");

//flag for saving data
bool shouldSaveConfig = false;
char clientId[32];

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

    File configFile = SPIFFS.open(CONFIG_FILE, "r");
    size_t size = configFile.size();
    // Allocate a buffer to store contents of the file.
    std::unique_ptr<char[]> buf(new char[size]);

    configFile.readBytes(buf.get(), size);
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(buf.get());
    json.printTo(Serial);
    configFile.close();

    if (json.success()) {
        Serial.println("\nparsed json");
        strcpy(mqttServer, json["server"]);
        strcpy(mqttPort, json["port"]);
        strcpy(mqttUser, json["user"]);
        strcpy(mqttPass, json["pass"]);
        Serial.println("Parsed config");
    }
}

void saveConfig() {
    strcpy(mqttServer, customMqttServer.getValue());
    strcpy(mqttPort, customMqttPort.getValue());
    strcpy(mqttUser, customMqttUser.getValue());
    strcpy(mqttPass, customMqttPass.getValue());

    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["server"] = mqttServer;
    json["port"] = mqttPort;
    json["user"] = mqttUser;
    json["pass"] = mqttPass;

    File configFile = SPIFFS.open(CONFIG_FILE, "w+");
    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    shouldSaveConfig = false;
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

bool mqttConnect() {
    debug_print("Attempting MQTT connection...");

    // Attempt to connect
    if (mqttUser && mqttPass) {
        mqtt.connect(MQTT::Connect(clientId)
                 .set_auth(mqttUser, mqttPass));
    } else {
        mqtt.connect(clientId);
    }

    if (mqtt.connected()) {
        debug_println("connected");
    } else {
        delay(1000);
    }
}

void mqttCallback(const MQTT::Publish& pub) {
    Serial.print(pub.topic());
    Serial.print(" => ");
    Serial.println(pub.payload_string());

    // response should be json string 
    // such as {"id": "AB1234", "r": 255, "g": 0, "b":128, "delay": 2000}
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(pub.payload_string());

    String chipId = String(ESP.getChipId(), DEC);
    if (chipId.equals(String(json["id"].as<char *>()))) {
        mLedColor = pixels.Color(json["r"], json["g"], json["b"]);
    }

    if (json["delay"] > 0) {
        delay(json["delay"]);
        mLedColor = pixels.Color(0, 0, 0);
        // Useless, Put the enable pin for ESP8266 to LOW, turn off whole button
        // digitalWrite(EN_PIN, LOW);
    }
}

void buttonPressed() {
    detachInterrupt(BUTTON_PIN);
    // button pressed

    if (strlen(mqttServer)) {
        Serial.print("request server: ");
        Serial.println(mqttServer);
        uint16_t port = String(mqttPort).toInt();
        mqtt.set_server(String(mqttServer), port);
        mqttConnect();
        char message[128];
        sprintf(message, "{\"id\": \"%s\", \"state\": \"pushed\"}", clientId); 

        if(!mqtt.publish("button", message)) {
            Serial.printf("[MQTT] Publish failed\n");
        } else {
            mqtt.set_callback(mqttCallback);
            mqtt.subscribe("button/led");
            mLedColor = pixels.Color(0, 255, 0);
        }

        delay(2000);
        setLed(0);
        // turn off
        // digitalWrite(EN_PIN, LOW);
    }

    attachInterrupt(BUTTON_PIN, buttonPressed, RISING);
}

void setup() {
    // Boot up
    pinMode(BUTTON_PIN, INPUT);
    attachInterrupt(BUTTON_PIN, buttonPressed, RISING);
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
    wifiManager.addParameter(&customMqttServer);
    wifiManager.addParameter(&customMqttPort);
    wifiManager.addParameter(&customMqttUser);
    wifiManager.addParameter(&customMqttPass);

    pixels.begin();
    mLedColor = pixels.Color(255, 0, 0);
    mBlink.attach(1, blinkHandler);
    wifiManager.autoConnect("AButton");
    mLedColor = pixels.Color(0, 0, 255);

    if (shouldSaveConfig) {
        saveConfig();
    }

    String id = "AB";
    id += String(ESP.getChipId(), DEC);
    strcpy(clientId, id.c_str());

    Serial.print("local ip: ");
    Serial.println(WiFi.localIP());

    // should not go here or button hold 
    // factory reset
    mLedColor = pixels.Color(255, 0, 0);
    delay(1500);
    factoryReset(wifiManager);
    wifiManager.autoConnect("AButton");
    if (shouldSaveConfig) {
        saveConfig();
    }
    Serial.println("\r\nstart OTA server\r\n");
    httpUpdater.setup(&server);
    server.begin();
}

void loop() {
    Serial.println("loop");
    server.handleClient();
    delay(1000);
}
