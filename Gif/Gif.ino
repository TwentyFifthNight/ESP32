#include "output.h"

#include <TFT_eSPI.h>
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();

int16_t xPos = 0;
int16_t yPos = 0;
int frame = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_WHITE);
  tft.setSwapBytes(true);
  Serial.println("TFT set");

  xPos = tft.width() / 2 - width / 2 - 1;
  yPos = tft.height() / 2 - height / 2 - 1;
}

void loop() {

  if(frame == frames) frame = 0;

  tft.pushImage(xPos, yPos, width, height, hollow[frame]);
  frame++;
  delay(200);
}
