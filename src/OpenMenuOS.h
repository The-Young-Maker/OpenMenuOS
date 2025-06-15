/**
 * @file OpenMenuOS.h
 * @brief Library to create menu system on color displays
 * @author Loic Daigle (The Young Maker)
 * @version 3.1.0
 * @date 2025
 * @license Public Domain
 *
 * OpenMenuOS is a comprehensive library for creating intuitive menu systems
 * on color TFT displays using the TFT_eSPI library. It supports multiple
 * screen types, settings management, and various input methods including
 * buttons and rotary encoders.
 *
 * Features:
 * - Multiple screen types (Menu, Settings, Custom)
 * - Button and rotary encoder input support
 * - Persistent settings storage (EEPROM/Preferences)
 * - Smooth animations and transitions
 * - Customizable themes and styles
 * - Scrollable content support
 * - Memory optimization options
 */

#ifndef OPENMENUOS_H
#define OPENMENUOS_H

//--------------------------------------------------------------------------
// Library Information
//--------------------------------------------------------------------------
#define OPENMENUOS_VERSION_MAJOR 3
#define OPENMENUOS_VERSION_MINOR 1
#define OPENMENUOS_VERSION_PATCH 0
#define OPENMENUOS_VERSION "3.1.0"

// Legacy compatibility
#define LIBRARY_VERSION OPENMENUOS_VERSION

//--------------------------------------------------------------------------
// Includes
//--------------------------------------------------------------------------
#include "Arduino.h"
#include <TFT_eSPI.h>
#include <vector>
#include <stack>
#include <map>
#include <functional>

// Platform specific includes
#if defined(ESP32)
#include <Preferences.h>
#else
#include <EEPROM.h>
#endif

#include "OpenMenuOS_images.h"

//--------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------
class Screen;
class SettingsScreen;
class MenuScreen;
class CustomScreen;
class ScreenManager;
class OpenMenuOS;
class Setting;
class PopupManager;

// Callback type for menu actions
typedef void (*ActionCallback)();

//--------------------------------------------------------------------------
// Popup System Enums and Structures
//--------------------------------------------------------------------------

/**
 * @brief Enumeration of available popup types
 *
 * Each type has its own color scheme and default styling to provide
 * consistent visual feedback for different message categories.
 */
enum class PopupType
{
  INFO,    ///< Informational message (blue theme)
  SUCCESS, ///< Success confirmation (green theme)
  WARNING, ///< Warning message (orange theme)
  ERROR,   ///< Error message (red theme)
  QUESTION ///< Question requiring user response (cyan theme)
};

/**
 * @brief Enumeration of popup interaction results
 *
 * Returned by PopupManager methods to indicate user actions.
 */
enum class PopupResult
{
  NONE,   ///< No user interaction yet
  OK,     ///< User clicked OK/Yes button
  CANCEL, ///< User clicked Cancel/No button
  YES,    ///< Alias for OK in question dialogs
  NO      ///< Alias for CANCEL in question dialogs
};

/**
 * @brief Configuration structure for popup appearance and behavior
 *
 * Allows complete customization of popup content, styling, and interaction.
 * Default values provide sensible behavior for most use cases.
 */
struct PopupConfig
{
  const char *title = nullptr;          ///< Custom title (uses type default if null)
  const char *message = nullptr;        ///< Main message text (required)
  PopupType type = PopupType::INFO;     ///< Popup type affecting colors and icons
  bool showButtons = true;              ///< Show interactive buttons
  bool showCancelButton = false;        ///< Show cancel/no button for confirmation
  bool autoClose = false;               ///< Automatically close after delay
  uint32_t autoCloseDelay = 3000;       ///< Auto-close delay in milliseconds
  uint16_t customColor = 0;             ///< Custom header color (0 = use type default)
  const uint16_t *customIcon = nullptr; ///< Custom icon image data
  uint16_t customIconWidth = 0;         ///< Custom icon width in pixels
  uint16_t customIconHeight = 0;        ///< Custom icon height in pixels
};

/**
 * @brief Professional popup management system
 *
 * PopupManager provides a modern, easy-to-use interface for displaying
 * various types of popup messages. It supports both simple one-line calls
 * for common scenarios and detailed configuration for advanced use cases.
 *
 * Features:
 * - Type-safe popup types with consistent styling
 * - Non-blocking operation via update() method
 * - Automatic word wrapping for long messages
 * - Configurable auto-close behavior
 * - Professional visual design with shadows and animations
 * - Button debouncing and input validation
 * - Memory efficient static implementation
 *
 * Usage:
 * @code
 * // Simple usage
 * PopupManager::showInfo("Operation completed successfully");
 *
 * // Question dialog
 * PopupResult result = PopupManager::showQuestion("Delete file?", "Confirm");
 * if (result == PopupResult::OK) {  }
 *
 * // Custom configuration
 * PopupConfig config;
 * config.message = "Custom popup";
 * config.type = PopupType::WARNING;
 * config.autoClose = true;
 * PopupManager::show(config);
 *
 * // In main loop
 * PopupResult result = PopupManager::update();
 * @endcode
 */
class PopupManager
{
public:
  /**
   * @brief Display a popup with custom configuration
   * @param config Complete popup configuration
   * @return PopupResult::NONE initially, check with update()
   */
  static PopupResult show(const PopupConfig &config);

  /**
   * @brief Show an informational popup
   * @param message Main message text
   * @param title Optional custom title (uses "Information" if null)
   * @return PopupResult::NONE initially, check with update()
   */
  static PopupResult showInfo(const char *message, const char *title = nullptr);

  /**
   * @brief Show a success popup (auto-closes by default)
   * @param message Main message text
   * @param title Optional custom title (uses "Success" if null)
   * @return PopupResult::NONE initially, check with update()
   */
  static PopupResult showSuccess(const char *message, const char *title = nullptr);

  /**
   * @brief Show a warning popup
   * @param message Main message text
   * @param title Optional custom title (uses "Warning" if null)
   * @return PopupResult::NONE initially, check with update()
   */
  static PopupResult showWarning(const char *message, const char *title = nullptr);

  /**
   * @brief Show an error popup
   * @param message Main message text
   * @param title Optional custom title (uses "Error" if null)
   * @return PopupResult::NONE initially, check with update()
   */
  static PopupResult showError(const char *message, const char *title = nullptr);

  /**
   * @brief Show a question popup with Yes/No buttons
   * @param message Main message text
   * @param title Optional custom title (uses "Question" if null)
   * @return PopupResult::NONE initially, check with update()
   */
  static PopupResult showQuestion(const char *message, const char *title = nullptr);

  /**
   * @brief Update popup display and handle input (call in main loop)
   * @return Current popup result (NONE if still active, OK/CANCEL when dismissed)
   */
  static PopupResult update();

  /**
   * @brief Manually hide the current popup
   */
  static void hide();

  /**
   * @brief Check if a popup is currently active
   * @return true if popup is visible, false otherwise
   */
  static bool isActive();

private:
  // Private implementation details
  struct PopupColors
  {
    uint16_t headerColor;
    uint16_t backgroundColor;
    uint16_t textColor;
    uint16_t buttonColor;
    uint16_t buttonTextColor;
    const char *defaultTitle;
    const uint16_t *defaultIcon;
    uint16_t iconWidth;
    uint16_t iconHeight;
  };
  static PopupResult currentResult;
  static bool isVisible;
  static uint32_t showTime;
  static PopupConfig currentConfig;
  static uint32_t lastButtonTime;
  static int selectedButton; // 0 = OK/Yes, 1 = Cancel/No (for encoder navigation)
  static const PopupColors colorSchemes[];

  // Rendering methods
  static void drawBackground(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
  static void drawHeader(uint16_t x, uint16_t y, uint16_t width, uint16_t headerHeight, const PopupColors &colors);
  static void drawContent(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const PopupColors &colors);
  static void drawButtons(uint16_t x, uint16_t y, uint16_t width, uint16_t buttonHeight, const PopupColors &colors);
  static void drawIcon(uint16_t x, uint16_t y, const PopupColors &colors);
  static void drawText(const char *text, uint16_t x, uint16_t y, uint16_t maxWidth, uint16_t textColor, bool bold = false);
  static void handleInput();
};

//--------------------------------------------------------------------------
// External variables
//--------------------------------------------------------------------------
extern TFT_eSPI tft;
extern TFT_eSprite canvas;
extern Screen *currentScreen;

#if defined(ESP32)
extern Preferences preferences;
#endif

//--------------------------------------------------------------------------
// Configuration structures
//--------------------------------------------------------------------------

/**
 * @brief Configuration structure for screen appearance and behavior
 *
 * This structure contains all the configurable options for customizing
 * the look and feel of the menu system. All color values use 16-bit
 * RGB565 format compatible with TFT_eSPI.
 */
struct ScreenConfig
{
  // Color configuration (RGB565 format)
  uint16_t scrollbarColor = TFT_WHITE;       ///< Color of the scrollbar
  uint16_t selectionBorderColor = TFT_WHITE; ///< Border color of selection rectangle
  uint16_t selectionFillColor = TFT_BLACK;   ///< Fill color of selection rectangle
  uint16_t selectedItemColor = TFT_WHITE;    ///< Text color of selected items

  // Feature toggles
  bool scrollbar = true;       ///< Enable/disable scrollbar display
  bool buttonAnimation = true; ///< Enable/disable button press animations
  bool textScroll = true;      ///< Enable/disable horizontal text scrolling
  bool showImages = false;     ///< Enable/disable image display in menus
  bool animation = true;       ///< Enable/disable general animations

  // Style configuration
  int menuStyle = 1;      ///< Menu style (0=outlined, 1=filled)
  int scrollbarStyle = 1; ///< Scrollbar style variant
  int textShift = -4;     ///< Vertical text offset for better centering

  // Layout proportions (as ratios of screen dimensions)
  const float itemHeightRatio = 0.30f;         ///< Selection rectangle height ratio
  const float marginRatioX = 0.05f;            ///< Horizontal text margin ratio (5%)
  const float marginRatioY = 0.01f;            ///< Vertical cleaning margin ratio (1%)
  const float toggleSwitchHeightRatio = 0.26f; ///< Toggle switch height ratio
  const float iconSizeRatio = 0.06f;           ///< Icon size ratio (6% of screen height)
};

//--------------------------------------------------------------------------
// Core classes
//--------------------------------------------------------------------------

/**
 * @brief Represents a configurable setting in the settings menu
 *
 * The Setting class encapsulates different types of configurable options
 * that can be displayed and modified in a SettingsScreen. It supports
 * boolean toggles, numeric ranges, multiple choice options, and navigation
 * to sub-screens.
 */
class Setting
{
public:
  /**
   * @brief Enumeration of available setting types
   */
  enum Type
  {
    BOOLEAN,  ///< On/off toggle setting
    RANGE,    ///< Numeric value within a specified range
    OPTION,   ///< Selection from a list of predefined options
    SUBSCREEN ///< Navigation to another screen
  };

  const char *name;  ///< Display name of the setting
  Type type;         ///< Type of setting (determines behavior)
  uint16_t id;       ///< Unique identifier for persistent storage
  Screen *subScreen; ///< Target screen for SUBSCREEN type

  // Union for storing different value types efficiently
  union
  {
    bool booleanValue;   ///< Value for BOOLEAN type
    uint8_t rangeValue;  ///< Value for RANGE type
    uint8_t optionIndex; ///< Selected index for OPTION type
  };

  // Range-specific configuration
  struct
  {
    uint8_t min;      ///< Minimum value for RANGE type
    uint8_t max;      ///< Maximum value for RANGE type
    const char *unit; ///< Unit label for RANGE type (e.g., "°C", "%")
  } range;

  // Option-specific configuration
  const char **options; ///< Array of option strings for OPTION type
  uint8_t optionCount;  ///< Number of available options

  /**
   * @brief Constructor for Setting
   * @param n Name of the setting
   * @param t Type of the setting
   */
  Setting(const char *n, Type t)
      : name(n), type(t)
  {
    generateId();
  }

private:
  /**
   * @brief Generate a unique ID using a hash of the name and type
   */
  void generateId();
};

/**
 * Screen - Base class for all screen types
 */
class Screen
{
public:
  static ScreenConfig &config;

  // Drawing methods
  virtual void draw() = 0;        // Display the screen
  virtual void handleInput() = 0; // Handle user input
  void drawScrollbar(int selectedItem, int nextItem, int numItem);
  void drawSelectionRect();

  // Text handling methods
  int calculateMaxCharacters(const char *text, uint16_t windowSize);
  void scrollTextHorizontal(int16_t x, int16_t y, const char *text, uint16_t textColor,
                            uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);
  void scrollTextHorizontal(int16_t x, int16_t y, const String &text, uint16_t textColor,
                            uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);

  virtual const char *getTitle() const
  {
    return "Untitled Screen";
  }

  // Virtual destructor
  virtual ~Screen() = default;

private:
  int lastSelectedItem = 0;
  float currentScrollPosition = 0;
  float targetScrollPosition = 0;
  unsigned long lastScrollTime = 0;
  const int SCROLL_ANIMATION_SPEED = 8; // Lower value = slower animation
};

/**
 * @brief Screen that presents a list of configurable settings
 *
 * SettingsScreen provides an interface for displaying and modifying
 * various types of settings including boolean toggles, numeric ranges,
 * option selections, and navigation to sub-screens. Settings are
 * automatically persisted to EEPROM or Preferences (ESP32).
 */
class SettingsScreen : public Screen
{
public:
  // Constants
  static const int MAX_ITEMS = 10; ///< Maximum number of settings items

  // Constructor and destructor
  SettingsScreen();
  SettingsScreen(const char *title);

  /**
   * @brief Destructor - properly closes preferences on ESP32
   */
  ~SettingsScreen() override
  {
#if defined(ESP32)
    preferences.end();
#endif
  }

  // Public methods
  void addSetting(Setting *setting);
  void resetSettings();
  void draw() override;
  void handleInput() override;
  void drawToggleSwitch(int16_t x, int16_t y, Setting *setting, uint16_t bgColor = TFT_BLACK, bool isSelected = false);

  // Setting management methods
  void modify(int8_t direction, int index);
  void modify(int8_t direction, const char *name);
  Setting::Type getSettingType(uint8_t index);
  String getSettingName(int index);
  uint8_t getSettingValue(int index);
  uint8_t getSettingValue(const char *name);
  const char *getTitle() const override
  {
    return title;
  }
  // Setting creation helper methods
  void addBooleanSetting(const char *name, bool defaultValue);
  void addRangeSetting(const char *name, uint8_t min, uint8_t max, uint8_t defaultValue, const char *unit);
  void addOptionSetting(const char *name, const char **options, uint8_t count, uint8_t defaultIndex);
  void addSubscreenSetting(const char *name, Screen *targetScreen);

  // Public properties
  const char *title;       // Title of the settings screen
  uint8_t totalSettings;   // Number of settings items
  int currentSettingIndex; // Index of the currently selected setting

  // Navigation tracking
  int item_selected_settings_previous = -1;
  int item_selected_settings = 0;
  int item_selected_settings_next = 1;

  // Button state tracking
  unsigned long pressedTime = 0;
  unsigned long releasedTime = 0;
  bool isPressing = false;
  bool isLongDetected = false;
  bool previousButtonState = LOW;
  bool buttonPressProcessed = false;
  int upButtonState;
  int downButtonState;

private:
  struct ToggleState
  {
    bool currentState;
    float currentPosition;
    float targetPosition;
    unsigned long lastToggleTime;
    bool animating;
  };

  std::vector<Setting *> settings;
  std::map<int, ToggleState> toggleStates;
  const int TOGGLE_ANIMATION_SPEED = 3; // Lower = slower, higher = faster

  // State variables
  bool settingSelectLock = false;
  bool flickerState = false;
  unsigned long previousMillis = 0;
  const long flickerInterval = 500; // Flicker interval in milliseconds

  // Private methods
  void ensureInitialized();

  uint16_t calculateAvailableWidth(int settingIndex, uint16_t rectWidth, float textMarginX, uint16_t toggleSwitchWidth);

  // EEPROM/Preferences methods
  void saveToEEPROM();
  void readFromEEPROM();
  bool settingExists(uint16_t settingId);
  uint8_t getBooleanFromEEPROM(uint16_t settingId);
  uint8_t getRangeFromEEPROM(uint16_t settingId);
  uint8_t getOptionIndexFromEEPROM(uint16_t settingId);
};

/**
 * MenuItem - Structure representing a single menu item
 */
struct MenuItem
{
  const char *label;     // Label for the menu item
  Screen *nextScreen;    // Screen to navigate to when selected
  ActionCallback action; // Action to perform when selected
  const uint16_t *image; // Image associated with this menu item
};

/**
 * MenuScreen - Screen that presents a list of menu options
 */
class MenuScreen : public Screen
{
public:
  // Constructor and destructor
  MenuScreen();
  MenuScreen(const char *title);
  ~MenuScreen() override = default;
  const char *getTitle() const override
  {
    return title;
  }

  // Public methods
  void addItem(const char *label, Screen *nextScreen = nullptr, ActionCallback action = nullptr, const uint16_t *image = nullptr);
  void addItem(Screen *nextScreen, ActionCallback action = nullptr, const uint16_t *image = nullptr);
  void draw() override;
  void handleInput() override;
  int getIndex();
  uint16_t alphaBlend(uint16_t fg, uint16_t bg, uint8_t alpha);

  // Properties
  const char *title;           // Title of the menu screen
  std::vector<MenuItem> items; // Collection of menu items
  int currentItemIndex;        // Index of the selected menu item
  int itemSize;                // Count of menu items

  // Layout configuration
  float itemHeightRatio; // Ratio for item height
  float marginRatioY;    // Y margin ratio
  float marginRatioX;    // X margin ratio

private:
  // Private initialization method
  void initializeDefaults();
};

/**
 * CustomScreen - Screen with custom drawing behavior
 */
class CustomScreen : public Screen
{
public:
  // Constructor and destructor
  CustomScreen();
  CustomScreen(const char *title);
  // Function pointer or lambda to override draw functionality
  std::function<void()> customDraw;

  // Implementation of virtual methods
  void draw() override
  {
    if (customDraw)
    {
      customDraw(); // Execute the custom draw behavior
    }
  }
  void handleInput() override;

  ~CustomScreen() override = default;

  const char *getTitle() const override
  {
    return title;
  }

  const char *title; // Title of the menu screen
};

/**
 * @brief Manages navigation between screens using a stack-based approach
 *
 * ScreenManager handles the navigation flow between different screens,
 * maintaining a history stack for back navigation and ensuring proper
 * screen lifecycle management.
 */
class ScreenManager
{
public:
  std::vector<Screen *> screenHistory; ///< Stack of previous screens
  Screen *currentScreen = nullptr;     ///< Currently active screen

  /**
   * @brief Push a new screen onto the navigation stack
   * @param newScreen Pointer to the screen to display
   */
  void pushScreen(Screen *newScreen)
  {
    if (newScreen == nullptr)
    {
      return; // Cannot push null screen
    }

    if (currentScreen != nullptr)
    {
      screenHistory.push_back(currentScreen);
    }

    currentScreen = newScreen;
    currentScreen->draw();

    // Update global reference
    ::currentScreen = currentScreen;
  }

  /**
   * @brief Pop the current screen and return to the previous one
   * @return true if successfully navigated back, false if already at root
   */
  bool popScreen()
  {
    if (screenHistory.empty())
    {
      return false; // Cannot go back further
    }

    currentScreen = screenHistory.back();
    screenHistory.pop_back();

    if (currentScreen != nullptr)
    {
      currentScreen->draw();
    }

    // Update global reference
    ::currentScreen = currentScreen;
    return true;
  }

  /**
   * @brief Check if back navigation is possible
   * @return true if there are screens in the history stack
   */
  bool canGoBack() const
  {
    return !screenHistory.empty();
  }

  /**
   * @brief Get the current navigation depth
   * @return Number of screens in the history stack
   */
  size_t getDepth() const
  {
    return screenHistory.size();
  }
};

/**
 * @brief Main library class for creating menu systems on color displays
 *
 * OpenMenuOS provides a complete framework for creating interactive menu
 * systems on TFT displays. It manages screens, handles input from buttons
 * or encoders, and provides various customization options for appearance
 * and behavior.
 *
 * Example usage:
 * @code
 * OpenMenuOS menu(2, 3, 4); // UP, DOWN, SELECT pins
 * MenuScreen mainMenu("Main Menu");
 *
 * void setup() {
 *   menu.begin(&mainMenu);
 * }
 *
 * void loop() {
 *   menu.loop();
 * }
 * @endcode
 */
class OpenMenuOS
{
public:
  // Constructor and core methods
  /**
   * @brief Constructor for OpenMenuOS
   * @param btn_up Pin number for UP button (-1 to disable)
   * @param btn_down Pin number for DOWN button (-1 to disable)
   * @param btn_sel Pin number for SELECT button (-1 to disable)
   */
  OpenMenuOS(int btn_up = -1, int btn_down = -1, int btn_sel = -1);

  /**
   * @brief Initialize the menu system with specified rotation
   * @param rotation Display rotation (0-3)
   * @param mainMenu Pointer to the main menu screen
   */
  void begin(int rotation, Screen *mainMenu);

  /**
   * @brief Initialize the menu system with default rotation
   * @param mainMenu Pointer to the main menu screen
   */
  void begin(Screen *mainMenu);

  /**
   * @brief Main loop function - call this in Arduino's loop()
   */
  void loop();

  // Navigation
  void redirectToScreen(Screen *nextScreen);
  void navigateBack();

  // Text handling
  void scrollTextHorizontal(int16_t x, int16_t y, const char *text, uint16_t textColor,
                            uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);
  void scrollTextHorizontal(int16_t x, int16_t y, const String &text, uint16_t textColor,
                            uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize);

  // UI Configuration methods
  void setDisplayRotation(int rotation);
  void setTextScroll(bool x);
  void showBootImage(bool x);
  void setBootImage(uint16_t *Boot_img, uint16_t height, uint16_t width);
  void setButtonAnimation(bool x);
  void setMenuStyle(int style);
  void setScrollbar(bool x);
  void setScrollbarColor(uint16_t color);
  void setScrollbarStyle(int style);
  void setSelectionBorderColor(uint16_t color);
  void setSelectionFillColor(uint16_t color);
  void setAnimation(bool enabled);
  void setMenuFont(const GFXfont *font);
  void setMenuFontBold(const GFXfont *font);
  void setOptimizeDisplayUpdates(bool enabled = true);

  // Style presets and button configuration
  void useStylePreset(int preset);
  void useStylePreset(char *preset);
  void setButtonsMode(char *mode);
  void setEncoderPin(uint8_t clk, uint8_t dt);
  void setUpPin(uint8_t btn_down);
  void setDownPin(uint8_t btn_up);
  void setSelectPin(uint8_t btn_sel);

  // Drawing methods
  void drawCanvasOnTFT();
  // Getters
  const char *getLibraryVersion();
  int getTftHeight() const;
  int getTftWidth() const;
  int UpButton() const;
  int DownButton() const;
  int SelectButton() const;
  bool getOptimizeDisplayUpdates() const;

private:
  // Private methods
  void drawPopup(char *message, bool &clicked, int type);

  // Boot image properties
  uint16_t *boot_image = nullptr;
  uint16_t boot_image_width = 0;
  uint16_t boot_image_height = 0;

  // Display optimization
  bool optimizeDisplayUpdates = false;

  // UI behavior flags
  bool bootImage = false;

  bool tftInitialized = false;

  int displayRotation = 0;
};

#endif // OPENMENUOS_H