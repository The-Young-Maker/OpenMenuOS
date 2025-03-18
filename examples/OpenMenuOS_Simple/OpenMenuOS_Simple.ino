// 2024/04/06 - V1.4.0
// Fixed debouncing issue when returning to back menu
// Added menu button animation when pressed
// Added a function to redirect to menu
// Added option to choose between different menu style
// Added option to enable to disable scrollbar
// Added option to choose the color of the selection rectangle
// Added option to choose from preset menu style

// V2.0.0
// 2024/04/09 - Added option to choose PULLUP or PULLDOWN for the buttons - Possible bugs?

// 2024/04/18
// Fixed the bug where the backlight was not set at the defined state at boot
// Centered icon in the colored section of the popup

// 2024/05/03
// Modified the drawPopup function to have more flexibility

// 2024/05/05
// Fixed the issue where going back from settings would toggle the selected item. Now, the item is toggled only on short press.

// 2024/05/10
// Added support for custom images in images.cpp

//2024/05/12
// Added support for faster scrolling when up/down button is long pressed

// 2024/05/16
// Changed the display library to TFT_eSPI
// Added support to take "screenshots" of the display (All rights reserved to Bodmer)

// 2024/05/19
// Modified the drawing method so that they can be called by "canvas.draw..." instead of "menu.canvas.draw..."
// Modified instance creation by removing the display pins definition/arguments as they need to be set in the TFT_eSPI User Setup file
// Modified begin function to accept display rotation as argument
// Modified the drawing functions to use anti-aliasing to make them smoother

// 2024/05/26
// Fixed the color variables to make them work with all display
// Modified the tft height and width variable to get them from TFT_eSPI instead of being static

// 2024/06/03
// Added option to set scrollbar color
// Added option to set scrollbar style

// 2024/07/24
// Solved the bug that made the scrolling text flicker

// V3.0.0
// 2024/12/31
// Refactored the code by moving variables to the private section of the class for better encapsulation and maintainability.

// 2025/01/02
// Fixed the issue where the scrollbar did not have the right height for the submenues

// 2025/03/07
// Updated the code to make the menus (main, submenu and settings) dynamically adapt to the display size.
// Added a function to retrieve the library version.

// 2025/03/08
// Fixed the issue where the scrolling text did not show with background color other than black

// 2025/03/09
// Refactored the code to use `preferences.h` on ESP32 and `EEPROM` on ESP8266 for improved storage handling.
// Refactored the code to implement object-based settings, enhancing usability. The new approach supports a variety of data types, including options, ranges, and booleans, and also support default values (overriden when the value is changes in EEPROM) replacing the previous implementation that only supported booleans. Currently, only booleans are fully functional, and the range feature can only increase as it is not fully implemented yet.

// 2025/03/10
// The library has been upgraded with a completely new screen system.
// The previous "createSubmenus" function, which was limited to only 3 levels of menus, is no longer used.
// Now, you can have an unlimited number of screens and menu levels, allowing for more flexibility and scalability!

// This update also changes how menus are handled and fixes the issue where main menu items would unexpectedly scroll
// when returning from a submenu.

// Improved the text Scroll function to make the text scroll smoother
// Added a function to let user set a boot image

// 2025/03/11
// Added functions to let user ability modify the font of the menu

// 2025/03/17
// Fixed the issue that long pressing the select button did not correctly go back to the last screen and returned to the first screen instead
// Removed the tft_bl argument from the menu instance as it can already be set in tft_eSPI's config

#include "OpenMenuOS.h"  // Include the OpenMenuOS library
#include "images.h"

// Create an instance of the OpenMenuOS class with button and display pins
// Use NULL for buttons you don't have
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
  settingsScreen->addBooleanSetting("Enable Feature", true);      // Toggle setting
  settingsScreen->addRangeSetting("Brightness", 0, 100, 75);      // Brightness slider
  const char* colorOptions[] = { "Red", "Green", "Blue" };        // Dropdown for colors
  settingsScreen->addOptionSetting("Color", colorOptions, 3, 0);  // Color options

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
  menu.useStylePreset("Rabbit_R1");  // Available presets: "Default", "Rabbit_R1"
  menu.setScrollbar(true);           // Enable scrollbar
  menu.setScrollbarStyle(1);         // Styles: 0 (Default), 1 (Modern)
  // menu.setMenuStyle(1);                     // Styles: 0 (Default), 1 (Modern)
  // menu.setSelectionBorderColor(0xFFFF);     // Set selection border color
  // menu.setSelectionFillColor(0x0000);       // Set selection fill color
  menu.setButtonsMode("High");  // Button active voltage: "High" or "Low"

  // Initialize the menu system
  menu.begin(1, &mainMenu);  // Parameters: display rotation, main menu
}

void loop() {
  menu.loop();  // Handle menu logic
}