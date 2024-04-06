/*
  OpenMenuOS.h - Library to create menu on ST7735 display.
  Created by Loic Daigle aka The Young Maker.
  Released into the public domain.
*/

#ifndef OPENMENUOS_H
#define OPENMENUOS_H

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "images.h"

#define MAX_MENU_ITEMS 20                        // Maximum number of menu items
#define MAX_SETTINGS_ITEMS 10                    // Maximum number of settings items
#define MAX_ITEM_LENGTH 100                      // Maximum length of each menu item
#define MAX_ITEM_LENGTH_NOT_SCROLLING 11         // Maximum length of each menu item when not scrolling
#define MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING 9  // Maximum length of each setting item when not scroling


class OpenMenuOS {
public:
  static bool menu_items_settings_bool[];
  GFXcanvas16 canvas;
  Adafruit_ST7735 tft;

  OpenMenuOS(int btn_up, int btn_down, int btn_sel, int tft_bl, int cs, int dc, int rst);  // BTN_UP pin, BTN_DOWN pin, BTN_SEL pin, CS pin, DC pin, RST pin, Menu Items

  void begin(uint8_t display);  // Display type
  void loop();

  void drawMenu(bool images, const char* names...);
  void drawSubmenu(bool images, const char* names...);
  void drawSettingMenu(const char* items...);
  void drawTileMenu(int rows, int columns, int tile_color);
  void redirectToMenu(int screen, int item);
  void drawPopup(char* message, bool& clicked);
  void drawScrollbar(int selectedItem, int nextItem);
  void scrollTextHorizontal(int16_t x, int16_t y, const char* text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);
  void eraseText(const char* text, int16_t xPos, int16_t x, int16_t y, uint16_t bgColor, uint16_t windowSize);
  void displayText(const char* text, int16_t xPos, int16_t x, int16_t y, uint16_t textColor, uint16_t windowSize);

  void setTextScroll(bool x);
  void setButtonAnimation(bool x);
  void setMenuStyle(int style);
  void setScrollbar(bool x);
  void setSelectionColor(uint16_t color);
  void useStylePreset(char* preset);

  void printMenuToSerial();
  void checkForButtonPress();
  void checkForButtonPressSubmenu();
  void drawCanvasOnTFT();
  void saveToEEPROM();
  void readFromEEPROM();

  int getCurrentScreen() const;          // Getter method for current_screen
  int getCurrentScreenTileMenu() const;  // Getter method for current_screen
  int getSelectedItem() const;           // Getter method for item_selected
  int getSelectedItemTileMenu() const;   // Getter method for item_selected
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