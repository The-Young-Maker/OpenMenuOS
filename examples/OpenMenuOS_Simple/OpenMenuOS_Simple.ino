#include <OpenMenuOS.h>               // Include the OpenMenuOS library
#include <Fonts/FreeMonoBold9pt7b.h>  // Include the required fonts
#include <Fonts/FreeMono9pt7b.h>

// Create an instance of the OpenMenuOS class with button and display pins, along with menu item names
OpenMenuOS menu(10, 11, 5, 12, 4, 21, 22);  //btn_up, btn_down, btn_sel, tft_bl, cs, dc, rst


void setup() {
  Serial.begin(921600);                                     // Initialize serial communication
  menu.begin(INITR_GREENTAB, "ESP32_OpenMenuOS", "esp32");  // Initialize the menu, parameters :  Display type, OTA Hostname, OTA Password
}

void loop() {
  menu.loop();           // Handle menu logic
  menu.checkSerial();    // Check for serial input
  menu.connectToWiFi();  // Attempt to connect to WiFi if enabled

  // Display the menu if the current screen is the main menu (screen 0)
  if (menu.getCurrentScreen() == 0) {
    delay(100);                                                     // Small delay for stability
    menu.drawMenu(true, "Tile Menu", "Submenu", "Settings", NULL);  // Draw the main menu on the screen. Set to "true" to display the 16x16 images for the items, else, set to "false"
  }
  // If the current screen is a submenu (screen 1)
  else if (menu.getCurrentScreen() == 1) {
    // Handle actions based on the selected menu item
    switch (menu.getSelectedItem()) {
      case 0:  // Tile Menu
        menu.drawTileMenu(2, 2, ST7735_GREEN);
        break;
      case 1:  // Submenu
        if (menu.getCurrentScreen() == 1) {
          delay(100);                                        // Small delay for stability
          menu.drawSubmenu(true, "1", "2", "3", "4", NULL);  // Draw the submenu on the screen. Set to "true" to display the 16x16 images for the items, else, set to "false"
        } else if (menu.getCurrentScreen() == 2) {
          // Handle actions based on the selected submenu item
          switch (menu.getSelectedItem()) {
            case 0:
              menu.canvas.fillScreen(ST7735_BLACK);
              break;
            case 1:
              menu.canvas.fillScreen(ST7735_BLACK);
              break;
            case 2:
              menu.canvas.fillScreen(ST7735_BLACK);
              break;
            case 3:
              menu.canvas.fillScreen(ST7735_BLACK);
              break;
          }
        }
        break;
      case 2:  // Settings
        // Draw the setting menu on the screen
        menu.drawSettingMenu("Setting_6", "Setting_7", "Setting_8", NULL);  // 5 items are by default in the settings. The one defined in this function are added after the one there by default
        break;
    }
  }
  // Additional functionality based on menu settings
  if (menu.menu_items_settings_bool[5]) {
    // Execute code if a specific menu item setting is enabled
    Serial.println("Menu item bool 5 is true");
  }
  menu.drawCanvasOnTFT();  // Draw the updated canvas on the screen. You need to call it at the END of your code (in the end of "loop()")
}