
/**
 * @file wifiConnection.cpp
 * @brief Implements Wi-Fi connection and OTA update functionality for the magloop-controller project.
 *
 * This file provides functions to initialize Wi-Fi connectivity, handle built-in LED
 * status indication, and enable Arduino OTA (Over-The-Air) updates.
 * It uses the ESP32 WiFi and ArduinoOTA libraries, and relies on configuration parameters
 * defined in "configuration.h" for SSID, password, and network settings.
 *
 * Features:
 * - Wi-Fi connection with status LED feedback.
 * - DHCP IP assignment (default).
 * - Built-in LED control and toggling.
 * - Arduino OTA update support with progress and error reporting.
 *
 * Dependencies:
 * - Arduino.h
 * - WiFi.h
 * - ArduinoOTA.h
 * - configuration.h
 *
 * @author Karl Berger
 * @date 2025-05-20
 */
#include "wifiConnection.h" // Wi-Fi connection header
#include <Arduino.h>		// for PlatformIO
#include <WiFi.h>			// for WiFi
#include <ArduinoOTA.h>		// for OTA updates
#include <ESPmDNS.h>		// for mDNS responder
#include "configuration.h"	// for SSID, password, static IP

/**
 * @brief Initializes and configures Over-The-Air (OTA) update functionality.
 *
 * This function sets up the ArduinoOTA library, registering event handlers for OTA start,
 * end, progress, and error events. It enables the device to receive firmware or filesystem
 * updates wirelessly. Upon successful initialization, a message is printed to the serial console.
 *
 * Event handlers:
 * - onStart: Prints the type of update being started ("sketch" or "filesystem").
 * - onEnd: Prints a message when the update is complete.
 * - onProgress: Prints the update progress as a percentage.
 * - onError: Prints an error message corresponding to the OTA error encountered.
 */
void otaBegin()
{	// Set OTA hostname and password for security
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
	// Print the actual assigned IP (may be DHCP); don't print the static
	// `LOCAL_IP` unconditionally because that can be misleading when
	// `USE_STATIC_IP == false` and the device is using DHCP.
	Serial.printf("OTA IP Address: %s\n", WiFi.localIP().toString().c_str());
} // otaBegin()

//! Global variables
static bool ledBuiltIn = LOW; // Built-in LED LOW = OFF, HIGH = ON
// Track whether mDNS/OTA services have been started for the current
// network interface. This prevents re-initializing services on repeated
// GOT_IP events.
static bool networkServicesStarted = false;

// WiFi GOT_IP event handler: start mDNS and OTA after obtaining an IP.
static void onGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
	Serial.printf("WiFi event: GOT IP %s\n", WiFi.localIP().toString().c_str());
	if (!networkServicesStarted) {
		if (MDNS.begin(OTA_HOSTNAME)) {
			Serial.println("mDNS responder started (GOT_IP)");
			MDNS.addService("http", "tcp", 80);
			MDNS.addService("arduino-ota", "tcp", 3232);
		} else {
			Serial.println("mDNS responder failed to start (continuing without mDNS)");
		}
		otaBegin();
		networkServicesStarted = true;
	} else {
		Serial.println("Network services already started; skipping init");
	}
}

/**
 * @brief Sets the state of the built-in LED.
 *
 * This function updates the state of the built-in LED to the specified value.
 * It also updates the internal `ledBuiltIn` variable to reflect the current state.
 *
 * @param state A boolean value indicating the desired LED state:
 *              - `true` to turn the LED ON.
 *              - `false` to turn the LED OFF.
 */
void setLED_PIN(bool state)
{
	ledBuiltIn = state;
	digitalWrite(PIN_LED, ledBuiltIn);
}

/**
 * @brief Toggles the state of the built-in LED.
 *
 * This function inverts the current state of the built-in LED. If the LED is ON,
 * it will be turned OFF, and vice versa. The internal `ledState` variable is
 * updated to reflect the new state.
 */
void toggleLED_PIN()
{
	ledBuiltIn = !ledBuiltIn;		   // Invert the current state
	digitalWrite(PIN_LED, ledBuiltIn); // Apply the new state
}

/**
 * @brief Attempts to connect to a WiFi network using predefined SSID and password.
 *
 * If the device is not already connected to WiFi, this function initiates the connection
 * process. While attempting to connect, it toggles the built-in LED and prints a dot to
 * the serial monitor every 250 milliseconds. Once connected, it turns on the built-in LED
 * and prints the assigned local IP address to the serial monitor.
 *
 * @note Requires WIFI_SSID and WIFI_PASSWORD to be defined.
 * @note Assumes Serial and WiFi have been initialized.
 */
void wifiBegin(bool waitForConnect)
{
	// Configure onboard LED pin
	pinMode(PIN_LED, OUTPUT);    // Set LED_PIN as output
	digitalWrite(PIN_LED, LOW);  // Start with LED off

	WiFi.mode(WIFI_STA);
	WiFi.persistent(false);
	WiFi.setAutoReconnect(true);
	WiFi.setSleep(false);
	Serial.printf("\n%s %s\n", "Connecting to", WIFI_SSID);

	// Register GOT_IP event handler so mDNS/OTA are started on reconnects
	WiFi.onEvent(onGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);

	// Start connecting (non-blocking). If caller wants to wait, perform
	// the blocking loop below to provide LED feedback and ensure IP is
	// assigned before proceeding.
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

	if (waitForConnect) {
		Serial.print("Connecting to WiFi");
		while (WiFi.status() != WL_CONNECTED) {
			toggleLED_PIN();
			delay(250);
			yield();
			Serial.print(".");
		}
		setLED_PIN(LOW); // Turn LED steady when connected
		Serial.println();
		Serial.printf("Connected to WiFi SSID: %s\n", WIFI_SSID);
		Serial.printf("Assigned IP: %s\n", WiFi.localIP().toString().c_str());
		Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
		Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());

		// Start mDNS responder only after we have an IP to advertise
		if (MDNS.begin(OTA_HOSTNAME)) {
			Serial.println("mDNS responder started");
			MDNS.addService("http", "tcp", 80);
			MDNS.addService("arduino-ota", "tcp", 3232);
		} else {
			Serial.println("mDNS responder failed to start (continuing without mDNS)");
		}

		// Start OTA now that network is up
		otaBegin();
		networkServicesStarted = true;
	}
}

/**
 * @brief Initializes the WiFi connection with specific settings.
 *
 * This function configures the onboard LED as an output and turns it off.
 * It sets the WiFi mode to station (WIFI_STA), disables WiFi persistence,
 * enables automatic reconnection, and disables WiFi sleep mode.
 * It uses DHCP for IP assignment by default; any fixed-address requirements
 * should be handled by a DHCP reservation on the router.
 */
// (removed old no-arg wifiBegin; use wifiBegin(bool) instead)