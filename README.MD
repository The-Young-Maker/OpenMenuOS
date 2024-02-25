# OpenMenuOS Documentation

## Introduction

OpenMenuOS is a versatile open-source menu system for embedded systems, designed to simplify the implementation of user interfaces in various applications. This documentation provides an overview of the OpenMenuOS library, including its features, usage, and customization options.

## Features

- Intuitive menu navigation system
- Support for multiple menu levels and submenus
- Flexible configuration options for button and display pins
- Customizable menu item names and icons
- Additional support for images and icons beyond menu items
- Integration with serial communication and WiFi functionality

## Getting Started

### Installation

Download the OpenMenuOS library and install it in your Arduino IDE.

### Setup

Include the `OpenMenuOS.h` header file in your sketch to access the OpenMenuOS functionality.

### Initialization

To begin using OpenMenuOS, create an instance of the OpenMenuOS class. Specify the button pins (UP, SELECT), TFT backlight pin, and display control pins (CS, DC, RST). Additionally, provide the names of menu items separated by commas, with a maximum of 20 items. Remember to end the list with "NULL".

Example:
```
OpenMenuOS(
  Button UP Pin,
  Button SELECT Pin,
  TFT Backlight Pin,
  Display CS Pin,
  Display DC Pin,
  Display RST Pin,
  Name of the item separated by a comma (You can go up to 20),
  NULL
)
```

Example Use: 'OpenMenuOS menu(10, 5, 12, 4, 21, 22, "Flashlight", "Profile", "Heat", "Reflow", "Settings", "Wifi", "Package", "Survival", "Other", NULL);'

### Usage

Implement the `setup()` and `loop()` functions in your sketch to initialize the menu system and handle user interactions. Use the provided methods to navigate the menu, display content on the screen, and respond to user inputs.

## Customization

### Menu Icons

OpenMenuOS supports menu icons to enhance the visual presentation of menu items. Icons should be provided as 16x16 pixel bitmaps and stored in an `images.h` file. Define each icon as a constant array of bytes.

### Additional Images

In addition to menu icons, OpenMenuOS allows for the integration of custom images or icons beyond menu items. These images can be used for boot screens, splash screens, or other graphical elements. Store additional images in the `images.h` file as constant arrays of bytes.
