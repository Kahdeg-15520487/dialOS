#include <Arduino.h>
#include <M5Dial.h>

// Global variables for encoder interaction
int encoderValue = 0;
String displayText = "Hello dialOS!";

// put function declarations here:
int myFunction(int, int);

void setup() {
  // Initialize M5Dial with all peripherals
  M5Dial.begin(true, true);
  
  // Set up display
  M5Dial.Display.setTextSize(2);
  M5Dial.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Dial.Display.setCursor(20, 100);
  M5Dial.Display.println(displayText);
  
  // Initialize encoder value
  encoderValue = M5Dial.Encoder.read();
}

void loop() {
  // Update M5Dial state
  M5Dial.update();
  
  // Handle encoder input
  int newEncoderValue = M5Dial.Encoder.read();
  if (newEncoderValue != encoderValue) {
    encoderValue = newEncoderValue;
    
    // Clear display and update
    M5Dial.Display.fillScreen(TFT_BLACK);
    M5Dial.Display.setCursor(20, 80);
    M5Dial.Display.printf("Hello dialOS!\nEncoder: %d", encoderValue);
  }
  
  // Small delay to prevent overwhelming the display
  delay(50);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}