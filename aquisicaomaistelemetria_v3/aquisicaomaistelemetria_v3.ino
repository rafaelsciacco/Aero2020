int RPM, elev, ail ,rud = 1;
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

//NRF
unsigned long currentMillis;
unsigned long prevMillis;
unsigned long txIntervalMillis = 10;
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
      Serial.println("  Acknowledge received");
      //updateMessage();
  }
  else {
      Serial.println("  Tx failed");
  }
  
}

//Funções microSD
File root;

void setupSD(){
  Serial.print("Initializing SD card...");
  /* initialize SD library with Soft SPI pins, if using Hard SPI replace with this SD.begin()*/
  if (!SD.begin(26, 14, 13, 27)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
  /* Begin at the root "/" */
  root = SD.open("/");
  if (root) {    
    printDirectory(root, 0);
    root.close();
  } else {
    Serial.println("error opening test.txt");
  }
}

void removeFile(char * path){
  root = SD.open(path);
  if (root) {
    SD.remove(path);
    root.close();
  } else {
    Serial.println("error removing file");
  }
}

void writeFile(const char * path,const char * message){
  /* open "test.txt" for writing */
  root = SD.open(path, FILE_WRITE);
  /* if open succesfully -> root != NULL 
    then write string "Hello world!" to it
  */
  if (root) {
    root.print(message);
    root.flush();
   /* close the file */
    root.close();
  } else {
    /* if the file open error, print an error */
    Serial.println("error opening test.txt");
  }
}

void readFile(char *path){
  /* after writing then reopen the file and read it */
  root = SD.open(path);
  if (root) {    
    /* read from the file until there's nothing else in it */
    while (root.available()) {
      /* read the file and print to Terminal */
      Serial.write(root.read());
    }
    root.close();
  } else {
    Serial.println("error reading file");
  }
}

void printDirectory(File dir, int numTabs) {
  
  while(true) {
     File entry =  dir.openNextFile();
     if (! entry) {
       break;
     }
     for (uint8_t i=0; i<numTabs; i++) {
       Serial.print('\t');   // we'll have a nice indentation
     }
     // Print the name
     Serial.print(entry.name());
     /* Recurse for directories, otherwise print the file size */
     if (entry.isDirectory()) {
       Serial.println("/");
       printDirectory(entry, numTabs+1);
     } else {
       /* files have sizes, directories do not */
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
  Serial.println("Could not find a valid BMP sensor, check wiring!");
  while (1) {}
  }
  
  //Escrever no arquivo test.txt os parâmetros e as unidades de medida  
  writeFile("test.txt", "      Tempo ");
  writeFile("test.txt", "      RPM ");
  writeFile("test.txt", "      WOW  ");
  writeFile("test.txt", "        VCAS  ");
  writeFile("test.txt", "      MagHead ");
  writeFile("test.txt", "      ELEV  ");
  writeFile("test.txt", "      AIL  ");
  writeFile("test.txt", "      RUD  ");
  writeFile("test.txt", "      HP  ");
  writeFile("test.txt", "            NZ  ");
  writeFile("test.txt", "      THETA ");
  writeFile("test.txt", "      PHI ");
  writeFile("test.txt", "          XGPS  ");
  writeFile("test.txt", "           YGPS  ");
  writeFile("test.txt", "        ZGPS  \r\n");
  
  writeFile("test.txt", "    [segundos] ");
  writeFile("test.txt", "  [RPM] ");
  writeFile("test.txt", "    [bit] ");
  writeFile("test.txt", "        [m/s] ");
  writeFile("test.txt", "       [deg] ");
  writeFile("test.txt", "       [deg] ");
  writeFile("test.txt", "     [deg] ");
  writeFile("test.txt", "     [deg] ");
  writeFile("test.txt", "     [ft] ");
  writeFile("test.txt", "            [g] ");
  writeFile("test.txt", "      [deg] ");
  writeFile("test.txt", "     [deg] ");
  writeFile("test.txt", "         [m] ");
  writeFile("test.txt", "             [m] ");
  writeFile("test.txt", "          [m] \r\n");

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

  // Low-pass filter
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
  HP = bmp.readAltitude(101800);

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
  
  //Escrever no arquivo test.txt o valor de cada parâmetro
  writeFile("test.txt", "     ");
  writeFile("test.txt", String(tempo).c_str());
  writeFile("test.txt", "    ");
  writeFile("test.txt", String(RPM).c_str());
  writeFile("test.txt", "        ");
  writeFile("test.txt", String(WOW).c_str());
  writeFile("test.txt","         ");
  writeFile("test.txt", String(velocidademps).c_str());
  writeFile("test.txt", "        ");
  writeFile("test.txt", String(MagBow).c_str());
  writeFile("test.txt", "         ");
  writeFile("test.txt", String(elev).c_str());
  writeFile("test.txt", "         ");
  writeFile("test.txt", String(ail).c_str());
  writeFile("test.txt", "         ");
  writeFile("test.txt", String(rud).c_str());
  writeFile("test.txt", "         ");
  writeFile("test.txt", String(HP).c_str());
  writeFile("test.txt", "      ");
  writeFile("test.txt", String(nz).c_str());
  writeFile("test.txt", "      ");
  writeFile("test.txt", String(pitch).c_str());
  writeFile("test.txt", "       ");
  writeFile("test.txt", String(roll).c_str());
  writeFile("test.txt", "        ");
  writeFile("test.txt", String(xgps,6).c_str());
  writeFile("test.txt", "       ");
  writeFile("test.txt", String(ygps,6).c_str());
  writeFile("test.txt", "      ");
  writeFile("test.txt", String(zgps).c_str());
  writeFile("test.txt", "\r\n");
  readFile("test.txt"); 
  delayMicroseconds(1000000);
}
