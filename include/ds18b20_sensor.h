/**
 * @file ds18b20_sensor.h
 * @version 2026.01.05
 * @author Karl Berger & OpenAI ChatGPT
 * @brief Header file for DS18B20 temperature sensor interface
 * 
 * This header provides function declarations for reading temperature and humidity
 * from DS18B20 sensor. Note: DS18B20 is a temperature-only sensor,
 * so the humidity function returns NAN.
 */

#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

/**
 * @brief Initializes the DS18B20 sensor
 * @return bool True if sensor initialized successfully, false otherwise
 */
bool initSensor();

/**
 * @brief Reads temperature from the DS18B20 sensor
 * @return Temperature value in degrees Celsius as a float, or NAN if read fails
 */
float readTemperature();

#endif // DS18B20_SENSOR_H