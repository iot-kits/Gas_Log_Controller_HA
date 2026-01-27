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
#include "webSocket.h"     // for updateWebStatus()
#include <time.h>

// --- Global State Variable ---
bool isValveOpen = false; // Current known state of the valve

// Safety accumulation state
static unsigned long cumulativeOpenMillis = 0; // total accumulated open time (ms)
static unsigned long lastOpenedAt = 0;        // timestamp when valve last opened (ms)
static bool timeLimitActive = false;          // true when limit exceeded and valve inhibited
static unsigned long inhibitStartMillis = 0;  // when system entered inhibited hours

// Convert minutes constant to milliseconds for checks
static const unsigned long MAX_TOTAL_OPEN_MS = MAX_TOTAL_OPEN_MINUTES * 60UL * 1000UL;
static const unsigned long INHIBIT_RESET_MS = INHIBIT_RESET_MINUTES * 60UL * 1000UL;

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
  Serial.printf("duty: %u bits (%.0f%%)\n", (unsigned int)duty, 100.0f * ratio);

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
 * @brief Check whether current time is within allowed operation window.
 *
 * Returns true when operation is permitted (default between 10:00 and 23:00),
 * false when the system should be inhibited (e.g., 23:00-10:00).
 */
static bool isOperationAllowed()
{
  time_t now = time(nullptr);
  struct tm timeinfo;
  if (!localtime_r(&now, &timeinfo))
  {
    // If time is not available yet, allow operation to avoid accidental lockout
    return true;
  }
  int hour = timeinfo.tm_hour;
  if (OPERATION_ALLOWED_BEGIN_HOUR <= OPERATION_ALLOWED_END_HOUR)
  {
    return hour >= OPERATION_ALLOWED_BEGIN_HOUR && hour < OPERATION_ALLOWED_END_HOUR;
  }
  else
  {
    // Wrapped interval (not used for default 10..23 but kept for completeness)
    return hour >= OPERATION_ALLOWED_BEGIN_HOUR || hour < OPERATION_ALLOWED_END_HOUR;
  }
}

/**
 * @brief Periodic housekeeping to enforce safety timers and inhibition/reset logic.
 *
 * Call from the main loop frequently.
 */
void valveDriverLoop()
{
  // Track inhibition window start/stop
  if (!isOperationAllowed())
  {
    if (inhibitStartMillis == 0)
      inhibitStartMillis = millis();
  }
  else
  {
    inhibitStartMillis = 0; // reset when allowed
  }

  // If valve currently open, compute running total and enforce limit
  unsigned long runningOpenMs = 0;
  if (isValveOpen && lastOpenedAt != 0)
  {
    runningOpenMs = millis() - lastOpenedAt;
  }

  unsigned long totalNow = cumulativeOpenMillis + runningOpenMs;
  if (!timeLimitActive && totalNow >= MAX_TOTAL_OPEN_MS)
  {
    // Exceeded allowed cumulative open time — close valve and inhibit
    Serial.println("Time limit exceeded: closing valve and inhibiting further operation");
    // If valve is open, close now and record elapsed
    if (isValveOpen)
    {
      // Add running portion to cumulative total
      cumulativeOpenMillis += runningOpenMs;
      lastOpenedAt = 0;
      closeValve();
      isValveOpen = false;
    }
    timeLimitActive = true;
    updateWebStatus("Time limit exceeded: Valve closed");
  }

  // If limit active, check whether we've been inhibited long enough to reset
  if (timeLimitActive && inhibitStartMillis != 0)
  {
    if (millis() - inhibitStartMillis >= INHIBIT_RESET_MS)
    {
      // Reset counters and clear inhibition
      Serial.println("Inhibited long enough — resetting cumulative open time and clearing time limit");
      cumulativeOpenMillis = 0;
      timeLimitActive = false;
      inhibitStartMillis = 0;
      updateWebStatus("Time limits reset after inhibition");
    }
  }
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

  // Initialize safety tracking variables
  cumulativeOpenMillis = 0;
  lastOpenedAt = 0;
  timeLimitActive = false;
  inhibitStartMillis = 0;

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
  // If requested state already matches, nothing to do
  if (openValveRequest == isValveOpen)
    return;

  // If attempting to open, enforce schedule and cumulative limits
  if (openValveRequest)
  {
    if (!isOperationAllowed())
    {
      Serial.println("Open request blocked: outside permitted hours");
      updateWebStatus("Operation inhibited by schedule");
      // Ensure valve is closed if it somehow was open
      if (isValveOpen)
      {
        unsigned long elapsed = 0;
        if (lastOpenedAt != 0)
        {
          elapsed = millis() - lastOpenedAt;
          cumulativeOpenMillis += elapsed;
          lastOpenedAt = 0;
          Serial.printf("Accumulated open time: %lu ms (%.2f min)\n", cumulativeOpenMillis, cumulativeOpenMillis / 60000.0f);
        }
        closeValve();
        isValveOpen = false;
      }
      return;
    }

    if (timeLimitActive)
    {
      Serial.println("Open request blocked: time limit active");
      updateWebStatus("Time limit active: Valve remains closed");
      return;
    }

    // Prevent opening if we've already exceeded the limit
    if (cumulativeOpenMillis >= MAX_TOTAL_OPEN_MS)
    {
      Serial.println("Open request blocked: cumulative open time exceeded");
      updateWebStatus("Time limit exceeded: Valve closed");
      timeLimitActive = true;
      return;
    }

    // OK to open
    openValve();
    isValveOpen = true;
    lastOpenedAt = millis(); // start accumulating open time
  }
  else
  {
    // Closing request: add elapsed open time to accumulator
    if (isValveOpen && lastOpenedAt != 0)
    {
      unsigned long elapsed = millis() - lastOpenedAt;
      cumulativeOpenMillis += elapsed;
      lastOpenedAt = 0;
      Serial.printf("Accumulated open time: %lu ms (%.2f min)\n", cumulativeOpenMillis, cumulativeOpenMillis / 60000.0f);
    }
    closeValve();
    isValveOpen = false;
  }
}
