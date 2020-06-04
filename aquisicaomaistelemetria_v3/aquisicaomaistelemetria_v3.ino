//FUNCIONAIS 20:47 03/06/2020: NRF + SD + RTC + MPU + MAG + BMP + GPS + HALL
//FALTAM: WOW, VCAS, ELEV, AIL, RUD, AOA, AOS
int elev, ail ,rud;
float WOW = 6;
float velocidademps = 7;

//Bibliotecas
#include <mySD.h> //SD
#include <SPI.h> //SD e NRF
#include <nRF24L01.h> //NRF
#include <RF24.h> //NRF
#include <RTClib.h> //RTC
#include <Wire.h> //MPU MAG E BMP
#include <Adafruit_Sensor.h> //MAG
#include <Adafruit_HMC5883_U.h> //MAG
#include <MapFloat.h> //MAG
#include <Adafruit_BMP085.h> //BMP
#include <TinyGPS.h> //GPS

//Variaveis RTC
float tempo  = 0;
RTC_DS1307 rtc;

//Variaveis MPU
const int MPU_addr=0x69;
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;
int minVal=265;
int maxVal=402;
float x, y, z;
float nz;
float pitch, roll, pitchprefiltro, rollprefiltro, pitchF, rollF;

//Variaveis MAG
Adafruit_HMC5883_Unified mag = Adafruit_HMC5883_Unified(12345);
float MagBow = 0;

//Variaveis BMP
Adafruit_BMP085 bmp;
float HP = 0;

//Variaveis GPS
#define RXD2 16
#define TXD2 17
TinyGPS gps1;

//Variaveis RPM
#define pinINT   27
volatile int conta_RPM = 0;
int copyconta_RPM = 0;
float RPM = 0;
float contaAtualMillis = 0;
float antigoMillis = 0;
float subMillis = 0;
float minutos = 0;

//Funcao interrupcao
void IRAM_ATTR ContaInterrupt() {
  conta_RPM = conta_RPM + 1; //incrementa o contador de interrupções
}

//NRF
unsigned long currentMillis;
unsigned long prevMillis;
unsigned long txIntervalMillis = 1;
#define CE_PIN   33
#define CSN_PIN 32
const byte slaveAddress[5] = {'R','x','A','A','A'};
RF24 radio(CE_PIN, CSN_PIN); 
String dataString;

void send() {
  
  float dataToSend[7] = {rollF,pitchF,nz,MagBow,HP,WOW,velocidademps};
  bool rslt;
  rslt = radio.write( &dataToSend, sizeof(dataToSend) );
  Serial.print("Data Sent ");
  for(int i = 0; i<7;i++){
    Serial.print(dataToSend[i]);
    Serial.print(" ");
  }
  //Serial.print(dataToSend);
  if (rslt) {
      Serial.println("  Informação recebida");
      //updateMessage();
  }
  else {
      Serial.println("  Tx falhou");
  }
  
}

//Funções microSD
File root;

void setupSD(){
  Serial.print("Iniciando cartão SD...");
  // Inicia a biblioteca mySD com os pinos desejados
  if (!SD.begin(26, 14, 13, 25)) { //(CS,MOSI,MISO,SCK)
    Serial.println("inicialização falhou!");
    return;
  }
  Serial.println("inicizaliação concluída.");
  root = SD.open("/");
  if (root) {    
    printDirectory(root, 0);
    root.close();
  } else {
    Serial.println("erro ao abrir arquivo");
  }
}

void removeFile(char * path){
  root = SD.open(path);
  if (root) {
    SD.remove(path);
    root.close();
  } else {
    Serial.println("erro ao remover arquivo");
  }
}

void writeFile(const char * path,const char * message){
  // Abrir arquivo de interesse para escrever
  root = SD.open(path, FILE_WRITE);    
  if (root) {
    root.print(message);
    root.flush();
    root.close();
  } else {
    Serial.println("erro ao abrir arquivo para escrever");
  }
}

void readFile(char *path){
  root = SD.open(path);
  if (root) {    
    //Ler arquivo até o final.
    while (root.available()) {
      Serial.write(root.read());
    }
    root.close();
  } else {
    Serial.println("erro ao ler arquivo");
  }
}

void printDirectory(File dir, int numTabs) { 
  //Exibir conteúdo do cartão SD.
  while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');
     }
     Serial.print(entry.name());
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       Serial.print("\t\t");
       Serial.println(entry.size());
     }
     entry.close();
   }
}

void setup()
{
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
    
  //Setup NRF
  radio.begin();
  radio.setDataRate( RF24_250KBPS );
  radio.setRetries(3,5); // delay, count
  radio.openWritingPipe(slaveAddress);

  //Setup SD
  setupSD();
  removeFile("test.txt"); 
  Serial.println("Arquivo test.txt já existente removido!");

  //Setup RTC
  if (! rtc.begin()) {
    Serial.println("RTC nao detectado");
    while (1);
  }
  //rtc.adjust(DateTime(2020, 6, 3, 19, 10, 0));  // (Ano,mês,dia,hora,minuto,segundo)

  //Setup MPU
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  //Setup MAG
  if(!mag.begin())
  {
    Serial.println("HMC5883 nao detectado");
    while(1);
  }

  //Setup BMP
  if (!bmp.begin()) {
  Serial.println("BMP nao detectado");
  while (1) {}
  }

  //Setup RPM
  pinMode(pinINT,INPUT_PULLUP);
  attachInterrupt(pinINT,ContaInterrupt, RISING);
  
  //Escrever no arquivo test.txt os parâmetros e as unidades de medida
  writeFile("test.txt", "   Tempo   RPM   WOW   VCAS    MagHead   ELEV    AIL   RUD   HP    NZ    THETA   PHI   XGPS    YGPS    ZGPS \r\n   [segundos]    [RPM]   [bit]   [m/s]   [deg]   [deg]   [deg]   [deg]   [ft]    [g]   [deg]   [deg]   [m]   [m]   [m] \r\n");

}

void loop(){
  //Enviar informação pelo NRF a cada txIntervalMillis 
  currentMillis = millis();
  if (currentMillis - prevMillis >= txIntervalMillis) {
      send();
      prevMillis = millis();
  }

  //Horario em segundos
  DateTime now = rtc.now();
  tempo = ((now.hour()*3600)+(now.minute()*60)+(now.second()));

  //Pitch, Roll e Nz
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
  nz = (AcZ/16384.0);
  if(pitchprefiltro > 180) pitchprefiltro -= 360;
  if(rollprefiltro > 180) rollprefiltro -= 360;
  if(abs(pitchprefiltro) < 35) pitch = pitchprefiltro;
  if(abs(rollprefiltro) < 30) roll = rollprefiltro;

  //Filtro passa baixas
  rollF = 0.94 * rollF + 0.06 * rollprefiltro;
  pitchF = 0.94 * pitchF + 0.06 * pitchprefiltro;

  //MagBow
  sensors_event_t event;
  mag.getEvent(&event);
  //eixo Z para cima
  float heading = atan2(event.magnetic.y, event.magnetic.x);
  //http://www.magnetic-declination.com/
  //Exemplo: -13* 2' W, ~13 graus = 0.22 radianos
  //Se não houver declinationAngle, comentar as duas linhas seguintes
  float declinationAngle = 0.37;
  heading += declinationAngle;
  if(heading < 0)
    heading += 2*PI;
  if(heading > 2*PI)
    heading -= 2*PI;
  float headingDegrees = heading * 180/M_PI;
  MagBow = headingDegrees;
  /*if(headingDegrees <= 264.00){
    MagBow = mapFloat(headingDegrees,264.00, 0.00,0.00, 264.00);
  }
  if(headingDegrees > 264.00){
    MagBow = mapFloat(headingDegrees,360.00 , 263.99, 264.01, 360.00);
  }*/

  //HP
  //Altitude mais precisa com a pressão a nível do mar atual:
  //https://climatologiageografica.com/pressao-barometrica-ao-nivel-mar-em-tempo-real/
  //Exemplo: 1015 milibars = 101500 Pascal. O parâmetro de readAltitude deve ser 101500.
  HP = bmp.readAltitude(101800)*3.28084; //*3.28084 = conversão de [m] para [ft].

  //GPS
  bool recebido = false;
  while (Serial2.available()) {
    char cIn = Serial2.read();
    recebido = (gps1.encode(cIn) || recebido);
  }
  float latitude, longitude, xgps, ygps;
  unsigned long idadeInfo;
  gps1.f_get_position(&latitude, &longitude, &idadeInfo);
  float altitudeGPS, zgps;
  altitudeGPS = gps1.f_altitude();
  if (latitude != TinyGPS::GPS_INVALID_F_ANGLE) {
    xgps = (latitude);
  }
  if (longitude != TinyGPS::GPS_INVALID_F_ANGLE) {
    ygps = (longitude);
  }
  if ((altitudeGPS != TinyGPS::GPS_INVALID_ALTITUDE) && (altitudeGPS != 1000000)) {
    zgps = altitudeGPS;
  }

  //RPM
  copyconta_RPM = conta_RPM;
  contaAtualMillis = millis(); 
  subMillis = contaAtualMillis - antigoMillis;
  minutos = (subMillis/(60000));
  RPM = copyconta_RPM/(4*minutos);
  antigoMillis = contaAtualMillis;
  conta_RPM = 0;
  
  //Escrever no arquivo test.txt o valor de cada parâmetro
  writeFile("test.txt",("   "+String(tempo)+"   "+String(RPM)+"   "+String(WOW)+"   "+String(velocidademps)+"   "+String(MagBow)+"   "+String(elev)+"   "+String(ail)+"   "+String(rud)+"   "+String(HP)+"   "+String(nz)+"   "+String(pitch)+"   "+String(roll)+"   "+String(xgps,6)+"   "+String(ygps,6)+"   "+String(zgps)+"\r\n").c_str());
  //readFile("test.txt");
  //delayMicroseconds(1);
}
