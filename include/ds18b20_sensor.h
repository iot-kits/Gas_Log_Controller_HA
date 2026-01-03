/**
 * @file ds18b20_sensor.h
 * @version 2026.01.03
 * @author Karl Berger & OpenAI ChatGPT
 * @brief Header file for DS18B20 temperature sensor interface
 * 
 * This header provides function declarations for reading temperature and humidity
 * from DS18B20 sensor. Note: DS18B20 is a temperature-only sensor,
 * so the humidity function may be for a different sensor or future implementation.
 */

#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

/**
 * @brief Reads temperature from the DS18B20 sensor
 * @return Temperature value in degrees Celsius as a float
 */

float readTemperature();

/**
 * @brief Reads humidity value
 * @return Humidity value as a percentage (0-100) as a float
 * @note DS18B20 sensors only measure temperature. This function
 *       may be intended for a different sensor or future implementation.
 */
float readHumidity();

#endif // DS18B20_SENSOR_H