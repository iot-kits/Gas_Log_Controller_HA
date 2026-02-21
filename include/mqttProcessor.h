// mqttProcessor.h

#pragma once
#include <Arduino.h>
#include <webSocket.h> // For ControlState and Mode enum

// MQTT functions
void setupMQTT();
void mqttLoop();
void publishTelemetry();
