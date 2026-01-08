/**
 * @file webSocket.h
 * @version 2026.01.08
 * @author Karl Berger & MS Copilot
 * @brief WebSocket communication interface for the Gas Log Controller
 *
 * This header file provides the WebSocket functionality for real-time bidirectional
 * communication between the ESP32 Gas Log Controller and connected web clients.
 * It handles client connections, message broadcasting, and status updates through
 * the AsyncWebSocket library.
 *
 * @note Requires ESPAsyncWebServer library: https://github.com/ESP32Async/ESPAsyncWebServer
 */

/**
 * @var server
 * @brief Global AsyncWebServer instance for handling HTTP requests
 *
 * External reference to the main web server object defined in the implementation file.
 */

/**
 * @var ws
 * @brief Global AsyncWebSocket instance for WebSocket connections
 *
 * External reference to the WebSocket server object that manages all WebSocket connections.
 */

/**
 * @brief WebSocket event handler callback
 *
 * Processes all WebSocket events including connections, disconnections, text messages,
 * binary data, pong responses, and errors. This function is automatically called by
 * the AsyncWebSocket library when events occur.
 *
 * @param server Pointer to the AsyncWebSocket server instance
 * @param client Pointer to the client that triggered the event
 * @param type The type of WebSocket event (WS_EVT_CONNECT, WS_EVT_DISCONNECT, etc.)
 * @param arg Pointer to additional event-specific arguments
 * @param data Pointer to the data payload (for data events)
 * @param len Length of the data payload in bytes
 */

#ifndef WEBSOCKET_H
#define WEBSOCKET_H

#include <Arduino.h>           // Core Arduino library
#include <ESPAsyncWebServer.h> // https://github.com/ESP32Async/ESPAsyncWebServer

extern AsyncWebServer server; // Global web server instance
extern AsyncWebSocket ws;     // Global WebSocket server instance
extern bool tempSensorAvailable; // Temperature sensor availability status

struct ControlState
{
    bool powerOn;           // true = ON, false = OFF
    bool autoMode;          // true = AUTOMATIC, false = MANUAL
    int setpointF;          // Temperature setpoint in Fahrenheit
    String valveState;      // "OFF", "IDLE", or "HEATING"
};

extern ControlState controlState;

//! Web Socket event handler
void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void broadcastControlState();                      //! Send current control state to all connected WebSocket clients
void notifyAllClients(const String &message);      //! Notify all connected clients with a message
void setRoomTempColor(const char *newState);       //! Update room temperature background color
void updateWebStatus(const String &statusMessage); //! Update system status and notify clients
void websocketBegin();                             //! Initialize WebSocket and serve UI files
void websocketCleanup();                           //! Periodically clean up disconnected clients

#endif                                             // WEBSOCKET_H