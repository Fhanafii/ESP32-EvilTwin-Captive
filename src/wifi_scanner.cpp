#include "wifi_scanner.h"
#include "config.h"
#include "esp_wifi.h"

WiFiScanner::WiFiScanner() {
    networkCount = 0;
    scanInProgress = false;
}

int WiFiScanner::scanNetworks(bool async) {
    if (async) {
        startAsyncScan();
        return -1; // Scan in progress
    }
    
    Serial.println("\n[*] Scanning for WiFi networks (blocking mode)...");
    
    // Configure scan parameters for minimal AP disruption
    wifi_scan_config_t scanConf;
    scanConf.ssid = NULL;
    scanConf.bssid = NULL;
    scanConf.channel = 0;  // Scan all channels
    scanConf.show_hidden = SCAN_SHOW_HIDDEN;
    scanConf.scan_type = SCAN_TYPE_ACTIVE ? WIFI_SCAN_TYPE_ACTIVE : WIFI_SCAN_TYPE_PASSIVE;
    scanConf.scan_time.active.min = SCAN_CHANNEL_TIME;
    scanConf.scan_time.active.max = SCAN_CHANNEL_TIME;
    
    // Perform scan
    networkCount = WiFi.scanNetworks(false, SCAN_SHOW_HIDDEN, false, SCAN_CHANNEL_TIME);
    
    if (networkCount == 0) {
        Serial.println("[-] No networks found");
        return 0;
    }
    
    Serial.printf("[+] Found %d networks\n", networkCount);
    
    // Store network information
    storeScanResults();
    
    return networkCount;
}

void WiFiScanner::startAsyncScan() {
    Serial.println("\n[*] Starting async WiFi scan...");
    scanInProgress = true;
    
    // Start async scan with optimized channel time
    WiFi.scanNetworks(true, SCAN_SHOW_HIDDEN, false, SCAN_CHANNEL_TIME);
}

bool WiFiScanner::isScanComplete() {
    if (!scanInProgress) return false;
    
    int result = WiFi.scanComplete();
    
    if (result >= 0) {
        // Scan complete
        scanInProgress = false;
        return true;
    } else if (result == WIFI_SCAN_FAILED) {
        Serial.println("[-] Scan failed");
        scanInProgress = false;
        return true;
    }
    
    // Still scanning
    return false;
}

int WiFiScanner::getScanResults() {
    int result = WiFi.scanComplete();
    
    if (result >= 0) {
        networkCount = result;
        Serial.printf("[+] Async scan complete: Found %d networks\n", networkCount);
        
        // Store network information
        storeScanResults();
        
        // Delete scan result from memory
        WiFi.scanDelete();
        
        return networkCount;
    }
    
    return 0;
}

void WiFiScanner::storeScanResults() {
    for (int i = 0; i < networkCount && i < MAX_NETWORKS; i++) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].channel = WiFi.channel(i);
        networks[i].encryption = WiFi.encryptionType(i);
        networks[i].bssid = WiFi.BSSID(i);
        networks[i].hasPassword = (networks[i].encryption != WIFI_AUTH_OPEN);
    }
}

NetworkInfo WiFiScanner::getNetwork(int index) {
    if (index >= 0 && index < networkCount) {
        return networks[index];
    }
    return NetworkInfo(); // Return empty struct
}

int WiFiScanner::getNetworkCount() {
    return networkCount;
}

int WiFiScanner::findNetwork(const char* ssid) {
    for (int i = 0; i < networkCount; i++) {
        if (networks[i].ssid.equals(ssid)) {
            return i;
        }
    }
    return -1; // Not found
}

void WiFiScanner::printNetworks() {
    Serial.println("\n╔════════════════════════════════════════════════════════════╗");
    Serial.println("║                   Available Networks                      ║");
    Serial.println("╠════╦══════════════════════╦═════╦═══════╦═════════╦════════╣");
    Serial.println("║ #  ║ SSID                ║ Ch  ║ RSSI  ║ Encrypt ║ Signal ║");
    Serial.println("╠════╬══════════════════════╬═════╬═══════╬═════════╬════════╣");
    
    for (int i = 0; i < networkCount && i < MAX_NETWORKS; i++) {
        char line[200];
        String ssidDisplay = networks[i].ssid;
        if (ssidDisplay.length() > 20) {
            ssidDisplay = ssidDisplay.substring(0, 17) + "...";
        }
        
        sprintf(line, "║ %-2d ║ %-20s║ %-3d ║ %-5d ║ %-7s ║ %-6s ║",
                i + 1,
                ssidDisplay.c_str(),
                networks[i].channel,
                networks[i].rssi,
                getEncryptionType(networks[i].encryption).c_str(),
                getSignalStrength(networks[i].rssi).c_str()
        );
        Serial.println(line);
    }
    
    Serial.println("╚════╩══════════════════════╩═════╩═══════╩═════════╩════════╝");
}

String WiFiScanner::getEncryptionType(uint8_t encType) {
    switch (encType) {
        case WIFI_AUTH_OPEN:
            return "Open";
        case WIFI_AUTH_WEP:
            return "WEP";
        case WIFI_AUTH_WPA_PSK:
            return "WPA";
        case WIFI_AUTH_WPA2_PSK:
            return "WPA2";
        case WIFI_AUTH_WPA_WPA2_PSK:
            return "WPA/2";
        case WIFI_AUTH_WPA2_ENTERPRISE:
            return "WPA2-E";
        case WIFI_AUTH_WPA3_PSK:
            return "WPA3";
        default:
            return "Unknown";
    }
}

String WiFiScanner::getSignalStrength(int32_t rssi) {
    if (rssi > -50) return "Excellent";
    if (rssi > -60) return "Good";
    if (rssi > -70) return "Fair";
    if (rssi > -80) return "Weak";
    return "Poor";
}

int WiFiScanner::rssiToQuality(int32_t rssi) {
    if (rssi <= -100) return 0;
    if (rssi >= -50) return 100;
    return 2 * (rssi + 100);
}

bool WiFiScanner::likelyHasCaptivePortal(int index) {
    if (index < 0 || index >= networkCount) return false;
    
    // Heuristic: Open networks often have captive portals
    // Common SSIDs that typically use portals
    String ssid = networks[index].ssid;
    ssid.toLowerCase();
    
    if (networks[index].encryption == WIFI_AUTH_OPEN) {
        return true; // Most open networks have portals
    }
    
    // Check for common portal keywords
    if (ssid.indexOf("guest") != -1 || 
        ssid.indexOf("public") != -1 ||
        ssid.indexOf("free") != -1 ||
        ssid.indexOf("wifi") != -1 ||
        ssid.indexOf("hotel") != -1 ||
        ssid.indexOf("airport") != -1) {
        return true;
    }
    
    return false;
}

String WiFiScanner::getFormattedResults() {
    String output = "";
    
    output += "\n╔════════════════════════════════════════════════════════════╗\n";
    output += "║                   Available Networks                      ║\n";
    output += "╠════╦══════════════════════╦═════╦═══════╦═════════╦════════╣\n";
    output += "║ #  ║ SSID                ║ Ch  ║ RSSI  ║ Encrypt ║ Signal ║\n";
    output += "╠════╬══════════════════════╬═════╬═══════╬═════════╬════════╣\n";
    
    for (int i = 0; i < networkCount && i < MAX_NETWORKS; i++) {
        String ssidDisplay = networks[i].ssid;
        if (ssidDisplay.length() > 20) {
            ssidDisplay = ssidDisplay.substring(0, 17) + "...";
        }
        
        char line[200];
        sprintf(line, "║ %-2d ║ %-20s║ %-3d ║ %-5d ║ %-7s ║ %-6s ║\n",
                i + 1,
                ssidDisplay.c_str(),
                networks[i].channel,
                networks[i].rssi,
                getEncryptionType(networks[i].encryption).c_str(),
                getSignalStrength(networks[i].rssi).c_str()
        );
        output += line;
    }
    
    output += "╚════╩══════════════════════╩═════╩═══════╩═════════╩════════╝\n";
    
    // Show networks with likely captive portals
    output += "\n[*] Networks likely to have captive portals:\n";
    bool foundPortal = false;
    for (int i = 0; i < networkCount; i++) {
        if (likelyHasCaptivePortal(i)) {
            char portalLine[150];
            sprintf(portalLine, "    - %s (Channel: %d, RSSI: %d)\n",
                    networks[i].ssid.c_str(),
                    networks[i].channel,
                    networks[i].rssi);
            output += portalLine;
            foundPortal = true;
        }
    }
    if (!foundPortal) {
        output += "    - None detected\n";
    }
    
    return output;
}