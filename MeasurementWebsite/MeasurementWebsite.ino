#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_BME280.h>
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include "tftUtils.h"

float dataT[1000]={0.0};
float dataP[1000]={0.0};
float dataH[1000]={0.0};
uint16_t dataSize = 0;

WebServer webServer(80);
const char *ssid = "*******";
const char *password = "*******";
String sOut="";
byte autorefresh = 120;
int chartWidth = 80;
int chartHeight = 10;

String timestamp = "12:23:33";

Adafruit_BME280 bme;
float bmpT,bmpP,bmpH;
uint8_t selected;

uint16_t measurmentCount = 0;
unsigned long measurementTime = 0;

void readBME();

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
    drawMeasurements({ temperature: bmpT, humidity: bmpH, pressure: (uint16_t)bmpP, timestamp: "12:23:33"}, measurmentCount);
    
    if(dataSize<=400){
      dataSize+=1;
      dataT[dataSize]=bmpT;
      dataP[dataSize]=bmpP;
      dataH[dataSize]=bmpH;
    }else { 
      dataSize=0;
    }
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
  .font_t{color:white;text-align:center;text-decoration:none;color:#111; fontweight:bold;padding:8px;margin:8px;}\
  .line_t{border-top:3pxsolid;border-image: linear-gradient(toleft,#111,#E00,#111);border-image-slice:1;}\
  .graph_t{text-align:center;}\
  </style> </head><body>\
  <h1 class=\"font_t\">ESP32 laboratorium</h1>";
  
  sOut+="<hr class=\"line_t\"><h1 class=\"font_t\">Temperatura = ";
  sOut+=String(bmpT)+" C";
  sOut+="</h1><br><div class=\"graph_t\">";
  PrintWykres(dataSize, 1, dataT);
  
  sOut+="<hr class=\"line_t\"><h1 class=\"font_t\">Temperatura = ";
  sOut+=String(bmpP)+" hPa";
  sOut+="</h1><br>";
  PrintWykres(dataSize, 2, dataP);
  
  sOut+="<hr class=\"line_t\"><h1 class=\"font_t\">Temperatura = ";
  sOut+=String(bmpH)+" %";
  sOut+="</h1><br>";
  PrintWykres(dataSize, 3, dataH);
  
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

void PrintWykres(int dataLen,int color, float* chartData) {
  char tempS[100];
  sOut += "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\"width=\"80vw\" height=\"10vh\">\n";
  sOut += "<rect width=\"80vw\" height=\"10vh\" fill=\"rgb(200, 200, 200)\" stroke-width=\"1\" stroke=\"rgb(255, 255, 255)\" />\n";
  if (color == 1) sOut += "<g stroke=\"red\">\n";
  if (color == 2) sOut += "<g stroke=\"blue\">\n";
  if (color == 3) sOut += "<g stroke=\"yellow\">\n";
  float miny=1000;
  float maxy=-1000;
  float dx, scale;
  int newX, x, y, newY;
  for (int i=1; i<dataLen ; i++) {
    if(chartData[i]>maxy)maxy=chartData[i];
    if(chartData[i]<miny)miny=chartData[i];
  }


  int lineMargin = 5; //odstęp lini od krawędzi pionowych wykresu(w procentach)

  dx=abs(maxy-miny);
  if(dx>0)  scale=(100 - lineMargin * 2)/dx; 
  else scale = 100 - lineMargin * 2;

  y=int(lineMargin + (chartData[1]-miny)*scale);
  x=1;

  for (int i=2; i<dataLen ; i+=1) {
    newY=int(lineMargin + (chartData[i]-miny)*scale
);
    newX=x+1;
    sprintf(tempS, "<line x1=\"%d\" y1=\"%d%%\" x2=\"%d\" y2=\"%d%%\" stroke-width=\"2\" />\n", x, 100-y, newX, 100-newY);
    sOut += tempS;
    y = newY; x = newX;
  }
  sOut += "</g>\n"; 
  sOut += "</svg>\n";
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
