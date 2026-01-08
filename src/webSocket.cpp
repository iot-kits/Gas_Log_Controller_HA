/**
 * @file webSocket.cpp
 * @version 2026.01.08
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
 * - "power": {"type": "power", "value": "ON"|"OFF"}
 * - "setpoint": {"type": "setpoint", "value": <integer>}
 * - "mode": {"type": "mode", "value": "automatic"|"manual"}
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

// Helper function to convert enum to string for JSON/WebSocket
const char* valveStateToString(ValveState state)
{
    switch (state)
    {
        case ValveState::OFF:     return "OFF";
        case ValveState::IDLE:    return "IDLE";  
        case ValveState::HEATING: return "HEATING";
        default:                  return "OFF";
    }
}

// Global control state for the gas log controller

ControlState controlState = {
    .powerOn = false,
    .autoMode = true,
    .setpointF = 70,
    .valveState = ValveState::OFF}; // Initialize valve state

void broadcastControlState()
{
    JsonDocument doc;
    doc["type"] = "state";
    doc["power"] = controlState.powerOn ? "on" : "off";
    doc["mode"] = controlState.autoMode ? "automatic" : "manual";
    doc["setpoint"] = controlState.setpointF;
    doc["valveState"] = valveStateToString(controlState.valveState);

    String payload;
    serializeJson(doc, payload);

    notifyAllClients(payload);
    Serial.println("Broadcasted control state: " + payload);
}

// Helper function to update valve state only when changed
void setRoomTempColor(const char *newState)
{
    ValveState newEnum;
    if (strcmp(newState, "OFF") == 0) newEnum = ValveState::OFF;
    else if (strcmp(newState, "IDLE") == 0) newEnum = ValveState::IDLE;
    else if (strcmp(newState, "HEATING") == 0) newEnum = ValveState::HEATING;
    else newEnum = ValveState::OFF; // Default fallback
    
    if (controlState.valveState != newEnum)
    {
        controlState.valveState = newEnum;
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
            doc["message"] = "Connected to Gas Log Controller";
            String connectionWelcome;
            serializeJson(doc, connectionWelcome);
            if (client)
            {
                client->text(connectionWelcome);
            }
        }
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

        if (strcmp(msgType, "power") == 0)
        {
            const char *value = doc["value"] | "";
            if (strcmp(value, "ON") == 0)
            {
                controlState.powerOn = true;
                Serial.println("Power ON command received");
                if (controlState.autoMode)
                {
                    setRoomTempColor("IDLE");
                    updateWebStatus("System Powered On - Automatic Mode");
                }
                else
                {
                    setRoomTempColor("HEATING");
                    updateWebStatus("Manual Mode: Heating");
                }
            }
            else if (strcmp(value, "OFF") == 0)
            {
                controlState.powerOn = false;
                setRoomTempColor("OFF");
                updateWebStatus("System Powered Off");
            }
            broadcastControlState();
        }
        else if (strcmp(msgType, "setpoint") == 0)
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
            String valueLower(value);
            valueLower.toLowerCase();

            if (valueLower == "automatic")
            {
                if (!tempSensorAvailable)
                {
                    updateWebStatus("Error: Automatic mode requires temperature sensor");
                }
                else
                {
                    controlState.autoMode = true;
                    Serial.println("Mode AUTO command received");
                    if (controlState.powerOn)
                    {
                        setRoomTempColor("IDLE");
                        updateWebStatus("Mode: Automatic - Idle");
                    }
                    else
                    {
                        setRoomTempColor("OFF");
                        updateWebStatus("Mode: Automatic - System Off");
                    }
                }
            }
            else if (valueLower == "manual")
            {
                controlState.autoMode = false;
                Serial.println("Mode MANUAL command received");
                if (controlState.powerOn)
                {
                    setRoomTempColor("HEATING");
                    updateWebStatus("Mode: Manual - Heating");
                }
                else
                {
                    setRoomTempColor("OFF");
                    updateWebStatus("Mode: Manual - System Off");
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
