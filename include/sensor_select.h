#ifndef SENSOR_SELECT_H
#define SENSOR_SELECT_H

//! definition set by build_flags in platformio.ini

#ifdef DS18B20
  #include "ds18b20_sensor.h"
#else
  #include "aht10_sensor.h"
#endif

#endif // SENSOR_SELECT_H