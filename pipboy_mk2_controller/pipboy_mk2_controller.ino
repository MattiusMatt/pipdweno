#include <Adafruit_GPS.h>
#include <Adafruit_FONA.h>
#include <Encoder.h>
#include <Wire.h>
#include <SoftwareSerial.h>

// Comment here when I know what this is for??
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

// ASCII Consts
#define ASCII_CR 13
#define ASCII_LF 10
#define ASCII_PIPE 124
#define ASCII_TILDE 126
#define ASCII_B 66
#define ASCII_M 77

// Fona Pins
//#define FONA_RX 2
//#define FONA_TX 3
#define FONA_RST 5

// GPS
#define GPS_SCREEN 3
#define GPS_ECHO false

// Rotary Encoder
#define ENCODER_BUTTON 2
#define ENCODER_A 18
#define ENCODER_B 19

// Audio Pin
#define AUDIO_TMR 11

// Radio
#define RADIO_SCREEN 4

// Map Screen
#define MAP_SCREEN 3
#define MAP_POSX 10
#define MAP_POSY 25
#define MAP_WIDTH 300
#define MAP_HEIGHT 195
#define MAP_POS_WIDTH 17
#define MAP_POS_HEIGHT 24
#define PIXEL_TILE_SIZE 256.000
//#define DEGREES_TO_RADIANS_RATIO 180.000 / PI
#define RADIANS_TO_DEGREES_RATIO PI / 180.000
#define ZOOM_LOCAL 15
#define ZOOM_WORLD 9
const char MAP_LOCAL[] = "LOCAL.BMP";
const char MAP_WORLD[] = "WORLD.BMP";

// Buttons
#define BUTTON_ONE 7

// Fona
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint8_t fona_type;
//#define AUDIO_OUTPUT FONA_HEADSETAUDIO
#define AUDIO_OUTPUT FONA_EXTAUDIO

// GPS
#define GPS_RX 2
#define GPS_TX 3

SoftwareSerial gpsSerial(GPS_TX, GPS_RX);
Adafruit_GPS gps(&gpsSerial);
String map_local_lat = "";
String map_local_lon = "";
String map_world_lat = "";
String map_world_lon = "";

// Radio
#define RADIO_MAXVOL 6
int radioVolume = 6;

// Timers
#define GPS_UPDATE 2
#define LOC_UPDATE 15
#define STA_UPDATE 30

// Encoder
Encoder encoder(ENCODER_A, ENCODER_B);

// Sound

// Initialisation

void setup() {
  // Debug using serial
  Serial.begin(115200);

  if (!enableFona()) { return; }

  if (!enableAudio()) { return; }

  if (!enableGPS()) { return; }

  if (!enableRadio()) { return; }

  // Encoder Button
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);

  // Buttons
  pinMode(BUTTON_ONE, INPUT_PULLUP);

  loadMapCentres();

  randomSeed(millis());

  /*if (!fona.sendSMS("07734264377", "PipBoy Booted!")) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
  }*/
}

bool enableFona() {
  // Fona
  Serial.println(F("Initialising FONA"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  // Hard reset the fona
  Serial.println(F("Resetting the fona"));
  pinMode(FONA_RST, OUTPUT);
  digitalWrite(FONA_RST, LOW);
  delay(200);
  digitalWrite(FONA_RST, HIGH);

  fonaSerial->begin(4800);
  
  if (!fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    return false;
  }
  
  fona_type = fona.type();
  
  Serial.println(F("FONA is OK"));
  Serial.print(F("Found "));
  
  switch (fona_type) {
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
  fona.setAudio(AUDIO_OUTPUT);
  fona.setMicVolume(AUDIO_OUTPUT, 10);

  // Test Audio
  //fona.playToolkitTone(2, 1000);

  return true;
}

/*
bool enableAudio() {
  Serial.println(F("Initialising Audio"));
  
  audio.speakerPin = AUDIO_TMR;
  audio.setVolume(6);
  audio.quality(1);

  return true;
}
*/

bool enableGPS() {
  Serial.println(F("Initialising GPS"));
  
  gps.begin(9600);
  gps.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  gps.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  gps.sendCommand(PGCMD_ANTENNA);

  // GPS Interupt
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);

  return true;
}

bool enableRadio() {
  Serial.println(F("Initialising Radio"));

  if (fona.FMradio(false, AUDIO_OUTPUT)) {
    Serial.println(F("Radio enabled"));
  }
  
  fona.setFMVolume(radioVolume);

  return true;
}

// Running

// GPS Interupt
SIGNAL(TIMER0_COMPA_vect) {
  char c = gps.read();
  
  if (GPS_ECHO)
  {
    if (c) { UDR0 = c; }
    // writing direct to UDR0 is much much faster than Serial.print 
    // but only one character can be written at a time. 
  }
}

// Serial Vars
bool writePipMode = false;

// Main Screen Vars
int currentScreen = -1;

// Second Screen Vars
long currentSubScreen = 0;

// Encoder Mode
int previousButtonValue = 1;
bool menuMode = false;

// Current Menu Option
long currentMenuOption = 0;
int menuOffset = 0;

// GPS Timer
uint32_t gps_timer = millis();
uint32_t loc_timer = millis();
uint32_t status_timer = millis();

bool reloadLocation = true;
bool reloadGpsImage = true;

void loop() {
  // Main Screen
  int newScreen = readMainSwitch();

  if (newScreen != currentScreen) {
    delay(500);

    newScreen = readMainSwitch();

    fona.FMradio(false);
    
    switch(newScreen){
      case 0:
        //loadPip("0.pip", true);
        break;
        
      case 1:
        //loadPip("1.pip", true);
        break;
        
      case 2:
        //loadPip("2.pip", true);
        break;
        
      case 3:
        //loadPip("3.pip", true);

        reloadGpsImage = true;
        
        break;
        
      case 4:
        //loadPip("4.pip", true);

        // Radio
        fona.FMradio(true, AUDIO_OUTPUT);
        
        break;
    }
    
    currentScreen = newScreen;

    // Reset Sub Screen
    currentSubScreen = 0;
    encoder.write(0);

    play("tab.wav");

    int playStatic = random(3);

    if (playStatic == 0) {
      play("static0.wav");
    }

    // Reset Menu Option
    currentMenuOption = 0;
    menuOffset = 0;
    menuMode = false;
  }

  // Button Control
  int buttonVal = digitalRead(BUTTON_ONE);

  if (buttonVal == LOW) {
    delay(10);
    buttonVal = digitalRead(BUTTON_ONE);

    if (buttonVal == LOW && currentScreen == MAP_SCREEN && gps.fix) {
      if (menuMode) {
        downloadMap(true, "53.5049", "-2.0154");
      } else {
        downloadMap(false, "53.5049", "-2.0154");
      }
    }
  }

  // Menu Mode Toggle
  int val = digitalRead(ENCODER_BUTTON);

  if (val != previousButtonValue) {
    delay(10);
    val = digitalRead(ENCODER_BUTTON);
    
    if (val == LOW) {
      menuMode = !menuMode;

      if (!menuMode) {
        Serial.print("Flange: ");
        Serial.println(currentSubScreen);
        encoder.write(currentSubScreen * 2);
      } else {
        encoder.write(currentMenuOption * 2);
      }

      Serial.print("Menu Mode: ");
      Serial.println(menuMode);

      // Toggle local / world map
      reloadGpsImage = true;
    }

    previousButtonValue = val;
  }

  // Sub Screen
  long newEncoderValue = encoder.read() / 2;

  if (!menuMode && newEncoderValue != currentSubScreen) {
    delay(500);

    newEncoderValue = encoder.read() / 2;

    Serial.println();
    Serial.print("New Encoder Value: ");
    Serial.println(newEncoderValue);

    play("tab.wav");
  }

  // Menu
  if (menuMode && newEncoderValue != currentMenuOption) {
    delay(500);

    newEncoderValue = encoder.read() / 2;

    Serial.println();
    Serial.print("New Encoder Value: ");
    Serial.println(newEncoderValue);

    play("scroll.wav");
  }

  // GPS
  if (gps.newNMEAreceived()) {
    if (!gps.parse(gps.lastNMEA())) {
      return;
    }
  }

  // Map
  if (currentScreen == GPS_SCREEN) {
    if (reloadGpsImage) {
      if (!gps.fix) {
        if (menuMode) {
          //drawPosition(map_local_lat.toDouble(), map_local_lon.toDouble(), map_local_lat.toDouble(), map_local_lon.toDouble(), ZOOM_LOCAL, 0);
        } else {
          //drawPosition(map_world_lat.toDouble(), map_world_lon.toDouble(), map_world_lat.toDouble(), map_world_lon.toDouble(), ZOOM_LOCAL, 0);
        }
      } else {
        loc_timer = millis();
        reloadLocation = true;
      }
      
      reloadGpsImage = false;
    }
  
    if (loc_timer > millis())  loc_timer = millis();
  
    if (millis() - loc_timer > LOC_UPDATE * 1000) {
      loc_timer = millis();
      reloadLocation = true;
    }
  
    if (gps_timer > millis())  gps_timer = millis();
  
    // approximately every 2 seconds or so, print out the current stats
    if (millis() - gps_timer > GPS_UPDATE * 1000) { 
      gps_timer = millis(); // reset the gps_timer
      
      if (gps.fix) {
        if (reloadLocation) {
          if (menuMode) {
            drawPosition(map_local_lat.toDouble(), map_local_lon.toDouble(), gps.latitudeDegrees, gps.longitudeDegrees, ZOOM_LOCAL, gps.angle);
          } else {
            drawPosition(map_world_lat.toDouble(), map_world_lon.toDouble(), gps.latitudeDegrees, gps.longitudeDegrees, ZOOM_WORLD, gps.angle);
          }
  
          reloadLocation = false;
        }
        
        Serial.print("Fix: "); Serial.print((int)gps.fix);
        Serial.print(" quality: "); Serial.println((int)gps.fixquality);
        Serial.print("Location: ");
        Serial.print(gps.latitude, 4); Serial.print(gps.lat);
        Serial.print(", "); 
        Serial.print(gps.longitude, 4); Serial.println(gps.lon);
        Serial.print("Location (in degrees, works with Google Maps): ");
        Serial.print(gps.latitudeDegrees, 4);
        Serial.print(", "); 
        Serial.println(gps.longitudeDegrees, 4);
        
        Serial.print("Speed (knots): "); Serial.println(gps.speed);
        Serial.print("Angle: "); Serial.println(gps.angle);
        Serial.print("Altitude: "); Serial.println(gps.altitude);
        Serial.print("Satellites: "); Serial.println((int)gps.satellites);
      }
      
      // Print Time
      int hour = gps.hour;
      int minute = gps.minute;
      bool pm = false;
      
      if (hour > 12) {
        hour -= 12;
        pm = true;
      }
    }
  }

  // Status Bar
  if (status_timer > millis())  status_timer = millis();
  
  if (millis() - status_timer > STA_UPDATE * 1000) {
    // Battery
    drawBatt(false);
    
    status_timer = millis();
  }
}

// Helper functions

// Status Bar
void drawBatt(bool fromScratch) {
  uint16_t vbat;

  if (!fona.getBattPercent(&vbat)) {
    Serial.println(F("Failed to read Batt"));
  } else {
    Serial.print(F("VPct = ")); Serial.print(vbat); Serial.println(F("%"));
  }
}

//Sound
void play(const char file[]) {
  audio.play((char *)file);

  while (audio.isPlaying()) {
  }

  audio.disable();
}

int readMainSwitch() {
  int sensorValue = analogRead(A7);

  //Serial.print("Val: ");
  //Serial.println(sensorValue);

  // Main Rotary Switch
  if (sensorValue > 200 && sensorValue < 230) { return 0; }
  else if (sensorValue > 250 && sensorValue < 280) { return 1; }
  else if (sensorValue > 340 && sensorValue < 360) { return 2; }
  else if (sensorValue > 510 && sensorValue < 530) { return 3; }
  else if (sensorValue > 1000 && sensorValue < 1050) { return 4; }

  return currentScreen;
}

// Map Coords Conversion
double pixelGlobeCentre;
double xPixelsToDegreesRatio;
double yPixelsToRadiansRatio;

void calibrateMapScale(int zoomLevel) {
  double pixelGlobeSize = PIXEL_TILE_SIZE * pow(2.000, zoomLevel);
  xPixelsToDegreesRatio = pixelGlobeSize / 360.000;
  yPixelsToRadiansRatio = pixelGlobeSize / (2.000 * PI);

  float halfPixelGlobeSize = (float)(pixelGlobeSize / 2.000);
  pixelGlobeCentre = halfPixelGlobeSize;
}

float convertLatToXPos(double lat) {
  float xPos = (float)round(pixelGlobeCentre + (lat * xPixelsToDegreesRatio));

  Serial.print("XPixels: ");
  Serial.println(xPos);

  return xPos;
}

float convertLonToYPos(double lon) {
  double f = min(max(sin(lon * (RADIANS_TO_DEGREES_RATIO)), -0.9999), 0.9999);

  Serial.print("F: ");
  Serial.println(f);

  float yPos = (float)round((pixelGlobeCentre) + 0.5 * log((1 + f) / (1 -f)) * -yPixelsToRadiansRatio);

  Serial.print("YPixels: ");
  Serial.println(yPos);

  return yPos;
}

void drawPosition(double centreLat, double centreLon, double posLat, double posLon, int zoomLevel, int angle) {
  calibrateMapScale(zoomLevel);
  
  Serial.print("PixelTileSize: ");
  Serial.println(PIXEL_TILE_SIZE);
  Serial.print("RadiansToDegreesRatio: ");
  Serial.println(RADIANS_TO_DEGREES_RATIO);
  Serial.print("XPixelsToDegreesRatio: ");
  Serial.println(xPixelsToDegreesRatio);
  Serial.print("YPixelsToRadiansRatio: ");
  Serial.println(yPixelsToRadiansRatio);
  Serial.print("PixelGlobeCentre: ");
  Serial.println(pixelGlobeCentre);
  
  Serial.print("Centre: ");
  Serial.print(centreLat);
  Serial.print(" ");
  Serial.print(centreLon);
  Serial.print(" ");
  Serial.println(zoomLevel);
  
  float centreX = convertLatToXPos(centreLat);
  float centreY = convertLonToYPos(centreLon);

  Serial.print("Pos: ");
  Serial.print(posLat);
  Serial.print(" ");
  Serial.print(posLon);
  Serial.print(" ");
  Serial.println(zoomLevel);
  
  float posX = convertLatToXPos(posLat);
  float posY = convertLonToYPos(posLon);

  float pixelDistanceX = centreX - posX;
  float pixelDistanceY = centreY - posY;

  float centreImageX = MAP_WIDTH / 2;
  float centreImageY = MAP_HEIGHT / 2;

  float centreImagePosX = MAP_POS_WIDTH / 2;
  float centreImagePosY = MAP_POS_HEIGHT / 2;
  
  int locationX = ((centreImageX - pixelDistanceX) - centreImagePosX) + MAP_POSX;
  int locationY = ((centreImageY - pixelDistanceY) - centreImagePosY) + MAP_POSY;

  float rad = (PI / 180) * angle;
  int locationRadius = 8;
  
  int x0 = locationX;
  int y0 = locationY - locationRadius * 2;
  int x1 = locationX - locationRadius;
  int y1 = locationY + locationRadius;
  int x2 = locationX + locationRadius;
  int y2 = locationY + locationRadius;

  x0 = x0 - locationX;
  y0 = y0 - locationY;
  x1 = x1 - locationX;
  y1 = y1 - locationY;
  x2 = x2 - locationX;
  y2 = y2 - locationY;

  int tx0 = (x0 * cos(rad)) - (y0 * sin(rad));
  int ty0 = (x0 * sin(rad)) + (y0 * cos(rad));
  int tx1 = (x1 * cos(rad)) - (y1 * sin(rad));
  int ty1 = (x1 * sin(rad)) + (y1 * cos(rad));
  int tx2 = (x2 * cos(rad)) - (y2 * sin(rad));
  int ty2 = (x2 * sin(rad)) + (y2 * cos(rad));

  x0 = tx0 + locationX;
  y0 = ty0 + locationY;
  x1 = tx1 + locationX;
  y1 = ty1 + locationY;
  x2 = tx2 + locationX;
  y2 = ty2 + locationY;
}

// Map Download
bool mapDownloading;
bool startOfBitmapFound;
bool endOfBitmapFound;

void loadMapCentres() {
  loadMapCentre(true);
  loadMapCentre(false);
}

void loadMapCentre(bool local) {
  File coordsFile;
  String lat;
  String lon;

  if (local) {
    coordsFile = SD.open("local.txt");
  } else {
    coordsFile = SD.open("world.txt");
  }

  if (coordsFile) {
    bool latFound = false;
    
    while (coordsFile.available()) {
      if (!latFound) {
        lat.concat(char(coordsFile.read()));
      } else {
        lon.concat(char(coordsFile.read()));
      }
      
      if (coordsFile.peek() == ASCII_PIPE) {
        latFound = true;
        coordsFile.read(); // Om nom nom nom
      }
    }
    
    coordsFile.close();
  }

  if (local) {
    map_local_lat = lat;
    map_local_lon = lon;
  } else {
    map_world_lat = lat;
    map_world_lon = lon;
  }
}

void downloadMap(bool localMap, const char lat[], const char lon[]) {
  if (!mapDownloading) {
    Serial.println(F("Attempting Download"));

    const char *imageName;

    if (localMap) {
      imageName = MAP_LOCAL;
    } else {
      imageName = MAP_WORLD;
    }

    // Blank out Map  
    startOfBitmapFound = false;
    endOfBitmapFound = false;
  
    fona.enableGPRS(true);
  
    atResponse(1000);

    // Build Url Command
    String urlCommand = "AT+HTTPPARA=\"URL\",\"http://mattius.no-ip.org:7507/";

    if (localMap) {
      urlCommand.concat("local?lat=");
      //atCommand("AT+HTTPPARA=\"URL\",\"http://mattius.no-ip.org:7507/local?lat=53.5049&lon=-2.0154\"");
    } else {
      urlCommand.concat("world?lat=");
      //atCommand("AT+HTTPPARA=\"URL\",\"http://mattius.no-ip.org:7507/world?lat=53.5049&lon=-2.0154\"");
    }
    
    urlCommand.concat(lat);
    urlCommand.concat("&lon=");
    urlCommand.concat(lon);
    urlCommand.concat("\"");
    
    char urlCommandChars[urlCommand.length()];
    urlCommand.toCharArray(urlCommandChars, urlCommand.length());
  
    atCommand("AT+HTTPTERM");
    atCommand("AT+HTTPINIT");
    atCommand("AT+HTTPPARA=\"CID\",1");
    atCommand(urlCommandChars);
    atCommand("AT+HTTPPARA=\"BREAK\",2000");
    atCommand("AT+HTTPACTION=0");
    
    delay(1000);
    
    // wait for the download
    Serial.print(atResponse(30000));

    mapDownloading = true;
    
    fona.println("AT+HTTPREAD");

    downloadMap_Resume(localMap, imageName, lat, lon);
  }
}

void downloadMap_Resume(bool localMap, const char imageName[], const char lat[], const char lon[]) {
  if (mapDownloading) {
    Serial.println("Open image");
    
    File imgWriter = SD.open(imageName, FILE_WRITE);
    
    while (mapDownloading) {
      mapDownloading = atResponseToFile(imgWriter);
    }
  
    imgWriter.close();

    delay(1000);

    Serial.println("Close image");

    if (!mapDownloading) {
      atCommand("AT+HTTPTERM");

      File locationWriter;
      
      if (localMap) {
        SD.remove("local.txt");
        locationWriter = SD.open("local.txt", FILE_WRITE);

        map_local_lat = lat;
        map_local_lon = lon;
      } else {
        SD.remove("world.txt");
        locationWriter = SD.open("world.txt", FILE_WRITE);

        map_world_lat = lat;
        map_world_lon = lon;
      }

      locationWriter.print(lat);
      locationWriter.print('|');
      locationWriter.print(lon);

      locationWriter.close();
    }
  }
}

void atCommand(const char command[]) {
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

bool atResponseToFile(File &file) {
  uint8_t data;

  while (!fona.available()) {
    delay(100);
  }
  
  while (fona.available()) {
    data = fona.read();

    if (data == ASCII_B) {
      if (fona.peek() == ASCII_M) {
        startOfBitmapFound = true;
      }
    }

    if (startOfBitmapFound) {
      if (data == ASCII_CR) {
        if (fona.peek() == ASCII_LF) {
          endOfBitmapFound = true;
        }
      }

      if (!endOfBitmapFound) {
        file.write(data);
      }
    }
    
    Serial.print((char)data);
  }

  return !endOfBitmapFound;
}
