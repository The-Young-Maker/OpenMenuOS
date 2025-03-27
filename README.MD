# OpenMenuOS Library Documentation

## **What's New in v3.0.0?**
Here are the significant updates and improvements:

### **Code Refactoring and Fixes**
- Variables moved to the private section of the class for better encapsulation and maintainability.
- Fixed issues with scrollbar height for submenus.
- Fixed scrolling text issues with non-black background colors.

### **Enhanced Menu System**
- Menus (main, submenus, and settings) now dynamically adapt to the display size.
- Unlimited screens and menu levels—replacing the previous limitation of 3 levels.
- Fixed main menu item scrolling issues when returning from submenus.

### **Settings Overhaul**
- Replaced the old boolean-only settings system with object-based settings.
  - Supports a variety of data types: options, ranges, and booleans.
  - Includes default value support (overridden when values are changed in EEPROM).
  - Note: Only booleans are fully functional; range and option settings are partially implemented and do not currently work.

### **New Features**
- Added a function to retrieve the library version.
- Added smoother text scrolling.
- Added the ability to set a boot image.
- Added font customization options for menus.

### **Storage Handling**
- Improved storage handling by using `preferences.h` on ESP32 and `EEPROM` on ESP8266.

### **Additional Fixes**
- Resolved an issue where long-pressing the select button did not correctly return to the last screen.
- Removed the `tft_bl` argument from the menu instance (can now be set in `TFT_eSPI`'s config).

This is a major release with a lot of exciting changes, and I'm working to update the documentation soon. Thanks for your patience!

![OpenMenuOS GIF Demo](images/OpenMenuOS_GIF.gif)  
This demo was recorded using the **"Screenshot"** feature provided by TFT_eSPI.  
The visible lines on the recording are not present on the actual display; they are likely caused by noise during data transmission from the display to the computer.  

Additionally, while the demo may appear laggy, this is due to the low recording frame rate. The display could not refresh while the screenshot data was being transmitted.

## Introduction
OpenMenuOS is an Arduino library designed to simplify the creation and display of menus on TFT displays supported by the TFT_eSPI library. It is compatible with ESP32 and ESP8266 boards. The library provides features for creating dynamic menus, custom screens, and handling user inputs.

## Installation
To install the OpenMenuOS library, follow these steps:
1. Download the OpenMenuOS library from the [GitHub repository](https://github.com/The-Young-Maker/OpenMenuOS).
2. Open the Arduino IDE.
3. Go to `Sketch` -> `Include Library` -> `Add .ZIP Library...`.
4. Select the downloaded ZIP file and click `Open`.
5. Alternatively, you can install it directly from the Arduino IDE 2.0 by searching for "OpenMenuOS" in the Library Manager.

## Usage
### Basic Example
Here is a basic example demonstrating how to use the OpenMenuOS library to create a simple menu system.

```c++
#include "OpenMenuOS.h"
#include "images.h"

// Create an instance of the OpenMenuOS class with button and display pins
OpenMenuOS menu(19, NULL, 2);  // btn_up, btn_down, btn_sel

// Create menu screens
MenuScreen mainMenu("Main Menu");
MenuScreen testScreen("Test Menu");
CustomScreen customScreen;

// Example action callback for redirection
void redirectToCustomScreen() {
  menu.redirectToScreen(&customScreen);
}

void setup() {
  // Custom drawing for the CustomScreen
  customScreen.customDraw = []() {
    canvas.drawSmoothRoundRect(-15, 50, 40, 0, 50, 50, TFT_BLUE, TFT_BLACK);
    canvas.drawSmoothRoundRect(10, 10, 200, 100, 20, 5, TFT_ORANGE, TFT_BLACK);
    canvas.drawSmoothRoundRect(120, -25, 40, 0, 40, 40, TFT_DARKGREEN, TFT_BLACK);
    canvas.drawString("V" + String(menu.getLibraryVersion()), 10, 10);
  };

  // Start Serial communication
  Serial.begin(921600);
  while (!Serial) {}
  Serial.println("Menu system started.");

  // Create a dynamic settings screen
  SettingsScreen* settingsScreen = new SettingsScreen("Settings");
  settingsScreen->addBooleanSetting("Enable Feature", true);
  settingsScreen->addRangeSetting("Brightness", 0, 100, 75);
  const char* colorOptions[] = { "Red", "Green", "Blue" };
  settingsScreen->addOptionSetting("Color", colorOptions, 3, 0);

  // Add menu items to the main menu
  mainMenu.addItem("Settings", settingsScreen, nullptr, (const uint16_t*)Menu_icon_1);
  mainMenu.addItem("Test Menu", &testScreen);
  mainMenu.addItem("Redirect To Screen", nullptr, redirectToCustomScreen);
  mainMenu.addItem("Update", nullptr);

  // Add items to the test menu
  testScreen.addItem("First Test Page");
  testScreen.addItem("Second Test Page");
  testScreen.addItem("Third Test Page");
  testScreen.addItem("Custom Screen", &customScreen);

  // Configure menu style and settings
  menu.useStylePreset("Rabbit_R1");
  menu.setScrollbar(true);
  menu.setScrollbarStyle(1);
  menu.setButtonsMode("High");

  // Initialize the menu system
  menu.begin(1, &mainMenu);
}

void loop() {
  menu.loop();
}
```

## API Reference
### Class: OpenMenuOS
- **OpenMenuOS(int btn_up, int btn_down, int btn_sel)**
  Constructor to initialize the menu system with button pins.
  
  Parameters:
  - `btn_up`: Pin number for the up button.
  - `btn_down`: Pin number for the down button.
  - `btn_sel`: Pin number for the select button.

- **void begin(int rotation, Screen* mainMenu)**
  Initialize the menu system with display rotation and main menu.

  Parameters:
  - `rotation`: Display rotation (0-3).
  - `mainMenu`: Pointer to the main menu screen.

- **void loop()**
  Handle menu logic, to be called in the `loop()` function.

- **void redirectToScreen(Screen* screen)**
  Redirect to a specified screen.

  Parameters:
  - `screen`: Pointer to the screen to redirect to.

- **void useStylePreset(char* preset)**
  Use predefined style presets.
  
  Parameters:
  - `preset`: Name of the style preset.

- **void setScrollbar(bool x)**
  Enable or disable the scrollbar.

  Parameters:
  - `x`: `true` to enable, `false` to disable.

- **void setScrollbarStyle(int style)**
  Set the style of the scrollbar.

  Parameters:
  - `style`: Style number (e.g., 0 for Default, 1 for Modern).

- **void setButtonsMode(char* mode)**
  Set the buttons' active voltage mode.

  Parameters:
  - `mode`: "High" or "Low".

- **void setMenuStyle(int style)**
  Set the menu style.

  Parameters:
  - `style`: Style number.

- **void setSelectionBorderColor(uint16_t color)**
  Set the color of the selection border.

  Parameters:
  - `color`: 16-bit color value.

- **void setSelectionFillColor(uint16_t color)**
  Set the color of the selection fill.

  Parameters:
  - `color`: 16-bit color value.

- **void setMenuFont(const GFXfont* font)**
  Set the font for the menu.

  Parameters:
  - `font`: Pointer to the GFXfont object.

- **void setMenuFontBold(const GFXfont* font)**
  Set the bold font for the menu.

  Parameters:
  - `font`: Pointer to the GFXfont object.

- **void drawCanvasOnTFT()**
  Draw the canvas on the TFT display.

### Class: MenuScreen
- **MenuScreen(const char* title)**
  Constructor to initialize a menu screen with a title.

  Parameters:
  - `title`: Title of the menu screen.

- **void addItem(const char* label, Screen* nextScreen, ActionCallback action, const uint16_t* image)**
  Add an item to the menu.

  Parameters:
  - `label`: Label of the menu item.
  - `nextScreen`: Pointer to the next screen (optional).
  - `action`: Action callback function (optional).
  - `image`: Pointer to the icon image (optional).

- **void draw()**
  Draw the menu screen.

- **void handleInput()**
  Handle user input for the menu screen.

### Class: SettingsScreen
- **SettingsScreen(const char* title)**
  Constructor to initialize a settings screen with a title.

  Parameters:
  - `title`: Title of the settings screen.

- **void addBooleanSetting(const char* name, bool defaultValue)**
  Add a boolean setting.

  Parameters:
  - `name`: Name of the setting.
  - `defaultValue`: Default value of the setting.

- **void addRangeSetting(const char* name, uint8_t min, uint8_t max, uint8_t defaultValue)**
  Add a range setting.

  Parameters:
  - `name`: Name of the setting.
  - `min`: Minimum value.
  - `max`: Maximum value.
  - `defaultValue`: Default value.

- **void addOptionSetting(const char* name, const char** options, uint8_t count, uint8_t defaultIndex)**
  Add an option setting.

  Parameters:
  - `name`: Name of the setting.
  - `options`: Array of option strings.
  - `count`: Number of options.
  - `defaultIndex`: Default selected index.

- **void saveToEEPROM()**
  Save settings to non-volatile storage.

- **void readFromEEPROM()**
  Read settings from non-volatile storage.

## Contributing
If you would like to contribute to the OpenMenuOS library, please fork the repository and submit a pull request with your changes. Make sure to follow the coding standards and provide detailed descriptions for your contributions.

## License
This library is licensed under the MIT License. See the [LICENSE](https://github.com/The-Young-Maker/OpenMenuOS/blob/main/LICENSE) file for more information.
