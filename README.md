# 3D Printer Chamber FAN Control with RP2040, ILI9341 TFT and Touch UI with PID PWM FAN CONTROL

## Version: 1.0
**Author**: Solutionphil  
**Date**: February 19th, 2025

## Description
This project leverages the RP2040 microcontroller to interface with an ILI9341 TFT display, creating a touch-based UI with multiple menu screens, PWM-based FAN and brightness control, PID Control, Air Quality measurement, Temperature and Humidity measurement and a file explorer using LittleFS. Key features include touch calibration, slider-based brightness and fan adjustment, and file management.

## Features
- **Touch-based UI** with multiple menu screens
- **PWM-based brightness control**
- **File explorer** using LittleFS
- **Touch calibration**
- **Slider-based brightness adjustment**
- **File management**
- **Manual slider based FAN control**
- **PID FAN control**
- **Temperature and VOC measurement**
- **Configurable display dimming and turning off**
- **Autosave and load for changed values**
- **System Info Screen**

## Hardware Requirements
- **RP2040 ZERO microcontroller** (`Arduino IDE Boardmanager: Raspberry Pi Pico/RP2040/RP2350 by Earle F. Philhower, III`)
- **ILI9341 TFT display** 2.8" 240x320 with touch screen (`Red Board`)
- **SGP40 Sensor** for VOC Index
- **BME280 Sensor** for temperature and humidity measurement
- **Wires**
- **USB-C cable** for power supply and flashing RP2040
- **3x 12V PWM PC Fans**

## Software Requirements
- **Arduino IDE 2.3.4+**
  
### Libraries for Arduino IDE
- **TFT_eSPI**: For TFT display control (by Bodmer)
- **TFT_eWidget**: For Slider Control (by Bodmer)
- **RP2040_PWM**: For PWM-based brightness control (by Khoi Hoang)
- **Adafruit_NeoPixel** For Neopixel control (by Adafruit)
- **Adafruit_BME280** For Temperature and Humidity measurement (Adafruit)
- **Adafruit_SGP40** For VOC Air Quality Measurement (Adafruit)
- **QuickPID** For PID FAN Control (by David Lloyd)
- **Wire** For I2C/Serial communication

# Instructions

### Hardware Setup
1. **Wire your RP2040 Zero** with the ILI9341 240x320 display as follows:

    - **ILI9341 TFT Display Pins:**
        - PIN 7 (LED): Backlight Brightness Control 
        - Pin 6 (RST): Reset
        - Pin 5 (TFT_CS and CSn): Chip Select
        - Pin 4 (DC): Data/Command
        - Pin 3 (T_DIN and MOSI): Master Out Slave In
        - Pin 2 (T_CLK and SCLK): Serial Clock
        - Pin 1 (CS): Chip Select
        - Pin 0 (T_D0 and MISO): Master In Slave Out

    - **PWM for Fan Control:**
        - FAN1_PIN 27
        - FAN2_PIN 28
        - FAN3_PIN 29

    - **NeoPixel:**
        - ONBOARD RP2040 NEOPIXEL LED on PIN16

    - **I2C Pins:**
        - I2C0 SDA 8
        - I2C0 SCL 9
        - I2C1 SDA 10 (not used yet)
        - I2C1 SCL 11 (not used yet)

    - **UART Pins:**
        - UART TX 12 (not used yet)
        - UART RX 13 (not used yet)

    - **Environmental Sensors (Connected to I2C0):**
        - Adafruit BME280
        - Adafruit SGP40

## Diagram
For a visual representation of the wiring, refer to the [wiring diagram](https://www.mermaidchart.com/raw/80084e9d-4179-43a7-a02a-414e01cb094a?theme=light&version=v0.1&format=svg).

### Software Setup
1. **Install the required libraries** in the Arduino IDE 2.3.x.
2. **Install the RP2040 board**(Raspberry Pi Pico/RP2040/RP2350 by Earle F. Philhower, III) in the Arduino IDE 2.3.x Boardmanager.
3. **Copy the files** from the TFT_eSPI folder to your Arduino IDE working folder into `/libraries/TFT_eSPI`.
4. **Download and open** the *.INO file in your Arduino IDE.
5. **Compile and upload** the code. `For the first upload, press reset+boot and release the reset button`. Your RP2040 Zero should be detected as a new drive in your operating system.
6. **Enjoy!** :)






## Disclaimer

Working with electricity and electronic components can be dangerous. Always ensure you take the necessary safety precautions when handling electrical devices.

This software and associated documentation are provided "as is" without warranty of any kind, either express or implied, including but not limited to the implied warranties of merchantability and fitness for a particular purpose. In no event shall the authors or copyright holders be liable for any claim, damages, or other liability, whether in an action of contract, tort, or otherwise, arising from, out of, or in connection with the software or the use or other dealings in the software.

Use this software at your own risk. The authors are not responsible for any damage to your equipment, personal injury, or any other consequences resulting from the use of this software.
