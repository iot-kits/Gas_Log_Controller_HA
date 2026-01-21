/**
 * @file configuration.h
 * @version 2026.01.16
 * @author Karl Berger & OpenAI ChatGPT
 * @brief Configuration header file for Gas Log Controller ESP32-C3 project
 *
 * This header file contains all the configuration constants and settings for the Gas Log Controller
 * system running on ESP32-C3 SuperMini. It includes WiFi credentials, network settings, OTA
 * configuration, GPIO pin assignments, and various operational parameters.
 *
 * @section wifi_config WiFi Configuration
 * - WIFI_SSID: Network name for WiFi connection
 * - WIFI_PASSWORD: WiFi network password
 *
 * @section ota_config Over-The-Air Update Configuration
 * - OTA_HOSTNAME: Device hostname for OTA identification and mDNS resolution
 * - OTA_PASSWORD: Security password for OTA updates
 *
 * @section gpio_config GPIO Pin Assignments
 * - LED_PIN: Built-in LED control pin (GPIO 8)
 * - ONE_WIRE_BUS: Dallas DS18B20 temperature sensor data pin (GPIO 6)
 * - SDA_PIN/SCL_PIN: I2C communication pins (GPIO 3/2)
 * - HBRIDGE_IN1_PIN/HBRIDGE_IN2_PIN: H-Bridge motor control pins (GPIO 1/0)
 *
 * @section timing_config Timing and Operational Settings
 * - STATUS_CHECK_INTERVAL: Frequency of system status checks (5 seconds)
 * - SENSOR_UPDATE_INTERVAL: Frequency of sensor reading updates (5 seconds)
 * - METRIC_UNITS: Unit system flag (false = Imperial, true = Metric)
 * - TEMP_RESOLUTION: DS18B20 sensor precision (11-bit resolution)
 * - timeToOpenValve/timeToCloseValve: Valve operation timing (7 seconds each)
 * - THERMOSTAT_HYSTERESIS: Temperature control deadband (0.2°F)
 *
 * @note This configuration is specifically designed for ESP32-C3 SuperMini hardware
 * @note IP addresses are normally provided by your network's DHCP server
 * @warning Ensure WiFi credentials are properly secured in production environments
 */
#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h> // for Arduino core

// WiFi Credentials
static const char *WIFI_SSID = "DCMNET";
static const char *WIFI_PASSWORD = "0F1A2D3E4D5G6L7O8R9Y";

// Network configuration: IP address is assigned via DHCP by default.
// If you require a fixed address, reserve a DHCP address on the router.

// OTA Configuration
static const char *OTA_HOSTNAME = "GasLogController"; // Hostname for OTA and local hostname resolution
static const char *OTA_PASSWORD = "GasLog2025";		  // Password for OTA security

// GPIO Pin Assignments
static const int PIN_LED = 8;			// Built-in LED GPIO for ESP32-C3 SuperMini
static const int PIN_ONE_WIRE_BUS = 6;	// 1-Wire Bus for DS18b20 temperature sensor
static const int PIN_SDA = 3;			// I2C SDA pin
static const int PIN_SCL = 9;			// I2C SCL pin
static const int PIN_HBRIDGE_IN1 = 1;	// H-Bridge IN1 pin
static const int PIN_HBRIDGE_IN2 = 0;	// H-Bridge IN2 pin
static const int PIN_VOLTAGE_SENSE = 2; // Voltage sense pin (ADC)

// Update intervals & settings
static unsigned long STATUS_CHECK_INTERVAL = 5000;		  // Check status periodically
static const unsigned long SENSOR_UPDATE_INTERVAL = 5000; // Update sensor readings periocally
static bool METRIC_UNITS = false;						  // Set to true for metric units, false for imperial
static const int TEMP_RESOLUTION = 12;					  // DS18b20 temperature sensor resolution (9-12 bits)
static const float THERMOSTAT_HYSTERESIS = 0.2;			  // Thermostat hysteresis in Fahrenheit degrees
static const float valveVoltage = 6.5;					  // Voltage to apply to valve motor
static const float voltageDividerRatio = 15.24;			  // Voltage divider ratio for power supply measurement
static unsigned long timeToOpenValve = 7000;			  // Time to fully open valve in milliseconds
static unsigned long timeToCloseValve = 7000;			  // Time to fully close valve in milliseconds

#endif // CONFIGURATION_H