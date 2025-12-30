
/**
 * @fileoverview WebSocket client for Gas Log Controller, handling real-time communication
 * between the web interface and the ESP32-C3 controller.
 * 
 * This script manages:
 * - WebSocket connection lifecycle (connect, reconnect, error handling)
 * - Bidirectional message passing (status, temperature, state updates)
 * - UI synchronization (toggle buttons, temperature slider)
 * - Command transmission to the Arduino/ESP32 controller
 * 
 * @author Karl Berger
 * @version 2025-12-29
 * @requires WebSocket API
 */

/**
 * WebSocket connection instance for real-time communication with the server.
 * @type {WebSocket|undefined}
 */

/**
 * Initializes and manages the WebSocket connection to the server.
 * Automatically reconnects after 5 seconds if the connection is lost.
 * Prevents multiple simultaneous connections.
 * 
 * @function initWebSocket
 * @returns {void}
 */

/**
 * Processes incoming WebSocket messages and updates the UI accordingly.
 * Handles three message types:
 * - status: System status messages
 * - temperature: Current room temperature updates
 * - state: Full controller state synchronization (power, mode, setpoint)
 * 
 * @function processWebSocketMessage
 * @param {string} message - JSON-formatted message from the server
 * @returns {void}
 */

/**
 * Updates the active state of toggle buttons within a button group.
 * 
 * @function updateToggleGroup
 * @param {string} groupId - The ID of the toggle button group container
 * @param {string} value - The value to match against button data-value attributes
 * @returns {void}
 */

/**
 * Updates the status message display element with styling based on message content.
 * Applies CSS classes for error, warning, or normal status states.
 * 
 * @function updateStatusDisplay
 * @param {string} statusMessage - The status message to display
 * @returns {void}
 */

/**
 * Updates the room temperature display element.
 * 
 * @function updateTemperatureDisplay
 * @param {number} temperature - The temperature value in Fahrenheit
 * @returns {void}
 */

/**
 * Sends a command string to the Arduino/ESP32 controller via WebSocket.
 * Validates WebSocket connection state before sending.
 * 
 * @function sendCommand
 * @param {string} command - JSON-formatted command string
 * @returns {void}
 */

/**
 * Sets up event listeners for a toggle button group.
 * Handles button clicks and sends corresponding commands to the controller.
 * 
 * @function setupToggleGroup
 * @param {string} groupId - The ID of the toggle button group container
 * @returns {void}
 */

/**
 * Sends the current temperature setpoint value to the controller.
 * Reads value from the temperature slider element.
 * 
 * @function sendSetpoint
 * @returns {void}
 */
// WebSocket connection
let socket;

// WebSocket initialization function
function initWebSocket() {
  if (socket && socket.readyState !== WebSocket.CLOSED) {
    return; // Already connected or connecting
  }

  socket = new WebSocket("ws://" + window.location.hostname + "/ws");

  socket.onopen = function () {
    console.log("WebSocket Connected");
  };

  socket.onmessage = function (event) {
    processWebSocketMessage(event.data);
  };

  socket.onerror = function (error) {
    console.error("WebSocket Error: ", error);
  };

  socket.onclose = function () {
    console.log("WebSocket Disconnected. Reconnecting in 5 seconds...");
    setTimeout(initWebSocket, 5000);
  };
}

// Function to process WebSocket messages
function processWebSocketMessage(message) {
  try {
    const data = JSON.parse(message);

    // Handle status updates
    if (data.type === "status") {
      updateStatusDisplay(data.message);
    }

    // Handle temperature updates
    if (data.type === "temperature") {
      updateTemperatureDisplay(data.value);
    }

    // Handle slider state/color updates
    if (data.type === "sliderState") {
      updateSliderColor(data.value);
    }

    // Handle full state sync
    if (data.type === "state") {
      if (data.power) {
        updateToggleGroup("power-group", data.power);
      }
      if (data.mode) {
        updateToggleGroup("mode-group", data.mode);
      }
      if (typeof data.setpoint === "number") {
        const slider = document.getElementById("temp-slider");
        const setpointDisplay = document.getElementById("setpoint-value");
        if (slider && setpointDisplay) {
          slider.value = data.setpoint;
          setpointDisplay.textContent = `${data.setpoint}°F`;
        }
      }
      // Handle slider color state
      if (data.sliderState) {
        updateSliderColor(data.sliderState);
      }
    }
  } catch (error) {
    console.error("Error parsing WebSocket message:", error);
  }
}

function updateToggleGroup(groupId, value) {
  const group = document.getElementById(groupId);
  if (!group) return;

  const buttons = group.querySelectorAll(".toggle-button");
  buttons.forEach(btn => {
    if (btn.dataset.value.toUpperCase() === value.toUpperCase()) {
      btn.classList.add("active");
    } else {
      btn.classList.remove("active");
    }
  });
}

// Function to update status display
function updateStatusDisplay(statusMessage) {
  const statusElement = document.getElementById('status-message');
  if (statusElement) {
    statusElement.textContent = statusMessage;

    // Add status-specific styling
    statusElement.className = 'status-display';

    if (statusMessage.includes('Error') || statusMessage.includes('Alert')) {
      statusElement.classList.add('error');
    } else if (statusMessage.includes('Warning')) {
      statusElement.classList.add('warning');
    } else if (statusMessage.includes('Normal') || statusMessage.includes('Heating') || statusMessage.includes('Idle')) {
      statusElement.classList.add('normal');
    }
  } else {
    console.error("Status element not found!");
  }
}

// Function to update temperature display
function updateTemperatureDisplay(temperature) {
  const tempElement = document.getElementById('room-temp');
  if (tempElement) {
    tempElement.textContent = `${temperature.toFixed(1)}°F`;
  }
}

// Function to update slider color based on state
function updateSliderColor(state) {
  const tempDisplay = document.querySelector('.temp-display');
  if (!tempDisplay) return;

  // Remove existing color classes
  tempDisplay.classList.remove('temp-heating', 'temp-idle', 'temp-off');

  // Add appropriate class based on state
  switch (state.toUpperCase()) {
    case 'HEATING':
      tempDisplay.classList.add('temp-heating');
      break;
    case 'IDLE':
      tempDisplay.classList.add('temp-idle');
      break;
    case 'OFF':
      tempDisplay.classList.add('temp-off');
      break;
    default:
      // Default to off state
      tempDisplay.classList.add('temp-off');
  }
}

// Function to send commands to Arduino
function sendCommand(command) {
  console.log("Sending command:", command); // Debug line

  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(command);
  } else {
    console.error("WebSocket not ready. State:", socket ? socket.readyState : "null");
  }
}

// Toggle button functionality
function setupToggleGroup(groupId) {
  const group = document.getElementById(groupId);
  if (!group) return;

  const buttons = group.querySelectorAll(".toggle-button");

  buttons.forEach(button => {
    button.addEventListener("click", () => {
      buttons.forEach(btn => btn.classList.remove("active"));
      button.classList.add("active");

      // Determine message type based on group
      const messageType = groupId === "power-group" ? "power" : "mode";

      const message = JSON.stringify({
        type: messageType,
        value: button.dataset.value
      });
      sendCommand(message);
    });
  });
}

// Temperature slider functionality
const slider = document.getElementById("temp-slider");
const setpointDisplay = document.getElementById("setpoint-value");

if (slider && setpointDisplay) {
  slider.addEventListener("input", () => {
    const value = slider.value;
    setpointDisplay.textContent = `${value}°F`;
    // No WebSocket send here — only update UI on mouseup/touchend
  });
}

function sendSetpoint() {
  const value = parseInt(slider.value);
  const message = JSON.stringify({
    type: "setpoint",
    value: value
  });
  sendCommand(message);
}

slider.addEventListener("mouseup", sendSetpoint);
slider.addEventListener("touchend", sendSetpoint);

// Initialize toggle groups
setupToggleGroup("power-group");
setupToggleGroup("mode-group");

document.addEventListener('DOMContentLoaded', function () {
  initWebSocket();

  setupToggleGroup("power-group");
  setupToggleGroup("mode-group");

  // Set initial state to OFF and AUTOMATIC
  sendCommand(JSON.stringify({ type: "power", value: "OFF" }));
  sendCommand(JSON.stringify({ type: "mode", value: "AUTOMATIC" }));

});
