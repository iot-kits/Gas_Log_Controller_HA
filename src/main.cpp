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
#include <fauxmoESP.h>      // for fauxmoESP Alexa emulation

fauxmoESP fauxmo; // Create fauxmoESP instance for Alexa emulation

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
    updateWebStatus("Sensor failed");
  }

  // Configure fauxmo (Alexa emulation) to use our existing AsyncWebServer
  fauxmo.createServer(false);             // Use external webserver to avoid port conflict
  fauxmo.setPort(80);                    // Required for Gen3 (ESP32-C3)
  fauxmo.enable(true);                   // Enable fauxmoESP
  fauxmo.addDevice("Gas Log Fireplace"); // Name for Alexa discovery

  // Initialize Alexa-reported state to match current control state
  fauxmo.setState("Gas Log Fireplace", (controlState.mode == MODE_ON), (controlState.mode == MODE_ON) ? 255 : 0);

  fauxmo.onSetState([](unsigned char device_id, const char *device_name,
                       bool state, unsigned char value)
                    {
    Serial.printf("[ALEXA] %s: %s\n", device_name, state ? "ON" : "OFF");
    
    if (strcmp(device_name, "Gas Log Fireplace") == 0) {
      if (state) {
        // Alexa ON → Manual mode (valve open)
        controlState.mode = MODE_ON;
        Serial.println("Alexa: Mode MANUAL");
        setRoomTempColor("HEATING");
        updateWebStatus("Heating (Alexa)");
      } else {
        // Alexa OFF → Off mode (valve closed)
        controlState.mode = MODE_OFF;
        Serial.println("Alexa: Mode OFF");
        setRoomTempColor("OFF");
        updateWebStatus("System Off (Alexa)");
      }
      broadcastControlState();  // Updates web clients + applies mode
    } });
}

void loop()
{
  fauxmo.handle();          // Handle Alexa emulation (non-blocking)
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
      // Update live room temperature in control state so new WS clients see it immediately
      controlState.roomTempF = tempF;
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "{\"type\":\"temperature\",\"value\":%.1f}", tempF);
      String tempMessage = String(buffer);
      notifyAllClients(tempMessage);
    }

    lastTempSensorUpdate = millis(); // update timestamp
  }

  // Handle three-state mode system
  switch (controlState.mode)
  {

  case MODE_OFF:
    setRoomTempColor("OFF");
    valveOpenRequest(false);
    break;

  case MODE_ON: // manual
    setRoomTempColor("HEATING");
    valveOpenRequest(true);
    break;

  case MODE_THERMOSTAT:
    if (thermostatHeatCall(tempF, controlState.setpointF))
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

  default:
    // Safety fallback - turn off if mode is invalid
    setRoomTempColor("OFF");
    valveOpenRequest(false);
    updateWebStatus("Invalid mode");
    break;
  }
}