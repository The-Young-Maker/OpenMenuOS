/*
  OpenMenuOS.h - Library to create menu on color display.
  Created by Loic Daigle aka The Young Maker.
  Released into the public domain.
*/

#pragma message "The OpenMenuOS library is a work in progress. If you find any bug, please create an issue on the OpenMenuOS's Github repository"

#include "Arduino.h"
#include <TFT_eSPI.h>
#include "OpenMenuOS.h"
#include <string>

const GFXfont* menuFont = &FreeMono9pt7b;
const GFXfont* menuFontBold = &FreeMonoBold9pt7b;

ScreenManager screenManager;

ScreenConfig menuConfig;
ScreenConfig& Screen::config = menuConfig;

SettingSelectionScreen SettingSelectionScreen("SettingSelectionScreen");
int setting_index;

Setting* settings[MAX_SETTINGS_ITEMS];

// Button pin definitions
int buttonsMode;
int buttonVoltage;
int BUTTON_UP_PIN;      // Pin for UP button
int BUTTON_DOWN_PIN;    // Pin for DOWN button
int BUTTON_SELECT_PIN;  // Pin for SELECT button

int prevSelectState = !buttonVoltage;  // Global var for the previous state of the select button as otherwise the previous state was not correct when changing screen and caused problems

// Button press constants
static constexpr int SHORT_PRESS_TIME = 300;                   // 300 milliseconds
static constexpr int LONG_PRESS_TIME = 1000;                   // 1000 milliseconds
static constexpr int LONG_PRESS_TIME_MENU = 500;               // 500 milliseconds
static constexpr int SELECT_BUTTON_LONG_PRESS_DURATION = 300;  // 300 milliseconds

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite canvas = TFT_eSprite(&tft);


// Define global screen pointers
Screen* currentScreen = nullptr;


OpenMenuOS::OpenMenuOS(int btn_up, int btn_down, int btn_sel) {
  BUTTON_UP_PIN = btn_up;
  BUTTON_DOWN_PIN = btn_down;
  BUTTON_SELECT_PIN = btn_sel;
}

void OpenMenuOS::begin(int rotation, Screen* mainMenu) {  //  Display Rotation
  // Set up display
  tft.init();
  tft.setRotation(rotation);

  tftWidth = tft.width();
  tftHeight = tft.height();

  // Show The Boot image if bootImage is true and boot_image is not null
  if (bootImage && boot_image != nullptr) {
    tft.pushImage(0, 0, boot_image_width, boot_image_height, boot_image);  // To display the boot image
    delay(3000);                                                           // Delay for 3 seconds
  }

  tft.setTextWrap(false);
  canvas.setSwapBytes(true);
  canvas.createSprite(tftWidth, tftWidth);
  canvas.fillSprite(TFT_BLACK);

  // Set TFT_BL as OUTPUT and se it HIGH or LOW depending on the settings
  pinMode(TFT_BL, OUTPUT);

  // Set up button pins
  pinMode(BUTTON_UP_PIN, buttonsMode);

  pinMode(BUTTON_DOWN_PIN, buttonsMode);

  pinMode(BUTTON_SELECT_PIN, buttonsMode);
  screenManager.pushScreen(mainMenu);
}

void OpenMenuOS::loop() {
  if (currentScreen != nullptr) {
    currentScreen->handleInput();
  }
  drawCanvasOnTFT();  // Draw the updated canvas on the screen. You need to call it at the END of your code (in the end of "loop()")
}

SettingsScreen::SettingsScreen(const char* title)
  : title(title), totalSettings(0), currentSettingIndex(0) {
  tftWidth = tft.width();
  tftHeight = tft.height();
  initializeSettings();
  readFromEEPROM();
}

void SettingsScreen::addSetting(Setting* setting) {
  if (totalSettings < MAX_ITEMS) {
    settings[totalSettings++] = setting;
  }
}

void SettingsScreen::draw() {
  tftWidth = tft.width();
  tftHeight = tft.height();


  // Rectangle dimensions for menu selection
  uint16_t rect_width = config.scrollbar ? tftWidth * 0.97f : tftWidth;     // Width of the selection rectangle
  uint16_t rect_height = rect_height = tftHeight * config.itemHeightRatio;  // Height of the selection rectangle
  if (!config.scrollbar) {
    rect_width = tftWidth;
  } else {
    rect_width = tftWidth * 0.97f;  // Garder 3 % de marge pour le scrollbar
  }

  // Placer le rectangle de sélection verticalement centré
  uint16_t rect_x = 0;
  uint16_t rect_y = (tftHeight - rect_height) / 2;

  uint16_t selectedItemColor;

  canvas.fillRoundRect(rect_x + 1, rect_y - 25, rect_width - 3, rect_height - 3, 4, TFT_BLACK);  // Remove Old Text. Change it by setting old text's color to white? (May be slower and more complicated??)
  canvas.fillRoundRect(rect_x + 1, rect_y - 1, rect_width - 3, rect_height - 3, 4, TFT_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 27, rect_width - 3, rect_height - 3, 4, TFT_BLACK);

  switch (config.menuStyle) {
    case 0:
      if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage && config.buttonAnimation) {
        // canvas.drawRoundRect(rect_x + 1, rect_y + 1, rect_width - 2, rect_height - 1, 4, selectionBorderColor);  // Display the rectangle
        canvas.drawSmoothRoundRect(rect_x + 1, rect_y + 1, 4, 4, rect_width - 2, rect_height - 1, config.selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly

      } else {
        if (!config.scrollbar) {
          canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, config.selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
          canvas.drawFastVLine(tftWidth - 2, 29, 23, config.selectionBorderColor);                                            // Display the inside part
          canvas.drawFastVLine(tftWidth, 29, 22, config.selectionBorderColor);                                                // Display the Shadow
          canvas.drawFastHLine(2, 51, tftWidth - 3, config.selectionBorderColor);                                             // Display the inside part
          canvas.drawFastHLine(3, 51, tftWidth - 4, config.selectionBorderColor);                                             // Display the Shadow
        } else {
          canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width - 2, rect_height, config.selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly
          canvas.drawFastVLine(rect_width - 4, rect_y + 2, 23, config.selectionBorderColor);                                      // Display the inside part
          canvas.drawFastVLine(rect_width - 3, rect_y + 2, 22, config.selectionBorderColor);                                      // Display the Shadow
          canvas.drawFastHLine(2, 51, 149, config.selectionBorderColor);                                                          // Display the inside part
          canvas.drawFastHLine(3, 52, 148, config.selectionBorderColor);                                                          // Display the Shadow
        }
      }
      selectedItemColor = TFT_WHITE;
      break;
    case 1:
      // canvas.fillRoundRect(rect_x, rect_y, rect_width, rect_height, 4, selectionBorderColor);  // Display the rectangle
      canvas.fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 4, config.selectionBorderColor, TFT_BLACK);  // Display the rectangle || The "-2" should be determined dynamicaly

      selectedItemColor = TFT_BLACK;

      break;
  }

  // Calcul adaptatif des positions du texte et des icônes
  float textMarginX = tftWidth * config.marginRatioX;

  // Position X du texte dépendant de la présence d’icônes
  int xPos = textMarginX;
  int x1Pos = xPos;
  int x2Pos = xPos;

  int16_t fontHeight;
  // Calcul des positions Y pour les trois éléments de menu
  // (précedent, sélectionné et suivant) en se basant sur le rectangle de sélection
  int yPos = tftHeight / 3;   // Élément précédent
  int y1Pos = tftHeight / 2;  // Élément sélectionné (centré)
  int y2Pos = tftHeight;      // Élément suivant

  int itemNumber = 3;  // Number of shown item

  /////// Variables for the toggle buttons ///////
  int rectWidth = 41;
  int rectHeight = 20;
  int rectRadius = 10;

  int roundXOffset = 2;  // Horizontal offset for the round shape
  int roundDiameter = 15;
  int roundRadius = 10;

  uint16_t toggleSwitchHeight = tftHeight * config.toggleSwitchHeightRatio;
  uint16_t toggleswitchWidth = toggleSwitchHeight * 2;

  // int rectX = 114;
  int rectX = 110;

  uint16_t textWidth;
  uint16_t availableWidth;
  int scrollWindowSize = rect_width - ((textMarginX * 2) + toggleswitchWidth);

  ////////////////////////////////////////////////
  // draw previous item as icon + label
  String previousItem = getSettingName(item_selected_settings_previous);

  // Calculate available width for the text (define the appropriate width for your layout)
  availableWidth = scrollWindowSize;  // Adjust for the appropriate available width
  textWidth = canvas.textWidth(previousItem);

  // Dynamically adjust the text to fit the available space
  if (textWidth > availableWidth) {
    // If the text doesn't fit, shorten it and add "..."
    uint16_t maxLength = calculateMaxCharacters(previousItem.c_str(), availableWidth);
    previousItem = previousItem.substring(0, maxLength - 3) + "...";
  }

  canvas.setFreeFont(menuFont);
  canvas.setTextSize(1);
  fontHeight = canvas.fontHeight();
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);

  // Set the cursor position for the previous item
  canvas.setCursor(x2Pos, (tftHeight / itemNumber - fontHeight) / 2 + fontHeight + config.textShift);

  // Print the adjusted text
  canvas.println(previousItem);



  if (getSettingType(item_selected_settings_previous) == Setting::BOOLEAN) {
    int rectY = 2;

    // Calculate the vertical center of the rounded rectangle
    int rectCenterY = rectY + (rectHeight / 2);

    // Calculate the Y position for the round shape to be centered vertically
    int roundY = rectCenterY - (roundDiameter / 2);
    // Récupère la valeur actuelle depuis la classe
    bool currentValue;
    if (item_selected_settings_previous >= 0 && item_selected_settings_previous < totalSettings) {
      currentValue = settings[item_selected_settings_previous]->booleanValue;
    }
    drawToggleSwitch(rectX, rectY, currentValue);
  } else if (getSettingType(item_selected_settings_previous) == Setting::RANGE) {
    int rectY = (tftHeight / itemNumber) / 2;

    uint8_t currentValue = 0;

    if (item_selected_settings_previous >= 0 && item_selected_settings_previous < totalSettings) {
      currentValue = settings[item_selected_settings_previous]->rangeValue;
    }

    // Construire le texte à afficher
    String text = String(currentValue) + "%";

    // Définir l'origine du texte à l'extrémité droite
    canvas.setTextDatum(MR_DATUM);  // Top Right Datum

    // Calculer la position pour aligner le texte près du bord droit
    int rectX = canvas.width() - 9;  // Position horizontale (bord droit avec une marge de 2 pixels)

    // Afficher le texte aligné à droite
    canvas.drawString(text, rectX, rectY);

    // Réinitialiser pour éviter d'impacter d'autres textes
    canvas.setTextDatum(TL_DATUM);  // Retour à l'origine par défaut
  } else if (getSettingType(item_selected_settings_previous) == Setting::OPTION) {
    int rectY = (tftHeight / itemNumber) / 2;

    // Définir l'origine du texte à l'extrémité droite
    canvas.setTextDatum(MR_DATUM);  // Top Right Datum

    // Calculer la position pour aligner le texte près du bord droit
    int rectX = canvas.width() - 9;  // Position horizontale (bord droit avec une marge de 2 pixels)

    // Afficher le texte aligné à droite
    canvas.drawString(">", rectX, rectY);

    // Réinitialiser pour éviter d'impacter d'autres textes
    canvas.setTextDatum(TL_DATUM);  // Retour à l'origine par défaut
  }




  // draw selected item as icon + label in bold font
  canvas.setFreeFont(menuFontBold);
  canvas.setTextSize(1);
  fontHeight = canvas.fontHeight();

  String selectedItem = getSettingName(item_selected_settings);

  canvas.setTextColor(config.selectedItemColor, config.selectionFillColor);

  // Calculate the available width for the text
  availableWidth = scrollWindowSize;  // Adjust for the appropriate available width
  textWidth = canvas.textWidth(selectedItem);

  // Check if the text fits the available width
  if (textWidth > availableWidth && config.textScroll) {
    // If it doesn't fit, scroll the text horizontally
    scrollTextHorizontal(x1Pos, tftHeight / 2 + (fontHeight / 2) + config.textShift, selectedItem,
                         config.selectedItemColor, config.selectionFillColor, 1, 50, availableWidth);
  } else {
    canvas.setCursor(x1Pos, tftHeight / 2 + (fontHeight / 2) + config.textShift);

    // If the text doesn't fit, truncate it and add "..."
    if (textWidth > availableWidth) {
      uint16_t maxLength = calculateMaxCharacters(selectedItem.c_str(), availableWidth);
      selectedItem = selectedItem.substring(0, maxLength - 3) + "...";
    }

    // Print the adjusted text
    canvas.println(selectedItem);
  }


  if (getSettingType(item_selected_settings) == Setting::BOOLEAN) {
    int rectY = tftHeight / 2 - (toggleSwitchHeight / 2);

    // Calculate the vertical center of the rounded rectangle
    int rectCenterY = rectY + (rectHeight / 2);

    // Calculate the Y position for the round shape to be centered vertically
    int roundY = rectCenterY - (roundDiameter / 2);
    bool currentValue;
    if (item_selected_settings >= 0 && item_selected_settings < totalSettings) {
      currentValue = settings[item_selected_settings]->booleanValue;
    }
    drawToggleSwitch(rectX, rectY, currentValue, config.selectionBorderColor);
  } else if (getSettingType(item_selected_settings) == Setting::RANGE) {
    int rectY = tftHeight / 2;

    uint8_t currentValue = 0;

    if (item_selected_settings >= 0 && item_selected_settings < totalSettings) {
      currentValue = settings[item_selected_settings]->rangeValue;
    }

    // Construire le texte à afficher
    String text = String(currentValue) + "%";

    // Définir l'origine du texte à l'extrémité droite
    canvas.setTextDatum(MR_DATUM);  // Top Right Datum

    // Calculer la position pour aligner le texte près du bord droit
    int rectX = canvas.width() - 9;  // Position horizontale (bord droit avec une marge de 2 pixels)

    // Afficher le texte aligné à droite
    canvas.drawString(text, rectX, rectY);

    // Réinitialiser pour éviter d'impacter d'autres textes
    canvas.setTextDatum(TL_DATUM);  // Retour à l'origine par défaut
  } else if (getSettingType(item_selected_settings) == Setting::OPTION) {
    int rectY = tftHeight / 2;

    // Définir l'origine du texte à l'extrémité droite
    canvas.setTextDatum(MR_DATUM);  // Top Right Datum

    // Calculer la position pour aligner le texte près du bord droit
    int rectX = canvas.width() - 9;  // Position horizontale (bord droit avec une marge de 2 pixels)

    // Afficher le texte aligné à droite
    canvas.drawString(">", rectX, rectY);

    // Réinitialiser pour éviter d'impacter d'autres textes
    canvas.setTextDatum(TL_DATUM);  // Retour à l'origine par défaut
  }




  // draw next item as icon + label
  String nextItem = getSettingName(item_selected_settings_next);

  // Calculate the available width for the text
  availableWidth = scrollWindowSize;  // Adjust for the appropriate available width
  textWidth = canvas.textWidth(nextItem);

  // Check if the text fits the available width
  canvas.setFreeFont(menuFont);
  canvas.setTextSize(1);
  fontHeight = canvas.fontHeight();
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(x2Pos, tftHeight - (tftHeight / itemNumber) + (tftHeight / itemNumber - fontHeight) / 2 + fontHeight + config.textShift);

  // If the text doesn't fit, truncate it and add "..."
  if (textWidth > availableWidth) {
    uint16_t maxLength = calculateMaxCharacters(nextItem.c_str(), availableWidth);
    nextItem = nextItem.substring(0, maxLength - 3) + "...";
  }

  // Print the adjusted text
  canvas.println(nextItem);


  if (getSettingType(item_selected_settings_next) == Setting::BOOLEAN) {
    int rectY = 55;

    // Calculate the vertical center of the rounded rectangle
    int rectCenterY = rectY + (rectHeight / 2);

    // Calculate the Y position for the round shape to be centered vertically
    int roundY = rectCenterY - (roundDiameter / 2);
    bool currentValue;
    if (item_selected_settings_next >= 0 && item_selected_settings_next < totalSettings) {
      currentValue = settings[item_selected_settings_next]->booleanValue;
    }

    drawToggleSwitch(rectX, rectY, currentValue);
  } else if (getSettingType(item_selected_settings_next) == Setting::RANGE) {
    int rectY = tftHeight - (tftHeight / itemNumber) / 2;

    uint8_t currentValue = 0;

    if (item_selected_settings_next >= 0 && item_selected_settings_next < totalSettings) {
      currentValue = settings[item_selected_settings_next]->rangeValue;
    }

    // Construire le texte à afficher
    String text = String(currentValue) + "%";

    // Définir l'origine du texte à l'extrémité droite
    canvas.setTextDatum(MR_DATUM);  // Top Right Datum

    // Calculer la position pour aligner le texte près du bord droit
    int rectX = canvas.width() - 9;  // Position horizontale (bord droit avec une marge de 2 pixels)

    // Afficher le texte aligné à droite
    canvas.drawString(text, rectX, rectY);

    // Réinitialiser pour éviter d'impacter d'autres textes
    canvas.setTextDatum(TL_DATUM);  // Retour à l'origine par défaut
  } else if (getSettingType(item_selected_settings_next) == Setting::OPTION) {
    int rectY = tftHeight - (tftHeight / itemNumber) / 2;

    // Définir l'origine du texte à l'extrémité droite
    canvas.setTextDatum(MR_DATUM);  // Top Right Datum

    // Calculer la position pour aligner le texte près du bord droit
    int rectX = canvas.width() - 9;  // Position horizontale (bord droit avec une marge de 2 pixels)

    // Afficher le texte aligné à droite
    canvas.drawString(">", rectX, rectY);

    // Réinitialiser pour éviter d'impacter d'autres textes
    canvas.setTextDatum(TL_DATUM);  // Retour à l'origine par défaut
  }

  if (config.scrollbar) {
    // Draw the scrollbar
    drawScrollbar(item_selected_settings, item_selected_settings_next, totalSettings);
  }
}

void SettingsScreen::handleInput() {
  // Handle UP button
  if (BUTTON_UP_PIN != NULL) {
    // Read current button state
    int currentUpButtonState = digitalRead(BUTTON_UP_PIN);
    static int previousUpButtonState = !buttonVoltage;
    static unsigned long upPressedTime = 0;
    static bool upIsPressing = false;
    static bool upIsLongDetected = false;
    static bool upButtonPressProcessed = false;
    static unsigned long lastUpScrollTime = 0;

    // Button press detection
    if (currentUpButtonState == buttonVoltage && !upButtonPressProcessed) {
      if (previousUpButtonState == !buttonVoltage) {
        // Button just pressed
        upPressedTime = millis();
        upIsPressing = true;
        upIsLongDetected = false;
        upButtonPressProcessed = true;
      }
    }

    // Long press detection
    if (upIsPressing && !upIsLongDetected) {
      long pressDuration = millis() - upPressedTime;
      if (pressDuration > LONG_PRESS_TIME_MENU) {
        upIsLongDetected = true;
      }
    }

    // Continuous scrolling on long press
    if (upIsPressing && upIsLongDetected) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastUpScrollTime >= 200) {  // Scroll every 200ms
        item_selected_settings--;
        if (item_selected_settings < 0) {
          item_selected_settings = totalSettings - 1;
        }
        lastUpScrollTime = currentMillis;
      }
    }

    // Button release handling
    if (currentUpButtonState == !buttonVoltage && previousUpButtonState == buttonVoltage) {
      upIsPressing = false;
      unsigned long releasedTime = millis();
      long pressDuration = releasedTime - upPressedTime;

      // Short press action
      if (pressDuration < SHORT_PRESS_TIME && !upIsLongDetected) {
        item_selected_settings--;
        if (item_selected_settings < 0) {
          item_selected_settings = totalSettings - 1;
        }
      }
      upButtonPressProcessed = false;
    }

    previousUpButtonState = currentUpButtonState;
  }

  // Handle DOWN button
  if (BUTTON_DOWN_PIN != NULL) {
    // Read current button state
    int currentDownButtonState = digitalRead(BUTTON_DOWN_PIN);
    static int previousDownButtonState = !buttonVoltage;
    static unsigned long downPressedTime = 0;
    static bool downIsPressing = false;
    static bool downIsLongDetected = false;
    static bool downButtonPressProcessed = false;
    static unsigned long lastDownScrollTime = 0;

    // Button press detection
    if (currentDownButtonState == buttonVoltage && !downButtonPressProcessed) {
      if (previousDownButtonState == !buttonVoltage) {
        // Button just pressed
        downPressedTime = millis();
        downIsPressing = true;
        downIsLongDetected = false;
        downButtonPressProcessed = true;
      }
    }

    // Long press detection
    if (downIsPressing && !downIsLongDetected) {
      long pressDuration = millis() - downPressedTime;
      if (pressDuration > LONG_PRESS_TIME_MENU) {
        downIsLongDetected = true;
      }
    }

    // Continuous scrolling on long press
    if (downIsPressing && downIsLongDetected) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastDownScrollTime >= 200) {  // Scroll every 200ms
        item_selected_settings++;
        if (item_selected_settings >= totalSettings) {
          item_selected_settings = 0;
        }
        lastDownScrollTime = currentMillis;
      }
    }

    // Button release handling
    if (currentDownButtonState == !buttonVoltage && previousDownButtonState == buttonVoltage) {
      downIsPressing = false;
      unsigned long releasedTime = millis();
      long pressDuration = releasedTime - downPressedTime;

      // Short press action
      if (pressDuration < SHORT_PRESS_TIME && !downIsLongDetected) {
        item_selected_settings++;
        if (item_selected_settings >= totalSettings) {
          item_selected_settings = 0;
        }
      }
      downButtonPressProcessed = false;
    }

    previousDownButtonState = currentDownButtonState;
  }

  // Calculate previous and next items for display purposes
  item_selected_settings_previous = item_selected_settings - 1;
  if (item_selected_settings_previous < 0) {
    item_selected_settings_previous = totalSettings - 1;
  }

  item_selected_settings_next = item_selected_settings + 1;
  if (item_selected_settings_next >= totalSettings) {
    item_selected_settings_next = 0;
  }

  // Handle SELECT button
  int selectButtonState = digitalRead(BUTTON_SELECT_PIN);
  static unsigned long selectPressedTime = 0;
  static bool selectIsPressing = false;
  static bool selectIsLongDetected = false;
  static bool selectButtonPressProcessed = false;

  // Button press detection
  if (selectButtonState == buttonVoltage && !selectButtonPressProcessed) {
    if (prevSelectState == !buttonVoltage) {
      selectPressedTime = millis();
      selectIsPressing = true;
      selectIsLongDetected = false;
      selectButtonPressProcessed = true;
    }
  }

  // Long press detection
  if (selectIsPressing && !selectIsLongDetected) {
    long pressDuration = millis() - selectPressedTime;
    if (pressDuration > SELECT_BUTTON_LONG_PRESS_DURATION) {
      selectIsLongDetected = true;

      // Handle long press action
      if (screenManager.canGoBack()) {
        screenManager.popScreen();  // Go back to the previous screen
      }
    }
  }

  // Button release handling
  if (selectButtonState == !buttonVoltage && prevSelectState == buttonVoltage) {
    selectIsPressing = false;
    unsigned long releasedTime = millis();
    long pressDuration = releasedTime - selectPressedTime;

    // Short press action
    if (pressDuration < SHORT_PRESS_TIME && !selectIsLongDetected) {
      if (item_selected_settings >= 0 && item_selected_settings < totalSettings) {
        setting_index = item_selected_settings;
        if (settings[setting_index]->type == Setting::BOOLEAN) {
          modify(1, item_selected_settings);  // Modify setting value (+1)
        } else {
          screenManager.pushScreen(&SettingSelectionScreen);
        }
        draw();  // Redraw after changing the setting
      }
    }
    selectButtonPressProcessed = false;
  }

  prevSelectState = selectButtonState;

  // Update the display to reflect any changes
  draw();
}
void SettingsScreen::drawToggleSwitch(int16_t x, int16_t y, bool state, uint16_t bgColor) {
  uint16_t toggleSwitchHeight = tftHeight * config.toggleSwitchHeightRatio;
  uint16_t switchWidth = toggleSwitchHeight * 2;
  uint16_t knobDiameter = toggleSwitchHeight - 4;

  uint16_t knobBgColor = state ? TFT_GREEN : TFT_RED;
  uint16_t knobColor = TFT_WHITE;

  // Draw switch background
  canvas.fillSmoothRoundRect(x, y, switchWidth, toggleSwitchHeight, toggleSwitchHeight / 2, knobBgColor, bgColor);

  // Draw knob
  int16_t knobX = state ? x + switchWidth - knobDiameter - 2 : x + 2;
  canvas.fillSmoothCircle(knobX + knobDiameter / 2, y + toggleSwitchHeight / 2, knobDiameter / 2, knobColor, knobBgColor);
}

// Constructor for MenuScreen – also sets default drawing configuration
MenuScreen::MenuScreen(const char* title)
  : title(title), itemCount(0), currentItemIndex(0) {
  // Default configuration; adjust these values as needed
  config.showImages = false;
  config.scrollbar = false;
  config.selectionBorderColor = TFT_WHITE;  // Assumes TFT_WHITE is defined
  config.selectionFillColor = TFT_BLACK;    // Assumes TFT_BLACK is defined
  config.buttonAnimation = false;
  config.menuStyle = 0;
  config.textScroll = true;
}

// Add a menu item
void MenuScreen::addItem(const char* label, Screen* nextScreen, ActionCallback action, const uint16_t* image) {
  if (itemCount < MAX_ITEMS) {
    items[itemCount].label = label;
    items[itemCount].nextScreen = nextScreen;
    items[itemCount].action = action;
    items[itemCount].image = image;
    itemCount++;
  }
}

int MenuScreen::getIndex() {
  return currentItemIndex;
}

// Advanced draw() function used by every MenuScreen instance
void MenuScreen::draw() {
  // Update display dimensions
  uint16_t tftWidth = tft.width();
  uint16_t tftHeight = tft.height();

  // Calculate adaptive selection rectangle dimensions
  uint16_t rect_height = tftHeight * config.itemHeightRatio;
  uint16_t rect_width = tftWidth * 0.97f;


  // Center the selection rectangle vertically
  uint16_t rect_x = 0;
  uint16_t rect_y = (tftHeight - rect_height) / 2;

  // Check for button press (implementation provided elsewhere)
  // checkForButtonPress();

  // Clear areas to remove previous text by drawing rounded rectangles
  float clearMargin = tftHeight * config.marginRatioY;
  canvas.fillRoundRect(rect_x + 1, rect_y - rect_height - clearMargin,
                       rect_width - 3, rect_height - 3, 4, TFT_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y - 1,
                       rect_width - 3, rect_height - 3, 4, TFT_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + rect_height + clearMargin,
                       rect_width - 3, rect_height - 3, 4, TFT_BLACK);

  uint16_t selectedItemColor;

  // Draw selection rectangle based on style and button state
  if (config.menuStyle == 0) {
    if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage && config.buttonAnimation) {
      canvas.drawSmoothRoundRect(rect_x + 1, rect_y + 1, 4, 4,
                                 rect_width - 2, rect_height - 1, config.selectionBorderColor, TFT_BLACK);
    } else {
      if (!config.scrollbar) {
        canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, config.selectionBorderColor, TFT_BLACK);
        uint16_t lineLength = rect_height * 0.9f;
        canvas.drawFastVLine(tftWidth - 2, rect_y + tftHeight * 0.05f, lineLength, config.selectionBorderColor);
        canvas.drawFastVLine(tftWidth, rect_y + tftHeight * 0.05f, lineLength - 1, config.selectionBorderColor);
        canvas.drawFastHLine(2, rect_y + rect_height, rect_width - 3, config.selectionBorderColor);
        canvas.drawFastHLine(3, rect_y + rect_height, rect_width - 4, config.selectionBorderColor);
      } else {
        canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width - 2, rect_height, config.selectionBorderColor, TFT_BLACK);
        uint16_t lineLength = rect_height * 0.9f;
        canvas.drawFastVLine(rect_width - 4, rect_y + 2, lineLength, config.selectionBorderColor);
        canvas.drawFastVLine(rect_width - 3, rect_y + 2, lineLength - 1, config.selectionBorderColor);
        canvas.drawFastHLine(2, rect_y + rect_height, rect_width * 0.95f, config.selectionBorderColor);
        canvas.drawFastHLine(3, rect_y + rect_height + 1, rect_width * 0.95f - 1, config.selectionBorderColor);
      }
    }
    config.selectedItemColor = TFT_WHITE;
  } else if (config.menuStyle == 1) {
    canvas.fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 4, config.selectionBorderColor, TFT_BLACK);
    config.selectedItemColor = TFT_BLACK;
  }

  // Calculate adaptive positions for text and (optionally) icons
  float textMarginX = tftWidth * config.marginRatioX;
  float iconSize = 16;  // Icon size based on screen height

  // Calculate indices for previous, current, and next items
  int item_sel_previous = (currentItemIndex - 1 + itemCount) % itemCount;
  int item_selected = currentItemIndex;
  int item_sel_next = (currentItemIndex + 1) % itemCount;

  int xPos = items[item_sel_previous].image != nullptr ? textMarginX + iconSize : textMarginX;
  int x1Pos = items[item_selected].image != nullptr ? textMarginX + iconSize : textMarginX;
  int x2Pos = items[item_sel_next].image != nullptr ? textMarginX + iconSize : textMarginX;

  int16_t fontHeight;

  // Y positions for the three displayed items (previous, selected, next)
  int itemNumber = 3;
  int yPos = tftHeight / 3;   // previous
  int y1Pos = tftHeight / 2;  // selected (centered)
  int y2Pos = tftHeight;      // next

  int scrollWindowSize = rect_width - (textMarginX * 2);
  int scrollWindowSizeImage = rect_width - ((textMarginX * 2) + iconSize);

  uint16_t textWidth;
  uint16_t availableWidth;

  // --- Draw previous item ---
  canvas.setFreeFont(menuFont);
  canvas.setTextSize(1);
  fontHeight = canvas.fontHeight();
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);

  // Calculate available width for the text
  textWidth = canvas.textWidth(items[item_sel_previous].label);
  availableWidth = items[item_sel_previous].image != nullptr ? scrollWindowSizeImage : scrollWindowSize;

  // Dynamically adjust the text to fit the available space
  String previousItem = String(items[item_sel_previous].label);
  if (textWidth > availableWidth) {
    // If the text doesn't fit, shorten it and add "..."
    uint16_t maxLength = calculateMaxCharacters(previousItem.c_str(), availableWidth);
    previousItem = previousItem.substring(0, maxLength - 3) + "...";
  }

  // Set the cursor position for the previous item
  canvas.setCursor(xPos, (tftHeight / itemNumber - fontHeight) / 2 + fontHeight + config.textShift);

  // Print the adjusted text
  canvas.println(previousItem);

  // Draw previous icon if present
  if (items[item_sel_previous].image != nullptr) {
    float iconPosY = ((tftHeight / itemNumber) - iconSize) / 2;
    canvas.pushImage(textMarginX / 2, iconPosY, iconSize, iconSize, (uint16_t*)items[item_sel_previous].image);  // Example position for the image
  }

  // --- Draw selected item ---
  // Set the font and text size once (don't repeat in the loop)
  canvas.setFreeFont(menuFontBold);
  canvas.setTextSize(1);
  fontHeight = canvas.fontHeight();

  textWidth = canvas.textWidth(items[item_selected].label);
  availableWidth = items[item_selected].image != nullptr ? scrollWindowSizeImage : scrollWindowSize;

  // Check if the text needs to scroll
  if (strlen(items[item_selected].label) > calculateMaxCharacters(items[item_selected].label, availableWidth) && config.textScroll) {
    // Call the scroll function if the text is too long
    scrollTextHorizontal(x1Pos, tftHeight / 2 + (fontHeight / 2) + config.textShift,
                         items[item_selected].label,
                         config.selectedItemColor, config.selectionFillColor, 1, 50, availableWidth);
  } else {
    // Set the text color and cursor for static text
    canvas.setTextColor(config.selectedItemColor, config.selectionFillColor);
    canvas.setCursor(x1Pos, tftHeight / 2 + (fontHeight / 2) + config.textShift);

    // Prepare the string to display
    String selectedItem = String(items[item_selected].label);
    if (textWidth > availableWidth) {
      // If the text doesn't fit, shorten it and add "..."
      uint16_t maxLength = calculateMaxCharacters(previousItem.c_str(), availableWidth);
      selectedItem = previousItem.substring(0, maxLength - 3) + "...";
    }


    // Print the string
    canvas.println(selectedItem);
  }

  // Draw selected icon if present
  if (items[item_selected].image != nullptr) {
    float iconPosY = (tftHeight - iconSize) / 2;
    canvas.pushImage(textMarginX / 2, iconPosY, iconSize, iconSize, (uint16_t*)items[item_selected].image);  // Example position for the image
  }

  // --- Draw next item ---
  canvas.setFreeFont(menuFont);
  canvas.setTextSize(1);
  fontHeight = canvas.fontHeight();
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);

  // Calculate available width for the text
  textWidth = canvas.textWidth(items[item_sel_next].label);
  availableWidth = items[item_sel_next].image != nullptr ? scrollWindowSizeImage : scrollWindowSize;

  // Dynamically adjust the text to fit the available space
  String nextItem = String(items[item_sel_next].label);
  if (textWidth > availableWidth) {
    // If the text doesn't fit, shorten it and add "..."
    uint16_t maxLength = calculateMaxCharacters(nextItem.c_str(), availableWidth);
    nextItem = nextItem.substring(0, maxLength - 3) + "...";
  }

  // Set the cursor position for the next item
  canvas.setCursor(x2Pos, tftHeight - (tftHeight / itemNumber) + (tftHeight / itemNumber - fontHeight) / 2 + fontHeight + config.textShift);

  // Print the adjusted text
  canvas.println(nextItem);

  // Draw next icon if present
  if (items[item_sel_next].image != nullptr) {
    float iconPosY = tftHeight - ((tftHeight / itemNumber) + iconSize) / 2;
    canvas.pushImage(textMarginX / 2, iconPosY, iconSize, iconSize, (uint16_t*)items[item_sel_next].image);  // Example position for the image
  }

  // Draw the scrollbar if activated
  if (config.scrollbar) {
    drawScrollbar(item_selected, item_sel_next, itemCount);
  }
}
// Handle input remains similar to the previous implementation
void MenuScreen::handleInput() {
  // --- UP Button Processing ---
  if (BUTTON_UP_PIN != NULL) {
    static int prevUpState = !buttonVoltage;
    static bool isPressingUp = false;
    static bool isLongDetectedUp = false;
    static bool upProcessed = false;
    static unsigned long upPressedTime = 0;
    static unsigned long lastUpAdjustTime = 0;

    int upState = digitalRead(BUTTON_UP_PIN);

    // Detect press start
    if (upState == buttonVoltage && !upProcessed) {
      if (prevUpState == !buttonVoltage) {
        upPressedTime = millis();
        isPressingUp = true;
        isLongDetectedUp = false;
        upProcessed = true;
      }
    }
    // Check for long press while held
    if (isPressingUp && !isLongDetectedUp) {
      long pressDuration = millis() - upPressedTime;
      if (pressDuration > LONG_PRESS_TIME_MENU) {
        isLongDetectedUp = true;
      }
    }
    // While held with long press detected, update every 200 ms
    if (isPressingUp && isLongDetectedUp) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastUpAdjustTime >= 200) {
        currentItemIndex = (currentItemIndex - 1 + itemCount) % itemCount;
        lastUpAdjustTime = currentMillis;
      }
    }
    // On release, if short press then update once
    if (upState == !buttonVoltage && prevUpState == buttonVoltage) {
      isPressingUp = false;
      unsigned long upReleasedTime = millis();
      long pressDuration = upReleasedTime - upPressedTime;
      if (pressDuration < SHORT_PRESS_TIME && !isLongDetectedUp) {
        currentItemIndex = (currentItemIndex - 1 + itemCount) % itemCount;
      }
      upProcessed = false;
    }
    prevUpState = upState;
  }

  // --- DOWN Button Processing ---
  if (BUTTON_DOWN_PIN != NULL) {
    static int prevDownState = !buttonVoltage;
    static bool isPressingDown = false;
    static bool isLongDetectedDown = false;
    static bool downProcessed = false;
    static unsigned long downPressedTime = 0;
    static unsigned long lastDownAdjustTime = 0;

    int downState = digitalRead(BUTTON_DOWN_PIN);

    if (downState == buttonVoltage && !downProcessed) {
      if (prevDownState == !buttonVoltage) {
        downPressedTime = millis();
        isPressingDown = true;
        isLongDetectedDown = false;
        downProcessed = true;
      }
    }
    if (isPressingDown && !isLongDetectedDown) {
      long pressDuration = millis() - downPressedTime;
      if (pressDuration > LONG_PRESS_TIME_MENU) {
        isLongDetectedDown = true;
      }
    }
    if (isPressingDown && isLongDetectedDown) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastDownAdjustTime >= 200) {
        currentItemIndex = (currentItemIndex + 1) % itemCount;
        lastDownAdjustTime = currentMillis;
      }
    }
    if (downState == !buttonVoltage && prevDownState == buttonVoltage) {
      isPressingDown = false;
      unsigned long downReleasedTime = millis();
      long pressDuration = downReleasedTime - downPressedTime;
      if (pressDuration < SHORT_PRESS_TIME && !isLongDetectedDown) {
        currentItemIndex = (currentItemIndex + 1) % itemCount;
      }
      downProcessed = false;
    }
    prevDownState = downState;
  }

  // --- SELECT Button Processing ---
  static bool isPressingSelect = false;
  static bool isLongDetectedSelect = false;
  static bool selectProcessed = false;
  static unsigned long selectPressedTime = 0;

  int selectState = digitalRead(BUTTON_SELECT_PIN);

  // Detect press start
  if (selectState == buttonVoltage && !selectProcessed) {
    if (prevSelectState == !buttonVoltage) {
      selectPressedTime = millis();
      isPressingSelect = true;
      isLongDetectedSelect = false;
      selectProcessed = true;
    }
  }

  // Check for long press while held
  if (isPressingSelect && !isLongDetectedSelect) {
    long pressDuration = millis() - selectPressedTime;
    if (pressDuration > SELECT_BUTTON_LONG_PRESS_DURATION) {
      isLongDetectedSelect = true;

      // Long press action: go back to previous screen
      if (screenManager.canGoBack()) {
        screenManager.popScreen();
      }
    }
  }

  // On release
  if (selectState == !buttonVoltage && prevSelectState == buttonVoltage) {
    isPressingSelect = false;
    unsigned long selectReleasedTime = millis();
    long pressDuration = selectReleasedTime - selectPressedTime;

    // Short press: execute current menu item action only if long press wasn't detected
    if (pressDuration < SHORT_PRESS_TIME && !isLongDetectedSelect) {
      MenuItem& item = items[currentItemIndex];
      if (item.action != nullptr) {
        item.action();
      }

      // Only push to next screen if not already handled by long press
      if (item.nextScreen != nullptr) {
        screenManager.pushScreen(item.nextScreen);
      }
    }

    selectProcessed = false;
  }

  prevSelectState = selectState;

  // Update the display to reflect any changes.
  draw();
}
void CustomScreen::handleInput() {
  // --- SELECT Button Processing ---
  static bool selectClicked = false;
  static bool longPressHandled = false;
  static unsigned long selectPressTime = 0;

  if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage) {
    // Button pressed: record press start if not already clicked
    if (!selectClicked && !longPressHandled) {
      selectPressTime = millis();
      selectClicked = true;
    } else {
      // While still pressed, check for long press
      if ((millis() - selectPressTime >= SELECT_BUTTON_LONG_PRESS_DURATION) && !longPressHandled) {
        // Long press: go back to the last menu.
        if (screenManager.canGoBack()) {
          screenManager.popScreen();  // Go back to the previous screen
        }
        longPressHandled = true;
        prevSelectState = buttonVoltage;
      }
    }
  } else if (digitalRead(BUTTON_SELECT_PIN) == !buttonVoltage) {
    // On button release
    if (selectClicked) {
      // Reset select button state.
      selectClicked = false;
      longPressHandled = false;
    }
  }

  // Update the display to reflect any changes.
  draw();
}
void SettingSelectionScreen::draw() {
  tftWidth = tft.width();
  tftHeight = tft.height();


  // Ensure setting is not nullptr
  if (settings == nullptr) {
    Serial.println("Error: Null setting encountered!");
    return;  // Return early if setting is invalid
  }

  // Check if the setting has a valid name
  if (settings[setting_index]->name == nullptr) {
    Serial.println("Error: Setting name is null!");
    return;
  }

  String selectedItem = settings[setting_index]->name;

  // Set text font and color
  canvas.setFreeFont(menuFontBold);
  canvas.setTextSize(1);
  int fontHeight = canvas.fontHeight();
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);

  // Calculate available width for text
  int availableWidth = tftWidth;  // Adjust for the appropriate available width
  int textWidth = canvas.textWidth(selectedItem);

  // Check if the text fits the available width
  if (textWidth > availableWidth && config.textScroll) {
    // If it doesn't fit, scroll the text horizontally
    scrollTextHorizontal(5, 15, selectedItem,
                         TFT_WHITE, TFT_BLACK, 1, 50, availableWidth);
  } else {
    // If the text fits, print it normally or truncate if needed
    canvas.setCursor(5, 15);

    if (textWidth > availableWidth) {
      // Truncate the text and add "..." if it exceeds the available width
      uint16_t maxLength = calculateMaxCharacters(selectedItem.c_str(), availableWidth);
      selectedItem = selectedItem.substring(0, maxLength - 3) + "...";
    }

    canvas.print(selectedItem);  // Print text without automatically moving to a new line
  }

  int rectY = tftHeight / 2;
  // Handle different setting types
  switch (settings[setting_index]->type) {
    case Setting::RANGE:
      {
        // Draw the range value
        uint8_t currentValue = settings[setting_index]->rangeValue;

        // Construct the text to display
        String text = String(currentValue) + "%";

        // Set text alignment to right
        canvas.setTextDatum(MR_DATUM);
        int rectX = canvas.width() - 9;  // Align near the right margin

        // Draw the value
        canvas.drawString(text, rectX, rectY);

        // Reset text alignment
        canvas.setTextDatum(TL_DATUM);
        break;
      }

    case Setting::OPTION:
      {
        // Ensure options array is initialized
        if (settings[setting_index]->options == nullptr || settings[setting_index]->optionCount == 0) {
          Serial.println("Error: Options array is null or empty!");
          return;  // Exit early if options are invalid
        }

        // Display all options
        for (int i = 0; i < settings[setting_index]->optionCount; ++i) {
          // Ensure that each option is a valid string (not nullptr)
          if (settings[setting_index]->options[i] == nullptr) {
            Serial.print("Warning: Option ");
            Serial.print(i);
            Serial.println(" is null!");
            continue;  // Skip this option if it's null
          }


          const char* option = settings[setting_index]->options[i];

          // If it's the selected option, highlight it
          if (i == settings[setting_index]->optionIndex) {
            canvas.setTextDatum(MR_DATUM);
            int rectX = 10;  // Align near the right margin

            // Draw the value
            canvas.drawString(">", rectX, rectY);

            // Reset text alignment
            canvas.setTextDatum(TL_DATUM);
            canvas.setTextColor(TFT_BLUE);  // Highlight selected option
          } else {
            canvas.setTextColor(TFT_WHITE);  // Regular color for other options
          }

          // Display option text
          canvas.setCursor(10, rectY + (i * 20));  // Adjust Y for each option
          canvas.println(option);

          // Reset text color
          canvas.setTextColor(TFT_WHITE);
        }
        break;
      }

    case Setting::BOOLEAN:
      // Handle BOOLEAN case if necessary
      break;
  }
}

void SettingSelectionScreen::handleInput() {
  // --- UP Button Processing ---
  if (BUTTON_UP_PIN != NULL) {
    static int prevUpState = !buttonVoltage;
    static bool isPressingUp = false;
    static bool isLongDetectedUp = false;
    static bool upProcessed = false;
    static unsigned long upPressedTime = 0;
    static unsigned long lastUpAdjustTime = 0;

    int upState = digitalRead(BUTTON_UP_PIN);

    if (upState == buttonVoltage && !upProcessed) {
      if (prevUpState == !buttonVoltage) {
        upPressedTime = millis();
        isPressingUp = true;
        isLongDetectedUp = false;
        upProcessed = true;
      }
    }
    if (isPressingUp && !isLongDetectedUp) {
      long pressDuration = millis() - upPressedTime;
      if (pressDuration > LONG_PRESS_TIME_MENU) {
        isLongDetectedUp = true;
      }
    }
    if (isPressingUp && isLongDetectedUp) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastUpAdjustTime >= 200) {
        modify(1, setting_index);  // Increase setting value
        lastUpAdjustTime = currentMillis;
      }
    }
    if (upState == !buttonVoltage && prevUpState == buttonVoltage) {
      isPressingUp = false;
      unsigned long upReleasedTime = millis();
      long pressDuration = upReleasedTime - upPressedTime;
      if (pressDuration < SHORT_PRESS_TIME && !isLongDetectedUp) {
        modify(1, setting_index);  // Single increase for short press
      }
      upProcessed = false;
    }
    prevUpState = upState;
  }

  // --- DOWN Button Processing ---
  if (BUTTON_DOWN_PIN != NULL) {
    static int prevDownState = !buttonVoltage;
    static bool isPressingDown = false;
    static bool isLongDetectedDown = false;
    static bool downProcessed = false;
    static unsigned long downPressedTime = 0;
    static unsigned long lastDownAdjustTime = 0;

    int downState = digitalRead(BUTTON_DOWN_PIN);

    if (downState == buttonVoltage && !downProcessed) {
      if (prevDownState == !buttonVoltage) {
        downPressedTime = millis();
        isPressingDown = true;
        isLongDetectedDown = false;
        downProcessed = true;
      }
    }
    if (isPressingDown && !isLongDetectedDown) {
      long pressDuration = millis() - downPressedTime;
      if (pressDuration > LONG_PRESS_TIME_MENU) {
        isLongDetectedDown = true;
      }
    }
    if (isPressingDown && isLongDetectedDown) {
      unsigned long currentMillis = millis();
      if (currentMillis - lastDownAdjustTime >= 200) {
        modify(-1, setting_index);  // Decrease setting value
        lastDownAdjustTime = currentMillis;
      }
    }
    if (downState == !buttonVoltage && prevDownState == buttonVoltage) {
      isPressingDown = false;
      unsigned long downReleasedTime = millis();
      long pressDuration = downReleasedTime - downPressedTime;
      if (pressDuration < SHORT_PRESS_TIME && !isLongDetectedDown) {
        modify(-1, setting_index);  // Single decrease for short press
      }
      downProcessed = false;
    }
    prevDownState = downState;
  }

  // --- SELECT Button Processing ---
  static bool isPressingSelect = false;
  static bool isLongDetectedSelect = false;
  static bool selectProcessed = false;
  static unsigned long selectPressedTime = 0;

  int selectState = digitalRead(BUTTON_SELECT_PIN);

  if (selectState == buttonVoltage && !selectProcessed) {
    if (prevSelectState == !buttonVoltage) {
      selectPressedTime = millis();
      isPressingSelect = true;
      isLongDetectedSelect = false;
      selectProcessed = true;
    }
  }
  if (isPressingSelect && !isLongDetectedSelect) {
    long pressDuration = millis() - selectPressedTime;
    if (pressDuration > LONG_PRESS_TIME_MENU) {
      isLongDetectedSelect = true;
      if (screenManager.canGoBack()) {
        screenManager.popScreen();  // Long press to go back
      }
    }
  }
  if (selectState == !buttonVoltage && prevSelectState == buttonVoltage) {
    isPressingSelect = false;
    unsigned long selectReleasedTime = millis();
    long pressDuration = selectReleasedTime - selectPressedTime;
    if (pressDuration < SHORT_PRESS_TIME && !isLongDetectedSelect) {
    }
    selectProcessed = false;
  }
  prevSelectState = selectState;

  // Update the display to reflect any changes
  draw();
}


void OpenMenuOS::redirectToScreen(Screen* screen) {
  screenManager.pushScreen(screen);
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
  canvas.setFreeFont(menuFontBold);
  canvas.setTextSize(1);
  canvas.setTextColor(TFT_BLACK, TFT_WHITE);
  canvas.print(title);

  // Draw the message
  // if (messageAreaWidth > tftWidth - 8) {
  // tft.setFreeFont(menuFontBold);  // Here, We don't use "canvas." because when using it, the calculated value in the "scrollTextHorizontal" function is wrong but not with "tft." #bug
  canvas.setFreeFont(menuFontBold);
  scrollTextHorizontal(spaceBetweenPopup + 1, titleAreaY + 20, message, TFT_BLACK, TFT_WHITE, 1, 50, popupWidth - 2);
  // } else {
  //   // If the message fits within the screen width, draw it normally
  //   messageX = messageAreaX + (messageAreaWidth - messageWidth) / 2;
  //   messageY = messageAreaY + (messageAreaHeight - messageHeight) / 2;

  //   canvas.setCursor(messageX, messageY);
  //   canvas.setFreeFont(menuFont);
  //   canvas.setTextSize(1);
  //   canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  //   canvas.print(message);
  // }
}
void Screen::drawScrollbar(int selectedItem, int nextItem, int numItem) {
  // Avoid division by zero
  if (numItem == 0) {
    return;  // If numItem is zero, simply exit the function to avoid the division
  }

  // Draw scrollbar handle
  tftWidth = tft.width();
  tftHeight = tft.height();

  // Calculate box height based on number of items
  int boxHeight = tftHeight / numItem;
  int boxY = boxHeight * selectedItem;

  if (config.scrollbarStyle == 0) {
    // Clear previous scrollbar handle
    canvas.fillRect(tftWidth - 3, boxHeight * nextItem, 3, boxHeight, TFT_BLACK);
    // Draw new scrollbar handle
    canvas.fillRect(tftWidth - 3, boxY, 3, boxHeight, config.scrollbarColor);

    for (int y = 0; y < tftHeight; y++) {  // Display the Scrollbar
      if (y % 2 == 0) {
        canvas.drawPixel(tftWidth - 2, y, TFT_WHITE);
      }
    }
  } else if (config.scrollbarStyle == 1) {
    // Clear previous scrollbar handle
    canvas.fillRoundRect(tftWidth - 3, boxY, 3, boxHeight, TFT_BLACK, TFT_BLACK);
    // Draw new scrollbar handle
    canvas.fillSmoothRoundRect(tftWidth - 3, boxY, 3, boxHeight, 4, config.scrollbarColor, TFT_BLACK);  // Display the rectangle || The "-3" should be determined dynamically
  }
}

void OpenMenuOS::scrollTextHorizontal(int16_t x, int16_t y, const char* text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize) {
  static int16_t xPos = x;
  static unsigned long previousMillis = 0;
  static String currentText = "";
  windowSize = windowSize + 25;
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
  canvas.setFreeFont(menuFontBold);

  tempSprite.createSprite(windowSize, canvas.fontHeight() + 2);
  canvas.setSwapBytes(false);
  tempSprite.fillSprite(TFT_TRANSPARENT);

  tempSprite.setFreeFont(menuFontBold);
  tempSprite.setTextSize(textSize);
  tempSprite.setTextColor(textColor, bgColor);

  // Draw text on the temporary sprite
  int16_t yPos = tempSprite.fontHeight() - 4;
  tempSprite.setCursor(xPos - x, yPos);
  tempSprite.print(text);
  tempSprite.fillRect(windowSize - 25, 0, 25, tempSprite.height(), TFT_TRANSPARENT);
  // Push the temporary sprite to the main canvas
  tempSprite.pushToSprite(&canvas, x, y - yPos, TFT_TRANSPARENT);
  tempSprite.deleteSprite();
  canvas.setSwapBytes(true);
}
// Overloaded function for String input
void OpenMenuOS::scrollTextHorizontal(int16_t x, int16_t y, const String& text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize) {
  scrollTextHorizontal(x, y, text.c_str(), textColor, bgColor, textSize, delayTime, windowSize);
}

void Screen::scrollTextHorizontal(int16_t x, int16_t y, const char* text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize) {
  static int16_t xPos = x;
  static unsigned long previousMillis = 0;
  static String currentText = "";
  windowSize = windowSize + 25;
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
  canvas.setFreeFont(menuFontBold);

  tempSprite.createSprite(windowSize, canvas.fontHeight() + 2);
  canvas.setSwapBytes(false);
  tempSprite.fillSprite(TFT_TRANSPARENT);

  tempSprite.setFreeFont(menuFontBold);
  tempSprite.setTextSize(textSize);
  tempSprite.setTextColor(textColor, bgColor);

  // Draw text on the temporary sprite
  int16_t yPos = tempSprite.fontHeight() - 4;
  tempSprite.setCursor(xPos - x, yPos);
  tempSprite.print(text);
  tempSprite.fillRect(windowSize - 25, 0, 25, tempSprite.height(), TFT_TRANSPARENT);
  // Push the temporary sprite to the main canvas
  tempSprite.pushToSprite(&canvas, x, y - yPos, TFT_TRANSPARENT);
  tempSprite.deleteSprite();
  canvas.setSwapBytes(true);
}

// Overloaded function for String input
void Screen::scrollTextHorizontal(int16_t x, int16_t y, const String& text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize) {
  scrollTextHorizontal(x, y, text.c_str(), textColor, bgColor, textSize, delayTime, windowSize);
}
int Screen::calculateMaxCharacters(const char* text, uint16_t windowSize) {
  int numCharacters = strlen(text);  // Get the length of the text
  char tempText[numCharacters + 1];  // Temporary buffer to hold the shortened text
  strcpy(tempText, text);            // Copy the original text to the buffer

  // Check if the text fits within the window
  while (canvas.textWidth(tempText) > windowSize && numCharacters > 0) {
    tempText[--numCharacters] = '\0';  // Remove the last character
  }

  // Return the number of characters that fit
  return numCharacters;
}

void OpenMenuOS::setTextScroll(bool x = true) {
  MenuScreen::config.textScroll = x;
}
void OpenMenuOS::showBootImage(bool x = true) {
  bootImage = x;
}
// Function to set the global image pointer and dimensions
void OpenMenuOS::setBootImage(uint16_t* Boot_img, uint16_t height, uint16_t width) {
  // Set the global variables to the provided values
  boot_image = Boot_img;
  boot_image_width = width;
  boot_image_height = height;
}
void OpenMenuOS::setButtonAnimation(bool x = true) {
  MenuScreen::config.buttonAnimation = x;
}
void OpenMenuOS::setMenuStyle(int style) {
  MenuScreen::config.menuStyle = style;
}
void OpenMenuOS::setScrollbar(bool x = true) {
  MenuScreen::config.scrollbar = x;
}
void OpenMenuOS::setScrollbarColor(uint16_t color = TFT_WHITE) {
  MenuScreen::config.scrollbarColor = color;
}
void OpenMenuOS::setScrollbarStyle(int style) {
  MenuScreen::config.scrollbarStyle = style;
}
void OpenMenuOS::setSelectionBorderColor(uint16_t color = TFT_WHITE) {
  MenuScreen::config.selectionBorderColor = color;
}
void OpenMenuOS::setSelectionFillColor(uint16_t color = TFT_BLACK) {
  MenuScreen::config.selectionFillColor = color;
}
void OpenMenuOS::setMenuFont(const GFXfont* font) {
  menuFont = font;
}

void OpenMenuOS::setMenuFontBold(const GFXfont* font) {
  menuFontBold = font;
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


void OpenMenuOS::drawCanvasOnTFT() {
  canvas.pushSprite(0, 0);
  canvas.fillSprite(TFT_BLACK);
}

// Save settings to non-volatile storage
void SettingsScreen::saveToEEPROM() {
  for (int i = 0; i < totalSettings; i++) {
    Setting* s = settings[i];
    if (s == nullptr) continue;  // Skip empty settings

    // Save based on the type of setting
    switch (s->type) {
      case Setting::BOOLEAN:
#if defined(ESP32)
        preferences.putBool(String(i).c_str(), s->booleanValue);  // Save boolean in Preferences
#else
        EEPROM.write(i, s->booleanValue ? 1 : 0);  // Store 1 for true, 0 for false in EEPROM
#endif
        break;
      case Setting::RANGE:
#if defined(ESP32)
        preferences.putInt(String(i).c_str(), s->rangeValue);  // Save range in Preferences
#else
        EEPROM.write(i, s->rangeValue);            // Store the range value directly in EEPROM
#endif
        break;
      case Setting::OPTION:
#if defined(ESP32)
        preferences.putInt(String(i).c_str(), s->optionIndex);  // Save option index in Preferences
#else
        EEPROM.write(i, s->optionIndex);           // Store option index in EEPROM
#endif
        break;
      default:
        break;
    }
  }

#if defined(ESP8266)
  EEPROM.commit();  // Commit changes to EEPROM for ESP8266
#endif
}

// Read settings from non-volatile storage
void SettingsScreen::readFromEEPROM() {
  for (int i = 0; i < totalSettings; i++) {
    Setting* s = settings[i];
    if (s == nullptr) continue;  // Skip empty settings

    // Read based on the type of setting
    switch (s->type) {
      case Setting::BOOLEAN:
#if defined(ESP32)
        s->booleanValue = preferences.getBool(String(i).c_str(), false);  // Get boolean from Preferences
#else
        s->booleanValue = EEPROM.read(i) == 1;     // Convert to boolean value from EEPROM
#endif
        break;
      case Setting::RANGE:
#if defined(ESP32)
        s->rangeValue = preferences.getInt(String(i).c_str(), 0);  // Get range from Preferences
#else
        s->rangeValue = EEPROM.read(i);            // Read the range value directly from EEPROM
#endif
        break;
      case Setting::OPTION:
#if defined(ESP32)
        s->optionIndex = preferences.getInt(String(i).c_str(), 0);  // Get option index from Preferences
#else
        s->optionIndex = EEPROM.read(i);           // Read the option index from EEPROM
#endif
        break;
      default:
        break;
    }
  }
}

const char* OpenMenuOS::getLibraryVersion() {
  return LIBRARY_VERSION;
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


void SettingsScreen::addBooleanSetting(const char* name, bool defaultValue) {
  Setting* s = new Setting(name, Setting::BOOLEAN);

  // Check if the setting already exists in EEPROM (for ESP32/ESP8266)
  if (!settingExists(totalSettings)) {
    s->booleanValue = defaultValue;
    saveToEEPROM();  // Save settings if not already in EEPROM
  } else {
    // Read value from EEPROM to associate with the setting
    s->booleanValue = getBooleanFromEEPROM(totalSettings);  // Add logic to retrieve the value from EEPROM
  }

  settings[totalSettings++] = s;
}

void SettingsScreen::addRangeSetting(const char* name, uint8_t min, uint8_t max, uint8_t defaultValue) {
  Setting* s = new Setting(name, Setting::RANGE);
  s->range.min = min;
  s->range.max = max;

  // Check if the setting already exists in EEPROM (for ESP32/ESP8266)
  if (!settingExists(totalSettings)) {
    s->rangeValue = defaultValue;
    saveToEEPROM();  // Save settings if not already in EEPROM
  } else {
    // Read value from EEPROM to associate with the setting
    s->rangeValue = getRangeFromEEPROM(totalSettings);  // Add logic to retrieve the value from EEPROM
  }

  settings[totalSettings++] = s;
}

void SettingsScreen::addOptionSetting(const char* name, const char** options, uint8_t count, uint8_t defaultIndex = 0) {
  Setting* s = new Setting(name, Setting::OPTION);

  // Dynamically allocate memory for options
  s->options = new const char*[count];
  for (uint8_t i = 0; i < count; ++i) {
    s->options[i] = options[i];
  }

  s->optionCount = count;

  // Check if the setting already exists in EEPROM (for ESP32/ESP8266)
  if (!settingExists(totalSettings)) {
    s->optionIndex = defaultIndex;
    saveToEEPROM();  // Save settings if not already in EEPROM
  } else {
    // Read value from EEPROM to associate with the setting
    s->optionIndex = getOptionIndexFromEEPROM(totalSettings);  // Add logic to retrieve the value from EEPROM
  }

  settings[totalSettings++] = s;
}


uint8_t SettingsScreen::getBooleanFromEEPROM(uint8_t index) {
  uint8_t value = 0;

#if defined(ESP32)
  value = preferences.getBool(String(index).c_str(), false);  // Get boolean from Preferences
#else
  value = EEPROM.read(index) == 1;                 // Convert to boolean value from EEPROM
#endif

  return value;
}

uint8_t SettingsScreen::getRangeFromEEPROM(uint8_t index) {
  uint8_t value = 0;

#if defined(ESP32)
  value = preferences.getInt(String(index).c_str(), 0);  // Get range value from Preferences
#else
  value = EEPROM.read(index);                      // Read the range value directly from EEPROM
#endif

  return value;
}

uint8_t SettingsScreen::getOptionIndexFromEEPROM(uint8_t index) {
  uint8_t value = 0;

#if defined(ESP32)
  value = preferences.getInt(String(index).c_str(), 0);  // Get option index from Preferences
#else
  value = EEPROM.read(index);                      // Read the option index directly from EEPROM
#endif

  return value;
}


// Check if setting already exists in EEPROM
bool SettingsScreen::settingExists(uint8_t index) {

#if defined(ESP32)
  // For ESP32, check if the setting exists in Preferences
  if (preferences.isKey(String(index).c_str())) {
    return true;  // Setting exists
  }
#else
  // For ESP8266, check if the value exists in EEPROM
  if (EEPROM.read(index) != -1) {
    return true;  // Setting exists
  }
#endif

  return false;  // Return false if setting doesn't exist
}


void SettingsScreen::modify(int8_t direction, int index) {
  // Setting* current = settings[item_selected_settings];
  Setting* current = settings[item_selected_settings];


  // Check if the current setting is valid
  if (current == nullptr) {
    return;  // Handle case where the setting is invalid
  }

  switch (current->type) {
    case Setting::BOOLEAN:
      // Toggle the boolean value between true and false
      current->booleanValue = !current->booleanValue;
      break;

    case Setting::RANGE:
      // Adjust the range value by the specified direction, within the range limits
      current->rangeValue = constrain(
        current->rangeValue + direction,
        current->range.min,
        current->range.max);
      break;

    case Setting::OPTION:
      // Change the option index by the direction, ensuring it wraps around if necessary
      current->optionIndex = (current->optionIndex + direction + current->optionCount) % current->optionCount;
      break;

    default:
      // Handle unknown setting types, if necessary
      break;
  }
  // After modifying the setting, save it to EEPROM
  saveToEEPROM();
}

String SettingsScreen::getSettingName(int index) {
  if (index < 0) {
    index = totalSettings + index;
  }

  if (index < 0 || index >= totalSettings) {
    return "Invalid Setting";
  }

  Setting* s = settings[index];

  if (s == nullptr) {
    Serial.println("Error: Setting is NULL!");
    return "Error: NULL Setting";
  }

  if (s->name == nullptr) {
    Serial.println("Error: Setting name is NULL!");
    return "Error: NULL Setting Name";
  }

  Serial.print("Setting Name: ");
  Serial.println(s->name);

  String result = String(s->name);

  return result;
}

/**
 * @brief Retrieves the raw numeric value of a specific setting.
 *
 * This function supports negative indexing (e.g. -1 returns the last setting) and returns a
 * raw value as a uint8_t. For BOOLEAN settings, it returns 1 if true and 0 if false.
 * For RANGE and OPTION settings, it returns the respective raw value stored in the setting.
 *
 * @param index The index of the setting item. Negative values wrap around.
 * @return uint8_t The raw numeric value of the setting, or 0 if the index is invalid or the setting is null.
 */
uint8_t SettingsScreen::getSettingValue(int index) {
  // Support negative indexing (e.g., -1 returns the last setting)
  if (index < 0) {
    index = totalSettings + index;
  }
  if (index < 0 || index >= totalSettings) {
    // Out-of-bounds: return 0 (or you could signal an error)
    return 0;
  }

  Setting* s = settings[index];
  if (s == nullptr) {
    return 0;
  }

  // Return a numeric value based on the setting type:
  switch (s->type) {
    case Setting::BOOLEAN:
      // Return 1 if true, 0 if false.
      return s->booleanValue ? 1 : 0;
    case Setting::RANGE:
      return s->rangeValue;
    case Setting::OPTION:
      return s->optionIndex;
    default:
      return 0;
  }
}
Setting::Type SettingsScreen::getSettingType(uint8_t index) {
  // Handle negative indices by wrapping around
  if (index < 0) {
    index = totalSettings + index;
  }

  // Ensure index is within bounds
  if (index < 0 || index >= totalSettings) {
    Serial.print("Error: Invalid index in getSettingType.:");
    Serial.println(index);
    return Setting::BOOLEAN;  // Return default value to avoid crash
  }

  Setting* s = settings[index];
  if (s == nullptr) {
    Serial.println("Error: Setting is NULL!");
    return Setting::BOOLEAN;  // Handle null settings
  }

  // Access the type of the setting
  switch (s->type) {
    case Setting::BOOLEAN:
      return Setting::BOOLEAN;
    case Setting::RANGE:
      return Setting::RANGE;
    case Setting::OPTION:
      return Setting::OPTION;
    default:
      return Setting::BOOLEAN;
  }
}
