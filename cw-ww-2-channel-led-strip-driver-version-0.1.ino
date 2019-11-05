#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <PubSubClient.h>
#include <WiFiManager.h>

#define MIN_MIREDS 153
#define MAX_MIREDS 500

char configEsp8266Id[64] = "esp8266-1";

char configMqttBroker[64] = "hassio.local";
char configMqttUser[64] = "esp8266-1";
char configMqttPassword[64] = "esp8266-1";

char nameLed1[64] = "ESP8266 Nr. 1, LED Nr. 1";
char nameLed2[64] = "ESP8266 Nr. 1, LED Nr. 2";

bool saveConfig = false;

bool stateLed1 = false, stateLed2 = false;
int brightnessLed1 = 255, brightnessLed2 = 255;
int colorTempLed1 = (MIN_MIREDS + MAX_MIREDS) / 2, colorTempLed2 = (MIN_MIREDS + MAX_MIREDS) / 2;

int analogWrite0 = 0, analogWrite1 = 0, analogWrite2 = 0, analogWrite3 = 0;

String configTopicLed1, configTopicLed2;
String commandTopicLed1, commandTopicLed2;
String stateTopicLed1, stateTopicLed2;

WiFiClient wiFiClient;
PubSubClient pubSubClient(wiFiClient);

void setup() {
  if (SPIFFS.begin()) {
    if (SPIFFS.exists("/config.json")) {
      File file_config = SPIFFS.open("/config.json", "r");

      if (file_config) {
        StaticJsonDocument<JSON_OBJECT_SIZE(16)> staticJsonDocument_config;
        deserializeJson(staticJsonDocument_config, file_config);

        strcpy(configEsp8266Id, staticJsonDocument_config["esp8266_id"]);

        strcpy(configMqttBroker, staticJsonDocument_config["mqtt_broker"]);
        strcpy(configMqttUser, staticJsonDocument_config["mqtt_user"]);
        strcpy(configMqttPassword, staticJsonDocument_config["mqtt_password"]);

        file_config.close();
      }
    }

    if (SPIFFS.exists("/led1.json")) {
      File fileLed1 = SPIFFS.open("/led1.json", "r");
      if (fileLed1) {
        StaticJsonDocument<JSON_OBJECT_SIZE(16)> staticJsonDocumentLed1;
        deserializeJson(staticJsonDocumentLed1, fileLed1);

        strcpy(nameLed1, staticJsonDocumentLed1["name"]);

        stateLed1 = staticJsonDocumentLed1["state"];
        brightnessLed1 = staticJsonDocumentLed1["brightness"];
        colorTempLed1 = staticJsonDocumentLed1["color_temp"];

        fileLed1.close();
      }
    }

    if (SPIFFS.exists("/led2.json")) {
      File fileLed2 = SPIFFS.open("/led2.json", "r");
      if (fileLed2) {
        StaticJsonDocument<JSON_OBJECT_SIZE(16)> staticJsonDocumentLed2;
        deserializeJson(staticJsonDocumentLed2, fileLed2);

        strcpy(nameLed2, staticJsonDocumentLed2["name"]);

        stateLed2 = staticJsonDocumentLed2["state"];
        brightnessLed2 = staticJsonDocumentLed2["brightness"];
        colorTempLed2 = staticJsonDocumentLed2["color_temp"];

        fileLed2.close();
      }
    }
  }

  pinMode(0, OUTPUT);
  analogWrite(0, 0);

  pinMode(1, OUTPUT);
  analogWrite(1, 0);

  pinMode(2, OUTPUT);
  analogWrite(2, 0);

  pinMode(3, OUTPUT);
  analogWrite(3, 0);

  led1();
  led2();

  WiFiManagerParameter wiFiManagerParameter_configEsp8266Id("esp8266_id", "ESP8266 ID", configEsp8266Id, 64);

  WiFiManagerParameter wiFiManagerParameter_configMqttBroker("mqtt_broker", "MQTT broker", configMqttBroker, 64);
  WiFiManagerParameter wiFiManagerParameter_configMqttUser("mqtt_user", "MQTT User", configMqttUser, 64);
  WiFiManagerParameter wiFiManagerParameter_configMqttPassword("mqtt_password", "MQTT password", configMqttPassword, 64);

  WiFiManagerParameter wiFiManagerParameter_nameLed1("nameLed1", "LED Nr. 1 name", nameLed1, 64);
  WiFiManagerParameter wiFiManagerParameter_nameLed2("nameLed2", "LED Nr. 2 name", nameLed2, 64);

  WiFiManager wifiManager;

  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.addParameter(&wiFiManagerParameter_configEsp8266Id);

  wifiManager.addParameter(&wiFiManagerParameter_configMqttBroker);
  wifiManager.addParameter(&wiFiManagerParameter_configMqttUser);
  wifiManager.addParameter(&wiFiManagerParameter_configMqttPassword);

  wifiManager.addParameter(&wiFiManagerParameter_nameLed1);
  wifiManager.addParameter(&wiFiManagerParameter_nameLed2);

  wifiManager.autoConnect(configEsp8266Id);

  WiFi.hostname(configEsp8266Id);

  strcpy(configEsp8266Id, wiFiManagerParameter_configEsp8266Id.getValue());

  strcpy(configMqttBroker, wiFiManagerParameter_configMqttBroker.getValue());
  strcpy(configMqttUser, wiFiManagerParameter_configMqttUser.getValue());
  strcpy(configMqttPassword, wiFiManagerParameter_configMqttPassword.getValue());

  strcpy(nameLed1, wiFiManagerParameter_nameLed1.getValue());
  strcpy(nameLed2, wiFiManagerParameter_nameLed2.getValue());

  if (saveConfig) {
    StaticJsonDocument<JSON_OBJECT_SIZE(16)> staticJsonDocument_config;

    staticJsonDocument_config["esp8266_id"] = configEsp8266Id;

    staticJsonDocument_config["mqtt_broker"] = configMqttBroker;
    staticJsonDocument_config["mqtt_user"] = configMqttUser;
    staticJsonDocument_config["mqtt_password"] = configMqttPassword;

    File file_config = SPIFFS.open("/config.json", "w");

    if (file_config) {
      serializeJson(staticJsonDocument_config, file_config);
    }

    file_config.close();

    StaticJsonDocument<JSON_OBJECT_SIZE(16)> staticJsonDocumentLed1;

    staticJsonDocumentLed1["name"] = nameLed1;

    staticJsonDocumentLed1["state"] = stateLed1;
    staticJsonDocumentLed1["brightness"] = brightnessLed1;
    staticJsonDocumentLed1["color_temp"] = colorTempLed1;

    File fileLed1 = SPIFFS.open("/led1.json", "w");

    if (fileLed1) {
      serializeJson(staticJsonDocumentLed1, fileLed1);
    }

    fileLed1.close();

    StaticJsonDocument<JSON_OBJECT_SIZE(16)> staticJsonDocumentLed2;

    staticJsonDocumentLed2["name"] = nameLed2;

    staticJsonDocumentLed2["state"] = stateLed2;
    staticJsonDocumentLed2["brightness"] = brightnessLed2;
    staticJsonDocumentLed2["color_temp"] = colorTempLed2;

    File fileLed2 = SPIFFS.open("/led2.json", "w");

    if (fileLed2) {
      serializeJson(staticJsonDocumentLed2, fileLed2);
    }

    fileLed2.close();
  }

  configTopicLed1 += "homeassistant/light/";
  configTopicLed1 += configEsp8266Id;
  configTopicLed1 += "/led-1/config";

  configTopicLed2 += "homeassistant/light/";
  configTopicLed2 += configEsp8266Id;
  configTopicLed2 += "/led-2/config";

  commandTopicLed1 += "homeassistant/light/";
  commandTopicLed1 += configEsp8266Id;
  commandTopicLed1 += "/led-1/command";

  commandTopicLed2 += "homeassistant/light/";
  commandTopicLed2 += configEsp8266Id;
  commandTopicLed2 += "/led-2/command";

  stateTopicLed1 += "homeassistant/light/";
  stateTopicLed1 += configEsp8266Id;
  stateTopicLed1 += "/led-1/state";

  stateTopicLed2 += "homeassistant/light/";
  stateTopicLed2 += configEsp8266Id;
  stateTopicLed2 += "/led-2/state";
}

void loop() {
  pubSubClient.setServer(configMqttBroker, 1883);
  pubSubClient.setCallback(callback);

  if (!pubSubClient.connected()) {
    if (pubSubClient.connect(configEsp8266Id, configMqttUser, configMqttPassword)) {
      StaticJsonDocument<JSON_OBJECT_SIZE(16)> staticJsonDocumentLed1;

      staticJsonDocumentLed1["name"] = nameLed1;
      staticJsonDocumentLed1["platform"] = "mqtt";
      staticJsonDocumentLed1["schema"] = "json";
      staticJsonDocumentLed1["command_topic"] = commandTopicLed1.c_str();
      staticJsonDocumentLed1["state_topic"] = stateTopicLed1.c_str();
      staticJsonDocumentLed1["brightness"] = "true";
      staticJsonDocumentLed1["color_temp"] = "true";

      String string1Led1;

      serializeJson(staticJsonDocumentLed1, string1Led1);

      pubSubClient.publish(configTopicLed1.c_str(), string1Led1.c_str(), true);
      pubSubClient.subscribe(commandTopicLed1.c_str());

      publish();

      StaticJsonDocument<JSON_OBJECT_SIZE(16)> staticJsonDocumentLed2;

      staticJsonDocumentLed2["name"] = nameLed2;
      staticJsonDocumentLed2["platform"] = "mqtt";
      staticJsonDocumentLed2["schema"] = "json";
      staticJsonDocumentLed2["command_topic"] = commandTopicLed2.c_str();
      staticJsonDocumentLed2["state_topic"] = stateTopicLed2.c_str();
      staticJsonDocumentLed2["brightness"] = "true";
      staticJsonDocumentLed2["color_temp"] = "true";

      String string2Led2;

      serializeJson(staticJsonDocumentLed2, string2Led2);

      pubSubClient.publish(configTopicLed2.c_str(), string2Led2.c_str(), true);
      pubSubClient.subscribe(commandTopicLed2.c_str());

      publish();
    }
  } else {
    pubSubClient.loop();
  }

  led1();
  led2();
}

void callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<JSON_OBJECT_SIZE(8)> staticJsonDocument;

  deserializeJson(staticJsonDocument, (char*)payload);

  if (strcmp(topic, commandTopicLed1.c_str()) == 0) {
    if (staticJsonDocument.containsKey("state")) {
      if (strcmp(staticJsonDocument["state"], "ON") == 0) {
        stateLed1 = true;

        if (staticJsonDocument.containsKey("brightness")) {
          brightnessLed1 = staticJsonDocument["brightness"];
        }

        if (staticJsonDocument.containsKey("color_temp")) {
          colorTempLed1 = staticJsonDocument["color_temp"];
        }
      }
      else if (strcmp(staticJsonDocument["state"], "OFF") == 0) {
        stateLed1 = false;
      }
    }

    StaticJsonDocument<JSON_OBJECT_SIZE(8)> staticJsonDocumentLed1;

    staticJsonDocumentLed1["name"] = nameLed1;

    staticJsonDocumentLed1["state"] = stateLed1;
    staticJsonDocumentLed1["brightness"] = brightnessLed1;
    staticJsonDocumentLed1["color_temp"] = colorTempLed1;

    File fileLed1 = SPIFFS.open("/led1.json", "w");

    serializeJson(staticJsonDocumentLed1, fileLed1);

    fileLed1.close();
  }

  if (strcmp(topic, commandTopicLed2.c_str()) == 0) {
    if (staticJsonDocument.containsKey("state")) {
      if (strcmp(staticJsonDocument["state"], "ON") == 0) {
        stateLed2 = true;

        if (staticJsonDocument.containsKey("brightness")) {
          brightnessLed2 = staticJsonDocument["brightness"];
        }

        if (staticJsonDocument.containsKey("color_temp")) {
          colorTempLed2 = staticJsonDocument["color_temp"];
        }
      }
      else if (strcmp(staticJsonDocument["state"], "OFF") == 0) {
        stateLed2 = false;
      }
    }

    StaticJsonDocument<JSON_OBJECT_SIZE(8)> staticJsonDocumentLed2;

    staticJsonDocumentLed2["name"] = nameLed2;

    staticJsonDocumentLed2["state"] = stateLed2;
    staticJsonDocumentLed2["brightness"] = brightnessLed2;
    staticJsonDocumentLed2["color_temp"] = colorTempLed2;

    File fileLed2 = SPIFFS.open("/led2.json", "w");

    serializeJson(staticJsonDocumentLed2, fileLed2);

    fileLed2.close();
  }
  publish();
}

void publish() {
  StaticJsonDocument<JSON_OBJECT_SIZE(8)> staticJsonDocumentLed1;

  if (stateLed1) {
    staticJsonDocumentLed1["state"] = "ON";
    staticJsonDocumentLed1["brightness"] = brightnessLed1;
    staticJsonDocumentLed1["color_temp"] = colorTempLed1;
  } else {
    staticJsonDocumentLed1["state"] = "OFF";
  }

  String stringLed1;

  serializeJson(staticJsonDocumentLed1, stringLed1);

  pubSubClient.publish(stateTopicLed1.c_str(), stringLed1.c_str(), true);

  StaticJsonDocument<JSON_OBJECT_SIZE(8)> staticJsonDocumentLed2;

  if (stateLed2) {
    staticJsonDocumentLed2["state"] = "ON";
    staticJsonDocumentLed2["brightness"] = brightnessLed2;
    staticJsonDocumentLed2["color_temp"] = colorTempLed2;
  } else {
    staticJsonDocumentLed2["state"] = "OFF";
  }

  String stringLed2;

  serializeJson(staticJsonDocumentLed2, stringLed2);

  pubSubClient.publish(stateTopicLed2.c_str(), stringLed2.c_str(), true);
}

void saveConfigCallback() {
  saveConfig = true;
}

void led1() {
  if (stateLed1) {
    analogWrite1 = (brightnessLed1 * (colorTempLed1 - MIN_MIREDS) * 255 / (MAX_MIREDS - MIN_MIREDS) / 255 + 1) * 4 - 1;
    analogWrite3 = (brightnessLed1 * (255 - (colorTempLed1 - MIN_MIREDS) * 255 / (MAX_MIREDS - MIN_MIREDS)) / 255) * 4 - 1;
  } else {
    analogWrite1 = 0;
    analogWrite3 = 0;
  }

  analogWrite(1, analogWrite1);
  analogWrite(3, analogWrite3);
}

void led2() {
  if (stateLed2) {
    analogWrite0 = (brightnessLed2 * (colorTempLed2 - MIN_MIREDS) * 255 / (MAX_MIREDS - MIN_MIREDS) / 255 + 1) * 4 - 1;
    analogWrite2 = (brightnessLed2 * (255 - (colorTempLed2 - MIN_MIREDS) * 255 / (MAX_MIREDS - MIN_MIREDS)) / 255) * 4 - 1;
  } else {
    analogWrite0 = 0;
    analogWrite2 = 0;
  }

  analogWrite(0, analogWrite0);
  analogWrite(2, analogWrite2);
}
