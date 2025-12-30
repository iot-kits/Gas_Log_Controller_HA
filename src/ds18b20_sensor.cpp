// ds18b20_sensor.cpp
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

void initSensor() {
    sensors.begin();
    Serial.print("Found ");
    Serial.print(sensors.getDeviceCount(), DEC);
    Serial.println(" devices.");

    if (!sensors.getAddress(roomThermometer, 0)) {
        tempSensorInitSuccess = false;
        Serial.println("Unable to find address for temperature sensor");
        updateWebStatus("Error: Temperature sensor failed");
    } else {
        tempSensorInitSuccess = true;
        Serial.println("DS18B20 Temperature sensor found");
        updateWebStatus("System initializing...");
        sensors.setResolution(roomThermometer, TEMP_RESOLUTION);
    }
}

float readTemperature() {
    sensors.requestTemperatures();
    delay(750);
    float tempC = sensors.getTempC(roomThermometer);
    if (tempC == DEVICE_DISCONNECTED_C) {
        Serial.println("Error: DS18B20 disconnected");
        updateWebStatus("Error: Temperature sensor read failed");
        return NAN;
    }
    return tempC;
}

float readHumidity() {
    return NAN;
}

#endif // DS18B20
