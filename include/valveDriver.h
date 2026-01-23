/**
 * @file valveDriver.h
 * @brief Valve driver interface for gas log controller
 * @author Karl Berger
 * @date 2025-11-20
 * 
 * This header file provides the interface for controlling a valve driver system
 * in response to thermostat heat calls. The driver manages valve state transitions
 * and hardware initialization for gas log control applications. Voltage applied to
 * the valve is set by an H-bridge with PWM with duty cycle determined from the
 * power supply voltage.
 * 
 * @note This module requires proper hardware initialization via valveDriverBegin()
 *       before calling valveUpdate().
 * 
 */
#ifndef VALVE_DRIVER_H
#define VALVE_DRIVER_H

/**
 * @brief Initializes the valve driver hardware and state.
 *        Must be called once during setup.
 */
void valveDriverBegin();

/**
 * @brief Main control function to be called periodically.
 *        Detects changes in thermostat state and operates valve accordingly.
 */
void valveOpenRequest(bool heatCall);

#endif // VALVE_DRIVER_H