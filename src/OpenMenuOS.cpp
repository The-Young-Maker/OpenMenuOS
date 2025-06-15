/**
 * @file OpenMenuOS.cpp
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
 */

#include "Arduino.h"
#include <TFT_eSPI.h>
#include "OpenMenuOS.h"
#include <string>

//-------------------------------------------------------------------------
// Constants and Global Variables
//-------------------------------------------------------------------------

// Default fonts for menu system
const GFXfont *menuFont = &FreeMono9pt7b;         ///< Regular menu font
const GFXfont *menuFontBold = &FreeMonoBold9pt7b; ///< Bold font for selected items

// Display and canvas instances
TFT_eSPI tft = TFT_eSPI();              ///< Main TFT display instance
TFT_eSprite canvas = TFT_eSprite(&tft); ///< Off-screen canvas for smooth rendering

// Screen management
ScreenManager screenManager;               ///< Handles screen navigation
ScreenConfig menuConfig;                   ///< Global configuration settings
ScreenConfig &Screen::config = menuConfig; ///< Reference for all screens
Screen *currentScreen = nullptr;           ///< Currently active screen

// Button configuration
int buttonsMode;                      ///< Button pin mode (INPUT/INPUT_PULLUP)
int buttonVoltage;                    ///< Expected voltage level for button press
int BUTTON_UP_PIN;                    ///< Pin number for UP button
int BUTTON_DOWN_PIN;                  ///< Pin number for DOWN button
int BUTTON_SELECT_PIN;                ///< Pin number for SELECT button
int prevSelectState = !buttonVoltage; ///< Previous state of select button

// Display dimensions (cached for performance)
int tftWidth;  ///< Display width in pixels
int tftHeight; ///< Display height in pixels

// Timing constants for button press detection
static constexpr int SHORT_PRESS_TIME = 300;                  ///< Short press threshold (ms)
static constexpr int LONG_PRESS_TIME = 1000;                  ///< Long press threshold (ms)
static constexpr int LONG_PRESS_TIME_MENU = 500;              ///< Menu-specific long press (ms)
static constexpr int SELECT_BUTTON_LONG_PRESS_DURATION = 300; ///< Select button long press (ms)

// Rotary encoder configuration
uint8_t encoderClkPin = 0;            ///< CLK pin for rotary encoder
uint8_t encoderDtPin = 0;             ///< DT pin for rotary encoder
bool useEncoder = false;              ///< Flag to enable encoder input
volatile int encoderPosition = 0;     ///< Current encoder position
volatile uint8_t oldState = 0;        ///< Previous encoder state
volatile bool encoderChanged = false; ///< Flag indicating encoder movement

// Non-volatile storage configuration
#if defined(ESP32)
Preferences preferences; ///< ESP32 preferences instance for settings storage
#endif
static bool preferencesInitialized = false; ///< Initialization flag for preferences

//-------------------------------------------------------------------------
// Interrupts and Low-level Functions
//-------------------------------------------------------------------------

/**
 * @brief Interrupt Service Routine for rotary encoder
 *
 * This ISR handles encoder state changes using a state transition table
 * approach for reliable direction detection. Called on both CLK and DT
 * pin changes to ensure accurate position tracking.
 */
void encoderISR()
{
  // State transition table for reliable encoder direction detection
  // Based on standard quadrature encoder state machine
  static const int8_t KNOBDIR[] = {
      0, -1, 1, 0, // States 0-3
      1, 0, 0, -1, // States 4-7
      -1, 0, 0, 1, // States 8-11
      0, 1, -1, 0  // States 12-15
  };

  // Read current encoder pin states
  uint8_t sig1 = digitalRead(encoderClkPin); // CLK pin state
  uint8_t sig2 = digitalRead(encoderDtPin);  // DT pin state
  uint8_t thisState = sig1 | (sig2 << 1);    // Combine into 2-bit state

  // Process state change using the state machine
  if (oldState != thisState)
  {
    // Calculate direction from state transition table
    // Combine current and previous states for table lookup
    int8_t direction = KNOBDIR[thisState | (oldState << 2)];
    encoderPosition += direction;
    oldState = thisState;

    // Signal that encoder position has changed
    encoderChanged = true;
  }
}

//-------------------------------------------------------------------------
// OpenMenuOS Class Implementation
//-------------------------------------------------------------------------

/**
 * @brief Constructor for OpenMenuOS
 * @param btn_up Pin number for UP button (-1 to disable)
 * @param btn_down Pin number for DOWN button (-1 to disable)
 * @param btn_sel Pin number for SELECT button (-1 to disable)
 */
OpenMenuOS::OpenMenuOS(int btn_up, int btn_down, int btn_sel)
{
  BUTTON_UP_PIN = btn_up;
  BUTTON_DOWN_PIN = btn_down;
  BUTTON_SELECT_PIN = btn_sel;
}
/**
 * @brief Initialize OpenMenuOS with default rotation
 * @param mainMenu Pointer to the main menu screen to display
 */
void OpenMenuOS::begin(Screen *mainMenu)
{
  begin(displayRotation, mainMenu);
}

/**
 * @brief Initialize OpenMenuOS with specified rotation
 * @param rotation Display rotation (0-3)
 * @param mainMenu Pointer to the main menu screen to display
 */
void OpenMenuOS::begin(int rotation, Screen *mainMenu)
{
  // Validate input parameters
  if (mainMenu == nullptr)
  {
    return; // Cannot proceed without a valid main menu
  }

  // Initialize display
  tft.init();
  tft.setRotation(rotation);
  tftInitialized = true;

  // Cache display dimensions
  tftWidth = tft.width();
  tftHeight = tft.height();

  // Display boot image if configured
  if (bootImage && boot_image != nullptr)
  {
    tft.pushImage(0, 0, boot_image_width, boot_image_height, boot_image);
    delay(3000); // 3 second boot delay
  }

  // Configure display settings
  tft.setTextWrap(false);
  canvas.setSwapBytes(true);

  // Create sprite with correct dimensions
  canvas.createSprite(tftWidth, tftHeight);
  canvas.fillSprite(TFT_BLACK);

// Configure backlight pin if available
#ifdef TFT_BL
  pinMode(TFT_BL, OUTPUT);
#endif
  // Initialize button pins if specified
  if (BUTTON_UP_PIN != -1)
  {
    pinMode(BUTTON_UP_PIN, buttonsMode);
  }

  if (BUTTON_DOWN_PIN != -1)
  {
    pinMode(BUTTON_DOWN_PIN, buttonsMode);
  }

  if (BUTTON_SELECT_PIN != -1)
  {
    pinMode(BUTTON_SELECT_PIN, buttonsMode);
  }

  // Initialize encoder if enabled
  if (useEncoder)
  {
    // Validate encoder pins
    if (encoderClkPin == 0 || encoderDtPin == 0)
    {
      useEncoder = false; // Disable encoder if pins not properly set
    }
    else
    {
      // Set initial encoder state
      oldState = (digitalRead(encoderClkPin) | (digitalRead(encoderDtPin) << 1));

      // Attach interrupts to both encoder pins for reliable operation
      attachInterrupt(digitalPinToInterrupt(encoderClkPin), encoderISR, CHANGE);
      attachInterrupt(digitalPinToInterrupt(encoderDtPin), encoderISR, CHANGE);
    }
  }

  // Initialize screen manager with main menu
  screenManager.pushScreen(mainMenu);
}

/**
 * @brief Main loop function - should be called in Arduino's loop()
 *
 * Handles input processing for the current screen and updates the display.
 * This function must be called regularly to maintain responsive UI.
 */
void OpenMenuOS::loop()
{
  // Process input if a screen is active and no popup is active
  if (currentScreen != nullptr && !PopupManager::isActive())
  {
    currentScreen->handleInput();
  }

  // Update display with current canvas content
  drawCanvasOnTFT();
}

//-------------------------------------------------------------------------
// SettingsScreen Class Implementation
//-------------------------------------------------------------------------

/**
 * @brief Default constructor for SettingsScreen
 */
SettingsScreen::SettingsScreen()
    : title(nullptr), totalSettings(0), currentSettingIndex(0)
{
}

/**
 * @brief Constructor for SettingsScreen with title
 * @param title The title to display for this settings screen
 */
SettingsScreen::SettingsScreen(const char *title)
    : title(title), totalSettings(0), currentSettingIndex(0)
{
}

/**
 * @brief Add a setting to the settings screen
 * @param setting Pointer to the setting to add
 */
void SettingsScreen::addSetting(Setting *setting)
{
  if (setting != nullptr)
  {
    settings.push_back(setting);
    totalSettings = settings.size();
  }
}

void Screen::drawSelectionRect()
{
  // Rectangle dimensions for menu selection
  uint16_t rect_width = config.scrollbar ? tftWidth * 0.97f : tftWidth; // Width of the selection rectangle
  uint16_t rect_height = tftHeight * config.itemHeightRatio;            // Height of the selection rectangle

  // Ensure proper width calculation
  if (!config.scrollbar)
  {
    rect_width = tftWidth;
  }
  else
  {
    rect_width = tftWidth * 0.97f;
  }

  // Vertically center the selection rectangle
  uint16_t rect_x = 0;
  uint16_t rect_y = (tftHeight - rect_height) / 2;

  // Clear areas to remove previous text by drawing rounded rectangles
  float clearMargin = tftHeight * config.marginRatioY;
  uint16_t selectedItemColor;
  uint16_t lineLength;

  canvas.fillRoundRect(rect_x + 1, rect_y - rect_height - clearMargin, rect_width - 3, rect_height - 3, 4, TFT_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y - 1, rect_width - 3, rect_height - 3, 4, TFT_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + rect_height + clearMargin, rect_width - 3, rect_height - 3, 4, TFT_BLACK);

  // Draw selection rectangle based on style and button state
  switch (config.menuStyle)
  {
  case 0:
    if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage && config.buttonAnimation)
    {
      canvas.drawSmoothRoundRect(rect_x + 1, rect_y + 1, 4, 4,
                                 rect_width - 2, rect_height - 1, config.selectionBorderColor, TFT_BLACK);
    }
    else
    {
      switch (config.scrollbar)
      {
      case false:
        canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, config.selectionBorderColor, TFT_BLACK);
        lineLength = rect_height * 0.9f;
        canvas.drawFastVLine(tftWidth - 2, rect_y + tftHeight * 0.05f, lineLength, config.selectionBorderColor);
        canvas.drawFastVLine(tftWidth, rect_y + tftHeight * 0.05f, lineLength - 1, config.selectionBorderColor);
        canvas.drawFastHLine(2, rect_y + rect_height, rect_width - 3, config.selectionBorderColor);
        canvas.drawFastHLine(3, rect_y + rect_height, rect_width - 4, config.selectionBorderColor);
        break;

      case true:
        canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width - 2, rect_height, config.selectionBorderColor, TFT_BLACK);
        lineLength = rect_height * 0.9f;
        canvas.drawFastVLine(rect_width - 4, rect_y + 2, lineLength, config.selectionBorderColor);
        canvas.drawFastVLine(rect_width - 3, rect_y + 2, lineLength - 1, config.selectionBorderColor);
        canvas.drawFastHLine(2, rect_y + rect_height, rect_width * 0.95f, config.selectionBorderColor);
        canvas.drawFastHLine(3, rect_y + rect_height + 1, rect_width * 0.95f - 1, config.selectionBorderColor);
        break;
      }
    }
    config.selectedItemColor = TFT_WHITE;
    break;

  case 1:
    canvas.fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 4, config.selectionBorderColor, TFT_BLACK);
    config.selectedItemColor = TFT_BLACK;
    break;

  default:
    // Default case to handle any other menu styles that might be added in the future
    canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, config.selectionBorderColor, TFT_BLACK);
    config.selectedItemColor = TFT_WHITE;
    break;
  }
}

void SettingsScreen::draw()
{
  // Rectangle dimensions for menu selection
  uint16_t rect_width = config.scrollbar ? tftWidth * 0.97f : tftWidth; // Width of the selection rectangle
  drawSelectionRect();

  // Calculate adaptive positions for text and icons
  float textMarginX = tftWidth * config.marginRatioX;

  // X Position of the text depending if icons are presents
  int xPos = textMarginX;
  int x1Pos = xPos;
  int x2Pos = xPos;

  int16_t fontHeight;

  int yPos = tftHeight / 3;  // Previous element
  int y1Pos = tftHeight / 2; // Selected element
  int y2Pos = tftHeight;     // Next element

  int itemNumber = 3; // Number of shown item

  /////// Variables for the toggle buttons ///////
  int rectHeight = 20;

  int roundDiameter = 15;

  uint16_t toggleSwitchHeight = tftHeight * config.toggleSwitchHeightRatio;
  uint16_t toggleSwitchWidth = toggleSwitchHeight * 2;

  int rectX = 110;

  uint16_t textWidth;
  uint16_t availableWidth;
  int scrollWindowSize = rect_width - ((textMarginX * 2) + toggleSwitchWidth);

  ////////////////////////////////////////////////
  // draw previous item as icon + label
  String previousItem = getSettingName(item_selected_settings_previous);

  // Calculate the available width for the text
  availableWidth = calculateAvailableWidth(item_selected_settings_previous, rect_width, textMarginX, toggleSwitchWidth);

  textWidth = canvas.textWidth(previousItem);

  // Dynamically adjust the text to fit the available space
  if (textWidth > availableWidth)
  {
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

  switch (getSettingType(item_selected_settings_previous))
  {
  case Setting::BOOLEAN:
  {
    int rectY = (tftHeight / itemNumber - toggleSwitchHeight) / 2;
    int rectX = rect_width - textMarginX + 4 - toggleSwitchWidth; // Horizontal position (right edge with 2 pixel margin)

    // Get the current value from the class
    if (item_selected_settings_previous >= 0 && item_selected_settings_previous < totalSettings)
    {
      drawToggleSwitch(rectX, rectY, settings[item_selected_settings_previous]);
    }
    break;
  }
  case Setting::RANGE:
  {
    int rectY = (tftHeight / itemNumber) / 2;

    uint8_t currentValue = 0;

    if (item_selected_settings_previous >= 0 && item_selected_settings_previous < totalSettings)
    {
      currentValue = settings[item_selected_settings_previous]->rangeValue;
    }

    // Build the text to display
    String text;

    if (settings[item_selected_settings_previous]->range.unit != nullptr && settings[item_selected_settings_previous]->range.unit != NULL)
    {
      text = String(currentValue) + settings[item_selected_settings_previous]->range.unit;
    }
    else
    {
      text = String(currentValue);
    }

    canvas.setTextDatum(MR_DATUM); // Middle Right Datum

    // Calculate position to align text near right edge
    int rectX = rect_width - textMarginX; // Horizontal position (right edge with 2 pixel margin)

    // Display right-aligned text
    canvas.drawString(text, rectX, rectY);

    // Reset to avoid affecting other texts
    canvas.setTextDatum(TL_DATUM); // Back to default origin
    break;
  }
  case Setting::OPTION:
  {
    int rectY = (tftHeight / itemNumber) / 2;

    // Set the text origin to the right edge
    canvas.setTextDatum(MR_DATUM); // Middle Right Datum

    // Calculate position to align text near right edge
    int rectX = rect_width - textMarginX; // Horizontal position (right edge with 2 pixel margin)

    // Display right-aligned text
    const char *option = settings[item_selected_settings_previous]->options[settings[item_selected_settings_previous]->optionIndex];

    canvas.drawString(option, rectX, rectY);
    // Reset to avoid affecting other texts
    canvas.setTextDatum(TL_DATUM); // Back to default origin
    break;
  }
  case Setting::SUBSCREEN:
  {
    int rectY = (tftHeight / itemNumber) / 2;

    // Set the text origin to the right edge
    canvas.setTextDatum(MR_DATUM); // Middle Right Datum

    // Calculate position to align text near right edge
    int rectX = rect_width - textMarginX; // Horizontal position (right edge with 2 pixel margin)

    canvas.drawString(">", rectX, rectY);

    // Reset to avoid affecting other texts
    canvas.setTextDatum(TL_DATUM); // Back to default origin
    break;
  }
  }
  // draw selected item as icon + label in bold font
  canvas.setFreeFont(menuFontBold);
  canvas.setTextSize(1);
  fontHeight = canvas.fontHeight();

  String selectedItem = getSettingName(item_selected_settings);

  canvas.setTextColor(config.selectedItemColor, config.selectionFillColor);

  // Calculate the available width for the text
  availableWidth = calculateAvailableWidth(item_selected_settings, rect_width, textMarginX, toggleSwitchWidth);

  textWidth = canvas.textWidth(selectedItem);

  // Check if the text fits the available width
  if (textWidth > availableWidth && config.textScroll)
  {
    // If it doesn't fit, scroll the text horizontally
    scrollTextHorizontal(x1Pos, tftHeight / 2 + (fontHeight / 2) + config.textShift, selectedItem,
                         config.selectedItemColor, config.selectionFillColor, 1, 50, availableWidth);
  }
  else
  {
    canvas.setCursor(x1Pos, tftHeight / 2 + (fontHeight / 2) + config.textShift);

    // If the text doesn't fit, truncate it and add "..."
    if (textWidth > availableWidth)
    {
      uint16_t maxLength = calculateMaxCharacters(selectedItem.c_str(), availableWidth);
      selectedItem = selectedItem.substring(0, maxLength - 3) + "...";
    }

    // Print the adjusted text
    canvas.println(selectedItem);
  }

  if (settingSelectLock)
  {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= flickerInterval)
    {
      previousMillis = currentMillis;
      flickerState = !flickerState; // Toggle flicker state
    }
  }

  switch (getSettingType(item_selected_settings))
  {
  case Setting::BOOLEAN:
  {
    int rectY = tftHeight / 2 - (toggleSwitchHeight / 2);
    int rectX = rect_width - textMarginX + 4 - toggleSwitchWidth; // Horizontal position (right edge with 2 pixel margin)

    if (item_selected_settings >= 0 && item_selected_settings < totalSettings)
    {
      drawToggleSwitch(rectX, rectY, settings[item_selected_settings], config.selectionBorderColor, true);
    }
    break;
  }
  case Setting::RANGE:
  {
    int rectY = tftHeight / 2;

    uint8_t currentValue = 0;

    if (item_selected_settings >= 0 && item_selected_settings < totalSettings)
    {
      currentValue = settings[item_selected_settings]->rangeValue;
    }

    // Build the text to display
    String text;

    if (settings[item_selected_settings]->range.unit != nullptr && settings[item_selected_settings]->range.unit != NULL)
    {
      text = String(currentValue) + settings[item_selected_settings]->range.unit;
    }
    else
    {
      text = String(currentValue);
    }

    // Set the text origin to the right edge
    canvas.setTextDatum(MR_DATUM); // Middle Right Datum

    // Calculate position to align text near right edge
    int rectX = rect_width - textMarginX; // Horizontal position (right edge with 2 pixel margin)

    // Display right-aligned text if flickerState is true
    if (settingSelectLock)
    {
      if (flickerState)
      {
        canvas.setTextColor(config.selectedItemColor);
      }
      else
      {
        canvas.setTextColor(config.selectionFillColor);
      }
    }
    else
    {
      canvas.setTextColor(config.selectedItemColor);
    }
    canvas.drawString(text, rectX, rectY);

    // Reset to avoid affecting other texts
    canvas.setTextDatum(TL_DATUM); // Back to default origin
    break;
  }
  case Setting::OPTION:
  {
    int rectY = tftHeight / 2;

    // Set the text origin to the right edge
    canvas.setTextDatum(MR_DATUM); // Middle Right Datum

    // Calculate position to align text near right edge
    int rectX = rect_width - textMarginX; // Horizontal position (right edge with 2 pixel margin)

    // Display right-aligned text if flickerState is true
    if (settingSelectLock)
    {
      if (flickerState)
      {
        canvas.setTextColor(config.selectedItemColor);
      }
      else
      {
        canvas.setTextColor(config.selectionFillColor);
      }
    }
    else
    {
      canvas.setTextColor(config.selectedItemColor);
    }
    const char *option = settings[item_selected_settings]->options[settings[item_selected_settings]->optionIndex];
    canvas.drawString(option, rectX, rectY);

    // Reset to avoid affecting other texts
    canvas.setTextDatum(TL_DATUM); // Back to default origin
    break;
  }
  case Setting::SUBSCREEN:
  {
    int rectY = tftHeight / 2;

    // Set the text origin to the right edge
    canvas.setTextDatum(MR_DATUM); // Middle Right Datum

    // Calculate position to align text near right edge
    int rectX = rect_width - textMarginX; // Horizontal position (right edge with 2 pixel margin)

    canvas.drawString(">", rectX, rectY);

    // Reset to avoid affecting other texts
    canvas.setTextDatum(TL_DATUM); // Back to default origin
    break;
  }
  }

  // draw next item as icon + label
  String nextItem = getSettingName(item_selected_settings_next);

  // Calculate the available width for the text
  availableWidth = calculateAvailableWidth(item_selected_settings_next, rect_width, textMarginX, toggleSwitchWidth);

  textWidth = canvas.textWidth(nextItem);

  // Check if the text fits the available width
  canvas.setFreeFont(menuFont);
  canvas.setTextSize(1);
  fontHeight = canvas.fontHeight();
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.setCursor(x2Pos, tftHeight - (tftHeight / itemNumber) + (tftHeight / itemNumber - fontHeight) / 2 + fontHeight + config.textShift);

  // If the text doesn't fit, truncate it and add "..."
  if (textWidth > availableWidth)
  {
    uint16_t maxLength = calculateMaxCharacters(nextItem.c_str(), availableWidth);
    nextItem = nextItem.substring(0, maxLength - 3) + "...";
  }

  // Print the adjusted text
  canvas.println(nextItem);

  switch (getSettingType(item_selected_settings_next))
  {
  case Setting::BOOLEAN:
  {
    int rectY = (5 * tftHeight / itemNumber - toggleSwitchHeight) / 2; // Simplified expression of (tftHeight / itemNumber - toggleSwitchHeight) / 2 + (tftHeight / itemNumber * 2)
    int rectX = rect_width - textMarginX - toggleSwitchWidth;          // Horizontal position (right edge with 2 pixel margin)
    if (item_selected_settings_next >= 0 && item_selected_settings_next < totalSettings)
    {
      drawToggleSwitch(rectX, rectY, settings[item_selected_settings_next]);
    }

    break;
  }
  case Setting::RANGE:
  {
    int rectY = tftHeight - (tftHeight / itemNumber) / 2;

    uint8_t currentValue = 0;

    if (item_selected_settings_next >= 0 && item_selected_settings_next < totalSettings)
    {
      currentValue = settings[item_selected_settings_next]->rangeValue;
    }

    // Build the text to display
    String text;

    if (settings[item_selected_settings_next]->range.unit != nullptr && settings[item_selected_settings_next]->range.unit != NULL)
    {
      text = String(currentValue) + settings[item_selected_settings_next]->range.unit;
    }
    else
    {
      text = String(currentValue);
    }

    // Set the text origin to the right edge
    canvas.setTextDatum(MR_DATUM); // Middle Right Datum

    // Calculate position to align text near right edge
    int rectX = rect_width - textMarginX; // Horizontal position (right edge with 2 pixel margin)

    // Display right-aligned text
    canvas.drawString(text, rectX, rectY);

    // Reset to avoid affecting other texts
    canvas.setTextDatum(TL_DATUM); // Back to default origin
    break;
  }
  case Setting::OPTION:
  {
    int rectY = tftHeight - (tftHeight / itemNumber) / 2;

    // Set the text origin to the right edge
    canvas.setTextDatum(MR_DATUM); // Middle Right Datum

    // Calculate position to align text near right edge
    int rectX = rect_width - textMarginX; // Horizontal position (right edge with 2 pixel margin)

    // Display right-aligned text
    const char *option = settings[item_selected_settings_next]->options[settings[item_selected_settings_next]->optionIndex];

    canvas.drawString(option, rectX, rectY);
    // Reset to avoid affecting other texts
    canvas.setTextDatum(TL_DATUM); // Back to default origin
    break;
  }
  case Setting::SUBSCREEN:
  {
    int rectY = tftHeight - (tftHeight / itemNumber) / 2;

    // Set the text origin to the right edge
    canvas.setTextDatum(MR_DATUM); // Middle Right Datum

    // Calculate position to align text near right edge
    int rectX = rect_width - textMarginX; // Horizontal position (right edge with 2 pixel margin)

    canvas.drawString(">", rectX, rectY);

    // Reset to avoid affecting other texts
    canvas.setTextDatum(TL_DATUM); // Back to default origin
    break;
  }
  }

  if (config.scrollbar)
  {
    // Draw the scrollbar
    drawScrollbar(item_selected_settings, item_selected_settings_next, totalSettings);
  }
}

uint16_t SettingsScreen::calculateAvailableWidth(int settingIndex, uint16_t rectWidth, float textMarginX, uint16_t toggleSwitchWidth)
{
  // Ensure settingIndex is within valid bounds
  if (settingIndex < 0 || settingIndex >= totalSettings)
  {
    Serial.println("Error: Invalid setting index!");
    return 0;
  }

  // Calculate initial available width considering text margins
  uint16_t availableWidth = rectWidth;
  if (textMarginX > 0)
  {
    availableWidth -= static_cast<uint16_t>(textMarginX * 2);
  }

  // Adjust available width based on setting type
  switch (getSettingType(settingIndex))
  {
  case Setting::BOOLEAN:
    if (availableWidth > toggleSwitchWidth)
    {
      availableWidth -= toggleSwitchWidth;
    }
    else
    {
      availableWidth = 0; // Ensure non-negative width
    }
    break;
  case Setting::OPTION:
  {
    uint16_t optionTextWidth = canvas.textWidth(settings[settingIndex]->options[settings[settingIndex]->optionIndex]);
    if (availableWidth > optionTextWidth)
    {
      availableWidth -= optionTextWidth;
    }
    else
    {
      availableWidth = 0; // Ensure non-negative width
    }
  }
  break;
  case Setting::RANGE:
  {
    String rangeText = String(settings[settingIndex]->rangeValue) + settings[settingIndex]->range.unit;
    uint16_t rangeTextWidth = canvas.textWidth(rangeText);
    if (availableWidth > rangeTextWidth)
    {
      availableWidth -= rangeTextWidth;
    }
    else
    {
      availableWidth = 0; // Ensure non-negative width
    }
  }
  break;
  case Setting::SUBSCREEN:
  {
    uint16_t subScreenTextWidth = canvas.textWidth(">");
    if (availableWidth > subScreenTextWidth)
    {
      availableWidth -= subScreenTextWidth;
    }
    else
    {
      availableWidth = 0; // Ensure non-negative width
    }
  }
  break;
  default:
    break;
  }

  return availableWidth;
}

void SettingsScreen::handleInput()
{
  if (useEncoder)
  {
    // --- Interrupt-driven Rotary Encoder Processing ---
    static int lastPosition = 0;

    // Only process when the encoder actually changed
    if (encoderChanged)
    {
      // Calculate the real position (each detent is 4 steps in the state machine)
      int newPosition = encoderPosition >> 2;

      // Only act when the position actually changed
      if (newPosition != lastPosition)
      {
        if (newPosition > lastPosition)
        {
          // Clockwise rotation (DOWN)
          if (settingSelectLock && settings[currentSettingIndex]->type != Setting::BOOLEAN)
          {
            modify(1, currentSettingIndex); // Increase setting value
          }
          else
          {
            item_selected_settings = (item_selected_settings + 1) % totalSettings;
          }
        }
        else
        {
          // Counterclockwise rotation (UP)
          if (settingSelectLock && settings[currentSettingIndex]->type != Setting::BOOLEAN)
          {
            modify(-1, currentSettingIndex); // Increase setting value
          }
          else
          {
            item_selected_settings = (item_selected_settings - 1 + totalSettings) % totalSettings;
          }
        }
        lastPosition = newPosition;
      }

      encoderChanged = false; // Reset the flag
    }
  }
  else
  {
    // Handle UP button
    if (BUTTON_UP_PIN != -1)
    {
      // Read current button state
      int currentUpButtonState = digitalRead(BUTTON_UP_PIN);
      static int previousUpButtonState = !buttonVoltage;
      static unsigned long upPressedTime = 0;
      static bool upIsPressing = false;
      static bool upIsLongDetected = false;
      static bool upButtonPressProcessed = false;
      static unsigned long lastUpScrollTime = 0;

      // Button press detection
      if (currentUpButtonState == buttonVoltage && !upButtonPressProcessed)
      {
        if (previousUpButtonState == !buttonVoltage)
        {
          // Button just pressed
          upPressedTime = millis();
          upIsPressing = true;
          upIsLongDetected = false;
          upButtonPressProcessed = true;
        }
      }

      // Long press detection
      if (upIsPressing && !upIsLongDetected)
      {
        long pressDuration = millis() - upPressedTime;
        if (pressDuration > LONG_PRESS_TIME_MENU)
        {
          upIsLongDetected = true;
        }
      }

      // Continuous scrolling on long press
      if (upIsPressing && upIsLongDetected)
      {
        unsigned long currentMillis = millis();
        if (currentMillis - lastUpScrollTime >= 200)
        { // Scroll every 200ms
          if (settingSelectLock && settings[currentSettingIndex]->type != Setting::BOOLEAN)
          {
            modify(1, currentSettingIndex); // Increase setting value
          }
          else
          {
            item_selected_settings--;
            if (item_selected_settings < 0)
            {
              item_selected_settings = totalSettings - 1;
            }
          }
          lastUpScrollTime = currentMillis;
        }
      }

      // Button release handling
      if (currentUpButtonState == !buttonVoltage && previousUpButtonState == buttonVoltage)
      {
        upIsPressing = false;
        unsigned long releasedTime = millis();
        long pressDuration = releasedTime - upPressedTime;

        // Short press action
        if (pressDuration < SHORT_PRESS_TIME && !upIsLongDetected)
        {
          if (settingSelectLock && settings[currentSettingIndex]->type != Setting::BOOLEAN)
          {
            modify(1, currentSettingIndex); // Increase setting value
          }
          else
          {
            item_selected_settings--;
            if (item_selected_settings < 0)
            {
              item_selected_settings = totalSettings - 1;
            }
          }
        }
        upButtonPressProcessed = false;
      }

      previousUpButtonState = currentUpButtonState;
    }

    // Handle DOWN button
    if (BUTTON_DOWN_PIN != -1)
    {
      // Read current button state
      int currentDownButtonState = digitalRead(BUTTON_DOWN_PIN);
      static int previousDownButtonState = !buttonVoltage;
      static unsigned long downPressedTime = 0;
      static bool downIsPressing = false;
      static bool downIsLongDetected = false;
      static bool downButtonPressProcessed = false;
      static unsigned long lastDownScrollTime = 0;

      // Button press detection
      if (currentDownButtonState == buttonVoltage && !downButtonPressProcessed)
      {
        if (previousDownButtonState == !buttonVoltage)
        {
          // Button just pressed
          downPressedTime = millis();
          downIsPressing = true;
          downIsLongDetected = false;
          downButtonPressProcessed = true;
        }
      }

      // Long press detection
      if (downIsPressing && !downIsLongDetected)
      {
        long pressDuration = millis() - downPressedTime;
        if (pressDuration > LONG_PRESS_TIME_MENU)
        {
          downIsLongDetected = true;
        }
      }

      // Continuous scrolling on long press
      if (downIsPressing && downIsLongDetected)
      {
        unsigned long currentMillis = millis();
        if (currentMillis - lastDownScrollTime >= 200)
        { // Scroll every 200ms
          if (settingSelectLock && settings[currentSettingIndex]->type != Setting::BOOLEAN)
          {
            modify(-1, currentSettingIndex); // Increase setting value
          }
          else
          {
            item_selected_settings++;
            if (item_selected_settings >= totalSettings)
            {
              item_selected_settings = 0;
            }
          }
          lastDownScrollTime = currentMillis;
        }
      }

      // Button release handling
      if (currentDownButtonState == !buttonVoltage && previousDownButtonState == buttonVoltage)
      {
        downIsPressing = false;
        unsigned long releasedTime = millis();
        long pressDuration = releasedTime - downPressedTime;

        // Short press action
        if (pressDuration < SHORT_PRESS_TIME && !downIsLongDetected)
        {
          if (settingSelectLock && settings[currentSettingIndex]->type != Setting::BOOLEAN)
          {
            modify(-1, currentSettingIndex); // Increase setting value
          }
          else
          {
            item_selected_settings++;
            if (item_selected_settings >= totalSettings)
            {
              item_selected_settings = 0;
            }
          }
        }
        downButtonPressProcessed = false;
      }

      previousDownButtonState = currentDownButtonState;
    }
  }

  // Calculate previous and next items for display purposes
  item_selected_settings_previous = item_selected_settings - 1;
  if (item_selected_settings_previous < 0)
  {
    item_selected_settings_previous = totalSettings - 1;
  }

  item_selected_settings_next = item_selected_settings + 1;
  if (item_selected_settings_next >= totalSettings)
  {
    item_selected_settings_next = 0;
  }

  // Handle SELECT button
  int selectButtonState = digitalRead(BUTTON_SELECT_PIN);
  static unsigned long selectPressedTime = 0;
  static bool selectIsPressing = false;
  static bool selectIsLongDetected = false;
  static bool selectButtonPressProcessed = false;

  // Button press detection
  if (selectButtonState == buttonVoltage && !selectButtonPressProcessed)
  {
    if (prevSelectState == !buttonVoltage)
    {
      selectPressedTime = millis();
      selectIsPressing = true;
      selectIsLongDetected = false;
      selectButtonPressProcessed = true;
    }
  }

  // Long press detection
  if (selectIsPressing && !selectIsLongDetected)
  {
    long pressDuration = millis() - selectPressedTime;
    if (pressDuration > SELECT_BUTTON_LONG_PRESS_DURATION)
    {
      selectIsLongDetected = true;

      // Handle long press action
      if (screenManager.canGoBack())
      {
        screenManager.popScreen(); // Go back to the previous screen
      }
    }
  }

  // Button release handling
  if (selectButtonState == !buttonVoltage && prevSelectState == buttonVoltage)
  {
    selectIsPressing = false;
    unsigned long releasedTime = millis();
    long pressDuration = releasedTime - selectPressedTime;

    // Short press action
    if (pressDuration < SHORT_PRESS_TIME && !selectIsLongDetected)
    {
      if (item_selected_settings >= 0 && item_selected_settings < totalSettings)
      {
        currentSettingIndex = item_selected_settings;
        if (settings[currentSettingIndex]->type == Setting::BOOLEAN)
        {
          modify(1, item_selected_settings); // Modify setting value (+1)
        }
        else if (settings[currentSettingIndex]->type == Setting::SUBSCREEN)
        {
          screenManager.pushScreen(settings[currentSettingIndex]->subScreen);
        }
        else
        {
          settingSelectLock = !settingSelectLock;
        }
        draw(); // Redraw after changing the setting
      }
    }
    selectButtonPressProcessed = false;
  }

  prevSelectState = selectButtonState;

  // Update the display to reflect any changes
  draw();
}

void SettingsScreen::drawToggleSwitch(int16_t x, int16_t y, Setting *setting, uint16_t bgColor, bool isSelected)
{
  uint16_t toggleSwitchHeight = tftHeight * config.toggleSwitchHeightRatio;
  uint16_t switchWidth = toggleSwitchHeight * 2;
  uint16_t knobDiameter = toggleSwitchHeight - 4;
  unsigned long currentTime = millis();

  // Get the current state from the setting parameter (renamed from settings to setting)
  bool state = setting->booleanValue;

  // Create a unique key for this toggle switch based on its setting ID
  int toggleKey = setting->id;

  // Initialize or update the toggle state if it doesn't exist or state changed
  if (toggleStates.find(toggleKey) == toggleStates.end())
  {
    // First time - initialize with the current state and position
    ToggleState newState;
    newState.currentState = state;
    newState.currentPosition = state ? switchWidth - knobDiameter - 2 : 2;
    newState.targetPosition = newState.currentPosition;
    newState.lastToggleTime = currentTime;
    newState.animating = false;
    toggleStates[toggleKey] = newState;
  }
  else if (toggleStates[toggleKey].currentState != state)
  {
    // State changed - update target position and animation state
    toggleStates[toggleKey].currentState = state;
    toggleStates[toggleKey].targetPosition = state ? switchWidth - knobDiameter - 2 : 2;
    toggleStates[toggleKey].lastToggleTime = currentTime;

    // Only set as animating if animation is enabled in config
    toggleStates[toggleKey].animating = config.animation;

    // If animation is disabled, immediately set to final position
    if (!config.animation)
    {
      toggleStates[toggleKey].currentPosition = toggleStates[toggleKey].targetPosition;
    }
  }

  // Get the current toggle state
  ToggleState &toggle = toggleStates[toggleKey];

  // Determine knob background color based on transition
  float normalizedPosition = (toggle.currentPosition - 2) / (switchWidth - knobDiameter - 4);

  // Blend colors based on position
  uint16_t knobBgColor;
  if (normalizedPosition < 0.5)
  {
    // Closer to left (red) position
    uint8_t blendFactor = 255 * (normalizedPosition * 2);
    knobBgColor = tft.color565(
        255 - blendFactor, // R: 255->0
        blendFactor,       // G: 0->255
        0                  // B: 0->0
    );
  }
  else
  {
    // Closer to right (green) position
    knobBgColor = TFT_GREEN;
  }

  // Only animate if config.animation is true
  if (config.animation && toggle.animating && isSelected)
  {
    // Calculate animation progress with time-based approach
    float animationProgress = min(1.0f, (float)(currentTime - toggle.lastToggleTime) / (1000.0f / TOGGLE_ANIMATION_SPEED));

    // Apply cubic easing for natural movement
    float easedProgress = 1.0f - pow(1.0f - animationProgress, 3);

    // Update current position
    toggle.currentPosition = toggle.currentPosition + (toggle.targetPosition - toggle.currentPosition) * easedProgress;

    // Check if animation is complete
    if (abs(toggle.currentPosition - toggle.targetPosition) < 0.5)
    {
      toggle.currentPosition = toggle.targetPosition;
      toggle.animating = false;
    }
  }
  else if (!config.animation || !isSelected)
  {
    // Immediately set to final position when animation disabled or not selected
    toggle.currentPosition = toggle.targetPosition;
    toggle.animating = false;
  }

  // Draw switch background
  canvas.fillSmoothRoundRect(x, y, switchWidth, toggleSwitchHeight, toggleSwitchHeight / 2, knobBgColor, bgColor);

  // Draw knob with current position
  int16_t knobX = x + toggle.currentPosition;
  uint16_t knobColor = TFT_WHITE;

  if (knobDiameter % 2 == 0)
  {
    // For even diameters, use a rounded rectangle
    canvas.fillSmoothRoundRect(knobX, y + toggleSwitchHeight / 2 - knobDiameter / 2, knobDiameter, knobDiameter, knobDiameter / 2, knobColor, knobBgColor);
  }
  else
  {
    // For odd diameters, use a circle
    canvas.fillSmoothCircle(knobX + knobDiameter / 2, y + toggleSwitchHeight / 2, knobDiameter / 2, knobColor, knobBgColor);
  }
}
// Constructor for MenuScreen – also sets default drawing configuration
// Private initialization method to avoid duplication
void MenuScreen::initializeDefaults()
{
  itemSize = 0;
  currentItemIndex = 0;

  // Default configuration
  config.showImages = false;
  config.scrollbar = false;
  config.selectionBorderColor = TFT_WHITE;
  config.selectionFillColor = TFT_BLACK;
  config.buttonAnimation = false;
  config.menuStyle = 0;
  config.textScroll = true;
}

MenuScreen::MenuScreen()
    : title(nullptr)
{
  initializeDefaults();
}

MenuScreen::MenuScreen(const char *title)
    : title(title)
{
  initializeDefaults();
}
// Add a menu item
void MenuScreen::addItem(const char *label, Screen *nextScreen, ActionCallback action, const uint16_t *image)
{
  MenuItem item = {
      label,
      nextScreen,
      action,
      image};

  items.push_back(item);
}
void MenuScreen::addItem(Screen *nextScreen, ActionCallback action, const uint16_t *image)
{
  const char *screenTitle = nextScreen ? nextScreen->getTitle() : "Untitled Screen";

  MenuItem item = {
      screenTitle,
      nextScreen,
      action,
      image};

  items.push_back(item);
}

int MenuScreen::getIndex()
{
  return currentItemIndex;
}

// Advanced draw() function used by every MenuScreen instance
void MenuScreen::draw()
{

  // Calculate adaptive selection rectangle dimensions
  uint16_t rect_width = tftWidth * 0.97f;

  drawSelectionRect();

  // Calculate adaptive positions for text and (optionally) icons
  float textMarginX = tftWidth * config.marginRatioX;
  float iconSize = 16; // Icon size based on screen height

  itemSize = items.size();

  // Calculate indices for previous, current, and next items
  int item_sel_previous = (currentItemIndex - 1 + itemSize) % itemSize;
  int item_selected = currentItemIndex;
  int item_sel_next = (currentItemIndex + 1) % itemSize;

  int xPos = items[item_sel_previous].image != nullptr ? textMarginX + iconSize : textMarginX;
  int x1Pos = items[item_selected].image != nullptr ? textMarginX + iconSize : textMarginX;
  int x2Pos = items[item_sel_next].image != nullptr ? textMarginX + iconSize : textMarginX;

  int16_t fontHeight;

  // Y positions for the three displayed items (previous, selected, next)
  int itemNumber = 3;
  int yPos = tftHeight / 3;  // previous
  int y1Pos = tftHeight / 2; // selected (centered)
  int y2Pos = tftHeight;     // next

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
  if (textWidth > availableWidth)
  {
    // If the text doesn't fit, shorten it and add "..."
    uint16_t maxLength = calculateMaxCharacters(previousItem.c_str(), availableWidth);
    previousItem = previousItem.substring(0, maxLength - 3) + "...";
  }

  // Set the cursor position for the previous item
  canvas.setCursor(xPos, (tftHeight / itemNumber - fontHeight) / 2 + fontHeight + config.textShift);

  // Print the adjusted text
  canvas.println(previousItem);

  // Draw previous icon if present
  if (items[item_sel_previous].image != nullptr)
  {
    float iconPosY = ((tftHeight / itemNumber) - iconSize) / 2;
    canvas.pushImage(textMarginX / 2, iconPosY, iconSize, iconSize, (uint16_t *)items[item_sel_previous].image); // Example position for the image
  }

  // --- Draw selected item ---
  // Set the font and text size once (don't repeat in the loop)
  canvas.setFreeFont(menuFontBold);
  canvas.setTextSize(1);
  fontHeight = canvas.fontHeight();

  textWidth = canvas.textWidth(items[item_selected].label);
  availableWidth = items[item_selected].image != nullptr ? scrollWindowSizeImage : scrollWindowSize;

  // Check if the text needs to scroll
  if (strlen(items[item_selected].label) > calculateMaxCharacters(items[item_selected].label, availableWidth) && config.textScroll)
  {
    // Call the scroll function if the text is too long
    scrollTextHorizontal(x1Pos, tftHeight / 2 + (fontHeight / 2) + config.textShift,
                         items[item_selected].label,
                         config.selectedItemColor, config.selectionFillColor, 1, 50, availableWidth);
  }
  else
  {
    // Set the text color and cursor for static text
    canvas.setTextColor(config.selectedItemColor, config.selectionFillColor);
    canvas.setCursor(x1Pos, tftHeight / 2 + (fontHeight / 2) + config.textShift);

    // Prepare the string to display
    String selectedItem = String(items[item_selected].label);
    if (textWidth > availableWidth)
    {
      // If the text doesn't fit, shorten it and add "..."
      uint16_t maxLength = calculateMaxCharacters(previousItem.c_str(), availableWidth);
      selectedItem = previousItem.substring(0, maxLength - 3) + "...";
    }

    // Print the string
    canvas.println(selectedItem);
  }

  // Draw selected icon if present
  if (items[item_selected].image != nullptr)
  {
    float iconPosY = (tftHeight - iconSize) / 2;
    canvas.pushImage(textMarginX / 2, iconPosY, iconSize, iconSize, (uint16_t *)items[item_selected].image); // Example position for the image
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
  if (textWidth > availableWidth)
  {
    // If the text doesn't fit, shorten it and add "..."
    uint16_t maxLength = calculateMaxCharacters(nextItem.c_str(), availableWidth);
    nextItem = nextItem.substring(0, maxLength - 3) + "...";
  }

  // Set the cursor position for the next item
  canvas.setCursor(x2Pos, tftHeight - (tftHeight / itemNumber) + (tftHeight / itemNumber - fontHeight) / 2 + fontHeight + config.textShift);

  // Print the adjusted text
  canvas.println(nextItem);

  // Draw next icon if present
  if (items[item_sel_next].image != nullptr)
  {
    float iconPosY = tftHeight - ((tftHeight / itemNumber) + iconSize) / 2;
    canvas.pushImage(textMarginX / 2, iconPosY, iconSize, iconSize, (uint16_t *)items[item_sel_next].image); // Example position for the image
  }

  // Draw the scrollbar if activated
  if (config.scrollbar)
  {
    drawScrollbar(item_selected, item_sel_next, itemSize);
  }
}
// void MenuScreen::draw() {
//   // Update display dimensions
//   uint16_t tftWidth = tft.width();
//   uint16_t tftHeight = tft.height();

//   // Calculate adaptive selection rectangle dimensions
//   uint16_t rect_height = tftHeight * config.itemHeightRatio;
//   uint16_t rect_width = tftWidth * 0.97f;

//   // Center the selection rectangle vertically
//   uint16_t rect_x = 0;
//   uint16_t rect_y = (tftHeight - rect_height) / 2;

//   // Animation variables
//   static int previousItemIndex = currentItemIndex;
//   static float animationProgress = 1.0f;  // 0.0 = animation start, 1.0 = animation complete
//   static unsigned long lastFrameTime = 0;
//   static float direction = 0.0f;  // 1 for scrolling down, -1 for scrolling up

//   // Check if item has changed
//   if (previousItemIndex != currentItemIndex) {
//     // Item has changed, start animation
//     animationProgress = 0.0f;

//     // Determine the scroll direction
//     // Need to handle wrap-around cases correctly:
//     // - Going from last item to first (wrapping forward)
//     // - Going from first item to last (wrapping backward)
//     if ((currentItemIndex > previousItemIndex && !(currentItemIndex == itemSize - 1 && previousItemIndex == 0)) || (currentItemIndex == 0 && previousItemIndex == itemSize - 1)) {
//       direction = 1.0f;  // Items are scrolling down (new item coming from top)
//     } else {
//       direction = -1.0f;  // Items are scrolling up (new item coming from bottom)
//     }

//     previousItemIndex = currentItemIndex;
//   }

//   // Animation timing (300ms for full animation)
//   const float animationDuration = 300.0f;  // milliseconds
//   unsigned long currentTime = millis();
//   unsigned long deltaTime = currentTime - lastFrameTime;
//   lastFrameTime = currentTime;

//   // Update animation progress
//   if (animationProgress < 1.0f) {
//     animationProgress += deltaTime / animationDuration;
//     if (animationProgress > 1.0f) animationProgress = 1.0f;
//   }

//   // Easing function for smoother animation (ease-out cubic)
//   float easedProgress = 1.0f - pow(1.0f - animationProgress, 3);

//   // Clear areas to remove previous text by drawing rounded rectangles
//   float clearMargin = tftHeight * config.marginRatioY;
//   canvas.fillRoundRect(rect_x + 1, rect_y - rect_height - clearMargin,
//                        rect_width - 3, rect_height - 3, 4, TFT_BLACK);
//   canvas.fillRoundRect(rect_x + 1, rect_y - 1,
//                        rect_width - 3, rect_height - 3, 4, TFT_BLACK);
//   canvas.fillRoundRect(rect_x + 1, rect_y + rect_height + clearMargin,
//                        rect_width - 3, rect_height - 3, 4, TFT_BLACK);

//   uint16_t selectedItemColor;

//   // Draw selection rectangle based on style and button state
//   if (config.menuStyle == 0) {
//     if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage && config.buttonAnimation) {
//       canvas.drawSmoothRoundRect(rect_x + 1, rect_y + 1, 4, 4,
//                                  rect_width - 2, rect_height - 1, config.selectionBorderColor, TFT_BLACK);
//     } else {
//       if (!config.scrollbar) {
//         canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width, rect_height, config.selectionBorderColor, TFT_BLACK);
//         uint16_t lineLength = rect_height * 0.9f;
//         canvas.drawFastVLine(tftWidth - 2, rect_y + tftHeight * 0.05f, lineLength, config.selectionBorderColor);
//         canvas.drawFastVLine(tftWidth, rect_y + tftHeight * 0.05f, lineLength - 1, config.selectionBorderColor);
//         canvas.drawFastHLine(2, rect_y + rect_height, rect_width - 3, config.selectionBorderColor);
//         canvas.drawFastHLine(3, rect_y + rect_height, rect_width - 4, config.selectionBorderColor);
//       } else {
//         canvas.drawSmoothRoundRect(rect_x, rect_y, 4, 4, rect_width - 2, rect_height, config.selectionBorderColor, TFT_BLACK);
//         uint16_t lineLength = rect_height * 0.9f;
//         canvas.drawFastVLine(rect_width - 4, rect_y + 2, lineLength, config.selectionBorderColor);
//         canvas.drawFastVLine(rect_width - 3, rect_y + 2, lineLength - 1, config.selectionBorderColor);
//         canvas.drawFastHLine(2, rect_y + rect_height, rect_width * 0.95f, config.selectionBorderColor);
//         canvas.drawFastHLine(3, rect_y + rect_height + 1, rect_width * 0.95f - 1, config.selectionBorderColor);
//       }
//     }
//     config.selectedItemColor = TFT_WHITE;
//   } else if (config.menuStyle == 1) {
//     canvas.fillSmoothRoundRect(rect_x, rect_y, rect_width, rect_height, 4, config.selectionBorderColor, TFT_BLACK);
//     config.selectedItemColor = TFT_BLACK;
//   }

//   // Calculate adaptive positions for text and (optionally) icons
//   float textMarginX = tftWidth * config.marginRatioX;
//   float iconSize = 16;  // Icon size based on screen height

//   // Calculate indices for previous, current, and next items
//   int item_sel_previous = (currentItemIndex - 1 + itemSize) % itemSize;
//   int item_selected = currentItemIndex;
//   int item_sel_next = (currentItemIndex + 1) % itemSize;

//   int xPos = items[item_sel_previous].image != nullptr ? textMarginX + iconSize : textMarginX;
//   int x1Pos = items[item_selected].image != nullptr ? textMarginX + iconSize : textMarginX;
//   int x2Pos = items[item_sel_next].image != nullptr ? textMarginX + iconSize : textMarginX;

//   int16_t fontHeight;

//   // Y positions for the three displayed items
//   int itemNumber = 3;
//   canvas.setFreeFont(menuFont);
//   canvas.setTextSize(1);
//   fontHeight = canvas.fontHeight();

//   // Base Y positions
//   float baseYprev = (tftHeight / itemNumber - fontHeight) / 2 + fontHeight / 2;
//   float baseYcurrent = tftHeight / 2;
//   float baseYnext = tftHeight - (tftHeight / itemNumber) + (tftHeight / itemNumber - fontHeight) / 2 + fontHeight / 2;

//   // Apply animation to y positions - items slide in/out
//   float yOffset = direction * (1.0f - easedProgress) * (tftHeight / itemNumber);
//   float yPos = baseYprev + yOffset;
//   float y1Pos = baseYcurrent + yOffset;
//   float y2Pos = baseYnext + yOffset;

//   // Calculate fade effect for transition
//   uint8_t prevOpacity = easedProgress * 255;
//   uint8_t nextOpacity = easedProgress * 255;
//   uint8_t currentOpacity = 255;

//   int scrollWindowSize = rect_width - (textMarginX * 2);
//   int scrollWindowSizeImage = rect_width - ((textMarginX * 2) + iconSize);

//   uint16_t textWidth;
//   uint16_t availableWidth;

//   // --- Draw previous item ---
//   canvas.setFreeFont(menuFont);
//   canvas.setTextSize(1);
//   fontHeight = canvas.fontHeight();

//   // Apply fade effect with animation
//   uint16_t prevTextColor = alphaBlend(TFT_WHITE, TFT_BLACK, prevOpacity);
//   canvas.setTextColor(prevTextColor, TFT_BLACK);

//   // Calculate available width for the text
//   textWidth = canvas.textWidth(items[item_sel_previous].label);
//   availableWidth = items[item_sel_previous].image != nullptr ? scrollWindowSizeImage : scrollWindowSize;

//   // Dynamically adjust the text to fit the available space
//   String previousItem = String(items[item_sel_previous].label);
//   if (textWidth > availableWidth) {
//     // If the text doesn't fit, shorten it and add "..."
//     uint16_t maxLength = calculateMaxCharacters(previousItem.c_str(), availableWidth);
//     previousItem = previousItem.substring(0, maxLength - 3) + "...";
//   }

//   // Set the cursor position for the previous item (with animation)
//   canvas.setCursor(xPos, yPos + (fontHeight / 2) + config.textShift);

//   // Print the adjusted text
//   canvas.println(previousItem);

//   // Draw previous icon if present
//   if (items[item_sel_previous].image != nullptr) {
//     float iconPosY = yPos - (fontHeight / 2);
//     // Add fade effect to icon
//     canvas.pushImage(textMarginX / 2, iconPosY, iconSize, iconSize, (uint16_t*)items[item_sel_previous].image);
//   }

//   // --- Draw selected item ---
//   // Set the font and text size once
//   canvas.setFreeFont(menuFontBold);
//   canvas.setTextSize(1);
//   fontHeight = canvas.fontHeight();

//   textWidth = canvas.textWidth(items[item_selected].label);
//   availableWidth = items[item_selected].image != nullptr ? scrollWindowSizeImage : scrollWindowSize;

//   // Check if the text needs to scroll
//   if (strlen(items[item_selected].label) > calculateMaxCharacters(items[item_selected].label, availableWidth) && config.textScroll) {
//     // Call the scroll function if the text is too long
//     scrollTextHorizontal(x1Pos, y1Pos + (fontHeight / 2) + config.textShift,
//                          items[item_selected].label,
//                          config.selectedItemColor, config.selectionFillColor, 1, 50, availableWidth);
//   } else {
//     // Set the text color and cursor for static text
//     canvas.setTextColor(config.selectedItemColor, config.selectionFillColor);
//     canvas.setCursor(x1Pos, y1Pos + (fontHeight / 2) + config.textShift);

//     // Prepare the string to display
//     String selectedItem = String(items[item_selected].label);
//     if (textWidth > availableWidth) {
//       // If the text doesn't fit, shorten it and add "..."
//       uint16_t maxLength = calculateMaxCharacters(selectedItem.c_str(), availableWidth);
//       selectedItem = selectedItem.substring(0, maxLength - 3) + "...";
//     }

//     // Print the string
//     canvas.println(selectedItem);
//   }

//   // Draw selected icon if present
//   if (items[item_selected].image != nullptr) {
//     float iconPosY = y1Pos - (fontHeight / 2);
//     canvas.pushImage(textMarginX / 2, iconPosY, iconSize, iconSize, (uint16_t*)items[item_selected].image);
//   }

//   // --- Draw next item ---
//   canvas.setFreeFont(menuFont);
//   canvas.setTextSize(1);
//   fontHeight = canvas.fontHeight();

//   // Apply fade effect with animation
//   uint16_t nextTextColor = alphaBlend(TFT_WHITE, TFT_BLACK, nextOpacity);
//   canvas.setTextColor(nextTextColor, TFT_BLACK);

//   // Calculate available width for the text
//   textWidth = canvas.textWidth(items[item_sel_next].label);
//   availableWidth = items[item_sel_next].image != nullptr ? scrollWindowSizeImage : scrollWindowSize;

//   // Dynamically adjust the text to fit the available space
//   String nextItem = String(items[item_sel_next].label);
//   if (textWidth > availableWidth) {
//     // If the text doesn't fit, shorten it and add "..."
//     uint16_t maxLength = calculateMaxCharacters(nextItem.c_str(), availableWidth);
//     nextItem = nextItem.substring(0, maxLength - 3) + "...";
//   }

//   // Set the cursor position for the next item (with animation)
//   canvas.setCursor(x2Pos, y2Pos + (fontHeight / 2) + config.textShift);

//   // Print the adjusted text
//   canvas.println(nextItem);

//   // Draw next icon if present
//   if (items[item_sel_next].image != nullptr) {
//     float iconPosY = y2Pos - (fontHeight / 2);
//     // Add fade effect to icon
//     canvas.pushImage(textMarginX / 2, iconPosY, iconSize, iconSize, (uint16_t*)items[item_sel_next].image);
//   }

//   // Draw the scrollbar if activated
//   if (config.scrollbar) {
//     drawScrollbar(item_selected, item_sel_next, itemSize);
//   }
// }

// Alpha blending helper function
uint16_t MenuScreen::alphaBlend(uint16_t fg, uint16_t bg, uint8_t alpha)
{
  // Extract RGB components
  uint8_t fgR = (fg >> 11) & 0x1F;
  uint8_t fgG = (fg >> 5) & 0x3F;
  uint8_t fgB = fg & 0x1F;

  uint8_t bgR = (bg >> 11) & 0x1F;
  uint8_t bgG = (bg >> 5) & 0x3F;
  uint8_t bgB = bg & 0x1F;

  // Blend based on alpha
  uint8_t outR = ((fgR * alpha) + (bgR * (255 - alpha))) / 255;
  uint8_t outG = ((fgG * alpha) + (bgG * (255 - alpha))) / 255;
  uint8_t outB = ((fgB * alpha) + (bgB * (255 - alpha))) / 255;

  // Combine back into 16-bit color
  return (outR << 11) | (outG << 5) | outB;
}
// Handle input remains similar to the previous implementation
void MenuScreen::handleInput()
{
  if (useEncoder)
  {
    // --- Interrupt-driven Rotary Encoder Processing ---
    static int lastPosition = 0;

    // Only process when the encoder actually changed
    if (encoderChanged)
    {
      // Calculate the real position (each detent is 4 steps in the state machine)
      int newPosition = encoderPosition >> 2;

      // Only act when the position actually changed
      if (newPosition != lastPosition)
      {
        if (newPosition > lastPosition)
        {
          // Clockwise rotation (DOWN)
          currentItemIndex = (currentItemIndex + 1) % itemSize;
        }
        else
        {
          // Counterclockwise rotation (UP)
          currentItemIndex = (currentItemIndex - 1 + itemSize) % itemSize;
        }
        lastPosition = newPosition;
      }

      encoderChanged = false; // Reset the flag
    }
  }
  else
  {
    // --- UP Button Processing ---
    if (BUTTON_UP_PIN != -1)
    {
      static int prevUpState = !buttonVoltage;
      static bool isPressingUp = false;
      static bool isLongDetectedUp = false;
      static bool upProcessed = false;
      static unsigned long upPressedTime = 0;
      static unsigned long lastUpAdjustTime = 0;

      int upState = digitalRead(BUTTON_UP_PIN);

      // Detect press start
      if (upState == buttonVoltage && !upProcessed)
      {
        if (prevUpState == !buttonVoltage)
        {
          upPressedTime = millis();
          isPressingUp = true;
          isLongDetectedUp = false;
          upProcessed = true;
        }
      }
      // Check for long press while held
      if (isPressingUp && !isLongDetectedUp)
      {
        long pressDuration = millis() - upPressedTime;
        if (pressDuration > LONG_PRESS_TIME_MENU)
        {
          isLongDetectedUp = true;
        }
      }
      // While held with long press detected, update every 200 ms
      if (isPressingUp && isLongDetectedUp)
      {
        unsigned long currentMillis = millis();
        if (currentMillis - lastUpAdjustTime >= 200)
        {
          currentItemIndex = (currentItemIndex - 1 + itemSize) % itemSize;
          lastUpAdjustTime = currentMillis;
        }
      }
      // On release, if short press then update once
      if (upState == !buttonVoltage && prevUpState == buttonVoltage)
      {
        isPressingUp = false;
        unsigned long upReleasedTime = millis();
        long pressDuration = upReleasedTime - upPressedTime;
        if (pressDuration < SHORT_PRESS_TIME && !isLongDetectedUp)
        {
          currentItemIndex = (currentItemIndex - 1 + itemSize) % itemSize;
        }
        upProcessed = false;
      }
      prevUpState = upState;
    }

    // --- DOWN Button Processing ---
    if (BUTTON_DOWN_PIN != -1)
    {
      static int prevDownState = !buttonVoltage;
      static bool isPressingDown = false;
      static bool isLongDetectedDown = false;
      static bool downProcessed = false;
      static unsigned long downPressedTime = 0;
      static unsigned long lastDownAdjustTime = 0;

      int downState = digitalRead(BUTTON_DOWN_PIN);

      if (downState == buttonVoltage && !downProcessed)
      {
        if (prevDownState == !buttonVoltage)
        {
          downPressedTime = millis();
          isPressingDown = true;
          isLongDetectedDown = false;
          downProcessed = true;
        }
      }
      if (isPressingDown && !isLongDetectedDown)
      {
        long pressDuration = millis() - downPressedTime;
        if (pressDuration > LONG_PRESS_TIME_MENU)
        {
          isLongDetectedDown = true;
        }
      }
      if (isPressingDown && isLongDetectedDown)
      {
        unsigned long currentMillis = millis();
        if (currentMillis - lastDownAdjustTime >= 200)
        {
          currentItemIndex = (currentItemIndex + 1) % itemSize;
          lastDownAdjustTime = currentMillis;
        }
      }
      if (downState == !buttonVoltage && prevDownState == buttonVoltage)
      {
        isPressingDown = false;
        unsigned long downReleasedTime = millis();
        long pressDuration = downReleasedTime - downPressedTime;
        if (pressDuration < SHORT_PRESS_TIME && !isLongDetectedDown)
        {
          currentItemIndex = (currentItemIndex + 1) % itemSize;
        }
        downProcessed = false;
      }
      prevDownState = downState;
    }
  }

  // --- SELECT Button Processing ---
  static bool isPressingSelect = false;
  static bool isLongDetectedSelect = false;
  static bool selectProcessed = false;
  static unsigned long selectPressedTime = 0;

  int selectState = digitalRead(BUTTON_SELECT_PIN);

  // Detect press start
  if (selectState == buttonVoltage && !selectProcessed)
  {
    if (prevSelectState == !buttonVoltage)
    {
      selectPressedTime = millis();
      isPressingSelect = true;
      isLongDetectedSelect = false;
      selectProcessed = true;
    }
  }

  // Long press detection
  if (isPressingSelect && !isLongDetectedSelect)
  {
    long pressDuration = millis() - selectPressedTime;
    if (pressDuration > SELECT_BUTTON_LONG_PRESS_DURATION)
    {
      isLongDetectedSelect = true;

      // Long press action: go back to previous screen
      if (screenManager.canGoBack())
      {
        screenManager.popScreen();
      }
    }
  }

  // On release
  if (selectState == !buttonVoltage && prevSelectState == buttonVoltage)
  {
    isPressingSelect = false;
    unsigned long selectReleasedTime = millis();
    long pressDuration = selectReleasedTime - selectPressedTime;

    // Short press: execute current menu item action only if long press wasn't detected
    if (pressDuration < SHORT_PRESS_TIME && !isLongDetectedSelect)
    {
      MenuItem &item = items[currentItemIndex];
      if (item.action != nullptr)
      {
        item.action();
      }

      // Only push to next screen if not already handled by long press
      if (item.nextScreen != nullptr)
      {
        screenManager.pushScreen(item.nextScreen);
      }
    }

    selectProcessed = false;
  }

  prevSelectState = selectState;

  // Update the display to reflect any changes.
  draw();
}
CustomScreen::CustomScreen()
    : title(nullptr)
{
}

CustomScreen::CustomScreen(const char *title)
    : title(title)
{
}

void CustomScreen::handleInput()
{
  // --- SELECT Button Processing ---
  static bool selectClicked = false;
  static bool longPressHandled = false;
  static unsigned long selectPressTime = 0;

  if (digitalRead(BUTTON_SELECT_PIN) == buttonVoltage)
  {
    // Button pressed: record press start if not already clicked
    if (!selectClicked && !longPressHandled)
    {
      selectPressTime = millis();
      selectClicked = true;
    }
    else
    {
      // While still pressed, check for long press
      if ((millis() - selectPressTime >= SELECT_BUTTON_LONG_PRESS_DURATION) && !longPressHandled)
      {
        // Long press: go back to the last menu.
        if (screenManager.canGoBack())
        {
          screenManager.popScreen(); // Go back to the previous screen
        }
        longPressHandled = true;
        prevSelectState = buttonVoltage;
      }
    }
  }
  else if (digitalRead(BUTTON_SELECT_PIN) == !buttonVoltage)
  {
    // On button release
    if (selectClicked)
    {
      // Reset select button state.
      selectClicked = false;
      longPressHandled = false;
    }
  }

  // Update the display to reflect any changes.
  draw();
}

void OpenMenuOS::redirectToScreen(Screen *screen)
{
  screenManager.pushScreen(screen);
}

void OpenMenuOS::navigateBack()
{
  if (screenManager.canGoBack())
  {
    screenManager.popScreen();
  }
}

//--------------------------------------------------------------------------
// PopupManager Implementation
//--------------------------------------------------------------------------

// Popup styling constants
constexpr uint16_t POPUP_MARGIN = 8;
constexpr uint16_t POPUP_RADIUS = 6;
constexpr uint16_t HEADER_HEIGHT_RATIO = 35; // percentage
constexpr uint16_t BUTTON_HEIGHT = 28;
constexpr uint16_t BUTTON_MARGIN = 6;
constexpr uint32_t DEBOUNCE_TIME = 200;

// Static member definitions
PopupResult PopupManager::currentResult = PopupResult::NONE;
bool PopupManager::isVisible = false;
uint32_t PopupManager::showTime = 0;
PopupConfig PopupManager::currentConfig;
uint32_t PopupManager::lastButtonTime = 0;
int PopupManager::selectedButton = 0;

const PopupManager::PopupColors PopupManager::colorSchemes[] = {
    // INFO
    {0x451F, TFT_WHITE, TFT_BLACK, 0x451F, TFT_WHITE, "Information", nullptr, 0, 0},
    // SUCCESS
    {0x07E0, TFT_WHITE, TFT_BLACK, 0x07E0, TFT_WHITE, "Success", nullptr, 0, 0},
    // WARNING
    {0xFD20, TFT_WHITE, TFT_BLACK, 0xFD20, TFT_WHITE, "Warning", nullptr, 0, 0},
    // ERROR
    {0xF800, TFT_WHITE, TFT_BLACK, 0xF800, TFT_WHITE, "Error", nullptr, 0, 0},
    // QUESTION
    {0x07FF, TFT_WHITE, TFT_BLACK, 0x07FF, TFT_WHITE, "Question", nullptr, 0, 0}};

PopupResult PopupManager::show(const PopupConfig &config)
{
  currentConfig = config;
  currentResult = PopupResult::NONE;
  isVisible = true;
  showTime = millis();
  selectedButton = 0; // Reset to OK/Yes button when showing popup
  return PopupResult::NONE;
}

PopupResult PopupManager::showInfo(const char *message, const char *title)
{
  PopupConfig config;
  config.message = message;
  config.title = title;
  config.type = PopupType::INFO;
  return show(config);
}

PopupResult PopupManager::showSuccess(const char *message, const char *title)
{
  PopupConfig config;
  config.message = message;
  config.title = title;
  config.type = PopupType::SUCCESS;
  config.autoClose = true;
  return show(config);
}

PopupResult PopupManager::showWarning(const char *message, const char *title)
{
  PopupConfig config;
  config.message = message;
  config.title = title;
  config.type = PopupType::WARNING;
  return show(config);
}

PopupResult PopupManager::showError(const char *message, const char *title)
{
  PopupConfig config;
  config.message = message;
  config.title = title;
  config.type = PopupType::ERROR;
  return show(config);
}

PopupResult PopupManager::showQuestion(const char *message, const char *title)
{
  PopupConfig config;
  config.message = message;
  config.title = title;
  config.type = PopupType::QUESTION;
  config.showCancelButton = true;
  return show(config);
}

bool PopupManager::isActive()
{
  return isVisible;
}

PopupResult PopupManager::update()
{
  if (!isVisible)
    return PopupResult::NONE;

  handleInput();

  // Auto-close logic
  if (currentConfig.autoClose && (millis() - showTime) > currentConfig.autoCloseDelay)
  {
    hide();
    return PopupResult::OK;
  }

  // Calculate popup dimensions
  uint16_t popupWidth = tftWidth - (POPUP_MARGIN * 2);
  uint16_t popupHeight = tftHeight - (POPUP_MARGIN * 2);
  uint16_t popupX = POPUP_MARGIN;
  uint16_t popupY = POPUP_MARGIN;

  // Draw popup
  const PopupColors &colors = colorSchemes[static_cast<int>(currentConfig.type)];
  drawBackground(popupX, popupY, popupWidth, popupHeight);

  uint16_t headerHeight = (popupHeight * HEADER_HEIGHT_RATIO) / 100;
  drawHeader(popupX, popupY, popupWidth, headerHeight, colors);
  drawContent(popupX, popupY + headerHeight, popupWidth, popupHeight - headerHeight, colors);

  return currentResult;
}

void PopupManager::drawBackground(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
  // Create semi-transparent overlay effect by dimming the existing content
  // Since TFT_eSPI doesn't have native alpha blending, we'll use a pattern-based approach
  uint16_t overlayColor = 0x1082; // Dark semi-transparent looking color

  // Draw overlay pattern to create semi-transparent effect
  for (int py = 0; py < tftHeight; py += 2)
  {
    for (int px = (py % 4); px < tftWidth; px += 4)
    {
      canvas.drawPixel(px, py, overlayColor);
    }
  }

  // Popup shadow
  canvas.fillSmoothRoundRect(x + 3, y + 3, width, height, POPUP_RADIUS, 0x2104, TFT_TRANSPARENT);

  // Main popup background
  canvas.fillSmoothRoundRect(x, y, width, height, POPUP_RADIUS, TFT_WHITE, TFT_TRANSPARENT);
}

void PopupManager::drawHeader(uint16_t x, uint16_t y, uint16_t width, uint16_t headerHeight, const PopupColors &colors)
{
  // Header background
  canvas.fillSmoothRoundRect(x, y, width, headerHeight + POPUP_RADIUS, POPUP_RADIUS, colors.headerColor, TFT_WHITE);
  canvas.fillRect(x, y + headerHeight - POPUP_RADIUS, width, POPUP_RADIUS, colors.headerColor);

  // Title text
  const char *title = currentConfig.title ? currentConfig.title : colors.defaultTitle;
  canvas.setFreeFont(menuFontBold);
  canvas.setTextSize(1);
  canvas.setTextColor(colors.buttonTextColor, colors.headerColor);

  uint16_t titleWidth = canvas.textWidth(title);
  uint16_t titleX = x + (width - titleWidth) / 2;
  uint16_t titleY = y + (headerHeight / 2) + (canvas.fontHeight() / 2);

  canvas.setCursor(titleX, titleY);
  canvas.print(title);

  // Draw icon if available
  drawIcon(x + 12, y + (headerHeight - 16) / 2, colors);
}

void PopupManager::drawContent(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const PopupColors &colors)
{
  if (!currentConfig.message)
    return;

  uint16_t contentHeight = currentConfig.showButtons ? height - BUTTON_HEIGHT - BUTTON_MARGIN : height;
  uint16_t messageY = y + 12;
  uint16_t messageMaxWidth = width - 24;

  drawText(currentConfig.message, x + 12, messageY, messageMaxWidth, colors.textColor);

  if (currentConfig.showButtons)
  {
    drawButtons(x, y + contentHeight, width, BUTTON_HEIGHT, colors);
  }
}

void PopupManager::drawButtons(uint16_t x, uint16_t y, uint16_t width, uint16_t buttonHeight, const PopupColors &colors)
{
  uint16_t buttonWidth = currentConfig.showCancelButton ? (width - 36) / 2 : width - 24;
  uint16_t okButtonX = currentConfig.showCancelButton ? x + 12 : x + (width - buttonWidth) / 2;
  uint16_t cancelButtonX = x + width - buttonWidth - 12;

  // Determine button colors based on encoder selection (if encoder is enabled)
  bool isOkSelected = !useEncoder || (selectedButton == 0);
  bool isCancelSelected = useEncoder && (selectedButton == 1);

  // OK/Yes button
  uint16_t okButtonColor = isOkSelected ? colors.buttonColor : 0x7BEF;
  uint16_t okTextColor = isOkSelected ? colors.buttonTextColor : TFT_BLACK;

  canvas.fillSmoothRoundRect(okButtonX, y + 6, buttonWidth, buttonHeight - 6, 4, okButtonColor, colors.backgroundColor);
  canvas.setFreeFont(menuFont);
  canvas.setTextColor(okTextColor, okButtonColor);

  const char *okText = (currentConfig.type == PopupType::QUESTION) ? "Yes" : "OK";
  uint16_t okTextWidth = canvas.textWidth(okText);
  uint16_t okTextX = okButtonX + (buttonWidth - okTextWidth) / 2;
  uint16_t okTextY = y + 6 + (buttonHeight / 2) + 3;

  canvas.setCursor(okTextX, okTextY);
  canvas.print(okText);

  // Cancel/No button
  if (currentConfig.showCancelButton)
  {
    uint16_t cancelButtonColor = isCancelSelected ? colors.buttonColor : 0x7BEF;
    uint16_t cancelTextColor = isCancelSelected ? colors.buttonTextColor : TFT_BLACK;
    canvas.fillSmoothRoundRect(cancelButtonX, y + 6, buttonWidth, buttonHeight - 6, 4, cancelButtonColor, colors.backgroundColor);
    canvas.setTextColor(cancelTextColor, cancelButtonColor);

    const char *cancelText = (currentConfig.type == PopupType::QUESTION) ? "No" : "Cancel";
    uint16_t cancelTextWidth = canvas.textWidth(cancelText);
    uint16_t cancelTextX = cancelButtonX + (buttonWidth - cancelTextWidth) / 2;

    canvas.setCursor(cancelTextX, okTextY);
    canvas.print(cancelText);
  }
}

void PopupManager::drawIcon(uint16_t x, uint16_t y, const PopupColors &colors)
{
  if (currentConfig.customIcon)
  {
    canvas.pushImage(x, y, currentConfig.customIconWidth, currentConfig.customIconHeight, currentConfig.customIcon);
  }
  else if (colors.defaultIcon)
  {
    canvas.pushImage(x, y, colors.iconWidth, colors.iconHeight, colors.defaultIcon);
  }
}

void PopupManager::drawText(const char *text, uint16_t x, uint16_t y, uint16_t maxWidth, uint16_t textColor, bool bold)
{
  if (!text)
    return;

  canvas.setFreeFont(bold ? menuFontBold : menuFont);
  canvas.setTextSize(1);
  canvas.setTextColor(textColor, TFT_WHITE);

  // Simple word wrapping
  String textStr = String(text);
  uint16_t currentY = y;
  uint16_t lineHeight = canvas.fontHeight() + 4;

  int start = 0;
  while (start < textStr.length())
  {
    int end = textStr.length();

    // Find the longest substring that fits
    for (int i = start + 1; i <= textStr.length(); i++)
    {
      String substr = textStr.substring(start, i);
      if (canvas.textWidth(substr.c_str()) > maxWidth)
      {
        end = i - 1;
        break;
      }
    }

    // Find last space to avoid breaking words
    if (end < textStr.length())
    {
      int lastSpace = textStr.lastIndexOf(' ', end);
      if (lastSpace > start)
      {
        end = lastSpace;
      }
    }

    String line = textStr.substring(start, end);
    canvas.setCursor(x, currentY + canvas.fontHeight() - 2);
    canvas.print(line);

    currentY += lineHeight;
    start = end + 1;

    // Skip leading spaces
    while (start < textStr.length() && textStr.charAt(start) == ' ')
    {
      start++;
    }
  }
}

void PopupManager::handleInput()
{
  if (!currentConfig.showButtons)
    return;

  uint32_t currentTime = millis();
  if (currentTime - lastButtonTime < DEBOUNCE_TIME)
    return;
  // Static variables for proper button state tracking (like main menu system)
  static int prevSelectState = !buttonVoltage;
  static int prevUpState = !buttonVoltage;
  static bool selectButtonPressProcessed = false;
  static bool upButtonPressProcessed = false;

  // Static variables for encoder support in popups
  static int lastEncoderPosition = 0;

  if (useEncoder)
  {
    // --- Encoder Processing for Button Selection ---
    if (encoderChanged && currentConfig.showCancelButton)
    {
      // Calculate the real position (each detent is 4 steps in the state machine)
      int newPosition = encoderPosition >> 2;

      // Only act when the position actually changed
      if (newPosition != lastEncoderPosition)
      {
        if (newPosition > lastEncoderPosition)
        {
          // Clockwise rotation - move to next button
          selectedButton = (selectedButton + 1) % 2;
        }
        else
        {
          // Counterclockwise rotation - move to previous button
          selectedButton = (selectedButton - 1 + 2) % 2;
        }
        lastEncoderPosition = newPosition;
        lastButtonTime = currentTime;
      }

      encoderChanged = false; // Reset the flag
    }

    // SELECT button with encoder - confirms the selected button
    int selectButtonState = digitalRead(BUTTON_SELECT_PIN);

    if (selectButtonState == buttonVoltage && !selectButtonPressProcessed)
    {
      if (prevSelectState == !buttonVoltage)
      {
        selectButtonPressProcessed = true;
        lastButtonTime = currentTime;
      }
    }

    // Button release detection for SELECT
    if (selectButtonState == !buttonVoltage && prevSelectState == buttonVoltage)
    {
      if (selectButtonPressProcessed)
      {
        // Use encoder selection or default to OK if only one button
        if (currentConfig.showCancelButton)
        {
          currentResult = (selectedButton == 0) ? PopupResult::OK : PopupResult::CANCEL;
        }
        else
        {
          currentResult = PopupResult::OK;
        }
        selectButtonPressProcessed = false;
      }
    }

    // Reset processed flag when button is not pressed
    if (selectButtonState == !buttonVoltage)
    {
      selectButtonPressProcessed = false;
    }

    // Update previous state
    prevSelectState = selectButtonState;
  }
  else
  {
    // Original button-only input handling
    int selectButtonState = digitalRead(BUTTON_SELECT_PIN);
    int upButtonState = digitalRead(BUTTON_UP_PIN);

    // SELECT button (OK) - using proper press/release detection
    if (selectButtonState == buttonVoltage && !selectButtonPressProcessed)
    {
      if (prevSelectState == !buttonVoltage)
      {
        selectButtonPressProcessed = true;
        lastButtonTime = currentTime;
      }
    }

    // Button release detection for SELECT
    if (selectButtonState == !buttonVoltage && prevSelectState == buttonVoltage)
    {
      if (selectButtonPressProcessed)
      {
        currentResult = PopupResult::OK;
        selectButtonPressProcessed = false;
      }
    }

    // UP button (Cancel) - only if cancel button is shown
    if (currentConfig.showCancelButton)
    {
      if (upButtonState == buttonVoltage && !upButtonPressProcessed)
      {
        if (prevUpState == !buttonVoltage)
        {
          upButtonPressProcessed = true;
          lastButtonTime = currentTime;
        }
      }

      // Button release detection for UP (Cancel)
      if (upButtonState == !buttonVoltage && prevUpState == buttonVoltage)
      {
        if (upButtonPressProcessed)
        {
          currentResult = PopupResult::CANCEL;
          upButtonPressProcessed = false;
        }
      }
    }

    // Reset processed flags when buttons are not pressed
    if (selectButtonState == !buttonVoltage)
    {
      selectButtonPressProcessed = false;
    }
    if (upButtonState == !buttonVoltage)
    {
      upButtonPressProcessed = false;
    }

    // Update previous states
    prevSelectState = selectButtonState;
    prevUpState = upButtonState;
  }

  // Hide popup if a result was set
  if (currentResult != PopupResult::NONE)
  {
    hide();
  }
}

void PopupManager::hide()
{
  isVisible = false;
  currentResult = PopupResult::NONE;
}

// Legacy wrapper for backward compatibility (optional)
void OpenMenuOS::drawPopup(char *message, bool &clicked, int type)
{
  // Convert old type system to new enum
  PopupType newType;
  switch (type)
  {
  case 1:
    newType = PopupType::WARNING;
    break;
  case 2:
    newType = PopupType::SUCCESS;
    break;
  case 3:
    newType = PopupType::INFO;
    break;
  default:
    newType = PopupType::INFO;
    break;
  }

  // Show popup using new system
  PopupConfig config;
  config.message = message;
  config.type = newType;

  PopupResult result = PopupManager::show(config);

  // Update the popup and check for interaction
  result = PopupManager::update();
  clicked = (result != PopupResult::NONE);
}
void Screen::drawScrollbar(int selectedItem, int nextItem, int numItem)
{
  // Avoid division by zero
  if (numItem == 0)
  {
    return; // If numItem is zero, simply exit the function to avoid the division
  }

  // Get current time for animation timing
  unsigned long currentTime = millis();

  // Calculate box height based on number of items
  int boxHeight = tftHeight / numItem;

  // Calculate target position for the scrollbar
  targetScrollPosition = boxHeight * selectedItem;

  // Check if animation is enabled
  if (config.animation)
  {
    // Check if selected item has changed
    if (lastSelectedItem != selectedItem)
    {
      // Item changed, start animation from current position to target
      lastSelectedItem = selectedItem;
      lastScrollTime = currentTime;
    }

    // Calculate animation progress (smooth transition)
    float animationProgress = min(1.0f, (float)(currentTime - lastScrollTime) / (1000.0f / SCROLL_ANIMATION_SPEED));

    // Apply easing function for smoother movement (ease out)
    float easedProgress = 1.0f - pow(1.0f - animationProgress, 3); // Cubic ease out

    // Update current position with smooth interpolation
    if (currentScrollPosition != targetScrollPosition)
    {
      currentScrollPosition = currentScrollPosition + (targetScrollPosition - currentScrollPosition) * easedProgress;

      // If we're very close to the target, snap to it
      if (abs(currentScrollPosition - targetScrollPosition) < 0.5)
      {
        currentScrollPosition = targetScrollPosition;
      }
    }
  }
  else
  {
    // No animation - immediately set to target position
    currentScrollPosition = targetScrollPosition;
    lastSelectedItem = selectedItem;
  }

  // Use the current position for drawing (animated or immediate)
  int boxY = (int)currentScrollPosition;

  if (config.scrollbarStyle == 0)
  {
    // Clear entire scrollbar area to prevent artifacts
    canvas.fillRect(tftWidth - 3, 0, 3, tftHeight, TFT_BLACK);

    // Draw new scrollbar handle at the position
    canvas.fillRect(tftWidth - 3, boxY, 3, boxHeight, config.scrollbarColor);

    for (int y = 0; y < tftHeight; y++)
    {
      // Display the Scrollbar
      if (y % 2 == 0)
      {
        canvas.drawPixel(tftWidth - 2, y, TFT_WHITE);
      }
    }
  }
  else if (config.scrollbarStyle == 1)
  {
    // Clear previous scrollbar handle (entire area)
    canvas.fillRect(tftWidth - 3, 0, 3, tftHeight, TFT_BLACK);

    // Draw new scrollbar handle at the position
    canvas.fillSmoothRoundRect(tftWidth - 3, boxY, 3, boxHeight, 4, config.scrollbarColor, TFT_BLACK);
  }
}

void OpenMenuOS::scrollTextHorizontal(int16_t x, int16_t y, const char *text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize)
{
  // Optimized scrolling text with cached sprite and reduced allocations
  static int16_t xPos = x;
  static unsigned long previousMillis = 0;
  static String currentText = "";
  static TFT_eSprite *cachedSprite = nullptr;
  static uint16_t cachedWindowSize = 0;
  static uint16_t cachedFontHeight = 0;
  static int16_t cachedTextWidth = 0;

  const uint16_t fadeWidth = 25;
  const uint16_t totalWindowSize = windowSize + fadeWidth;

  // Check if text changed - reset position and cache text width
  if (currentText != text)
  {
    xPos = x;
    currentText = text;

    // Pre-calculate text width to avoid repeated calls
    canvas.setTextSize(textSize);
    cachedTextWidth = canvas.textWidth(text);
  }

  // Update animation position
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delayTime)
  {
    previousMillis = currentMillis;
    xPos--;

    if (xPos <= x - cachedTextWidth)
    {
      xPos = x + totalWindowSize;
    }
  }

  // Get font height once
  canvas.setFreeFont(menuFontBold);
  uint16_t fontHeight = canvas.fontHeight();

  // Create or reuse sprite only when dimensions change
  if (cachedSprite == nullptr || cachedWindowSize != totalWindowSize || cachedFontHeight != fontHeight)
  {
    if (cachedSprite != nullptr)
    {
      cachedSprite->deleteSprite();
      delete cachedSprite;
    }

    cachedSprite = new TFT_eSprite(&tft);
    if (cachedSprite->createSprite(totalWindowSize, fontHeight + 2))
    {
      cachedWindowSize = totalWindowSize;
      cachedFontHeight = fontHeight;
    }
    else
    {
      // Sprite creation failed - fallback to direct drawing
      delete cachedSprite;
      cachedSprite = nullptr;
      canvas.setTextColor(textColor, bgColor);
      canvas.setCursor(xPos, y);
      canvas.print(text);
      return;
    }
  }

  // Configure sprite efficiently
  bool swapBytesState = canvas.getSwapBytes();
  canvas.setSwapBytes(false);

  cachedSprite->fillSprite(TFT_TRANSPARENT);
  cachedSprite->setFreeFont(menuFontBold);
  cachedSprite->setTextSize(textSize);
  cachedSprite->setTextColor(textColor, bgColor);

  // Draw text on the sprite
  int16_t yPos = fontHeight - 4;
  cachedSprite->setCursor(xPos - x, yPos);
  cachedSprite->print(text);

  // Add fade effect efficiently
  cachedSprite->fillRect(totalWindowSize - fadeWidth, 0, fadeWidth, cachedSprite->height(), TFT_TRANSPARENT);

  // Push sprite to canvas
  cachedSprite->pushToSprite(&canvas, x, y - yPos, TFT_TRANSPARENT);

  canvas.setSwapBytes(swapBytesState);
}
// Overloaded function for String input
void OpenMenuOS::scrollTextHorizontal(int16_t x, int16_t y, const String &text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize)
{
  scrollTextHorizontal(x, y, text.c_str(), textColor, bgColor, textSize, delayTime, windowSize);
}

void Screen::scrollTextHorizontal(int16_t x, int16_t y, const char *text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize)
{
  // Optimized scrolling text with cached sprite and reduced allocations
  static int16_t xPos = x;
  static unsigned long previousMillis = 0;
  static String currentText = "";
  static TFT_eSprite *cachedSprite = nullptr;
  static uint16_t cachedWindowSize = 0;
  static uint16_t cachedFontHeight = 0;
  static int16_t cachedTextWidth = 0;

  const uint16_t fadeWidth = 25;
  const uint16_t totalWindowSize = windowSize + fadeWidth;

  // Check if text changed - reset position and cache text width
  if (currentText != text)
  {
    xPos = x;
    currentText = text;

    // Pre-calculate text width to avoid repeated calls
    canvas.setTextSize(textSize);
    cachedTextWidth = canvas.textWidth(text);
  }

  // Update animation position
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= delayTime)
  {
    previousMillis = currentMillis;
    xPos--;

    if (xPos <= x - cachedTextWidth)
    {
      xPos = x + totalWindowSize;
    }
  }

  // Get font height once
  canvas.setFreeFont(menuFontBold);
  uint16_t fontHeight = canvas.fontHeight();

  // Create or reuse sprite only when dimensions change
  if (cachedSprite == nullptr || cachedWindowSize != totalWindowSize || cachedFontHeight != fontHeight)
  {
    if (cachedSprite != nullptr)
    {
      cachedSprite->deleteSprite();
      delete cachedSprite;
    }

    cachedSprite = new TFT_eSprite(&tft);
    if (cachedSprite->createSprite(totalWindowSize, fontHeight + 2))
    {
      cachedWindowSize = totalWindowSize;
      cachedFontHeight = fontHeight;
    }
    else
    {
      // Sprite creation failed - fallback to direct drawing
      delete cachedSprite;
      cachedSprite = nullptr;
      canvas.setTextColor(textColor, bgColor);
      canvas.setCursor(xPos, y);
      canvas.print(text);
      return;
    }
  }

  // Configure sprite efficiently
  bool swapBytesState = canvas.getSwapBytes();
  canvas.setSwapBytes(false);

  cachedSprite->fillSprite(TFT_TRANSPARENT);
  cachedSprite->setFreeFont(menuFontBold);
  cachedSprite->setTextSize(textSize);
  cachedSprite->setTextColor(textColor, bgColor);

  // Draw text on the sprite
  int16_t yPos = fontHeight - 4;
  cachedSprite->setCursor(xPos - x, yPos);
  cachedSprite->print(text);

  // Add fade effect efficiently
  cachedSprite->fillRect(totalWindowSize - fadeWidth, 0, fadeWidth, cachedSprite->height(), TFT_TRANSPARENT);

  // Push sprite to canvas
  cachedSprite->pushToSprite(&canvas, x, y - yPos, TFT_TRANSPARENT);

  canvas.setSwapBytes(swapBytesState);
}

// Overloaded function for String input
void Screen::scrollTextHorizontal(int16_t x, int16_t y, const String &text, uint16_t textColor, uint16_t bgColor, uint8_t textSize, uint16_t delayTime, uint16_t windowSize)
{
  scrollTextHorizontal(x, y, text.c_str(), textColor, bgColor, textSize, delayTime, windowSize);
}
int Screen::calculateMaxCharacters(const char *text, uint16_t windowSize)
{
  if (text == nullptr || windowSize == 0)
    return 0;

  int textLength = strlen(text);
  if (textLength == 0)
    return 0;

  // Quick check: if full text fits, return early
  if (canvas.textWidth(text) <= windowSize)
  {
    return textLength;
  }

  // Use binary search for efficient character counting
  int left = 0;
  int right = textLength;
  int maxFit = 0;

  // Create a small static buffer to avoid repeated allocations
  static char tempBuffer[256];

  while (left <= right)
  {
    int mid = (left + right) / 2;

    // Safely copy substring using smaller buffer or dynamic allocation
    if (mid + 1 <= sizeof(tempBuffer))
    {
      strncpy(tempBuffer, text, mid);
      tempBuffer[mid] = '\0';

      if (canvas.textWidth(tempBuffer) <= windowSize)
      {
        maxFit = mid;
        left = mid + 1;
      }
      else
      {
        right = mid - 1;
      }
    }
    else
    {
      // For very long strings, fall back to linear search from current best fit
      for (int i = maxFit; i < textLength; i++)
      {
        strncpy(tempBuffer, text, sizeof(tempBuffer) - 1);
        tempBuffer[sizeof(tempBuffer) - 1] = '\0';

        if (canvas.textWidth(tempBuffer) > windowSize)
        {
          return i - 1;
        }
        maxFit = i;
      }
      break;
    }
  }

  return maxFit;
}

void OpenMenuOS::setDisplayRotation(int rotation = 0)
{
  displayRotation = rotation;

  if (tftInitialized)
  {
    tft.setRotation(displayRotation);
  }
}
void OpenMenuOS::setTextScroll(bool x = true)
{
  MenuScreen::config.textScroll = x;
}
void OpenMenuOS::showBootImage(bool x = true)
{
  bootImage = x;
}
// Function to set the global image pointer and dimensions
void OpenMenuOS::setBootImage(uint16_t *Boot_img, uint16_t height, uint16_t width)
{
  // Set the global variables to the provided values
  boot_image = Boot_img;
  boot_image_width = width;
  boot_image_height = height;
}
void OpenMenuOS::setButtonAnimation(bool x = true)
{
  MenuScreen::config.buttonAnimation = x;
}
void OpenMenuOS::setMenuStyle(int style)
{
  MenuScreen::config.menuStyle = style;
}
void OpenMenuOS::setScrollbar(bool x = true)
{
  MenuScreen::config.scrollbar = x;
}
void OpenMenuOS::setScrollbarColor(uint16_t color = TFT_WHITE)
{
  MenuScreen::config.scrollbarColor = color;
}
void OpenMenuOS::setScrollbarStyle(int style)
{
  MenuScreen::config.scrollbarStyle = style;
}
void OpenMenuOS::setSelectionBorderColor(uint16_t color = TFT_WHITE)
{
  MenuScreen::config.selectionBorderColor = color;
}
void OpenMenuOS::setSelectionFillColor(uint16_t color = TFT_BLACK)
{
  MenuScreen::config.selectionFillColor = color;
}
void OpenMenuOS::setMenuFont(const GFXfont *font)
{
  menuFont = font;
}

void OpenMenuOS::setMenuFontBold(const GFXfont *font)
{
  menuFontBold = font;
}
void OpenMenuOS::useStylePreset(int preset)
{
  // Apply the preset based on the preset number
  switch (preset)
  {
  case 0:
    setMenuStyle(0);
    break;
  case 1:
    setScrollbar(false); // Disable scrollbar
    setMenuStyle(1);
    setSelectionBorderColor(0xfa60); // Setting the selection rectangle's color to Rabbit R1's Orange/Leuchtorange
    setSelectionFillColor(0xfa60);
    break;
  default:
    break;
  }
}
void OpenMenuOS::useStylePreset(char *preset)
{
  int presetNumber = 0; // Initialize with the default preset number

  // Convert the preset name to lowercase for case-insensitive comparison
  String lowercasePreset = String(preset);
  lowercasePreset.toLowerCase();

  // Compare the preset name with lowercase versions of the preset names
  if (lowercasePreset == "default")
  {
    presetNumber = 0;
  }
  else if (lowercasePreset == "rabbit_r1")
  {
    presetNumber = 1;
  }

  useStylePreset(presetNumber);
}

void OpenMenuOS::setButtonsMode(char *mode)
{ // The mode is either Pullup or Pulldown
  // Convert mode to lowercase for case-insensitive comparison
  String lowercaseMode = String(mode);
  lowercaseMode.toLowerCase();

  // Check if mode is valid
  if (lowercaseMode == "high")
  {
    buttonsMode = INPUT_PULLDOWN;
    buttonVoltage = HIGH;
  }
  else if (lowercaseMode == "low")
  {
    buttonsMode = INPUT_PULLUP;
    buttonVoltage = LOW;
  }
  else
  {
    // Invalid mode, print an error message
    Serial.println("Error: Invalid mode. Please use 'high' or 'low'.");
  }
}

void OpenMenuOS::setEncoderPin(uint8_t clk, uint8_t dt)
{
  // Set the pins as input
  pinMode(clk, INPUT);
  pinMode(dt, INPUT);

  // Store the pin values in the variables
  encoderClkPin = clk;
  encoderDtPin = dt;

  // Set the useEncoder flag to true
  useEncoder = true;
}

void OpenMenuOS::setUpPin(uint8_t btn_up)
{
  BUTTON_UP_PIN = btn_up;
}

void OpenMenuOS::setDownPin(uint8_t btn_down)
{
  BUTTON_DOWN_PIN = btn_down;
}

void OpenMenuOS::setSelectPin(uint8_t btn_sel)
{
  BUTTON_SELECT_PIN = btn_sel;
}

void OpenMenuOS::drawCanvasOnTFT()
{
  if (!optimizeDisplayUpdates)
  {
    // Simple direct drawing without comparison

    // Draw popup on top if active
    if (PopupManager::isActive())
    {
      PopupManager::update();
    }

    canvas.pushSprite(0, 0);

    // Only clear canvas if no popup is active
    if (!PopupManager::isActive())
    {
      canvas.fillSprite(TFT_BLACK);
    }
    return;
  }

  // Static buffers for frame comparison - allocated once and reused
  static uint16_t *lastFrame = nullptr;
  static uint16_t *currentFrame = nullptr;
  static size_t bufferSize = 0;
  static bool buffersInitialized = false;

  // Calculate buffer size and initialize if needed
  size_t requiredBufferSize = tftWidth * tftHeight * sizeof(uint16_t);

  if (!buffersInitialized || bufferSize != requiredBufferSize)
  {
    // Clean up old buffers if they exist
    if (lastFrame)
    {
      free(lastFrame);
      lastFrame = nullptr;
    }
    if (currentFrame)
    {
      free(currentFrame);
      currentFrame = nullptr;
    }

    bufferSize = requiredBufferSize;

    // Allocate both buffers
    lastFrame = (uint16_t *)malloc(bufferSize);
    currentFrame = (uint16_t *)malloc(bufferSize);

    if (lastFrame == nullptr || currentFrame == nullptr)
    {
      // Memory allocation failed - clean up and fallback
      if (lastFrame)
      {
        free(lastFrame);
        lastFrame = nullptr;
      }
      if (currentFrame)
      {
        free(currentFrame);
        currentFrame = nullptr;
      }
      canvas.pushSprite(0, 0);
      canvas.fillSprite(TFT_BLACK);
      return;
    }

    // Initialize last frame to force first update
    memset(lastFrame, 0xFF, bufferSize); // Use 0xFF instead of 0 for better contrast
    buffersInitialized = true;
  }
  // Get current frame data efficiently
  // Try to access sprite buffer directly if possible, otherwise fall back to pixel reading
  uint16_t *spriteBuffer = (uint16_t *)canvas.getPointer();
  bool hasChanged = false;

  if (spriteBuffer != nullptr)
  {
    // Direct buffer access - much faster
    // Use memcmp for initial quick check
    if (memcmp(spriteBuffer, lastFrame, bufferSize) != 0)
    {
      hasChanged = true;
      memcpy(currentFrame, spriteBuffer, bufferSize);
    }
  }
  else
  {
    // Fallback to pixel-by-pixel reading with optimizations
    size_t totalPixels = tftWidth * tftHeight;
    size_t index = 0;

    // Process in chunks for better cache performance
    const int CHUNK_SIZE = 32; // Process 32 pixels at a time

    for (int y = 0; y < tftHeight && !hasChanged; y++)
    {
      for (int x = 0; x < tftWidth; x += CHUNK_SIZE)
      {
        int endX = min(x + CHUNK_SIZE, (int)tftWidth);

        // Read chunk of pixels
        for (int px = x; px < endX; px++)
        {
          uint16_t pixel = canvas.readPixel(px, y);
          currentFrame[index] = pixel;

          // Early exit on first difference found
          if (pixel != lastFrame[index])
          {
            hasChanged = true;
            // Continue reading rest of frame for complete buffer
            for (int ry = y; ry < tftHeight; ry++)
            {
              int startX = (ry == y) ? px + 1 : 0;
              for (int rx = startX; rx < tftWidth; rx++)
              {
                size_t ridx = ry * tftWidth + rx;
                currentFrame[ridx] = canvas.readPixel(rx, ry);
              }
            }
            goto comparison_done;
          }
          index++;
        }
      }
    }
  comparison_done:;
  } // Only update display if content has changed OR if popup is active
  if (hasChanged || PopupManager::isActive())
  {
    // Draw popup on top if active
    if (PopupManager::isActive())
    {
      PopupManager::update();
    }

    canvas.pushSprite(0, 0);

    // Swap buffers instead of copying - more efficient
    uint16_t *temp = lastFrame;
    lastFrame = currentFrame;
    currentFrame = temp;
  }

  // Clear canvas for next frame - but ONLY if no popup is active
  if (!PopupManager::isActive())
  {
    canvas.fillSprite(TFT_BLACK);
  }
}

const char *OpenMenuOS::getLibraryVersion()
{
  return LIBRARY_VERSION;
}
int OpenMenuOS::getTftHeight() const
{
  return tftHeight;
}
int OpenMenuOS::getTftWidth() const
{
  return tftWidth;
}
int OpenMenuOS::UpButton() const
{
  return BUTTON_UP_PIN;
}
int OpenMenuOS::DownButton() const
{
  return BUTTON_DOWN_PIN;
}
int OpenMenuOS::SelectButton() const
{
  return BUTTON_SELECT_PIN;
}
/**
 * @brief Controls whether to optimize display updates by comparing frames.
 *
 * When enabled, this feature reduces unnecessary display refreshes by only updating
 * when content has actually changed. This can improve performance and reduce
 * flickering, but requires additional memory for frame buffering.
 *
 * Memory usage: tftWidth * tftHeight * 2 bytes
 * Example: 160x128 display = 40KB of RAM
 *
 * @note WARNING: Not recommended for ESP8266 with large displays (>160x128)
 * due to memory constraints.
 *
 * @param enabled true to enable frame comparison, false to disable
 */
void OpenMenuOS::setOptimizeDisplayUpdates(bool enabled)
{
  optimizeDisplayUpdates = enabled;
}
void OpenMenuOS::setAnimation(bool enabled)
{
  Screen::config.animation = enabled;
}
/**
 * @brief Gets the current state of display update optimization.
 *
 * @return true if frame comparison is enabled
 * @return false if direct display updates are being used
 */
bool OpenMenuOS::getOptimizeDisplayUpdates() const
{
  return optimizeDisplayUpdates;
}

void Setting::generateId()
{
  // Skip ID generation for SUBSCREEN type
  if (type == SUBSCREEN)
  {
    id = 0; // Assign a default value of 0 for SUBSCREEN type
    return;
  }

  uint16_t hash = 0;
  // Hash the name string
  const char *ptr = name;
  while (*ptr != '\0')
  {
    hash = (hash << 5) - hash + *ptr;
    ptr++;
  }
  // Add type to the hash
  hash = (hash << 3) - hash + static_cast<uint8_t>(type);

  // Ensure the ID is never zero (0 is reserved for SUBSCREEN type)
  if (hash == 0)
    hash = 1;

  id = hash;
}

// Save settings to non-volatile storage
void SettingsScreen::saveToEEPROM()
{
  for (int i = 0; i < totalSettings; i++)
  {
    Setting *s = settings[i];
    if (s == nullptr || s->type == Setting::SUBSCREEN)
      continue; // Skip empty settings and redirects

    // Save based on the type of setting
    switch (s->type)
    {
    case Setting::BOOLEAN:
#if defined(ESP32)
      preferences.putBool(String(s->id).c_str(), s->booleanValue); // Save boolean in Preferences
#else
      EEPROM.write(s->id, s->booleanValue ? 1 : 0); // Store 1 for true, 0 for false in EEPROM
#endif
      break;
    case Setting::RANGE:
#if defined(ESP32)
      preferences.putInt(String(s->id).c_str(), s->rangeValue); // Save range in Preferences
#else
      EEPROM.write(s->id, s->rangeValue); // Store the range value directly in EEPROM
#endif
      break;
    case Setting::OPTION:
#if defined(ESP32)
      preferences.putInt(String(s->id).c_str(), s->optionIndex); // Save option index in Preferences
#else
      EEPROM.write(s->id, s->optionIndex); // Store option index in EEPROM
#endif
      break;
    default:
      break;
    }
  }

#if defined(ESP8266)
  EEPROM.commit(); // Commit changes to EEPROM for ESP8266
#endif
}

// Read settings from non-volatile storage
void SettingsScreen::readFromEEPROM()
{
  for (int i = 0; i < totalSettings; i++)
  {
    Setting *s = settings[i];
    if (s == nullptr || s->type == Setting::SUBSCREEN)
      continue; // Skip empty settings and redirects

    // Read based on the type of setting
    switch (s->type)
    {
    case Setting::BOOLEAN:
#if defined(ESP32)
      s->booleanValue = preferences.getBool(String(s->id).c_str(), false); // Get boolean from Preferences
#else
      s->booleanValue = EEPROM.read(s->id) == 1; // Convert to boolean value from EEPROM
#endif
      break;
    case Setting::RANGE:
#if defined(ESP32)
      s->rangeValue = preferences.getInt(String(s->id).c_str(), 0); // Get range from Preferences
#else
      s->rangeValue = EEPROM.read(s->id); // Read the range value directly from EEPROM
#endif
      break;
    case Setting::OPTION:
#if defined(ESP32)
      s->optionIndex = preferences.getInt(String(s->id).c_str(), 0); // Get option index from Preferences
#else
      s->optionIndex = EEPROM.read(s->id); // Read the option index from EEPROM
#endif
      break;
    default:
      break;
    }
  }
}
void SettingsScreen::ensureInitialized()
{
  // Only initialize once across all instances
  if (!preferencesInitialized)
  {
#if defined(ESP32)
    preferences.begin("Settings", false); // Initialize Preferences for ESP32
#else
    EEPROM.begin(512); // Initialize EEPROM for ESP8266
#endif
    preferencesInitialized = true;
  }
}

void SettingsScreen::addBooleanSetting(const char *name, bool defaultValue)
{
  ensureInitialized();

  // Check if a setting with the same name and type already exists
  for (int i = 0; i < settings.size(); i++)
  {
    if (settings[i]->type == Setting::BOOLEAN && strcmp(settings[i]->name, name) == 0)
    {
      // Setting with same name and type already exists, don't add it
      return;
    }
  }

  Setting *s = new Setting(name, Setting::BOOLEAN);

  // Check if the setting already exists in EEPROM (for ESP32/ESP8266)
  if (!settingExists(s->id))
  {
    s->booleanValue = defaultValue;
    saveToEEPROM(); // Save settings if not already in EEPROM
  }
  else
  {
    // Read value from EEPROM to associate with the setting
    s->booleanValue = getBooleanFromEEPROM(s->id); // Add logic to retrieve the value from EEPROM
  }

  settings.push_back(s);
  totalSettings = settings.size();
}

void SettingsScreen::addRangeSetting(const char *name, uint8_t min, uint8_t max, uint8_t defaultValue, const char *unit = nullptr)
{
  ensureInitialized();

  // Check if a setting with the same name and type already exists
  for (int i = 0; i < settings.size(); i++)
  {
    if (settings[i]->type == Setting::RANGE && strcmp(settings[i]->name, name) == 0)
    {
      // Setting with same name and type already exists, don't add it
      return;
    }
  }

  Setting *s = new Setting(name, Setting::RANGE);
  s->range.min = min;
  s->range.max = max;
  s->range.unit = unit;

  // Check if the setting already exists in EEPROM (for ESP32/ESP8266)
  if (!settingExists(s->id))
  {
    s->rangeValue = defaultValue;
    saveToEEPROM(); // Save settings if not already in EEPROM
  }
  else
  {
    // Read value from EEPROM to associate with the setting
    s->rangeValue = getRangeFromEEPROM(s->id);
  }

  settings.push_back(s);
  totalSettings = settings.size();
}

void SettingsScreen::addOptionSetting(const char *name, const char **options, uint8_t count, uint8_t defaultIndex = 0)
{
  ensureInitialized();

  // Check if a setting with the same name and type already exists
  for (int i = 0; i < settings.size(); i++)
  {
    if (settings[i]->type == Setting::OPTION && strcmp(settings[i]->name, name) == 0)
    {
      // Setting with same name and type already exists, don't add it
      return;
    }
  }

  Setting *s = new Setting(name, Setting::OPTION);

  // Dynamically allocate memory for options
  s->options = new const char *[count];
  for (uint8_t i = 0; i < count; ++i)
  {
    s->options[i] = options[i];
  }

  s->optionCount = count;

  // Ensure the default index is within bounds
  if (defaultIndex >= count)
  {
    defaultIndex = 0;
  }

  // Check if the setting already exists in EEPROM (for ESP32/ESP8266)
  if (!settingExists(s->id))
  {
    s->optionIndex = defaultIndex;
    saveToEEPROM(); // Save settings if not already in EEPROM
  }
  else
  {
    // Read value from EEPROM to associate with the setting
    s->optionIndex = getOptionIndexFromEEPROM(s->id); // Add logic to retrieve the value from EEPROM
    if (s->optionIndex >= count)
    {
      s->optionIndex = defaultIndex;
    }
  }

  settings.push_back(s);
  totalSettings = settings.size();
}

void SettingsScreen::addSubscreenSetting(const char *name, Screen *targetScreen)
{
  Setting *s = new Setting(name, Setting::SUBSCREEN);

  // Set the target screen
  s->subScreen = targetScreen;

  // Add the setting to our vector
  settings.push_back(s);
  totalSettings = settings.size();
}

uint8_t SettingsScreen::getBooleanFromEEPROM(uint16_t settingId)
{
  uint8_t value = 0;

#if defined(ESP32)
  value = preferences.getBool(String(settingId).c_str(), false); // Get boolean from Preferences
#else
  value = EEPROM.read(settingId) == 1; // Convert to boolean value from EEPROM
#endif

  return value;
}

uint8_t SettingsScreen::getRangeFromEEPROM(uint16_t settingId)
{
  uint8_t value = 0;

#if defined(ESP32)
  value = preferences.getInt(String(settingId).c_str(), 0); // Get range value from Preferences
#else
  value = EEPROM.read(settingId); // Read the range value directly from EEPROM
#endif

  return value;
}

uint8_t SettingsScreen::getOptionIndexFromEEPROM(uint16_t settingId)
{
  uint8_t value = 0;

#if defined(ESP32)
  value = preferences.getInt(String(settingId).c_str(), 0); // Get option index from Preferences
#else
  value = EEPROM.read(settingId); // Read the option index directly from EEPROM
#endif

  return value;
}

// Check if setting already exists in EEPROM
bool SettingsScreen::settingExists(uint16_t settingId)
{

#if defined(ESP32)
  // For ESP32, check if the setting exists in Preferences
  if (preferences.isKey(String(settingId).c_str()))
  {
    return true; // Setting exists
  }
#else
  // For ESP8266, check if the value exists in EEPROM
  if (EEPROM.read(settingId) != -1)
  {
    return true; // Setting exists
  }
#endif

  return false; // Return false if setting doesn't exist
}

void SettingsScreen::modify(int8_t direction, int index)
{
  Setting *s = settings[item_selected_settings];

  if (s == nullptr || s->type == Setting::SUBSCREEN)
    return; // Skip empty settings and redirects

  switch (s->type)
  {
  case Setting::BOOLEAN:
    // Toggle the boolean value between true and false
    s->booleanValue = !s->booleanValue;
    break;

  case Setting::RANGE:
    // Adjust the range value by the specified direction, within the range limits
    s->rangeValue = constrain(
        s->rangeValue + direction,
        s->range.min,
        s->range.max);
    break;

  case Setting::OPTION:
    // Change the option index by the direction, ensuring it wraps around if necessary
    s->optionIndex = (s->optionIndex + direction + s->optionCount) % s->optionCount;
    break;

  default:
    // Handle unknown setting types, if necessary
    break;
  }
  // After modifying the setting, save it to EEPROM
  saveToEEPROM();
}

void SettingsScreen::modify(int8_t direction, const char *name)
{
  if (name == nullptr)
  {
    return; // Invalid input
  }

  // Find the first setting with the matching name
  for (int i = 0; i < totalSettings; i++)
  {
    Setting *s = settings[i];
    if (s != nullptr && s->name != nullptr && strcmp(s->name, name) == 0)
    {
      // Found a matching setting
      if (s->type == Setting::SUBSCREEN)
      {
        return; // Skip redirects
      }

      switch (s->type)
      {
      case Setting::BOOLEAN:
        // Toggle the boolean value between true and false
        s->booleanValue = !s->booleanValue;
        break;

      case Setting::RANGE:
        // Adjust the range value by the specified direction, within the range limits
        s->rangeValue = constrain(
            s->rangeValue + direction,
            s->range.min,
            s->range.max);
        break;

      case Setting::OPTION:
        // Change the option index by the direction, ensuring it wraps around if necessary
        s->optionIndex = (s->optionIndex + direction + s->optionCount) % s->optionCount;
        break;

      default:
        // Handle unknown setting types, if necessary
        break;
      }
      // After modifying the setting, save it to EEPROM
      saveToEEPROM();
      return; // Exit after modifying the first matching setting
    }
  }
  // No matching setting found
}

String SettingsScreen::getSettingName(int index)
{
  // Handle negative index
  if (index < 0)
  {
    index = totalSettings + index;
  }

  // Check if index is valid
  if (index < 0 || index >= totalSettings)
  {
    return String("");
  }

  Setting *s = settings[index];

  // Check if setting exists
  if (s == nullptr)
  {
    return String("");
  }

  // Check if name exists
  if (s->name == nullptr)
  {
    return String("");
  }

  // Return the setting name
  return String(s->name);
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
uint8_t SettingsScreen::getSettingValue(int index)
{
  // Support negative indexing (e.g., -1 returns the last setting)
  if (index < 0)
  {
    index = totalSettings + index;
  }
  if (index < 0 || index >= totalSettings)
  {
    // Out-of-bounds: return 0 (or you could signal an error)
    return 0;
  }

  Setting *s = settings[index];
  if (s == nullptr || s->type == Setting::SUBSCREEN)
    return 0; // Skip empty settings and redirects

  // Return a numeric value based on the setting type:
  switch (s->type)
  {
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
uint8_t SettingsScreen::getSettingValue(const char *name)
{
  if (name == nullptr)
  {
    return 0; // Invalid input
  }

  // Find the first setting with the matching name
  for (int i = 0; i < totalSettings; i++)
  {
    Setting *s = settings[i];
    if (s != nullptr && s->name != nullptr && strcmp(s->name, name) == 0)
    {
      // Found a matching setting
      if (s->type == Setting::SUBSCREEN)
      {
        return 0; // Skip redirects
      }

      // Return value based on type
      switch (s->type)
      {
      case Setting::BOOLEAN:
        return s->booleanValue ? 1 : 0;
      case Setting::RANGE:
        return s->rangeValue;
      case Setting::OPTION:
        return s->optionIndex;
      default:
        return 0;
      }
    }
  }

  // No matching setting found
  return 0;
}
Setting::Type SettingsScreen::getSettingType(uint8_t index)
{
  // Handle negative indices by wrapping around
  if (index < 0)
  {
    index = totalSettings + index;
  }

  // Ensure index is within bounds
  if (index < 0 || index >= totalSettings)
  {
    return Setting::BOOLEAN; // Return default value to avoid crash
  }

  Setting *s = settings[index];
  if (s == nullptr)
  {
    return Setting::BOOLEAN; // Handle null settings
  }

  // Access the type of the setting
  switch (s->type)
  {
  case Setting::BOOLEAN:
    return Setting::BOOLEAN;
  case Setting::RANGE:
    return Setting::RANGE;
  case Setting::OPTION:
    return Setting::OPTION;
  case Setting::SUBSCREEN:
    return Setting::SUBSCREEN;
  default:
    return Setting::BOOLEAN;
  }
}

void SettingsScreen::resetSettings()
{
  for (int i = 0; i < totalSettings; i++)
  {
    Setting *s = settings[i];
    if (s == nullptr || s->type == Setting::SUBSCREEN)
      continue; // Skip empty settings and redirects

    // Clear/remove setting from storage based on type
    switch (s->type)
    {
    case Setting::BOOLEAN:
#if defined(ESP32)
      preferences.remove(String(s->id).c_str()); // Remove boolean from Preferences
#else
      EEPROM.write(s->id, 0); // Reset to 0 (false) in EEPROM
#endif
      s->booleanValue = false; // Reset the value in memory
      break;

    case Setting::RANGE:
#if defined(ESP32)
      preferences.remove(String(s->id).c_str()); // Remove range from Preferences
#else
      EEPROM.write(s->id, 0); // Reset to 0 in EEPROM
#endif
      s->rangeValue = s->range.min; // Reset to minimum value in memory
      break;

    case Setting::OPTION:
#if defined(ESP32)
      preferences.remove(String(s->id).c_str()); // Remove option from Preferences
#else
      EEPROM.write(s->id, 0); // Reset to 0 in EEPROM
#endif
      s->optionIndex = 0; // Reset to first option in memory
      break;

    default:
      break;
    }
  }

#if defined(ESP8266)
  EEPROM.commit(); // Commit changes to EEPROM for ESP8266
#endif

#if defined(ESP8266) || defined(ESP32)
  esp_restart(); // Restart the ESP to apply changes
#endif
}