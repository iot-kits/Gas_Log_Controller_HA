//! Main application file for Smart Thermostat
//! moved to github 2025.12.30

#include <Arduino.h>        // for Arduino core
#include "ArduinoOTA.h"     // for ArduinoOTA.handle() in loop()
#include "configuration.h"  // for credentials, hardware connections, and control parameters
#include "ds18b20_sensor.h" // for DS18B20 temperature sensor functions
#include "thermostat.h"     // for thermostat logic
#include "valveDriver.h"    // for valve control
#include "webSocket.h"      // set up webSocket
#include <WiFi.h>           // for WiFi connection
#include "wifiConnection.h" // set up local WiFi
#include <Wire.h>           // I2C library

// Status management
unsigned long lastStatusCheck = 0;      // Timestamp for last status check
unsigned long lastTempSensorUpdate = 0; // Timestamp for last temperature sensor update
bool tempSensorAvailable = false;       // Track if sensor initialized successfully

void setup()
{
  Serial.begin(115200);               // Initialize serial communication at 115200 baud
  Wire.begin(SDA_PIN, SCL_PIN);       // Begin I2C on pins assigned in configuration.h
  wifiBegin();                        // Initialize WiFi
  wifiConnect();                      // Connect to WiFi
  otaBegin();                         // Initialize Over-The-Air update service
  websocketBegin();                   // Initialize webSocket for bi-directional communication with web UI
  valveDriverBegin();                 // Initialize valve driver pins and state
  tempSensorAvailable = initSensor(); // Initialize temperature sensor and check if successful

  if (!tempSensorAvailable)
  {
    controlState.mode = MODE_OFF; // Force OFF mode when no temperature sensor
    updateWebStatus("Warning: Temperature sensor not detected - Thermostat mode disabled");
  }
}

void loop()
{
  static float tempF = NAN; // Initialize to NAN to indicate no valid reading yet
  wifiConnect();            // Automatically reconnects if disconnected
  ArduinoOTA.handle();      // Handle OTA updates
  websocketCleanup();       // Perform web client cleanup

  // Non-blocking periodic sensor update
  if (millis() - lastTempSensorUpdate > SENSOR_UPDATE_INTERVAL)
  {
    // readTemperature() returns NAN if sensor not initialized or disconnected
    float tempC = readTemperature();

    if (!isnan(tempC))
    {
      tempF = tempC * 9.0 / 5.0 + 32.0; // Convert Celsius to Fahrenheit
      Serial.printf("Room Temp: %.1f °F\n", tempF);
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "{\"type\":\"temperature\",\"value\":%.1f}", tempF);
      String tempMessage = String(buffer);
      notifyAllClients(tempMessage);
    }
 
    lastTempSensorUpdate = millis(); // update timestamp
  }

  // Handle three-state mode system
switch (controlState.mode) {

  case MODE_OFF:
    setRoomTempColor("OFF");
    valveOpenRequest(false);
    break;

  case MODE_ON:  // manual
    setRoomTempColor("HEATING");
    valveOpenRequest(true);
    // if (controlState.manualValveOpen) {
    //   setRoomTempColor("HEATING");
    // } 
    // valveOpenRequest(controlState.manualValveOpen);
    break;

  case MODE_THERMOSTAT:
    if (thermostatHeatCall(tempF, controlState.setpointF)) {
      setRoomTempColor("HEATING");
      valveOpenRequest(true);
    } else {
      setRoomTempColor("IDLE");
      valveOpenRequest(false);
    }
    break;

  default:
    // Safety fallback - turn off if mode is invalid
    valveOpenRequest(false);
    setRoomTempColor("OFF");
    updateWebStatus("Error: Invalid mode");
    break;
  }
}