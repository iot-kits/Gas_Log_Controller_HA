/**
 * @file aht10_sensor.h
 * @brief AHT10 temperature and humidity sensor interface
 * @version 2026.01.05
 * @author Karl Berger & OpenAI ChatGPT
 *
 * This header provides function declarations for reading temperature and humidity
 * from the AHT10 sensor.
 */

#ifndef AHT10_SENSOR_H
#define AHT10_SENSOR_H

/**
 * @brief Initializes the AHT10 sensor
 * @return bool True if sensor initialized successfully, false otherwise
 */
bool initSensor();

/**
 * @brief Reads temperature from the AHT10 sensor
 * @return Temperature value in degrees Celsius as a float, or NAN if read fails
 */
float readTemperature();

/**
 * @brief Reads humidity from the AHT10 sensor
 * @return Humidity value as a percentage (0-100) as a float, or NAN if read fails
 */
float readHumidity();

#endif // AHT10_SENSOR_H