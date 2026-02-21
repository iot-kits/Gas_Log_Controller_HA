/**
 * @file main.cpp
 * @brief Gas Log Controller main application for ESP32-C3 with MQTT for Home Assistant
 * @author Karl Berger
 * @version 2026-02-21
 *
 * This application implements a WiFi-enabled thermostat controller for gas log systems.
 * It provides web-based control interface, temperature monitoring, and safety features.
 *
 * Key Features:
 * - WiFi connectivity with auto-reconnect capability
 * - Over-the-Air (OTA) firmware updates
 * - WebSocket-based real-time communication with web UI
 * - DS18B20 temperature sensor integration with Celsius to Fahrenheit conversion
 * - Three operating modes: OFF, THERMOSTAT (automatic), and ON (manual)
 * - Safety valve control with timeout protection
 * - Real-time temperature broadcasting to connected clients
 *
 * Operating Modes:
 * - MODE_OFF: System disabled, valve closed
 * - MODE_THERMOSTAT: Automatic temperature control based on setpoint
 * - MODE_ON: Manual override, valve always open
 *
 * Safety Features:
 * - Automatic fallback to OFF mode when temperature sensor is unavailable
 * - Invalid mode detection with safety shutdown
 * - Valve driver safety timers and scheduling
 *
 * Communication:
 * - Serial console at 115200 baud for debugging
 * - WebSocket JSON messages for temperature updates
 * - mDNS for network discovery
 *
 * Hardware Requirements:
 * - ESP32-C3 microcontroller
 * - DS18B20 temperature sensor
 * - Valve control hardware (configuration dependent)
 */
#include <Arduino.h>        // for Arduino core
#include <ArduinoOTA.h>     // for ArduinoOTA.handle() in loop()
#include <WiFi.h>           // for WiFi connection
#include <ArduinoJson.h>    // for JSON serialization
#include "configuration.h"  // for credentials, hardware connections, and control parameters
#include "ds18b20_sensor.h" // for DS18B20 temperature sensor functions
#include "mqttProcessor.h"  // for MQTT communication with Home Assistant
#include "thermostat.h"     // for thermostat logic
#include "valveDriver.h"    // for valve control
#include "webSocket.h"      // set up webSocket
#include "wifiConnection.h" // set up local WiFi

// Status management
unsigned long lastTempSensorUpdate = 0; // Timestamp for last temperature sensor update
bool tempSensorAvailable = false;       // Track if sensor initialized successfully

void setup()
{
  Serial.begin(115200);               // Start serial at 115200 baud
  delay(3000);                        // Delay for host to attach (needed for ESP32-C3)
  Serial.println("Serial is ready!"); // Debug message
  wifiBegin(true);                    // Initialize WiFi and wait for connection (starts mDNS + OTA)
  websocketBegin();                   // Initialize webSocket for bi-directional communication with web UI
  valveDriverBegin();                 // Initialize valve driver pins and state
  setupMQTT();                        // Initialize MQTT client and set callback
  tempSensorAvailable = initSensor(); // Initialize temperature sensor and check if successful
  if (!tempSensorAvailable)
  {
    controlState.mode = MODE_OFF; // Force OFF mode when no temperature sensor
    updateWebStatus("Sensor failure: Thermostat mode disabled");
  }
} // end of setup()

void loop()
{
  static float tempF = NAN;
  static unsigned long lastTelemetry = 0;

  // --- Network & OTA ---
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.reconnect();
  }

  ArduinoOTA.handle();
  websocketCleanup();

  // --- MQTT ---
  mqttLoop(); // must run every iteration for reconnect + callbacks

  // Publish telemetry every 5 seconds
  unsigned long now = millis();
  if (now - lastTelemetry > 5000)
  {
    publishTelemetry();
    lastTelemetry = now;
  }

  // --- Temperature Sensor Update ---
  if (millis() - lastTempSensorUpdate > SENSOR_UPDATE_INTERVAL)
  {
    float tempC = readTemperature();
    if (!isnan(tempC))
    {
      tempF = tempC * 9.0 / 5.0 + 32.0;
      controlState.roomTempF = tempF;
      Serial.printf("Room Temp: %.1f °F\n", tempF);
      broadcastTemperature();
    }

    lastTempSensorUpdate = millis();
  }

  // --- Mode Logic ---
  switch (controlState.mode)
  {
  case MODE_OFF:
    setRoomTempColor("OFF");
    valveOpenRequest(false);
    break;

  case MODE_THERMOSTAT:
    if (!tempSensorAvailable)
    {
      setRoomTempColor("OFF");
      valveOpenRequest(false);
      updateWebStatus("Sensor failure: Thermostat mode disabled");
      controlState.mode = MODE_OFF;
    }
    else if (thermostatHeatCall(tempF, controlState.setpointF))
    {
      setRoomTempColor("HEATING");
      valveOpenRequest(true);
    }
    else
    {
      setRoomTempColor("IDLE");
      valveOpenRequest(false);
    }
    break;

  case MODE_ON:
    setRoomTempColor("HEATING");
    valveOpenRequest(true);
    break;

  default:
    setRoomTempColor("OFF");
    valveOpenRequest(false);
    updateWebStatus("Error: Invalid mode");
    break;
  }
}
