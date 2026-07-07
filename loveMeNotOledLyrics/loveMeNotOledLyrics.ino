/*
 * Project: Love me not OLED Lyrics Animation
 * Author: @smartengineers32
 * 
 * Hardware Requirements:
 * - Microcontroller: ESP8266 (NodeMCU/Wemos) or ESP32.
 *   [WARNING: DO NOT USE ARDUINO UNO OR NANO! They only have 32KB of memory. 
 *    This animation is ~102KB and will fail to compile on Uno/Nano.]
 * - 0.96 inch I2C OLED Display (SSD1306, 128x64)
 *
 * Required Libraries:
 * - Adafruit GFX Library
 * - Adafruit SSD1306
 *
 * Wiring (I2C):
 * - ESP8266 (NodeMCU/Wemos): SDA -> D2 (GPIO4), SCL -> D1 (GPIO5)
 * - ESP32: SDA -> 21, SCL -> 22
 */

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

// Include the animation frames data
#include "animation_frames.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Create OLED object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

const int TOTAL_FRAMES = 102;
const int ANIMATION_DELAY_MS = 64;

void setup() {

  Wire.begin();  // IMPORTANT FOR ESP-01 OR UNO

  // Start the display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;)
      ;  // Stop if OLED not found
  }

  display.clearDisplay();
  display.display();
}


void loop() {
  playAnim(epd_bitmap_allArray, TOTAL_FRAMES, ANIMATION_DELAY_MS);
}

// ---------------- PLAY ANIMATION FUNCTION -----------------

void playAnim(const unsigned char* frames[], int totalFrames, int delayTime) {

  for (int i = 0; i < totalFrames; i++) {

    display.clearDisplay();

    // EASY: no pgm_read_ptr() needed
    const unsigned char* framePtr = frames[i];

    display.drawBitmap(0, 0, framePtr, 128, 64, 1);
    display.display();

    delay(delayTime);
  }
}