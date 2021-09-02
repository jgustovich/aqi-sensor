#include <TFT_eSPI.h>       // Include the graphics library
 
TFT_eSPI tft = TFT_eSPI();  // Create object "tft"
 
void setup() {
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_WHITE);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(0, 0);
}
 
void loop() {
  tft.println("Hello, world!");
  delay(1000);
}
