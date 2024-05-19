#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "FS.h"
#include "SD.h"

WebServer webServer(80);
float dataT[1000]={0.0};
uint16_t dataSize = 0;
String sOut="";
byte autorefresh = 10;

const char *ssid = "******";
const char *password = "*****";

String timestamp = "12:23:33";

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
uint16_t countXPosition = 0;
unsigned long measurementTime = 0;

TFT_eSPI tft = TFT_eSPI();

void PrintWykres(int dataLen,int color);//funkcja rysująca wykres, dane z dataT
void handleRoot(); // funkcja generująca stronę w formacie html
void handleNotFound();

void drawBorders();
void drawLabels();
void drawMeasurements(const measurmentData measurements, uint16_t count);

void readBME();

void setup(void) {
  Serial.begin(115200);
  //Konfiguracja połączenia WiFi jako STA
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  webServer.on("/", handleRoot);
  webServer.onNotFound(handleNotFound);
  webServer.begin();
  Serial.println("HTTP server started");

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(COLORT1);
  tft.setTextSize(1);
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
}

void loop(void) {
  webServer.handleClient();
  delay(2);//allow the cpu to switch to other tasks
  //odczyt zegara, BME, zapis na karcie
  
  if(measurementTime < millis()){
    readBME();
    drawMeasurements({ temperature: bmpT, humidity: bmpH, pressure: (uint16_t)bmpP}, measurmentCount);

    if(dataSize<=400){
      dataSize+=1;
      dataT[dataSize]=bmpT; //buforowanie pomiarów
    }else { 
      dataSize=0;
    }
    measurementTime = millis() + 5000;
  }
}

void handleRoot() {
  int sec = millis() / 1000;
  int mi = sec / 60;
  int hr = mi / 60;
  char tempS[100];

  sOut="<html><head>\
  <meta http-equiv='refresh' content='";
  sOut+=String(int(autorefresh))+"'/>";
  sOut+="<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\
  <meta http-equiv=\"Content-Language\" content=\"pl\" />\
  <title>ESP32</title>\
  <style> html{font-family:Verdana;display:inline-block;backgroundcolor:#FFF;}\
  .font_t{color:white;text-align:center;text-decoration:none;color:#111; fontweight:bold;padding:8px;margin:8px;}\
  .line_t{border-top:3pxsolid;border-image: linear-gradient(toleft,#111,#E00,#111);border-image-slice:1;}\
  .graph_t{text-align:center;}\
  </style> </head><body>\
  <h1 class=\"font_t\">ESP32 laboratorium</h1>";
  
  sOut+="<hr class=\"line_t\"><h1 class=\"font_t\">Temperatura = ";
  sOut+=String(bmpT)+" C";
  sOut+="</h1><br><div class=\"graph_t\">";
  
  PrintWykres(dataSize, 1);
  
  sOut+="</div>";
  sOut+="<br>Current time: "+timestamp;
  sprintf(tempS, "<br><p>Uptime: %02d:%02d:%02d</p>", hr, mi % 60, sec % 60);
  sOut += tempS;
  sOut+="</body></html>";
  webServer.send(200, "text/html", sOut);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += webServer.uri();
  message += "\nMethod: ";
  message += (webServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += webServer.args();
  message += "\n";
  webServer.send(404, "text/html", message);
}

void PrintWykres(int dataLen,int color) {
  char tempS[100];
  sOut += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"width=\"400\" height=\"150\">\n";
  sOut += "<rect width=\"400\" height=\"150\" fill=\"rgb(200, 200, 200)\" stroke-width=\"1\" stroke=\"rgb(255, 255, 255)\" />\n";
  if (color == 1) sOut += "<g stroke=\"red\">\n";
  if (color == 2) sOut += "<g stroke=\"blue\">\n";
  if (color == 3) sOut += "<g stroke=\"yellow\">\n";
  float miny=1000;
  float maxy=-1000;
  float dx,skala;
  int newX,x,y,newY;
  for (int i=1; i<dataLen ; i++) {
    if(dataT[i]>maxy)maxy=dataT[i];
    if(dataT[i]<miny)miny=dataT[i];
  }

  dx=abs(maxy-miny);
  if(dx>0)  skala=120/dx; 
  else skala=75;

  y=int((dataT[1]-miny)*skala);
  x=1;

  for (int i=2; i<dataLen ; i+=1) {
    newY=int((dataT[i]-miny)*skala);
    newX=x+1;
    sprintf(tempS, "<line x1=\"%d\" y1=\"%d\" x2=\"%d\" y2=\"%d\" \
    stroke-width=\"2\" />\n", x, 130-y, newX, 130-newY);
    sOut += tempS;
    y = newY; x = newX;
  }
  sOut += "</g>\n"; sOut += "</svg>\n";
  // webServer.send(200, "image/svg+xml", sOut);
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  
  if(!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  
  if(file.print(message)){ 
    Serial.println("File written");
  } else { 
    Serial.println("Write failed"); 
  }
  
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Appending to file: %s\n", path);
  File file = fs.open(path, FILE_APPEND);
  
  if(!file) {
    Serial.println("Failed to open file for appending");
    return;
  }

  if(file.print(message)) { 
    Serial.println("Message appended");
  } else { 
    Serial.println("Append failed"); 
  }
  
  file.close();
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
  uint8_t width = tft.width() / 4;
  tft.drawCentreString("BME280", width, 40, 1);
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

void readBME(){
  if (selected) {
    bmpT=bme.readTemperature();
    bmpH=bme.readHumidity();
    bmpP=bme.readPressure()/100.0;
    measurmentCount++;
  } 
  else {
    Serial.println("Device not found!");
  }
}