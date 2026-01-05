/*
* @ File:   Sketch_02.2_Touch_Button.ino
* @ Author: [Vegetable-SYC]
* @ Date:   [2025-12-19]
*/

const int TOUCH_PIN = 32; 
const int TOUCH_THRESHOLD = 80; // Touch sensitivity threshold
const int PRESS_Time = 3000;    // Threshold duration for long press (ms)
uint32_t Touch_Time = 0;        // Timestamp of the initial touch
int Touch_data = 0, Touch_old_data = 0; 
int short_press_num = 0;
int long_press_num = 0;
bool LONG_PRESS_FLAG = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);  // Wait for serial monitor to initialize
  
  /* Configure touch sensor cycles */
  touchSetCycles(0xf000, 0x1000);
  
  /* Initial calibration and baseline data reading */
  Touch_old_data = touchRead(TOUCH_PIN);
  Serial.println(Touch_old_data);
}

void loop() {
  // Read touch sensor data
  Touch_data = touchRead(TOUCH_PIN);
  
  // Track and update the maximum baseline value
  if(Touch_old_data < Touch_data)
    Touch_old_data = Touch_data;

  // Press Detection Logic
  if((Touch_old_data - Touch_data) > TOUCH_THRESHOLD)
  {
    // Start timer on initial touch detection
    if(Touch_Time == 0)
      Touch_Time = millis();
    
    // Evaluate if the press duration meets Long Press requirement
    if(millis() - Touch_Time > PRESS_Time)
    {
      Touch_Time = 0;
      LONG_PRESS_FLAG = 1;
      
      Serial.print("Long Press Detected: ");
      long_press_num++;
      Serial.println(long_press_num);
    }
  }
  else {
    // Release Detection Logic
    if(Touch_Time != 0)
    {
      Touch_Time = 0;
      
      if(LONG_PRESS_FLAG != 0)
      {
        // Event: Long press released
        LONG_PRESS_FLAG = 0;
      }
      else 
      {
        // Event: Short press released
        Serial.print("Short Press Detected: ");
        short_press_num++;
        Serial.println(short_press_num);
      }
    }
  }
}