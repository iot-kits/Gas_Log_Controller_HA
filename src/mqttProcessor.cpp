
/*
 * MQTT Topic Schema
 *
 * State topics (ESP32 → HA)
 *   gaslog/mode              → "OFF", "THERMOSTAT", "ON"
 *   gaslog/valve_state       → "OPEN", "CLOSED"
 *   gaslog/temperature       → numeric (°F)
 *   gaslog/setpoint          → numeric (°F)
 * Command topics (HA → ESP32)
 *   gaslog/set_mode          → "OFF", "THERMOSTAT", "ON"
 *   gaslog/set_setpoint      → numeric (°F)
 */

#include <Arduino.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

void setupMQTT()
{
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
}

// Publish Telemetry
void publishTelemetry()
{
  mqttClient.publish("gaslog/mode", currentModeString); // OFF / THERMOSTAT / ON
  mqttClient.publish("gaslog/valve_state", valveOpen ? "OPEN" : "CLOSED");

  char tempBuf[8];
  dtostrf(roomTempF, 1, 1, tempBuf);
  mqttClient.publish("gaslog/temperature", tempBuf);

  char spBuf[8];
  dtostrf(setpointF, 1, 0, spBuf);
  mqttClient.publish("gaslog/setpoint", spBuf);
}

// Subscribe to Commands
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String msg;
  for (int i = 0; i < length; i++)
    msg += (char)payload[i];

  if (strcmp(topic, "gaslog/set_mode") == 0)
  {
    if (msg == "OFF")
      setMode(MODE_OFF);
    else if (msg == "ON")
      setMode(MODE_ON);
    else if (msg == "THERMOSTAT")
      setMode(MODE_THERMOSTAT);
  }

  if (strcmp(topic, "gaslog/set_setpoint") == 0)
  {
    setpointF = msg.toInt(); // You can clamp/validate here
  }
}
