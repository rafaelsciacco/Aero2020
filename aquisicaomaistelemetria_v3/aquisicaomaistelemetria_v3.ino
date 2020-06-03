int RPM, pitch, roll, latitude, longitude, altitudeGPS, elev, ail ,rud = 1;
float rollF = 1;
float pitchF = 2;
float nz = 3;
float MagBow = 4;
float HP = 5;
float WOW = 6;
float velocidademps = 7;

//Bibliotecas
#include <mySD.h> //SD
#include <SPI.h> //SD e NRF
#include <nRF24L01.h> //NRF
#include <RF24.h> //NRF
#include <RTClib.h> //RTC

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

//microSD
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

//Variaveis tempo
float tempo  = 0;
RTC_DS1307 rtc;



void setup()
{
  Serial.begin(115200);

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
  //rtc.adjust(DateTime(2020, 6, 3, 13, 46, 0));  // (Ano,mês,dia,hora,minuto,segundo)

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
  /*if (latitude != TinyGPS::GPS_INVALID_F_ANGLE) {
    writeFile("test.txt", "        ");
    writeFile("test.txt", String(latitude,6).c_str());
  }
  if (longitude != TinyGPS::GPS_INVALID_F_ANGLE) {
    writeFile("test.txt", "       ");
    writeFile("test.txt", String(longitude,6).c_str());
  }
  if ((altitudeGPS != TinyGPS::GPS_INVALID_ALTITUDE) && (altitudeGPS != 1000000)) {
    writeFile("test.txt", "      ");
    writeFile("test.txt", String(altitudeGPS).c_str());
  }*/
  writeFile("test.txt", "\r\n");
  readFile("test.txt"); 
  delayMicroseconds(1000000);
}
