# OpenMenuOS Documentation

#### This library is continuously evolving, with numerous features and bug fixes in the pipeline. Stay tuned for exciting updates!

#### If you have any suggestions to enhance the library or encounter any bugs, please don't hesitate to create a new issue. Your contributions play a vital role in shaping the future of this library.

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

To begin using OpenMenuOS, create an instance of the OpenMenuOS class. Specify the button pins (UP, SELECT), TFT backlight pin, and display control pins (CS, DC, RST).

#### NOTE : For all the documentation, I will use menu as the constructor

Example:
```
OpenMenuOS menu(
  Button UP Pin,
  Button DOWN Pin,
  Button SELECT Pin,
  TFT Backlight Pin,
  Display CS Pin,
  Display DC Pin,
  Display RST Pin
)
```

Example Use:

`OpenMenuOS menu(10, 11, 5, 12, 4, 21, 22);`

### begin()

Example:
```
menu.begin(
Display type, 
OTA hostname, 
OTA password
)
```

Example Use: 

`menu.begin(INITR_GREENTAB, "ESP32_OpenMenuOS", "esp32");`


Initializes the OpenMenuOS library. Call this function in the setup() function of your sketch.

### loop()

`menu.loop()`

The main loop function of the OpenMenuOS library. Call this function in the loop() function of your sketch. This function checks for button presses and handle the OTA.
## Function

### Display Functions

#### drawMenu()

Draws the menu on the display. Provide the names of menu items separated by commas, with a maximum of 20 items. Remember to end the list with "NULL".

Example:
```
menu.drawMenu(
  Name of the item separated by a comma (You can go up to 20),
  NULL
)
```

Example Use: 

`menu.drawMenu("Flashlight", "Profile", "Heat", "Reflow", "Settings", "Wifi", "Package", "Survival", "Other", NULL);`

#### drawSubmenu()

Draws the menu on the display. Provide the names of menu items separated by commas, with a maximum of 20 items. Remember to end the list with "NULL".

Example:
```
menu.drawSubmenu(
  Name of the item separated by a comma (You can go up to 20),
  NULL
)
```

Example Use: 

`menu.drawMenu("Submenu_1", "Submenu_2", "Submenu_3", "Submenu_4", NULL);`

#### drawSettingMenu()

Draws the settings menu on the display.
NOTE: There are by default 5 settings which can not be changed

Example:
```
menu.drawSettingMenu(
  Name of the item separated by a comma (You can go up to 5),
  NULL
)
```

Example Use: 

`menu.drawSettingMenu("Light", "Sound", "Deep-Sleep", NULL);`

#### drawTileMenu()

Draws a tile menu on the display.

Example:
```
menu.drawSettingMenu(
Number of rows,
Number of columns,
Color of the tiles
)
```

Example Use: 

`menu.drawTileMenu(2, 4, ST7735_GREEN);`

#### void drawCanvasOnTFT()

Draws the canvas on the TFT display. You need to call it at the END of your code (in the end of "loop()")

### Button Handling

#### checkForButtonPress()

Checks for button presses and handles navigation.

#### checkSerial()

Checks for input from the serial monitor for configuring Wi-Fi settings.

### Settings Management

####  saveToEEPROM()

Saves settings to EEPROM memory.

#### readFromEEPROM()

Reads settings from EEPROM memory.

### Utility Functions

#### printMenuToSerial()

Prints the menu items to the serial monitor.

#### getCurrentScreen()

Returns the current screen (0 for menu, 1 for submenu).

#### getSelectedItem()

Returns the index of the selected menu item.

#### getCurrentScreenTileMenu()

Returns the current screen (0 for tile menu, 1 for tile submenu).

#### getSelectedItemTileMenu()

Returns the index of the selected tile menu item.

#### getTftHeight()

Returns the height of the display (in pixels).

#### getTftWidth()

Returns the width of the display (in pixels).

#### UpButton()

Returns the status (HIGH or LOW) of the Up button.

#### DownButton()

Returns the status (HIGH or LOW) of the Down button.

#### SelectButton()

Returns the status (HIGH or LOW) of the Select button.

## Menu Navigation

#### Moving Through Menu Items: 
Utilize the UP and DOWN button to navigate through the menu items.

#### Selecting an Item: 
Press the SELECT button to select a menu item.

#### Returning to Previous Menu: 
Perform a long press on the SELECT button to return to the previous menu level.

## Customization

### Menu Icons

OpenMenuOS supports menu icons to enhance the visual presentation of menu items. Icons should be provided as 16x16 pixel bitmaps and stored in an `images.cpp` file. Define each icon as a constant array of bytes.

### Additional Images

In addition to menu icons, OpenMenuOS allows for the integration of custom images or icons beyond menu items. These images can be used for boot screens, splash screens, or other graphical elements. Store additional images in the `images.cpp` file as constant arrays of bytes.


## Additional Notes
The library is still in beta. If you encounter any bugs or if you have any suggestion, please create an issue on the [OpenMenuOS GitHub repository](https://github.com/The-Young-Maker/OpenMenuOS).
Ensure that you have the necessary dependencies installed and configured in your Arduino IDE.
