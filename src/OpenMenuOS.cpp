/*
  OpenMenuOS.h - Library to create menu on ST7735 display.
  Created by Loic Daigle aka The Young Maker.
  Released into the public domain.
*/


#pragma message "The OpenMenuOS library is still in Beta. If you find any bug, please create an issue on the OpenMenuOS's Github repository"

#include "Arduino.h"
#include <ArduinoOTA.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <EEPROM.h>
#include <ESPmDNS.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include "images.h"
#include "OpenMenuOS.h"
#include <Preferences.h>
#include <string>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>


// Display Constants
int tftWidth = 160;
int tftHeight = 80;

// Rectangle dimensions
const uint16_t rect_width = 155;  // Width of the selection rectangle
const uint16_t rect_height = 26;  // Height of the selection rectangle


// Menu Item Positioning Variables
int item_sel_previous;  // Previous item - used in the menu screen to draw the item before the selected one
int item_selected;      // Current item -  used in the menu screen to draw the selected item
int item_sel_next;      // Next item - used in the menu screen to draw next item after the selected one

// Settings Item Positioning Variables
int item_selected_settings_previous;  // Previous item - used in the menu screen to draw the item before the selected one
int item_selected_settings = 0;       // Current item -  used in the menu screen to draw the selected item
int item_selected_settings_next;      // Next item - used in the menu screen to draw next item after the selected one

// Wi-Fi Credentials
String ssid1;      // SSID of the first WiFi network
String password1;  // Password of the first WiFi network
String ssid2;      // SSID of the second WiFi network
String password2;  // Password of the second WiFi network

// Button Press Timing and Control
unsigned long select_button_press_time = 0;
int button_up_clicked = 0;           // Only perform action when the button is clicked, and wait until another press
int button_up_clicked_settings = 0;  // Only perform action when the button is clicked, and wait until another press
int button_select_clicked = 0;
bool previousButtonState = LOW;
bool buttonPressProcessed = false;

// Button Pin Definitions
int BUTTON_UP_PIN;      // Pin for UP button
int BUTTON_SELECT_PIN;  // Pin for SELECT button
int TFT_BL_PIN;         // Pin for TFT Backlight

// Button Constants
#define SELECT_BUTTON_LONG_PRESS_DURATION 300

// Timing Constants
unsigned long previousMillis = 0;      // Variable to store the time in milliseconds of the last execution
const unsigned long interval = 30000;  // 30 seconds (in milliseconds)

#define EEPROM_SIZE (MAX_SETTINGS_ITEMS * sizeof(bool) + (32 + 1) + (64 + 1) + (32 + 1) + (64 + 1))  // Possibly change the "MAX_SETTINGS_ITEMS" to the value of NUM_SETTINGS_ITEMS if setting array made before eeprom ; to also do in readFromEEPROM and saveToEEPROM
int NUM_SETTINGS_ITEMS = 5;

char menu_items_settings[MAX_SETTINGS_ITEMS][MAX_ITEM_LENGTH] = {
  // array with item names
  { "Wifi" },
  { "Open wifi" },
  { "Bluetooth" },
  { "Backlight" },
  { "OTA" },
};


bool OpenMenuOS::menu_items_settings_bool[MAX_SETTINGS_ITEMS] = {
  true,   // Wifi
  false,  // Open Wifi
  true,   // Bluetooth
  false,  // Backlight
  false,  // OTA Updates
  false,
  false,
  false,
  false,
  false,
};


OpenMenuOS::OpenMenuOS(int btn_up, int btn_sel, int tft_bl, int cs, int dc, int rst, const char* names...)
  : tft(cs, dc, rst), canvas(160, 80) {
  BUTTON_UP_PIN = btn_up;
  BUTTON_SELECT_PIN = btn_sel;
  TFT_BL_PIN = tft_bl;
  NUM_MENU_ITEMS = 0;
  va_list args;
  va_start(args, names);
  const char* name = names;
  while (name != NULL) {
    strncpy(menu_items[NUM_MENU_ITEMS], name, MAX_ITEM_LENGTH);
    name = va_arg(args, const char*);
    NUM_MENU_ITEMS++;
  }
  va_end(args);
}

void OpenMenuOS::begin() {
  item_selected = 3;
  current_screen = 0;  // 0 = Menu, 1 = Submenu

  // Set up display
  tft.initR(INITR_GREENTAB);
  tft.setRotation(3);
  canvas.fillScreen(ST7735_BLACK);

  // Initialize EEPROM
  prefs.begin("Settings");  //namespace
  EEPROM.begin(EEPROM_SIZE);
  readFromEEPROM();

  // If enabled in settings, connect to wifi
  WiFi.mode(WIFI_STA);
  connectToWiFi();

  // Set TFT_BL_PIN as OUTPUT and se it HIGH or LOW depending on the settings
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, menu_items_settings_bool[4] ? LOW : HIGH);

  // Set up ArduinoOTA for Over-The-Air updates
  ArduinoOTA.setHostname("ESP32-OpenMenuOS");
  ArduinoOTA.setPassword("esp32");
  if (menu_items_settings_bool[6]) {
    ArduinoOTA.begin();
  }

  // Set up button pins
  pinMode(BUTTON_UP_PIN, INPUT);
  pinMode(BUTTON_SELECT_PIN, INPUT);
}
void OpenMenuOS::loop() {
  if (menu_items_settings_bool[6] == true) {
    ArduinoOTA.handle();
  }
  // checkSerial();
  // connectToWiFi();
  checkForButtonPress();
}

void OpenMenuOS::checkSerial() {
  String receivedInput;
  // Check if there is data available to read from Serial
  if (Serial.available() > 0) {
    // Read the input until a newline character is encountered
    receivedInput = Serial.readStringUntil('\n');

    // Check if the input starts with "wifi1:" as a prefix
    if (receivedInput.startsWith("wifi1:")) {
      // Remove the "wifi1:" prefix
      receivedInput.remove(0, 6);

      // Find the position of the comma in the input
      int commaPosition = receivedInput.indexOf(',');

      // Check if a comma is found
      if (commaPosition != -1) {
        // Extract SSID and password using substring
        ssid1 = receivedInput.substring(0, commaPosition);
        password1 = receivedInput.substring(commaPosition + 1);

        // Print the extracted SSID and password
        Serial.print("SSID 1: ");
        Serial.println(ssid1);
        Serial.print("Password 1: ");
        Serial.println(password1);
        if (ssid1.length() > 0) {
          prefs.putString("ssid1", ssid1);
        }

        if (password1.length() > 0) {
          prefs.putString("password1", password1);
        }
        connectToWiFi();
      } else {
        Serial.println("Invalid format. Please use 'wifi1:ssid1,password1'");
      }
    } else if (receivedInput.startsWith("wifi2:")) {
      // Remove the "wifi2:" prefix
      receivedInput.remove(0, 6);

      // Find the position of the comma in the input
      int commaPosition = receivedInput.indexOf(',');

      // Check if a comma is found
      if (commaPosition != -1) {
        // Extract SSID and password using substring
        ssid2 = receivedInput.substring(0, commaPosition);
        password2 = receivedInput.substring(commaPosition + 1);

        // Print the extracted SSID and password
        Serial.print("SSID 2: ");
        Serial.println(ssid2);
        Serial.print("Password 2: ");
        Serial.println(password2);
        if (prefs.getString("ssid2").length() > 0) {
          ssid2 = prefs.getString("ssid2");
        }

        if (prefs.getString("password2").length() > 0) {
          password2 = prefs.getString("password2");
        }
        connectToWiFi();
      } else {
        Serial.println("Invalid format. Please use 'wifi2:ssid2,password2'");
      }
    } else {
      Serial.println("Invalid command. Available command are :'wifi1:ssid1,password1' , 'wifi2:ssid2,password2'");
    }
  }
}
void OpenMenuOS::connectToWiFi() {
  if (menu_items_settings_bool[0] == true) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
      if (WiFi.status() != WL_CONNECTED) {

        int numNetworks = WiFi.scanNetworks();
        if (numNetworks == 0) {
          Serial.println("No network found.");
          return;
        }

        for (int i = 0; i < numNetworks; i++) {
          String ssid = WiFi.SSID(i);
          int rssi = WiFi.RSSI(i);

          if (ssid.equals(ssid1)) {
            Serial.println("Connection to the first network...");
            WiFi.begin(ssid1, password1);
            while (WiFi.status() != WL_CONNECTED) {
              delay(1000);
              Serial.print(".");
            }

            if (WiFi.status() == WL_CONNECTED) {
              Serial.println("Connected to the first network!");
              return;
            }
          }

          if (ssid.equals(ssid2)) {
            Serial.println("Connection to the second network...");
            WiFi.begin(ssid2, password2);
            while (WiFi.status() != WL_CONNECTED) {
              delay(1000);
              Serial.print(".");
            }

            if (WiFi.status() == WL_CONNECTED) {
              Serial.println("Connected to the second network!");
              return;
            }
          }
        }

        Serial.println("Failed to connect to networks.");
        if (menu_items_settings_bool[1] == true) {
          connectToStrongestOpenWiFi();
        }
      }
    }
  }
}
void OpenMenuOS::connectToStrongestOpenWiFi() {
  // Scan for available Wi-Fi networks
  int numNetworks = WiFi.scanNetworks();
  Serial.print("Found ");
  Serial.print(numNetworks);
  Serial.println(" networks");

  // Find the strongest open network
  int strongestSignal = -100;  // Set a low initial value for signal strength
  int strongestIndex = -1;

  for (int i = 0; i < numNetworks; ++i) {
    if (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) {
      int signal = WiFi.RSSI(i);
      Serial.print("Network: ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" | Signal strength: ");
      Serial.println(signal);

      if (signal > strongestSignal) {
        strongestSignal = signal;
        strongestIndex = i;
      }
    }
  }

  // Connect to the strongest open network if found
  if (strongestIndex != -1) {
    Serial.print("Connecting to: ");
    Serial.println(WiFi.SSID(strongestIndex));
    WiFi.begin(WiFi.SSID(strongestIndex).c_str());

    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
      Serial.print(".");
    }

    Serial.println("\nConnected to WiFi");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("No open Wi-Fi networks found");
  }
}
void OpenMenuOS::drawMenu() {
  // Calculate the position of the rectangle
  uint16_t rect_x = (tftWidth - rect_width - 4) / 2;                               // Center the rectangle horizontally with a 4-pixel space on the right
  uint16_t rect_y = (tftHeight - rect_height) / 2;                                 // Center the rectangle vertically
  canvas.drawFastVLine(153, 29, 22, ST7735_WHITE);                                 // Display the Shadow
  canvas.drawFastHLine(3, 51, 150, ST7735_WHITE);                                  // Display the Shadow
  canvas.drawRoundRect(rect_x, rect_y, rect_width, rect_height, 4, ST7735_WHITE);  // Display the rectangle

  int xPos = 30;   // Position X du texte 0
  int yPos = 17;   // Position Y du texte 0
  int x1Pos = 30;  // Position X du texte 1
  int y1Pos = 44;  // Position Y du texte 1
  int x2Pos = 30;  // Position X du texte 2
  int y2Pos = 70;  // Position Y du texte 2

  canvas.fillRoundRect(rect_x + 1, rect_y - 25, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);  // Effacer les ancien texte
  canvas.fillRoundRect(rect_x + 1, rect_y + 27, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 1, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);


  // draw previous item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(xPos, yPos);
  canvas.println(menu_items[item_sel_previous]);

  canvas.drawRGBBitmap(5, 5, (uint16_t*)bitmap_icons[item_sel_previous], 16, 16);
  // draw selected item as icon + label in bold font
  canvas.setFont(&FreeMonoBold9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(x1Pos, y1Pos);
  canvas.println(menu_items[item_selected]);

  canvas.drawRGBBitmap(5, 32, (uint16_t*)bitmap_icons[item_selected], 16, 16);
  // draw next item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(x2Pos, y2Pos);
  canvas.println(menu_items[item_sel_next]);

  canvas.drawRGBBitmap(5, 59, (uint16_t*)bitmap_icons[item_sel_next], 16, 16);

  for (int y = 0; y < tftHeight; y++) {  // Display the Scrollbar
    if (y % 2 == 0) {
      canvas.drawPixel(tftWidth - 2, y, ST7735_WHITE);
    }
  }
  // Draw scrollbar handle
  int boxHeight = tftHeight / (NUM_MENU_ITEMS);
  int boxY = boxHeight * item_selected;
  // Clear previous scrollbar handle
  canvas.fillRect(tftWidth - 3, boxHeight * item_sel_next, 3, boxHeight, ST7735_BLACK);
  // Draw new scrollbar handle
  canvas.fillRect(tftWidth - 3, boxY, 3, boxHeight, ST7735_WHITE);

  for (int y = 0; y < tftHeight; y++) {  // Display the Scrollbar
    if (y % 2 == 0) {
      canvas.drawPixel(tftWidth - 2, y, ST7735_WHITE);
    }
  }
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height());
  canvas.fillScreen(ST7735_BLACK);
}
void OpenMenuOS::drawSettingMenu(const char* items...) {

  va_list args;
  va_start(args, items);
  const char* item = items;
  while (item != NULL && NUM_SETTINGS_ITEMS < MAX_SETTINGS_ITEMS) {
    strncpy(menu_items_settings[NUM_SETTINGS_ITEMS++], item, MAX_ITEM_LENGTH - 1);
    item = va_arg(args, const char*);
  }
  va_end(args);

  if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked_settings == 0)) {  // up button clicked - jump to previous menu item
    item_selected_settings = item_selected_settings - 1;                            // select previous item
    button_up_clicked_settings = 1;                                                 // set button to clicked to only perform the action once
    if (item_selected_settings < 0) {                                               // if first item was selected, jump to last item
      item_selected_settings = NUM_SETTINGS_ITEMS - 1;
    }
  }

  if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked_settings == 1)) {  // unclick
    button_up_clicked_settings = 0;
  }

  uint16_t rect_x = (tftWidth - rect_width) / 2;                                   // Center the rectangle horizontally with a 4-pixel space on the right
  uint16_t rect_y = (tftHeight - rect_height) / 2;                                 // Center the rectangle vertically
  canvas.drawFastVLine(155, 29, 22, ST7735_WHITE);                                 // Display the Shadow
  canvas.drawFastHLine(3, 51, 150, ST7735_WHITE);                                  // Display the Shadow
  canvas.drawRoundRect(rect_x, rect_y, rect_width, rect_height, 4, ST7735_WHITE);  // Display the rectangle

  canvas.fillRoundRect(rect_x + 1, rect_y - 25, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 27, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);
  canvas.fillRoundRect(rect_x + 1, rect_y + 1, rect_width - 3, rect_height - 3, 4, ST7735_BLACK);

  // draw next item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(10, 17);
  canvas.println(menu_items_settings[item_selected_settings_previous]);
  if (item_selected_settings_previous >= 0) {
    if (menu_items_settings_bool[item_selected_settings_previous] == false) {
      canvas.drawRoundRect(114, 3, 40, 20, 11, ST7735_RED);
      canvas.fillRoundRect(116, 5, 16, 16, 11, ST7735_RED);
    } else {
      canvas.drawRoundRect(114, 3, 40, 20, 11, ST7735_GREEN);
      canvas.fillRoundRect(136, 5, 16, 16, 11, ST7735_GREEN);
    }
  }

  // draw selected item as icon + label in bold font
  canvas.setFont(&FreeMonoBold9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(10, 44);
  canvas.println(menu_items_settings[item_selected_settings]);
  if (item_selected_settings >= 0 && item_selected_settings < NUM_SETTINGS_ITEMS) {
    if (menu_items_settings_bool[item_selected_settings] == false) {
      canvas.drawRoundRect(114, 30, 40, 20, 11, ST7735_RED);
      canvas.fillRoundRect(116, 32, 16, 16, 11, ST7735_RED);
    } else {
      canvas.drawRoundRect(114, 30, 40, 20, 11, ST7735_GREEN);
      canvas.fillRoundRect(136, 32, 16, 16, 11, ST7735_GREEN);
    }
  }

  // draw previous item as icon + label
  canvas.setFont(&FreeMono9pt7b);
  canvas.setTextSize(1);
  canvas.setTextColor(ST7735_WHITE);
  canvas.setCursor(10, 70);
  canvas.println(menu_items_settings[item_selected_settings_next]);

  if ((item_selected_settings_next) < NUM_SETTINGS_ITEMS) {
    if (menu_items_settings_bool[item_selected_settings_next] == false) {
      canvas.drawRoundRect(114, 56, 40, 20, 11, ST7735_RED);
      canvas.fillRoundRect(116, 58, 16, 16, 11, ST7735_RED);
    } else {
      canvas.drawRoundRect(114, 56, 40, 20, 11, ST7735_GREEN);
      canvas.fillRoundRect(136, 58, 16, 16, 11, ST7735_GREEN);
    }
  }

  if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
    if (previousButtonState == LOW && !buttonPressProcessed) {
      if (item_selected_settings == 0) {
        if (menu_items_settings_bool[0] == true) {
          menu_items_settings_bool[0] = false;
          if (WiFi.status() == WL_CONNECTED) {
            WiFi.disconnect();
          }
        } else {
          menu_items_settings_bool[0] = true;
          if (WiFi.status() != WL_CONNECTED) {
            connectToWiFi();
          }
        }
      } else if (item_selected_settings == 1) {
        if (menu_items_settings_bool[1] == true) {
          menu_items_settings_bool[1] = false;
        } else {
          menu_items_settings_bool[1] = true;
        }
      } else if (item_selected_settings == 2) {
        if (menu_items_settings_bool[2] == true) {
          menu_items_settings_bool[2] = false;
        } else {
          menu_items_settings_bool[2] = true;
        }
      } else if (item_selected_settings == 3) {
        if (menu_items_settings_bool[3] == true) {
          menu_items_settings_bool[3] = false;
          digitalWrite(TFT_BL_PIN, HIGH);
        } else {
          menu_items_settings_bool[3] = true;
          digitalWrite(TFT_BL_PIN, LOW);
        }
      } else if (item_selected_settings == 4) {
        if (menu_items_settings_bool[4] == true) {
          menu_items_settings_bool[4] = false;
        } else {
          menu_items_settings_bool[4] = true;
          ArduinoOTA.begin();
          ArduinoOTA.handle();
        }
      } else if (item_selected_settings == 5) {
        if (menu_items_settings_bool[5] == true) {
          menu_items_settings_bool[5] = false;
        } else {
          menu_items_settings_bool[5] = true;
        }
      } else if (item_selected_settings == 6) {
        if (menu_items_settings_bool[6] == true) {
          menu_items_settings_bool[6] = false;
        } else {
          menu_items_settings_bool[6] = true;
        }
      } else if (item_selected_settings == 7) {
        if (menu_items_settings_bool[7] == true) {
          menu_items_settings_bool[7] = false;
        } else {
          menu_items_settings_bool[7] = true;
        }
      } else if (item_selected_settings == 8) {
        if (menu_items_settings_bool[8] == true) {
          menu_items_settings_bool[8] = false;
        } else {
          menu_items_settings_bool[8] = true;
        }
      } else if (item_selected_settings == 9) {
        if (menu_items_settings_bool[9] == true) {
          menu_items_settings_bool[9] = false;
        } else {
          menu_items_settings_bool[9] = true;
        }
      }
      buttonPressProcessed = true;
      saveToEEPROM();
    }
    previousButtonState = HIGH;
  } else {
    previousButtonState = LOW;
    buttonPressProcessed = false;  // Reinitialiser la variable de controle lorsque le bouton est relache
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
  if (current_screen == 0) {                                                 // MENU SCREEN // up and down buttons only work for the menu screen
    if ((digitalRead(BUTTON_UP_PIN) == HIGH) && (button_up_clicked == 0)) {  // up button clicked - jump to previous menu item
      item_selected = item_selected - 1;                                     // select previous item
      button_up_clicked = 1;                                                 // set button to clicked to only perform the action once
      if (item_selected < 0) {                                               // if first item was selected, jump to last item
        item_selected = NUM_MENU_ITEMS - 1;
      }
    }

    if ((digitalRead(BUTTON_UP_PIN) == LOW) && (button_up_clicked == 1)) {  // unclick
      button_up_clicked = 0;
    }
  }
  if (digitalRead(BUTTON_SELECT_PIN) == HIGH) {
    if (button_select_clicked == 0) {
      select_button_press_time = millis();  // Start measuring button press time
      button_select_clicked = 1;
    } else if (current_screen == 1 && (millis() - select_button_press_time >= SELECT_BUTTON_LONG_PRESS_DURATION)) {
      current_screen = 0;
      delay(350);
    }
  } else if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
    if (button_select_clicked == 1) {
      if (current_screen == 0) {
        current_screen = 1;
      }
      button_select_clicked = 0;  // Reset the button state
    }
  }
  if ((digitalRead(BUTTON_SELECT_PIN) == LOW) && (button_select_clicked == 1)) {  // unclick
    button_select_clicked = 0;
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
void OpenMenuOS::drawCanvasOnTFT() {
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height());
  canvas.fillScreen(ST7735_BLACK);
}
void OpenMenuOS::saveToEEPROM() {
  // Save the contents of the array in EEPROM memory
  for (int i = 0; i < MAX_SETTINGS_ITEMS; i++) {
    EEPROM.write(i, menu_items_settings_bool[i]);
  }

  EEPROM.commit();  // Don't forget to call the commit() function to save the data to EEPROM

  // Save Wi-Fi credentials if not empty
  if (ssid1.length() > 0) {
    prefs.putString("ssid1", ssid1);
  }

  if (password1.length() > 0) {
    prefs.putString("password1", password1);
  }

  if (ssid2.length() > 0) {
    prefs.putString("ssid2", ssid2);
  }

  if (password2.length() > 0) {
    prefs.putString("password2", password2);
  }
  // Commit the changes to preferences
  prefs.end();
}
void OpenMenuOS::readFromEEPROM() {
  // Read the contents of the EEPROM memory and restore it to the array
  for (int i = 0; i < MAX_SETTINGS_ITEMS; i++) {
    menu_items_settings_bool[i] = EEPROM.read(i);
  }
  // Restore Wi-Fi credentials if not empty
  if (prefs.getString("ssid1").length() > 0) {
    ssid1 = prefs.getString("ssid1");
  }

  if (prefs.getString("password1").length() > 0) {
    password1 = prefs.getString("password1");
  }

  if (prefs.getString("ssid2").length() > 0) {
    ssid2 = prefs.getString("ssid2");
  }

  if (prefs.getString("password2").length() > 0) {
    password2 = prefs.getString("password2");
  }
}

int OpenMenuOS::getCurrentScreen() const {
  return current_screen;
}
int OpenMenuOS::getSelectedItem() const {
  return item_selected;
}
