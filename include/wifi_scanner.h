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
    
    // Scan for networks (async version)
    int scanNetworks(bool async = false);
    
    // Start async scan
    void startAsyncScan();
    
    // Check if scan is complete
    bool isScanComplete();
    
    // Get scan results (call after isScanComplete returns true)
    int getScanResults();
    
    // Get network info by index
    NetworkInfo getNetwork(int index);
    
    // Get total networks found
    int getNetworkCount();
    
    // Find specific network by SSID
    int findNetwork(const char* ssid);
    
    // Print all networks to serial
    void printNetworks();
    
    // Get scan results as formatted string (for buffering)
    String getFormattedResults();
    
    // Get encryption type as string
    String getEncryptionType(uint8_t encType);
    
    // Check if network has captive portal (heuristic)
    bool likelyHasCaptivePortal(int index);
    
    // Get signal strength description
    String getSignalStrength(int32_t rssi);

private:
    int networkCount;
    NetworkInfo networks[30]; // Store up to 30 networks (increased from 20)
    bool scanInProgress;
    
    // Helper to convert RSSI to signal quality
    int rssiToQuality(int32_t rssi);
    
    // Store scan results internally
    void storeScanResults();
};

#endif // WIFI_SCANNER_H