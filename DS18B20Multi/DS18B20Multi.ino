#include <DS18B20.h> //https://github.com/matmunk/DS18B20
#include <SPI.h>
#include <TFT_eSPI.h>

#define COLORT1 TFT_BLACK
#define COLORT2 TFT_GREEN
#define COLORT3 TFT_RED
#define COUNTCOLOR TFT_YELLOW

DS18B20 ds(4); // pin GPIO4
TFT_eSPI tft = TFT_eSPI();

uint8_t countXPosition = 0;

uint8_t address[8];
uint16_t measurmentCount = 0;
uint16_t y_pos = 75;
uint8_t sensor_index = 0;

void drawMeasurement();
void drawBorders();
void drawLabels();

void setup() {
  Serial.begin(115200);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(COLORT1);
  tft.setTextSize(1);
  Serial.println();
  countXPosition = tft.width() - tft.width() / 3;

  drawBorders();
  drawLabels();

  Serial.println();
}

void loop() {
  measurmentCount++;
  tft.setTextColor(COUNTCOLOR, COLORT3);
  tft.setCursor(countXPosition + 5, 10, 1);
  tft.print(measurmentCount);
  
  y_pos = 75;
  sensor_index = 0;
  while (ds.selectNext()) {
    sensor_index++;
    ds.getAddress(address);
    drawMeasurement(ds.getTempC(), address, ds.getResolution(), y_pos, sensor_index);
    y_pos += 60;
  }
  delay(10000);
}

void drawMeasurement(double temp, uint8_t address[8], uint8_t resolution, uint16_t y_pos, uint8_t sensor_index){
  tft.setTextSize(2);
  tft.setTextColor(COLORT2, COLORT1);
  
  tft.setCursor(0, y_pos, 1);
  tft.print("T(");
  tft.print(sensor_index);
  tft.print(")=");
  tft.print(temp);
  tft.print(" C");
  y_pos += 25;

  tft.setCursor(0, y_pos, 1);
  for (byte i = 0; i < 8; i++) {
    tft.print(address[i], HEX);
  }
  
  tft.print(" R:");
  tft.print(resolution);
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
  tft.drawCentreString("Sensor DS18B20", tft.width() / 2, 40, 1);

  tft.setTextColor(COLORT3);
  tft.drawCentreString("Laboratorium 4b", tft.width() / 2, tft.height() - 40, 2);
}
