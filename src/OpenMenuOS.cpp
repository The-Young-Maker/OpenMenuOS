/*
  OpenMenuOS.h - Library to create menu on color display.
  Created by Loic Daigle aka The Young Maker.
  Released into the public domain.
*/

#pragma message "The OpenMenuOS library is still in Beta. If you find any bug, please create an issue on the OpenMenuOS's Github repository"

#include "Arduino.h"
#include <TFT_eSPI.h>
#include <EEPROM.h>
#include "OpenMenuOS.h"

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite canvas = TFT_eSprite(&tft);

#define SHORT_PRESS_TIME 300      // 300 milliseconds
#define LONG_PRESS_TIME 1000      // 1000 milliseconds
#define LONG_PRESS_TIME_MENU 500  // 500 milliseconds

////////////////// Variables for button presses //////////////////
unsigned long pressedTime = 0;
unsigned long releasedTime = 0;
bool isPressing = false;
bool isLongDetected = false;

bool PreviousButtonState = LOW;
bool ButtonPressProcessed = false;
int upButtonState;
int downButtonState;
////////////////// Variables for text scrolling //////////////////
int CHAR_WIDTH;
int CHAR_SPACING;
unsigned long previousMillis1 = 0;
unsigned long previousMillis2 = 0;
uint16_t w, h;
//////////////////////////////////////////////////////////////////

// Display Constants
int tftWidth;
int tftHeight;

// Tile menu variable
int current_screen_tile_menu = 0;
int item_selected_tile_menu = 2;
int tile_menu_selection_X = 0;
int tile_menu_selection_Y = 0;

// Rectangle dimensions
uint16_t rect_width = tftWidth - 5;  // Width of the selection rectangle
uint16_t rect_height = 26;           // Height of the selection rectangle

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
bool long_press_handled = false;  // Flag to track if long press action has been handled


bool textScroll = true;
bool buttonAnimation = true;
bool scrollbar = true;
bool bootImage = false;
int menuStyle = 0;
uint16_t selectionBorderColor = TFT_WHITE;
int scrollbarStyle = 0;
uint16_t selectionFillColor = TFT_BLACK;
uint16_t scrollbarColor = TFT_WHITE;

// Button Pin Definitions
int buttonsMode;
int buttonVoltage;
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
  true,
  false,
  true,
  false,
  false,
  false,
  false,
  false,
  false,
  false,
};

OpenMenuOS::OpenMenuOS(int btn_up, int btn_down, int btn_sel, int tft_bl) {
  BUTTON_UP_PIN = btn_up;
  BUTTON_DOWN_PIN = btn_down;
  BUTTON_SELECT_PIN = btn_sel;
  TFT_BL_PIN = tft_bl;
}

void OpenMenuOS::begin(int rotation) {  //  Display Rotation
  // Set up display
  tft.init();
  tft.setRotation(rotation);

  tftWidth = tft.width();
  tftHeight = tft.height();

  // Show The Boot image if bootImage is true
  if (bootImage) {
    tft.pushImage(0, 0, 160, 80, (uint16_t*)Boot_img);  // To remove as will not work correctly with other display size
  }

  tft.setTextWrap(false);
  canvas.setSwapBytes(true);
  canvas.createSprite(tftWidth, tftWidth);
  canvas.fillSprite(TFT_BLACK);

  item_selected = 0;
  current_screen = 0;  // 0 = Menu, 1 = Submenu

  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  readFromEEPROM();

  // Set TFT_BL_PIN as OUTPUT and se it HIGH or LOW depending on the settings
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, menu_items_settings_bool[0] ? LOW : HIGH);

  // Set up button pins
  pinMode(BUTTON_UP_PIN, buttonsMode);

  pinMode(BUTTON_DOWN_PIN, buttonsMode);

  pinMode(BUTTON_SELECT_PIN, buttonsMode);
}
void OpenMenuOS::loop() {
  checkForButtonPress();
  checkForButtonPressSubmenu();
  canvas.fillSprite(TFT_BLACK);  // Set the background of the canvas/sprite to black instead of transparent
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
  if (!scrollbar) {       // If no scrollbar, offset the text from 18px
    rect_width = tftWidth;
  } else if (scrollbar) {  // If no scrollbar, offset the text from 18px
    rect_width = tftWidth - 5;
  }

  // Calculate the position of the rectangle
  uint16_t rect_x = 0;
  uint16_t rect_y = (tftHeight - rect_height) / 2;  // Center the rectangle vertically

  uint16_t selectedItemColor;

  canvas.fillRoundRect(rect_x + 1, rect_y - 25, rect_width - 3, rect_height - 3, 4, TFT_BLACK);  // Remove Old Text. Change it by setting old text's color to white? (May be slower and more complicated??)
  canvas.fillRoundRect(rect_x + 1, rect_y - 1, rect_width - 3, rect_height - 3, 4, TFT_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 27, rect_width - 3, rect_height - 3, 4, TFT_BLACK);

  switch (menuStyle) {
    case 0:
      if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage && buttonAnimation) {
        canvas.drawSmoothRoundRect(rect_x + 1, rect_y + 1, 4, 4, rect_width - 2, rect_height - 1, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly

      } else {
        if (!scrollbar) {
          canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
          canvas.drawFastVLine(tftWidth - 2, 29, 23, selectionBorderColor);                                            // Display the inside part
          canvas.drawFastVLine(tftWidth, 29, 22, selectionBorderColor);                                                // Display the Shadow
          canvas.drawFastHLine(2, 51, tftWidth - 3, selectionBorderColor);                                             // Display the inside part
          canvas.drawFastHLine(3, 51, tftWidth - 4, selectionBorderColor);                                             // Display the Shadow
        } else {
          canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width - 2, rect_height, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
          canvas.drawFastVLine(rect_width - 4, rect_y + 2, 23, selectionBorderColor);                                      // Display the inside part
          canvas.drawFastVLine(rect_width - 3, rect_y + 2, 22, selectionBorderColor);                                      // Display the Shadow
          canvas.drawFastHLine(2, 51, 149, selectionBorderColor);                                                          // Display the inside part
          canvas.drawFastHLine(3, 52, 148, selectionBorderColor);                                                          // Display the Shadow
        }
      }
      selectedItemColor = TFT_WHITE;
      break;
    case 1:
      if (!scrollbar) {
        canvas.fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 4, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
        // canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly                                          // Display the Shadow
      } else {
        canvas.fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 4, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
        // canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly                                                         // Display the Shadow
      }
      selectedItemColor = TFT_BLACK;

      break;
  }
  int xPos = 30;   // X Position of text 0
  int x1Pos = 30;  // X Position of text 1
  int x2Pos = 30;  // X Position of text 2

  int yPos = 17;   // Y Position of text 0
  int y1Pos = 44;  // Y Position of text 1
  int y2Pos = 70;  // Y Position of text 2
  // int sectionHeight = tftHeight / 3;
  // int yPos = sectionHeight / 2 + 6;   // Center of the first section
  // canvas.drawFastHLine(0, yPos, 160, TFT_RED);
  // int y1Pos = sectionHeight + sectionHeight / 2 + 6;  // Center of the second section
  // canvas.drawFastHLine(0, y1Pos, 160, TFT_RED);
  // int y2Pos = 2 * sectionHeight + sectionHeight / 2 +6;  // Center of the third section
  // canvas.drawFastHLine(0, y2Pos, 160, TFT_RED);
  int scrollWindowSize = 120;
  if (!images) {  // If no image, offset the text from 18px
    xPos -= 18;
    x1Pos -= 18;
    x2Pos -= 18;
    scrollWindowSize += 18;
  }

  // draw previous item as icon + label
  canvas.setFreeFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
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
    canvas.pushImage(5, 5, 16, 16, (uint16_t*)bitmap_icons[item_sel_previous]);
  }
  // draw selected item as icon + label in bold font
  if (strlen(menu_items[item_selected]) > MAX_ITEM_LENGTH_NOT_SCROLLING && textScroll) {
    // tft.setFreeFont(&FreeMonoBold9pt7b);  // Here, We don't use "canvas." because when using it, the calculated value in the "scrollTextHorizontal" function is wrong but not with "tft." #bug
    canvas.setFreeFont(&FreeMonoBold9pt7b);
    scrollTextHorizontal(x1Pos, y1Pos, menu_items[item_selected], selectedItemColor, selectionFillColor, 1, 50, scrollWindowSize);  // Adjust windowSize as needed
  } else if (!textScroll) {
    // draw selected item as icon + label
    canvas.setFreeFont(&FreeMono9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(selectedItemColor, selectionFillColor);
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
    canvas.setFreeFont(&FreeMonoBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(selectedItemColor, selectionFillColor);
    canvas.setCursor(x1Pos, y1Pos);
    canvas.println(menu_items[item_selected]);
  }

  if (images) {
    canvas.pushImage(5, 32, 16, 16, (uint16_t*)bitmap_icons[item_selected]);
  }
  // draw next item as icon + label
  canvas.setFreeFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
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
    canvas.pushImage(5, 59, 16, 16, (uint16_t*)bitmap_icons[item_sel_next]);
  }
  if (scrollbar) {
    // Draw the scrollbar
    drawScrollbar(item_selected, item_sel_next);
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

  if (!scrollbar) {  // If no scrollbar, offset the text from 18px
    rect_width = tftWidth;
  }
  // Calculate the position of the rectangle
  uint16_t rect_x = 0;
  uint16_t rect_y = (tftHeight - rect_height) / 2;  // Center the rectangle vertically
  uint16_t selectedItemColor;
  canvas.fillRoundRect(rect_x + 1, rect_y - 25, rect_width - 3, rect_height - 3, 4, TFT_BLACK);  // Remove Old Text. Change it by setting old text's color to white? (May be slower and more complicated??)
  canvas.fillRoundRect(rect_x + 1, rect_y - 1, rect_width - 3, rect_height - 3, 4, TFT_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 27, rect_width - 3, rect_height - 3, 4, TFT_BLACK);

  switch (menuStyle) {
    case 0:
      if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage && buttonAnimation) {
        // canvas.drawRoundRect(rect_x + 1, rect_y + 1, rect_width - 2, rect_height - 1, 4, selectionBorderColor);  // Display the rectangle
        canvas.drawSmoothRoundRect(rect_x + 1, rect_y + 1, 4, 4, rect_width - 2, rect_height - 1, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly

      } else {
        if (!scrollbar) {
          canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
          canvas.drawFastVLine(tftWidth - 2, 29, 23, selectionBorderColor);                                            // Display the inside part
          canvas.drawFastVLine(tftWidth, 29, 22, selectionBorderColor);                                                // Display the Shadow
          canvas.drawFastHLine(2, 51, tftWidth - 3, selectionBorderColor);                                             // Display the inside part
          canvas.drawFastHLine(3, 51, tftWidth - 4, selectionBorderColor);                                             // Display the Shadow
        } else {
          canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width - 2, rect_height, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
          canvas.drawFastVLine(rect_width - 4, rect_y + 2, 23, selectionBorderColor);                                      // Display the inside part
          canvas.drawFastVLine(rect_width - 3, rect_y + 2, 22, selectionBorderColor);                                      // Display the Shadow
          canvas.drawFastHLine(2, 51, 149, selectionBorderColor);                                                          // Display the inside part
          canvas.drawFastHLine(3, 52, 148, selectionBorderColor);                                                          // Display the Shadow
        }
      }
      selectedItemColor = TFT_WHITE;
      break;
    case 1:
      canvas.fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 4, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly

      selectedItemColor = TFT_BLACK;

      break;
  }
  int xPos = 30;   // X Position of text 0
  int yPos = 17;   // Y Position of text 0
  int x1Pos = 30;  // X Position of text 1
  int y1Pos = 44;  // Y Position of text 1
  int x2Pos = 30;  // X Position of text 2
  int y2Pos = 70;  // Y Position of text 2
  int scrollWindowSize = 120;

  if (!images) {  // If no image, offset the text from 18px
    xPos -= 18;
    x1Pos -= 18;
    x2Pos -= 18;
    scrollWindowSize += 18;
  }
  // draw previous item as icon + label
  canvas.setFreeFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
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
    canvas.pushImage(5, 5, 16, 16, (uint16_t*)bitmap_icons[item_sel_previous_submenu]);
  }

  // draw selected item as icon + label in bold font
  if (strlen(submenu_items[item_selected_submenu]) > MAX_ITEM_LENGTH_NOT_SCROLLING && textScroll) {
    // tft.setFreeFont(&FreeMonoBold9pt7b);  // Here, We don't use "canvas." because when using it, the calculated value in the "scrollTextHorizontal" function is wrong but not with "tft." #bug
    canvas.setFreeFont(&FreeMonoBold9pt7b);
    scrollTextHorizontal(x1Pos, y1Pos, submenu_items[item_selected_submenu], selectedItemColor, selectionFillColor, 1, 50, 120);  // Adjust windowSize as needed
  } else if (!textScroll) {
    // draw selected item as icon + label
    canvas.setFreeFont(&FreeMono9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(selectedItemColor, selectionFillColor);
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
    canvas.setFreeFont(&FreeMonoBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(selectedItemColor, selectionFillColor);
    canvas.setCursor(x1Pos, y1Pos);
    canvas.println(submenu_items[item_selected_submenu]);
  }
  if (images) {
    canvas.pushImage(5, 32, 16, 16, (uint16_t*)bitmap_icons[item_selected_submenu]);
  }
  // draw next item as icon + label
  canvas.setFreeFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
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
    canvas.pushImage(5, 59, 16, 16, (uint16_t*)bitmap_icons[item_sel_next_submenu]);
  }

  if (scrollbar) {
    // Draw the scrollbar
    drawScrollbar(item_selected_submenu, item_sel_next_submenu);
  }
}

void OpenMenuOS::drawSettingMenu(const char* items...) {

  NUM_SETTINGS_ITEMS = 1;
  va_list args;
  va_start(args, items);
  const char* item = items;
  while (item != NULL && NUM_SETTINGS_ITEMS < MAX_SETTINGS_ITEMS) {
    strncpy(menu_items_settings[NUM_SETTINGS_ITEMS++], item, MAX_ITEM_LENGTH - 1);
    item = va_arg(args, const char*);
  }
  va_end(args);

  if (BUTTON_UP_PIN != NULL) {
    ///////////////////////IF REMOVED, THE SHORT PRESS DOESN'T DO ANYTHING #Bug////////////////////////////////
    if ((digitalRead(BUTTON_UP_PIN) == buttonVoltage) && (button_up_clicked_settings == 0)) {  // up button clicked - jump to previous menu item
      item_selected_settings = item_selected_settings - 1;                                     // select previous item
      button_up_clicked_settings = 1;                                                          // set button to clicked to only perform the action once
      if (item_selected_settings < 0) {                                                        // if first item was selected, jump to last item
        item_selected_settings = NUM_SETTINGS_ITEMS - 1;
      }
    }

    if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked_settings == 1)) {  // unclick
      button_up_clicked_settings = 0;
    }
    ///////////////////////////////////////////////////////

    upButtonState = digitalRead(BUTTON_UP_PIN);  // Read the current button state

    if (upButtonState == HIGH && !ButtonPressProcessed) {
      if (PreviousButtonState == LOW) {
        pressedTime = millis();
        isPressing = true;
        isLongDetected = false;  // Ensure long press flag is reset each time button is pressed
        ButtonPressProcessed = true;
      }
    }

    if (isPressing && !isLongDetected) {
      long pressDuration = millis() - pressedTime;

      if (pressDuration > LONG_PRESS_TIME_MENU) {
        isLongDetected = true;
      }
    }
    if (isPressing && isLongDetected) {
      static unsigned long lastTempAdjustTime = 0;
      unsigned long currentMillis = millis();
      if (currentMillis - lastTempAdjustTime >= 200) {        // Check if 00ms have passed
        item_selected_settings = item_selected_settings - 1;  // select previous item
        button_up_clicked_settings = 1;                       // set button to clicked to only perform the action once
        if (item_selected_settings < 0) {                     // if first item was selected, jump to last item
          item_selected_settings = NUM_SETTINGS_ITEMS - 1;
        }
        lastTempAdjustTime = currentMillis;  // Update the last adjustment time
      }
    }

    if (upButtonState == LOW && PreviousButtonState == HIGH) {
      isPressing = false;
      releasedTime = millis();

      long pressDuration = releasedTime - pressedTime;

      // Determine the type of press based on the duration
      if (pressDuration >= LONG_PRESS_TIME_MENU) {
        isLongDetected = true;
      } else {
        isLongDetected = false;
      }

      // Check if it was a short press and long press was not detected
      if (pressDuration < SHORT_PRESS_TIME && !isLongDetected) {
        // item_selected_settings = item_selected_settings - 1;  // select previous item
        // button_up_clicked_settings = 1;                       // set button to clicked to only perform the action once
        // if (item_selected_settings < 0) {                     // if first item was selected, jump to last item
        //   item_selected_settings = NUM_SETTINGS_ITEMS - 1;
        // }
      }
      ButtonPressProcessed = false;  // Reset the control variable when the button is not pressed anymore
    }

    PreviousButtonState = upButtonState;
  }


  if (BUTTON_DOWN_PIN != NULL) {
    ///////////////////////IF REMOVED, THE SHORT PRESS DOESN'T DO ANYTHING #Bug////////////////////////////////
    if ((digitalRead(BUTTON_DOWN_PIN) == buttonVoltage) && (button_down_clicked_settings == 0)) {  // up button clicked - jump to previous menu item
      item_selected_settings = item_selected_settings + 1;                                         // select next item
      button_down_clicked_settings = 1;                                                            // set button to clicked to only perform the action once
      if (item_selected_settings >= NUM_SETTINGS_ITEMS) {                                          // if last item was selected, jump to first item
        item_selected_settings = 0;
      }
    }

    if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked_settings == 1)) {  // unclick
      button_down_clicked_settings = 0;
    }
    ///////////////////////////////////////////////////////

    int downButtonState = digitalRead(BUTTON_DOWN_PIN);  // Read the current button state

    if (downButtonState == HIGH && !ButtonPressProcessed) {
      if (PreviousButtonState == LOW) {
        pressedTime = millis();
        isPressing = true;
        isLongDetected = false;  // Ensure long press flag is reset each time button is pressed
        ButtonPressProcessed = true;
      }
    }

    if (isPressing && !isLongDetected) {
      long pressDuration = millis() - pressedTime;

      if (pressDuration > LONG_PRESS_TIME_MENU) {
        isLongDetected = true;
      }
    }
    if (isPressing && isLongDetected) {
      static unsigned long lastTempAdjustTime = 0;
      unsigned long currentMillis = millis();
      if (currentMillis - lastTempAdjustTime >= 200) {        // Check if 200ms have passed
        item_selected_settings = item_selected_settings + 1;  // select next item
        if (item_selected_settings >= NUM_SETTINGS_ITEMS) {   // if last item was selected, jump to first item
          item_selected_settings = 0;
        }
        lastTempAdjustTime = currentMillis;  // Update the last adjustment time
      }
    }

    if (downButtonState == LOW && PreviousButtonState == HIGH) {
      isPressing = false;
      releasedTime = millis();

      long pressDuration = releasedTime - pressedTime;

      // Determine the type of press based on the duration
      if (pressDuration >= LONG_PRESS_TIME_MENU) {
        isLongDetected = true;
      } else {
        isLongDetected = false;
      }

      // Check if it was a short press and long press was not detected
      if (pressDuration < SHORT_PRESS_TIME && !isLongDetected) {
        // item_selected_settings = item_selected_settings + 1;  // select next item
        // if (item_selected_settings >= NUM_SETTINGS_ITEMS) {   // if last item was selected, jump to first item
        //   item_selected_settings = 0;
        // }
      }
      ButtonPressProcessed = false;  // Reset the control variable when the button is not pressed anymore
    }

    PreviousButtonState = downButtonState;
  }
  // Calculate the position of the rectangle
  uint16_t rect_x = 0;
  uint16_t rect_y = (tftHeight - rect_height) / 2;  // Center the rectangle vertically

  uint16_t selectedItemColor;

  canvas.fillRoundRect(rect_x + 1, rect_y - 25, rect_width - 3, rect_height - 3, 4, TFT_BLACK);  // Remove Old Text. Change it by setting old text's color to white? (May be slower and more complicated??)
  canvas.fillRoundRect(rect_x + 1, rect_y - 1, rect_width - 3, rect_height - 3, 4, TFT_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 27, rect_width - 3, rect_height - 3, 4, TFT_BLACK);

  switch (menuStyle) {
    case 0:
      if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage && buttonAnimation) {
        // canvas.drawRoundRect(rect_x + 1, rect_y + 1, rect_width - 2, rect_height - 1, 4, selectionBorderColor);  // Display the rectangle
        canvas.drawSmoothRoundRect(rect_x + 1, rect_y + 1, 4, 4, rect_width - 2, rect_height - 1, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly

      } else {
        if (!scrollbar) {
          canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
          canvas.drawFastVLine(tftWidth - 2, 29, 23, selectionBorderColor);                                            // Display the inside part
          canvas.drawFastVLine(tftWidth, 29, 22, selectionBorderColor);                                                // Display the Shadow
          canvas.drawFastHLine(2, 51, tftWidth - 3, selectionBorderColor);                                             // Display the inside part
          canvas.drawFastHLine(3, 51, tftWidth - 4, selectionBorderColor);                                             // Display the Shadow
        } else {
          canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width - 2, rect_height, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
          canvas.drawFastVLine(rect_width - 4, rect_y + 2, 23, selectionBorderColor);                                      // Display the inside part
          canvas.drawFastVLine(rect_width - 3, rect_y + 2, 22, selectionBorderColor);                                      // Display the Shadow
          canvas.drawFastHLine(2, 51, 149, selectionBorderColor);                                                          // Display the inside part
          canvas.drawFastHLine(3, 52, 148, selectionBorderColor);                                                          // Display the Shadow
        }
      }
      selectedItemColor = TFT_WHITE;
      break;
    case 1:
      // canvas.fillRoundRect(rect_x, rect_y, rect_width, rect_height, 4, selectionBorderColor);  // Display the rectangle
      canvas.fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 4, selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly

      selectedItemColor = TFT_BLACK;

      break;
  }
  int xPos = 10;   // X Position of text 0
  int yPos = 17;   // Y Position of text 0
  int x1Pos = 10;  // X Position of text 1
  int y1Pos = 44;  // Y Position of text 1
  int x2Pos = 10;  // X Position of text 2
  int y2Pos = 70;  // Y Position of text 2
  int scrollWindowSize = 100;

  // draw previous item as icon + label
  canvas.setFreeFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(xPos, yPos);

  // Get the text of the previous item
  String previousItem = menu_items_settings[item_selected_settings_previous];

  // Check if the length of the text exceeds the maximum length for non-scrolling items
  if (previousItem.length() > MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING) {
    // Truncate the text and add "..." at the end
    previousItem = previousItem.substring(0, MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
  }

  // Print the modified text
  canvas.println(previousItem);

  /////// Variables for the toggle buttons ///////
  int rectWidth = 41;
  int rectHeight = 20;
  int rectRadius = 10;

  int roundXOffset = 2;  // Horizontal offset for the round shape
  int roundDiameter = 15;
  int roundRadius = 10;

  // int rectX = 114;
  int rectX = 110;
  ////////////////////////////////////////////////

  if (item_selected_settings_previous >= 0) {
    int rectY = 2;

    // Calculate the vertical center of the rounded rectangle
    int rectCenterY = rectY + (rectHeight / 2);

    // Calculate the Y position for the round shape to be centered vertically
    int roundY = rectCenterY - (roundDiameter / 2);

    drawToggleSwitch(rectX, rectY, menu_items_settings_bool[item_selected_settings_previous]);
  }

  // draw selected item as icon + label in bold font
  if (strlen(menu_items_settings[item_selected_settings]) > MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING && textScroll) {
    tft.setFreeFont(&FreeMonoBold9pt7b);  // Here, We don't use "canvas." because when using it, the calculated value in the "scrollTextHorizontal" function is wrong but not with "tft." #bug
    canvas.setFreeFont(&FreeMonoBold9pt7b);
    scrollTextHorizontal(x1Pos, y1Pos, menu_items_settings[item_selected_settings], selectedItemColor, selectionFillColor, 1, 50, scrollWindowSize);  // Adjust windowSize as needed
  } else if (!textScroll) {
    // draw selected item as icon + label
    canvas.setFreeFont(&FreeMono9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(selectedItemColor, selectionFillColor);
    canvas.setCursor(x1Pos, y1Pos);

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
    canvas.setFreeFont(&FreeMonoBold9pt7b);
    canvas.setTextSize(1);
    canvas.setTextColor(selectedItemColor, selectionFillColor);
    canvas.setCursor(x1Pos, y1Pos);
    canvas.println(menu_items_settings[item_selected_settings]);
  }

  // if (item_selected_settings >= 0 && item_selected_settings < NUM_SETTINGS_ITEMS) {
  //   if (menu_items_settings_bool[item_selected_settings] == false) {
  //     // canvas.drawRoundRect(114, 30, 40, 20, 11, TFT_RED);
  //     // canvas.fillRoundRect(116, 32, 16, 16, 11, TFT_RED);
  //     canvas.drawSmoothRoundRect(114, 29, 10, 10, 41, 20, TFT_RED, selectionBorderColor);
  //     canvas.fillSmoothRoundRect(116, 32, 16, 16, 11, TFT_RED, selectionBorderColor);
  //   } else {
  //     // canvas.drawRoundRect(114, 30, 40, 20, 11, TFT_GREEN);
  //     // canvas.fillRoundRect(136, 32, 16, 16, 11, TFT_GREEN);
  //     canvas.drawSmoothRoundRect(114, 29, 10, 10, 41, 20, TFT_GREEN, selectionBorderColor);
  //     canvas.fillSmoothRoundRect(136, 32, 16, 16, 11, TFT_GREEN, selectionBorderColor);
  //   }
  // }
  if (item_selected_settings_previous >= 0 && item_selected_settings < NUM_SETTINGS_ITEMS) {
    int rectY = 29;

    // Calculate the vertical center of the rounded rectangle
    int rectCenterY = rectY + (rectHeight / 2);

    // Calculate the Y position for the round shape to be centered vertically
    int roundY = rectCenterY - (roundDiameter / 2);
    drawToggleSwitch(rectX, rectY, menu_items_settings_bool[item_selected_settings]);
  }


  // draw next item as icon + label
  canvas.setFreeFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(x2Pos, y2Pos);

  // Get the text of the next item
  String nextItem = menu_items_settings[item_selected_settings_next];

  // Check if the length of the text exceeds the maximum length for non-scrolling items
  if (nextItem.length() > MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING) {
    // Truncate the text and add "..." at the end
    nextItem = nextItem.substring(0, MAX_SETTING_ITEM_LENGTH_NOT_SCROLLING - 3) + "...";
  }

  // Print the modified text
  canvas.println(nextItem);

  // if ((item_selected_settings_next) < NUM_SETTINGS_ITEMS) {
  //   if (menu_items_settings_bool[item_selected_settings_next] == false) {
  //     // canvas.drawRoundRect(114, 56, 40, 20, 11, TFT_RED);
  //     // canvas.fillRoundRect(116, 58, 16, 16, 11, TFT_RED);
  //     canvas.drawSmoothRoundRect(114, 55, 10, 10, 41, 20, TFT_RED, TFT_BLACK);
  //     canvas.fillSmoothRoundRect(116, 58, 16, 16, 11, TFT_RED, TFT_BLACK);
  //   } else {
  //     // canvas.drawRoundRect(114, 56, 40, 20, 11, TFT_GREEN);
  //     // canvas.fillRoundRect(136, 58, 16, 16, 11, TFT_GREEN);
  //     canvas.drawSmoothRoundRect(114, 55, 10, 10, 41, 20, TFT_GREEN, TFT_BLACK);
  //     canvas.fillSmoothRoundRect(136, 58, 16, 16, 11, TFT_GREEN, TFT_BLACK);
  //   }
  // }
  if (item_selected_settings_next < NUM_SETTINGS_ITEMS) {
    int rectY = 55;

    // Calculate the vertical center of the rounded rectangle
    int rectCenterY = rectY + (rectHeight / 2);

    // Calculate the Y position for the round shape to be centered vertically
    int roundY = rectCenterY - (roundDiameter / 2);

    drawToggleSwitch(rectX, rectY, menu_items_settings_bool[item_selected_settings_next]);
  }

  /////////////////////////////////////Still there only for reference purpose///////////////////////////////////////////

  // if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage) {
  //   if (previousButtonState == LOW && !buttonPressProcessed) {
  //     if (item_selected_settings == 0) {
  //       if (menu_items_settings_bool[0] == true) {
  //         menu_items_settings_bool[0] = false;
  //         digitalWrite(TFT_BL_PIN, HIGH);
  //       } else {
  //         menu_items_settings_bool[0] = true;
  //         digitalWrite(TFT_BL_PIN, LOW);
  //       }
  //     } else if (item_selected_settings == 1) {
  //       if (menu_items_settings_bool[1] == true) {
  //         menu_items_settings_bool[1] = false;
  //       } else {
  //         menu_items_settings_bool[1] = true;
  //       }
  //     } else if (item_selected_settings == 2) {
  //       if (menu_items_settings_bool[2] == true) {
  //         menu_items_settings_bool[2] = false;
  //       } else {
  //         menu_items_settings_bool[2] = true;
  //       }
  //     } else if (item_selected_settings == 3) {
  //       if (menu_items_settings_bool[3] == true) {
  //         menu_items_settings_bool[3] = false;
  //       } else {
  //         menu_items_settings_bool[3] = true;
  //       }
  //     } else if (item_selected_settings == 4) {
  //       if (menu_items_settings_bool[4] == true) {
  //         menu_items_settings_bool[4] = false;
  //       } else {
  //         menu_items_settings_bool[4] = true;
  //       }
  //     } else if (item_selected_settings == 5) {
  //       if (menu_items_settings_bool[5] == true) {
  //         menu_items_settings_bool[5] = false;
  //       } else {
  //         menu_items_settings_bool[5] = true;
  //       }
  //     } else if (item_selected_settings == 6) {
  //       if (menu_items_settings_bool[6] == true) {
  //         menu_items_settings_bool[6] = false;
  //       } else {
  //         menu_items_settings_bool[6] = true;
  //       }
  //     } else if (item_selected_settings == 7) {
  //       if (menu_items_settings_bool[7] == true) {
  //         menu_items_settings_bool[7] = false;
  //       } else {
  //         menu_items_settings_bool[7] = true;
  //       }
  //     } else if (item_selected_settings == 8) {
  //       if (menu_items_settings_bool[8] == true) {
  //         menu_items_settings_bool[8] = false;
  //       } else {
  //         menu_items_settings_bool[8] = true;
  //       }
  //     } else if (item_selected_settings == 9) {
  //       if (menu_items_settings_bool[9] == true) {
  //         menu_items_settings_bool[9] = false;
  //       } else {
  //         menu_items_settings_bool[9] = true;
  //       }
  //     }
  //     buttonPressProcessed = true;
  //     saveToEEPROM();
  //   }
  //   previousButtonState = HIGH;
  // } else {
  //   previousButtonState = LOW;
  //   buttonPressProcessed = false;  // Reset the control variable when the button is not pressed anymore
  // }

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////

  upButtonState = digitalRead(BUTTON_SELECT_PIN);  // Read the current button state

  if (upButtonState == buttonVoltage && !buttonPressProcessed) {
    if (previousButtonState == !buttonVoltage) {
      pressedTime = millis();
      isPressing = true;
      isLongDetected = false;  // Ensure long press flag is reset each time button is pressed
      buttonPressProcessed = true;
    }
  }

  if (isPressing && !isLongDetected) {
    long pressDuration = millis() - pressedTime;

    if (pressDuration > LONG_PRESS_TIME) {
      isLongDetected = true;
    }
  }

  if (upButtonState == !buttonVoltage && previousButtonState == buttonVoltage) {
    isPressing = false;
    releasedTime = millis();

    long pressDuration = releasedTime - pressedTime;

    // Determine the type of press based on the duration
    if (pressDuration >= LONG_PRESS_TIME) {
      isLongDetected = true;
    } else {
      isLongDetected = false;
    }

    // Check if it was a short press and long press was not detected
    if (pressDuration < SHORT_PRESS_TIME && !isLongDetected) {
      if (current_screen == 1) {
        if (item_selected_settings >= 0 && item_selected_settings < 10) {
          menu_items_settings_bool[item_selected_settings] = !menu_items_settings_bool[item_selected_settings];
          if (item_selected_settings == 0) {
            digitalWrite(TFT_BL_PIN, menu_items_settings_bool[0] ? LOW : HIGH);  // Toggle TFT backlight pin based on boolean state
          }
          saveToEEPROM();
        }
      }
    }
    buttonPressProcessed = false;  // Reset the control variable when the button is not pressed anymore
  }

  previousButtonState = upButtonState;

  if (scrollbar) {
    // Draw the scrollbar
    drawScrollbar(item_selected_settings, item_selected_settings_next);
  }
}
void OpenMenuOS::drawToggleSwitch(int16_t x, int16_t y, bool state) {
  uint16_t switchWidth = 40;
  uint16_t switchHeight = 20;
  uint16_t knobDiameter = 16;

  uint16_t bgColor = state ? TFT_GREEN : TFT_RED;
  uint16_t knobColor = TFT_WHITE;

  // Draw switch background
  canvas.fillSmoothRoundRect(x, y, switchWidth, switchHeight, switchHeight / 2, bgColor, TFT_BLACK);

  // Draw knob
  int16_t knobX = state ? x + switchWidth - knobDiameter - 2 : x + 2;
  canvas.fillSmoothCircle(knobX + knobDiameter / 2, y + switchHeight / 2, knobDiameter / 2, knobColor, bgColor);
}

void OpenMenuOS::drawTileMenu(int rows, int columns, int tile_color) {
  int TILE_ROUND_RADIUS = 5;
  int TILE_MARGIN = 2;
  int tileWidth = (tftWidth - (columns + 1) * TILE_MARGIN) / columns;
  int tileHeight = (tftHeight - (rows + 1) * TILE_MARGIN) / rows;

  if (current_screen_tile_menu == 0) {
    if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage) {
      if (current_screen_tile_menu == 0) {
        current_screen_tile_menu = 1;
      }
      button_select_clicked = 1;
    }
    if ((digitalRead(BUTTON_UP_PIN) == buttonVoltage) && (button_up_clicked_tile == 0)) {  // up button clicked - jump to previous menu item
      item_selected_tile_menu = item_selected_tile_menu + 1;                               // select previous item
      button_up_clicked_tile = 1;                                                          // set button to clicked to only perform the action once
      if (item_selected_tile_menu >= rows * columns) {                                     // if first item was selected, jump to last item
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

    canvas.fillSprite(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE, tile_color);
    canvas.setTextSize(1);

    for (int i = 0; i < rows * columns; ++i) {
      int row = i / columns;
      int col = i % columns;
      int tileX = col * (tileWidth + TILE_MARGIN) + TILE_MARGIN;
      int tileY = row * (tileHeight + TILE_MARGIN) + TILE_MARGIN;
      canvas.fillSmoothRoundRect(tileX, tileY, tileWidth, tileHeight, TILE_ROUND_RADIUS, tile_color, TFT_BLACK);
    }
    canvas.drawSmoothRoundRect(tile_menu_selection_X, tile_menu_selection_Y, TILE_ROUND_RADIUS, TILE_ROUND_RADIUS, tileWidth, tileHeight, TFT_WHITE, TFT_BLACK);
  } else if (current_screen_tile_menu == 1) {
    delay(200);
    if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage) {
      if (current_screen_tile_menu == 1) {
        current_screen_tile_menu = 0;
        delay(200);
      }
    }
    canvas.fillSprite(TFT_BLACK);
    canvas.setFreeFont(nullptr);
  }
}
void OpenMenuOS::redirectToMenu(int screen, int item) {
  current_screen = screen;
  if (current_screen == 0) {
    item_selected = item;
  } else if (current_screen == 1) {
    item_selected_submenu = item;
  }
}
void OpenMenuOS::drawPopup(char* message, bool& clicked, int type) {
  static bool selectClicked = false;
  uint16_t color;
  char* title;
  int iconW, iconH;
  int iconX, iconY;
  int spaceBetweenPopup = 3;   // The space between the screen limits and the popup
  int coloredPercentage = 30;  // The percentage of the colored section. Change to 25????
  int popupWidth = tftWidth - spaceBetweenPopup * 2;
  int popupHeight = tftHeight - spaceBetweenPopup * 2;
  int buttonWidth = popupWidth / 3;
  int buttonHeight = popupHeight / 5;
  int buttonX = (tftWidth - buttonWidth) / 2;                      // X position of the OK button
  int buttonY = tftHeight - buttonHeight - spaceBetweenPopup - 3;  // Y position of the OK button
  int radius = 5;                                                  // The radius of the popup's corners
  // Set color depending on popup type (Warning, Success, Info)
  switch (type) {
    case 1:  // Warning
      color = 0xf2aa;
      title = "Warning!";
      iconW = 21;
      iconH = 18;
      break;
    case 2:  // Success
      color = 0x260f;
      title = "Success!";
      iconW = 13;
      iconH = 10;
      break;
    case 3:  // Info
      color = 0x453e;
      title = "Info";
      iconW = 6;
      iconH = 14;
      break;
  }

  // Determine icon's positionning
  iconX = (tftWidth - iconW) / 2;                                                           // Center the icon on the X axis
  iconY = (((popupHeight * coloredPercentage) / 100) - iconH) / 2 + spaceBetweenPopup + 1;  // Center the icon on the Y axis

  // Check if the select button is pressed and hasn't been clicked before
  if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage && !selectClicked) {
    selectClicked = true;
    clicked = true;
  }

  // If select button not clicked, draw the popup

  // Draw the background of the popupcanvas.fillSprite
  canvas.fillSprite(TFT_BLACK);  // Uncomment?
  canvas.fillSmoothRoundRect(spaceBetweenPopup, spaceBetweenPopup, tftWidth - spaceBetweenPopup * 2, popupHeight, radius, TFT_WHITE, TFT_BLACK);
  canvas.fillSmoothRoundRect(spaceBetweenPopup, spaceBetweenPopup, tftWidth - spaceBetweenPopup * 2, ((popupHeight * coloredPercentage) / 100) + radius, radius, color, TFT_BLACK);  // The colored part is 34% of the height of the popup

  // To be corrected, Y not placed correctly when popup is another size #bug
  canvas.fillRect(spaceBetweenPopup, ((popupHeight * coloredPercentage) / 100) + radius, tftWidth - spaceBetweenPopup * 2, radius, TFT_WHITE);  // Hide the bottom part of the colored rounded rectangle to make it flat



  switch (type) {
    case 1:  // Warning
      canvas.pushImage(iconX, iconY, iconW, iconH, (uint16_t*)Warning_icon);
      break;
    case 2:  // Success
      canvas.pushImage(iconX, iconY, iconW, iconH, (uint16_t*)Success_icon);
      break;
    case 3:  // Info
      canvas.pushImage(iconX, iconY, iconW, iconH, (uint16_t*)Info_icon);
      break;
  }

  // Calculate message area dimensions
  int messageAreaWidth = tftWidth - 40;
  int messageAreaHeight = buttonY - 40;  // Gap between button and top of popup
  int messageAreaX = (tftWidth - messageAreaWidth) / 2;
  int messageAreaY = 50;  // Adjust the position as needed
  int titleAreaWidth = tftWidth - 40;
  int titleAreaHeight = buttonY - 40;                               // Gap between button and top of popup
  int titleAreaX = (tftWidth - titleAreaWidth) / 2;                 // Center title horizontally
  int titleAreaY = ((popupHeight * coloredPercentage) / 100) + 17;  // Adjust the position as needed

  // Draw the message
  uint16_t messageWidth = canvas.textWidth(message);
  uint16_t messageHeight = canvas.fontHeight();
  int16_t messageX, messageY;
  uint16_t titleWidth = canvas.textWidth(title);
  uint16_t titleHeight = canvas.fontHeight();
  int16_t titleX, titleY;

  // Draw the title
  titleX = titleAreaX + (titleAreaWidth - titleWidth) / 2;  // Center title horizontally
  canvas.setCursor(titleX, titleAreaY);                     // Set the cursor to the center
  canvas.setFreeFont(&FreeMonoBold9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_BLACK, TFT_WHITE);
  canvas.print(title);

  // Draw the message
  // if (messageAreaWidth > tftWidth - 8) {
  // tft.setFreeFont(&FreeMonoBold9pt7b);  // Here, We don't use "canvas." because when using it, the calculated value in the "scrollTextHorizontal" function is wrong but not with "tft." #bug
  canvas.setFreeFont(&FreeMonoBold9pt7b);
  scrollTextHorizontal(spaceBetweenPopup + 1, titleAreaY + 20, message, TFT_BLACK, TFT_WHITE, 1, 50, popupWidth - 2);
  // } else {
  //   // If the message fits within the screen width, draw it normally
  //   messageX = messageAreaX + (messageAreaWidth - messageWidth) / 2;
  //   messageY = messageAreaY + (messageAreaHeight - messageHeight) / 2;

  //   canvas.setCursor(messageX, messageY);
  //   canvas.setFreeFont(&FreeMono9pt7b);
  //   canvas.setTextSize(1);
  //   canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  //   canvas.print(message);
  // }
}

void OpenMenuOS::drawScrollbar(int selectedItem, int nextItem) {
  // Draw scrollbar handle
  int boxHeight = tftHeight / (NUM_MENU_ITEMS);
  int boxY = boxHeight * selectedItem;
  if (scrollbarStyle == 0) {
    // Clear previous scrollbar handle
    canvas.fillRect(tftWidth - 3, boxHeight * nextItem, 3, boxHeight, TFT_BLACK);
    // Draw new scrollbar handle
    canvas.fillRect(tftWidth - 3, boxY, 3, boxHeight, scrollbarColor);

    for (int y = 0; y < tftHeight; y++) {  // Display the Scrollbar
      if (y % 2 == 0) {
        canvas.drawPixel(tftWidth - 2, y, TFT_WHITE);
      }
    }
  } else if (scrollbarStyle == 1) {
    // Clear previous scrollbar handle
    canvas.fillRoundRect(tftWidth - 3, boxY, 3, boxHeight, TFT_BLACK, TFT_BLACK);
    // Draw new scrollbar handle
    canvas.fillSmoothRoundRect(tftWidth - 3, boxY, 3, boxHeight, 4, scrollbarColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
  }
}

void OpenMenuOS::scrollTextHorizontal(int16_t x, int16_t y, const char* text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize) {
  static int16_t xPos = x;
  static unsigned long previousMillis = 0;
  static String currentText = "";

  if (currentText != text) {
    xPos = x;
    currentText = text;
  }

  canvas.setTextSize(textSize);
  int16_t textWidth = canvas.textWidth(text);

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delayTime) {
    previousMillis = currentMillis;
    xPos--;

    if (xPos <= x - textWidth) {
      xPos = x + windowSize;
    }
  }

  // Create a temporary sprite for double buffering
  TFT_eSprite tempSprite = TFT_eSprite(&tft);
  tempSprite.createSprite(windowSize, canvas.fontHeight());
  // tempSprite.setTextFont(canvas.textfont);
  tempSprite.setFreeFont(&FreeMonoBold9pt7b);
  tempSprite.setTextSize(textSize);
  tempSprite.setTextColor(textColor, bgColor);
  // Draw text on the temporary sprite
  int16_t yPos = tempSprite.fontHeight();
  tempSprite.setCursor(xPos - x, yPos);
  tempSprite.print(text);



  // Push the temporary sprite to the main canvas
  // y -= 5;
  tempSprite.pushToSprite(&canvas, x, y - yPos, bgColor);
  tempSprite.deleteSprite();
}

void OpenMenuOS::setTextScroll(bool x = true) {
  textScroll = x;
}
void OpenMenuOS::showBootImage(bool x = true) {
  bootImage = x;
}
void OpenMenuOS::setButtonAnimation(bool x = true) {
  buttonAnimation = x;
}
void OpenMenuOS::setMenuStyle(int style) {
  menuStyle = style;
}
void OpenMenuOS::setScrollbar(bool x = true) {
  scrollbar = x;
}
void OpenMenuOS::setScrollbarColor(uint16_t color = TFT_WHITE) {
  scrollbarColor = color;
}
void OpenMenuOS::setScrollbarStyle(int style) {
  scrollbarStyle = style;
}
void OpenMenuOS::setSelectionBorderColor(uint16_t color = TFT_WHITE) {
  selectionBorderColor = color;
}
void OpenMenuOS::setSelectionFillColor(uint16_t color = TFT_BLACK) {
  selectionFillColor = color;
}
void OpenMenuOS::useStylePreset(char* preset) {
  int presetNumber = 0;  // Initialize with the default preset number

  // Convert the preset name to lowercase for case-insensitive comparison
  String lowercasePreset = String(preset);
  lowercasePreset.toLowerCase();

  // Compare the preset name with lowercase versions of the preset names
  if (lowercasePreset == "default") {
    presetNumber = 0;
  } else if (lowercasePreset == "rabbit_r1") {
    presetNumber = 1;
  }

  // Apply the preset based on the preset number
  switch (presetNumber) {
    case 0:
      setMenuStyle(0);
      break;
    case 1:
      setScrollbar(false);  // Disable scrollbar
      setMenuStyle(1);
      setSelectionBorderColor(0xfa60);  // Setting the selection rectangle's color to Rabbit R1's Orange/Leuchtorange
      setSelectionFillColor(0xfa60);
      break;
    default:
      // Handle invalid presets
      // Optionally, you can log an error or take other actions here
      break;
  }
}

void OpenMenuOS::setButtonsMode(char* mode) {  // The mode is either Pullup or Pulldown
  // Convert mode to lowercase for case-insensitive comparison
  String lowercaseMode = String(mode);
  lowercaseMode.toLowerCase();

  // Check if mode is valid
  if (lowercaseMode == "high") {
    buttonsMode = INPUT_PULLDOWN;
    buttonVoltage = HIGH;
  } else if (lowercaseMode == "low") {
    buttonsMode = INPUT_PULLUP;
    buttonVoltage = LOW;
  } else {
    // Invalid mode, print an error message
    Serial.println("Error: Invalid mode. Please use 'high' or 'low'.");
  }
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
  if (current_screen == 0) {  // MENU SCREEN // up and down buttons only work for the menu screen
                              // if ((digitalRead(BUTTON_UP_PIN) == buttonVoltage) && (button_up_clicked == 0)) {  // up button clicked - jump to previous menu item
                              //   item_selected = item_selected - 1;                                              // select previous item
                              //   button_up_clicked = 1;                                                          // set button to clicked to only perform the action once
                              //   if (item_selected < 0) {                                                        // if first item was selected, jump to last item
                              //     item_selected = NUM_MENU_ITEMS - 1;
                              //   }
                              // }

    // if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 1)) {  // unclick
    //   button_up_clicked = 0;
    // }

    if (BUTTON_UP_PIN != NULL) {

      upButtonState = digitalRead(BUTTON_UP_PIN);  // Read the current button state

      if (upButtonState == HIGH && !ButtonPressProcessed) {
        if (PreviousButtonState == LOW) {
          pressedTime = millis();
          isPressing = true;
          isLongDetected = false;  // Ensure long press flag is reset each time button is pressed
          ButtonPressProcessed = true;
        }
      }

      if (isPressing && !isLongDetected) {
        long pressDuration = millis() - pressedTime;

        if (pressDuration > LONG_PRESS_TIME_MENU) {
          isLongDetected = true;
        }
      }
      if (isPressing && isLongDetected) {
        static unsigned long lastTempAdjustTime = 0;
        unsigned long currentMillis = millis();
        if (currentMillis - lastTempAdjustTime >= 200) {  // Check if 500ms have passed
          item_selected = item_selected - 1;              // select previous item
          if (item_selected < 0) {                        // if first item was selected, jump to last item
            item_selected = NUM_MENU_ITEMS - 1;
          }
          lastTempAdjustTime = currentMillis;  // Update the last adjustment time
        }
      }

      if (upButtonState == LOW && PreviousButtonState == HIGH) {
        isPressing = false;
        releasedTime = millis();

        long pressDuration = releasedTime - pressedTime;

        // Determine the type of press based on the duration
        if (pressDuration >= LONG_PRESS_TIME_MENU) {
          isLongDetected = true;
        } else {
          isLongDetected = false;
        }

        // Check if it was a short press and long press was not detected
        if (pressDuration < SHORT_PRESS_TIME && !isLongDetected) {
          item_selected = item_selected - 1;  // select previous item
          if (item_selected < 0) {            // if first item was selected, jump to last item
            item_selected = NUM_MENU_ITEMS - 1;
          }
        }
        ButtonPressProcessed = false;  // Reset the control variable when the button is not pressed anymore
      }

      PreviousButtonState = upButtonState;
    }
    if (BUTTON_DOWN_PIN != NULL) {
      //   if ((digitalRead(BUTTON_DOWN_PIN) == buttonVoltage) && (button_down_clicked == 0)) {  // down button clicked - jump to next menu item
      // item_selected = item_selected + 1;                                                  // select next item
      // button_down_clicked = 1;                                                            // set button to clicked to only perform the action once
      // if (item_selected >= NUM_MENU_ITEMS) {                                              // if last item was selected, jump to first item
      //   item_selected = 0;
      // }
      //   }

      //   if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 1)) {  // unclick
      //     button_down_clicked = 0;
      //   }

      int downButtonState = digitalRead(BUTTON_DOWN_PIN);  // Read the current button state

      if (downButtonState == HIGH && !ButtonPressProcessed) {
        if (PreviousButtonState == LOW) {
          pressedTime = millis();
          isPressing = true;
          isLongDetected = false;  // Ensure long press flag is reset each time button is pressed
          ButtonPressProcessed = true;
        }
      }

      if (isPressing && !isLongDetected) {
        long pressDuration = millis() - pressedTime;

        if (pressDuration > LONG_PRESS_TIME_MENU) {
          isLongDetected = true;
        }
      }
      if (isPressing && isLongDetected) {
        static unsigned long lastTempAdjustTime = 0;
        unsigned long currentMillis = millis();
        if (currentMillis - lastTempAdjustTime >= 200) {  // Check if 500ms have passed
          item_selected = item_selected + 1;              // select next item
          if (item_selected >= NUM_MENU_ITEMS) {          // if last item was selected, jump to first item
            item_selected = 0;
          }
          lastTempAdjustTime = currentMillis;  // Update the last adjustment time
        }
      }

      if (downButtonState == LOW && PreviousButtonState == HIGH) {
        isPressing = false;
        releasedTime = millis();

        long pressDuration = releasedTime - pressedTime;

        // Determine the type of press based on the duration
        if (pressDuration >= LONG_PRESS_TIME_MENU) {
          isLongDetected = true;
        } else {
          isLongDetected = false;
        }

        // Check if it was a short press and long press was not detected
        if (pressDuration < SHORT_PRESS_TIME && !isLongDetected) {
          item_selected = item_selected + 1;      // select next item
          if (item_selected >= NUM_MENU_ITEMS) {  // if last item was selected, jump to first item
            item_selected = 0;
          }
        }
        ButtonPressProcessed = false;  // Reset the control variable when the button is not pressed anymore
      }

      PreviousButtonState = downButtonState;
    }
  }
  if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage) {
    if (button_select_clicked == 0 && !long_press_handled) {
      select_button_press_time = millis();  // Start measuring button press time
      button_select_clicked = 1;
      long_press_handled = false;  // Reset the long press handled flag
    } else {
      // Button still pressed, check for long press
      if (current_screen == 1 && (millis() - select_button_press_time >= SELECT_BUTTON_LONG_PRESS_DURATION)) {
        if (!long_press_handled) {  // Check if long press action has been handled
          current_screen = 0;
          long_press_handled = true;  // Set the long press handled flag
          upButtonState = !buttonVoltage;
        }
      }
    }
  } else if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    if (button_select_clicked == 1) {
      if (current_screen == 0 && !long_press_handled) {
        current_screen = 1;
      }
      button_select_clicked = 0;   // Reset the button state
      long_press_handled = false;  // Reset the long press handled flag when the button is released
    }
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
  if (current_screen == 1) {  // MENU SCREEN // up and down buttons only work for the menu screen
                              // if ((digitalRead(BUTTON_UP_PIN) == buttonVoltage) && (button_up_clicked == 0)) {  // up button clicked - jump to previous menu item
                              //   item_selected_submenu = item_selected_submenu - 1;                              // select previous item
                              //   button_up_clicked = 1;                                                          // set button to clicked to only perform the action once
                              //   if (item_selected_submenu < 0) {                                                // if first item was selected, jump to last item
                              //     item_selected_submenu = NUM_SUBMENU_ITEMS - 1;
                              //   }
                              // }

    // if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 1)) {  // unclick
    //   button_up_clicked = 0;
    // }


    // if ((digitalRead(BUTTON_DOWN_PIN) == buttonVoltage) && (button_down_clicked == 0)) {  // down button clicked - jump to next menu item
    // item_selected_submenu = item_selected_submenu + 1;                                  // select next item
    // button_down_clicked = 1;                                                            // set button to clicked to only perform the action once
    // if (item_selected_submenu >= NUM_SUBMENU_ITEMS) {                                   // if last item was selected, jump to first item
    //   item_selected_submenu = 0;
    // }
    // }

    // if ((digitalRead(BUTTON_DOWN_PIN) == LOW) && (button_down_clicked == 1)) {  // unclick
    //   button_down_clicked = 0;
    // }
    if (BUTTON_UP_PIN != NULL) {

      upButtonState = digitalRead(BUTTON_UP_PIN);  // Read the current button state

      if (upButtonState == HIGH && !ButtonPressProcessed) {
        if (PreviousButtonState == LOW) {
          pressedTime = millis();
          isPressing = true;
          isLongDetected = false;  // Ensure long press flag is reset each time button is pressed
          ButtonPressProcessed = true;
        }
      }

      if (isPressing && !isLongDetected) {
        long pressDuration = millis() - pressedTime;

        if (pressDuration > LONG_PRESS_TIME_MENU) {
          isLongDetected = true;
        }
      }
      if (isPressing && isLongDetected) {
        static unsigned long lastTempAdjustTime = 0;
        unsigned long currentMillis = millis();
        if (currentMillis - lastTempAdjustTime >= 200) {      // Check if 500ms have passed
          item_selected_submenu = item_selected_submenu - 1;  // select previous item
          if (item_selected_submenu < 0) {                    // if first item was selected, jump to last item
            item_selected_submenu = NUM_SUBMENU_ITEMS - 1;
          }
          lastTempAdjustTime = currentMillis;  // Update the last adjustment time
        }
      }

      if (upButtonState == LOW && PreviousButtonState == HIGH) {
        isPressing = false;
        releasedTime = millis();

        long pressDuration = releasedTime - pressedTime;

        // Determine the type of press based on the duration
        if (pressDuration >= LONG_PRESS_TIME_MENU) {
          isLongDetected = true;
        } else {
          isLongDetected = false;
        }

        // Check if it was a short press and long press was not detected
        if (pressDuration < SHORT_PRESS_TIME && !isLongDetected) {
          item_selected_submenu = item_selected_submenu - 1;  // select previous item
          if (item_selected_submenu < 0) {                    // if first item was selected, jump to last item
            item_selected_submenu = NUM_SUBMENU_ITEMS - 1;
          }
        }
        ButtonPressProcessed = false;  // Reset the control variable when the button is not pressed anymore
      }

      PreviousButtonState = upButtonState;
    }
    if (BUTTON_DOWN_PIN != NULL) {

      int downButtonState = digitalRead(BUTTON_DOWN_PIN);  // Read the current button state

      if (downButtonState == HIGH && !ButtonPressProcessed) {
        if (PreviousButtonState == LOW) {
          pressedTime = millis();
          isPressing = true;
          isLongDetected = false;  // Ensure long press flag is reset each time button is pressed
          ButtonPressProcessed = true;
        }
      }

      if (isPressing && !isLongDetected) {
        long pressDuration = millis() - pressedTime;

        if (pressDuration > LONG_PRESS_TIME_MENU) {
          isLongDetected = true;
        }
      }
      if (isPressing && isLongDetected) {
        static unsigned long lastTempAdjustTime = 0;
        unsigned long currentMillis = millis();
        if (currentMillis - lastTempAdjustTime >= 200) {      // Check if 500ms have passed
          item_selected_submenu = item_selected_submenu + 1;  // select next item
          if (item_selected_submenu >= NUM_SUBMENU_ITEMS) {   // if last item was selected, jump to first item
            item_selected_submenu = 0;
          }
          lastTempAdjustTime = currentMillis;  // Update the last adjustment time
        }
      }

      if (downButtonState == LOW && PreviousButtonState == HIGH) {
        isPressing = false;
        releasedTime = millis();

        long pressDuration = releasedTime - pressedTime;

        // Determine the type of press based on the duration
        if (pressDuration >= LONG_PRESS_TIME_MENU) {
          isLongDetected = true;
        } else {
          isLongDetected = false;
        }

        // Check if it was a short press and long press was not detected
        if (pressDuration < SHORT_PRESS_TIME && !isLongDetected) {
          item_selected_submenu = item_selected_submenu + 1;  // select next item
          if (item_selected_submenu >= NUM_SUBMENU_ITEMS) {   // if last item was selected, jump to first item
            item_selected_submenu = 0;
          }
        }
        ButtonPressProcessed = false;  // Reset the control variable when the button is not pressed anymore
      }

      PreviousButtonState = downButtonState;
    }
  }
  if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage) {
    if (button_select_clicked == 0 && !long_press_handled) {
      select_button_press_time = millis();  // Start measuring button press time
      button_select_clicked = 1;
      long_press_handled = false;  // Reset the long press handled flag
    } else {
      // Button still pressed, check for long press
      if (current_screen == 2 && (millis() - select_button_press_time >= SELECT_BUTTON_LONG_PRESS_DURATION)) {
        if (!long_press_handled) {  // Check if long press action has been handled
          current_screen = 1;
          long_press_handled = true;  // Set the long press handled flag
        }
      }
    }
  } else if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    if (button_select_clicked == 1) {
      if (current_screen == 1 && !long_press_handled) {
        current_screen = 2;
      }
      button_select_clicked = 0;   // Reset the button state
      long_press_handled = false;  // Reset the long press handled flag when the button is released
    }
  }

  // set correct values for the previous and next items
  item_sel_previous_submenu = item_selected_submenu - 1;
  if (item_sel_previous_submenu < 0) { item_sel_previous_submenu = NUM_SUBMENU_ITEMS - 1; }  // previous item would be below first = make it the last
  item_sel_next_submenu = item_selected_submenu + 1;
  if (item_sel_next_submenu >= NUM_SUBMENU_ITEMS) { item_sel_next_submenu = 0; }  // next item would be after last = make it the first
}


void OpenMenuOS::drawCanvasOnTFT() {
  canvas.pushSprite(0, 0);
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
int OpenMenuOS::getSelectedItemSubmenu() const {
  return item_selected_submenu;
}
int OpenMenuOS::getSelectedItemTileMenu() const {
  return item_selected_submenu;
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
