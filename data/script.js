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
    tempElement.textContent = temperature.toFixed(1);
  }
}

// Function to update dial colors and state based on valve state
function setRoomTempBackground(state) {
  const dialRing = document.querySelector(".dial-ring");
  const dialInner = document.querySelector(".dial-inner");
  const tempState = document.getElementById("temp-state");

  if (!dialRing || !dialInner || !tempState) return;

  // Update based on state
  switch (state.toUpperCase()) {
    case "HEATING":
      // Red/orange heating gradient
      dialRing.style.background = `conic-gradient(
        from 220deg,
        #ffc371 0deg,
        #ff7f50 120deg,
        #e53935 260deg,
        #ffc371 360deg
      )`;
      dialRing.style.filter = "drop-shadow(0 0 20px rgba(229, 57, 53, 0.35))";
      dialInner.style.background = `radial-gradient(
        circle at top,
        #ffffff 0,
        #eef1f6 60%,
        #e1e5ee 100%
      )`;
      tempState.textContent = "Heating";
      tempState.style.color = "var(--accent-heating)";
      break;

    case "IDLE":
      // Blue idle gradient
      dialRing.style.background = `conic-gradient(
        from 220deg,
        #64b5f6 0deg,
        #42a5f5 120deg,
        #1e88e5 260deg,
        #64b5f6 360deg
      )`;
      dialRing.style.filter = "drop-shadow(0 0 20px rgba(30, 136, 229, 0.35))";
      dialInner.style.background = `radial-gradient(
        circle at top,
        #ffffff 0,
        #e3f2fd 60%,
        #bbdefb 100%
      )`;
      tempState.textContent = "Idle";
      tempState.style.color = "var(--accent-idle)";
      break;

    case "OFF":
      // Gray off state
      dialRing.style.background = `conic-gradient(
        from 220deg,
        #bdbdbd 0deg,
        #9e9e9e 120deg,
        #757575 260deg,
        #bdbdbd 360deg
      )`;
      dialRing.style.filter = "drop-shadow(0 0 20px rgba(158, 158, 158, 0.25))";
      dialInner.style.background = `radial-gradient(
        circle at top,
        #fafafa 0,
        #f5f5f5 60%,
        #eeeeee 100%
      )`;
      tempState.textContent = "Off";
      tempState.style.color = "var(--accent-off)";
      break;

    default:
      // Default to OFF
      dialRing.style.background = `conic-gradient(
        from 220deg,
        #bdbdbd 0deg,
        #9e9e9e 120deg,
        #757575 260deg,
        #bdbdbd 360deg
      )`;
      dialRing.style.filter = "drop-shadow(0 0 20px rgba(158, 158, 158, 0.25))";
      dialInner.style.background = `radial-gradient(
        circle at top,
        #fafafa 0,
        #f5f5f5 60%,
        #eeeeee 100%
      )`;
      tempState.textContent = "Off";
      tempState.style.color = "var(--accent-off)";
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
