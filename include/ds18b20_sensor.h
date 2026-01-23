/**
 * @file ds18b20_sensor.h
 * @version 2026.01.05
 * @author Karl Berger & OpenAI ChatGPT
 * @brief DS18B20 temperature sensor interface declarations
 *
 * Provides initialization and temperature-read function declarations for the
 * DS18B20 sensor used by the Gas Log Controller project.
 */

#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

/**
 * @brief Initialize the DS18B20 sensor and detect the device on the bus.
 *
 * Performs bus initialization and device discovery. Also configures the
 * desired resolution for the found sensor.
 *
 * @return true if sensor initialized and device found, false otherwise
 */
bool initSensor();

/**
 * @brief Read temperature from the DS18B20 sensor.
 *
 * Requests a temperature conversion (non-blocking in implementation) and
 * returns the measured temperature in degrees Celsius.
 *
 * @return Temperature in degrees Celsius, or NAN if the sensor is not
 *         initialized or a read error occurs.
 */
float readTemperature();

#endif // DS18B20_SENSOR_H