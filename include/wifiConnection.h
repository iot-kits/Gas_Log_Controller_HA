/**
 * @file wifiConnection.h
 * @brief Declarations for WiFi and OTA connection management functions.
 *
 * This header provides function declarations for initializing WiFi, defining
 * mulicast DNS (mDNS) responder, managing the built-in LED during
 * connection to a WiFi network, and starting OTA (Over-The-Air) updates.
 * 
 * @author Karl Berger
 * @date 2026-01-17
 */

/**
 * @brief Initializes WiFi hardware and network configuration. Connects to the
 * specified WiFi network and optionally waits for connection. Starts mDNS.
 * Starts OTA updates after network is up.
 *
 * @param waitForConnect If true, blocks until WiFi successfully connects
 *                       (shows LED feedback). If false, starts the WiFi
 *                       interface and returns immediately; autoreconnect is
 *                       enabled.
 */
void wifiBegin(bool waitForConnect = true);

// `otaBegin()` is internal to `wifiConnection.cpp` and started by `wifiBegin()`.
