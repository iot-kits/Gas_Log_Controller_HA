/**
 * @file ds18b20_sensor.cpp
 * @version 2026.01.05
 * @author Karl Berger & OpenAI ChatGPT
 * @brief DS18B20 temperature sensor interface implementation
 * 
 * This module provides functionality to interface with DS18B20 temperature sensors
 * using the Dallas Temperature library. It handles sensor initialization, temperature
 * reading, and error handling for disconnected sensors.
 */
#ifdef DS18B20

#include "webSocket.h"	 // for updateWebStatus()
#include "configuration.h" // for TEMP_RESOLUTION
#include "ds18b20_sensor.h"
#include <DallasTemperature.h>
#include <OneWire.h>

// private module state
static OneWire oneWire(ONE_WIRE_BUS);
static DallasTemperature sensors(&oneWire);
static DeviceAddress roomThermometer;
static bool tempSensorInitSuccess = false;

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
bool initSensor() {
    sensors.begin();
    
    int deviceCount = sensors.getDeviceCount();
    Serial.print("Found ");
    Serial.print(deviceCount, DEC);
    Serial.println(" DS18B20 device(s).");

    if (deviceCount == 0) {
        tempSensorInitSuccess = false;
        Serial.println("Error: No DS18B20 devices found on OneWire bus");
        updateWebStatus("Error: No temperature sensor found");
        return false;
    }

    if (!sensors.getAddress(roomThermometer, 0)) {
        tempSensorInitSuccess = false;
        Serial.println("Error: Unable to find address for temperature sensor");
        updateWebStatus("Error: Temperature sensor address not found");
        return false;
    }
    
    tempSensorInitSuccess = true;
    Serial.println("DS18B20 Temperature sensor initialized successfully");
    updateWebStatus("System initializing...");
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
 *              not initialized, or reading fails
 * 
 * @note Function includes a 750ms delay to allow for temperature conversion
 * @note Prints error message to Serial and updates web status on sensor failure
 */
float readTemperature() {
    if (!tempSensorInitSuccess) {
        Serial.println("Error: Cannot read temperature - sensor not initialized");
        return NAN;
    }
    
    sensors.requestTemperatures();
    delay(750);
    float tempC = sensors.getTempC(roomThermometer);
    
    if (tempC == DEVICE_DISCONNECTED_C) {
        Serial.println("Error: DS18B20 disconnected");
        updateWebStatus("Error: Temperature sensor disconnected");
        return NAN;
    }
    
    return tempC;
}

/**
 * @brief Reads the humidity value from the sensor
 * 
 * @note The DS18B20 sensor is a temperature-only sensor and does not measure 
 *       humidity. This function always returns NAN.
 * 
 * @return float NAN (Not a Number) as DS18B20 does not support humidity
 */
float readHumidity() {
    return NAN;
}

#endif // DS18B20