#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

// WebSocket server for secure remote terminal access
class WSTerminalServer {
public:
    WSTerminalServer(uint16_t port = 81);
    
    // Start WebSocket server
    void begin();
    
    // Handle WebSocket events (call in loop)
    void loop();
    
    // Check if client is connected
    bool isClientConnected();
    
    // Get number of connected clients
    uint8_t getClientCount();
    
    // Send text to all connected clients
    void broadcastText(const String &text);
    void broadcastText(const char* text);
    
    // Send text to specific client
    void sendText(uint8_t clientNum, const String &text);
    void sendText(uint8_t clientNum, const char* text);
    
    // Printf-style output
    void printf(const char* format, ...);
    
    // Callback for received messages
    void setMessageCallback(void (*callback)(uint8_t clientNum, String message));
    
private:
    WebSocketsServer* ws;
    uint16_t port;
    uint8_t connectedClients;
    void (*messageCallback)(uint8_t, String);
    
    // Internal WebSocket event handler
    static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
    static WSTerminalServer* instance; // For static callback
};

#endif // WEBSOCKET_SERVER_H