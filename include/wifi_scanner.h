#ifndef WIFI_SCANNER_H
#define WIFI_SCANNER_H

#include <Arduino.h>
#include <WiFi.h>

// Structure to hold network information
struct NetworkInfo {
    String ssid;
    int32_t rssi;
    uint8_t channel;
    uint8_t encryption;
    uint8_t* bssid;
    bool hasPassword;
};

class WiFiScanner {
public:
    WiFiScanner();
    
    // Scan for networks
    int scanNetworks();
    
    // Get network info by index
    NetworkInfo getNetwork(int index);
    
    // Get total networks found
    int getNetworkCount();
    
    // Find specific network by SSID
    int findNetwork(const char* ssid);
    
    // Print all networks to serial
    void printNetworks();
    
    // Get encryption type as string
    String getEncryptionType(uint8_t encType);
    
    // Check if network has captive portal (heuristic)
    bool likelyHasCaptivePortal(int index);
    
    // Get signal strength description
    String getSignalStrength(int32_t rssi);

private:
    int networkCount;
    NetworkInfo networks[20]; // Store up to 20 networks
    
    // Helper to convert RSSI to signal quality
    int rssiToQuality(int32_t rssi);
};

#endif // WIFI_SCANNER_H