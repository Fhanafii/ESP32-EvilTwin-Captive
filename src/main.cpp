#include <Arduino.h>
#include "config.h"
#include "wifi_scanner.h"

// Global objects
WiFiScanner scanner;

// Function prototypes
void printBanner();
void printMenu();
void handleMenuChoice();
void scanMode();
void printSystemInfo();
void setupLED();
void blinkLED(int times, int delayMs);

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(STARTUP_DELAY);
    
    // Setup LED only if enabled
    if (ENABLE_LED) {
        setupLED();
    }
    
    // Print banner
    printBanner();
    
    // Initial setup
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    
    Serial.println("[+] ESP32 initialized successfully");
    Serial.println("[+] Ready to start...\n");
    
    // Show menu
    printMenu();
}

void loop() {
    // Check for serial input
    if (Serial.available() > 0) {
        handleMenuChoice();
    }
    
    delay(100);
}

void printBanner() {
    Serial.println("\n\n");
    Serial.println("╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║                                                              ║");
    Serial.println("║              ESP32 WiFi Security Research Tool               ║");
    Serial.println("║                                                              ║");
    Serial.println("║                    ⚠️  EDUCATIONAL USE ONLY ⚠️                ║");
    Serial.println("║                                                              ║");
    Serial.println("║   Only use on networks you own or have permission to test   ║");
    Serial.println("║                                                              ║");
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
    Serial.println();
}

void printMenu() {
    Serial.println("\n╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║                        MAIN MENU                             ║");
    Serial.println("╠══════════════════════════════════════════════════════════════╣");
    Serial.println("║                                                              ║");
    Serial.println("║  [1] Scan WiFi Networks                                      ║");
    Serial.println("║  [2] Clone Captive Portal (Coming Soon)                     ║");
    Serial.println("║  [3] Start Evil Twin AP (Coming Soon)                       ║");
    Serial.println("║  [4] View Captured Credentials (Coming Soon)                ║");
    Serial.println("║  [5] System Information                                      ║");
    Serial.println("║  [0] Restart ESP32                                           ║");
    Serial.println("║                                                              ║");
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
    Serial.print("\nEnter your choice: ");
}

void handleMenuChoice() {
    char choice = Serial.read();
    
    // Clear serial buffer
    while (Serial.available()) {
        Serial.read();
    }
    
    Serial.println(choice);
    Serial.println();
    
    switch (choice) {
        case '1':
            scanMode();
            break;
            
        case '2':
            Serial.println("[!] Clone Captive Portal - Coming in next part!");
            break;
            
        case '3':
            Serial.println("[!] Evil Twin AP - Coming in next part!");
            break;
            
        case '4':
            Serial.println("[!] Credential Viewer - Coming in next part!");
            break;
            
        case '5':
            printSystemInfo();
            break;
            
        case '0':
            Serial.println("[*] Restarting ESP32...");
            delay(1000);
            ESP.restart();
            break;
            
        default:
            Serial.println("[-] Invalid choice!");
            break;
    }
    
    delay(2000);
    printMenu();
}

void scanMode() {
    Serial.println("[*] Entering WiFi Scanner Mode");
    if (ENABLE_LED) blinkLED(3, 200);
    
    bool scanning = true;
    
    while (scanning) {
        // Perform scan
        int count = scanner.scanNetworks();
        
        if (count > 0) {
            scanner.printNetworks();
            
            // Show networks with likely captive portals
            Serial.println("\n[*] Networks likely to have captive portals:");
            bool foundPortal = false;
            for (int i = 0; i < count; i++) {
                if (scanner.likelyHasCaptivePortal(i)) {
                    NetworkInfo net = scanner.getNetwork(i);
                    Serial.printf("    - %s (Channel: %d, RSSI: %d)\n", 
                                net.ssid.c_str(), 
                                net.channel, 
                                net.rssi);
                    foundPortal = true;
                }
            }
            if (!foundPortal) {
                Serial.println("    - None detected");
            }
        }
        
        Serial.println("\n[*] Press 'q' to quit scanner, or any key to scan again...");
        
        // Wait for input
        unsigned long startWait = millis();
        while (millis() - startWait < SCAN_INTERVAL) {
            if (Serial.available() > 0) {
                char input = Serial.read();
                if (input == 'q' || input == 'Q') {
                    scanning = false;
                    Serial.println("\n[*] Exiting scanner mode");
                }
                // Clear buffer
                while (Serial.available()) Serial.read();
                break;
            }
            delay(100);
        }
        
        if (scanning && ENABLE_LED) {
            blinkLED(1, 100);
        }
    }
}

void printSystemInfo() {
    Serial.println("╔══════════════════════════════════════════════════════════════╗");
    Serial.println("║                     SYSTEM INFORMATION                       ║");
    Serial.println("╠══════════════════════════════════════════════════════════════╣");
    
    Serial.printf("║ Chip Model:        %-40s ║\n", ESP.getChipModel());
    Serial.printf("║ Chip Revision:     %-40d ║\n", ESP.getChipRevision());
    Serial.printf("║ CPU Frequency:     %-40d ║\n", ESP.getCpuFreqMHz());
    Serial.printf("║ Flash Size:        %-40d ║\n", ESP.getFlashChipSize());
    Serial.printf("║ Free Heap:         %-40d ║\n", ESP.getFreeHeap());
    Serial.printf("║ MAC Address:       %-40s ║\n", WiFi.macAddress().c_str());
    
    Serial.println("╠══════════════════════════════════════════════════════════════╣");
    Serial.println("║                     FEATURE STATUS                           ║");
    Serial.println("╠══════════════════════════════════════════════════════════════╣");
    
    Serial.printf("║ WiFi Scanner:      %-40s ║\n", ENABLE_SCANNER ? "Enabled" : "Disabled");
    Serial.printf("║ Web Cloner:        %-40s ║\n", ENABLE_WEB_CLONER ? "Enabled" : "Disabled");
    Serial.printf("║ Evil Twin AP:      %-40s ║\n", ENABLE_EVIL_TWIN ? "Enabled" : "Disabled");
    Serial.printf("║ Deauth Attack:     %-40s ║\n", ENABLE_DEAUTH ? "Enabled" : "Disabled");
    
    Serial.println("╚══════════════════════════════════════════════════════════════╝");
}

void setupLED() {
    if (!ENABLE_LED) return;
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Startup blink sequence
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }
}

void blinkLED(int times, int delayMs) {
    if (!ENABLE_LED) return;
    
    for (int i = 0; i < times; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(delayMs);
        digitalWrite(LED_PIN, LOW);
        delay(delayMs);
    }
}