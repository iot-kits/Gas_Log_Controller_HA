/*
  @file Gas Fireplace Valve Controller
  @author Karl Berger
  @version 2026.01.19

  This code controls a gas valve that requires timed pulses of
  positive or negative voltage to open and close.

  Hardware Assumptions:
  1. An H-Bridge (like DRV8871) is used to reverse polarity.
     - Arduino PIN_VALVE_IN1 -> DRV8871 IN1
     - Arduino PIN_VALVE_IN2 -> DRV8871 IN2
     - DRV8871 OUT1 -> Valve Terminal 1
     - DRV8871 OUT2 -> Valve Terminal 2
     (DRV8871 VM connected to valve power supply)

  2. DRV8871 Logic:
    - IN1=L,   IN2=L:   Idle (de-energized)
    - IN1=L,   IN2=PWM: Forward (Open Valve)
    - IN1=PWM, IN2=L:   Reverse (Close Valve)
    - IN1=H,   IN2=H:   Brake (Not used)

  Main Logic:
  - Uses state-change detection (edge detection) to act only
    when the thermostat signal *changes*.
  - When heat is called (LOW -> HIGH):
    - Applies "Forward" voltage for `timeToOpen` milliseconds.
    - Sets state to OPEN.
  - When call for heat stops (HIGH -> LOW):
    - Applies "Reverse" voltage for `timeToClose` milliseconds.
    - Sets state to CLOSED.
  - The valve is de-energized (Idle) after each operation.
  - On startup, the system forces the valve closed for safety,
    then checks the thermostat and opens the valve if needed
    to match the initial state.
*/

#include <Arduino.h>       // for Arduino core
#include "valveDriver.h"   // own header
#include "configuration.h" // for pin definitions and timing

// --- Global State Variables ---
bool isValveOpen = false; // Current known state of the valve

// LEDC (PWM) channels for H-bridge inputs
static const int HBRIDGE_LEDC_CH1 = 0; // channel for HBRIDGE_IN1_PIN
static const int HBRIDGE_LEDC_CH2 = 1; // channel for HBRIDGE_IN2_PIN

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

  // Reverse Voltage (IN1=H, IN2=L)
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

// Read supply voltage and return PWM duty cycle (0-255)
// Uses ADC attenuation 2.5 dB and `analogReadMilliVolts`.
uint8_t readVoltageDutyCycle()
{
  const int N = 10;
  unsigned long sum = 0;
  for (int i = 0; i < N; ++i)
  {
    sum += analogReadMilliVolts(PIN_VOLTAGE_SENSE);
    delay(10);
  }
  unsigned long avg_mV = sum / (unsigned long)N;

  // supplyVoltage in volts = voltageDividerRatio * avg_mV (mV) / 1000
  float supplyVoltage = (voltageDividerRatio * (float)avg_mV) / 1000.0f;
  
  Serial.printf("avg_mV: %lu mV, ", (unsigned long)avg_mV);

  Serial.printf("Supply Voltage: %.2f V, ", supplyVoltage);


  if (supplyVoltage <= 0.0f)
  {
    return 0;
  }

  // Duty cycle ratio (0..1) = valveVoltage / supplyVoltage
  float ratio = valveVoltage / supplyVoltage;
  ratio = constrain(ratio, 0.0f, 1.0f);

  // Map to 0..255 PWM duty (avoid math library by using simple rounding)
  uint8_t duty = (uint8_t)(ratio * 255.0f + 0.5f);
  Serial.printf("duty: %u, ", duty);
  return duty;
}

/**
 * @brief Updates the valve state based on thermostat heat call status.
 *
 * This function monitors the thermostat heat call signal and performs edge detection
 * to control the gas valve. It only acts on state changes (rising or falling edges)
 * to avoid redundant valve operations.
 *
 * @param heatCall Current state of the thermostat heat call (true = heating requested, false = idle)
 *
 * Behavior:
 * - Rising Edge (heat call starts): Opens the valve if currently closed and updates status
 * - Falling Edge (heat call stops): Closes the valve if currently open and updates status
 * - No Change: Takes no action
 *
 * The function maintains internal state variables (isHeatCalled, isValveOpen) to track
 * previous states and prevent duplicate operations.
 *
 * @note Requires global state variables: isHeatCalled, isValveOpen
 * @note Calls helper functions: openValve(), closeValve()
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
