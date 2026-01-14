/**
 * @file webSocket.cpp
 * @version 2026.01.14
 * @author Karl Berger & MS Copilot
 * @brief WebSocket server implementation for Gas Log Controller ESP32
 *
 * This module provides WebSocket communication functionality for a gas log controller,
 * enabling real-time bidirectional communication between the ESP32 device and web clients.
 * It handles control state management, JSON message parsing, and web file serving.
 *
 * Key Features:
 * - WebSocket server on port 80 with endpoint "/ws"
 * - Real-time control state broadcasting to all connected clients
 * - JSON-based message protocol for commands and status updates
 * - Web file serving (HTML, CSS, JS, favicon) from LittleFS
 * - Automatic client cleanup and connection management
 * - Valve state control with string/enum conversion utilities
 *
 * Supported WebSocket Messages:

 * - "setpoint": {"type": "setpoint", "value": <integer>}
 * - "mode": {"type": "mode", "value": "OFF"|"MANUAL"|"THERMOSTAT"}
 *
 * Outbound Message Types:
 * - "state": Complete control state broadcast
 * - "status": Status/error messages for user feedback
 *
 * Dependencies:
 * - ESPAsyncWebServer library for HTTP/WebSocket server
 * - ArduinoJson library for JSON serialization/deserialization
 * - LittleFS for web file storage and serving
 *
 * Global State:
 * - controlState: Main controller state (power, mode, setpoint, valve)
 * - server: AsyncWebServer instance on port 80
 * - ws: AsyncWebSocket instance at "/ws" endpoint
 */
#include "webSocket.h"         // header file for this module
#include <ESPAsyncWebServer.h> // for AsyncWebServer and AsyncWebSocket
#include <LittleFS.h>          // for index.html, styles.css, and script.js
#include <ArduinoJson.h>       // for JSON formatting

//! Instantiate WebSocket server on port 80
AsyncWebServer server(80); // Create AsyncWebServer object on port 80
AsyncWebSocket ws("/ws");  // Create WebSocket object at URL /ws

// Global control state for the gas log controller
// mode: 0 = OFF, 1 = MANUAL, 2 = THERMOSTAT

ControlState controlState = {
    .mode = MODE_OFF,    // Start in OFF mode
    .setpointF = 70,     // Default setpoint temperature
    .valveState = "OFF", // Initial valve state
    .roomTempF = 0.0f};  // Initialize valve state and room temperature

void broadcastControlState()
{
    JsonDocument doc;
    doc["type"] = "state";

    // Send mode as string: OFF, MANUAL, or THERMOSTAT
    if (controlState.mode == 0)
    {
        doc["mode"] = "OFF";
    }
    else if (controlState.mode == 1)
    {
        doc["mode"] = "MANUAL";
    }
    else
    {
        doc["mode"] = "THERMOSTAT";
    }

    doc["setpoint"] = controlState.setpointF;
    doc["valveState"] = controlState.valveState;
    doc["roomTemp"] = controlState.roomTempF;

    String payload;
    serializeJson(doc, payload);

    notifyAllClients(payload);
    Serial.println("Broadcasted control state: " + payload);
}

// Helper function to update valve state only when changed
void setRoomTempColor(const char *newState)
{
    String newStateStr(newState);
    if (controlState.valveState != newStateStr)
    {
        controlState.valveState = newStateStr;
        broadcastControlState();
    }
}

void notifyAllClients(const String &message)
{
    if (ws.count() > 0)
    {
        ws.textAll(message);
    }
}

//! WebSocket event handler
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    switch (type)
    {
    case WS_EVT_CONNECT:
        Serial.println("WS client connected");
        Serial.printf("Free Heap: %u\n", ESP.getFreeHeap());
        {
            JsonDocument doc;
            doc["type"] = "status";
            doc["message"] = "Connected";
            String connectionWelcome;
            serializeJson(doc, connectionWelcome);
            if (client)
            {
                client->text(connectionWelcome);
            }
        }
        // Send current control state (includes latest roomTemp)
        broadcastControlState();
        break;

    case WS_EVT_DISCONNECT:
        Serial.println("WS client disconnected");
        break;

    case WS_EVT_DATA:
    {
        String message = String((char *)data, len);
        Serial.printf("WS msg rcvd: %s\n", message.c_str());

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, message);

        if (error)
        {
            Serial.printf("JSON parse failed: %s\n", error.c_str());
            return;
        }

        const char *msgType = doc["type"] | "";

        if (strcmp(msgType, "setpoint") == 0)
        {
            if (doc["value"].is<int>())
            {
                controlState.setpointF = doc["value"].as<int>();
                broadcastControlState();
            }
        }
        else if (strcmp(msgType, "mode") == 0)
        {
            const char *value = doc["value"] | "";
            String valueUpper(value);
            valueUpper.toUpperCase();

            if (valueUpper == "OFF")
            {
                controlState.mode = MODE_OFF;
                Serial.println("Mode OFF command received");
                setRoomTempColor("OFF");
                updateWebStatus("System Off");
            }
            else if (valueUpper == "MANUAL")
            {
                controlState.mode = MODE_ON;
                Serial.println("Mode MANUAL command received");
                setRoomTempColor("HEATING");
                updateWebStatus("Heating");
            }
            else if (valueUpper == "THERMOSTAT")
            {
                if (!tempSensorAvailable)
                {
                    updateWebStatus("Sensor failed");
                    controlState.mode = MODE_OFF; // Force to OFF if no sensor
                    setRoomTempColor("OFF");
                }
                else
                {
                    controlState.mode = MODE_THERMOSTAT;
                    Serial.println("Mode THERMOSTAT command received");
                    setRoomTempColor("IDLE");
                    updateWebStatus("Idle");
                }
            }
            broadcastControlState();
        }
        else
        {
            Serial.printf("Unknown message type: %s\n", msgType);
        }
    }
    break;

    default:
        break;
    }
}

//! Initialize WebSocket and serve UI files
void websocketBegin()
{
    if (!LittleFS.begin())
    {
        Serial.println("Error mounting LittleFS");
        return;
    }

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/index.html", "text/html"); });

    server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/styles.css", "text/css"); });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/script.js", "application/javascript"); });

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(LittleFS, "/favicon.ico", "image/x-icon"); });

    ws.onEvent(onWsEvent);
    server.addHandler(&ws);
    server.begin();
}

//! Periodically clean up disconnected clients
void websocketCleanup()
{
    static unsigned long cleanTime = millis() + 5000;
    if (millis() > cleanTime)
    {
        ws.cleanupClients();
        cleanTime = millis() + 5000;
    }
}

void updateWebStatus(const String &statusMessage)
{
    // Static local variable to track last status message and prevent duplicates
    static String lastStatusMessage = "";

    // Only send if status changed
    if (statusMessage != lastStatusMessage)
    {
        JsonDocument doc;
        doc["type"] = "status";
        doc["message"] = statusMessage;

        String jsonPayload;
        serializeJson(doc, jsonPayload);

        Serial.println("Sending WebSocket message: " + jsonPayload);
        notifyAllClients(jsonPayload);
        Serial.println("Status: " + statusMessage);
        lastStatusMessage = statusMessage;
    }
}
