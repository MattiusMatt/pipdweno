#include "Adafruit_FONA.h"

//#define FONA_RX 2
//#define FONA_TX 3
#define FONA_RST 5

// ASCII Consts
#define ASCII_CR 13
#define ASCII_LF 10

// this is a large buffer for replies
char replybuffer[255];

// Hardware serial is also possible!
HardwareSerial *fonaSerial = &Serial2;

// Use this for FONA 800 and 808s
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

uint8_t type;

void setup() {
  while (!Serial);

  Serial.begin(115200);
  Serial.println(F("FONA basic test"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  // Hard reset the fona
  Serial.println(F("Resetting the fona"));
  pinMode(FONA_RST, OUTPUT);
  digitalWrite(FONA_RST, LOW);
  delay(200);
  digitalWrite(FONA_RST, HIGH);

  fonaSerial->begin(4800);
  
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while (1);
  }
  
  type = fona.type();
  
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  
  switch (type) {
    case FONA800L:
      Serial.println(F("FONA 800L")); break;
    case FONA800H:
      Serial.println(F("FONA 800H")); break;
    case FONA808_V1:
      Serial.println(F("FONA 808 (v1)")); break;
    case FONA808_V2:
      Serial.println(F("FONA 808 (v2)")); break;
    case FONA3G_A:
      Serial.println(F("FONA 3G (American)")); break;
    case FONA3G_E:
      Serial.println(F("FONA 3G (European)")); break;
    default: 
      Serial.println(F("???")); break;
  }
  
  // Print module IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }

  fona.setGPRSNetworkSettings(F("idata.o2.co.uk"), F("vertigo"), F("password"));

  /*if (!fona.sendSMS("07734264377", "I'm Alive!")) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
  }*/

  if (!fona.enableGPRS(true))
    Serial.println(F("Failed to turn on"));

  delay(1000);
  
  Serial.println(F("Attempting Download"));

  atCommand("AT+HTTPTERM");
  atCommand("AT+HTTPINIT");
  atCommand("AT+HTTPPARA=\"CID\",1");
  //atCommand("AT+HTTPPARA=\"URL\",\"http://mattius.no-ip.org:7507/local?lat=53.486050&lon=-2.242141\"");
  atCommand("AT+HTTPPARA=\"URL\",\"http://mattius.no-ip.org:7507/test\"");
  //atCommand("AT+HTTPPARA=\"BREAK\",2000");
  atCommand("AT+HTTPACTION=0");
  // wait for the download
  Serial.print(atResponse(30000));
  atCommand("AT+HTTPREAD");

  while (1) {
    String response = atResponse(1000);

    Serial.print(response);

    if (response.length() == 0) {
      break;
    }
  }

  atCommand("AT+HTTPTERM");
}

void loop() {
}

void atCommand(char *command) {
  Serial.println(command);
  fona.println(command);
  
  Serial.print(atResponse(1000));
}

String atResponse(int maxWait) {
  String line;
  int currentWait = 0;
  bool maxWaitReached = false;

  while (line.length() == 0) {
    while (!fona.available()) {
      delay(100);
      currentWait += 100;

      if (currentWait > maxWait) {
        maxWaitReached = true;
        break;
      }
    }

    if (maxWaitReached) {
      break;
    }
    
    while (fona.available()) {
      char c = fona.read();

      line += c;
      currentWait = 0;
    }
  }

  return line;
}

