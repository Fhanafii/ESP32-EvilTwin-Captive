#include "websocket_server.h"
#include <stdarg.h>

// Static instance for callback
WSTerminalServer* WSTerminalServer::instance = nullptr;

WSTerminalServer::WSTerminalServer(uint16_t port) {
    this->port = port;
    this->ws = nullptr;
    this->connectedClients = 0;
    this->messageCallback = nullptr;
    instance = this; // Set static instance
}

void WSTerminalServer::begin() {
    if (ws != nullptr) {
        delete ws;
    }
    
    ws = new WebSocketsServer(port);
    ws->begin();
    ws->onEvent(webSocketEvent);
    
    Serial.printf("[+] WebSocket server started on port %d\n", port);
    Serial.printf("[+] Connect via: ws://%s:%d\n", WiFi.softAPIP().toString().c_str(), port);
}

void WSTerminalServer::loop() {
    if (ws != nullptr) {
        ws->loop();
    }
}

void WSTerminalServer::webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (instance == nullptr) return;
    
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[-] WebSocket: Client #%u disconnected\n", num);
            instance->connectedClients--;
            break;
            
        case WStype_CONNECTED: {
            IPAddress ip = instance->ws->remoteIP(num);
            Serial.printf("[+] WebSocket: Client #%u connected from %s\n", num, ip.toString().c_str());
            instance->connectedClients++;
            
            // Send welcome message
            String welcome = "\n╔══════════════════════════════════════════════════════════════╗\n";
            welcome += "║       ESP32 WiFi Security Research Tool - Web Terminal      ║\n";
            welcome += "║                  ⚠️  EDUCATIONAL USE ONLY ⚠️                  ║\n";
            welcome += "╚══════════════════════════════════════════════════════════════╝\n\n";
            welcome += "[+] Connected to ESP32 via WebSocket\n";
            welcome += "[+] Type 'help' for available commands\n\n";
            instance->ws->sendTXT(num, welcome);
            break;
        }
            
        case WStype_TEXT:
            Serial.printf("[*] WebSocket: Received from #%u: %s\n", num, payload);
            
            // Call message callback if set
            if (instance->messageCallback != nullptr) {
                String message = String((char*)payload);
                instance->messageCallback(num, message);
            }
            break;
            
        case WStype_BIN:
            Serial.printf("[*] WebSocket: Binary data received from #%u\n", num);
            break;
            
        case WStype_ERROR:
            Serial.printf("[-] WebSocket: Error from #%u\n", num);
            break;
            
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            // Handle fragmented messages if needed
            break;
            
        default:
            break;
    }
}

bool WSTerminalServer::isClientConnected() {
    return connectedClients > 0;
}

uint8_t WSTerminalServer::getClientCount() {
    return connectedClients;
}

void WSTerminalServer::broadcastText(const String &text) {
    if (ws != nullptr) {
        String copy = text;
        ws->broadcastTXT(copy);
    }
}

void WSTerminalServer::broadcastText(const char* text) {
    if (ws != nullptr) {
        ws->broadcastTXT(text);
    }
}

void WSTerminalServer::sendText(uint8_t clientNum, const String &text) {
    if (ws != nullptr) {
        String copy = text;
        ws->sendTXT(clientNum, copy);
    }
}

void WSTerminalServer::sendText(uint8_t clientNum, const char* text) {
    if (ws != nullptr) {
        ws->sendTXT(clientNum, text);
    }
}

void WSTerminalServer::printf(const char* format, ...) {
    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    broadcastText(buffer);
}

void WSTerminalServer::setMessageCallback(void (*callback)(uint8_t, String)) {
    this->messageCallback = callback;
}