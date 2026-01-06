/**
 * @file sensor.h
 * @brief Temperature and humidity sensor interface
 * @version 2026.01.05
 * @author Karl Berger & OpenAI ChatGPT
 *
 * This header file provides a unified interface for temperature and humidity sensors.
 * The implementation supports sensors that may not have humidity measurement capabilities,
 * returning NAN for humidity readings when not supported.
 *
 * @note This is a generic sensor interface that can be implemented for various sensor types
 * @warning Ensure initSensor() is called before attempting to read sensor values
 *
 * This header file provides function declarations for initializing and reading
 * from temperature and humidity sensors. The implementation supports sensors
 * that may not have humidity measurement capabilities.
 */

#ifndef SENSOR_H
#define SENSOR_H

/**
 * @brief Initialize the sensor hardware
 *
 * This function performs the necessary initialization of the sensor hardware
 * and communication interfaces. Must be called before attempting to read
 * sensor values.
 * 
 * @return bool True if sensor initialized successfully, false otherwise
 */
bool initSensor();

/**
 * @brief Read the current temperature from the sensor
 *
 * @return float The temperature value in degrees Celsius, or NAN if sensor read fails
 */
float readTemperature();

/**
 * @brief Read the current humidity from the sensor
 *
 * @return float The relative humidity as a percentage (0-100%).
 *               Returns NAN if humidity measurement is not supported by the sensor.
 */
float readHumidity(); // returns NAN if not supported

#endif // SENSOR_H