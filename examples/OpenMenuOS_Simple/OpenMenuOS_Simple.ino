//==============================================================================
// INCLUDES
//==============================================================================
#include "OpenMenuOS.h" // Include the OpenMenuOS library
#include "images.h"     // Include custom images

//==============================================================================
// MENU SCREEN DEFINITIONS
//==============================================================================
// Create an instance of the OpenMenuOS class with button pins
// Use -1 for buttons you don't have
// OpenMenuOS menu(2, 5, 19);  // btn_up, btn_down, btn_sel
OpenMenuOS menu; // btn_up, btn_down, btn_sel

// Create menu screens
MenuScreen mainMenu("Main Menu");
MenuScreen testScreen("Test Menu");
CustomScreen customScreen("Custom Screen");
SettingsScreen settingsScreen("Settings");
SettingsScreen speakerSettingsScreen("Advanced Settings");

//==============================================================================
// CALLBACK FUNCTIONS
//==============================================================================
// Example action callback for redirection
void redirectToCustomScreen()
{
  menu.redirectToScreen(&customScreen);
}

// Popup demonstration callbacks
void showInfoPopup()
{
  PopupManager::showInfo("This is an information message!", "Info Demo");
}

void showSuccessPopup()
{
  PopupManager::showSuccess("Operation completed successfully!");
}

void showWarningPopup()
{
  PopupManager::showWarning("This action cannot be undone", "Warning!");
}

void showErrorPopup()
{
  PopupManager::showError("Failed to save settings", "Error");
}

void showQuestionPopup()
{
  PopupResult result = PopupManager::showQuestion("Are you sure you want to delete all data?", "Confirm Delete");
  // Note: Result will be handled in the main loop via PopupManager::update()
}

void showCustomPopup()
{
  PopupConfig config;
  config.title = "Custom Popup";
  config.message = "This is a custom popup with auto-close feature enabled. It will close automatically after 5 seconds.";
  config.type = PopupType::INFO;
  config.autoClose = true;
  config.autoCloseDelay = 5000; // 5 seconds
  config.customColor = 0x7E0;   // Custom green color
  PopupManager::show(config);
}

//==============================================================================
// SETUP
//==============================================================================
void setup()
{
  // Initialize serial communication
  Serial.begin(921600);
  while (!Serial)
  {
  }
  Serial.println("Menu system started.");
  // Configure custom screen drawing
  customScreen.customDraw = []()
  {
    canvas.drawSmoothRoundRect(-15, 50, 40, 0, 50, 50, TFT_BLUE, TFT_BLACK);
    canvas.drawSmoothRoundRect(10, 10, 200, 100, 20, 5, TFT_ORANGE, TFT_BLACK);
    canvas.drawSmoothRoundRect(120, -25, 40, 0, 40, 40, TFT_DARKGREEN, TFT_BLACK);
    canvas.drawString("V" + String(menu.getLibraryVersion()), 10, 10);
    canvas.setTextColor(TFT_WHITE, TFT_BLACK);
    canvas.drawString("Press UP for popup demo", 10, 30);
  };

  // Configure sound settings
  speakerSettingsScreen.addRangeSetting("Speaker Power", 1, 30, 5, "dB");
  speakerSettingsScreen.addBooleanSetting("Sound", false);

  // Configure main settings
  settingsScreen.addBooleanSetting("Animations", true);                  // Toggle setting
  settingsScreen.addBooleanSetting("Animations", true);                  // Toggle setting
  settingsScreen.addBooleanSetting("Optimize Display Updates", true);    // Toggle setting
  settingsScreen.addRangeSetting("Brightness", 0, 100, 75, "%");         // Range setting
  settingsScreen.addSubscreenSetting("Speaker", &speakerSettingsScreen); // Subscreen link

  // Add style options
  const char *styleOptions[] = {"Default", "Modern"};
  settingsScreen.addOptionSetting("Style", styleOptions, 2, 1); // Option setting
  // Configure main menu
  mainMenu.addItem("Settings", &settingsScreen, nullptr, (const uint16_t *)Menu_icon_1);
  mainMenu.addItem(&testScreen);
  mainMenu.addItem("Redirect To Screen", nullptr, redirectToCustomScreen);
  mainMenu.addItem("Info Popup", nullptr, showInfoPopup);
  mainMenu.addItem("Success Popup", nullptr, showSuccessPopup);
  mainMenu.addItem("Warning Popup", nullptr, showWarningPopup);
  mainMenu.addItem("Error Popup", nullptr, showErrorPopup);
  mainMenu.addItem("Question Popup", nullptr, showQuestionPopup); // With encoder: rotate to select Yes/No, press to confirm
  mainMenu.addItem("Custom Popup", nullptr, showCustomPopup);
  // Configure test menu
  testScreen.addItem("First Test Page");
  testScreen.addItem("Second Test Page");
  testScreen.addItem("Third Test Page");
  testScreen.addItem("Quick Info Popup", nullptr, showInfoPopup); // Popup from submenu
  testScreen.addItem(&customScreen);

  // Configure menu appearance and behavior
  menu.useStylePreset("Rabbit_R1"); // Available presets: "Default", "Rabbit_R1" OR the number of the preset (0, 1, 2, etc.)
  menu.setScrollbar(true);          // Enable scrollbar
  menu.setScrollbarStyle(1);        // Styles: 0 (Default), 1 (Modern)
  menu.setButtonsMode("low");       // Button active voltage: "High" or "Low"

  // Optional configurations
  // menu.setMenuStyle(1);                     // Styles: 0 (Default), 1 (Modern)  // menu.setSelectionBorderColor(0xFFFF);     // Set selection border color
  // menu.setSelectionFillColor(0x0000);       // Set selection fill color
  menu.setEncoderPin(5, 2); // Configure encoder pins (also enables encoder support for popups)
  menu.setSelectPin(19);

  menu.setDisplayRotation(0); // Set display rotation

  // Initialize the menu system
  menu.begin(&mainMenu); // Parameter:  Main menu object
}

//==============================================================================
// MAIN LOOP
//==============================================================================
void loop()
{
  // Handle popup interactions first
  PopupResult popupResult = PopupManager::update();

  menu.loop(); // Handle menu logic

  // Handle popup results when user interacts with popup
  if (popupResult != PopupResult::NONE)
  {
    // Handle popup results
    switch (popupResult)
    {
    case PopupResult::OK:
      Serial.println("User clicked OK/Yes");
      // Add your OK/Yes handling logic here
      break;
    case PopupResult::CANCEL:
      Serial.println("User clicked Cancel/No");
      // Add your Cancel/No handling logic here
      break;
    default:
      break;
    }
  }

  // Update settings-based configurations
  menu.setAnimation(settingsScreen.getSettingValue("Animations"));
  menu.setOptimizeDisplayUpdates(settingsScreen.getSettingValue("Optimize Display Updates"));
  // menu.setMenuStyle(settingsScreen.getSettingValue("Style"));
}