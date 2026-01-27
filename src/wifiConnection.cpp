/**
 * @file wifiConnection.cpp
 * @brief WiFi connection management with OTA (Over-The-Air) update support for ESP32
 * @author Karl Berger
 * @version 2026.01.23
 *
 * This module provides WiFi connectivity functionality with integrated OTA update
 * capabilities and mDNS service discovery. It handles automatic reconnection,
 * network service initialization, and visual feedback through built-in LED.
 */
#include <Arduino.h>		// for PlatformIO
#include <ArduinoOTA.h>		// for OTA updates
#include <ESPmDNS.h>		// for mDNS responder
#include <WiFi.h>			// for WiFi
#include "configuration.h"	// for SSID, password, OTA settings
#include "wifiConnection.h" // Wi-Fi connection header
#include <time.h>

/**
 * @brief Initialize and configure OTA (Over-The-Air) update service
 *
 * Sets up ArduinoOTA with hostname and password authentication. Configures
 * event handlers for start, progress, completion, and error states during
 * OTA updates. Prints status information to serial console.
 *
 * @note Requires OTA_HOSTNAME and OTA_PASSWORD to be defined in configuration
 * @see configuration.h for hostname and password definitions
 */
void otaBegin()
{ // Set OTA hostname and password for security
	ArduinoOTA.setHostname(OTA_HOSTNAME);
	ArduinoOTA.setPassword(OTA_PASSWORD);
	ArduinoOTA.begin();
	ArduinoOTA.onStart([]()
					   {
    String type = (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem";
    Serial.println("Start updating " + type); });
	ArduinoOTA.onEnd([]()
					 { Serial.println("\nUpdate Complete"); });
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
						  { Serial.printf("Progress: %u%%\r", (progress * 100) / total); });
	ArduinoOTA.onError([](ota_error_t error)
					   {
					   Serial.printf("Error[%u]: ", error);
					   if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
					   else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
					   else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
					   else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
					   else if (error == OTA_END_ERROR) Serial.println("End Failed"); });
	Serial.println("OTA Ready");
	Serial.printf("OTA Hostname: %s\n", OTA_HOSTNAME);
	Serial.printf("OTA IP Address: %s\n", WiFi.localIP().toString().c_str());
} // otaBegin()

/**
 * @brief WiFi event handler for IP address assignment
 *
 * Automatically triggered when ESP32 receives an IP address from DHCP.
 * Initializes mDNS responder and OTA services on first IP acquisition,
 * preventing duplicate service initialization on subsequent reconnections.
 *
 * @param event WiFi event type (should be ARDUINO_EVENT_WIFI_STA_GOT_IP)
 * @param info Additional event information structure
 */
static bool networkServicesStarted = false;  // global flag for network service state

static void onGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
	Serial.printf("WiFi event: GOT IP %s\n", WiFi.localIP().toString().c_str());
	if (!networkServicesStarted)
	{
		// Configure NTP time source (simple, free pool)
		configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER);
		Serial.println("NTP configured");

		if (MDNS.begin(OTA_HOSTNAME))
		{
			Serial.println("mDNS responder started (GOT_IP)");
			MDNS.addService("http", "tcp", 80);
			MDNS.addService("arduino-ota", "tcp", 3232);
		}
		else
		{
			Serial.println("mDNS responder failed to start (continuing without mDNS)");
		}
		otaBegin();
		networkServicesStarted = true;

			// Print current local time (may be 0/UNIX epoch until NTP sync completes)
			time_t now = time(nullptr);
			struct tm timeinfo;
			if (localtime_r(&now, &timeinfo))
			{
				Serial.printf("Current local time: %02d:%02d:%02d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
			}
	}
	else
	{
		Serial.println("Network services already started; skipping init");
	}
}

/**
 * @brief Callback function invoked when the WiFi connection loses its IP address.
 * 
 * This function is called when a WiFi disconnect event occurs. It performs cleanup
 * operations to gracefully stop network services and prepare for a potential reconnection.
 * 
 * @param event The WiFi event type that triggered this callback (expected to be disconnect-related)
 * @param info Additional information about the WiFi event
 * 
 * @details The function performs the following cleanup operations:
 *          - Logs the disconnection event to Serial
 *          - Stops mDNS service to allow clean restart on reconnection
 *          - Sets networkServicesStarted flag to false to indicate services are stopped
 *          - Avoids stopping ArduinoOTA due to platform portability issues
 * 
 * @note ArduinoOTA is not explicitly stopped here because the 'end' method is not
 *       available on all platforms. The onGotIP callback will handle OTA restart
 *       when the network connection is reestablished.
 */
static void onLostIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
	Serial.println("WiFi event: DISCONNECTED; stopping network services");
	// Stop mDNS so it can be restarted cleanly on reconnect
	MDNS.end();
	// Note: ArduinoOTA does not provide a portable 'end' on all platforms,
	// so we avoid calling it here; `onGotIP` will call `otaBegin()` again
	// when network comes back and `networkServicesStarted` is false.
	networkServicesStarted = false;
}

/**
 * @brief Initialize WiFi connection with optional blocking wait
 *
 * Configures WiFi in station mode with auto-reconnect enabled. Sets up
 * built-in LED for visual connection feedback. Registers event handler
 * for IP assignment. Optionally blocks until connection is established
 * with LED blinking feedback.
 *
 * @param waitForConnect If true, blocks until WiFi connection is established
 *                      If false, initiates connection asynchronously
 *
 * Features:
 * - Non-persistent WiFi configuration
 * - Auto-reconnect on connection loss
 * - LED feedback during connection process
 * - mDNS service advertisement (HTTP and Arduino-OTA)
 * - Connection status reporting via serial
 *
 * @note Requires WIFI_SSID, WIFI_PASSWORD, and PIN_LED to be defined
 * @see configuration.h for WiFi credentials and pin definitions
 */
void wifiBegin(bool waitForConnect)
{
	// Configure onboard LED pin
	static bool ledBuiltInState = LOW;		// Built-in LED LOW = OFF, HIGH = ON
	pinMode(PIN_LED, OUTPUT);				// Set LED_PIN as output
	digitalWrite(PIN_LED, ledBuiltInState); // Start with LED off

	WiFi.mode(WIFI_STA);
	WiFi.persistent(false);
	WiFi.setAutoReconnect(true);
	WiFi.setSleep(false);
	Serial.printf("\n%s %s\n", "Connecting to", WIFI_SSID);

	// Register GOT_IP event handler so mDNS/OTA are started on reconnects
	WiFi.onEvent(onGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);
	// Register disconnect handler to reset network service state
	WiFi.onEvent(onLostIP, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);

	// Start connecting (non-blocking). If caller wants to wait, perform
	// the blocking loop below to provide LED feedback and ensure IP is
	// assigned before proceeding.
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	if (waitForConnect)
	{
		Serial.print("Connecting to WiFi");
		while (WiFi.status() != WL_CONNECTED)
		{
			ledBuiltInState = !ledBuiltInState;		// Invert the current state
			digitalWrite(PIN_LED, ledBuiltInState); // Apply the new state
			delay(250);
			yield();
			Serial.print(".");
		}
		digitalWrite(PIN_LED, ledBuiltInState); // Turn LED steady when connected
		Serial.println();
		Serial.printf("Connected to WiFi SSID: %s\n", WIFI_SSID);
		Serial.printf("Assigned IP: %s\n", WiFi.localIP().toString().c_str());
		Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
		Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
	}
}

// end of wifiConnection.cpp