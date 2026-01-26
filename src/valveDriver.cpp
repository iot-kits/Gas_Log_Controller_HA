/**
 * @file valveDriver.cpp
 * @version 2026.01.23
 * @author Karl Berger
 * @brief Valve driver implementation for Gas Log Controller
 *
 * Controls a gas valve using an H-bridge (DRV8871). Uses edge-detection
 * to perform operations only when the thermostat request changes.
 *
 * Hardware assumptions:
 *  - H-bridge inputs controlled by `PIN_HBRIDGE_IN1` / `PIN_HBRIDGE_IN2`
 *  - PWM is used on the appropriate H-bridge input to apply controlled voltage
 *
 * Behavior:
 *  - On rising edge (heat requested): apply forward voltage for
 *    `timeToOpenValve` ms then de-energize (idle).
 *  - On falling edge (heat removed): apply reverse voltage for
 *    `timeToCloseValve` ms then de-energize (idle).
 *  - On startup the valve is forced closed for safety.
 */

#include <Arduino.h>       // for Arduino core
#include "configuration.h" // for pin definitions and timing
#include "valveDriver.h"   // own header

// --- Global State Variable ---
bool isValveOpen = false; // Current known state of the valve

// LEDC (PWM) channels for H-bridge inputs
static const int HBRIDGE_LEDC_CH1 = 0; // channel for HBRIDGE_IN1_PIN
static const int HBRIDGE_LEDC_CH2 = 1; // channel for HBRIDGE_IN2_PIN

/**
 * @brief Read supply voltage and return PWM duty cycle (0-255).
 *
 * Uses ADC attenuation 2.5 dB and `analogReadMilliVolts()` on
 * `PIN_VOLTAGE_SENSE`. Computes a duty value that attempts to produce
 * `valveVoltage` at the valve given the measured supply voltage.
 *
 * @return uint8_t PWM duty cycle (0..255)
 */
uint8_t readVoltageDutyCycle()
{
  const unsigned long N = 10UL; // number of samples for averaging
  unsigned long sum = 0;        // sum of mV readings
  for (unsigned long i = 0; i < N; ++i)
  {
    sum += analogReadMilliVolts(PIN_VOLTAGE_SENSE);
    delay(10);
  }
  unsigned long avg_mV = sum / N; // smoothed reading

  // Use float for the voltage calculation: sufficient precision, lower cost
  float supplyVoltage = static_cast<float>(avg_mV) * 0.001f; // volts
  supplyVoltage *= static_cast<float>(voltageDividerRatio);

  // Duty cycle ratio (0..1) = valveVoltage / supplyVoltage (float precision)
  float ratio = static_cast<float>(valveVoltage) / supplyVoltage;
  if (ratio < 0.0f)
    ratio = 0.0f;
  else if (ratio > 1.0f)
    ratio = 1.0f;

  // Map to 0..255 PWM duty (rounding)
  uint8_t duty = static_cast<uint8_t>(ratio * 255.0f + 0.5f);
  Serial.printf("avg_mV: %lu mV\n", (unsigned long)avg_mV);
  Serial.printf("Supply Voltage: %.2f V\n", supplyVoltage);
  Serial.printf("duty: %u%%\n", duty);
  return duty;
}

/**
 * @brief De-energizes the DRV8871 by setting both inputs to LOW (Idle).
 */
static void deenergizeValve()
{
  // Idle mode (IN1=L, IN2=L)
  digitalWrite(PIN_HBRIDGE_IN1, LOW);
  digitalWrite(PIN_HBRIDGE_IN2, LOW);
  // Ensure PWM outputs are zeroed
  ledcWrite(HBRIDGE_LEDC_CH1, 0);
  ledcWrite(HBRIDGE_LEDC_CH2, 0);
}

/**
 * @brief Applies "Forward" voltage (IN1=L, IN2=PWM) to open the valve.
 */
static void openValve()
{
  Serial.printf("Opening valve (%lu ms)...\n", timeToOpenValve);

  // Forward Voltage (IN1=L, IN2=PWM)
  uint8_t duty = readVoltageDutyCycle();
  digitalWrite(PIN_HBRIDGE_IN1, LOW);
  ledcWrite(HBRIDGE_LEDC_CH2, duty);

  // Wait for travel time (non-blocking)
  unsigned long startTime = millis();
  while (millis() - startTime < timeToOpenValve)
  {
    yield(); // Allow other processing during wait
  }

  // Remove power (Idle)
  deenergizeValve();
  Serial.println("...Valve OPEN.");
}

/**
 * @brief Applies "Reverse" voltage (IN1=L, IN2=H) to close the valve.
 */
static void closeValve()
{
  Serial.printf("Closing valve (%lu ms)...\n", timeToCloseValve);

  // Reverse Voltage (IN1=PWM, IN2=L)
  // Read duty and apply PWM: IN2=LOW, PWM on IN1
  uint8_t duty = readVoltageDutyCycle();
  digitalWrite(PIN_HBRIDGE_IN2, LOW);
  ledcWrite(HBRIDGE_LEDC_CH1, duty);

  // Wait for travel time (non-blocking)
  unsigned long startTime = millis();
  while (millis() - startTime < timeToCloseValve)
  {
    yield(); // Allow other processing during wait
  }

  // Remove power (Idle)
  deenergizeValve();
  Serial.println("...Valve CLOSED.");
}

/**
 * @brief Initializes the valve driver hardware and sets it to a safe state.
 *
 * This function configures the H-Bridge control pins as outputs and ensures
 * the valve is in a known safe state by closing it during system startup.
 *
 * @details The initialization sequence:
 *          1. Configures HBRIDGE_IN1_PIN and HBRIDGE_IN2_PIN as OUTPUT pins
 *          2. Deenergizes the H-Bridge to prevent unintended operation
 *          3. Closes the valve completely for safety
 *          4. Sets the valve state flag to closed (isValveOpen = false)
 *
 * @note This function should be called once during system initialization
 *       before any valve operations are performed.
 * @note Debug messages are printed to Serial for initialization tracking.
 *
 * @see deenergizeValve()
 * @see closeValve()
 */
void valveDriverBegin()
{
  // Configure ADC for voltage sensing (0 to 1.05V input range 2.5 dB attenuation)
  pinMode(PIN_VOLTAGE_SENSE, INPUT);
  analogSetPinAttenuation(PIN_VOLTAGE_SENSE, ADC_2_5db);

  // Set pin modes
  pinMode(PIN_HBRIDGE_IN1, OUTPUT);
  pinMode(PIN_HBRIDGE_IN2, OUTPUT);

  // Configure PWM channels (2 kHz, 8-bit resolution)
  const int pwmFreq = 2000;
  const int pwmResolution = 8; // duty 0-255
  ledcSetup(HBRIDGE_LEDC_CH1, pwmFreq, pwmResolution);
  ledcSetup(HBRIDGE_LEDC_CH2, pwmFreq, pwmResolution);
  ledcAttachPin(PIN_HBRIDGE_IN1, HBRIDGE_LEDC_CH1);
  ledcAttachPin(PIN_HBRIDGE_IN2, HBRIDGE_LEDC_CH2);

  // --- Initial Safe State ---
  // Ensure H-Bridge is off (Idle)
  deenergizeValve();
  Serial.println("Forcing valve closed for safety startup.");
  closeValve(); // This function handles timing
  isValveOpen = false;

  Serial.println("Initialization complete. Watching for changes.");
}

/**
 * @brief Updates the valve state based on a requested open/close command.
 *
 * Acts on state changes only: opens the valve when `openValveRequest` is
 * true and closes it when `openValveRequest` is false. No action is taken
 * if the requested state matches the current `isValveOpen` state.
 *
 * @param openValveRequest true to open the valve, false to close it
 */
void valveOpenRequest(bool openValveRequest)
{
  if (openValveRequest == isValveOpen)
    return; // Desired state already matches, do nothing

  if (openValveRequest)
  {
    openValve();
    isValveOpen = true;
  }
  else
  {
    closeValve();
    isValveOpen = false;
  }
}
