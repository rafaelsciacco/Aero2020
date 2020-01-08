//Bibliotecas
#include <RTClib.h> //RTC
#include <Adafruit_GFX.h> //Display OLED
#include <Adafruit_SSD1306.h>//Display OLED
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
  
  Serial.print("RPM");
  Serial.print("  ");
  Serial.print("Tempo");
  Serial.println("  ");
  delayMicroseconds(1000);
}

void loop() {
  copyconta_RPM = conta_RPM;
  contaAtualMillis = millis(); 
  subMillis = contaAtualMillis - antigoMillis;
  minutos = (subMillis/(60000));
  RPM = copyconta_RPM/(4*minutos);
  antigoMillis = contaAtualMillis;
  conta_RPM = 0;
  Serial.print(RPM);
  Serial.print("  ");

  DateTime now = rtc.now();
  tempo = ((now.hour()*3600)+(now.minute()*60)+(now.second()));
  Serial.print(tempo);
  Serial.println("  ");
  delayMicroseconds(1000000);

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
}
