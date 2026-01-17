/**
 * @file thermostat.h
 * @version 2026.01.17
 * @author Karl Berger
 * @brief Thermostat control interface for heating system management.
 * 
 * This header file provides the interface for thermostat functionality,
 * including temperature-based heating control decisions.
 */
#ifndef THERMOSTAT_H
#define THERMOSTAT_H

/**
 * @brief Determines if heating should be activated based on temperature comparison.
 * 
 * Compares the current room temperature against the desired setpoint temperature
 * to determine whether the heating system should be called to activate.
 * 
 * @param roomTemp The current room temperature in degrees Fahrenheit.
 * @param setpoint The desired target temperature in degrees Fahrenheit.
 * @return true if heating should be activated (roomTemp < setpoint - THERMOSTAT_HYSTERESIS).
 * @return false if heating should remain off (roomTemp >= setpoint + THERMOSTAT_HYSTERESIS).
 */
bool thermostatHeatCall(float roomTemp, float setpoint);

#endif // THERMOSTAT_H