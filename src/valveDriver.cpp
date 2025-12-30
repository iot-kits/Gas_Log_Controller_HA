/*
  Gas Fireplace Valve Controller

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
    - IN1=L, IN2=L: Idle (de-energized)
    - IN1=L, IN2=H: Forward (Open Valve)
    - IN1=H, IN2=L: Reverse (Close Valve)
    - IN1=H, IN2=H: Brake (Not used)

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
bool isValveOpen = false;  // Current known state of the valve

/**
 * @brief De-energizes the DRV8871 by setting both inputs to LOW (Idle).
 */
static void deenergizeValve()
{
  // Idle mode (IN1=L, IN2=L)
  digitalWrite(HBRIDGE_IN1_PIN, LOW);
  digitalWrite(HBRIDGE_IN2_PIN, LOW);
}

/**
 * @brief Applies "Forward" voltage (IN1=H, IN2=L) to open the valve.
 */
static void openValve()
{
  Serial.print("Opening valve (");
  Serial.print(timeToOpenValve);
  Serial.println("ms)...");

  // Forward Voltage (IN1=L, IN2=H)
  digitalWrite(HBRIDGE_IN1_PIN, LOW);
  digitalWrite(HBRIDGE_IN2_PIN, HIGH);

  // Wait for travel time (non-blocking)
  unsigned long startTime = millis();
  while (millis() - startTime < timeToOpenValve)
  {
    // Allow other processing during wait
    yield();
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
  Serial.print("Closing valve (");
  Serial.print(timeToCloseValve);
  Serial.println("ms)...");

  // Reverse Voltage (IN1=H, IN2=L)
  digitalWrite(HBRIDGE_IN1_PIN, HIGH);
  digitalWrite(HBRIDGE_IN2_PIN, LOW);

  // Wait for travel time (non-blocking)
  unsigned long startTime = millis();
  while (millis() - startTime < timeToCloseValve)
  {
    // Allow other processing during wait
    yield();
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
  // Set pin modes
  pinMode(HBRIDGE_IN1_PIN, OUTPUT);
  pinMode(HBRIDGE_IN2_PIN, OUTPUT);

  // delay(5000); // Wait for system stabilization

  // --- Initial Safe State ---
  // Ensure H-Bridge is off (Idle)
  deenergizeValve();
  Serial.println("Forcing valve closed for safety startup.");
  closeValve(); // This function handles timing
  isValveOpen = false;

  Serial.println("Initialization complete. Watching for changes.");
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
 * @note Calls helper functions: openValve(), closeValve(), updateWebStatus()
 */

void valveOpenRequest(bool openValveRequest)
{
  if (openValveRequest == isValveOpen)
    return;  // Desired state already matches, do nothing

  if (openValveRequest) {
    openValve();
    isValveOpen = true;
  } else {
    closeValve();
    isValveOpen = false;
  }
}

