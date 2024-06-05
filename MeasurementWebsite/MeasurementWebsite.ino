#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include "tftUtils.h"
#include "wifiProperties.h"
#include "time.h"

uint16_t dataSize = 0;
const uint16_t dataTargetSize = 101;
uint16_t startIndex = 0;
float dataT[dataTargetSize] = {0.0};
float dataP[dataTargetSize] = {0.0};
float dataH[dataTargetSize] = {0.0};

WebServer webServer(80);
String sOut="";
const uint16_t autorefresh = 2000;

String chartWidth = "80";
String chartHeight = "10";
const int lineMargin = 5; //distance of the line from the vertical edges of the chart(percents)
const int chartXOffset = 0;
const byte xLabelCount = 21;
const byte xSecondLabelCount = 5;

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

Adafruit_BME280 bme;
float bmpT, bmpP, bmpH = 0;
uint8_t selected;

uint16_t measurmentCount = 0;
unsigned long measurementTime = 0;


void setup(void) {
  Serial.begin(115200);
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

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

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
  delay(2);
  
  if(measurementTime < millis()){
    readBME();

    char currentTime[10];
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      sprintf(currentTime, "");
    }else{
      strftime(currentTime, 10, "%H:%M:%S", &timeinfo);
    }
    drawMeasurements({temperature: bmpT, humidity: bmpH, pressure: (uint16_t)bmpP, timestamp: currentTime}, measurmentCount);
    
    uint16_t currentIndex = dataSize % dataTargetSize;
    dataT[currentIndex] = bmpT;
    dataP[currentIndex] = bmpP;
    dataH[currentIndex] = bmpH;
    if(dataSize >= dataTargetSize) { 
      startIndex = (currentIndex + 1) % dataTargetSize;
    }
    dataSize += 1;

    measurementTime = millis() + 1000;
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
  .font_t{color:white;text-align:center;text-decoration:none;color:#111; font-weight:bold;padding:8px;margin:8px;}\
  .line_t{border-top:2px solid;border-image: linear-gradient(to left,#111,#E00,#111);border-image-slice:1;}\
  .graph_t{}\
  .main_content{text-align:center;}\
  .axismark-main {stroke: black;stroke-width: 1;}\
  .axismark-second {stroke: black;stroke-width: 1;}\
  .axisnumber-x {font-size: 12px;color: black;}\
  .axisnumber-y {font-size: 12px;color: black;}\
  </style> </head><body>\
  <h1 class=\"font_t\">ESP32 laboratorium</h1>";
  webServer.setContentLength(CONTENT_LENGTH_UNKNOWN);
  webServer.send(200, "text/html", sOut);

  sOut="<div class=\"main_content\"><hr class=\"line_t\"><h1 class=\"font_t\">Temperatura = ";
  sOut+=String(bmpT)+" C";
  sOut+="</h1><br><div class=\"graph_t\">";
  webServer.sendContent(sOut);
  PrintWykres(dataSize, 1, dataT);

  sOut="</div><br><hr class=\"line_t\"><h1 class=\"font_t\">Ciśnienie = ";
  sOut+=String(bmpP)+" hPa";
  sOut+="</h1><br><div class=\"graph_t\">";
  webServer.sendContent(sOut);
  PrintWykres(dataSize, 2, dataP);
  
  sOut="</div><br><hr class=\"line_t\"><h1 class=\"font_t\">Wilgotność = ";
  sOut+=String(bmpH)+" %";
  sOut+="</h1><br><div class=\"graph_t\">";
  webServer.sendContent(sOut);
  PrintWykres(dataSize, 3, dataH);
  
  sOut="</div></div><br>";

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
  }else{
    strftime(tempS, 100, "<br>Current time: %H:%M:%S %Y-%m-%d", &timeinfo);
    sOut+= tempS;
  }
  sprintf(tempS, "<br><p>Uptime: %02d:%02d:%02d</p>", hr, mi % 60, sec % 60);
  sOut += tempS;
  sOut+="</body></html>";
  webServer.sendContent(sOut);
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

void PrintWykres(int dataLen, int color, float* chartData) {
  char tempS[150];
  String output = "";
  output += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"width=\""+ chartWidth + "dvw\" height=\"" + chartHeight + "dvh\" overflow=visible>\n";
  sprintf(tempS, "<g transform=\"translate(%d, 0)\">", chartXOffset);
  output += tempS;
  output += "<rect width=\"100%\" height=\"100%\" fill=\"rgb(200, 200, 200)\" stroke-width=\"1\" stroke=\"rgb(255, 255, 255)\" />\n";
  float miny=1000;
  float maxy=-1000;
  float dx, scale;
  float y, newY;
  float x, newX;
  unsigned int maxElements = 400;
  float xShift = 100 * 1. / (dataTargetSize - 1);

  dataLen = (dataTargetSize > dataLen ? dataLen : dataTargetSize);
  for (int i=0; i< dataLen ; i++) {
    if(chartData[i] > maxy) maxy = chartData[i];
    if(chartData[i] < miny) miny = chartData[i];
  }

  dx=abs(maxy - miny);
  
  if(dx > 0)  scale = (100 - lineMargin * 2)/dx; 
  else scale = (100 - lineMargin * 2);

  output += "<g id=\"axis-y-values\" class=\"axisnumber-y\" text-anchor=\"end\">";
  sprintf(tempS, "<text y=\"%d%%\" x=\"%d\">%.2f</text>", 0 + lineMargin, -8, maxy);
  output += tempS;
  sprintf(tempS, "<text y=\"%d%%\" x=\"%d\">%.2f</text>", 100 - lineMargin, -8, miny);
  output += tempS;

  if(dx >= 0.02){
    sprintf(tempS, "<text y=\"%d%%\" x=\"%d\">%.2f</text>", 50, -8, miny + dx / 2);
    output += tempS;
  }
  
  output += "</g>";

  output += "<g id=\"axis-x-values\" class=\"axisnumber-x\" text-anchor=\"middle\">";

  unsigned int step = dataTargetSize / (xLabelCount - 1);

  unsigned int labelStart = 0;
  unsigned int labelEnd = dataTargetSize - 1;
  if(dataSize > dataTargetSize){
    labelStart = dataSize - dataTargetSize;
    labelEnd = dataSize - 1;
  }

  for(unsigned int i = labelStart; i <= labelEnd; i += step){
    sprintf(tempS, "<text y=\"%d%%\" x=\"%f%%\" transform=\"translate(0, 18)\">%d</text>", 100, xShift * (i - labelStart), i);
    output += tempS;
  }
  output += "</g>";

  output += "<defs xmlns=\"http://www.w3.org/2000/svg\">";
  
  sprintf(tempS, "<pattern id=\"x-axismark-main\" x=\"0\" width=\"%d%%\" height=\"6\" patternUnits=\"userSpaceOnUse\">", 100 / (xLabelCount - 1));
  output += tempS;
  
  output += "<line x1=\"0\" y1=\"0\" x2=\"0\" y2=\"6\" class=\"axismark-main\"/>\
  </pattern>";

  sprintf(tempS, "<pattern id=\"x-axismark-second\" x=\"0\" width=\"%f%%\" height=\"3\" patternUnits=\"userSpaceOnUse\">", float(100 / (xLabelCount - 1)) / (xSecondLabelCount + 1));
  output += tempS;

  output += "<line x1=\"0\" y1=\"-1\" x2=\"0\" y2=\"3\" class=\"axismark-second\"/>\
  </pattern>";
  sprintf(tempS, "<pattern id=\"y-axismark-main\" y=\"-%d%%\" width=\"6\" height=\"%d%%\" patternUnits=\"userSpaceOnUse\">", lineMargin, 50 - lineMargin);
  output += tempS;
  output += "<line x1=\"-1\" y1=\"0\" x2=\"6\" y2=\"0\" class=\"axismark-main\"/>\
  </pattern>";
  sprintf(tempS, "<pattern id=\"y-axismark-second\" y=\"-%d%%\" width=\"3\" height=\"%f%%\" patternUnits=\"userSpaceOnUse\">", lineMargin, (50 - lineMargin * 1.) / 4);
  output += tempS;
  output += "<line x1=\"-1\" y1=\"0\" x2=\"3\" y2=\"0\" class=\"axismark-second\"/>\
  </pattern>\
  </defs>";

  output += "<g transform=\"scale(1, -1)\">\
  <rect id=\"x-axismark2\" x=\"0\" y=\"calc(-100% - 3px)\" width=\"100.5%\" height=\"calc(100% + 3px)\" fill=\"url(#x-axismark-second)\"/>\
  <rect id=\"x-axismark\" x=\"0\" y=\"calc(-100% - 6px)\" width=\"100.5%\" height=\"6\" fill=\"url(#x-axismark-main)\"/>\
  <rect id=\"y-axismark2\" x=\"-3\" y=\"-100%\" width=\"calc(100% + 3px)\" height=\"100%\" fill=\"url(#y-axismark-second)\"/>\
  <rect id=\"y-axismark\" x=\"-6\" y=\"-100%\" width=\"6\" height=\"101%\" fill=\"url(#y-axismark-main)\"/>\
  </g>";
  webServer.sendContent(output);
  output = "";

  if (color == 1) output += "<g stroke=\"red\">\n";
  if (color == 2) output += "<g stroke=\"blue\">\n";
  if (color == 3) output += "<g stroke=\"yellow\">\n";

  y = lineMargin + (chartData[startIndex] - miny) * scale;
  x = 0;

  for (int i=(startIndex + 1) % dataTargetSize; i<dataLen ; i+=1) {
    newY = lineMargin + (chartData[i] - miny) * scale;
    newX = x + xShift;
    sprintf(tempS, "<line x1=\"%f%%\" y1=\"%f%%\" x2=\"%f%%\" y2=\"%f%%\" stroke-width=\"2\" />\n", x, 100-y, newX, 100-newY);
    output += tempS;

    if(maxElements < newX){
      webServer.sendContent(output);
      output = "";
      maxElements += 400;
    }

    y = newY; 
    x = newX;
  }

  for (int i=0; i<startIndex ; i+=1) {
    newY = lineMargin + (chartData[i] - miny) * scale;
    newX = x + xShift;

    sprintf(tempS, "<line x1=\"%f%%\" y1=\"%f%%\" x2=\"%f%%\" y2=\"%f%%\" stroke-width=\"2\" />\n", x, 100-y, newX, 100-newY);
    output += tempS;

    if(maxElements < newX){
      webServer.sendContent(output);
      output = "";
      maxElements += 400;
    }

    y = newY; 
    x = newX;
  }

  output += "</g>\n"; 
  output += "</g>\n"; 
  output += "</svg>\n";
  webServer.sendContent(output);
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
