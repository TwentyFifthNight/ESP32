// void setup(void) {
  
//   const MeasurmentData measurment = { temperature:"25.55", humidity:"56.76", pressure:"1013"};
//   drawMeasurements(measurment);
// }



#include <DS18B20.h> //https://github.com/matmunk/DS18B20
#include <SPI.h>
#include <TFT_eSPI.h>

#define COLORT1 TFT_BLACK
#define COLORT2 TFT_GREEN
#define COLORT3 TFT_RED
#define COUNTCOLOR TFT_YELLOW

DS18B20 ds(4); // pin GPIO4
TFT_eSPI tft = TFT_eSPI();

const uint16_t width = 240;
const uint16_t height = 320;
const uint8_t countXPosition = width - width / 3;

struct MeasurmentData {
  const char* temperature;
  const char* humidity;
  const char* pressure;
};

uint8_t address[8];
uint8_t selected;
uint16_t measurmentCount = 0;

void drawMeasurements(const MeasurmentData measurements);
void drawBorders();
void drawLabels();

void setup() {
  Serial.begin(115200);
  Serial.print("Devices: ");
  ds.getAddress(address);

  selected = ds.select(address);
  if (selected) {
    Serial.println("Device found!");
  } else {
    Serial.println("Device not found!");
  }

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(COLORT1);
  tft.setTextSize(1);
  Serial.println();

  drawBorders();
  drawLabels();

  Serial.println();
}

void loop() {
  if (selected) {
    Serial.print(ds.getTempC());
    Serial.println(" C");
    measurmentCount++;
    drawMeasurements(ds.getTempC(), measurmentCount, address);
  } else {
    Serial.println("Device not found!");
  }

  delay(1000);
}

void drawMeasurements(double temp, uint16_t count, uint8_t address[8]){
  tft.setTextColor(COUNTCOLOR, COLORT3);
  
  tft.setCursor(countXPosition + 5, 10, 1);
  tft.print(count);

  tft.setTextColor(COLORT2, COLORT1);
  uint16_t y_pos = 75;
  tft.setCursor(70, y_pos, 1);
  tft.print(temp);
  tft.print(" C");
  y_pos += 25;

  tft.setCursor(30, y_pos, 1);
  for (byte i = 0; i < 8; i++) {
    tft.print(address[i], HEX);
  }
}

void drawBorders(){
  tft.setTextColor(COLORT2);
  uint16_t x_pos = countXPosition;
  //top vertical line
  tft.drawFastVLine(x_pos, 0, 30, COLORT2);
  //top horizontal line
  tft.drawFastHLine(0, 30, width, COLORT2);
  //2nd horizontal line
  tft.drawFastHLine(0, 65, width, COLORT2);
  //bottom horizontal line
  tft.drawFastHLine(0, height - 45, width, COLORT2);

  tft.fillRect(countXPosition + 1, 0, 100, 29, COLORT3);
}

void drawLabels(){
  tft.setTextColor(COLORT2);

  tft.setTextSize(2);
  tft.drawCentreString("Sensor DS18B20", width / 2, 40, 1);

  tft.setTextColor(COLORT3);
  tft.drawCentreString("Laboratorium 4a", width / 2, height - 40, 2);
}