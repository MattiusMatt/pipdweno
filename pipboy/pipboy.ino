#include <Adafruit_GPS.h>
#include <Adafruit_ILI9340.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>
#include <Encoder.h>

// Comment here when I know what this is for??
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

// ASCII Consts
#define ASCII_CR 13
#define ASCII_PIPE 124

// TFT Pins
// pin 11 (51) = MOSI, pin 12 (50) = MISO, pin 13 (52) = SCK
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10

// SD Pin
#define SD_CS 4

// Rotary Encoder
#define ENCODER_BUTTON 19
#define ENCODER_A 20
#define ENCODER_B 21

// Audio Pin
#define AUDIO_TMR 11

// PIP Colours
#define PIP_GREEN 2016
#define PIP_GREEN_2 800
#define PIP_GREEN_3 416

// TFT
Adafruit_ILI9340 tft = Adafruit_ILI9340(TFT_CS, TFT_DC, TFT_RST);

// Encoder
Encoder encoder(ENCODER_A, ENCODER_B);

// Sound
TMRpcm audio;

void setup() {
  // Debug using serial
  Serial.begin(9600);

  // SD Card
  Serial.print("Initializing SD card...");
  
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    return;
  }
  Serial.println("OK!");

  // Sound
  audio.speakerPin = AUDIO_TMR;
  audio.setVolume(5);

  // Encoder Button
  pinMode(ENCODER_BUTTON, INPUT_PULLUP);
  
  // TFT
  tft.begin();
  tft.setRotation(3);
  tft.setTextColor(ILI9340_GREEN);
  tft.setTextSize(1);

  // Splash 0
  tft.fillScreen(ILI9340_BLACK);
  loadText("0.txt", 0, 0, 0);
  delay(1000);

  // Loading
  tft.fillScreen(ILI9340_BLACK);
  loadText("1.txt", 0, 0, 20);
  delay(1000);

  // Vault Boy Loading
  tft.fillScreen(ILI9340_BLACK);
  bmpDraw("l.bmp", 95, 35);
  delay(1000);

  //play("1.wav");
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

void loop() {
  // Main Screen
  int newScreen = readMainSwitch();

  if (newScreen != currentScreen) {
    delay(500);

    newScreen = readMainSwitch();
    
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
        break;
        
      case 4:
        loadPip("4.pip", true);
        break;
    }
    
    currentScreen = newScreen;

    // Reset Sub Screen
    currentSubScreen = 0;
    encoder.write(0);
    drawSubScreen(currentSubScreen);

    // Reset Menu Option
    currentMenuOption = 0;
    menuMode = false;
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
        currentMenuOption = 0;
        encoder.write(currentMenuOption * 2);
      }

      Serial.print("Menu Mode: ");
      Serial.println(menuMode);
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

    if (drawSubScreen(newEncoderValue)) {
      currentSubScreen = newEncoderValue;
    } else {
      encoder.write(currentSubScreen * 2);
    }

    // TEMP
    if (currentSubScreen == 1) {
      loadMenuOptions();
      drawMenuOptions(0);
    }
  }

  // Menu
  if (menuMode && newEncoderValue != currentMenuOption) {
    delay(500);

    newEncoderValue = encoder.read() / 2;

    Serial.println();
    Serial.print("New Encoder Value: ");
    Serial.println(newEncoderValue);
    
    if (updateMenuOptions(newEncoderValue, currentMenuOption)) {
      currentMenuOption = newEncoderValue;
    } else {
      encoder.write(currentMenuOption * 2);
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
}

// Helper functions

// Menu Screen
#define MAX_MENU_OPTIONS 20
#define MAX_MENU_DISPLAY 8
#define MENU_ITEM_HEIGHT 20
#define MENU_ITEM_WIDTH 140
#define MENU_START_X 20
#define MENU_START_Y 55

String menuOptions[MAX_MENU_OPTIONS];
int noOfMenuOptions;
int menuOffset = 0;

void loadMenuOptions() {
  for (int i = 0; i < MAX_MENU_OPTIONS; i++) {
    menuOptions[i] = "";
  }
  
  menuOptions[0] = "Option One";
  menuOptions[1] = "Option Two";
  menuOptions[2] = "Option Three";
  menuOptions[3] = "Option Four";
  menuOptions[4] = "Option Five";
  menuOptions[5] = "Option Six";
  menuOptions[6] = "Option Seven";
  menuOptions[7] = "Option Eight";
  menuOptions[8] = "Option Nine";
  menuOptions[9] = "Option Ten";

  noOfMenuOptions = 10;
}

void drawMenuOptions(int current) {
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

bool updateMenuOptions(int newMenu, int previousMenu) {
  if (newMenu < 0 || (newMenu > noOfMenuOptions - 1)) { return false; }

  int newMenuOffset = newMenu - (MAX_MENU_DISPLAY - 1);

  if (newMenuOffset < 0) { newMenuOffset = 0; }

  if (newMenuOffset != menuOffset) {
    menuOffset = newMenuOffset;
    
    Serial.print("New: ");
    Serial.println(newMenu);
    
    Serial.print("Menu Offset: ");
    Serial.println(menuOffset);

    drawMenuOptions(newMenu);
  
    return true;
  }

  /*if (newMenu > (MAX_MENU_DISPLAY - 1) || previousMenu > (MAX_MENU_DISPLAY - 1)) {
    menuOffset = newMenu - (MAX_MENU_DISPLAY - 1);

    Serial.print("New: ");
    Serial.println(newMenu);
    
    Serial.print("Menu Offset: ");
    Serial.println(menuOffset);

    drawMenuOptions(newMenu);
  
    return true;
  } else {
    menuOffset = 0;
  }*/

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

  return true;
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

bool drawSubScreen(int current) {
  if (current < 0 || (current > noOfSubScreens - 1)) { return false; }

  Serial.print("Loading Sub Screen: ");
  Serial.println(current);
  
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

  return true;
}

//Sound
void play(char *file) {
  audio.play(file);

  while (audio.isPlaying()) {
  }

  audio.disable();
}

int readMainSwitch() {
  int sensorValue = analogRead(A7);

  // Main Rotary Switch
  // 0-2        = 1
  // 1009-1010  = 2
  // 974-975    = 3
  // 928-929    = 4
  // 699-700    = 5

  if (sensorValue > 30 && sensorValue < 600) { return 0; }
  else if (sensorValue > 680 && sensorValue < 720) { return 4; }
  else if (sensorValue > 900 && sensorValue < 950) { return 3; }
  else if (sensorValue > 960 && sensorValue < 995) { return 2; }
  else if (sensorValue > 995 && sensorValue < 1020) { return 1; }

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

void loadPip(char *screen, bool mainScreen) {
  File pip = SD.open(screen);

  Serial.println();
  Serial.print("Loading PIP: ");
  Serial.print(screen);
  
  if (pip) {
    if (mainScreen) {
      // Reset Sub Screens
      noOfSubScreens = 0;

      tft.fillScreen(ILI9340_BLACK);
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

void loadText(char *file, uint16_t x, uint16_t y, int sleep) {
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

// Bmp loading
// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 40

void bmpDraw(char *filename, uint16_t x, uint16_t y) {

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
  if ((bmpFile = SD.open(filename)) == NULL) {
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
