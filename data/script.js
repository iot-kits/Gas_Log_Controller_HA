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

    // Handle full state sync
    if (data.type === "state") {
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
      // Handle valve state for room temp background color
      if (data.valveState) {
        setRoomTempBackground(data.valveState);
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
  buttons.forEach((btn) => {
    if (btn.dataset.value.toUpperCase() === value.toUpperCase()) {
      btn.classList.add("active");
    } else {
      btn.classList.remove("active");
    }
  });
}

// Function to update status display
function updateStatusDisplay(statusMessage) {
  const statusElement = document.getElementById("status-message");
  if (statusElement) {
    statusElement.textContent = statusMessage;

    // Add status-specific styling
    statusElement.className = "status-display";

    if (statusMessage.includes("Error") || statusMessage.includes("Alert")) {
      statusElement.classList.add("error");
    } else if (statusMessage.includes("Warning")) {
      statusElement.classList.add("warning");
    } else if (
      statusMessage.includes("Normal") ||
      statusMessage.includes("Heating") ||
      statusMessage.includes("Idle")
    ) {
      statusElement.classList.add("normal");
    }
  } else {
    console.error("Status element not found!");
  }
}

// Function to update temperature display
function updateTemperatureDisplay(temperature) {
  const tempElement = document.getElementById("room-temp");
  if (tempElement) {
    tempElement.textContent = `${temperature.toFixed(1)}°F`;
  }
}

// Function to update room temperature background color based on state
function setRoomTempBackground(state) {
  const tempElement = document.querySelector(".temp-display");
  if (!tempElement) return;

  // Remove existing color classes
  tempElement.classList.remove("temp-heating", "temp-idle", "temp-off");

  // Add appropriate class based on state
  switch (state.toUpperCase()) {
    case "HEATING":
      tempElement.classList.add("temp-heating");
      break;
    case "IDLE":
      tempElement.classList.add("temp-idle");
      break;
    case "OFF":
      tempElement.classList.add("temp-off");
      break;
    default:
      tempElement.classList.add("temp-off");
  }
}

// Function to send commands to Arduino
function sendCommand(command) {
  console.log("Sending command:", command); // Debug line

  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(command);
  } else {
    console.error(
      "WebSocket not ready. State:",
      socket ? socket.readyState : "null"
    );
  }
}

// Toggle button functionality
function setupToggleGroup(groupId) {
  const group = document.getElementById(groupId);
  if (!group) return;

  const buttons = group.querySelectorAll(".toggle-button");

  buttons.forEach((button) => {
    button.addEventListener("click", () => {
      buttons.forEach((btn) => btn.classList.remove("active"));
      button.classList.add("active");

      // Send mode command
      const message = JSON.stringify({
        type: "mode",
        value: button.dataset.value,
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
    value: value,
  });
  sendCommand(message);
}

slider.addEventListener("mouseup", sendSetpoint);
slider.addEventListener("touchend", sendSetpoint);

// Initialize toggle group
setupToggleGroup("mode-group");

document.addEventListener("DOMContentLoaded", function () {
  initWebSocket();

  setupToggleGroup("mode-group");

  // Set initial state to OFF
  sendCommand(JSON.stringify({ type: "mode", value: "OFF" }));
});
