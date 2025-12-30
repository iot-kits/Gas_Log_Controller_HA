#include "webSocket.h"		   // header file for this module
#include <ESPAsyncWebServer.h> // for AsyncWebServer and AsyncWebSocket
#include <LittleFS.h>		   // for index.html, styles.css, and script.js
#include <ArduinoJson.h>	   // for JSON formatting

//! Instantiate WebSocket server on port 80
AsyncWebServer server(80); // Create AsyncWebServer object on port 80
AsyncWebSocket ws("/ws");  // Create WebSocket object at URL /ws

// Global control state for the gas log controller

ControlState controlState = {
	.powerOn = false,
	.autoMode = true,
	.setpointF = 70,
	.sliderState = "OFF"}; // Initialize slider state

/**
 * @brief Formats the current control state as a JSON string.
 *
 * Creates a JSON object containing the system's state information including
 * power status, operational mode, and temperature setpoint.
 *
 * @return String JSON formatted string with fields:
 *         - type: "state"
 *         - power: "on" or "off"
 *         - mode: "automatic" or "manual"
 *         - setpoint: temperature value in Fahrenheit
 *
 * @example
 * {"type":"state","power":"off","mode":"automatic","setpoint":70}
 */

/**
 * @brief Broadcasts the current control state to all connected WebSocket clients.
 *
 * Formats the current control state as JSON and sends it to all connected
 * WebSocket clients via the notifyClients function. Also logs the broadcast
 * message to the serial console for debugging purposes.
 *
 * @note This function depends on the webSocket module's notifyClients function.
 */
void broadcastControlState()
{
    JsonDocument doc;
    doc["type"] = "state";
    doc["power"] = controlState.powerOn ? "on" : "off";
    doc["mode"] = controlState.autoMode ? "automatic" : "manual";
    doc["setpoint"] = controlState.setpointF;
    doc["sliderState"] = controlState.sliderState;
    
    String payload;
    serializeJson(doc, payload);
    
    notifyAllClients(payload);
    Serial.println("Broadcasted control state: " + payload);
}

void notifySingleClient(AsyncWebSocketClient *client, const String &message)
{
	if (client)
	{
		client->text(message);
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
            String welcome;
            serializeJson(doc, welcome);
            notifySingleClient(client, welcome);
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
        
        if (error) {
            Serial.printf("JSON parse failed: %s\n", error.c_str());
            return;
        }

        const char* msgType = doc["type"] | "";
        
        if (strcmp(msgType, "power") == 0)
        {
            const char* value = doc["value"] | "";
            if (strcmp(value, "ON") == 0)
            {
                controlState.powerOn = true;
                Serial.println("Power ON command received");
                // updateWebStatus("Gas Log controller ON");
            }
            else if (strcmp(value, "OFF") == 0)
            {
                controlState.powerOn = false;
                Serial.println("Power OFF command received");
                // updateWebStatus("Gas Log controller OFF");
            }
            broadcastControlState();
        }
        else if (strcmp(msgType, "setpoint") == 0)
        {
            if (doc["value"].is<int>())
            {
                controlState.setpointF = doc["value"].as<int>();
                Serial.printf("Setpoint command received: %d°F\n", controlState.setpointF);
                broadcastControlState();
            }
        }
        else if (strcmp(msgType, "mode") == 0)
        {
            const char* value = doc["value"] | "";
            String valueLower(value);
            valueLower.toLowerCase();
            
            if (valueLower == "automatic")
            {
                controlState.autoMode = true;
                Serial.println("Mode AUTO command received");
                updateWebStatus("Mode: Automatic");
            }
            else if (valueLower == "manual")
            {
                controlState.autoMode = false;
                Serial.println("Mode MANUAL command received");
                updateWebStatus("Mode: Manual");
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
    // Only send if status changed
    if (statusMessage != lastStatusMessage)
    {
        JsonDocument doc;
        doc["type"] = "status";
        doc["message"] = statusMessage;
        
        String message;
        serializeJson(doc, message);
        
        Serial.println("Sending WebSocket message: " + message);
        notifyAllClients(message);
        Serial.println("Status: " + statusMessage);
        lastStatusMessage = statusMessage;
    }
}
