//Bibliotecas
#include <RTClib.h> //RTC
#include <Adafruit_GFX.h> //OLED
#include <Adafruit_SSD1306.h> //OLED
#include <Wire.h> //MPU
#include <Adafruit_HMC5883_U.h> //HMC
#include <MapFloat.h> //HMC

//Variaveis rpm
#define pinINT   27
volatile int conta_RPM = 0;
int copyconta_RPM = 0;
float RPM = 0;
float contaAtualMillis = 0;
float antigoMillis = 0;
float subMillis = 0;
float minutos = 0;

//Variaveis tempo
float tempo  = 0;
RTC_DS1307 rtc;

//Objeto display
Adafruit_SSD1306 display(128, 64);
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMono9pt7b.h>

//Variaveis mpu
const int MPU_addr=0x69;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int minVal=265;
int maxVal=402;
float x, y, z;
float pitch, roll, pitchprefiltro, rollprefiltro;

//Variaveis mag
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
float MagBow = 0;

//Funcao interrupcao
void IRAM_ATTR ContaInterrupt() {
  conta_RPM = conta_RPM + 1; //incrementa o contador de interrupções
}

void setup() {
  Serial.begin(115200);
  pinMode(pinINT,INPUT_PULLUP);
  attachInterrupt(pinINT,ContaInterrupt, RISING);
  
  if (! rtc.begin()) {
    Serial.println("RTC nao detectado");
    while (1);
  }
  //rtc.adjust(DateTime(2019, 10, 26, 15, 22, 0));  // (Ano,mês,dia,hora,minuto,segundo)

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setRotation(0);
  display.setTextWrap(true);
  display.dim(0);
  display.setFont(&FreeMono9pt7b);
  display.setTextSize(0);

  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  if(!mag.begin())
  {
    Serial.println("HMC5883 nao detectado");
    display.setCursor(0,25);
    display.println("HMC5883 nao detectado!");
    display.display();
    while(1);
  }
  
  Serial.print("RPM");
  Serial.print("  ");
  Serial.print("Tempo");
  Serial.print("  ");
  Serial.print("NZ");
  Serial.print("  ");
  Serial.print("THETA");
  Serial.print("  ");
  Serial.print("PHI");
  Serial.print("  ");
  Serial.print("MagBow");
  Serial.println("  ");
  
  delayMicroseconds(1000000);
}

void loop() {
  copyconta_RPM = conta_RPM;
  contaAtualMillis = millis(); 
  subMillis = contaAtualMillis - antigoMillis;
  minutos = (subMillis/(60000));
  RPM = copyconta_RPM/(4*minutos);
  antigoMillis = contaAtualMillis;
  conta_RPM = 0;

  DateTime now = rtc.now();
  tempo = ((now.hour()*3600)+(now.minute()*60)+(now.second()));

  char stringrpm[10];
  dtostrf(RPM, 6, 0, stringrpm);
  display.clearDisplay();
  display.setFont(&FreeMonoBold18pt7b);
  display.setCursor(0, 25);
  display.println("RPM:");
  display.setCursor(0, 60);
  display.print(stringrpm);
  display.println("[RPM]");
  display.display();
  
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_addr,14,true); 
  AcX=Wire.read()<<8|Wire.read();
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  int xAng = map(AcX,minVal,maxVal,-90,90);
  int yAng = map(AcY,minVal,maxVal,-90,90);
  int zAng = map(AcZ,minVal,maxVal,-90,90);

  x= RAD_TO_DEG * (atan2(-yAng, -zAng)+PI);
  y= RAD_TO_DEG * (atan2(-xAng, -zAng)+PI);
  z= RAD_TO_DEG * (atan2(-yAng, -xAng)+PI);
  pitchprefiltro = x;
  rollprefiltro = y;
  if(pitchprefiltro > 180) pitchprefiltro -= 360;
  if(rollprefiltro > 180) rollprefiltro -= 360;
  if(abs(pitchprefiltro) < 35) pitch = pitchprefiltro;
  if(abs(rollprefiltro) < 30) roll = rollprefiltro;

  sensors_event_t event;
  mag.getEvent(&event);
  float heading = atan2(event.magnetic.y, event.magnetic.x);
  float declinationAngle = 0.37;
  heading += declinationAngle;
  if(heading < 0)
    heading += 2*PI;
  if(heading > 2*PI)
    heading -= 2*PI;
  float headingDegrees = heading * 180/M_PI;
  if(headingDegrees <= 264.00){
    MagBow = mapFloat(headingDegrees,264.00, 0.00,0.00, 264.00);
  }
  if(headingDegrees > 264.00){
    MagBow = mapFloat(headingDegrees,360.00 , 263.99, 264.01, 360.00);
  }
  
  Serial.print(RPM);
  Serial.print("  ");
  Serial.print(tempo);
  Serial.print("  ");
  Serial.print(AcZ/16384.0);
  Serial.print("  ");
  Serial.print(pitch);
  Serial.print("  ");
  Serial.print(roll);
  Serial.print("  ");
  Serial.print(MagBow);
  Serial.println("  ");
  
  delayMicroseconds(1000000);
}
