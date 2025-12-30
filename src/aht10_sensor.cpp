// aht10_sensor.cpp
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
