/**
 * @file thermostat.cpp
 * @version 2026.01.08
 * @author Karl Berger & MS Copilot
 * @brief Determines the heating system call status based on room temperature and setpoint with hysteresis.
 * 
 * This function implements a thermostat control logic with hysteresis to prevent rapid on/off cycling.
 * The heating system turns ON when the room temperature drops below (setpoint - hysteresis) and turns
 * OFF when the temperature rises above (setpoint + hysteresis).
 *
 * @param roomTemp The current room temperature in degrees (Celsius or Fahrenheit, depending on system configuration)
 * @param setpoint The desired target temperature in the same units as roomTemp
 *
 * @return true if heating is required (heat call active), false if heating should be off
 *
 * @note Uses a static variable to maintain state between calls
 * @note Hysteresis value is defined by THERMOSTAT_HYSTERESIS in configuration.h
 * @note The function maintains state between calls, so it should be called regularly with updated temperature readings
 *
 * @warning This function is not thread-safe due to the static variable. Ensure it's called from a single thread only.
 */
#include <Arduino.h>       // for Arduino core
#include "configuration.h" // For THERMOSTAT_HYSTERESIS
#include "thermostat.h"    // own header


bool thermostatHeatCall(float roomTemp, float setpoint)
{
    // Hysteresis logic: Creates a deadband around setpoint to prevent rapid cycling
    static bool heatCall = false; // Maintain state between calls for hysteresis
    if (heatCall)
    {
        // Stay on until roomTemp exceeds setpoint + hysteresis (upper threshold)
        if (roomTemp >= setpoint + THERMOSTAT_HYSTERESIS)
        {
            heatCall = false;
        }
    }
    else
    {
        // Stay off until roomTemp drops below setpoint - hysteresis (lower threshold)
        if (roomTemp <= setpoint - THERMOSTAT_HYSTERESIS)
        {
            heatCall = true;
        }
    }
      // If the desired state already matches heatCall, do nothing.

    return heatCall;
}