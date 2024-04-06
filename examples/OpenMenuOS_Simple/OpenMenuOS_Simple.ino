// Fixed debouncing issue when returning to back menu
// Added menu button animation when pressed
// Added a function to redirect to menu
// Added option to choose between different menu style
// Added option to enable to disable scrollbar
// Added option to choose the color of the selection rectangle
// Added option to choose from preset menu style

// TODO : Clean and reorganize drawMenu() and drawSubmenu() function

#include <OpenMenuOS.h>               // Include the OpenMenuOS library
#include <Fonts/FreeMonoBold9pt7b.h>  // Include the required fonts
#include <Fonts/FreeMono9pt7b.h>

// Create an instance of the OpenMenuOS class with button and display pins, along with menu item names
OpenMenuOS menu(10, 11, 5, 12, 4, 21, 22);  //btn_up, btn_down, btn_sel, tft_bl, cs, dc, rst    If you don't have all of the 3 buttons, just put "NULL" instead of the pin of this button

void setup() {
  Serial.begin(921600);  // Initialize serial communication
  // menu.setTextScroll(false); // Disable text scrolling
  // menu.setButtonAnimation(false); // Disable button animation

  // You can use these function to set the style OR you can use the preset function

  // menu.setScrollbar(false);        // Disable scrollbar
  // menu.setMenuStyle(1);            // Current style available : 0 (Default), 1 (Modern)
  // menu.setSelectionColor(0xfa60);  // Setting the selection rectangle's color

  // OR
  menu.useStylePreset("Rabbit_R1");  // Current preset available : "Default" and "Rabbit_R1"

  menu.begin(INITR_GREENTAB);  // Initialize the menu library, parameter :  Display type
}

void loop() {
  menu.loop();  // Handle menu logic

  // Display the menu if the current screen is the main menu (screen 0)
  if (menu.getCurrentScreen() == 0) {
    delay(100);                                                                                                             // Small delay for stability
    menu.drawMenu(true, "Tile Menu", "Submenu", "Settings", "This item's name is so long that it will be scrolled", NULL);  // Draw the main menu on the screen. Set to "true" to display the 16x16 images for the items, else, set to "false"
  }
  // If the current screen is a submenu (screen 1)
  else if (menu.getCurrentScreen() == 1) {
    // Handle actions based on the selected menu item
    switch (menu.getSelectedItem()) {
      case 0:  // Tile Menu
        menu.drawTileMenu(2, 2, ST7735_GREEN);
        bool clicked;
        menu.drawPopup("Set backlight?", clicked);
        if (clicked) {
          menu.redirectToMenu(0, 2);
          menu.redirectToMenu(1, 3);
        }
        break;
      case 1:  // Submenu
        if (menu.getCurrentScreen() == 1) {
          delay(100);                                                                 // Small delay for stability
          menu.drawSubmenu(true, "Long name submenu item....", "2", "3", "4", NULL);  // Draw the submenu on the screen. Set to "true" to display the 16x16 images for the items, else, set to "false"
        } else if (menu.getCurrentScreen() == 2) {
          // Handle actions based on the selected submenu item
          switch (menu.getSelectedItem()) {
            case 0:
              menu.canvas.fillScreen(ST7735_BLACK);
              break;
            case 1:
              menu.canvas.fillScreen(ST7735_BLACK);
              break;
            case 2:
              menu.canvas.fillScreen(ST7735_BLACK);
              break;
            case 3:
              menu.canvas.fillScreen(ST7735_BLACK);
              break;
          }
        }
        break;
      case 2:                                                                                 // Settings
                                                                                              // Draw the setting menu on the screen
        menu.drawSettingMenu("Long name setting item.....", "Setting_3", "Setting_4", NULL);  // 1 items is by default in the settings (For the backlight). The one defined in this function are added after the one there by default
        break;
    }
  }
  // Additional functionality based on menu settings
  if (menu.menu_items_settings_bool[3]) {
    // Execute code if a specific menu item setting is enabled
    Serial.println("Menu item bool 5 is true");
  }
  menu.drawCanvasOnTFT();  // Draw the updated canvas on the screen. You need to call it at the END of your code (in the end of "loop()")
}