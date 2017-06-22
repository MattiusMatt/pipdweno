#include <Adafruit_ILI9340.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <SD.h>

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

// TFT Pins
// pin 11 (51) = MOSI, pin 12 (50) = MISO, pin 13 (52) = SCK
#define TFT_RST 9
#define TFT_DC 8
#define TFT_CS 10

// SD Pin
#define SD_CS 4

// GPS
#define GPS_SCREEN 3

// Radio
#define RADIO_SCREEN 4

// Pip Screens
const char PIP_0[] = "0.pip";
const char PIP_1[] = "1.pip";
const char PIP_2[] = "2.pip";
const char PIP_3[] = "3.pip";
const char PIP_4[] = "4.pip";

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

// TFT
Adafruit_ILI9340 tft = Adafruit_ILI9340(TFT_CS, TFT_DC, TFT_RST);

// Initialisation

void setup() {
  // Debug using serial
  Serial.begin(9600);

  if (!enableSD()) { return; }

  if (!enableTFT()) { return; }
  
  runLoadSequence();
}

bool enableSD() {
  //Serial.print(F("Initialising SD card..."));
  
  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD failed!"));
    return false;
  }
  
  //Serial.println(F("SD Initialised!"));

  return true;
}

bool enableTFT() {
  //Serial.println(F("Initialising TFT"));
  
  // TFT
  tft.begin();
  tft.setRotation(1);
  tft.setTextColor(ILI9340_GREEN);
  tft.setTextSize(1);

  return true;
}

void runLoadSequence() {
  //Serial.println(F("Running load sequence"));
  
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

// Screens
int currentScreen = -1;
int currentSubScreen = -1;

// Current Menu Option
long currentMenuOption = 0;
int menuOffset = 0;

void loop() {  
  // Serial control
  if (Serial.available()) {
    switch(Serial.read()){
      case 'm':
        loadMainPip();
        break;

      case 's':
        loadSubPip();
        break;
    }

    delay(1000);
  }
}

void loadSubPip() {
  int subScreen = -1;

  while (subScreen == -1) {
    subScreen = Serial.read();
  }

  drawSubScreen(subScreen, false);

  currentSubScreen = subScreen;
}

void loadMainPip() {
  int screen = -1;

  while (screen == -1) {
    screen = Serial.read();
  }
  
  switch(screen) {
      case '0':
        loadPip(PIP_0, true);
        currentScreen = 0;
        break;

      case '1':
        loadPip(PIP_1, true);
        currentScreen = 1;
        break;

      case '2':
        loadPip(PIP_2, true);
        currentScreen = 2;
        break;

      case '3':
        loadPip(PIP_3, true);
        currentScreen = 3;
        break;

      case '4':
        loadPip(PIP_4, true);
        currentScreen = 4;
        break;
    }

    // Reset Sub Screen
    drawSubScreen(0, true);
    currentSubScreen = 0;

    // Reset Menu Option
    currentMenuOption = 0;
    menuOffset = 0;
}

// Helper functions

// Status Bar
/*void drawBatt(bool fromScratch, uint16_t percentage) {
  int batWidth = round(percentage / 10) * 2;

  if (fromScratch) {
    tft.drawFastVLine(287, 10, 5, PIP_GREEN);
    tft.drawFastHLine(287, 10, 3, PIP_GREEN);
    tft.drawFastHLine(287, 15, 3, PIP_GREEN);
    tft.drawRect(291, 8, 20, 10, PIP_GREEN);
  }
  
  tft.fillRect(292, 9, 18, 8, ILI9340_BLACK);
  tft.fillRect(291 + (20 - batWidth), 8, batWidth, 10, PIP_GREEN);
}*/

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
  
  //Serial.println("Loading Sub Menus...");

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
      //Serial.println();
    }

    if (!readData) {
      menuOptions[currentMenuText].concat(char(character));
    } else {
      menuOptionsData[currentMenuText].concat(char(character));
    }

    //Serial.print(char(character));
  }

  noOfMenuOptions = currentMenuText + 1;

  //Serial.println();
  //Serial.print("No of Menu Options: ");
  //Serial.println(noOfMenuOptions);
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
    
    //Serial.print("New: ");
    //Serial.println(newMenu);
    
    //Serial.print("Menu Offset: ");
    //Serial.println(menuOffset);

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

    //Serial.print("New Station: ");
    //Serial.println(station);
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

  //Serial.println("Loading Sub Screens...");

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
      //Serial.println();
    }

    subScreens[currentScreenText].concat(char(character));

    //Serial.print(char(character));
  }

  noOfSubScreens = currentScreenText + 1;

  //Serial.println();
  //Serial.print("No of Sub Screens: ");
  //Serial.println(noOfSubScreens);
}

int drawSubScreen(int current, bool force) {
  //Serial.print("Loading Sub Screen: ");
  //Serial.println(current);

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

/*bool writePip(bool appendMode) {
  //Serial.println("Open PIP");

  if (!appendMode) {
    SD.remove("t.pip");
  }
  
  File pipWriter = SD.open("t.pip", FILE_WRITE);
  uint8_t data;
  
  while (Serial.available()) {
    data = Serial.read();

    if (data == 11) { break; }
    
    pipWriter.write(data);

    //Serial.print((char)data);
  }

  pipWriter.close();
  //Serial.println("Close PIP");

  return data == 11;
}*/

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
      //drawBatt(true, 10);
    }

    while (pip.available()) {
      uint8_t type = pip.read();
      uint16_t x = read16(pip);
      uint16_t y = read16(pip);
  
      //Serial.println();
      //Serial.print("X: ");
      //Serial.print(x);
      //Serial.print(" ");
      //Serial.print("Y: ");
      //Serial.println(y);
  
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

  //Serial.print("Width: ");
  //Serial.println(width);
  //Serial.print("Height: ");
  //Serial.println(height);
  //Serial.print("Colour: ");
  //Serial.println(color);

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

  //Serial.print("End X: ");
  //Serial.println(endX);
  //Serial.print("End Y: ");
  //Serial.println(endY);
  //Serial.print("Colour: ");
  //Serial.println(color);

  tft.drawLine(x, y, endX, endY, color);

  read16(pip); // Om nom nom
}

void renderPipImage(File &pip, uint16_t x, uint16_t y) {
  //Serial.print("Image: ");

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

  //Serial.print(imageName);

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

  //Serial.print("Size: ");
  //Serial.println(textSize);
  //Serial.print("Colour: ");
  //Serial.println(textColor);
  //Serial.print("Back Colour: ");
  //Serial.println(backColor);
  //Serial.print("Text: ");
  
  while (pip.available()) {
    uint8_t character = pip.read();
    
    if (character == ASCII_CR) {
      pip.read(); // Om nom nom
      return;
    }

    //Serial.print(char(character));
    tft.print(char(character));
  }

  //Serial.println();
}

void loadText(const char file[], uint16_t x, uint16_t y, int sleep) {
  File txt = SD.open(file);
  
  if (txt) {
    //Serial.println(file);

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

#define BUFFPIXEL 20

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
    Serial.print("File not found ");
    Serial.print(filename);
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print("File size: "); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    //Serial.print("Image Offset: "); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print("Header size: "); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      //Serial.print("Bit Depth: "); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        //Serial.print("Image size: ");
        //Serial.print(bmpWidth);
        //Serial.print('x');
        //Serial.println(bmpHeight);

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
        //Serial.print("Loaded in ");
        //Serial.print(millis() - startTime);
        //Serial.println(" ms");
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
