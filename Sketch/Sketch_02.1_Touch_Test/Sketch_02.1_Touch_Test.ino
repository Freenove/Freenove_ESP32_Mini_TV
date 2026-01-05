/*
* @ File:   Sketch_02.1_Touch_Test.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-11]
*/

const int TOUCH_PIN = 32; 

void setup() {
  Serial.begin(115200);
  delay(1000);  // give me time to bring up serial monitor
  Serial.println("ESP32 Touch Test");
  touchSetCycles(0xf000, 0x1000);
}

void loop() {
  Serial.println(touchRead(TOUCH_PIN));  // get value using T0
  delay(100);
}