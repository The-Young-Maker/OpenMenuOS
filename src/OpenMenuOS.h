/*
  OpenMenuOS.h - Library to create menu on ST7735 display.
  Created by Loic Daigle aka The Young Maker.
  Released into the public domain.
*/

#ifndef OPENMENUOS_H
#define OPENMENUOS_H

#include "Arduino.h"
#include <TFT_eSPI.h>
#include "images.h"

#define MAX_MENU_ITEMS 20                        // Maximum number of menu items
#define MAX_SETTINGS_ITEMS 10                    // Maximum number of settings items
#define MAX_ITEM_LENGTH 100                      // Maximum length of each menu item
#define MAX_ITEM_LENGTH_NOT_SCROLLING 11         // Maximum length of each menu item when not scrolling
#define MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING 9  // Maximum length of each setting item when not scroling

extern TFT_eSPI tft;        // Declare tft as extern
extern TFT_eSprite canvas;  // Declare canvas as extern

class OpenMenuOS {
public:
  static bool menu_items_settings_bool[];

  OpenMenuOS(int btn_up, int btn_down, int btn_sel, int tft_bl);  // BTN_UP pin, BTN_DOWN pin, BTN_SEL pin, TFT Backlight pin

  void begin(int rotation);  // Display type
  void loop();

  // Draw the main menu
  void drawMenu(bool images, const char* names...);
  // Draw a submenu
  void drawSubmenu(bool images, const char* names...);
  // Draw the setting menu
  void drawSettingMenu(const char* items...);

  void drawToggleSwitch(int16_t x, int16_t y, bool state);

  // Draw a tile submenu
  void drawTileMenu(int rows, int columns, int tile_color);
  // Redirect to a menu
  void redirectToMenu(int screen, int item);
  // Draw a popup
  void drawPopup(char* message, bool& clicked, int type);
  // Draw the scrollbar
  void drawScrollbar(int selectedItem, int nextItem);
  // Scroll text horizontally
  void scrollTextHorizontal(int16_t x, int16_t y, const char* text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);
  // Enable or disable text scrolling
  void setTextScroll(bool x);
  // Enable or disable boot image
  void showBootImage(bool x);
  // Enable or disable button animation
  void setButtonAnimation(bool x);
  // Set the style of the menu
  void setMenuStyle(int style);
  // Enable or disable scrollbar
  void setScrollbar(bool x);

  void setScrollbarColor(uint16_t color);
  void setScrollbarStyle(int style);
  void setSelectionBorderColor(uint16_t color);
  void setSelectionFillColor(uint16_t color);
  void useStylePreset(char* preset);
  void setButtonsMode(char* mode);

  void printMenuToSerial();
  void checkForButtonPress();
  void checkForButtonPressSubmenu();
  void drawCanvasOnTFT();
  void saveToEEPROM();
  void readFromEEPROM();

  int getCurrentScreen() const;          // Getter method for current_screen
  int getCurrentScreenTileMenu() const;  // Getter method for current_screen
  int getSelectedItem() const;           // Getter method for item_selected
  int getSelectedItemSubmenu() const;    // Getter method for item_selected_submenu
  int getSelectedItemTileMenu() const;   // Getter method for item_selected_tile_menu
  int getTftHeight() const;              // Getter method for tftHeight
  int getTftWidth() const;               // Getter method for tftWidth
  int UpButton() const;                  // Getter method for Up Button
  int DownButton() const;                // Getter method for Down Button
  int SelectButton() const;              // Getter method for Select Button
private:
  char menu_items[MAX_MENU_ITEMS][MAX_ITEM_LENGTH];
  char submenu_items[MAX_MENU_ITEMS][MAX_ITEM_LENGTH];
  int NUM_MENU_ITEMS;
  int NUM_SUBMENU_ITEMS;
  int current_screen;
};

#endif