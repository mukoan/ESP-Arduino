/**
 * @file   relay_mqtt.ino
 * @brief  ESP-01S Remote Relay using MQTT
 * @author Lyndon Hill
 * @date   2024.10.06
 */

#include <ESP8266WiFi.h>
#include <AsyncMqtt_Generic.h>
#include "general.h"

// MQTT Topic to subscribe, that will control relay
const char *subTopic = "lh-mqtt/relay";

// GPIO pin number that operates relay
// This file uses Normally Open configuration.
// If you want to use Normally Closed then you need to change the logic
// state between LOW and HIGH for every line that uses relayPin and
// add a comment here
const int relayPin = 0;

WiFiClient espClient;
AsyncMqttClient mqttClient;


void setupWifi() {
  delay(10);
  WiFi.begin(MY_SSID, MY_WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void connectMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent)
{
  Serial.print("Connected to MQTT broker: "); Serial.print(MQTT_HOST);
  Serial.print(", port: "); Serial.println(MQTT_PORT);

  mqttClient.subscribe(subTopic, 0);
  Serial.print("Session present: "); Serial.println(sessionPresent);
}

void onMqttSubscribe(const uint16_t &packetId, const uint8_t &qos)
{
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttMessage(char *topic, char *payload, const AsyncMqttClientMessageProperties &properties,
                   const size_t &len, const size_t &index, const size_t &total)
{
  Serial.println("Publish received.");
  Serial.print("  topic: ");  Serial.println(topic);
  Serial.print("  qos: ");    Serial.println(properties.qos);
  Serial.print("  dup: ");    Serial.println(properties.dup);
  Serial.print("  retain: "); Serial.println(properties.retain);
  Serial.print("  len: ");    Serial.println(len);
  Serial.print("  index: ");  Serial.println(index);
  Serial.print("  total: ");  Serial.println(total);

  String content = "";
  for(size_t i = 0; i < len; i++)
    content.concat(payload[i]);

  Serial.print(content);
  Serial.println();

  // Set the relay according to Normally Open configuration
  if(content == "ON") {
    digitalWrite(relayPin, LOW);
    Serial.println("Switched on");
  }

  if(content == "OFF") {
    digitalWrite(relayPin, HIGH);
    Serial.println("Switched off");
  }
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{
  (void) reason;
  Serial.println("Disconnected from MQTT.");
}

void setup()
{
  Serial.begin(115200);
  pinMode(relayPin, OUTPUT);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);

  setupWifi();
  connectMqtt();

  digitalWrite(relayPin, HIGH); // Switch off at start up (Normally Open)
}

void loop()
{
}
