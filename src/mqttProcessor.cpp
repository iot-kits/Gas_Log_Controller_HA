
/**
 * @file mqttProcessor.cpp
 * @version 2026.02.21
 * @author Karl Berger
 * @brief MQTT processor implementation for Gas Log Controller ESP32
 **/

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
// mqttProcessor.cpp

#include "mqttProcessor.h"
#include "configuration.h"
#include "websocket.h"
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient client(espClient);

void mqttCallback(char *topic, byte *payload, unsigned int length); // forward declare
void mqttReconnect();

void setupMQTT()
{
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(mqttCallback);
}

void mqttLoop()
{
  if (!client.connected())
  {
    mqttReconnect();
  }
  client.loop();
}

void mqttReconnect()
{
  while (!client.connected())
  {
    if (client.connect("gaslog-controller"))
    {
      client.subscribe("gaslog/set_mode");
      client.subscribe("gaslog/set_setpoint");
    }
    else
    {
      delay(2000);
    }
  }
}

// ---------------------------
// Publish Telemetry
// ---------------------------
void publishTelemetry()
{
  // MODE
  const char *modeStr =
      (controlState.mode == MODE_OFF) ? "OFF" : (controlState.mode == MODE_ON)       ? "ON"
                                            : (controlState.mode == MODE_THERMOSTAT) ? "THERMOSTAT"
                                                                                     : "UNKNOWN";

  client.publish("gaslog/mode", modeStr);

  // VALVE STATE ("OFF", "IDLE", "HEATING")
  client.publish("gaslog/valve_state", controlState.valveState.c_str());

  // TEMPERATURE
  char tempBuf[16];
  snprintf(tempBuf, sizeof(tempBuf), "%.1f", controlState.roomTempF);
  client.publish("gaslog/temperature", tempBuf);

  // SETPOINT
  char spBuf[16];
  snprintf(spBuf, sizeof(spBuf), "%d", controlState.setpointF);
  client.publish("gaslog/setpoint", spBuf);
}

// ---------------------------
// MQTT Command Handler
// ---------------------------
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }

  // MODE COMMAND
  if (strcmp(topic, "gaslog/set_mode") == 0)
  {
    if (msg == "OFF")
      controlState.mode = MODE_OFF;
    else if (msg == "ON")
      controlState.mode = MODE_ON;
    else if (msg == "THERMOSTAT")
      controlState.mode = MODE_THERMOSTAT;

    // If you have a central handler:
    // setMode(controlState.mode);
  }

  // SETPOINT COMMAND
  if (strcmp(topic, "gaslog/set_setpoint") == 0)
  {
    controlState.setpointF = msg.toInt();
  }
}
