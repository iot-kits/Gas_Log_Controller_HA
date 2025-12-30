/**
 * @file configuration.h
 * @brief Configuration header file for Gas Log Controller
 *
 * This file contains all the configuration settings for the Gas Log Controller
 * running on an ESP32-C3 SuperMini board. It includes WiFi credentials, network
 * settings, GPIO pin assignments, and various operational parameters.
 *
 * @note All constants are defined as inline to allow inclusion in multiple
 *       translation units without violating the One Definition Rule (ODR).
 *
 * @section wifi WiFi Configuration
 * - WIFI_SSID: Network SSID for WiFi connection
 * - WIFI_PASSWORD: Network password for WiFi authentication
 *
 * @section network Network Configuration
 * - LOCAL_IP: Static IP address (192.168.0.50)
 * - GATEWAY: Network gateway address (192.168.0.1)
 * - SUBNET: Subnet mask (255.255.255.0)
 *
 * @section gpio GPIO Pin Assignments
 * - LED_PIN (GPIO 8): Built-in LED indicator
 * - DS18b20_PIN (GPIO 6): Temperature sensor data line
 * - SDA_PIN (GPIO 3): I2C data line
 * - SCL_PIN (GPIO 2): I2C clock line
 * - HBRIDGE_IN1_PIN (GPIO 1): H-Bridge control input 1
 * - HBRIDGE_IN2_PIN (GPIO 0): H-Bridge control input 2
 * - MIC_PIN (GPIO 10): Analog microphone input
 *
 * @section timing Timing Configuration
 * - STATUS_CHECK_INTERVAL: System status check interval (10000ms)
 * - SENSOR_UPDATE_INTERVAL: Sensor reading update interval (5000ms)
 *
 * @section sensor Sensor Configuration
 * - METRIC_UNITS: Temperature unit selection (false = imperial, true = metric)
 * - TEMP_RESOLUTION: DS18B20 sensor resolution in bits (9-12)
 */
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>   // for Arduino core
#include <IPAddress.h> // for IPAddress

// WiFi Credentials
static const char *WIFI_SSID = "DCMNET";
static const char *WIFI_PASSWORD = "0F1A2D3E4D5G6L7O8R9Y";

// Static IP Settings
static const IPAddress LOCAL_IP(192, 168, 0, 50); // DHCP reservation range is .100 to .249
static const IPAddress GATEWAY(192, 168, 0, 1);
static const IPAddress SUBNET(255, 255, 255, 0);

// OTA Configuration
static const char *OTA_HOSTNAME = "GasLogController"; // Hostname for OTA identification
static const char *OTA_PASSWORD = "GasLog2025";		  // Password for OTA security

// GPIO Pin Assignments
static const int LED_PIN = 8;		  // Built-in LED GPIO for ESP32-C3 SuperMini
static const int ONE_WIRE_BUS = 6;	  // 1-Wire Bus for DS18b20 temperature sensor
static const int SDA_PIN = 3;		  // I2C SDA pin
static const int SCL_PIN = 2;		  // I2C SCL pin
static const int HBRIDGE_IN1_PIN = 1; // H-Bridge IN1 pin
static const int HBRIDGE_IN2_PIN = 0; // H-Bridge IN2 pin
static const int MIC_PIN = 10;		  // Microphone analog input pin

// Update intervals & settings
static unsigned long STATUS_CHECK_INTERVAL = 5000;		  // Check status every 10 seconds
static const unsigned long SENSOR_UPDATE_INTERVAL = 5000; // Update sensor readings every 5 seconds
static bool METRIC_UNITS = false;						  // Set to true for metric units, false for imperial
static const int TEMP_RESOLUTION = 11;					  // DS18b20 temperature sensor resolution (9-12 bits)
static unsigned long timeToOpenValve = 5000;			  // Time to fully open valve in milliseconds
static unsigned long timeToCloseValve = 5000;			  // Time to fully close valve in milliseconds
static const float THERMOSTAT_HYSTERESIS = 0.2;			  // Thermostat hysteresis in Fahrenheit
#endif													  // CONFIGURATION_H