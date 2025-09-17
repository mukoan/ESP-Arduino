// Robustified temperature and humidity to MQTT
// for ESP-01S
// Based on code example, original comments follow:

// DHT Temperature & Humidity Sensor
// Unified Sensor Library Example
// Written by Tony DiCola for Adafruit Industries
// Released under an MIT license.

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor

#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <Ticker.h>
#include <AsyncMqtt_Generic.h>

#include "general.h"

// Sensor settings
#define DHTPIN   2                // Digital pin connected to the DHT sensor
#define DHTTYPE  DHT11            // DHT 11
DHT_Unified dht(DHTPIN, DHTTYPE);

float temperature, humidity;      // Values read from sensor
unsigned long delayTime;

// MQTT client
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

// WiFi client
WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

// MQTT topics to publish
const char *PubTopic1 = "loft/temperature";
const char *PubTopic2 = "loft/humidity";

// Connect to WiFi
void connectWiFi(){
  WiFi.begin(MY_SSID, MY_WIFI_PASSWD);
  Serial.println("Connecting Wifi");
}

void connectMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP &event)
{
  (void) event;

  Serial.print("Connected to Wi-Fi. IP address: ");
  Serial.println(WiFi.localIP());
  connectMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event)
{
  (void) event;

  Serial.println("Disconnected from Wi-Fi.");
  // Ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  mqttReconnectTimer.detach();
  wifiReconnectTimer.once(2, connectWiFi);
}

void onMqttConnect(bool sessionPresent)
{
  Serial.print("Connected to MQTT broker: "); Serial.print(MQTT_HOST);
  Serial.print(", port: "); Serial.println(MQTT_PORT);
  Serial.print("Session present: "); Serial.println(sessionPresent);
}

void onMqttPublish(const uint16_t& packetId)
{
  Serial.println("Publish acknowledged");
  Serial.print("  packetId: "); Serial.println(packetId);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  (void) reason;

  Serial.println("Disconnected from MQTT.");
  if(WiFi.isConnected())
  {
    // Start reconnect timer if WiFi still connected
    mqttReconnectTimer.once(2, connectMqtt);
  }
}


void setup() {
  Serial.begin(9600);

  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  // Set MQTT callbacks
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  // Initialize Wi-Fi
  connectWiFi();

  // Initialize device.
  dht.begin();

  Serial.println("DHT Weather Service");
  Serial.print("Connected to ");
  Serial.println(MY_SSID);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  temperature = 0.0;
  humidity    = 0.0;
  delayTime   = 1000;  // time between updates, in milliseconds
}

void loop() {
  updateValues();
  delay(delayTime);
}


void updateValues() {
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    temperature = event.temperature;
  }

  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    humidity = event.relative_humidity;
  }

  // Publish MQTT messages
  char pubMesg[5];

  snprintf(pubMesg, 5, "%2.1f\0", temperature);
  mqttClient.publish(PubTopic1, 0, true, pubMesg);

  snprintf(pubMesg, 3, "%2.0f\0", humidity);
  mqttClient.publish(PubTopic2, 0, true, pubMesg);
}
