/**
 * @file ds18b20_sensor.cpp
 * @version 2026.01.08
 * @author Karl Berger & MS Copilot
 * @brief DS18B20 temperature sensor interface implementation
 *
 * This module provides functionality to interface with DS18B20 temperature sensors
 * using the Dallas Temperature library. It handles sensor initialization, temperature
 * reading, and error handling for disconnected sensors.
 */

#include "webSocket.h"         // for updateWebStatus()
#include "configuration.h"     // for TEMP_RESOLUTION
#include "ds18b20_sensor.h"    // for DS18B20 sensor function declarations
#include <DallasTemperature.h> // for DallasTemperature class
#include <OneWire.h>           // for OneWire bus communication

// private module state
static OneWire oneWire(ONE_WIRE_BUS);       // Instantiate OneWire bus on defined pin
static DallasTemperature sensors(&oneWire); // Instantiate DallasTemperature sensor object
static DeviceAddress roomThermometer;       // Device address for the room temperature sensor
static bool tempSensorInitSuccess = false;  // Track if sensor initialized successfully

/**
 * @brief Initializes the DS18B20 temperature sensor and configures it for operation.
 *
 * This function performs the following operations:
 * - Begins communication with DS18B20 sensors on the OneWire bus
 * - Counts and reports the number of detected devices via Serial
 * - Attempts to get the address of the first temperature sensor
 * - Sets global flags and status messages based on initialization success/failure
 * - Configures sensor resolution if initialization is successful
 *
 * @return bool True if sensor initialized successfully, false otherwise
 * @note Updates web status with appropriate messages during initialization
 * @note Uses TEMP_RESOLUTION constant for sensor precision configuration
 */
bool initSensor()
{
    sensors.begin();

    int deviceCount = sensors.getDeviceCount();
    Serial.printf("Found %d DS18B20 device(s).\n", deviceCount);

    if (deviceCount == 0)
    {
        tempSensorInitSuccess = false;
        updateWebStatus("Error: No DS18B20 devices found");
        return false;
    }

    if (!sensors.getAddress(roomThermometer, 0))
    {
        tempSensorInitSuccess = false;        
        updateWebStatus("Error: Unable to find address for temperature sensor");
        return false;
    }

    tempSensorInitSuccess = true;
    updateWebStatus("Temperature sensor initialized successfully");
    sensors.setResolution(roomThermometer, TEMP_RESOLUTION);

    return true;
}

/**
 * @brief Reads temperature from the DS18B20 sensor
 *
 * Requests temperature measurement from the DS18B20 sensor, waits for conversion
 * to complete, and retrieves the temperature reading. Handles disconnection errors
 * and initialization failures by returning NAN.
 *
 * @return float Temperature in Celsius degrees, or NAN if sensor is disconnected,
 *         not initialized, or reading fails
 *
 * @note Function includes a 750ms delay to allow for temperature conversion
 * @note Prints error message to Serial and updates web status on sensor failure
 */
float readTemperature()
{   
    if (!tempSensorInitSuccess)
    {
        updateWebStatus("Error: Temperature sensor not initialized");
        return NAN;
    }

    // Use non-blocking conversion with polling for optimal performance
    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();

    // Poll for conversion completion with timeout protection
    unsigned long startTime = millis();
    while (!sensors.isConversionComplete() && (millis() - startTime < 1000))
    {
        delay(10); // Small delay to prevent excessive polling
    }

    float tempC = sensors.getTempC(roomThermometer);

    if (tempC <= DEVICE_DISCONNECTED_C)
    {
        Serial.println("Error: DS18B20 disconnected");
        updateWebStatus("Error: Temperature sensor disconnected");
        return NAN;
    }

    return tempC;
}
