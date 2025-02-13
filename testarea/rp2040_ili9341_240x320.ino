/*
  The TFT_eSPI library incorporates an Adafruit_GFX compatible
  button handling class. This sketch is based on the Arduin-o-phone
  example.

  This example displays a keypad where numbers can be entered and
  sent to the Serial Monitor window.

  The sketch has been tested on the ESP8266 (which supports SPIFFS)

  The minimum screen size is 320 x 240 as that is the keypad size.
*/

#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>      // Hardware-specific library
#include "RP2040_PWM.h"    // PWM library

#define _PWM_LOGLEVEL_        1
#define CALIBRATION_FILE "/TouchCalData1"
#define REPEAT_CAL false
#define pinToUse      7

// Keypad parameters
#define KEY_X 45
#define KEY_Y 90
#define KEY_W 66
#define KEY_H 40
#define KEY_SPACING_X 10
#define KEY_SPACING_Y 10
#define KEY_TEXTSIZE 1
#define DISP_X 1
#define DISP_Y 10
#define DISP_W 238
#define DISP_H 50
#define DISP_TSIZE 3
#define DISP_TCOLOR TFT_CYAN
#define NUM_LEN 12
#define STATUS_X 120
#define STATUS_Y 65

TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
RP2040_PWM* PWM_Instance;

float frequency;
float dutyCycle = 90;
float dutyCycle2 = 0;

char numberBuffer[NUM_LEN + 1] = "";
uint8_t numberIndex = 0;

char keyLabel[15][5] = {"New", "Del", "Send", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "0", "#" };
uint16_t keyColor[15] = {TFT_RED, TFT_BLACK, TFT_DARKGREEN, TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE, TFT_BLUE, TFT_BLUE, TFT_BLUE,
                         TFT_BLUE, TFT_BLUE, TFT_BLUE};

TFT_eSPI_Button key[15];  // Invoke the TFT_eSPI button class and create all the button objects

void setup() {
  Serial.begin(9600);  // Use serial port
  tft.init();  // Initialise the TFT screen
  tft.setRotation(0);  // Set the rotation before we calibrate
  touch_calibrate();  // Calibrate the touch screen and retrieve the scaling factors

  // Initialize PWM for backlight
  PWM_Instance = new RP2040_PWM(pinToUse, 20000, 100);

  // Draw initial screen
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, 240, 320, TFT_DARKGREY);  // Draw keypad background
  tft.fillRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_BLACK);  // Draw number display area
  tft.drawRect(DISP_X, DISP_Y, DISP_W, DISP_H, TFT_WHITE);  // Draw number display frame
  drawKeypad();
}

void loop(void) {
  if (dutyCycle2 != dutyCycle) {
    delay(1000);
    dutyCycle2 = dutyCycle;
    PWM_Instance->setPWM(pinToUse, frequency, dutyCycle);
  }

  uint16_t t_x = 0, t_y = 0;  // To store the touch coordinates
  bool pressed = tft.getTouch(&t_x, &t_y);  // Check for valid touch

  for (uint8_t b = 0; b < 15; b++) {
    key[b].press(pressed && key[b].contains(t_x, t_y));  // Update button state
  }

  for (uint8_t b = 0; b < 15; b++) {
    tft.setFreeFont(b < 3 ? LABEL1_FONT : LABEL2_FONT);

    if (key[b].justReleased()) key[b].drawButton();  // Draw normal state
    if (key[b].justPressed()) handleKeyPress(b);  // Handle key press
  }
}

void handleKeyPress(uint8_t b) {
  key[b].drawButton(true);  // Draw inverted state

  if (b >= 3 && numberIndex < NUM_LEN) {
    numberBuffer[numberIndex++] = keyLabel[b][0];
    numberBuffer[numberIndex] = 0;  // Zero terminate
    status("");  // Clear status
  } else if (b == 1 && numberIndex > 0) {
    numberBuffer[--numberIndex] = 0;  // Delete last character
    adjustBacklight(10);  // Increase backlight
    status("");  // Clear status
  } else if (b == 2) {
    status("Sent value to serial port");
    Serial.println(numberBuffer);
  } else if (b == 0) {
    status("Value cleared");
    numberIndex = 0;  // Reset buffer
    numberBuffer[numberIndex] = 0;  // Zero terminate
    adjustBacklight(-10);  // Decrease backlight
  }

  updateDisplay();
}

void updateDisplay() {
  tft.setTextDatum(TL_DATUM);
  tft.setFreeFont(&FreeSans18pt7b);
  tft.setTextColor(DISP_TCOLOR);

  int xwidth = tft.drawString(numberBuffer, DISP_X + 4, DISP_Y + 12);
  tft.fillRect(DISP_X + 4 + xwidth, DISP_Y + 1, DISP_W - xwidth - 5, DISP_H - 2, TFT_BLACK);
}

void adjustBacklight(int change) {
  dutyCycle = constrain(dutyCycle + change, 0, 100);
  dutyCycle2 = dutyCycle;
  PWM_Instance->setPWM(pinToUse, frequency, dutyCycle);
}

void drawKeypad() {
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      uint8_t b = col + row * 3;
      tft.setFreeFont(b < 3 ? LABEL1_FONT : LABEL2_FONT);
      key[b].initButton(&tft, KEY_X + col * (KEY_W + KEY_SPACING_X),
                        KEY_Y + row * (KEY_H + KEY_SPACING_Y),
                        KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
                        keyLabel[b], KEY_TEXTSIZE);
      key[b].drawButton();
    }
  }
}

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

void status(const char *msg) {
  tft.setTextPadding(240);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextFont(0);
  tft.setTextDatum(TC_DATUM);
  tft.setTextSize(1);
  tft.drawString(msg, STATUS_X, STATUS_Y);
}
