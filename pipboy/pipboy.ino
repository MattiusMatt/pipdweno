#include <Adafruit_GPS.h>
#include <Adafruit_ILI9340.h>
#include <Adafruit_GFX.h>
#include <Adafruit_FONA.h>
#include <gfxfont.h>
#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
#include <Encoder.h>
#include <Wire.h>

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

// TFT Pins
// pin 11 (51) = MOSI, pin 12 (50) = MISO, pin 13 (52) = SCK
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 12

// SD Pin
#define SD_CS 4

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

// PIP Colours
#define PIP_GREEN 2016
#define PIP_GREEN_2 800
#define PIP_GREEN_3 416

// Buttons
#define BUTTON_ONE 7

// Fona
HardwareSerial *fonaSerial = &Serial2;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
uint8_t fona_type;
//#define AUDIO_OUTPUT FONA_HEADSETAUDIO
#define AUDIO_OUTPUT FONA_EXTAUDIO

// TFT
Adafruit_ILI9340 tft = Adafruit_ILI9340(TFT_CS, TFT_DC, TFT_RST);

// GPS
Adafruit_GPS gps(&Serial3);
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
TMRpcm audio;

// Initialisation

void setup() {
  // Debug using serial
  Serial.begin(115200);

  if (!enableSD()) { return; }

  if (!enableFona()) { return; }

  if (!enableAudio()) { return; }

  if (!enableGPS()) { return; }

  if (!enableRadio()) { return; }

  if (!enableTFT()) { return; }

  // Encoder Button
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);

  // Buttons
  pinMode(BUTTON_ONE, INPUT_PULLUP);
  
  runLoadSequence();

  loadMapCentres();

  randomSeed(millis());

  /*if (!fona.sendSMS("07734264377", "PipBoy Booted!")) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("Sent!"));
  }*/
}

bool enableSD() {
  Serial.print(F("Initialising SD card..."));
  
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD failed!"));
    return false;
  }
  
  Serial.println(F("SD Initialised!"));

  return true;
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

bool enableAudio() {
  Serial.println(F("Initialising Audio"));
  
  audio.speakerPin = AUDIO_TMR;
  audio.setVolume(6);
  audio.quality(1);

  return true;
}

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

bool enableTFT() {
  Serial.println(F("Initialising TFT"));
  
  // TFT
  tft.begin();
  tft.setRotation(3);
  tft.setTextColor(ILI9340_GREEN);
  tft.setTextSize(1);

  return true;
}

void runLoadSequence() {
  Serial.println(F("Running load sequence"));
  
  // Splash 0
  tft.fillScreen(ILI9340_BLACK);
  loadText("0.txt", 0, 0, 0);
  delay(1000);

  // Loading
  tft.fillScreen(ILI9340_BLACK);
  loadText("1.txt", 0, 0, 20);
  //Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  delay(1000);

  // Vault Boy Loading
  tft.fillScreen(ILI9340_BLACK);
  bmpDraw("l.bmp", 95, 35);
  delay(1000);
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
        loadPip("0.pip", true);
        break;
        
      case 1:
        loadPip("1.pip", true);
        break;
        
      case 2:
        loadPip("2.pip", true);
        break;
        
      case 3:
        loadPip("3.pip", true);

        reloadGpsImage = true;
        
        break;
        
      case 4:
        loadPip("4.pip", true);

        // Radio
        fona.FMradio(true, AUDIO_OUTPUT);
        loadMenuData(0);
        
        break;
    }
    
    currentScreen = newScreen;

    if (currentScreen == GPS_SCREEN) {
      tft.fillRect(5, 225, 310, 8, PIP_GREEN_3);
    }

    // Reset Sub Screen
    currentSubScreen = 0;
    encoder.write(0);
    drawSubScreen(currentSubScreen, true);

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

    int newSubScreen = drawSubScreen(newEncoderValue, false);

    play("tab.wav");

    if (newSubScreen != currentSubScreen) {
      currentSubScreen = newSubScreen;
      currentMenuOption = 0;
      renderSubMenu(currentMenuOption);
    } else {
      encoder.write(newSubScreen * 2);
    }
  }

  // Menu
  if (menuMode && newEncoderValue != currentMenuOption) {
    delay(500);

    newEncoderValue = encoder.read() / 2;

    Serial.println();
    Serial.print("New Encoder Value: ");
    Serial.println(newEncoderValue);

    int newMenu = updateMenuOptions(newEncoderValue, currentMenuOption);

    play("scroll.wav");

    currentMenuOption = newMenu;
    renderSubMenu(newMenu);
    
    if (newMenu != newEncoderValue) {
      encoder.write(newMenu * 2);
    }
  }
  
  // Serial control
  if (writePipMode) {
    writePipMode = !writePip(true);
    
    if (!writePipMode) {
      loadPip("t.pip", true);
    }
  } else {
    if (Serial.available()) {
      switch(Serial.read()){
        case 'u':
          writePipMode = !writePip(false);
          break;
          
        default:
          break;
      }
    }
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
          bmpDraw(MAP_LOCAL, MAP_POSX, MAP_POSY);
          //drawPosition(map_local_lat.toDouble(), map_local_lon.toDouble(), map_local_lat.toDouble(), map_local_lon.toDouble(), ZOOM_LOCAL, 0);
        } else {
          bmpDraw(MAP_WORLD, MAP_POSX, MAP_POSY);
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
  
      // Toolbar
      tft.setTextColor(PIP_GREEN, PIP_GREEN_3);
      
      tft.setTextSize(1);
      tft.setCursor(5, 225);
  
      tft.print(' ');
      tft.print(gps.latitudeDegrees, 4);
      tft.print(", "); 
      tft.print(gps.longitudeDegrees, 4);
      tft.print(' ');
  
      tft.setTextColor(PIP_GREEN, ILI9340_BLACK);
      tft.print(' ');
      tft.setTextColor(PIP_GREEN, PIP_GREEN_3);
      
      if (gps.fix) {
        if (reloadLocation) {
          if (menuMode) {
            bmpDraw(MAP_LOCAL, MAP_POSX, MAP_POSY);
            drawPosition(map_local_lat.toDouble(), map_local_lon.toDouble(), gps.latitudeDegrees, gps.longitudeDegrees, ZOOM_LOCAL, gps.angle);
          } else {
            bmpDraw(MAP_WORLD, MAP_POSX, MAP_POSY);
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
      
      tft.print(' ');
      if (hour < 10) { tft.print('0'); tft.print(hour, DEC); } else { tft.print(hour, DEC); };
      tft.print(':');
      if (minute < 10) { tft.print('0'); tft.print(minute, DEC); } else { tft.print(minute, DEC); };
      if (pm) { tft.print(" PM "); } else { tft.print(" AM "); };
  
      tft.setTextColor(PIP_GREEN, ILI9340_BLACK);
      tft.print(' ');
  
      // Print Local / World Map
      tft.setCursor(248, 225);
      tft.setTextColor(ILI9340_BLACK, PIP_GREEN_2);
  
      if (menuMode) {
        tft.print(" LOCAL MAP ");
      } else {
        tft.print(" WORLD MAP ");
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
  
    int batWidth = round(vbat / 10) * 2;

    if (fromScratch) {
      tft.drawFastVLine(287, 10, 5, PIP_GREEN);
      tft.drawFastHLine(287, 10, 3, PIP_GREEN);
      tft.drawFastHLine(287, 15, 3, PIP_GREEN);
      tft.drawRect(291, 8, 20, 10, PIP_GREEN);
    }
    
    tft.fillRect(292, 9, 18, 8, ILI9340_BLACK);
    tft.fillRect(291 + (20 - batWidth), 8, batWidth, 10, PIP_GREEN);
  }
}

// Menu Screen
#define MAX_MENU_OPTIONS 20
#define MAX_MENU_DISPLAY 8
#define MENU_ITEM_HEIGHT 20
#define MENU_ITEM_WIDTH 130
#define MENU_START_X 20
#define MENU_START_Y 55

String menuOptions[MAX_MENU_OPTIONS];
String menuOptionsData[MAX_MENU_OPTIONS];
int noOfMenuOptions;

void loadMenuOptions(File &pip, uint16_t x, uint16_t y) {
  uint8_t currentMenuText = 0;
  
  Serial.println("Loading Sub Menus...");

  for (int i = 0; i < MAX_MENU_OPTIONS; i++) {
    menuOptions[i] = "";
    menuOptionsData[i] = "";
  }

  String data;
  bool readData = false;
  
  while (pip.available()) {
    int character = pip.read();
    
    if (character == ASCII_CR) {
      pip.read(); // Om nom nom
      break;
    }

    if (character == ASCII_TILDE) {
      character = pip.read();
      readData = true;
    }
    
    if (character == ASCII_PIPE) {
      character = pip.read();
      data = "";
      readData = false;
      currentMenuText++;
      Serial.println();
    }

    if (!readData) {
      menuOptions[currentMenuText].concat(char(character));
    } else {
      menuOptionsData[currentMenuText].concat(char(character));
    }

    Serial.print(char(character));
  }

  noOfMenuOptions = currentMenuText + 1;

  Serial.println();
  Serial.print("No of Menu Options: ");
  Serial.println(noOfMenuOptions);
}

void drawMenuOptions(int current) {
  if (noOfMenuOptions == 0) { return; }
  
  int menu_x = MENU_START_X;
  int menu_y = MENU_START_Y;
  
  tft.setTextSize(1);

  int renderItems;

  if (noOfMenuOptions < MAX_MENU_DISPLAY) {
    renderItems = noOfMenuOptions;
  } else {
    renderItems = MAX_MENU_DISPLAY;
  }

  for (uint8_t i = menuOffset; i < renderItems + menuOffset; i++) {
    if (i == current) {
      tft.fillRect(menu_x - 5, menu_y - 5, MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT - 2, PIP_GREEN);
      tft.setTextColor(ILI9340_BLACK, PIP_GREEN);
    } else {
      tft.fillRect(menu_x - 5, menu_y - 5, MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT - 2, ILI9340_BLACK);
      tft.setTextColor(PIP_GREEN, ILI9340_BLACK);
    }
    
    tft.setCursor(menu_x, menu_y);
    tft.print(menuOptions[i]);

    menu_y += MENU_ITEM_HEIGHT;
  }
}

int updateMenuOptions(int newMenu, int previousMenu) {
  if (newMenu < 0) { newMenu = 0; }
  if (newMenu > noOfMenuOptions - 1) { newMenu = noOfMenuOptions - 1; }

  if (newMenu == previousMenu) { return newMenu; }

  int newMenuOffset = newMenu - (MAX_MENU_DISPLAY - 1);

  if (newMenuOffset < 0) { newMenuOffset = 0; }

  if (newMenuOffset != menuOffset) {
    menuOffset = newMenuOffset;
    
    Serial.print("New: ");
    Serial.println(newMenu);
    
    Serial.print("Menu Offset: ");
    Serial.println(menuOffset);

    drawMenuOptions(newMenu);
  
    return newMenu;
  }

  int menu_y_new = MENU_START_Y + (MENU_ITEM_HEIGHT * newMenu);
  int menu_y_old = MENU_START_Y + (MENU_ITEM_HEIGHT * previousMenu);
  
  // Overwrite old menu item
  tft.fillRect(MENU_START_X - 5, menu_y_old - 5, MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT - 2, ILI9340_BLACK);
  tft.setTextColor(PIP_GREEN);
  tft.setCursor(MENU_START_X, menu_y_old);
  tft.print(menuOptions[previousMenu]);

  // Overwrite new menu item
  tft.fillRect(MENU_START_X - 5, menu_y_new - 5, MENU_ITEM_WIDTH, MENU_ITEM_HEIGHT - 2, PIP_GREEN);
  tft.setTextColor(ILI9340_BLACK);
  tft.setCursor(MENU_START_X, menu_y_new);
  tft.print(menuOptions[newMenu]);

  // Data
  loadMenuData(newMenu);

  return newMenu;
}

void loadMenuData(int current) {
  // Only Radio at the moment
  if (menuOptionsData[current] != "") {
    int station = menuOptionsData[current].toInt();

    Serial.print("New Station: ");
    Serial.println(station);
    
    if (fona.tuneFMradio(station)) {
      Serial.println(F("New Station: Tuned!"));
    } else {
      Serial.println(F("New Station: failed..."));
    }
  }
}

void renderSubMenu(int current) {
  if (noOfMenuOptions == 0) { return; }
  
  tft.fillRect(160, 35, 159, 190, ILI9340_BLACK);
  
  String subMenu = "";
  subMenu.concat(currentScreen);
  subMenu.concat("-");
  subMenu.concat(currentSubScreen);
  subMenu.concat("-");
  subMenu.concat(current);
  subMenu.concat(".pip");

  int len = subMenu.length() + 1;
  char subMenuName[len];
  subMenu.toCharArray(subMenuName, len);
  
  loadPip(subMenuName, false);
}

// Sub Screen
#define MAX_SUB_SCREENS 8

uint16_t menuColours[3] = { PIP_GREEN, PIP_GREEN_2, PIP_GREEN_3 };
String subScreens[MAX_SUB_SCREENS];
int noOfSubScreens;
uint16_t subscreen_x;
uint16_t subscreen_y;

void loadSubScreens(File &pip, uint16_t x, uint16_t y) {
  uint8_t currentScreenText = 0;

  subscreen_x = x;
  subscreen_y = y;

  Serial.println("Loading Sub Screens...");

  for (int i = 0; i < MAX_SUB_SCREENS; i++) {
    subScreens[i] = "";
  }
  
  while (pip.available()) {
    int character = pip.read();
    
    if (character == ASCII_CR) {
      pip.read(); // Om nom nom
      break;
    }

    if (character == ASCII_PIPE) {
      character = pip.read();
      currentScreenText++;
      Serial.println();
    }

    subScreens[currentScreenText].concat(char(character));

    Serial.print(char(character));
  }

  noOfSubScreens = currentScreenText + 1;

  Serial.println();
  Serial.print("No of Sub Screens: ");
  Serial.println(noOfSubScreens);
}

int drawSubScreen(int current, bool force) {
  if (currentScreen == RADIO_SCREEN) {
    // Fall back on volume no sub screens
    radioVolume += current;

    if (radioVolume > RADIO_MAXVOL) {
      radioVolume = RADIO_MAXVOL;
    } else if (radioVolume < 0) {
      radioVolume = 0;
    }

    Serial.print("Radio Volume: ");
    Serial.println(radioVolume);

    fona.setFMVolume(radioVolume);
    
    return currentSubScreen;
  }

  if (current < 0) { current = 0; }
  if (current > noOfSubScreens - 1) { current = noOfSubScreens - 1; }
  
  if (!force && currentSubScreen == current) {
    return current;
  }

  Serial.print("Loading Sub Screen: ");
  Serial.println(current);

  noOfMenuOptions = 0;
  
  tft.setTextSize(1);
  tft.setCursor(subscreen_x, subscreen_y);
  tft.fillRect(0, 25, 320, 10, ILI9340_BLACK);

  for (uint8_t i = current; i < current + 3; i++) {
    tft.setTextColor(menuColours[i - current], ILI9340_BLACK);
    
    tft.print(subScreens[i]);
    tft.print(" ");
  }

  tft.fillRect(0, 35, 320, 185, ILI9340_BLACK);

  String subScreen = "";
  subScreen.concat(currentScreen);
  subScreen.concat("-");
  subScreen.concat(current);
  subScreen.concat(".pip");

  int len = subScreen.length() + 1;
  char subScreenName[len];
  subScreen.toCharArray(subScreenName, len);
  
  loadPip(subScreenName, false);

  return current;
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

bool writePip(bool appendMode) {
  Serial.println("Open PIP");

  if (!appendMode) {
    SD.remove("t.pip");
  }
  
  File pipWriter = SD.open("t.pip", FILE_WRITE);
  uint8_t data;
  
  while (Serial.available()) {
    data = Serial.read();

    if (data == 11) { break; }
    
    pipWriter.write(data);

    Serial.print((char)data);
  }

  pipWriter.close();
  Serial.println("Close PIP");

  return data == 11;
}

void loadPip(const char screen[], bool mainScreen) {
  File pip = SD.open(screen);

  Serial.println();
  Serial.print("Loading PIP: ");
  Serial.print(screen);
  
  if (pip) {
    if (mainScreen) {
      // Reset Sub Screens
      noOfSubScreens = 0;

      tft.fillScreen(ILI9340_BLACK);
      drawBatt(true);
    }

    while (pip.available()) {
      uint8_t type = pip.read();
      uint16_t x = read16(pip);
      uint16_t y = read16(pip);
  
      Serial.println();
      Serial.print("X: ");
      Serial.print(x);
      Serial.print(" ");
      Serial.print("Y: ");
      Serial.println(y);
  
      switch (type) {
        // Text
        case 0:
          renderPipText(pip, x, y);
          break;

        // Image
        case 1:
          renderPipImage(pip, x, y);
          break;

        // Line
        case 2:
          renderPipLine(pip, x, y);
          break;

        // Rect
        case 3:
          renderPipRect(pip, x, y, false);
          break;

        // Fill Rect
        case 4:
          renderPipRect(pip, x, y, true);
          break;

        // Sub Screens
        case 5:
          loadSubScreens(pip, x, y);
          break;

        // Sub Menus
        case 6:
          loadMenuOptions(pip, x, y);
          drawMenuOptions(0);
          loadMenuData(0);
          break;
      }
    }
    
    pip.close();
  }
}

void renderPipRect(File &pip, uint16_t x, uint16_t y, bool fill) {
  uint16_t width = read16(pip);
  uint16_t height = read16(pip);
  uint16_t color = read16(pip);

  Serial.print("Width: ");
  Serial.println(width);
  Serial.print("Height: ");
  Serial.println(height);
  Serial.print("Colour: ");
  Serial.println(color);

  if (fill) {
    tft.fillRect(x, y, width, height, color);
  } else {
    tft.drawRect(x, y, width, height, color);
  }

  read16(pip); // Om nom nom
}

void renderPipLine(File &pip, uint16_t x, uint16_t y) {
  uint16_t endX = read16(pip);
  uint16_t endY = read16(pip);
  uint16_t color = read16(pip);

  Serial.print("End X: ");
  Serial.println(endX);
  Serial.print("End Y: ");
  Serial.println(endY);
  Serial.print("Colour: ");
  Serial.println(color);

  tft.drawLine(x, y, endX, endY, color);

  read16(pip); // Om nom nom
}

void renderPipImage(File &pip, uint16_t x, uint16_t y) {
  Serial.print("Image: ");

  int pos = 0;
  String imageName;

  while (pip.available()) {
    uint8_t character = pip.read();

    if (character == ASCII_CR) {
      pip.read(); // Om nom nom
      break;
    }

    imageName += char(character);
    
    pos ++;
  }

  Serial.print(imageName);

  int len = imageName.length() + 1;
  char imageChars[len];

  imageName.toCharArray(imageChars, len);

  bmpDraw(imageChars, x, y);
}

void renderPipText(File &pip, uint16_t x, uint16_t y) {
  uint8_t textSize = pip.read();
  uint16_t textColor = read16(pip);
  uint16_t backColor = read16(pip);

  if (backColor == 0) {
    tft.setTextColor(textColor);
  } else {
    tft.setTextColor(textColor, backColor);
  }
  
  tft.setTextSize(textSize);
  tft.setCursor(x, y);

  Serial.print("Size: ");
  Serial.println(textSize);
  Serial.print("Colour: ");
  Serial.println(textColor);
  Serial.print("Back Colour: ");
  Serial.println(backColor);
  Serial.print("Text: ");
  
  while (pip.available()) {
    uint8_t character = pip.read();
    
    if (character == ASCII_CR) {
      pip.read(); // Om nom nom
      return;
    }

    Serial.print(char(character));
    tft.print(char(character));
  }

  Serial.println();
}

void loadText(const char file[], uint16_t x, uint16_t y, int sleep) {
  File txt = SD.open(file);
  
  if (txt) {
    Serial.println(file);

    tft.setCursor(x, y);
 
    while (txt.available()) {
      tft.print(char(txt.read()));
      delay(sleep);
    }
    
    txt.close();
  }
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

  tft.fillTriangle(x0, y0, x1, y1, x2, y2, PIP_GREEN);
  tft.drawTriangle(x0, y0, x1, y1, x2, y2, ILI9340_BLACK);
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
    tft.setTextColor(PIP_GREEN, ILI9340_BLACK);
    tft.fillRect(0, 25, 320, 200, ILI9340_BLACK);
    tft.setCursor(0,30);
    
    tft.println(F("Downloading new map..."));
  
    startOfBitmapFound = false;
    endOfBitmapFound = false;

    tft.println(F("Enabling GPRS..."));
  
    fona.enableGPRS(true);
  
    atResponse(1000);

    tft.println(F("Requesting Map..."));

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

    tft.println(F("Saving Map..."));
    delay(1000);
    
    // wait for the download
    Serial.print(atResponse(30000));

    mapDownloading = true;
    
    fona.println("AT+HTTPREAD");

    // Save Image
    SD.remove(imageName);

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
      tft.fillRect(0, 25, 320, 200, ILI9340_BLACK);
      bmpDraw(imageName, MAP_POSX, MAP_POSY);

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

// Bmp loading
// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 40

void bmpDraw(const char filename[], uint16_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print("Loading image '");
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == 0) {
    Serial.print("File not found");
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print("File size: "); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print("Image Offset: "); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print("Header size: "); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print("Bit Depth: "); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print("Image size: ");
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];

            tft.pushColor(tft.Color565(r,g,b));

            /*if (b < 255 && g < 255 && r < 255) {
              tft.pushColor(tft.Color565(r,g,b));
            } else {
              tft.pushColor(tft.readPixel());
            }*/
          } // end pixel
        } // end scanline
        Serial.print("Loaded in ");
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println("BMP format not recognized.");
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File & f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File & f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}
