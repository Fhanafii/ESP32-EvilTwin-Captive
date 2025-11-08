```
╔═══════════════════════════════════════════════════════════════════════════════════════════════════╗
║                                                                                                   ║
║   ███████╗███████╗██████╗ ██████╗ ██████╗       ███╗   ███╗██╗██████╗  █████╗  ██████╗ ███████╗   ║
║   ██╔════╝██╔════╝██╔══██╗╚════██╗╚════██╗      ████╗ ████║██║██╔══██╗██╔══██╗██╔════╝ ██╔════╝   ║
║   █████╗  ███████╗██████╔╝ █████╔╝ █████╔╝█████╗██╔████╔██║██║██████╔╝███████║██║  ███╗█████╗     ║
║   ██╔══╝  ╚════██║██╔═══╝  ╚═══██╗██╔═══╝ ╚════╝██║╚██╔╝██║██║██╔══██╗██╔══██║██║   ██║██╔══╝     ║
║   ███████╗███████║██║     ██████╔╝███████╗      ██║ ╚═╝ ██║██║██║  ██║██║  ██║╚██████╔╝███████╗   ║
║   ╚══════╝╚══════╝╚═╝     ╚═════╝ ╚══════╝      ╚═╝     ╚═╝╚═╝╚═╝  ╚═╝╚═╝  ╚═╝ ╚═════╝ ╚══════╝   ║
║                                                                                                   ║
║                                      ░ E S P - M I R A G E ░                                      ║
║                          ESP32 Captive-Portal Toolkit — Research Only                             ║
║                                                                                                   ║
║                      ⚠️  EDUCATIONAL — DO NOT USE ON OTHERS' NETWORKS ⚠️                            ║
║                                                                                                   ║
║                          Author:./runme             •   Version: 1.5                              ║
║                                                                                                   ║
╚═══════════════════════════════════════════════════════════════════════════════════════════════════╝
```

# ESP32-EvilTwin-Captive

A research-oriented ESP32-based toolkit for WiFi security testing, including evil twin access points, captive portal cloning, and credential harvesting. This tool is designed for educational and research purposes only to understand WiFi security concepts.

## ⚠️ IMPORTANT DISCLAIMER

**This project is strictly for educational and research purposes. I do not support or condone the use of this tool for illegal activities, including unauthorized access to networks, harvesting credentials without permission, or any form of network intrusion. Using this tool on networks without explicit permission from the owner is illegal and unethical. The author takes no responsibility for any misuse of this software. Always obtain written consent before testing on any network.**

## Features

- **WiFi Scanner**: Scan for available networks, detect likely captive portals
- **Portal Cloner**: Clone captive portals from open WiFi networks
- **Captive Portal**: Set up evil twin access points with various portal types (Generic, Hotel, Airport, Coffee Shop)
- **Credential Harvesting**: Capture and store login credentials
- **WebSocket Server**: Remote access via WebSocket for control and monitoring
- **Control Access Point**: Dedicated AP for remote management
- **Menu-Driven Interface**: Interactive menu via Serial or WebSocket
- **LED Indicators**: Optional visual feedback (if enabled)

## Requirements

### Hardware
- ESP32 development board (e.g., ESP32-WROOM-32)
- USB cable for flashing and serial communication
- Optional: LED connected to GPIO 2 for status indication

### Software
- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- Git (for cloning the repository)

## Installation

1. **Clone the Repository**:
   ```bash
   git clone https://github.com/yourusername/ESP32-EvilTwin-Captive.git
   cd ESP32-EvilTwin-Captive
   ```

2. **Install PlatformIO** (if not already installed):
   - Download and install from [platformio.org](https://platformio.org/)
   - Or install via VS Code extension

3. **Install Dependencies**:
   - PlatformIO will automatically download required libraries based on `platformio.ini`
   - Required libraries: WebSockets, DNSServer, ESPAsyncWebServer, AsyncTCP

## Setup

1. **Configure Settings** (optional):
   - Edit `include/config.h` to customize settings like AP name, password, ports, etc.
   - Default control AP: SSID "ESP32-Control", Password "12345678"

2. **Build and Upload**:
   - Open the project in PlatformIO
   - Connect your ESP32 via USB
   - Run `PlatformIO: Build` then `PlatformIO: Upload`
   - Monitor serial output: `PlatformIO: Serial Monitor` (115200 baud)

3. **Initial Boot**:
   - ESP32 will create a control access point
   - Connect to "ESP32-Control" with password "12345678"
   - Access via Serial or WebSocket (port 81)

## Usage

### Connecting to the Device

1. **Via Serial**:
   - Use PlatformIO Serial Monitor or any serial terminal at 115200 baud
   - The device will display a menu on startup

2. **Via WebSocket** (Remote Access):
   - Connect to the control AP: "ESP32-Control" / "12345678"
   - Open `terminal_client.html` in a browser (or host it locally)
   - Enter WebSocket URL: `ws://192.168.4.1:81` (default IP)
   - Click Connect to access the interactive terminal

### Main Menu Options

- **[1] Scan WiFi Networks**: Scan and display available networks
- **[2] Clone Captive Portal**: Connect to open networks and clone their portals
- **[3] View Cloned Portal Info**: Display information about cloned portals
- **[4] Start Captive Portal**: Launch evil twin with selected portal type
- **[5] Stop Captive Portal**: Stop the running portal
- **[6] View Captured Credentials**: Display harvested credentials
- **[7] System Information**: Show ESP32 system details
- **[8] Network Information**: Display network status
- **[9] Clear Captured Credentials**: Remove stored credentials
- **[C] Clear Cloned Portal**: Remove cloned portal data
- **[0] Restart ESP32**: Reboot the device
- **[h] Help / Show Menu**: Display this menu

### Portal Types

When starting a captive portal, choose from:
- Generic Wi-Fi Portal
- Hotel Wi-Fi Portal
- Airport Wi-Fi Portal
- Coffee Shop Wi-Fi Portal
- Cloned Portal (from previously cloned network)

### Cloning a Portal

1. Select option [2] to clone
2. Device connects to open networks automatically
3. Detects and downloads captive portal HTML
4. Use option [4] then [5] to serve the cloned portal

## Configuration

Key settings in `include/config.h`:

- `CONTROL_AP_SSID`: Control access point name
- `CONTROL_AP_PASSWORD`: Control AP password
- `WEBSOCKET_PORT`: WebSocket port (default 81)
- `ENABLE_WEBSOCKET`: Enable/disable WebSocket server
- `ENABLE_LED`: Enable LED indicators
- Various timeouts and limits

## Troubleshooting

- **Build Errors**: Ensure all libraries are installed via PlatformIO
- **Upload Fails**: Check USB connection and board selection in PlatformIO
- **No Control AP**: Verify ESP32 is powered and flashed correctly
- **WebSocket Issues**: Ensure you're connected to the control AP and using correct URL

## Contributing

This is a research project. Contributions for educational improvements are welcome, but please maintain ethical guidelines.

## License

This project is provided as-is for educational purposes. See LICENSE file for details.

---

**Remember: Use this tool responsibly and only on networks you own or have permission to test.**
