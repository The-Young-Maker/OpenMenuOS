/*
  OpenMenuOS.h - Library to create menu on ST7735 display.
  Created by Loic Daigle aka The Young Maker.
  Released into the public domain.
*/

#ifndef OPENMENUOS_H
#define OPENMENUOS_H

#include "Arduino.h"
#include <TFT_eSPI.h>
#include "OpenMenuOS_images.h"
#include <stack>
#if defined(ESP32)
#include <Preferences.h>
#else
#include <EEPROM.h>
#endif
#define MAX_SETTINGS_ITEMS 20  // Maximum number of settings items
#define LIBRARY_VERSION "3.0.0"

extern TFT_eSPI tft;        // Declare tft as extern
extern TFT_eSprite canvas;  // Declare canvas as extern

class Setting {
public:
  enum Type { BOOLEAN,
              RANGE,
              OPTION };

  const char* name;
  Type type;
  union {
    bool booleanValue;
    uint8_t rangeValue;
    uint8_t optionIndex;
  };

  struct {
    uint8_t min;
    uint8_t max;
  } range;

  const char** options;
  uint8_t optionCount;

  Setting(const char* n, Type t)
    : name(n), type(t) {}
};
struct ScreenConfig {
  uint16_t scrollbarColor = TFT_WHITE;
  uint16_t selectionBorderColor = TFT_WHITE;
  uint16_t selectionFillColor = TFT_BLACK;
  uint16_t selectedItemColor = TFT_WHITE;
  bool scrollbar = true;
  bool buttonAnimation = true;
  int menuStyle = 1;  // 0 or 1
  bool textScroll = true;
  int scrollbarStyle = 1;
  bool showImages;

  int textShift = -4;  // The Y shift of the menu item text (to make them more centered)

  // Ratio constant
  const float itemHeightRatio = 0.30f;          // Height of the selection rectangle in proportion to the height of the screen
  const float marginRatioX = 0.05f;             // Horizontal margin (for text) – 5% of width
  const float marginRatioY = 0.01f;             // Vertical cleaning margin – 1% of height
  const float toggleSwitchHeightRatio = 0.26f;  // Toggle switch height in proportion to screen height
  const float iconSizeRatio = 0.06f;            // Icon size in proportion to screen height
};
// Forward declaration for Screen class and action callback type
class Screen;
typedef void (*ActionCallback)();

// Global pointer to track the current screen
extern Screen* currentScreen;
// Base class for all screens
class Screen {
public:
  static ScreenConfig& config;

  // Display dimensions
  int tftWidth;
  int tftHeight;
  void drawScrollbar(int selectedItem, int nextItem, int numItem);
  int calculateMaxCharacters(const char* text, uint16_t windowSize);
  // Scroll text horizontally
  void scrollTextHorizontal(int16_t x, int16_t y, const char* text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);
  void scrollTextHorizontal(int16_t x, int16_t y, const String& text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);
  virtual void draw() = 0;         // Display the screen
  virtual void handleInput() = 0;  // Handle user input
  virtual ~Screen() = default;
};


class SettingsScreen : public Screen {
private:
#if defined(ESP32)
  Preferences preferences;  // Declare a Preferences instance for ESP32
#endif
  void initializeSettings() {
#if defined(ESP32)
    preferences.begin("Settings", false);  // Initialize Preferences for ESP32
#else
    EEPROM.begin(512);  // Initialize EEPROM for ESP8266
#endif
    readFromEEPROM();  // Read saved settings
  }
public:
  void modify(int8_t direction, int index);

  Setting::Type getSettingType(uint8_t index);

  String getSettingName(int index);
  uint8_t getSettingValue(int index);
  void saveToEEPROM();
  void readFromEEPROM();
  void addBooleanSetting(const char* name, bool defaultValue);
  void addRangeSetting(const char* name, uint8_t min, uint8_t max, uint8_t defaultValue);
  void addOptionSetting(const char* name, const char** options, uint8_t count, uint8_t defaultIndex);
  bool settingExists(uint8_t index);
  uint8_t getBooleanFromEEPROM(uint8_t index);
  uint8_t getRangeFromEEPROM(uint8_t index);
  uint8_t getOptionIndexFromEEPROM(uint8_t index);
  const char* title;                // Title of the settings screen
  static const int MAX_ITEMS = 10;  // Maximum number of settings items
  uint8_t totalSettings;
  // Number of settings items
  int currentSettingIndex;  // Index of the currently selected setting

  // Display dimensions
  int tftWidth;
  int tftHeight;

  int button_up_clicked_settings = 0;
  int button_down_clicked_settings = 0;
  // Settings item positioning
  int item_selected_settings_previous = -1;
  int item_selected_settings = 0;
  int item_selected_settings_next = 1;

  // Button press timing and control
  unsigned long pressedTime = 0;
  unsigned long releasedTime = 0;
  bool isPressing = false;
  bool isLongDetected = false;
  bool isLongDetssdected = false;
  bool previousButtonState = LOW;
  bool buttonPressProcessed = false;
  int upButtonState;
  int downButtonState;

  // Constructor to initialize the title, counters, and default configuration
  SettingsScreen(const char* title);

  // Add a setting to the screen
  void addSetting(Setting* setting);

  // Draw the settings screen using advanced graphics
  virtual void draw() override;

  virtual void handleInput() override;

  // Draw the toggle switch for boolean settings
  void drawToggleSwitch(int16_t x, int16_t y, bool state, uint16_t bgColor = TFT_BLACK);

  // Destructor to clean up resources
  ~SettingsScreen() override {
#if defined(ESP32)
    preferences.end();  // Close Preferences for ESP32
#endif
  }
};


// Structure representing one menu item
struct MenuItem {
  const char* label;      // Label for the menu item
  Screen* nextScreen;     // Screen to navigate to when the item is selected
  ActionCallback action;  // Action to perform when the item is selected
  const uint16_t* image;  // Pointer to the image (bitmap) associated with this menu item
};

// MenuScreen class that uses the same advanced drawing logic for all menus
class MenuScreen : public Screen {
public:
  const char* title;                // Title of the menu screen
  static const int MAX_ITEMS = 10;  // Maximum number of menu items
  MenuItem items[MAX_ITEMS];        // Array of menu items
  int itemCount;                    // Count of menu items
  int currentItemIndex;             // Index of the currently selected menu item

  // Configuration for drawing logic
  float itemHeightRatio;  // Ratio for item height
  float marginRatioY;     // Y margin ratio
  float marginRatioX;     // X margin ratio

  // Display dimensions
  int tftWidth;
  int tftHeight;

  // Constructor initializes the title, counters, and default configuration
  MenuScreen(const char* title);

  // Add a menu item to the screen
  void addItem(const char* label, Screen* nextScreen = nullptr, ActionCallback action = nullptr, const uint16_t* image = nullptr);

  // Get the current index of the screen
  int getIndex();

  // Draw the menu using advanced graphics
  virtual void draw() override;

  virtual void handleInput() override;

  ~MenuScreen() override = default;
};

class SettingSelectionScreen : public SettingsScreen {
public:
  SettingSelectionScreen(const char* title)
    : SettingsScreen(title) {
    // Additional initialization for SettingSelectionScreen, if necessary
  }
  void draw();

  void handleInput();

  ~SettingSelectionScreen() override = default;
};
class CustomScreen : public Screen {
public:
  // Function pointer or lambda to override draw functionality
  std::function<void()> customDraw;

  void draw() override {
    if (customDraw) {
      customDraw();  // Executes the custom draw behavior
    }
  }

  virtual void handleInput() override;

  ~CustomScreen() override = default;
};

#include <vector>

class ScreenManager {
public:
  std::vector<Screen*> screenHistory;
  Screen* currentScreen = nullptr;

  // Push a new screen onto the stack
  void pushScreen(Screen* newScreen) {
    if (currentScreen != nullptr) {
      screenHistory.push_back(currentScreen);  // Store the current screen before changing
    }
    currentScreen = newScreen;  // Set the new screen
    currentScreen->draw();      // Draw the new screen

    // Update the global currentScreen
    ::currentScreen = currentScreen;
  }

  // Pop the current screen and go back to the last one
  void popScreen() {
    if (!screenHistory.empty()) {
      currentScreen = screenHistory.back();  // Get the last screen
      screenHistory.pop_back();              // Remove the last screen from the history stack
      currentScreen->draw();                 // Draw the last screen

      // Update the global currentScreen
      ::currentScreen = currentScreen;
    }
  }

  // Check if we can go back to a previous screen
  bool canGoBack() {
    return !screenHistory.empty();
  }
};
class OpenMenuOS {
public:

  OpenMenuOS(int btn_up = NULL, int btn_down = NULL, int btn_sel = NULL);  // BTN_UP pin, BTN_DOWN pin, BTN_SEL pin, TFT Backlight pin

  void begin(int rotation, Screen* mainMenu);  // Display type
  void loop();


  // Redirect to a menu
  void redirectToScreen(Screen* nextScreen);
  // Draw a popup
  void drawPopup(char* message, bool& clicked, int type);
  // Scroll text horizontally
  void scrollTextHorizontal(int16_t x, int16_t y, const char* text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);
  void scrollTextHorizontal(int16_t x, int16_t y, const String& text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);

  // Enable or disable text scrolling
  void setTextScroll(bool x);
  // Enable or disable boot image
  void showBootImage(bool x);
  void setBootImage(uint16_t* Boot_img, uint16_t height, uint16_t width);
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

  void drawCanvasOnTFT();

  const char* getLibraryVersion();

  int getTftHeight() const;  // Getter method for tftHeight
  int getTftWidth() const;   // Getter method for tftWidth
  int UpButton() const;      // Getter method for Up Button
  int DownButton() const;    // Getter method for Down Button
  int SelectButton() const;  // Getter method for Select Button


  void setMenuFont(const GFXfont* font);

  void setMenuFontBold(const GFXfont* font);
private:
  // Global variables
  uint16_t* boot_image = nullptr;  // Pointer to store the boot image
  uint16_t boot_image_width = 0;   // Width of the boot image
  uint16_t boot_image_height = 0;  // Height of the boot image

  // Display dimensions
  int tftWidth;
  int tftHeight;

  // UI behavior flags
  bool bootImage = false;
};
#endif