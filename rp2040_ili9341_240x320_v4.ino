/*
 * Program Name: RP2040 TFT Touch UI with PWM Brightness Control
 * Version: 1.0
 * Author: [Your Name]
 * Date: [Today's Date]
 *
 * Description:
 * This program is designed for the RP2040 microcontroller to interface with an ILI9341 TFT display.
 * It provides a touch-based UI with multiple screens, PWM-based brightness control, and a file explorer
 * using SPIFFS. The program includes features like touch calibration, slider-based brightness adjustment,
 * and file management.
 *
 * Hardware:
 * - RP2040 microcontroller
 * - ILI9341 TFT display with touch screen
 * - PWM pin for brightness control
 *
 * Libraries:
 * - TFT_eSPI: For TFT display control
 * - RP2040_PWM: For PWM-based brightness control
 */

#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>      // Hardware-specific library
#include <TFT_eWidget.h>  // Widget library for sliders
#include "RP2040_PWM.h"   // PWM library

// Initialize TFT and slider objects
TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
TFT_eSprite knob = TFT_eSprite(&tft); // Create TFT sprite for slider knob
SliderWidget slider = SliderWidget(&tft, &knob);

#define _PWM_LOGLEVEL_        1
#define CALIBRATION_FILE "/TouchCalData1"
#define REPEAT_CAL false
#define pinToUse      7

// Define constants for screen dimensions and colors
#define DISP_X 1
#define DISP_Y 10
#define DISP_W 238
#define DISP_H 50
#define DISP_TCOLOR TFT_CYAN
#define NUM_LEN 12
#define STATUS_X 120

// Fonts for key labels
#define LABEL1_FONT &FreeSansOblique12pt7b // Key label font 1 
#define LABEL2_FONT &FreeSansBold9pt7b    // Key label font 2

RP2040_PWM* PWM_Instance;

// Initialize PWM instance for brightness control
float frequency = 1831;
float dutyCycle = 80;

TFT_eSPI_Button screenButton;  // Button to switch screens
TFT_eSPI_Button increaseButton;  // Button to switch screens
TFT_eSPI_Button decreaseButton;  // Button to switch screens
TFT_eSPI_Button fileButtons[10]; // Buttons for file explorer
String fileNames[10]; // Array to store file names
String buttonLabels[10];
TFT_eSPI_Button backButton;
TFT_eSPI_Button yesButton;
TFT_eSPI_Button noButton;
TFT_eSPI_Button mainMenuButtons[4]; // Buttons for main menu

int currentScreen = 0;
const int totalScreens = 5;

// Labels for other buttons
char backButtonLabel[] = "Back"; // Define the button label as a mutable char array
char incButtonLabel[] = "Increase"; // Define the button label as a mutable char array
char decButtonLabel[] = "Decrease"; // Define the button label as a mutable char array
char yesLabel[] = "YES"; // Define the button label as a mutable char array
char noLabel[] = "NO"; // Define the button label as a mutable char array
char backLabel[] = "Back"; // Define the button label as a mutable char array

void setup() {
  // Initialize serial communication for debugging
  Serial.begin(9600);  

  // Set up PWM for brightness control
  PWM_Instance = new RP2040_PWM(pinToUse, frequency, dutyCycle);

  // Initialize the knob sprite early
  knob.setColorDepth(8);
  knob.createSprite(30, 40);  // Size for the slider knob
  knob.fillSprite(TFT_BLACK);

  delay(1000);
  PWM_Instance->setPWM(pinToUse, frequency, dutyCycle);

  // Initialize the TFT display and set its rotation
  tft.init();  
  tft.setRotation(0);  

  // Calibrate the touch screen and display the initial screen
  touch_calibrate();  
  displayScreen(currentScreen);  
}

void loop(void) {
  // Main loop to handle touch input and update screens
  uint16_t t_x = 0, t_y = 0;  // Variables to store touch coordinates
  bool pressed = tft.getTouch(&t_x, &t_y);  // Boolean indicating if the screen is being touched

  if (currentScreen == 0) {  // Main menu screen
    for (uint8_t b = 0; b < 4; b++) {
      mainMenuButtons[b].press(pressed && mainMenuButtons[b].contains(t_x, t_y));  // Update button state
    }

    for (uint8_t b = 0; b < 4; b++) {
      if (mainMenuButtons[b].justReleased()) mainMenuButtons[b].drawButton();  // Draw normal state
      if (mainMenuButtons[b].justPressed()) {
        currentScreen = b + 1;  // Navigate to the selected screen
        displayScreen(currentScreen);  // Display the selected screen
        delay(500);  // Debounce delay
      }
    }
  } else if (currentScreen == 2) {
    if (pressed) {
      if (slider.checkTouch(t_x, t_y)) {
        // Handle slider touch logic
        dutyCycle = round(slider.getSliderPosition() / 10) * 10; // Snap to nearest 10% increment
        slider.setSliderPosition(dutyCycle); // Update slider position to snapped value
        PWM_Instance->setPWM(pinToUse, frequency, dutyCycle);
        // Update percentage display
        tft.fillRect(90, 110, 80, 30, TFT_BLACK);
        tft.setTextColor(TFT_GREEN);
        tft.drawString(String(int(dutyCycle)) + "%", 100, 120);
      }
    }
  }

  // Check for touch on the screen switch button
  if(currentScreen!=0){
  screenButton.press(pressed && screenButton.contains(t_x, t_y));
  if (screenButton.justReleased()) screenButton.drawButton();

  // Switch to the next screen when the button is pressed
  if (screenButton.justPressed()) {
    currentScreen = 0;
    displayScreen(currentScreen);
    delay(500);  // Debounce delay
  }
  }

  // Check for touch on brightness control buttons only on screen 3
  if (currentScreen == 3) {
    increaseButton.press(pressed && increaseButton.contains(t_x, t_y));
    decreaseButton.press(pressed && decreaseButton.contains(t_x, t_y));

    // Handle brightness adjustment buttons
    if (increaseButton.justReleased()) increaseButton.drawButton();
    if (increaseButton.justPressed()) adjustBacklight(10);  // Increase brightness

    if (decreaseButton.justReleased()) decreaseButton.drawButton();
    if (decreaseButton.justPressed()) adjustBacklight(-10);  // Decrease brightness
  }

  // Check for touch on file explorer buttons only on screen 4
  if (currentScreen == 4) {
    for (uint8_t i = 0; i < 10; i++) {
      fileButtons[i].press(pressed && fileButtons[i].contains(t_x, t_y));

      // Handle file button interactions
      if (fileButtons[i].justReleased()) fileButtons[i].drawButton();
      if (fileButtons[i].justPressed()) handleFileButtonPress(i);
    }
  }
}

void displayScreen(int screen) {  // Update screen display logic
  tft.fillScreen(TFT_BLACK);  // Clear the screen

  switch (screen) {
    case 0:
      drawMainMenu();
      break;
    case 1:
      displayScreen1();  // Display screen 1
      break;
    case 2:
      displayScreen2();
      break;
    case 3:
      displayScreen3();
      break;
    case 4:
      displayScreen4();  // Display file explorer
      break;
  }

  // Draw the screen switch button
  if(screen!=0){
  tft.setFreeFont(LABEL2_FONT);
  screenButton.initButton(&tft, 200, 20, 60, 30, TFT_WHITE, TFT_BLUE, TFT_WHITE, backButtonLabel, 1);
  screenButton.drawButton();
  //backButton.initButton(&tft, 200, 280, 60, 30, TFT_WHITE, TFT_RED, TFT_WHITE, backLabel, 1); // Back button
  }
}

void displayScreen1() {
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(LABEL2_FONT);
  tft.setTextSize(1);
  tft.setCursor(10, 20);
  tft.print("Screen 1");
}

void displayScreen2() {
  // Clear screen and set up title
  tft.fillScreen(TFT_BLACK);
  
  // Ensure knob sprite is ready
  if (!knob.created()) knob.createSprite(30, 40);
  
  tft.setTextColor(TFT_CYAN);
  tft.setFreeFont(LABEL2_FONT);
  tft.setTextSize(0);
  tft.drawString("Brightness", 30, 50);

  // Update percentage display
  tft.fillRect(90, 110, 80, 30, TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.drawString(String(int(dutyCycle)) + "%", 100, 120);

  // Create slider parameters
  slider_t param;
  
  // Slider slot parameters
  param.slotWidth = 10;
  param.slotLength = 200;
  param.slotColor = TFT_BLUE;
  param.slotBgColor = TFT_YELLOW;
  param.orientation = H_SLIDER;
  
  // Slider knob parameters
  param.knobWidth = 20;
  param.knobHeight = 30;
  param.knobRadius = 5;
  param.knobColor = TFT_WHITE;
  param.knobLineColor = TFT_RED;
   
  // Slider range and movement
  param.sliderLT = 10;
  param.sliderRB = 100;
  param.startPosition = int16_t(50);
  param.sliderDelay = 0;
  // Draw the slider
  slider.drawSlider(20, 160, param);

  int16_t x, y;    // x and y can be negative
  uint16_t w, h;   // Width and height
  slider.getBoundingRect(&x, &y, &w, &h);     // Update x,y,w,h with bounding box
  tft.drawRect(x, y, w, h, TFT_DARKGREY); // Draw rectangle outline
  slider.setSliderPosition(dutyCycle);
}

void displayScreen3() {
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(LABEL2_FONT);
  tft.setTextSize(1);
  tft.setCursor(10, 70);
  tft.print("Control Brightness");
  
  // Add brightness control buttons 
  tft.setFreeFont(LABEL2_FONT); 

  increaseButton.initButton(&tft, 120, 150, 190, 80, TFT_WHITE, TFT_GREEN, TFT_WHITE, incButtonLabel, 1); 
  decreaseButton.initButton(&tft, 120, 250, 190, 80, TFT_WHITE, TFT_RED, TFT_WHITE, decButtonLabel, 1); 
  increaseButton.drawButton(); 
  decreaseButton.drawButton(); 
}

void displayScreen4() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(LABEL2_FONT);
  tft.setTextSize(1);
  tft.setCursor(10, 20);
  tft.print("File Explorer");
  tft.setFreeFont(LABEL2_FONT);
  screenButton.initButton(&tft, 200, 20, 60, 30, TFT_WHITE, TFT_BLUE, TFT_WHITE, backButtonLabel, 1);
  screenButton.drawButton();

  // List files in SPIFFS
  File root = SPIFFS.open("/", "r");
  File file = root.openNextFile();
  uint8_t i = 0;
  while (file && i < 10) {
    String fileName = file.name();
    fileNames[i] = fileName; // Store file name in array
    buttonLabels[i] = fileName; // Store label in array
    fileButtons[i].initButton(&tft, 120, 80 + i * 30, 200, 30, TFT_WHITE, TFT_BLUE, TFT_WHITE, (char*)fileName.c_str(), 1);
    fileButtons[i].drawButton();
    file = root.openNextFile();
    i++;
  }
}

bool displayDeletionPrompt(String fileName) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(LABEL2_FONT);
  tft.setTextSize(1);
  tft.setCursor(10, 20);
  tft.print("Delete file: ");
  tft.setCursor(10, 55);
  tft.print(fileName);
  tft.setCursor(10, 90);
  tft.print("Are you sure?");

  yesButton.initButton(&tft, 60, 160, 80, 40, TFT_WHITE, TFT_GREEN, TFT_WHITE, yesLabel, 1);
  noButton.initButton(&tft, 180, 160, 80, 40, TFT_WHITE, TFT_RED, TFT_WHITE, noLabel, 1);

  yesButton.drawButton();
  noButton.drawButton();

  while (true) {
    uint16_t t_x = 0, t_y = 0;
    bool pressed = tft.getTouch(&t_x, &t_y);

    yesButton.press(pressed && yesButton.contains(t_x, t_y));
    noButton.press(pressed && noButton.contains(t_x, t_y));

    if (yesButton.justReleased()) {
      return true;
    }
    if (noButton.justReleased()) {
      return false;
    }
  }
}

void handleFileButtonPress(uint8_t index) {
  String fileName = buttonLabels[index];
  if (displayDeletionPrompt(fileName)) {
    if (SPIFFS.exists(fileName)) {
      SPIFFS.remove(fileName);
    }
  } else {
    displayFileContents(fileName);  // Show file contents if deletion is canceled
  }
  displayScreen4();  // Refresh the file explorer screen
}

void displayFileContents(String fileName) {
  tft.fillScreen(TFT_BLACK);
  t ft.setTextColor(TFT_WHITE);
  tft.setFreeFont(LABEL2_FONT);
  tft.setTextSize(1);
  tft.setCursor(10, 20);
  tft.print("File: ");
  tft.print(fileName);

  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    tft.setCursor(10, 50);
    tft.print("Failed to open file");
    return;
  }

  tft.setCursor(10, 50);
  while (file.available()) {
    tft.print((char)file.read());
  }
  file.close();

  // Add a button to go back to the file explorer screen
  backButton.initButton(&tft, 120, 220, 80, 40, TFT_WHITE, TFT_BLUE, TFT_WHITE, backLabel, 1);
  backButton.drawButton();

  while (true) {
    uint16_t t_x = 0, t_y = 0;
    bool pressed = tft.getTouch(&t_x, &t_y);

    backButton.press(pressed && backButton.contains(t_x, t_y));

    if (backButton.justReleased()) {
      displayScreen4();
      return;
    }
  }
}

// Function to adjust backlight brightness
void adjustBacklight(int change) {
  dutyCycle = constrain(dutyCycle + change, 0, 100);
  PWM_Instance->setPWM(pinToUse, frequency, dutyCycle);
}

void drawMainMenu() {
  // Function to display the main menu
  // Draws buttons for navigating to different screens
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setFreeFont(LABEL2_FONT);
  tft.setTextSize(1);
  tft.setCursor(10, 50);
  tft.print("Main Menu");

  mainMenuButtons[0].initButton(&tft, 120, 100, 200, 40, TFT_WHITE, TFT_BLUE, TFT_WHITE, (char*)"Screen 1", 1);
  mainMenuButtons[1].initButton(&tft, 120, 150, 200, 40, TFT_WHITE, TFT_BLUE, TFT_WHITE, (char*)"Brightness", 1);
  mainMenuButtons[2].initButton(&tft, 120, 200, 200, 40, TFT_WHITE, TFT_BLUE, TFT_WHITE, (char*)"Control Brightness", 1);
  mainMenuButtons[3].initButton(&tft, 120, 250, 200, 40, TFT_WHITE, TFT_BLUE, TFT_WHITE, (char*)"File Explorer", 1);

  for (uint8_t i = 0; i < 4; i++) {
    mainMenuButtons[i].drawButton();
  }
}

// Function to handle touch calibration
void touch_calibrate() {
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  if (!SPIFFS.begin()) {
    Serial.println("formatting file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL) {
      SPIFFS.remove(CALIBRATION_FILE);
    } else {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14) calDataOK = 1;
        f.close();
      }
    }
  }

  if (calDataOK && !REPEAT_CAL) {
    tft.setTouch(calData);
  } else {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 0);
    tft.setTextFont(2);
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.println("Touch corners as indicated");
    tft.setTextFont(1);
    tft.println();
    if (REPEAT_CAL) {
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.println("Set REPEAT_CAL to false to stop this running again!");
    }
    tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.println("Calibration complete!");
    File f = SPIFFS.open(CALIBRATION_FILE, "w");
    if (f) {
      f.write((const unsigned char *)calData, 14);
      f.close();
    }
  }
}
