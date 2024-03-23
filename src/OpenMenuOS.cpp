/*
  OpenMenuOS.h - Library to create menu on ST7735 display.
  Created by Loic Daigle aka The Young Maker.
  Released into the public domain.
*/

#pragma message "The OpenMenuOS library is still in Beta. If you find any bug, please create an issue on the OpenMenuOS's Github repository"

#include "Arduino.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <EEPROM.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include "OpenMenuOS.h"

////////////////// Variables for text scrolling //////////////////
int CHAR_WIDTH;
int CHAR_SPACING;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
uint16_t w, h;
////////////////// Variables for text scrolling //////////////////

// Display Constants
int tftWidth = 160;
int tftHeight = 80;

// Tile menu variable
int current_screen_tile_menu = 0;
int item_selected_tile_menu = 2;
int tile_menu_selection_X = 0;
int tile_menu_selection_Y = 0;

// Rectangle dimensions
const uint16_t rect_width = 155;  // Width of the selection rectangle
const uint16_t rect_height = 26;  // Height of the selection rectangle

// Menu Item Positioning Variables
int item_sel_previous;  // Previous item - used in the menu screen to draw the item before the selected one
int item_selected;      // Current item -  used in the menu screen to draw the selected item
int item_sel_next;      // Next item - used in the menu screen to draw next item after the selected one

// Submenu Item Positioning Variables
int item_sel_previous_submenu;  // Previous item - used in the submenu screen to draw the item before the selected one
int item_selected_submenu;      // Current item -  used in the submenu screen to draw the selected item
int item_sel_next_submenu;      // Next item - used in the submenu screen to draw next item after the selected one

// Settings Item Positioning Variables
int item_selected_settings_previous;  // Previous item - used in the menu screen to draw the item before the selected one
int item_selected_settings = 0;       // Current item -  used in the menu screen to draw the selected item
int item_selected_settings_next;      // Next item - used in the menu screen to draw next item after the selected one

// Button Press Timing and Control
unsigned long select_button_press_time = 0;
int button_up_clicked = 0;  // Only perform action when the button is clicked, and wait until another press
int button_up_clicked_tile = 0;
int button_up_clicked_settings = 0;  // Only perform action when the button is clicked, and wait until another press
int button_down_clicked = 0;         // Only perform action when the button is clicked, and wait until another press
int button_down_clicked_tile = 0;
int button_down_clicked_settings = 0;  // Only perform action when the button is clicked, and wait until another press
int button_select_clicked = 0;
bool previousButtonState = LOW;
bool buttonPressProcessed = false;

bool textScroll = true;

// Button Pin Definitions
int BUTTON_UP_PIN;      // Pin for UP button
int BUTTON_DOWN_PIN;    // Pin for DOWN button
int BUTTON_SELECT_PIN;  // Pin for SELECT button
int TFT_BL_PIN;         // Pin for TFT Backlight

// Button Constants
#define SELECT_BUTTON_LONG_PRESS_DURATION 300

#define EEPROM_SIZE (MAX_SETTINGS_ITEMS * sizeof(bool) + (32 + 1) + (64 + 1) + (32 + 1) + (64 + 1))  // Possibly change the "MAX_SETTINGS_ITEMS" to the value of NUM_SETTINGS_ITEMS if setting array made before eeprom ; to also do in readFromEEPROM and saveToEEPROM
int NUM_SETTINGS_ITEMS = 1;

char menu_items_settings[MAX_SETTINGS_ITEMS][MAX_ITEM_LENGTH] = {
  // array with item names
  { "Backlight" },
};

bool OpenMenuOS::menu_items_settings_bool[MAX_SETTINGS_ITEMS] = {
  true,   // Wifi
  false,  // Open Wifi
  true,   // Bluetooth
  false,  // Backlight
  false,  // OTA Updates
  false,
  false,
  false,
  false,
  false,
};

OpenMenuOS::OpenMenuOS(int btn_up, int btn_down, int btn_sel, int tft_bl, int cs, int dc, int rst)
  : tft(cs, dc, rst), canvas(160, 80) {
  BUTTON_UP_PIN = btn_up;
  BUTTON_DOWN_PIN = btn_down;
  BUTTON_SELECT_PIN = btn_sel;
  TFT_BL_PIN = tft_bl;
}

void OpenMenuOS::begin(uint8_t display) {  // Display type
  // Set up display
  tft.initR(display);
  tft.setRotation(3);
  canvas.fillScreen(ST7735_BLACK);
  tft.setTextWrap(false);

  // Show The Boot image
  tft.drawRGBBitmap(0, 0, (uint16_t*)Boot_img, 160, 80);

  item_selected = 0;
  current_screen = 0;  // 0 = Menu, 1 = Submenu

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  readFromEEPROM();

  // Set TFT_BL_PIN as OUTPUT and se it HIGH or LOW depending on the settings
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, menu_items_settings_bool[4] ? LOW : HIGH);

  // Set up button pins
  pinMode(BUTTON_UP_PIN, INPUT);
  pinMode(BUTTON_DOWN_PIN, INPUT);
  pinMode(BUTTON_SELECT_PIN, INPUT);
}
void OpenMenuOS::loop() {
  checkForButtonPress();
  checkForButtonPressSubmenu();
}
void OpenMenuOS::drawMenu(bool images, const char* names...) {
  NUM_MENU_ITEMS = 0;
  va_list args;
  va_start(args, names);
  const char* name = names;
  // Clear the menu_items array before putting in new values
  memset(menu_items, 0, sizeof(menu_items));
  while (name != NULL) {
    strncpy(menu_items[NUM_MENU_ITEMS], name, MAX_ITEM_LENGTH);
    name = va_arg(args, const char*);
    NUM_MENU_ITEMS++;
  }
  va_end(args);

  checkForButtonPress();  // Check for button presses to control the menu

  // Calculate the position of the rectangle
  uint16_t rect_x = (tftWidth - rect_width - 4) / 2;                               // Center the rectangle horizontally with a 4-pixel space on the right
  uint16_t rect_y = (tftHeight - rect_height) / 2;                                 // Center the rectangle vertically
  canvas.drawFastVLine(153, 29, 22, ST7735_WHITE);                                 // Display the Shadow
  canvas.drawFastHLine(3, 51, 150, ST7735_WHITE);                                  // Display the Shadow
  canvas.drawRoundRect(rect_x, rect_y, rect_width, rect_height, 4, ST7735_WHITE);  // Display the rectangle

  int xPos = 30;   // X Position of text 0
  int yPos = 17;   // Y Position of text 0
  int x1Pos = 30;  // X Position of text 1
  int y1Pos = 44;  // Y Position of text 1
  int x2Pos = 30;  // X Position of text 2
  int y2Pos = 70;  // Y Position of text 2

  canvas.fillRoundRect(rect_x + 1, rect_y - 25, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);  // Remove Old Text
  canvas.fillRoundRect(rect_x + 1, rect_y + 27, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 1, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);

  // draw previous item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(xPos, yPos);

  // Get the text of the previous item
  String previousItem = menu_items[item_sel_previous];

  // Check if the length of the text exceeds the maximum length for non-scrolling items
  if (previousItem.length() > MAX_ITEM_LENGTH_NOT_SCROLLING) {
    // Truncate the text and add "..." at the end
    previousItem = previousItem.substring(0, MAX_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
  }

  // Print the modified text
  canvas.println(previousItem);

  if (images) {
    canvas.drawRGBBitmap(5, 5, (uint16_t*)bitmap_icons[item_sel_previous], 16, 16);
  }
  // draw selected item as icon + label in bold font
  if (strlen(menu_items[item_selected]) > MAX_ITEM_LENGTH_NOT_SCROLLING && textScroll) {
    tft.setFont(&FreeMonoBold9pt7b);  // Here, We don't use "canvas." because when using it, the calculated value in the "scrollTextHorizontal" function is wrong but not with "tft." #bug
    canvas.setFont(&FreeMonoBold9pt7b);
    scrollTextHorizontal(x1Pos, y1Pos, menu_items[item_selected], ST7735_WHITE, ST7735_BLACK, 1, 50, 120);  // Adjust windowSize as needed
  } else if (!textScroll) {
    // draw selected item as icon + label
    canvas.setFont(&FreeMono9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(ST7735_WHITE);
    canvas.setCursor(x1Pos, y1Pos);

    // Get the text of the selected item
    String selectedItem = menu_items[item_selected];

    // Check if the length of the text exceeds the maximum length for non-scrolling items
    if (selectedItem.length() > MAX_ITEM_LENGTH_NOT_SCROLLING) {
      // Truncate the text and add "..." at the end
      selectedItem = selectedItem.substring(0, MAX_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
    }

    // Print the modified text
    canvas.println(selectedItem);
  } else {
    canvas.setFont(&FreeMonoBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(ST7735_WHITE);
    canvas.setCursor(x1Pos, y1Pos);
    canvas.println(menu_items[item_selected]);
  }

  if (images) {
    canvas.drawRGBBitmap(5, 32, (uint16_t*)bitmap_icons[item_selected], 16, 16);
  }
  // draw next item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(x2Pos, y2Pos);

  // Get the text of the next item
  String nextItem = menu_items[item_sel_next];

  // Check if the length of the text exceeds the maximum length for non-scrolling items
  if (nextItem.length() > MAX_ITEM_LENGTH_NOT_SCROLLING) {
    // Truncate the text and add "..." at the end
    nextItem = nextItem.substring(0, MAX_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
  }

  // Print the modified text
  canvas.println(nextItem);

  if (images) {
    canvas.drawRGBBitmap(5, 59, (uint16_t*)bitmap_icons[item_sel_next], 16, 16);
  }

  for (int y = 0; y < tftHeight; y++) {  // Display the Scrollbar
    if (y % 2 == 0) {
      canvas.drawPixel(tftWidth - 2, y, ST7735_WHITE);
    }
  }
  // Draw scrollbar handle
  int boxHeight = tftHeight / (NUM_MENU_ITEMS);
  int boxY = boxHeight * item_selected;
  // Clear previous scrollbar handle
  canvas.fillRect(tftWidth - 3, boxHeight * item_sel_next, 3, boxHeight, ST7735_BLACK);
  // Draw new scrollbar handle
  canvas.fillRect(tftWidth - 3, boxY, 3, boxHeight, ST7735_WHITE);

  for (int y = 0; y < tftHeight; y++) {  // Display the Scrollbar
    if (y % 2 == 0) {
      canvas.drawPixel(tftWidth - 2, y, ST7735_WHITE);
    }
  }
}
void OpenMenuOS::drawSubmenu(bool images, const char* names...) {
  NUM_SUBMENU_ITEMS = 0;
  va_list args;
  va_start(args, names);
  const char* name = names;
  // Clear the submenu_items array before putting in new values
  memset(submenu_items, 0, sizeof(submenu_items));
  while (name != NULL) {
    strncpy(submenu_items[NUM_SUBMENU_ITEMS], name, MAX_ITEM_LENGTH);
    name = va_arg(args, const char*);
    NUM_SUBMENU_ITEMS++;
  }
  va_end(args);

  checkForButtonPressSubmenu();  // Check for button presses to control the submenu

  // Calculate the position of the rectangle
  uint16_t rect_x = (tftWidth - rect_width - 4) / 2;                               // Center the rectangle horizontally with a 4-pixel space on the right
  uint16_t rect_y = (tftHeight - rect_height) / 2;                                 // Center the rectangle vertically
  canvas.drawFastVLine(153, 29, 22, ST7735_WHITE);                                 // Display the Shadow
  canvas.drawFastHLine(3, 51, 150, ST7735_WHITE);                                  // Display the Shadow
  canvas.drawRoundRect(rect_x, rect_y, rect_width, rect_height, 4, ST7735_WHITE);  // Display the rectangle

  int xPos = 30;   // X Position of text 0
  int yPos = 17;   // Y Position of text 0
  int x1Pos = 30;  // X Position of text 1
  int y1Pos = 44;  // Y Position of text 1
  int x2Pos = 30;  // X Position of text 2
  int y2Pos = 70;  // Y Position of text 2

  canvas.fillRoundRect(rect_x + 1, rect_y - 25, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);  // Remove Old Text
  canvas.fillRoundRect(rect_x + 1, rect_y + 27, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 1, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);


  // draw previous item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(xPos, yPos);

  // Get the text of the previous item
  String previousItem = submenu_items[item_sel_previous_submenu];

  // Check if the length of the text exceeds the maximum length for non-scrolling items
  if (previousItem.length() > MAX_ITEM_LENGTH_NOT_SCROLLING) {
    // Truncate the text and add "..." at the end
    previousItem = previousItem.substring(0, MAX_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
  }

  // Print the modified text
  canvas.println(previousItem);
  if (images) {
    canvas.drawRGBBitmap(5, 5, (uint16_t*)bitmap_icons[item_sel_previous_submenu], 16, 16);
  }

  // draw selected item as icon + label in bold font
  if (strlen(submenu_items[item_selected_submenu]) > MAX_ITEM_LENGTH_NOT_SCROLLING && textScroll) {
    tft.setFont(&FreeMonoBold9pt7b);  // Here, We don't use "canvas." because when using it, the calculated value in the "scrollTextHorizontal" function is wrong but not with "tft." #bug
    canvas.setFont(&FreeMonoBold9pt7b);
    scrollTextHorizontal(x1Pos, y1Pos, submenu_items[item_selected_submenu], ST7735_WHITE, ST7735_BLACK, 1, 50, 120);  // Adjust windowSize as needed
  } else if (!textScroll) {
    // draw selected item as icon + label
    canvas.setFont(&FreeMono9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(ST7735_WHITE);
    canvas.setCursor(x1Pos, y1Pos);

    // Get the text of the selected item
    String selectedItem = submenu_items[item_selected_submenu];

    // Check if the length of the text exceeds the maximum length for non-scrolling items
    if (selectedItem.length() > MAX_ITEM_LENGTH_NOT_SCROLLING) {
      // Truncate the text and add "..." at the end
      selectedItem = selectedItem.substring(0, MAX_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
    }

    // Print the modified text
    canvas.println(selectedItem);
  } else {
    canvas.setFont(&FreeMonoBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(ST7735_WHITE);
    canvas.setCursor(x1Pos, y1Pos);
    canvas.println(submenu_items[item_selected_submenu]);
  }
  if (images) {
    canvas.drawRGBBitmap(5, 32, (uint16_t*)bitmap_icons[item_selected_submenu], 16, 16);
  }
  // draw next item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(x2Pos, y2Pos);

  // Get the text of the next item
  String nextItem = submenu_items[item_sel_next_submenu];

  // Check if the length of the text exceeds the maximum length for non-scrolling items
  if (nextItem.length() > MAX_ITEM_LENGTH_NOT_SCROLLING) {
    // Truncate the text and add "..." at the end
    nextItem = nextItem.substring(0, MAX_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
  }

  // Print the modified text
  canvas.println(nextItem);

  if (images) {
    canvas.drawRGBBitmap(5, 59, (uint16_t*)bitmap_icons[item_sel_next_submenu], 16, 16);
  }

  for (int y = 0; y < tftHeight; y++) {  // Display the Scrollbar
    if (y % 2 == 0) {
      canvas.drawPixel(tftWidth - 2, y, ST7735_WHITE);
    }
  }
  // Draw scrollbar handle
  int boxHeight = tftHeight / (NUM_SUBMENU_ITEMS);
  int boxY = boxHeight * item_selected_submenu;
  // Clear previous scrollbar handle
  canvas.fillRect(tftWidth - 3, boxHeight * item_sel_next_submenu, 3, boxHeight, ST7735_BLACK);
  // Draw new scrollbar handle
  canvas.fillRect(tftWidth - 3, boxY, 3, boxHeight, ST7735_WHITE);

  for (int y = 0; y < tftHeight; y++) {  // Display the Scrollbar
    if (y % 2 == 0) {
      canvas.drawPixel(tftWidth - 2, y, ST7735_WHITE);
    }
  }
}
void OpenMenuOS::drawSettingMenu(const char* items...) { // The scrolling text is flickering (only in settings) #bug
  NUM_SETTINGS_ITEMS = 1;
  va_list args;
  va_start(args, items);
  const char* item = items;
  while (item != NULL && NUM_SETTINGS_ITEMS < MAX_SETTINGS_ITEMS) {
    strncpy(menu_items_settings[NUM_SETTINGS_ITEMS++], item, MAX_ITEM_LENGTH - 1);
    item = va_arg(args, const char*);
  }
  va_end(args);

  if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked_settings == 0)) {  // up button clicked - jump to previous menu item
    item_selected_settings = item_selected_settings - 1;                            // select previous item
    button_up_clicked_settings = 1;                                                 // set button to clicked to only perform the action once
    if (item_selected_settings < 0) {                                               // if first item was selected, jump to last item
      item_selected_settings = NUM_SETTINGS_ITEMS - 1;
    }
  }

  if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked_settings == 1)) {  // unclick
    button_up_clicked_settings = 0;
  }

  uint16_t rect_x = (tftWidth - rect_width) / 2;                                   // Center the rectangle horizontally with a 4-pixel space on the right
  uint16_t rect_y = (tftHeight - rect_height) / 2;                                 // Center the rectangle vertically
  canvas.drawFastVLine(155, 29, 22, ST7735_WHITE);                                 // Display the Shadow
  canvas.drawFastHLine(3, 51, 150, ST7735_WHITE);                                  // Display the Shadow
  canvas.drawRoundRect(rect_x, rect_y, rect_width, rect_height, 4, ST7735_WHITE);  // Display the rectangle

  canvas.fillRoundRect(rect_x + 1, rect_y - 25, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 27, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 1, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);

  // draw previous item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(10, 17);

  // Get the text of the previous item
  String previousItem = menu_items_settings[item_selected_settings_previous];

  // Check if the length of the text exceeds the maximum length for non-scrolling items
  if (previousItem.length() > MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING) {
    // Truncate the text and add "..." at the end
    previousItem = previousItem.substring(0, MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
  }

  // Print the modified text
  canvas.println(previousItem);
  if (item_selected_settings_previous >= 0) {
    if (menu_items_settings_bool[item_selected_settings_previous] == false) {
      canvas.drawRoundRect(114, 3, 40, 20, 11, ST7735_RED);
      canvas.fillRoundRect(116, 5, 16, 16, 11, ST7735_RED);
    } else {
      canvas.drawRoundRect(114, 3, 40, 20, 11, ST7735_GREEN);
      canvas.fillRoundRect(136, 5, 16, 16, 11, ST7735_GREEN);
    }
  }

  // draw selected item as icon + label in bold font
  if (strlen(menu_items_settings[item_selected_settings]) > MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING && textScroll) {
    tft.setFont(&FreeMonoBold9pt7b);  // Here, We don't use "canvas." because when using it, the calculated value in the "scrollTextHorizontal" function is wrong but not with "tft." #bug
    canvas.setFont(&FreeMonoBold9pt7b);
    scrollTextHorizontal(10, 44, menu_items_settings[item_selected_settings], ST7735_WHITE, ST7735_BLACK, 1, 50, 100);  // Adjust windowSize as needed
  } else if (!textScroll) {
    // draw selected item as icon + label
    canvas.setFont(&FreeMono9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(ST7735_WHITE);
    canvas.setCursor(10, 44);

    // Get the text of the selected item
    String selectedItem = menu_items_settings[item_selected_settings];

    // Check if the length of the text exceeds the maximum length for non-scrolling items
    if (selectedItem.length() > MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING) {
      // Truncate the text and add "..." at the end
      selectedItem = selectedItem.substring(0, MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
    }

    // Print the modified text
    canvas.println(selectedItem);
  } else {
    canvas.setFont(&FreeMonoBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(ST7735_WHITE);
    canvas.setCursor(10, 44);
    canvas.println(menu_items_settings[item_selected_settings]);
  }

  if (item_selected_settings >= 0 && item_selected_settings < NUM_SETTINGS_ITEMS) {
    if (menu_items_settings_bool[item_selected_settings] == false) {
      canvas.drawRoundRect(114, 30, 40, 20, 11, ST7735_RED);
      canvas.fillRoundRect(116, 32, 16, 16, 11, ST7735_RED);
    } else {
      canvas.drawRoundRect(114, 30, 40, 20, 11, ST7735_GREEN);
      canvas.fillRoundRect(136, 32, 16, 16, 11, ST7735_GREEN);
    }
  }

  // draw next item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(10, 70);

  // Get the text of the next item
  String nextItem = menu_items_settings[item_selected_settings_next];

  // Check if the length of the text exceeds the maximum length for non-scrolling items
  if (nextItem.length() > MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING) {
    // Truncate the text and add "..." at the end
    nextItem = nextItem.substring(0, MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
  }

  // Print the modified text
  canvas.println(nextItem);

  if ((item_selected_settings_next) < NUM_SETTINGS_ITEMS) {
    if (menu_items_settings_bool[item_selected_settings_next] == false) {
      canvas.drawRoundRect(114, 56, 40, 20, 11, ST7735_RED);
      canvas.fillRoundRect(116, 58, 16, 16, 11, ST7735_RED);
    } else {
      canvas.drawRoundRect(114, 56, 40, 20, 11, ST7735_GREEN);
      canvas.fillRoundRect(136, 58, 16, 16, 11, ST7735_GREEN);
    }
  }

  if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
    if (previousButtonState == LOW && !buttonPressProcessed) {
      if (item_selected_settings == 0) {
        if (menu_items_settings_bool[0] == true) {
          menu_items_settings_bool[0] = false;
          digitalWrite(TFT_BL_PIN, HIGH);
        } else {
          menu_items_settings_bool[0] = true;
          digitalWrite(TFT_BL_PIN, LOW);
        }
      } else if (item_selected_settings == 1) {
        if (menu_items_settings_bool[1] == true) {
          menu_items_settings_bool[1] = false;
        } else {
          menu_items_settings_bool[1] = true;
        }
      } else if (item_selected_settings == 2) {
        if (menu_items_settings_bool[2] == true) {
          menu_items_settings_bool[2] = false;
        } else {
          menu_items_settings_bool[2] = true;
        }
      } else if (item_selected_settings == 3) {
        if (menu_items_settings_bool[3] == true) {
          menu_items_settings_bool[3] = false;
        } else {
          menu_items_settings_bool[3] = true;
        }
      } else if (item_selected_settings == 4) {
        if (menu_items_settings_bool[4] == true) {
          menu_items_settings_bool[4] = false;
        } else {
          menu_items_settings_bool[4] = true;
        }
      } else if (item_selected_settings == 5) {
        if (menu_items_settings_bool[5] == true) {
          menu_items_settings_bool[5] = false;
        } else {
          menu_items_settings_bool[5] = true;
        }
      } else if (item_selected_settings == 6) {
        if (menu_items_settings_bool[6] == true) {
          menu_items_settings_bool[6] = false;
        } else {
          menu_items_settings_bool[6] = true;
        }
      } else if (item_selected_settings == 7) {
        if (menu_items_settings_bool[7] == true) {
          menu_items_settings_bool[7] = false;
        } else {
          menu_items_settings_bool[7] = true;
        }
      } else if (item_selected_settings == 8) {
        if (menu_items_settings_bool[8] == true) {
          menu_items_settings_bool[8] = false;
        } else {
          menu_items_settings_bool[8] = true;
        }
      } else if (item_selected_settings == 9) {
        if (menu_items_settings_bool[9] == true) {
          menu_items_settings_bool[9] = false;
        } else {
          menu_items_settings_bool[9] = true;
        }
      }
      buttonPressProcessed = true;
      saveToEEPROM();
    }
    previousButtonState = HIGH;
  } else {
    previousButtonState = LOW;
    buttonPressProcessed = false;  // Reinitialiser la variable de controle lorsque le bouton est relache
  }
}
void OpenMenuOS::drawTileMenu(int rows, int columns, int tile_color) {
  int TILE_ROUND_RADIUS = 5;
  int TILE_MARGIN = 2;
  int tileWidth = (tftWidth - (columns + 1) * TILE_MARGIN) / columns;
  int tileHeight = (tftHeight - (rows + 1) * TILE_MARGIN) / rows;

  if (current_screen_tile_menu == 0) {
    if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
      if (current_screen_tile_menu == 0) {
        current_screen_tile_menu = 1;
      }
      button_select_clicked = 1;
    }
    if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked_tile == 0)) {  // up button clicked - jump to previous menu item
      item_selected_tile_menu = item_selected_tile_menu + 1;                      // select previous item
      button_up_clicked_tile = 1;                                                 // set button to clicked to only perform the action once
      if (item_selected_tile_menu >= rows * columns) {                            // if first item was selected, jump to last item
        item_selected_tile_menu = 0;
      }
    }

    if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked_tile == 1)) {  // unclick
      button_up_clicked_tile = 0;
    }

    if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 1)) {  // unclick
      button_select_clicked = 0;
    }

    int row = item_selected_tile_menu / columns;
    int col = item_selected_tile_menu % columns;
    tile_menu_selection_X = col * (tileWidth + TILE_MARGIN) + TILE_MARGIN;
    tile_menu_selection_Y = row * (tileHeight + TILE_MARGIN) + TILE_MARGIN;

    canvas.fillScreen(ST7735_BLACK);
    canvas.setTextColor(ST7735_WHITE);
    canvas.setTextSize(1);

    for (int i = 0; i < rows * columns; ++i) {
      int row = i / columns;
      int col = i % columns;
      int tileX = col * (tileWidth + TILE_MARGIN) + TILE_MARGIN;
      int tileY = row * (tileHeight + TILE_MARGIN) + TILE_MARGIN;
      canvas.fillRoundRect(tileX, tileY, tileWidth, tileHeight, TILE_ROUND_RADIUS, tile_color);
    }
    canvas.drawRoundRect(tile_menu_selection_X, tile_menu_selection_Y, tileWidth, tileHeight, TILE_ROUND_RADIUS, ST7735_WHITE);
  } else if (current_screen_tile_menu == 1) {
    delay(200);
    if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
      if (current_screen_tile_menu == 1) {
        current_screen_tile_menu = 0;
        delay(200);
      }
    }
    canvas.fillScreen(ST7735_BLACK);
    canvas.setFont(nullptr);
  }
}
// Function to scroll text horizontally on the display within a specified window size
void OpenMenuOS::scrollTextHorizontal(int16_t x, int16_t y, const char* text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize) {
  static int16_t xPos1 = x + (windowSize / 3);
  static unsigned long previousMillis1 = 0;

  // Set text size and color
  canvas.setTextSize(textSize);
  canvas.setTextColor(textColor);

  // Get text width and height
  int16_t x1, y1;
  tft.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);  // Here, We don't use "canvas.getTextBounds" because when using it, the calculated value is wrong but not with "tft." #bug
  CHAR_WIDTH = w / strlen(text);                    // Width of each character
  CHAR_SPACING = ceil(CHAR_WIDTH / 7);  // Spacing between characters

  // Calculate the next position for text 1
  unsigned long currentMillis1 = millis();
  if (currentMillis1 - previousMillis1 >= delayTime) {
    previousMillis1 = currentMillis1;
    xPos1--;

    // Check if text 1 goes out of bounds
    if (xPos1 <= x - w) {
      xPos1 = x + windowSize;
    }

    // Erase old text 1
    eraseText(text, xPos1 + CHAR_WIDTH + CHAR_SPACING, x, y, bgColor, windowSize);
    // Display text 1 at current position
    displayText(text, xPos1, x, y, textColor, windowSize);
  }
}
void OpenMenuOS::eraseText(const char* text, int16_t xPos, int16_t x, int16_t y, uint16_t bgColor, uint16_t windowSize) {
  for (uint16_t i = 0; i < strlen(text); i++) {
    if (xPos + i * (CHAR_WIDTH + CHAR_SPACING) >= x && xPos + i * (CHAR_WIDTH + CHAR_SPACING) <= x + windowSize - 10) {  // "- 10" is used to adjust where the text appears
      canvas.setTextColor(bgColor);
      canvas.setCursor(xPos + i * (CHAR_WIDTH + CHAR_SPACING), y);
      canvas.write(text[i]);
    }
  }
}
void OpenMenuOS::displayText(const char* text, int16_t xPos, int16_t x, int16_t y, uint16_t textColor, uint16_t windowSize) {
  for (uint16_t i = 0; i < strlen(text); i++) {
    if (xPos + i * (CHAR_WIDTH + CHAR_SPACING) >= x && xPos + i * (CHAR_WIDTH + CHAR_SPACING) <= x + windowSize - 10) {  // "- 10" is used to adjust where the text appears
      canvas.setTextColor(textColor);
      canvas.setCursor(xPos + i * (CHAR_WIDTH + CHAR_SPACING), y);
      canvas.write(text[i]);
    }
  }
}
void OpenMenuOS::setTextScroll(bool x) {
  textScroll = x;
}

void OpenMenuOS::printMenuToSerial() {
  Serial.println("Menu Items:");
  for (int i = 0; i < NUM_MENU_ITEMS; i++) {
    Serial.print("Item ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(menu_items[i]);
  }
}
void OpenMenuOS::checkForButtonPress() {
  if (current_screen == 0) {                                                 // MENU SCREEN // up and down buttons only work for the menu screen
    if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 0)) {  // up button clicked - jump to previous menu item
      item_selected = item_selected - 1;                                     // select previous item
      button_up_clicked = 1;                                                 // set button to clicked to only perform the action once
      if (item_selected < 0) {                                               // if first item was selected, jump to last item
        item_selected = NUM_MENU_ITEMS - 1;
      }
    }

    if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 1)) {  // unclick
      button_up_clicked = 0;
    }

    if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 0)) {  // down button clicked - jump to next menu item
      item_selected = item_selected + 1;                                         // select next item
      button_down_clicked = 1;                                                   // set button to clicked to only perform the action once
      if (item_selected >= NUM_MENU_ITEMS) {                                     // if last item was selected, jump to first item
        item_selected = 0;
      }
    }

    if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 1)) {  // unclick
      button_down_clicked = 0;
    }
  }
  if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
    if (button_select_clicked == 0) {
      select_button_press_time = millis();  // Start measuring button press time
      button_select_clicked = 1;
    } else if (current_screen == 1 && (millis() - select_button_press_time >= SELECT_BUTTON_LONG_PRESS_DURATION)) {
      current_screen = 0;
      delay(350);
    }
  } else if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    if (button_select_clicked == 1) {
      if (current_screen == 0) {
        current_screen = 1;
      }
      button_select_clicked = 0;  // Reset the button state
    }
  }
  if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 1)) {  // unclick
    button_select_clicked = 0;
  }

  // set correct values for the previous and next items
  item_sel_previous = item_selected - 1;
  if (item_sel_previous < 0) { item_sel_previous = NUM_MENU_ITEMS - 1; }  // previous item would be below first = make it the last
  item_sel_next = item_selected + 1;
  if (item_sel_next >= NUM_MENU_ITEMS) { item_sel_next = 0; }  // next item would be after last = make it the first
  item_selected_settings_previous = item_selected_settings - 1;
  if (item_selected_settings_previous < 0) { item_selected_settings_previous = NUM_SETTINGS_ITEMS - 1; }  // previous item would be below first = make it the last
  item_selected_settings_next = item_selected_settings + 1;
  if (item_selected_settings_next >= NUM_SETTINGS_ITEMS) { item_selected_settings_next = 0; }  // next item would be after last = make it the first
}
void OpenMenuOS::checkForButtonPressSubmenu() {
  if (current_screen == 1) {                                                 // MENU SCREEN // up and down buttons only work for the menu screen
    if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 0)) {  // up button clicked - jump to previous menu item
      item_selected_submenu = item_selected_submenu - 1;                     // select previous item
      button_up_clicked = 1;                                                 // set button to clicked to only perform the action once
      if (item_selected_submenu < 0) {                                       // if first item was selected, jump to last item
        item_selected_submenu = NUM_SUBMENU_ITEMS - 1;
      }
    }

    if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 1)) {  // unclick
      button_up_clicked = 0;
    }
  }

  if ((digitalRead(BUTTON_DOWN_PIN) == HIGH) && (button_down_clicked == 0)) {  // down button clicked - jump to next menu item
    item_selected_submenu = item_selected_submenu + 1;                         // select next item
    button_down_clicked = 1;                                                   // set button to clicked to only perform the action once
    if (item_selected_submenu >= NUM_SUBMENU_ITEMS) {                          // if last item was selected, jump to first item
      item_selected_submenu = 0;
    }
  }

  if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 1)) {  // unclick
    button_down_clicked = 0;
  }
  if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
    if (button_select_clicked == 0) {
      select_button_press_time = millis();  // Start measuring button press time
      button_select_clicked = 1;
    } else if (current_screen == 2 && (millis() - select_button_press_time >= SELECT_BUTTON_LONG_PRESS_DURATION)) {
      current_screen = 1;
      delay(350);
    }
  } else if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    if (button_select_clicked == 1) {
      if (current_screen == 1) {
        current_screen = 2;
      }
      button_select_clicked = 0;  // Reset the button state
    }
  }
  if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 1)) {  // unclick
    button_select_clicked = 0;
  }

  // set correct values for the previous and next items
  item_sel_previous_submenu = item_selected_submenu - 1;
  if (item_sel_previous_submenu < 0) { item_sel_previous_submenu = NUM_SUBMENU_ITEMS - 1; }  // previous item would be below first = make it the last
  item_sel_next_submenu = item_selected_submenu + 1;
  if (item_sel_next_submenu >= NUM_SUBMENU_ITEMS) { item_sel_next_submenu = 0; }  // next item would be after last = make it the first
}
void OpenMenuOS::drawCanvasOnTFT() {
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height());
  canvas.fillScreen(ST7735_BLACK);
}
void OpenMenuOS::saveToEEPROM() {
  // Save the contents of the array in EEPROM memory
  for (int i = 0; i < MAX_SETTINGS_ITEMS; i++) {
    EEPROM.write(i, menu_items_settings_bool[i]);
  }

  EEPROM.commit();  // Don't forget to call the commit() function to save the data to EEPROM
}
void OpenMenuOS::readFromEEPROM() {
  // Read the contents of the EEPROM memory and restore it to the array
  for (int i = 0; i < MAX_SETTINGS_ITEMS; i++) {
    menu_items_settings_bool[i] = EEPROM.read(i);
  }
}

int OpenMenuOS::getCurrentScreen() const {
  return current_screen;
}
int OpenMenuOS::getCurrentScreenTileMenu() const {
  return current_screen_tile_menu;
}
int OpenMenuOS::getSelectedItem() const {
  return item_selected;
}
int OpenMenuOS::getSelectedItemTileMenu() const {
  return item_selected_tile_menu;
}
int OpenMenuOS::getTftHeight() const {
  return tftHeight;
}
int OpenMenuOS::getTftWidth() const {
  return tftWidth;
}
int OpenMenuOS::UpButton() const {
  return BUTTON_UP_PIN;
}
int OpenMenuOS::DownButton() const {
  return BUTTON_DOWN_PIN;
}
int OpenMenuOS::SelectButton() const {
  return BUTTON_SELECT_PIN;
}