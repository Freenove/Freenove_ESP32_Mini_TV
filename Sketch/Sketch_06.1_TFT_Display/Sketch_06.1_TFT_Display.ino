/*
* @ File:   Sketch_06.1_TFT_Display.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-11]
*/

#include <TFT_eSPI.h>

const int TFT_VCC = 21;

// Initialize TFT instance
TFT_eSPI tft = TFT_eSPI();

uint16_t colors[] = {
  TFT_RED,     // Red
  TFT_GREEN,   // Green
  TFT_BLUE,    // Blue
  TFT_YELLOW,  // Yellow
  TFT_PURPLE   // Purple
};

int colorIndex = 0;

void setup() {
  Serial.begin(115200);

  // Power on and initialize display
  pinMode(TFT_VCC, OUTPUT);
  digitalWrite(TFT_VCC, LOW); // Active Low enable

  tft.begin();
  tft.fillScreen(TFT_BLACK);

  Serial.println("Test Start...");
}

void loop() {
  // Fill screen with the current color
  tft.fillScreen(colors[colorIndex]);

  // Configure text settings
  tft.setTextColor(TFT_BLACK);       // Set text color
  tft.setTextDatum(MC_DATUM);        // Set text datum to middle center
  tft.drawString("TFT Test", 120, 100, 4); // Draw text string

  // Update index
  colorIndex++;
  if (colorIndex >= 5) {
    colorIndex = 0;
  }

  // Delay for 1 second
  delay(1000);
}