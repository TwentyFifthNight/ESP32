#include <Wire.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <TFT_eSPI.h>

#define COLORT1 TFT_BLACK
#define COLORT2 TFT_GREEN
#define COLORT3 TFT_RED
#define COUNTCOLOR TFT_YELLOW


Adafruit_BME280 bme;
float bmpT,bmpP,bmpH;
uint8_t selected;

struct measurmentData {
  const float temperature;
  const float humidity;
  const uint16_t pressure;
};
uint16_t measurmentCount = 0;


TFT_eSPI tft = TFT_eSPI();

void drawBorders();
void drawLabels();
void drawMeasurements(const measurmentData measurements, uint16_t count);
uint16_t countXPosition = 0;


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

  Serial.println();
}

void loop() {
  if (selected) {
    bmpT=bme.readTemperature();
    bmpH=bme.readHumidity();
    bmpP=bme.readPressure()/100.0;

    measurmentCount++;
    drawMeasurements({ temperature: bmpT, humidity: bmpH, pressure: (uint16_t)bmpP}, measurmentCount);
  } 
  else {
    Serial.println("Device not found!");
  }

  delay(2000);
}

void drawBorders(){
  tft.setTextColor(COLORT2);
  //top vertical line
  tft.drawFastVLine(countXPosition, 0, 30, COLORT2);
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


void drawMeasurements(const measurmentData measurements, uint16_t count){
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