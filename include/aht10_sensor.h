/**
 * @file aht10_sensor.h
 * @brief Header file for AHT10 temperature and humidity sensor interface
 * 
 * This header provides function declarations for reading temperature and humidity
 * values from an AHT10 sensor. The AHT10 is a digital temperature and humidity
 * sensor with high accuracy and reliability.
 * 
 * The implementation is conditionally compiled based on the DS18B20 build flag -
 * if DS18B20 is not defined, this AHT10 implementation is used instead.
 * @version 2026.01.03
 * @author Karl Berger & OpenAI ChatGPT
 */

#ifndef AHT10_SENSOR_H
#define AHT10_SENSOR_H

/**
 * @brief Reads the current temperature from the AHT10 sensor
 * 
 * @return float Temperature value in degrees Celsius
 */
float readTemperature();

/**
 * @brief Reads the current humidity from the AHT10 sensor
 * 
 * @return float Relative humidity value as a percentage (0-100%)
 */
float readHumidity();

#endif // AHT10_SENSOR_H