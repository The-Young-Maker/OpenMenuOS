# OpenMenuOS Examples

Quick reference guide with practical examples for common use cases.

## Basic Setup

### Minimal Setup
```cpp
#include "OpenMenuOS.h"

OpenMenuOS menu;

void setup() {
  menu.begin();
  menu.setupButtons(19, -1, 2, INPUT_PULLUP, LOW);
  
  menu.setMainMenuItems({"Option 1", "Option 2", "Settings"});
  menu.createMainMenu();
}

void loop() {
  menu.loop();
}
```

### With Encoder Support
```cpp
#include "OpenMenuOS.h"

OpenMenuOS menu;

void setup() {
  menu.begin();
  menu.setupButtons(19, -1, 2, INPUT_PULLUP, LOW);
  menu.setEncoderPin(5, 4);  // CLK, DT pins
  
  menu.setMainMenuItems({"LED Control", "Settings", "About"});
  menu.createMainMenu();
}

void loop() {
  menu.loop();
}
```

## Menu Examples

### Simple Menu with Callbacks
```cpp
MenuScreen mainMenu("Device Control");

void setup() {
  mainMenu.addItem("Toggle LED", nullptr, toggleLED);
  mainMenu.addItem("Show Info", nullptr, showDeviceInfo);
  mainMenu.addItem("Reset", nullptr, confirmReset);
  
  menu.addScreen(&mainMenu);
  menu.navigateToScreen(0);
}

void toggleLED() {
  static bool ledState = false;
  ledState = !ledState;
  digitalWrite(LED_BUILTIN, ledState);
  PopupManager::showSuccess(ledState ? "LED ON" : "LED OFF");
}

void showDeviceInfo() {
  String info = "Device: ESP32\\n";
  info += "Firmware: v1.0\\n";
  info += "Free RAM: " + String(ESP.getFreeHeap()) + " bytes";
  PopupManager::showInfo(info, "Device Information");
}

void confirmReset() {
  PopupResult result = PopupManager::showQuestion(
    "This will reset all settings.\\nAre you sure?",
    "Confirm Reset"
  );
  
  if (result.buttonIndex == 0) {  // OK button
    // Perform reset
    PopupManager::showSuccess("System reset complete!");
  }
}
```

### Nested Menu Structure
```cpp
MenuScreen mainMenu("Main Menu");
MenuScreen wifiMenu("WiFi Settings");
MenuScreen deviceMenu("Device Settings");

void setup() {
  // Main menu
  mainMenu.addItem("WiFi", &wifiMenu);
  mainMenu.addItem("Device", &deviceMenu);
  mainMenu.addItem("About", nullptr, showAbout);
  
  // WiFi submenu
  wifiMenu.addItem("Connect", nullptr, connectWiFi);
  wifiMenu.addItem("Scan Networks", nullptr, scanNetworks);
  wifiMenu.addItem("Disconnect", nullptr, disconnectWiFi);
  
  // Device submenu
  deviceMenu.addItem("LED Control", nullptr, toggleLED);
  deviceMenu.addItem("Restart", nullptr, restartDevice);
  
  menu.addScreen(&mainMenu);
  menu.addScreen(&wifiMenu);
  menu.addScreen(&deviceMenu);
  menu.navigateToScreen(0);
}
```

## Settings Examples

### Basic Settings Screen
```cpp
SettingsScreen settings("System Settings");

void setup() {
  settings.addBooleanSetting("WiFi Enabled", true);
  settings.addBooleanSetting("Auto Sleep", false);
  settings.addRangeSetting("Brightness", 0, 100, 75, "%");
  settings.addRangeSetting("Volume", 0, 10, 5);
  
  const char* themes[] = {"Dark", "Light", "Auto"};
  settings.addOptionSetting("Theme", themes, 3, 0);
  
  menu.addScreen(&settings);
}

// Access setting values
void checkSettings() {
  bool wifiEnabled = settings.getBooleanValue(0);
  int brightness = settings.getRangeValue(2);
  int themeIndex = settings.getOptionValue(4);
  
  if (wifiEnabled) {
    // Enable WiFi
  }
  
  // Set display brightness
  setBrightness(brightness);
}
```

### Advanced Settings with Callbacks
```cpp
SettingsScreen settings("Advanced Settings");

void setup() {
  settings.addBooleanSetting("Debug Mode", false);
  settings.addRangeSetting("Timeout", 5, 60, 30, "s");
  
  // Add callback for when settings change
  settings.onSettingChanged = [](int index, SettingType type) {
    switch (index) {
      case 0:  // Debug mode changed
        bool debugMode = settings.getBooleanValue(0);
        Serial.println("Debug mode: " + String(debugMode ? "ON" : "OFF"));
        break;
        
      case 1:  // Timeout changed
        int timeout = settings.getRangeValue(1);
        setSystemTimeout(timeout * 1000);  // Convert to milliseconds
        break;
    }
  };
}
```

## Popup Examples

### Information Popups
```cpp
void showStatus() {
  // Auto-closing info popup
  PopupManager::showInfo("System is running normally", "Status", 2000);
}

void showDetailedInfo() {
  String details = "System Status:\\n";
  details += "• CPU: Normal\\n";
  details += "• Memory: 78% free\\n";
  details += "• Network: Connected\\n";
  details += "• Sensors: All OK";
  
  PopupManager::showInfo(details, "Detailed Status");
}
```

### User Confirmation
```cpp
void deleteAllData() {
  PopupResult result = PopupManager::showQuestion(
    "This will permanently delete all stored data.\\n\\nThis action cannot be undone.",
    "Delete All Data?",
    {"Delete", "Cancel"}
  );
  
  if (result.buttonIndex == 0) {
    // User confirmed deletion
    eraseAllData();
    PopupManager::showSuccess("All data deleted successfully");
  }
}

void saveConfiguration() {
  std::vector<String> options = {"Save", "Save & Exit", "Cancel"};
  PopupResult result = PopupManager::showQuestion(
    "Save current configuration?",
    "Save Changes",
    options
  );
  
  switch (result.buttonIndex) {
    case 0:  // Save
      saveConfig();
      PopupManager::showSuccess("Configuration saved");
      break;
      
    case 1:  // Save & Exit
      saveConfig();
      menu.goToMainMenu();
      break;
      
    case 2:  // Cancel
      // Do nothing
      break;
  }
}
```

### Status Notifications
```cpp
void performLongOperation() {
  PopupManager::showInfo("Processing, please wait...", "Working", 0);  // No auto-close
  
  // Simulate long operation
  delay(3000);
  
  // Close working popup and show result
  PopupManager::close();
  PopupManager::showSuccess("Operation completed successfully!");
}

void handleErrors() {
  if (!wifiConnected) {
    PopupManager::showError("WiFi connection failed", "Network Error");
    return;
  }
  
  if (batteryLow) {
    PopupManager::showWarning("Battery level is low (15%)", "Low Battery");
  }
  
  // Continue with operation...
}
```

## Custom Screen Examples

### Information Display Screen
```cpp
class InfoScreen : public CustomScreen {
private:
  String deviceName;
  String firmwareVersion;
  String ipAddress;
  
public:
  InfoScreen(const String& title) : CustomScreen(title) {
    customDraw = [this]() { this->drawInfo(); };
  }
  
  void updateInfo(const String& name, const String& version, const String& ip) {
    deviceName = name;
    firmwareVersion = version;
    ipAddress = ip;
  }
  
private:
  void drawInfo() {
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextDatum(MC_DATUM);
    
    int centerX = canvas.width() / 2;
    int y = 40;
    
    canvas.drawString("Device Information", centerX, y);
    y += 30;
    
    canvas.setTextColor(TFT_CYAN);
    canvas.drawString("Name: " + deviceName, centerX, y);
    y += 20;
    
    canvas.drawString("Firmware: " + firmwareVersion, centerX, y);
    y += 20;
    
    canvas.drawString("IP: " + ipAddress, centerX, y);
    y += 30;
    
    canvas.setTextColor(TFT_YELLOW);
    canvas.drawString("Press SELECT to return", centerX, y);
  }
};

// Usage
InfoScreen infoScreen("About Device");
infoScreen.updateInfo("My Device", "v1.2.3", "192.168.1.100");
menu.addScreen(&infoScreen);
```

### Real-time Data Display
```cpp
class SensorScreen : public CustomScreen {
private:
  float temperature;
  float humidity;
  int lightLevel;
  unsigned long lastUpdate;
  
public:
  SensorScreen(const String& title) : CustomScreen(title), lastUpdate(0) {
    customDraw = [this]() { this->drawSensors(); };
  }
  
  void updateSensors(float temp, float hum, int light) {
    temperature = temp;
    humidity = hum;
    lightLevel = light;
    lastUpdate = millis();
  }
  
private:
  void drawSensors() {
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE);
    canvas.setTextDatum(TL_DATUM);
    
    // Title
    canvas.drawString("Sensor Readings", 10, 10);
    
    // Temperature
    canvas.setTextColor(TFT_RED);
    canvas.drawString("Temperature:", 10, 40);
    canvas.drawString(String(temperature, 1) + "°C", 120, 40);
    
    // Humidity
    canvas.setTextColor(TFT_BLUE);
    canvas.drawString("Humidity:", 10, 60);
    canvas.drawString(String(humidity, 1) + "%", 120, 60);
    
    // Light level
    canvas.setTextColor(TFT_YELLOW);
    canvas.drawString("Light:", 10, 80);
    canvas.drawString(String(lightLevel) + " lux", 120, 80);
    
    // Last update time
    canvas.setTextColor(TFT_GRAY);
    canvas.drawString("Updated: " + String((millis() - lastUpdate) / 1000) + "s ago", 10, 110);
    
    // Draw simple bar graphs
    drawBarGraph(10, 130, 100, 10, lightLevel, 1000, TFT_YELLOW);
    drawBarGraph(10, 150, 100, 10, (int)temperature, 50, TFT_RED);
    drawBarGraph(10, 170, 100, 10, (int)humidity, 100, TFT_BLUE);
  }
  
  void drawBarGraph(int x, int y, int width, int height, int value, int maxValue, uint16_t color) {
    // Background
    canvas.drawRect(x, y, width, height, TFT_WHITE);
    
    // Fill
    int fillWidth = map(value, 0, maxValue, 0, width - 2);
    canvas.fillRect(x + 1, y + 1, fillWidth, height - 2, color);
  }
  
  void readSensors() {
    // Simulate sensor readings
    temperature = 20 + (random(100) / 10.0);
    humidity = 40 + random(40);
    lightLevel = random(1000);
    lastUpdate = millis();
  }
};

// Usage
SensorScreen sensorScreen("Live Sensors");
menu.addScreen(&sensorScreen);
```

## Theme and Styling Examples

### Custom Theme
```cpp
void setupCustomTheme() {
  // Use built-in theme as base
  menu.useStylePreset("Rabbit_R1");
  
  // Customize colors
  menu.setSelectionBorderColor(TFT_CYAN);
  menu.setBackgroundColor(TFT_NAVY);
  
  // Configure scrollbar
  menu.setScrollbar(true);
  menu.setScrollbarColor(TFT_DARKGREY);
  
  // Configure animations
  menu.setAnimation(true);
  menu.setTextScroll(true);
}
```

### Performance-Optimized Setup
```cpp
void setupForPerformance() {
  // Disable visual effects for better performance
  menu.setAnimation(false);
  menu.setTextScroll(false);
  menu.setScrollbar(false);
  
  // Enable frame comparison for ESP32
  #ifdef ESP32
    menu.setOptimizeDisplayUpdates(true);
  #else
    menu.setOptimizeDisplayUpdates(false);  // ESP8266 with large displays
  #endif
}
```

## Input Handling Examples

### Button-Only Setup
```cpp
void setup() {
  // Configure for 3-button operation
  menu.setupButtons(
    19,              // UP button pin
    18,              // DOWN button pin
    2,               // SELECT button pin
    INPUT_PULLUP,    // Use internal pullup resistors
    LOW              // Button pressed = LOW
  );
}
```

### Encoder-Only Setup
```cpp
void setup() {
  // Configure for encoder + button operation
  menu.setupButtons(
    -1,              // No UP button
    -1,              // No DOWN button
    2,               // SELECT button (encoder button)
    INPUT_PULLUP,
    LOW
  );
  
  menu.setEncoderPin(5, 4);  // CLK, DT pins
}
```

### Hybrid Input Setup
```cpp
void setup() {
  // Both buttons and encoder
  menu.setupButtons(19, 18, 2, INPUT_PULLUP, LOW);
  menu.setEncoderPin(5, 4);
  
  // Users can navigate with either input method
}
```

## Advanced Features

### Auto-Save Settings
```cpp
class AutoSaveSettings : public SettingsScreen {
public:
  AutoSaveSettings(const String& title) : SettingsScreen(title) {
    onSettingChanged = [this](int index, SettingType type) {
      // Auto-save when any setting changes
      this->saveToEEPROM();
      PopupManager::showSuccess("Settings saved", "Auto-Save", 1000);
    };
  }
};
```

### Conditional Menu Items
```cpp
void updateMenuItems() {
  mainMenu.clearItems();
  
  mainMenu.addItem("Status", nullptr, showStatus);
  
  if (wifiConnected) {
    mainMenu.addItem("Cloud Sync", nullptr, syncToCloud);
    mainMenu.addItem("Remote Control", &remoteMenu);
  } else {
    mainMenu.addItem("Connect WiFi", nullptr, connectWiFi);
  }
  
  if (isAdminMode) {
    mainMenu.addItem("Admin Panel", &adminMenu);
  }
  
  mainMenu.addItem("Settings", &settingsMenu);
}
```

### Progress Indication
```cpp
void showProgress() {
  for (int i = 0; i <= 100; i += 10) {
    String message = "Processing... " + String(i) + "%";
    PopupManager::showInfo(message, "Please Wait", 500);
    
    // Do work
    delay(200);
  }
  
  PopupManager::showSuccess("Process completed!");
}
```

---

For more examples, check the `examples/` folder in the library directory.

**OpenMenuOS Examples Guide v3.1.0**  
Documentation for practical implementation patterns
