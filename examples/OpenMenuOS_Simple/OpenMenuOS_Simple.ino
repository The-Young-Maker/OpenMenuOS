//2024/04/06 - V1.4.0
// Fixed debouncing issue when returning to back menu
// Added menu button animation when pressed
// Added a function to redirect to menu
// Added option to choose between different menu style
// Added option to enable to disable scrollbar
// Added option to choose the color of the selection rectangle
// Added option to choose from preset menu style

// V1.5.0
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

//2024/05/16
// Changed the display library to TFT_eSPI
// Added support to take "screenshots" of the display (All rights reserved to Bodmer)

//2024/05/19
// Modified the drawing method so that they can be called by "canvas.draw..." instead of "menu.canvas.draw..."
// Modified instance creation by removing the display pins definition/arguments as they need to be set in the TFT_eSPI User Setup file
// Modified begin function to accept display rotation as argument
// Modified the drawing functions to use anti-aliasing to make them smoother

//2024/05/26
// Fixed the color variables to make them work with all display
// Modified the tft height and width variable to get them from TFT_eSPI instead

//2024/06/03
// Added option to set scrollbar color
// Added option to set scrollbar style

//2024/07/24
// Solved the bug that made the scrolling text flicker

// TODO : 
//  Clean and reorganize drawMenu() and drawSubmenu() function
//  Change function for boot image to make it possible to change the boot image and its size
//  Improve the functionning of popups

#include "OpenMenuOS.h"  // Include the OpenMenuOS library


// Create an instance of the OpenMenuOS class with button and display pins, along with menu item names
// OpenMenuOS menu(10, NULL, 5, 12);  //btn_up, btn_down, btn_sel, tft_bl    If you don't have all of the 3 buttons, just put "NULL" instead of the pin of this button
OpenMenuOS menu(19, NULL, 2, 36);  //btn_up, btn_down, btn_sel, tft_bl

void setup() {
  Serial.begin(921600);  // Initialize serial communication
                         // menu.setTextScroll(false); // Disable text scrolling
                         // menu.setButtonAnimation(false); // Disable button animation

  // You can use these function to set the style OR you can use the preset function

  // menu.setScrollbar(false);        // Disable scrollbar
  menu.setMenuStyle(0);                  // Current style available : 0 (Default), 1 (Modern)
  menu.setSelectionBorderColor(0xffff);  // Setting the selection rectangle's border color
  menu.setSelectionFillColor(0x0000);    // Setting the selection rectangle's interior color
  // menu.setScrollbarStyle(1);  // Current style available : 0 (Default), 1 (Modern)
  // OR
  // menu.useStylePreset("Rabbit_R1");  // Current preset available : "Default" and "Rabbit_R1"
  // menu.useStylePreset("Rabbit_R1");  // Current preset available : "Default" and "Rabbit_R1"
  menu.setScrollbar(true);    // Disable scrollbar
  menu.setScrollbarStyle(1);  // Current style available : 0 (Default), 1 (Modern)

  menu.setButtonsMode("High");  // High or Low (case doesn't matter)
  menu.begin(1);                // Initialize the menu library, parameter :  Display rotation
}

void loop() {
  menu.loop();  // MUST call the loop() function first

  // Display the menu if the current screen is the main menu (screen 0)
  if (menu.getCurrentScreen() == 0) {
    delay(100);                                                                     // Small delay for stability
    menu.drawMenu(true, "Tile Menu", "Submenu", "Settings", "Informations", NULL);  // Draw the main menu on the screen. Set to "true" to display the 16x16 images for the items, else, set to "false"
                                                                                    // canvas.fillSmoothRoundRect(1, 1, 50, 50, 5, 0xfa60, TFT_BLACK);

  }
  // If the current screen is a submenu (screen 1)
  else if (menu.getCurrentScreen() == 1) {
    // Handle actions based on the selected menu item
    switch (menu.getSelectedItem()) {
      case 0:  // Tile Menu
        menu.drawTileMenu(2, 2, TFT_GREEN);
        static bool clicked;
        static bool done = false;
        if (!clicked) {
          menu.drawPopup("Set backlight?", clicked, 2);  // 1 = Warning, 2 = Success, 3 = Info
        } else if (clicked && !done) {
          menu.redirectToMenu(0, 2);
          menu.redirectToMenu(1, 3);
          done = true;
        }
        break;
      case 1:  // Submenu
        if (menu.getCurrentScreen() == 1) {
          Serial.println("screen 1");

          delay(100);                                                                 // Small delay for stability
          menu.drawSubmenu(true, "Long name submenu item....", "2", "3", "4", NULL);  // Draw the submenu on the screen. Set to "true" to display the 16x16 images for the items, else, set to "false"
        } else {
          Serial.println(menu.getSelectedItemSubmenu() + "eee");
          // Handle actions based on the selected submenu item
          switch (menu.getSelectedItemSubmenu()) {
            case 0:
              // canvas.fillScreen(TFT_BLACK);
              canvas.fillScreen(TFT_BLACK);
              canvas.setTextColor(TFT_WHITE);
              canvas.setCursor(0, 10);
              canvas.print("OpenMenuOS\n");
              canvas.print("Version: 1.4.0\n");
              break;
            case 1:
              canvas.fillScreen(TFT_BLACK);
              canvas.setTextColor(TFT_WHITE);
              canvas.setCursor(0, 10);
              canvas.print("OpenMenuOS\n");
              canvas.print("Version: 1.4.0\n");
              break;
            case 2:
              canvas.fillScreen(TFT_BLACK);
              canvas.setTextColor(TFT_WHITE);
              canvas.setCursor(0, 10);
              canvas.print("OpenMenuOS\n");
              canvas.print("Version: 1.4.0\n");
              break;
            case 3:
              canvas.fillScreen(TFT_BLACK);
              canvas.setTextColor(TFT_WHITE);
              canvas.setCursor(0, 10);
              canvas.print("OpenMenuOS\n");
              canvas.print("Version: 1.4.0\n");
              break;
          }
        }
        break;
      case 2:                                                                                 // Settings
                                                                                              // Draw the setting menu on the screen
        menu.drawSettingMenu("Long name setting item.....", "Setting_3", "Setting_4", NULL);  // 1 items is by default in the settings (For the backlight). The one defined in this function are added after the one there by default
        break;
      case 3:  // Settings
               // Draw the setting menu on the screen
        canvas.fillScreen(TFT_BLACK);
        canvas.setTextColor(TFT_WHITE);
        canvas.setCursor(0, 10);
        canvas.print("OpenMenuOS\n");
        canvas.print("Version: 1.4.0\n");
        break;
    }
  }
  // Additional functionality based on menu settings
  if (menu.menu_items_settings_bool[3]) {
    // Execute code if a specific menu item setting is enabled
    Serial.println("Menu item bool 5 is true");
  }
  // screenServer(); // If enabled, will send "screenshots" of the display to the serial and the processing sketch will read the data

  menu.drawCanvasOnTFT();  // Draw the updated canvas on the screen. You need to call it at the END of your code (in the end of "loop()")
}