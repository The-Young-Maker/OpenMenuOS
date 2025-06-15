# OpenMenuOS API Reference

Complete reference for all classes, methods, and functions in the OpenMenuOS library.

## Table of Contents

- [OpenMenuOS Class](#openmenuos-class)
- [PopupManager Class](#popupmanager-class)
- [Screen Classes](#screen-classes)
- [Settings System](#settings-system)
- [Enums and Structures](#enums-and-structures)
- [Global Functions](#global-functions)

---

## OpenMenuOS Class

The main class for managing the entire menu system.

### Constructor

```cpp
OpenMenuOS();
```

### Core Methods

#### `void begin()`
Initialize the menu system and display.
```cpp
menu.begin();
```

#### `void loop()`
Main update loop - call this in your Arduino loop() function.
```cpp
void loop() {
  menu.loop();
}
```

#### `void setupButtons(int upPin, int downPin, int selectPin, int mode, int voltage)`
Configure button pins and behavior.
- `upPin`: GPIO pin for UP button (-1 to disable)
- `downPin`: GPIO pin for DOWN button (-1 to disable)
- `selectPin`: GPIO pin for SELECT button
- `mode`: INPUT or INPUT_PULLUP
- `voltage`: Expected voltage when pressed (HIGH or LOW)

```cpp
menu.setupButtons(19, -1, 2, INPUT_PULLUP, LOW);
```

#### `void setEncoderPin(int clkPin, int dtPin)`
Enable rotary encoder support.
- `clkPin`: Clock pin of encoder
- `dtPin`: Data pin of encoder

```cpp
menu.setEncoderPin(5, 4);
```

### Display Configuration

#### `void setBootImage(const uint16_t* image, int width, int height)`
Set a custom boot image displayed during initialization.
```cpp
menu.setBootImage(myBootImage, 128, 160);
```

#### `void setOptimizeDisplayUpdates(bool enabled)`
Enable/disable frame comparison for performance optimization.
```cpp
menu.setOptimizeDisplayUpdates(true);  // Better performance
```

#### `void setAnimation(bool enabled)`
Enable/disable menu animations.
```cpp
menu.setAnimation(false);  // Disable for better performance
```

#### `void setTextScroll(bool enabled)`
Enable/disable text scrolling for long menu items.
```cpp
menu.setTextScroll(true);
```

#### `void setScrollbar(bool enabled)`
Show/hide scrollbar in menus.
```cpp
menu.setScrollbar(true);
```

### Menu Management

#### `void setMainMenuItems(const std::vector<String>& items)`
Set items for the automatically created main menu.
```cpp
menu.setMainMenuItems({"Settings", "About", "WiFi", "Exit"});
```

#### `void createMainMenu()`
Create the main menu screen with the configured items.
```cpp
menu.createMainMenu();
```

#### `void addSettingsScreen(const String& title, Setting* settings, int count)`
Add a settings screen to the menu system.
```cpp
Setting settings[] = {
  {"WiFi Enabled", SETTING_BOOLEAN, true},
  {"Brightness", SETTING_RANGE, 50, 0, 100}
};
menu.addSettingsScreen("System Settings", settings, 2);
```

#### `void addScreen(Screen* screen)`
Add a custom screen to the menu system.
```cpp
MenuScreen* customMenu = new MenuScreen("Custom Menu");
menu.addScreen(customMenu);
```

### Screen Navigation

#### `void navigateToScreen(int screenIndex)`
Navigate to a specific screen by index.
```cpp
menu.navigateToScreen(0);  // Go to first screen
```

#### `void goBack()`
Return to the previous screen.
```cpp
menu.goBack();
```

#### `void goToMainMenu()`
Navigate directly to the main menu.
```cpp
menu.goToMainMenu();
```

### Styling and Themes

#### `void useStylePreset(const String& preset)`
Apply a predefined visual theme.
- Available presets: "Rabbit_R1", "Classic", "Modern"

```cpp
menu.useStylePreset("Rabbit_R1");
```

#### `void setSelectionBorderColor(uint16_t color)`
Set the color for selection borders.
```cpp
menu.setSelectionBorderColor(TFT_CYAN);
```

#### `void setBackgroundColor(uint16_t color)`
Set the background color for all screens.
```cpp
menu.setBackgroundColor(TFT_BLACK);
```

### Information Methods

#### `String getLibraryVersion()`
Get the current library version.
```cpp
String version = menu.getLibraryVersion();
```

#### `int getTftWidth()` / `int getTftHeight()`
Get display dimensions.
```cpp
int width = menu.getTftWidth();
int height = menu.getTftHeight();
```

---

## PopupManager Class

Static class for managing popup dialogs.

### Popup Types

#### `static void showInfo(const String& message, const String& title = "Info", int autoCloseMs = 0)`
Display an informational popup.
```cpp
PopupManager::showInfo("Operation completed successfully");
PopupManager::showInfo("Status update", "System", 3000);  // Auto-close after 3s
```

#### `static void showSuccess(const String& message, const String& title = "Success", int autoCloseMs = 2000)`
Display a success popup (auto-closes by default).
```cpp
PopupManager::showSuccess("Data saved successfully!");
```

#### `static void showWarning(const String& message, const String& title = "Warning", int autoCloseMs = 0)`
Display a warning popup.
```cpp
PopupManager::showWarning("Low battery detected", "Battery Warning");
```

#### `static void showError(const String& message, const String& title = "Error", int autoCloseMs = 0)`
Display an error popup.
```cpp
PopupManager::showError("Failed to connect to WiFi", "Connection Error");
```

#### `static PopupResult showQuestion(const String& message, const String& title = "Question", const std::vector<String>& buttons = {"OK", "Cancel"})`
Display a question popup with custom buttons.
```cpp
PopupResult result = PopupManager::showQuestion("Delete all data?", "Confirm");
if (result.buttonIndex == 0) {
  // User clicked first button (OK)
}

// Custom buttons
std::vector<String> options = {"Save", "Don't Save", "Cancel"};
PopupResult result = PopupManager::showQuestion("Save changes?", "Unsaved Changes", options);
```

### Configuration

#### `static void setConfig(const PopupConfig& config)`
Set global popup configuration.
```cpp
PopupConfig config;
config.overlayOpacity = 128;      // Semi-transparent overlay
config.cornerRadius = 8;          // Rounded corners
config.shadowOffset = 3;          // Shadow effect
config.animationSpeed = 200;      // Animation duration in ms
config.buttonSpacing = 10;        // Space between buttons
config.padding = 15;              // Internal padding
PopupManager::setConfig(config);
```

### State Management

#### `static bool isActive()`
Check if any popup is currently active.
```cpp
if (PopupManager::isActive()) {
  // A popup is currently shown
}
```

#### `static PopupResult getResult()`
Get the result of the last popup interaction.
```cpp
PopupResult result = PopupManager::getResult();
if (result.buttonIndex != -1) {
  // User interacted with popup
}
```

---

## Screen Classes

Base classes for creating different types of screens.

### Screen (Base Class)

#### `Screen(const String& title)`
Base constructor for all screen types.
```cpp
Screen myScreen("My Screen");
```

#### `virtual void draw()`
Override to implement custom drawing logic.
```cpp
class CustomScreen : public Screen {
public:
  CustomScreen(const String& title) : Screen(title) {}
  
  void draw() override {
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE);
    canvas.drawString("Custom content", 10, 50);
  }
};
```

### MenuScreen

A screen that displays a list of menu items.

#### `MenuScreen(const String& title)`
Create a new menu screen.
```cpp
MenuScreen mainMenu("Main Menu");
```

#### `void addItem(const String& name, Screen* targetScreen = nullptr, void (*callback)() = nullptr)`
Add an item to the menu.
```cpp
// Link to another screen
mainMenu.addItem("Settings", &settingsScreen);

// Callback function
mainMenu.addItem("Toggle LED", nullptr, toggleLED);

// Both (callback executes first)
mainMenu.addItem("Save & Exit", &mainMenu, saveSettings);
```

#### `void removeItem(int index)`
Remove an item by index.
```cpp
mainMenu.removeItem(0);  // Remove first item
```

#### `void clearItems()`
Remove all items from the menu.
```cpp
mainMenu.clearItems();
```

#### `int getItemCount()`
Get the number of items in the menu.
```cpp
int count = mainMenu.getItemCount();
```

### SettingsScreen

A specialized screen for managing settings.

#### `SettingsScreen(const String& title)`
Create a new settings screen.
```cpp
SettingsScreen settings("Device Settings");
```

#### `void addBooleanSetting(const String& name, bool defaultValue)`
Add a boolean (on/off) setting.
```cpp
settings.addBooleanSetting("WiFi Enabled", true);
```

#### `void addRangeSetting(const String& name, int min, int max, int defaultValue, const String& unit = "")`
Add a numeric range setting.
```cpp
settings.addRangeSetting("Brightness", 0, 100, 75, "%");
settings.addRangeSetting("Timeout", 1, 60, 30, "s");
```

#### `void addOptionSetting(const String& name, const char* options[], int optionCount, int defaultIndex)`
Add a multiple choice setting.
```cpp
const char* themes[] = {"Dark", "Light", "Auto"};
settings.addOptionSetting("Theme", themes, 3, 0);
```

#### `void addSubscreenSetting(const String& name, Screen* targetScreen)`
Add a setting that opens another screen.
```cpp
settings.addSubscreenSetting("Advanced", &advancedSettings);
```

#### Setting Value Access

```cpp
// Get setting values
bool wifiEnabled = settings.getBooleanValue(0);
int brightness = settings.getRangeValue(1);
int themeIndex = settings.getOptionValue(2);

// Set setting values
settings.setBooleanValue(0, false);
settings.setRangeValue(1, 80);
settings.setOptionValue(2, 1);
```

### CustomScreen

A screen with full control over drawing and input.

#### `CustomScreen(const String& title)`
Create a custom screen.
```cpp
CustomScreen customScreen("Custom Screen");
```

#### `std::function<void()> customDraw`
Lambda function for custom drawing logic. Set this in your `setup()` function.
```cpp
// Configure custom screen drawing in setup()
customScreen.customDraw = []() {
  canvas.drawSmoothRoundRect(-15, 50, 40, 0, 50, 50, TFT_BLUE, TFT_BLACK);
  canvas.drawSmoothRoundRect(10, 10, 200, 100, 20, 5, TFT_ORANGE, TFT_BLACK);
  canvas.drawSmoothRoundRect(120, -25, 40, 0, 40, 40, TFT_DARKGREEN, TFT_BLACK);
  canvas.drawString("V" + String(menu.getLibraryVersion()), 10, 10);
  canvas.setTextColor(TFT_WHITE, TFT_BLACK);
  canvas.drawString("Press UP for popup demo", 10, 30);
};
```



#### Complete Example
```cpp
// Define the custom screen globally
CustomScreen customScreen("Custom Screen");

void setup() {
  // Configure the custom drawing in setup()
  customScreen.customDraw = []() {
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE);
    canvas.drawString("OpenMenuOS v" + String(menu.getLibraryVersion()), 10, 10);
    canvas.drawString("Custom Screen Content", 10, 30);
    
    // Draw some graphics
    canvas.fillCircle(50, 80, 20, TFT_BLUE);
    canvas.drawRect(10, 110, 100, 30, TFT_GREEN);
    canvas.fillRect(12, 112, 96, 26, TFT_DARKGREEN);
    canvas.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    canvas.drawString("Press SELECT", 15, 120);
  };
  
  // Add to menu system
  menu.addScreen(&customScreen);
}
```

---

## Settings System

### Setting Structure

```cpp
struct Setting {
  String name;                // Display name
  SettingType type;          // Type of setting
  union {
    bool boolValue;          // For SETTING_BOOLEAN
    struct {                 // For SETTING_RANGE
      int value;
      int min;
      int max;
    } range;
    struct {                 // For SETTING_OPTION
      int selectedIndex;
      const char** options;
      int optionCount;
    } option;
    Screen* subscreenTarget; // For SETTING_SUBSCREEN
  };
  String unit;               // Unit for range settings (e.g., "%", "ms")
  bool hasBeenSet;          // Internal flag for EEPROM storage
};
```

### Setting Types

```cpp
enum SettingType {
  SETTING_BOOLEAN,    // True/false toggle
  SETTING_RANGE,      // Numeric value within range
  SETTING_OPTION,     // Multiple choice selection
  SETTING_SUBSCREEN   // Link to another screen
};
```

### Usage Examples

```cpp
// Boolean setting
Setting wifiSetting = {"WiFi", SETTING_BOOLEAN, {.boolValue = true}};

// Range setting
Setting brightnessSetting = {"Brightness", SETTING_RANGE, {.range = {75, 0, 100}}, "%"};

// Option setting
const char* modes[] = {"Auto", "Manual", "Off"};
Setting modeSetting = {"Mode", SETTING_OPTION, {.option = {0, modes, 3}}};

// Subscreen setting
Setting advancedSetting = {"Advanced", SETTING_SUBSCREEN, {.subscreenTarget = &advancedScreen}};
```

---

## Enums and Structures

### PopupType

```cpp
enum PopupType {
  POPUP_INFO,     // Blue information popup
  POPUP_SUCCESS,  // Green success popup
  POPUP_WARNING,  // Orange warning popup
  POPUP_ERROR,    // Red error popup
  POPUP_QUESTION  // Blue question popup with buttons
};
```

### PopupResult

```cpp
struct PopupResult {
  int buttonIndex;      // Index of clicked button (-1 if none)
  String buttonText;    // Text of clicked button
  bool timedOut;        // True if popup auto-closed
  unsigned long timestamp; // When interaction occurred
  
  // Comparison operators
  bool operator==(const PopupResult& other) const;
  bool operator!=(const PopupResult& other) const;
};

// Predefined results
static const PopupResult OK;       // {0, "OK", false, 0}
static const PopupResult CANCEL;   // {1, "Cancel", false, 0}
static const PopupResult TIMEOUT;  // {-1, "", true, 0}
```

### PopupConfig

```cpp
struct PopupConfig {
  uint8_t overlayOpacity;    // Background overlay opacity (0-255)
  uint8_t cornerRadius;      // Popup corner radius in pixels
  uint8_t shadowOffset;      // Shadow offset in pixels
  uint16_t animationSpeed;   // Animation duration in milliseconds
  uint8_t buttonSpacing;     // Space between buttons in pixels
  uint8_t padding;           // Internal popup padding in pixels
  uint16_t maxWidth;         // Maximum popup width (0 = auto)
  uint16_t maxHeight;        // Maximum popup height (0 = auto)
  bool wordWrap;             // Enable word wrapping for long text
  bool showIcons;            // Show type-specific icons
};
```

### ButtonState

```cpp
enum ButtonState {
  BUTTON_IDLE,      // No interaction
  BUTTON_PRESSED,   // Button currently pressed
  BUTTON_RELEASED,  // Button just released
  BUTTON_HELD       // Button held for long time
};
```

---

## Global Functions

### Utility Functions

#### `uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)`
Convert RGB values to 16-bit color format.
```cpp
uint16_t myColor = rgb565(255, 128, 0);  // Orange color
```

#### `void drawRoundedRect(int x, int y, int w, int h, int radius, uint16_t color)`
Draw a rounded rectangle on the canvas.
```cpp
drawRoundedRect(10, 10, 100, 50, 8, TFT_BLUE);
```

#### `void drawShadow(int x, int y, int w, int h, int offset, uint8_t opacity)`
Draw a shadow effect for UI elements.
```cpp
drawShadow(10, 10, 100, 50, 3, 128);
```

### Display Functions

#### `void drawCanvasOnTFT()`
Transfer the canvas buffer to the physical display.
```cpp
drawCanvasOnTFT();  // Usually called automatically
```

#### `void clearCanvas()`
Clear the canvas buffer.
```cpp
clearCanvas();
```

### Input Functions

#### `ButtonState getButtonState(int pin)`
Get the current state of a button pin.
```cpp
ButtonState state = getButtonState(SELECT_PIN);
if (state == BUTTON_PRESSED) {
  // Handle button press
}
```

#### `bool isEncoderMoved()`
Check if the encoder has been rotated.
```cpp
if (isEncoderMoved()) {
  int direction = getEncoderDirection();
  // Handle encoder movement
}
```

#### `int getEncoderDirection()`
Get encoder rotation direction (-1 for CCW, 1 for CW, 0 for none).
```cpp
int direction = getEncoderDirection();
if (direction > 0) {
  // Clockwise rotation
} else if (direction < 0) {
  // Counter-clockwise rotation
}
```

---

## Usage Examples

### Complete Basic Setup

```cpp
#include "OpenMenuOS.h"
#include "images.h"  // Include custom images if needed

//==============================================================================
// MENU SCREEN DEFINITIONS
//==============================================================================
OpenMenuOS menu;  // Main menu system instance

// Create menu screens
MenuScreen mainMenu("Main Menu");
MenuScreen testScreen("Test Menu");
CustomScreen customScreen("Custom Screen");
SettingsScreen settingsScreen("Settings");

//==============================================================================
// CALLBACK FUNCTIONS
//==============================================================================
void redirectToCustomScreen() {
  menu.redirectToScreen(&customScreen);
}

void showInfoPopup() {
  PopupManager::showInfo("This is an information message!", "Info Demo");
}

void toggleLED() {
  static bool ledState = false;
  ledState = !ledState;
  digitalWrite(LED_BUILTIN, ledState);
  PopupManager::showSuccess(ledState ? "LED On" : "LED Off");
}

//==============================================================================
// SETUP
//==============================================================================
void setup() {
  Serial.begin(115200);
  
  // Configure custom screen drawing
  customScreen.customDraw = []() {
    canvas.drawSmoothRoundRect(-15, 50, 40, 0, 50, 50, TFT_BLUE, TFT_BLACK);
    canvas.drawSmoothRoundRect(10, 10, 200, 100, 20, 5, TFT_ORANGE, TFT_BLACK);
    canvas.drawSmoothRoundRect(120, -25, 40, 0, 40, 40, TFT_DARKGREEN, TFT_BLACK);
    canvas.drawString("V" + String(menu.getLibraryVersion()), 10, 10);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("Press UP for popup demo", 10, 30);
  };
  
  // Configure main menu
  mainMenu.addItem("Settings", &settingsScreen, nullptr, (const uint16_t*)Menu_icon_1);
  mainMenu.addItem(&testScreen);
  mainMenu.addItem("Custom Screen", nullptr, redirectToCustomScreen);
  mainMenu.addItem("LED Control", nullptr, toggleLED);
  mainMenu.addItem("Info Popup", nullptr, showInfoPopup);
  
  // Configure test menu
  testScreen.addItem("First Test Page");
  testScreen.addItem("Second Test Page");
  testScreen.addItem(&customScreen);
  
  // Configure settings
  settingsScreen.addBooleanSetting("Animations", true);
  settingsScreen.addBooleanSetting("Optimize Display Updates", true);
  settingsScreen.addRangeSetting("Brightness", 0, 100, 75, "%");
  
  const char* styleOptions[] = {"Default", "Modern"};
  settingsScreen.addOptionSetting("Style", styleOptions, 2, 1);
  
  // Configure menu appearance and behavior
  menu.useStylePreset("Rabbit_R1");
  menu.setScrollbar(true);
  menu.setScrollbarStyle(1);
  menu.setButtonsMode("low");
  
  // Configure input
  menu.setEncoderPin(5, 2);  // Enable encoder support
  menu.setSelectPin(19);
  
  menu.setDisplayRotation(0);
  
  // Initialize the menu system
  menu.begin(&mainMenu);
}

//==============================================================================
// MAIN LOOP
//==============================================================================
void loop() {
  // Handle popup interactions first
  PopupResult popupResult = PopupManager::update();
  
  // Handle menu logic
  menu.loop();
  
  // Handle popup results when user interacts with popup
  if (popupResult != PopupResult::NONE) {
    switch (popupResult) {
      case PopupResult::OK:
        Serial.println("User clicked OK/Yes");
        break;
      case PopupResult::CANCEL:
        Serial.println("User clicked Cancel/No");
        break;
      default:
        break;
    }
  }
  
  // Update settings-based configurations
  menu.setAnimation(settingsScreen.getSettingValue("Animations"));
  menu.setOptimizeDisplayUpdates(settingsScreen.getSettingValue("Optimize Display Updates"));
}
```

### Advanced Popup Usage

```cpp
void handleUserChoice() {
  std::vector<String> options = {"Save", "Don't Save", "Cancel"};
  PopupResult result = PopupManager::showQuestion(
    "You have unsaved changes.\\nWhat would you like to do?",
    "Unsaved Changes",
    options
  );
  
  switch (result.buttonIndex) {
    case 0:  // Save
      saveData();
      PopupManager::showSuccess("Data saved successfully!");
      break;
    case 1:  // Don't Save
      PopupManager::showWarning("Changes discarded", "Warning");
      break;
    case 2:  // Cancel
      // Do nothing, stay on current screen
      break;
    default:
      // Timed out or other
      break;
  }
}

void saveData() {
  // Simulate save operation
  delay(1000);
  
  if (saveSuccessful) {
    PopupManager::showSuccess("Data saved!", "Success", 2000);
  } else {
    PopupManager::showError("Save failed!", "Error");
  }
}
```

### Custom Screen Implementation

```cpp
// Define custom screen globally
CustomScreen graphScreen("Sensor Graph");

class GraphData {
private:
  static float dataPoints[50];
  static int dataCount;
  
public:
  static void addDataPoint(float value) {
    if (dataCount < 50) {
      dataPoints[dataCount++] = value;
    } else {
      // Shift array and add new point
      for (int i = 0; i < 49; i++) {
        dataPoints[i] = dataPoints[i + 1];
      }
      dataPoints[49] = value;
    }
  }
  
  static void drawGraph() {
    canvas.fillScreen(TFT_BLACK);
    canvas.setTextColor(TFT_WHITE);
    canvas.drawString("Sensor Data", 10, 10);
    
    // Draw axes
    canvas.drawLine(20, 30, 20, 130, TFT_WHITE);  // Y-axis
    canvas.drawLine(20, 130, 220, 130, TFT_WHITE); // X-axis
    
    // Plot data points
    for (int i = 1; i < dataCount; i++) {
      int x1 = 20 + (i - 1) * 4;
      int y1 = 130 - (int)(dataPoints[i - 1] * 100);
      int x2 = 20 + i * 4;
      int y2 = 130 - (int)(dataPoints[i] * 100);
      canvas.drawLine(x1, y1, x2, y2, TFT_CYAN);
    }
  }
  

};

// Initialize static members
float GraphData::dataPoints[50];
int GraphData::dataCount = 0;

void setup() {
  // Configure the custom screen with lambda functions
  graphScreen.customDraw = []() {
    GraphData::drawGraph();
  };
  
  
  // Add to menu
  menu.addScreen(&graphScreen);
  
  // Add some initial data
  for (int i = 0; i < 10; i++) {
    GraphData::addDataPoint(random(100) / 100.0);
  }
}

void loop() {
  menu.loop();
  
  // Continuously add new sensor data
  static unsigned long lastDataUpdate = 0;
  if (millis() - lastDataUpdate > 1000) {  // Every second
    float sensorValue = analogRead(A0) / 1024.0;
    GraphData::addDataPoint(sensorValue);
    lastDataUpdate = millis();
  }
}
```

---

## Best Practices

### Performance Optimization

1. **Enable Frame Comparison**: Use `setOptimizeDisplayUpdates(true)` for better performance
2. **Disable Animations**: Use `setAnimation(false)` on slower devices
3. **Limit Menu Items**: Keep menus concise to reduce memory usage
4. **Use Static Callbacks**: Prefer static functions for menu callbacks

### Memory Management

1. **Initialize in setup()**: Create screens and menus in `setup()`, not in callbacks
2. **Use References**: Pass screens by reference when possible
3. **Limit Recursion**: Avoid deeply nested menu structures
4. **Clean Up**: Remove unused screens and menu items

### User Experience

1. **Provide Feedback**: Use popups to confirm actions
2. **Keep Text Short**: Long text may scroll or be truncated
3. **Use Appropriate Colors**: Follow color conventions (red for errors, green for success)
4. **Test on Hardware**: Always test on actual devices, not just emulators

---

## Error Handling

### Common Issues

#### Display Not Working
```cpp
// Check TFT_eSPI configuration
#if !defined(TFT_eSPI_VERSION)
  #error "TFT_eSPI library not found or configured"
#endif

// Verify display initialization
if (tft.width() == 0 || tft.height() == 0) {
  Serial.println("Display initialization failed");
}
```

#### Button Not Responding
```cpp
// Verify button configuration
void checkButtons() {
  Serial.println("UP: " + String(digitalRead(UP_PIN)));
  Serial.println("SELECT: " + String(digitalRead(SELECT_PIN)));
  
  // Check button mode
  if (buttonsMode == INPUT_PULLUP && buttonVoltage == HIGH) {
    Serial.println("Warning: Inconsistent button configuration");
  }
}
```

#### Memory Issues
```cpp
// Monitor free heap
void checkMemory() {
  #ifdef ESP32
    Serial.println("Free heap: " + String(ESP.getFreeHeap()));
    Serial.println("Min free heap: " + String(ESP.getMinFreeHeap()));
  #endif
  
  #ifdef ESP8266
    Serial.println("Free heap: " + String(ESP.getFreeHeap()));
  #endif
}
```

### Debugging Tips

```cpp
// Enable verbose output
#define OPENMENUOS_DEBUG 1

// Check library version
void printSystemInfo() {
  Serial.println("OpenMenuOS Version: " + menu.getLibraryVersion());
  Serial.println("Display Size: " + String(menu.getTftWidth()) + "x" + String(menu.getTftHeight()));
  Serial.println("Current Screen: " + String(currentScreen ? currentScreen->getTitle() : "None"));
}

// Monitor popup state
void checkPopupState() {
  if (PopupManager::isActive()) {
    Serial.println("Popup is active");
  } else {
    PopupResult lastResult = PopupManager::getResult();
    if (lastResult.buttonIndex != -1) {
      Serial.println("Last popup result: " + lastResult.buttonText);
    }
  }
}
```

---

**OpenMenuOS v3.1.0 API Reference**  
Last Updated: June 15, 2025  
For the latest documentation, visit: https://github.com/The-Young-Maker/OpenMenuOS
