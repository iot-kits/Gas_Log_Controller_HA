#ifndef SENSOR_H
#define SENSOR_H

void initSensor();
float readTemperature();
float readHumidity(); // returns NAN if not supported

#endif // SENSOR_H