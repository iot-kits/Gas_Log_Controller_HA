/**
 * @file aht10_sensor.cpp
 * @brief AHT10 temperature and humidity sensor interface implementation
 * @author Karl Berger & Copilot
 * @date 2026-01-05
 * 
 * This file provides the implementation for interfacing with the AHT10 temperature
 * and humidity sensor using the Adafruit AHTX0 library. The implementation is
 * conditionally compiled based on the DS18B20 build flag - if DS18B20 is not
 * defined, this AHT10 implementation is used instead.
 * 
 * The sensor provides both temperature (in Celsius) and relative humidity (in %)
 * readings, though currently only temperature readings are actively used in the
 * gas log controller application.
 * 
 * @note This implementation uses "fail soft" error handling - if sensor
 *       initialization fails, the program continues but readings will return NaN.
 * 
 * @dependencies
 * - Adafruit_AHTX0 library for sensor communication
 * - Adafruit_Sensor library for sensor event structures
 * - webSocket module for status updates
 */

#ifndef DS18B20 //! if not DS18B20 then AHT10

#include "aht10_sensor.h"	 // own header
#include <Adafruit_AHTX0.h>	 // <-- Needed for Adafruit_AHTX0 class
#include <Adafruit_Sensor.h> // <-- Needed for sensors_event_t
#include "webSocket.h"		 // for updateWebStatus()

static Adafruit_AHTX0 aht; // reference to the global object
static bool ahtSensorInitSuccess = false;

/**
 * @brief Initializes the AHT10 sensor
 * 
 * @return bool True if sensor initialized successfully, false otherwise
 */
bool initSensor()
{
    if (!aht.begin())
    {
        ahtSensorInitSuccess = false;
        Serial.println("Error: Failed to initialize AHT10 sensor!");
        updateWebStatus("Error: AHT10 init failed");
        return false;
    }
    
    ahtSensorInitSuccess = true;
    Serial.println("AHT10 sensor initialized successfully");
    updateWebStatus("System initializing...");
    return true;
}

/**
 * @brief Reads temperature from the AHT10 sensor
 * 
 * @return float Temperature in Celsius, or NAN if sensor not initialized or read fails
 */
float readTemperature()
{
    if (!ahtSensorInitSuccess) {
        Serial.println("Error: Cannot read temperature - AHT10 sensor not initialized");
        return NAN;
    }
    
    sensors_event_t humidity, temp; // humidity event unused here
    aht.getEvent(&humidity, &temp); // populate temp and humidity objects
    return temp.temperature;		// return temperature in Celsius
}

/**
 * @brief Reads humidity from the AHT10 sensor
 * 
 * @return float Relative humidity as a percentage (0-100), or NAN if sensor not initialized or read fails
 */
float readHumidity()
{
    if (!ahtSensorInitSuccess) {
        Serial.println("Error: Cannot read humidity - AHT10 sensor not initialized");
        return NAN;
    }
    
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    return humidity.relative_humidity;
}

#endif // !DS18B20