#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#include "hollow_cp.h"

//TFT_eSPI
#define COLORT1 TFT_BLACK
#define COLORT2 TFT_GREEN
#define COLORT3 TFT_RED
#define COUNTCOLOR TFT_YELLOW

TFT_eSPI tft = TFT_eSPI();

//LED and Buttons
#define LED_BUTTON_PIN 33
#define LED 32
#define TOGGLE_BUTTON 27

bool lastLEDButtonState = false;
bool ledState = false;
bool lastToogleButtonState = false;

//BME280
Adafruit_BME280 bme;
float bmpT,bmpP,bmpH;
uint8_t selected;

struct measurmentData {
  const float temperature;
  const float humidity;
  const uint16_t pressure;
};
uint16_t measurmentCount = 0;
unsigned long lastMeasurmentTime = 0;
uint16_t countXPosition = 0;

//functions
void drawBorders();
void drawLabels();
void clearContent();
void drawBME280Data(const measurmentData measurements, const uint16_t count);
void initDisplay(short display);
void checkToggleButton();
void updateDisplay(short display);

//gif display
int16_t xPos = 0;
int16_t yPos = 0;
int frame = 0;
unsigned long lastGifTime = 0;

//changing displays
short display = 0;
short display_count = 2;

void setup() {
  Serial.begin(115200);
  delay(1000);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(COLORT1);
  tft.setTextSize(1);
  Serial.println();
  countXPosition = tft.width() - tft.width() / 3;

  drawBorders();
  drawLabels();

  selected = bme.begin(0x76);

  if (!selected) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
  }
  else {
    Serial.println("BME280 sensor OK");
  }

  pinMode(LED_BUTTON_PIN, INPUT_PULLUP);
  pinMode(TOGGLE_BUTTON, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  digitalWrite(LED, ledState);


  xPos = tft.width() / 2 - width / 2 - 1;
  yPos = tft.height() / 2 - height / 2 - 1;

  Serial.println();
}

void loop() {
  const bool state = !digitalRead(LED_BUTTON_PIN);
  const unsigned currentTime = millis();

  if(state != lastLEDButtonState && state == 1){
    Serial.println("pressed led button");
    ledState = !ledState;
  }
  lastLEDButtonState = state;
  digitalWrite(LED, ledState);

  checkToggleButton();

  updateDisplay(display, currentTime);

  delay(50);
}


void drawBorders(){
  tft.setTextColor(COLORT2);
  uint16_t x_pos = countXPosition;
  //top vertical line
  tft.drawFastVLine(x_pos, 0, 30, COLORT2);
  //top horizontal line
  tft.drawFastHLine(0, 30, tft.width(), COLORT2);
  //2nd horizontal line
  tft.drawFastHLine(0, 65, tft.width(), COLORT2);
  //bottom horizontal line
  tft.drawFastHLine(0, tft.height() - 45, tft.width(), COLORT2);

  tft.fillRect(countXPosition + 1, 0, 100, 29, COLORT3);
}


void drawLabels(){
  tft.setTextColor(COLORT2);

  tft.setTextSize(2);
  tft.drawCentreString("Sensor BME280", tft.width() / 2, 40, 1);

  tft.setTextColor(COLORT3);
  tft.drawCentreString("Laboratorium 3", tft.width() / 2, tft.height() - 40, 2);
}


void drawBME280Data(const measurmentData measurements, const uint16_t count){
  String data;

  tft.setTextSize(2);
  tft.setTextColor(COUNTCOLOR, COLORT3);
  tft.drawNumber(count, countXPosition + 15, 10, 1);

  tft.setTextColor(COLORT2, COLORT1);

  uint16_t y_pos = 75;
  data = measurements.temperature;
  data += " C";
  tft.drawCentreString(data, tft.width() / 2, y_pos, 2);
  
  y_pos += 50;
  data = measurements.humidity;
  data += " %";
  tft.drawCentreString(data, tft.width() / 2, y_pos, 2);
  
  y_pos += 50;
  data = measurements.pressure;
  data += " hPa";
  tft.drawCentreString(data, tft.width() / 2, y_pos, 2);
}


void initDisplay(const short display){
  switch(display){
    case 0:
      tft.fillScreen(COLORT1);
      drawBorders();
      drawLabels();
      break;
    case 1:
      tft.fillScreen(TFT_WHITE);
      break;
  }
}


void checkToggleButton(){
  const bool state = !digitalRead(TOGGLE_BUTTON);

  if(state != lastToogleButtonState && state == 1){
    Serial.println("pressed toogle");

    display++;
    if(display >= (display_count))  display = 0;

    initDisplay(display);
  }
  lastToogleButtonState = state;
}


void updateDisplay(const short display, const unsigned long currentTime){
  switch(display){
    case 0:
      if(currentTime - lastMeasurmentTime > 2000){
        lastMeasurmentTime = currentTime;
        if (selected) {
          bmpT=bme.readTemperature();
          bmpH=bme.readHumidity();
          bmpP=bme.readPressure()/100.0;

          measurmentCount++;
          drawBME280Data({ temperature: bmpT, humidity: bmpH, pressure: (uint16_t)bmpP}, measurmentCount);
        } 
        else {
          Serial.println("Device not found!");
        }
      }
      break;
    case 1:
      if(currentTime - lastGifTime >= 200){
        lastGifTime = currentTime;
        if(frame == frames) frame = 0;

        tft.pushImage(xPos, yPos, width, height, hollow[frame]);
        frame++;
      }
      break;
  }
}