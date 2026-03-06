
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
    // Connect with Last Will (offline) on availability topic so Home Assistant
    // will know when the device goes offline.
    if (client.connect("gaslog-controller", "gaslog/availability", 0, true, "offline"))
    {
      client.subscribe("gaslog/set_mode");
      client.subscribe("gaslog/set_setpoint");

      // Publish availability as retained so HA knows we're online
      client.publish("gaslog/availability", "online", true);

      // Device information object used in discovery payloads
      String deviceInfo = "{\"identifiers\":[\"gaslog_controller\"],\"name\":\"Gas Log Controller\",\"model\":\"ESP32-C3\",\"manufacturer\":\"Custom\"}";

      // Temperature sensor discovery
      {
        String payload = "{";
        payload += "\"name\":\"GasLog Temperature\",";
        payload += "\"state_topic\":\"gaslog/temperature\",";
        payload += "\"unit_of_measurement\":\"°F\",";
        payload += "\"device_class\":\"temperature\",";
        payload += "\"unique_id\":\"gaslog_temperature_1\",";
        payload += "\"device\":" + deviceInfo;
        payload += "}";
        client.publish("homeassistant/sensor/gaslog_temperature/config", payload.c_str(), true);
      }

      // Setpoint (number) discovery
      {
        String payload = "{";
        payload += "\"name\":\"GasLog Setpoint\",";
        payload += "\"command_topic\":\"gaslog/set_setpoint\",";
        payload += "\"state_topic\":\"gaslog/setpoint\",";
        payload += "\"unit_of_measurement\":\"°F\",";
        payload += "\"min\":40,";
        payload += "\"max\":90,";
        payload += "\"unique_id\":\"gaslog_setpoint_1\",";
        payload += "\"device\":" + deviceInfo;
        payload += "}";
        client.publish("homeassistant/number/gaslog_setpoint/config", payload.c_str(), true);
      }

      // Mode (select) discovery
      {
        String payload = "{";
        payload += "\"name\":\"GasLog Mode\",";
        payload += "\"options\":[\"OFF\",\"THERMOSTAT\",\"ON\"],";
        payload += "\"command_topic\":\"gaslog/set_mode\",";
        payload += "\"state_topic\":\"gaslog/mode\",";
        payload += "\"unique_id\":\"gaslog_mode_1\",";
        payload += "\"device\":" + deviceInfo;
        payload += "}";
        client.publish("homeassistant/select/gaslog_mode/config", payload.c_str(), true);
      }

      // Valve state (binary_sensor) discovery
      {
        String payload = "{";
        payload += "\"name\":\"GasLog Valve\",";
        payload += "\"state_topic\":\"gaslog/valve_state\",";
        payload += "\"payload_on\":\"OPEN\",";
        payload += "\"payload_off\":\"CLOSED\",";
        payload += "\"unique_id\":\"gaslog_valve_1\",";
        payload += "\"device\":" + deviceInfo;
        payload += "}";
        client.publish("homeassistant/binary_sensor/gaslog_valve/config", payload.c_str(), true);
      }
      // === NEW: Climate discovery (add this last) ===
      {
        String payload = "{";
        payload += "\"name\":\"Gas Log Controller\",";
        payload += "\"unique_id\":\"gaslog_climate_1\",";
        payload += "\"device\":" + deviceInfo + ",";   // Now safe to use here

        payload += "\"current_temperature_topic\":\"gaslog/temperature\",";
        payload += "\"current_temperature_template\":\"{{ value | float }}\",";

        payload += "\"temperature_state_topic\":\"gaslog/setpoint\",";
        payload += "\"temperature_command_topic\":\"gaslog/set_setpoint\",";
        payload += "\"temperature_unit\":\"F\",";
        payload += "\"min_temp\":40,";
        payload += "\"max_temp\":90,";
        payload += "\"temp_step\":1,";

        payload += "\"mode_state_topic\":\"gaslog/mode\",";
        payload += "\"mode_command_topic\":\"gaslog/set_mode\",";
        payload += "\"modes\":[\"off\",\"heat\",\"on\"],";

        payload += "\"action_topic\":\"gaslog/valve_state\",";
        payload += "\"action_template\":\"{% if value == 'HEATING' %}heating{% elif value == 'IDLE' %}idle{% elif value == 'OFF' %}off{% else %}off{% endif %}\",";

        payload += "\"availability_topic\":\"gaslog/availability\",";
        payload += "\"payload_available\":\"online\",";
        payload += "\"payload_not_available\":\"offline\"";

        payload += "}";
        client.publish("homeassistant/climate/gaslog_climate/config", payload.c_str(), true);
      }
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
