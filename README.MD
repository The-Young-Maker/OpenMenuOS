> ⚠️ **Warning: Version 3.0 is Out!**  
> The documentation below is outdated and will be updated soon when I have time. You can explore the example provided in the examples folder to understand how the new version works.  

### **What's New in v3.0.0?**  
Here are the significant updates and improvements:  

#### **Code Refactoring and Fixes**  
- Variables moved to the private section of the class for better encapsulation and maintainability.  
- Fixed issues with scrollbar height for submenus.  
- Fixed scrolling text issues with non-black background colors.  

#### **Enhanced Menu System**  
- Menus (main, submenus, and settings) now dynamically adapt to the display size.  
- Unlimited screens and menu levels—replacing the previous limitation of 3 levels.  
- Fixed main menu item scrolling issues when returning from submenus.  

#### **Settings Overhaul**  
- Replaced the old boolean-only settings system with object-based settings.  
  - Supports a variety of data types: options, ranges, and booleans.  
  - Includes default value support (overridden when values are changed in EEPROM).  
  - Note: Only booleans are fully functional; range and option settings are partially implemented and do not currently work.  
.  

#### **New Features**  
- Added a function to retrieve the library version.  
- Added smoother text scrolling.  
- Added the ability to set a boot image.  
- Added font customization options for menus.  

#### **Storage Handling**  
- Improved storage handling by using `preferences.h` on ESP32 and `EEPROM` on ESP8266.  

#### **Additional Fixes**  
- Resolved an issue where long-pressing the select button did not correctly return to the last screen.  
- Removed the `tft_bl` argument from the menu instance (can now be set in `TFT_eSPI`'s config).  

This is a major release with a lot of exciting changes, and I'm working to update the documentation soon. Thanks for your patience!
 

![OpenMenuOS GIF Demo](images/OpenMenuOS_GIF.gif)  
This demo was recorded using the **"Screenshot"** feature provided by TFT_eSPI.  
The visible lines on the recording are not present on the actual display; they are likely caused by noise during data transmission from the display to the computer.  

Additionally, while the demo may appear laggy, this is due to the low recording frame rate. The display could not refresh while the screenshot data was being transmitted.


# OpenMenuOS Documentation

#### This library is continuously evolving, with numerous features and bug fixes in the pipeline. Stay tuned for exciting updates!

#### If you have any suggestions to enhance the library or encounter any bugs, please don't hesitate to create a new issue. Your contributions play a vital role in shaping the future of this library.


#### This library has been tested on ST7735 and ST7789 Color TFT Displays but should work seamlessly with any display supported by the TFT_eSPI library.

## Introduction

OpenMenuOS is a versatile open-source menu system for ESP32 and ESP8266, designed to simplify the implementation of user interfaces in various applications. This documentation provides an overview of the OpenMenuOS library, including its features, usage, and customization options.

## Features

- Intuitive menu navigation system
- Support for multiple menu levels including submenus, settings, and tile menu 
- Flexible configuration options for button and display pins
- Customizable menu item names and icons
- Additional support for images and icons beyond menu items
- Automatic scrolling of long item names
- Different menu styles
- Redirection to other parts of the menu (e.g., Settings)
- Functionality to draw popups

## Getting Started

### Installation

Download the OpenMenuOS library and install it in your Arduino IDE.

### Setup

Include the `OpenMenuOS.h` header file in your sketch to access the OpenMenuOS functionality.

## Initialization

### Constructor

To begin using OpenMenuOS, create an instance of the OpenMenuOS class. Specify the button pins (UP, DOWN, SELECT), TFT backlight pin, and display control pins (CS, DC, RST).

#### NOTE: OpenMenuOS relies on the TFT_eSPI library for display handling. Ensure your display settings are properly configured according to the TFT_eSPI library's documentation.

#### NOTE: For all the documentation, I will use `menu` as the constructor

Example:
```
OpenMenuOS menu(
  Button UP Pin,
  Button DOWN Pin,
  Button SELECT Pin,
  TFT Backlight Pin,
)
```

Example Use:

`OpenMenuOS menu(10, 11, 5, 12);`

### begin()

Example:
```
menu.begin(
Display rotation, 
)
```

Example Use: 

`menu.begin(1);`

Initializes the OpenMenuOS library. Call this function in the setup() function of your sketch.

### loop()

`menu.loop()`

The main loop function of the OpenMenuOS library. Call this function in the loop() function of your sketch. This function checks for button presses.

## Display Functions

### drawMenu()

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

### drawSubmenu()

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

### drawSettingMenu()

Draws the settings menu on the display.
NOTE: There are by default 1 item in the settings (For the backlight)

Example:
```
menu.drawSettingMenu(
  Name of the item separated by a comma (You can go up to 5),
  NULL
)
```

Example Use: 

`menu.drawSettingMenu("Light", "Sound", "Deep-Sleep", NULL);`

### drawTileMenu()

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

### redirectToMenu()

Redirects to a menu.

Example:
```
menu.redirectToMenu(
Screen (The number of the screen ex.:0,1,2),
Item (The number of the item)
)
```

Example Use: 

```
    menu.redirectToMenu(0, 2);
    menu.redirectToMenu(1, 3);
```

#### Note: It needs to be done in step. Ex: you can not pass from screen 2 to 0 in one call of the function. You need to go from 2 to 1, and from 1 to 0.

### drawPopup()

Show a popup on the screen.

Example:
```
menu.drawPopup(
Message,
Variable to see if the button as been clicked
)
```

Example Use: 

```       
 bool clicked;
 menu.drawPopup("Set backlight?", clicked);
  if (clicked) {
    // If clicked....
  }
```
### scrollTextHorizontal()
Example:
```
menu.setTextScroll(
int16_t x,           // X coordinate
int16_t y,           // Y coordinate
const char* text,    // Text to be scrolled
uint16_t textColor,  // Text color
uint16_t bgColor,    // Background color
uint8_t textSize,    // Size of the text
uint16_t delayTime,  // Delay between the frame of the scroll
uint16_t windowSize  // Size of the window where the text will be displayed (the scrolling text is only display in that window)
)
```

Example Use: 

`menu.scrollTextHorizontal(10, 44,"Hello, World!", ST7735_WHITE, ST7735_BLACK, 1, 50, 100);`

### setTextScroll()

Example:
```
menu.setTextScroll(
bool textScroll, 
)
```

Example Use: 

`menu.setTextScroll(false);`

### drawScrollbar()

Example:
```
menu.drawScrollbar(
int selectedItem,
int nextItem
)
```

Example Use: 

`menu.drawScrollbar(item_selected, item_selected_next);`

#### Note: You should pass the argument with variable like in the example use.

### setButtonAnimation()

Example:
```
menu.setButtonAnimation(
bool button animation (true or false)
)
```

Example Use: 

`menu.setButtonAnimation(false);`

#### Note: Button animation is not available with style "1"

### setMenuStyle()

Example:
```
menu.setMenuStyle(
int style // Current style available : 0 (Default), 1 (Modern)
)
```

Example Use: 

`menu.setMenuStyle(1);`

### setSelectionColor()

Example:
```
menu.setSelectionColor(
uint16_t color // Setting the selection rectangle's color
)
```

Example Use: 

`menu.setSelectionColor(0xfa60);`
OR
`menu.setSelectionColor(ST7735_RED);`

### useStylePreset()

Example:
```
menu.useStylePreset(
char* preset // Current preset available : "Default" and "Rabbit_R1"
)
```

Example Use: 

`menu.useStylePreset(Rabbit_R1);`

### void drawCanvasOnTFT()

Draws the canvas on the TFT display. You need to call it at the END of your code (in the end of "loop()")

## Button Handling

#### checkForButtonPress()

Checks for button presses and handles navigation.

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

## Known Bug

1. The scrolling text in the settings is flickering when not supposed.
2. When long pressing the select button to go back, menu item scrolls when they shouldn't

The know bugs are also labeled as "#bug" in the code. Just do "CTRL+F" and search for "#bug"

## Additional Notes
The library is a work in progress. If you encounter any bugs or if you have any suggestion, please create an issue on the [OpenMenuOS GitHub repository](https://github.com/The-Young-Maker/OpenMenuOS).
Ensure that you have the necessary dependencies installed and configured in your Arduino IDE.
