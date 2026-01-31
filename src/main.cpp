/**
 * @file main.cpp
 * @brief Gas Log Controller main application for ESP32-C3
 * @author Karl Berger
 * @version 2026-01-28
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
  tempSensorAvailable = initSensor(); // Initialize temperature sensor and check if successful
  if (!tempSensorAvailable)
  {
    controlState.mode = MODE_OFF; // Force OFF mode when no temperature sensor
    updateWebStatus("Sensor failure: Thermostat mode disabled");
  }
} // end of setup()

void loop()
{
  static float tempF = NAN; // Initialize to NAN to indicate no valid reading yet
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.reconnect(); // rely on auto-reconnect; attempt a reconnect if disconnected
  }
  ArduinoOTA.handle(); // Handle OTA updates
  websocketCleanup();  // Perform web client cleanup
  // valveDriverLoop();   // Enforce safety timers and schedule

  // Non-blocking periodic sensor update
  if (millis() - lastTempSensorUpdate > SENSOR_UPDATE_INTERVAL)
  {
    // readTemperature() returns NAN if sensor not initialized or disconnected
    float tempC = readTemperature();
    if (!isnan(tempC))
    {
      tempF = tempC * 9.0 / 5.0 + 32.0; // Convert Celsius to Fahrenheit
      Serial.printf("Room Temp: %.1f °F\n", tempF);
      // Update live room temperature in control state so new WS clients see it immediately
      controlState.roomTempF = tempF;
      broadcastTemperature();
    }

    lastTempSensorUpdate = millis(); // update timestamp
  }

  // Handle three-state mode buttons
  switch (controlState.mode)
  {
  case MODE_OFF:
    setRoomTempColor("OFF");
    valveOpenRequest(false);
    break;

  case MODE_THERMOSTAT:
    if (!tempSensorAvailable)
    {
      // Sensor not available - disable thermostat mode for safety
      setRoomTempColor("OFF");
      valveOpenRequest(false);
      updateWebStatus("Sensor failure: Thermostat mode disabled");
      controlState.mode = MODE_OFF; // Force back to OFF mode
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
    // Safety fallback - turn off if mode is invalid
    setRoomTempColor("OFF");
    valveOpenRequest(false);
    updateWebStatus("Error: Invalid mode");
    break;
  }
} // end of loop()