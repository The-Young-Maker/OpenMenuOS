#include "OpenMenuOS.h"               // Include the OpenMenuOS library
#include <Fonts/FreeMonoBold9pt7b.h>  // Include the required fonts
#include <Fonts/FreeMono9pt7b.h>

// Create an instance of the OpenMenuOS class with button and display pins, along with menu item names
OpenMenuOS menu(10, 5, 12, 4, 21, 22, "Flashlight", "Profile", "Heat", "Reflow", "Settings", "Wifi", "Package", "Survival", "Other", NULL);

void setup() {
  Serial.begin(921600);  // Initialize serial communication
  menu.begin();          // Initialize the menu
}

void loop() {
  menu.loop();           // Continuously check for button presses and handle menu logic
  menu.checkSerial();    // Check for serial input
  menu.connectToWiFi();  // Attempt to connect to WiFi if enabled

  // Display the menu if the current screen is the main menu (screen 0)
  if (menu.getCurrentScreen() == 0) {
    delay(100);       // Small delay for stability
    menu.drawMenu();  // Draw the main menu on the screen
  }
  // If the current screen is a submenu (screen 1)
  else if (menu.getCurrentScreen() == 1) {
    // Handle actions based on the selected menu item
    switch (menu.getSelectedItem()) {
      case 0:  // Flashlight
        // Implement flashlight functionality
        break;
      case 1:  // Profile
        // Implement profile functionality
        break;
      case 2:  // Heat
        // Implement heat functionality
        break;
      case 3:  // Reflow
        // Implement reflow functionality
        break;
      case 4:  // Settings
        // Draw the setting menu on the screen
        menu.drawSettingMenu("Setting_6", "Setting_7", "Setting_8", NULL);// 5 items are by default in the settings. The one defined in this function are added after the one there by default
        break;
      case 5:  // Wifi
        // Implement WiFi functionality
        break;
      case 6:  // Package
        // Implement package functionality
        break;
      case 7:  // Survival
        // Implement survival functionality
        break;
      case 8:  // Other
        // Implement other functionality
        break;
    }
    menu.drawCanvasOnTFT();  // Draw the updated canvas on the screen
  }

  // Additional functionality based on menu settings
  if (menu.menu_items_settings_bool[5]) {
    // Execute code if a specific menu item setting is enabled
    Serial.print("menu item bool");
  }
}
