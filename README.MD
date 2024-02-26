# OpenMenuOS Documentation

## Introduction

OpenMenuOS is a versatile open-source menu system for ESP32, designed to simplify the implementation of user interfaces in various applications. This documentation provides an overview of the OpenMenuOS library, including its features, usage, and customization options.

## Features

- Intuitive menu navigation system
- Support for multiple menu levels and submenus
- Flexible configuration options for button and display pins
- Customizable menu item names and icons
- Additional support for images and icons beyond menu items
- Integration with serial communication and WiFi functionality

## Getting Started

### Installation

Download the OpenMenuOS library and install it in your Arduino IDE.

### Setup

Include the `OpenMenuOS.h` header file in your sketch to access the OpenMenuOS functionality.

## Initialization

### Constructor

To begin using OpenMenuOS, create an instance of the OpenMenuOS class. Specify the button pins (UP, SELECT), TFT backlight pin, and display control pins (CS, DC, RST). Additionally, provide the names of menu items separated by commas, with a maximum of 20 items. Remember to end the list with "NULL".

#### NOTE : For all the documentation, I will use menu as the constructor

Example:
```
OpenMenuOS menu(
  Button UP Pin,
  Button SELECT Pin,
  TFT Backlight Pin,
  Display CS Pin,
  Display DC Pin,
  Display RST Pin,
  Name of the item separated by a comma (You can go up to 20),
  NULL
)
```

Example Use: `OpenMenuOS menu(10, 5, 12, 4, 21, 22, "Flashlight", "Profile", "Heat", "Reflow", "Settings", "Wifi", "Package", "Survival", "Other", NULL);`

### begin()

`menu.begin()`
Initializes the OpenMenuOS library. Call this function in the setup() function of your sketch.

### loop()

`menu.loop()`
The main loop function of the OpenMenuOS library. Call this function in the loop() function of your sketch. This function checks for button presses and handle the OTA.
## Function

#### Display Functions
void drawMenu(): Draws the menu on the display.
void drawSettingMenu(const char* items...): Draws the settings menu on the display.
void drawCanvasOnTFT(): Draws the canvas on the TFT display.
#### Button Handling
void checkForButtonPress(): Checks for button presses and handles navigation.
void checkSerial(): Checks for input from the serial monitor for configuring Wi-Fi settings.
#### Settings Management
void saveToEEPROM(): Saves settings to EEPROM memory.
void readFromEEPROM(): Reads settings from EEPROM memory.
#### Utility Functions
void printMenuToSerial(): Prints the menu items to the serial monitor.
int getCurrentScreen() const: Returns the current screen (0 for menu, 1 for settings).
int getSelectedItem() const: Returns the index of the selected menu item.



## Customization

### Menu Icons

OpenMenuOS supports menu icons to enhance the visual presentation of menu items. Icons should be provided as 16x16 pixel bitmaps and stored in an `images.h` file. Define each icon as a constant array of bytes.

### Additional Images

In addition to menu icons, OpenMenuOS allows for the integration of custom images or icons beyond menu items. These images can be used for boot screens, splash screens, or other graphical elements. Store additional images in the `images.h` file as constant arrays of bytes.


## Additional Notes
The library is still in beta. If you encounter any bugs, please create an issue on the [OpenMenuOS GitHub repository](https://github.com/The-Young-Maker/OpenMenuOS).
Ensure that you have the necessary dependencies installed and configured in your Arduino IDE.
