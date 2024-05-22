#include <TFT_eSPI.h>

#define COLORT1 TFT_BLACK
#define COLORT2 TFT_GREEN
#define COLORT3 TFT_RED
#define COUNTCOLOR TFT_YELLOW

struct measurmentData {
  const float temperature;
  const float humidity;
  const uint16_t pressure;
  const char* timestamp;
};

uint16_t countXPosition = 0;
TFT_eSPI tft = TFT_eSPI();

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
  uint8_t width = tft.width() / 4;
  tft.drawCentreString("BME280", width - 20, 40, 1);
  tft.drawCentreString("RTC", width * 2, 40, 1);
  tft.drawCentreString("SD", width * 3, 40, 1);

  tft.setTextColor(COLORT3);
  tft.drawCentreString("Laboratorium 6", tft.width() / 2, tft.height() - 40, 2);
}

void drawMeasurements(const measurmentData measurements, uint16_t count){
  String data;

  tft.setTextSize(2);
  tft.setTextColor(COUNTCOLOR, COLORT3);
  tft.drawNumber(count, countXPosition + 15, 10, 1);

  tft.setTextColor(COLORT2, COLORT1);

  tft.drawCentreString(measurements.timestamp, tft.width() / 3, 5, 1);
  
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
