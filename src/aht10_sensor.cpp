/**
 * @file aht10_sensor.cpp
 * @brief AHT10 temperature and humidity sensor interface implementation
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

//! DS18B20 is defined in platformio.ini build_flags if DS18B20 is selected
#ifndef DS18B20 //! if not DS18B20 then AHT10

#include "aht10_sensor.h"	 // own header
#include <Adafruit_AHTX0.h>	 // <-- Needed for Adafruit_AHTX0 class
#include <Adafruit_Sensor.h> // <-- Needed for sensors_event_t
#include "webSocket.h"		 // for updateWebStatus()

static Adafruit_AHTX0 aht; // reference to the global object

void initSensor()
{
	if (!aht.begin())
	{
		Serial.println("Failed to initialize AHT10 sensor!");
		updateWebStatus("Error: AHT10 init failed");
		// fail soft: continue, readings will be NAN
	}
}

float readTemperature()
{
	sensors_event_t humidity, temp; // humidity event unused here
	aht.getEvent(&humidity, &temp); // populate temp and humidity objects
	return temp.temperature;		// return temperature in Celsius
}

float readHumidity() // return relative humidity in % NOT USED
{
	sensors_event_t humidity, temp;
	aht.getEvent(&humidity, &temp);
	return humidity.relative_humidity;
}

#endif // !DS18B20
