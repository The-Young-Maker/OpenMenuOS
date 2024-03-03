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
#include <Preferences.h>
#include "images.h"

#define MAX_MENU_ITEMS 20      // Maximum number of menu items
#define MAX_SETTINGS_ITEMS 10  // Maximum number of settings items
#define MAX_ITEM_LENGTH 20     // Maximum length of each menu item

class OpenMenuOS {
public:
  static bool menu_items_settings_bool[];
  GFXcanvas16 canvas;

  OpenMenuOS(int btn_up, int btn_sel, int tft_bl, int cs, int dc, int rst);  // BTN_UP pin, BTN_SEL pin, CS pin, DC pin, RST pin, Menu Items

  void begin();
  void loop();

  void checkSerial();
  void connectToWiFi();
  void connectToStrongestOpenWiFi();
  void drawMenu(bool images, const char* names...);
  void drawSubmenu(bool images, const char* names...);
  void drawSettingMenu(const char* items...);
  void drawTileMenu(int rows, int columns, int tile_color);
  void printMenuToSerial();
  void checkForButtonPress();
  void checkForButtonPressSubmenu();
  void drawCanvasOnTFT();
  void saveToEEPROM();
  void readFromEEPROM();

  int getCurrentScreen() const;  // Getter method for current_screen
  int getCurrentScreenTileMenu() const;  // Getter method for current_screen
  int getSelectedItem() const;   // Getter method for item_selected
  int getSelectedItemTileMenu() const;   // Getter method for item_selected
private:
  Adafruit_ST7735 tft;
  Preferences prefs;

  char menu_items[MAX_MENU_ITEMS][MAX_ITEM_LENGTH];
  char submenu_items[MAX_MENU_ITEMS][MAX_ITEM_LENGTH];
  int NUM_MENU_ITEMS;
  int NUM_SUBMENU_ITEMS;
  int current_screen;
};

#endif