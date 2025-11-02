#include <Arduino.h>
#include "config.h"
#include "wifi_scanner.h"
#include "websocket_server.h"

// Global objects
WiFiScanner scanner;
WSTerminalServer wsServer(WEBSOCKET_PORT);

// Input buffer for WebSocket
String inputBuffer = "";
uint8_t currentClient = 0;

// Output wrapper - prints to both Serial and WebSocket
class Output {
public:
    void print(const String &s) {
        Serial.print(s);
        if (ENABLE_WEBSOCKET && wsServer.isClientConnected()) {
            wsServer.broadcastText(s);
        }
    }
    
    void print(const char* s) {
        Serial.print(s);
        if (ENABLE_WEBSOCKET && wsServer.isClientConnected()) {
            wsServer.broadcastText(s);
        }
    }
    
    void println(const String &s) {
        Serial.println(s);
        if (ENABLE_WEBSOCKET && wsServer.isClientConnected()) {
            wsServer.broadcastText(s + "\n");
        }
    }
    
    void println(const char* s) {
        Serial.println(s);
        if (ENABLE_WEBSOCKET && wsServer.isClientConnected()) {
            String str = String(s) + "\n";
            wsServer.broadcastText(str);
        }
    }
    
    void printf(const char* format, ...) {
        char buffer[256];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        
        Serial.print(buffer);
        if (ENABLE_WEBSOCKET && wsServer.isClientConnected()) {
            wsServer.broadcastText(buffer);
        }
    }
};

Output out;

// Function prototypes
void printBanner();
void printMenu();
void handleSerialInput();
void handleCommand(String cmd);
void scanMode();
void setupControlAP();
void setupLED();
void blinkLED(int times, int delayMs);
void printSystemInfo();
void printNetworkInfo();
void onWebSocketMessage(uint8_t clientNum, String message);

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(STARTUP_DELAY);
    
    // Setup LED only if enabled
    if (ENABLE_LED) {
        setupLED();
    }
    
    // Print banner
    printBanner();
    
    // Setup Control AP for remote access
    setupControlAP();
    
    // Start WebSocket server if enabled
    if (ENABLE_WEBSOCKET) {
        wsServer.begin();
        wsServer.setMessageCallback(onWebSocketMessage);
        out.println("[+] WebSocket server started");
        out.printf("[+] Connect from browser or terminal client\n");
        out.printf("[+] URL: ws://%s:%d\n", WiFi.softAPIP().toString().c_str(), WEBSOCKET_PORT);
    }
    
    out.println("[+] ESP32 initialized successfully");
    out.println("[+] Ready to start...\n");
    
    // Show menu
    printMenu();
}

void loop() {
    // Handle WebSocket events
    if (ENABLE_WEBSOCKET) {
        wsServer.loop();
    }
    
    // Check for Serial input
    if (Serial.available() > 0) {
        handleSerialInput();
    }
    
    delay(10);
}

void setupControlAP() {
    out.println("\n[*] Setting up Control Access Point...");
    
    // Configure AP
    WiFi.mode(WIFI_AP_STA);  // AP + Station mode
    WiFi.softAP(CONTROL_AP_SSID, CONTROL_AP_PASSWORD, 
                CONTROL_AP_CHANNEL, CONTROL_AP_HIDDEN, 
                CONTROL_AP_MAX_CONN);
    
    IPAddress IP = WiFi.softAPIP();
    
    out.println("\n╔══════════════════════════════════════════════════════════════╗");
    out.println("║              CONTROL ACCESS POINT READY                      ║");
    out.println("╠══════════════════════════════════════════════════════════════╣");
    out.printf("║ SSID:          %-45s ║\n", CONTROL_AP_SSID);
    out.printf("║ Password:      %-45s ║\n", CONTROL_AP_PASSWORD);
    out.printf("║ IP Address:    %-45s ║\n", IP.toString().c_str());
    out.printf("║ WebSocket:     ws://%-38s ║\n", (IP.toString() + ":" + String(WEBSOCKET_PORT)).c_str());
    out.println("╠══════════════════════════════════════════════════════════════╣");
    out.println("║ How to connect:                                              ║");
    out.printf(" ║ 1. Connect to WiFi: %s                         ║\n", CONTROL_AP_SSID);
    out.println("║ 2. Open the HTML terminal client in your browser            ║");
    out.println("║ 3. Enter WebSocket URL and click Connect                    ║");
    out.println("╚══════════════════════════════════════════════════════════════╝\n");
}

void printBanner() {
    out.println("\n\n");
    out.println("╔══════════════════════════════════════════════════════════════╗");
    out.println("║                                                              ║");
    out.println("║                     ESP32 WiFi EvilTwin                      ║");
    out.println("║                                                              ║");
    out.println("║                  ⚠️ EDUCATIONAL USE ONLY ⚠️                  ║");
    out.println("║                                                              ║");
    out.println("║           Saya tidak bertanggung jawab jika digunakan        ║");
    out.println("║                    untuk keuntungan pribadi                  ║");
    out.println("╚══════════════════════════════════════════════════════════════╝");
    out.println("");
}

void printMenu() {
    out.println("\n╔══════════════════════════════════════════════════════════════╗");
    out.println("║                        MAIN MENU                             ║");
    out.println("╠══════════════════════════════════════════════════════════════╣");
    out.println("║                                                              ║");
    out.println("║  [1] Scan WiFi Networks (Only work using serial connection)  ║");
    out.println("║  [2] Clone Captive Portal (Coming Soon)                      ║");
    out.println("║  [3] Start Evil Twin AP (Coming Soon)                        ║");
    out.println("║  [4] View Captured Credentials (Coming Soon)                 ║");
    out.println("║  [5] System Information                                      ║");
    out.println("║  [6] Network Information                                     ║");
    out.println("║  [h] Help / Show Menu                                        ║");
    out.println("║  [0] Restart ESP32                                           ║");
    out.println("║                                                              ║");
    out.println("╚══════════════════════════════════════════════════════════════╝");
    out.print("\nEnter your choice: ");
}

void handleSerialInput() {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
        if (inputBuffer.length() > 0) {
            handleCommand(inputBuffer);
            inputBuffer = "";
        }
    } else {
        inputBuffer += c;
        Serial.print(c); // Echo
    }
}

void onWebSocketMessage(uint8_t clientNum, String message) {
    currentClient = clientNum;
    message.trim();
    
    if (message.length() > 0) {
        handleCommand(message);
    }
}

void handleCommand(String cmd) {
    cmd.trim();
    if (cmd.length() == 0) return;
    
    out.println("");
    
    char choice = cmd.charAt(0);
    
    switch (choice) {
        case '1':
            scanMode();
            break;
            
        case '2':
            out.println("[!] Clone Captive Portal - Coming in next part!");
            break;
            
        case '3':
            out.println("[!] Evil Twin AP - Coming in next part!");
            break;
            
        case '4':
            out.println("[!] Credential Viewer - Coming in next part!");
            break;
            
        case '5':
            printSystemInfo();
            break;
            
        case '6':
            printNetworkInfo();
            break;
            
        case 'h':
        case 'H':
            printMenu();
            return; // Don't show menu again
            
        case '0':
            out.println("[*] Restarting ESP32...");
            delay(1000);
            ESP.restart();
            break;
            
        default:
            out.printf("[-] Invalid choice: '%s'\n", cmd.c_str());
            out.println("[*] Type 'h' for help");
            break;
    }
    
    delay(1000);
    printMenu();
}

void scanMode() {
    out.println("[*] Entering WiFi Scanner Mode");
    if (ENABLE_LED) blinkLED(3, 200);
    
    bool scanning = true;
    
    while (scanning) {
        // Perform scan
        int count = scanner.scanNetworks();
        
        if (count > 0) {
            // Print to output
            out.println("\n╔════════════════════════════════════════════════════════════╗");
            out.println("║                   Available Networks                      ║");
            out.println("╠════╦══════════════════════╦═════╦═══════╦═════════╦════════╣");
            out.println("║ #  ║ SSID                ║ Ch  ║ RSSI  ║ Encrypt ║ Signal ║");
            out.println("╠════╬══════════════════════╬═════╬═══════╬═════════╬════════╣");
            
            for (int i = 0; i < count && i < MAX_NETWORKS; i++) {
                NetworkInfo net = scanner.getNetwork(i);
                String ssidDisplay = net.ssid;
                if (ssidDisplay.length() > 20) {
                    ssidDisplay = ssidDisplay.substring(0, 17) + "...";
                }
                
                out.printf("║ %-2d ║ %-20s║ %-3d ║ %-5d ║ %-7s ║ %-6s ║\n",
                        i + 1,
                        ssidDisplay.c_str(),
                        net.channel,
                        net.rssi,
                        scanner.getEncryptionType(net.encryption).c_str(),
                        scanner.getSignalStrength(net.rssi).c_str()
                );
            }
            
            out.println("╚════╩══════════════════════╩═════╩═══════╩═════════╩════════╝");
            
            // Show networks with likely captive portals
            out.println("\n[*] Networks likely to have captive portals:");
            bool foundPortal = false;
            for (int i = 0; i < count; i++) {
                if (scanner.likelyHasCaptivePortal(i)) {
                    NetworkInfo net = scanner.getNetwork(i);
                    out.printf("    - %s (Channel: %d, RSSI: %d)\n", 
                                net.ssid.c_str(), 
                                net.channel, 
                                net.rssi);
                    foundPortal = true;
                }
            }
            if (!foundPortal) {
                out.println("    - None detected");
            }
        }
        
        out.println("\n[*] Type 'q' to quit scanner, or press Enter to scan again...");
        
        // Wait for input with timeout
        unsigned long startWait = millis();
        bool exitScanner = false;
        
        while (millis() - startWait < SCAN_INTERVAL && !exitScanner) {
            // Handle WebSocket
            if (ENABLE_WEBSOCKET) {
                wsServer.loop();
            }
            
            // Check Serial
            if (Serial.available() > 0) {
                char c = Serial.read();
                if (c == 'q' || c == 'Q') {
                    exitScanner = true;
                }
                while (Serial.available()) Serial.read(); // Clear buffer
            }
            
            delay(100);
        }
        
        if (exitScanner) {
            scanning = false;
            out.println("\n[*] Exiting scanner mode");
        } else if (scanning && ENABLE_LED) {
            blinkLED(1, 100);
        }
    }
}

void printSystemInfo() {
    out.println("╔══════════════════════════════════════════════════════════════╗");
    out.println("║                     SYSTEM INFORMATION                       ║");
    out.println("╠══════════════════════════════════════════════════════════════╣");
    
    out.printf("║ Chip Model:        %-40s ║\n", ESP.getChipModel());
    out.printf("║ Chip Revision:     %-40d ║\n", ESP.getChipRevision());
    out.printf("║ CPU Frequency:     %-37d MHz ║\n", ESP.getCpuFreqMHz());
    out.printf("║ Flash Size:        %-37d KB ║\n", ESP.getFlashChipSize() / 1024);
    out.printf("║ Free Heap:         %-37d KB ║\n", ESP.getFreeHeap() / 1024);
    out.printf("║ MAC Address:       %-40s ║\n", WiFi.macAddress().c_str());
    
    out.println("╠══════════════════════════════════════════════════════════════╣");
    out.println("║                     FEATURE STATUS                           ║");
    out.println("╠══════════════════════════════════════════════════════════════╣");
    
    out.printf("║ WiFi Scanner:      %-40s ║\n", ENABLE_SCANNER ? "Enabled" : "Disabled");
    out.printf("║ WebSocket Server:  %-40s ║\n", ENABLE_WEBSOCKET ? "Enabled" : "Disabled");
    out.printf("║ Web Cloner:        %-40s ║\n", ENABLE_WEB_CLONER ? "Enabled" : "Disabled");
    out.printf("║ Evil Twin AP:      %-40s ║\n", ENABLE_EVIL_TWIN ? "Enabled" : "Disabled");
    out.printf("║ Deauth Attack:     %-40s ║\n", ENABLE_DEAUTH ? "Enabled" : "Disabled");
    
    out.println("╚══════════════════════════════════════════════════════════════╝");
}

void printNetworkInfo() {
    out.println("╔══════════════════════════════════════════════════════════════╗");
    out.println("║                   NETWORK INFORMATION                        ║");
    out.println("╠══════════════════════════════════════════════════════════════╣");
    
    out.printf("║ Control AP SSID:   %-40s ║\n", CONTROL_AP_SSID);
    out.printf("║ Control AP IP:     %-40s ║\n", WiFi.softAPIP().toString().c_str());
    out.printf("║ Connected Clients: %-40d ║\n", WiFi.softAPgetStationNum());
    out.printf("║ WebSocket Port:    %-40d ║\n", WEBSOCKET_PORT);
    out.printf("║ WebSocket Clients: %-40d ║\n", wsServer.getClientCount());
    out.printf("║ WebSocket URL:     ws://%-35s ║\n", 
               (WiFi.softAPIP().toString() + ":" + String(WEBSOCKET_PORT)).c_str());
    
    out.println("╚══════════════════════════════════════════════════════════════╝");
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