# OpenMenuOS Documentation

Welcome to the OpenMenuOS documentation! This library provides a professional menu system for Arduino projects using TFT displays.

## Quick Links

- 📚 **[Complete API Reference](api.md)** - Detailed API documentation
- 🚀 **[Getting Started](#getting-started)** - Quick setup guide
- 💡 **[Examples](#examples)** - Code examples and tutorials
- 🛠️ **[Hardware Setup](#hardware-setup)** - Wiring and hardware requirements

## Getting Started

### Installation
Install OpenMenuOS through the Arduino Library Manager:
1. Open Arduino IDE
2. Go to `Tools` → `Manage Libraries...`
3. Search for "OpenMenuOS"
4. Click `Install`

### Basic Usage
```cpp
#include "OpenMenuOS.h"

OpenMenuOS menu(19, -1, 2);  // up, down, select pins
MenuScreen mainMenu("Main Menu");

void setup() {
  mainMenu.addItem("Settings", nullptr, showSettings);
  menu.begin(&mainMenu);
}

void loop() {
  menu.loop();
}

void showSettings() {
  PopupManager::showInfo("Settings would open here", "Settings");
}
```

## Key Features

### 🎛️ Professional Menu System
- Multi-level menus with unlimited depth
- Responsive design for any display size
- Customizable themes and styling
- Smooth animations and transitions

### 💬 Advanced PopupManager
- 5 popup types: Info, Success, Warning, Error, Question
- Auto-close functionality
- Encoder and button support
- Visual feedback and animations

### ⚙️ Comprehensive Settings
- Boolean, Range, Option, and Subscreen settings
- Automatic EEPROM/Preferences storage
- Real-time value updates
- Animated controls

### 🎮 Multiple Input Methods
- Button support with debouncing
- Rotary encoder with interrupts
- Configurable pin assignments
- Long-press detection

## Hardware Setup

### Minimum Requirements
- **ESP32** or **ESP8266** microcontroller
- **TFT display** supported by TFT_eSPI
- **3 GPIO pins** for buttons (optional encoder)

### Recommended Setup
```
ESP32/ESP8266    TFT Display
GND      ←→      GND
3.3V     ←→      VCC
GPIO_X   ←→      CS
GPIO_Y   ←→      DC
GPIO_Z   ←→      MOSI
...etc (see TFT_eSPI docs)

Buttons:
GPIO_19  ←→      UP button
GPIO_2   ←→      SELECT button

Encoder (optional):
GPIO_5   ←→      CLK
GPIO_2   ←→      DT
GPIO_19  ←→      SW (button)
```

## Examples

### Simple Menu
```cpp
MenuScreen mainMenu("Device");
mainMenu.addItem("LED Control", nullptr, toggleLED);
mainMenu.addItem("Settings", &settingsScreen);
```

### Popup Notifications
```cpp
// Success notification (auto-closes)
PopupManager::showSuccess("Data saved successfully!");

// Question dialog
PopupResult result = PopupManager::showQuestion("Delete all data?", "Confirm");
if (result == PopupResult::OK) {
  // User confirmed
}
```

### Settings Configuration
```cpp
SettingsScreen settings("Device Settings");
settings.addBooleanSetting("WiFi", true);
settings.addRangeSetting("Brightness", 0, 100, 75, "%");

const char* modes[] = {"Auto", "Manual", "Off"};
settings.addOptionSetting("Mode", modes, 3, 0);
```

### Custom Screens
```cpp
CustomScreen aboutScreen("About");
aboutScreen.customDraw = []() {
  canvas.fillScreen(TFT_BLACK);
  canvas.setTextColor(TFT_WHITE);
  canvas.drawString("Custom Content", 10, 50);
  canvas.fillCircle(100, 100, 20, TFT_BLUE);
};
```

## Advanced Features

### Performance Optimization
```cpp
menu.setOptimizeDisplayUpdates(true);  // Enable frame comparison
menu.setAnimation(false);              // Disable for better performance
```

### Visual Customization
```cpp
menu.useStylePreset("Rabbit_R1");           // Built-in theme
menu.setSelectionBorderColor(TFT_CYAN);     // Custom colors
menu.setScrollbar(true);                    // Enable scrollbar
```

### Input Configuration
```cpp
menu.setEncoderPin(5, 2);           // Enable encoder support
menu.setButtonsMode("High");        // Button logic level
```

## Support

- 📖 **Documentation**: Complete API reference in [api.md](api.md)
- 🐛 **Issues**: Report bugs on [GitHub Issues](https://github.com/The-Young-Maker/OpenMenuOS/issues)
- 💡 **Feature Requests**: Suggest improvements on GitHub
- ⭐ **Star** the repository if you find it useful!

## License

MIT License - free for personal and commercial use.

---

**OpenMenuOS v3.1.0** - Professional Menu System for Arduino  
Created by The Young Maker
