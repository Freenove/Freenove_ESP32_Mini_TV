/*
* @ File:   Sketch_03.1_EEPROM.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-22]
*/

#include "EEPROM.h"

#define EEPROM_SIZE 1
int addr = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  int val = EEPROM.read(addr) + 1;
  delay(5);
  EEPROM.write(addr, val);
  EEPROM.commit();
  delay(1000);
  Serial.print("The data in EEPROM(0) is:");
  Serial.println(EEPROM.read(addr));
}

void loop() {
  // put your main code here, to run repeatedly:
  
}
