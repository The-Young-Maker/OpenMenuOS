#include "OpenMenuOS.h"
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

OpenMenuOS menu(10, 5, 12, 4, 21, 22, "Flashlight", "Profile", "Heat", "Reflow", "Settings", "Wifi", "Package", "Survival", "Other", NULL);  // Setup the menu instance with the Buttons pins (2) the display pins (3) and the menu item's names (3-20)


void setup() {
  Serial.begin(921600);
  // menu.printMenuToSerial(); // Call printMenu() method on the menu instance
  menu.begin();
}

void loop() {
  menu.loop();
  menu.checkSerial();
  menu.connectToWiFi();

  if (menu.getCurrentScreen() == 0) {
    delay(100);
    menu.drawMenu();
  } else if (menu.getCurrentScreen() == 1) {
    switch (menu.getSelectedItem()) {
      case 0:  //
        {
          Serial.print(menu.getSelectedItem());
          menu.canvas.setFont(&FreeMono9pt7b);
          menu.canvas.setTextSize(1);
          menu.canvas.setTextColor(ST7735_WHITE);
          menu.canvas.setCursor(10, 10);
          menu.canvas.print("123");
          break;
        }
      case 1:  //
        {
          Serial.print(menu.getSelectedItem());
          break;
        }
      case 2:  //
        {
          Serial.print(menu.getSelectedItem());
          break;
        }
      case 3:  //
        {
          Serial.print(menu.getSelectedItem());
          break;
        }
      case 4:  //Settings
        {
          menu.drawSettingMenu("Wifi", "Open Wifi", "Bluetooth", "Backlight", "OTA", "other...", NULL);
          break;
        }
      case 5:  //
        {
          Serial.print(menu.getSelectedItem());
          break;
        }
      case 6:  //
        {
          Serial.print(menu.getSelectedItem());
          break;
        }
      case 7:  //
        {
          Serial.print(menu.getSelectedItem());
          break;
        }
        Serial.print(menu.getSelectedItem());

      case 8:  // Other
        {
          Serial.print(menu.getSelectedItem());
          break;
        }
    }
    menu.drawCanvasOnTFT();
  }
  if (menu.menu_items_settings_bool[2]) {
    Serial.print("menu item bool");
  }
}
