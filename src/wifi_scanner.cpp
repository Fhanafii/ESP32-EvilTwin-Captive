#include "wifi_scanner.h"
#include "config.h"

WiFiScanner::WiFiScanner() {
    networkCount = 0;
}

int WiFiScanner::scanNetworks() {
    Serial.println("\n[*] Scanning for WiFi networks...");
    
    // Disconnect if connected
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    
    // Scan for networks
    networkCount = WiFi.scanNetworks();
    
    if (networkCount == 0) {
        Serial.println("[-] No networks found");
        return 0;
    }
    
    Serial.printf("[+] Found %d networks\n", networkCount);
    
    // Store network information
    for (int i = 0; i < networkCount && i < MAX_NETWORKS; i++) {
        networks[i].ssid = WiFi.SSID(i);
        networks[i].rssi = WiFi.RSSI(i);
        networks[i].channel = WiFi.channel(i);
        networks[i].encryption = WiFi.encryptionType(i);
        networks[i].bssid = WiFi.BSSID(i);
        networks[i].hasPassword = (networks[i].encryption != WIFI_AUTH_OPEN);
    }
    
    return networkCount;
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