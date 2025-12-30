//! Main application file for Smart Thermostat
//! moved to github 2025.12.30


#include <Arduino.h>        // for Arduino core
#include "ArduinoOTA.h"     // for ArduinoOTA.handle() in loop()
#include "configuration.h"  // for credentials, hardware connections, and control parameters
#include "sensor.h"         // for sensor reading functions
#include "thermostat.h"     // for thermostat logic
#include "valveDriver.h"    // for valve control
#include "webSocket.h"      // set up webSocket
#include <WiFi.h>           // for WiFi connection
#include "wifiConnection.h" // set up local WiFi
#include <Wire.h>           // I2C library

// Status management
String lastStatusMessage = ""; // Made available to webSocket.cpp
String lastSliderState = "";   // Track slider state changes
unsigned long lastStatusCheck = 0;
unsigned long lastTempSensorUpdate = 0;

// Helper function to update slider state only when changed
void updateSliderState(const char* newState) {
  if (strcmp(controlState.sliderState, newState) != 0) {
    controlState.sliderState = newState;
    broadcastControlState();
  }
}

void setup()
{
  Serial.begin(115200);         // Initialize serial communication at 115200 baud
  Wire.begin(SDA_PIN, SCL_PIN); // Begin I2C on pins assigned in configuration.h
  pinMode(MIC_PIN, INPUT);      // Initialize the microphone pin as an input
  wifiBegin();                  // Initialize WiFi
  wifiConnect();                // Connect to WiFi
  otaBegin();                   // Initialize Over-The-Air update service
  websocketBegin();             // Initialize webSocket for bi-directional communication with web UI
  valveDriverBegin();           // Initialize valve driver pins and state
  initSensor();                 // Initialize temperature sensor
}

void loop()
{
  static float tempF = 0.0; // Declare tempF at function scope
  wifiConnect();            // Automatically reconnects if disconnected
  ArduinoOTA.handle();      // Handle OTA updates
  websocketCleanup();       // Perform web client cleanup

  // Non-blocking periodic sensor update
  if (millis() - lastTempSensorUpdate > SENSOR_UPDATE_INTERVAL)
  {
    // If sensor failed to initialize, skip
    // if (!tempSensorInitSuccess)
    //   return;

    float tempC = readTemperature();
    float humidity = readHumidity();

    if (!isnan(tempC))
    {
      tempF = tempC * 9.0 / 5.0 + 32.0; // Convert to Fahrenheit
      Serial.print("Temperature: ");
      Serial.print(tempF);
      Serial.println(" °F");
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "{\"type\":\"temperature\",\"value\":%.1f}", tempF);
      String tempMessage = String(buffer);
      notifyAllClients(tempMessage);
    }
    else
    {
      Serial.println("Error: Temperature sensor read failed");
      updateWebStatus("Error: Temperature sensor read failed");
    }

    lastTempSensorUpdate = millis(); // update timestamp
  }

  // MANUAL mode handling
  if (!controlState.autoMode)
  {
    if (controlState.powerOn) // Manual mode with power ON
    {
      valveOpenRequest(true);
      updateSliderState("HEATING");
      updateWebStatus("Manual Mode: Heating");
    }
    else // Manual mode with power OFF
    {
      valveOpenRequest(false);
      updateSliderState("OFF");
      updateWebStatus("Manual Mode: Off");
    }
    return; // exit loop after handling MANUAL mode
  }

  // fall through to AUTOMATIC mode
  if (controlState.powerOn)
  {
    // Automatic Mode with Power ON: run periodic status check
    if (millis() - lastStatusCheck > STATUS_CHECK_INTERVAL)
    {
      Serial.println("Power ON, AUTO mode.");
      if (controlState.autoMode)
      {
      Serial.println("Automatic mode active.");
      if (thermostatHeatCall(tempF, controlState.setpointF))
      {
        updateSliderState("HEATING");
        updateWebStatus("Thermostat: Heating");
        valveOpenRequest(true);
      }
      else
      {
        updateSliderState("IDLE");
        updateWebStatus("Thermostat: Idle");
        valveOpenRequest(false);
      }
    }
    lastStatusCheck = millis();
    }
  }
  else
  {
    valveOpenRequest(false); // power state is OFF, ensure valve is closed
  }
}
