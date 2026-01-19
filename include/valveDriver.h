/**
 * @file valveDriver.h
 * @brief Valve driver interface for gas log controller
 * 
 * This header file provides the interface for controlling a valve driver system
 * in response to thermostat heat calls. The driver manages valve state transitions
 * and hardware initialization for gas log control applications.
 * 
 * @note This module requires proper hardware initialization via valveDriverBegin()
 *       before calling valveUpdate().
 * 
 * @author Karl Berger
 * @date 2025-11-20
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

/**
 * @brief Read supply voltage and return PWM duty cycle (0-255).
 *
 * Uses ADC attenuation 2.5 dB and `analogReadMilliVolts` on `PIN_VOLTAGE_SENSE`.
 */
uint8_t readVoltageDutyCycle();

#endif // VALVE_DRIVER_H