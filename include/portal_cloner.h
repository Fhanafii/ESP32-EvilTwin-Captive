#ifndef PORTAL_CLONER_H
#define PORTAL_CLONER_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Structure to hold cloned portal data
struct ClonedPortal {
    String ssid;
    String html;
    String portalURL;
    bool isCloned;
    int htmlSize;
};

class PortalCloner {
public:
    PortalCloner();
    
    // Connect to target network
    bool connectToNetwork(const char* ssid, const char* password = "");
    
    // Detect captive portal
    bool detectCaptivePortal();
    
    // Download portal HTML
    bool downloadPortal();
    
    // Get cloned portal data
    ClonedPortal getClonedPortal();
    
    // Check if portal is cloned
    bool hasClonedPortal();
    
    // Clear cloned portal
    void clearClonedPortal();
    
    // Get portal info as formatted string
    String getCloneInfo();
    
    // Disconnect from target network
    void disconnect();

private:
    ClonedPortal clonedPortal;
    HTTPClient http;
    
    // Portal detection URLs (common endpoints)
    const char* detectionURLs[10] = {
        "http://detectportal.firefox.com/success.txt",
        "http://www.msftconnecttest.com/connecttest.txt",
        "http://captive.apple.com/hotspot-detect.html",
        "http://connectivitycheck.gstatic.com/generate_204",
        "http://clients3.google.com/generate_204",
        "http://www.google.com/gen_204",
        "http://play.googleapis.com/generate_204",
        "http://nmcheck.gnome.org/check_network_status.txt",
        "http://network-test.debian.org/nm",
        "http://www.android.com/generate_204"
    };
    
    // Fetch URL and return HTML
    String fetchURL(const String& url);
    
    // Parse HTML and simplify
    String simplifyHTML(const String& html);
    
    // Extract base URL from redirect
    String extractPortalURL(const String& redirectURL);
    
    // Remove external resources
    String stripExternalResources(const String& html);
    
    // Convert to self-contained HTML
    String makePortalSelfContained(const String& html, const String& baseURL);
};

#endif // PORTAL_CLONER_H