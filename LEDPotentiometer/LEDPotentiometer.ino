#define LED 32
#define POT 34
#define pwmChannel 0
#define pwmRes 12
#define pwmFreq 5000

u_int16_t potValue = 0;
bool isLedOn = false;

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED, OUTPUT);
  ledcSetup(pwmChannel, pwmFreq, pwmRes);
  ledcAttachPin(LED, pwmChannel);
}

void loop() {
  potValue = analogRead(POT);
  ledcWrite(pwmChannel, potValue);
  delay(10);
}
