#include <Adafruit_GPS.h>
#include <Adafruit_ILI9340.h>
#include <Adafruit_GFX.h>
#include <gfxfont.h>
#include <SPI.h>
#include <SD.h>

// Comment here when I know what this is for??
#if defined(__SAM3X8E__)
    #undef __FlashStringHelper::F(string_literal)
    #define F(string_literal) string_literal
#endif

// TFT Pins
// pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10

// SD Pin
#define SD_CS 4

// tft variable for writing to the screen
Adafruit_ILI9340 tft = Adafruit_ILI9340(TFT_CS, TFT_DC, TFT_RST);

// Sound
int speakerOut = 6;

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
}

bool writePipMode = false;
int currentScreen = 0;

void loop() {
  int newScreen = readMainSwitch();

  if (newScreen != currentScreen) {
    delay(500);

    newScreen = readMainSwitch();
    
    switch(newScreen){
      case 1:
        //loadScreen(0);
        loadPip("0.pip");
        break;
        
      case 2:
        //loadScreen(1);
        loadPip("1.pip");
        break;
        
      case 3:
        //loadScreen(2);
        loadPip("2.pip");
        break;
        
      case 4:
        //loadScreen(3);
        loadPip("3.pip");
        break;
        
      case 5:
        //loadScreen(4);
        loadPip("4.pip");
        break;
    }
    
    currentScreen = newScreen;  
  }
  
  // Serial control
  if (writePipMode) {
    writePipMode = !writePip(true);
    
    if (!writePipMode) {
      loadPip("t.pip");
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

int readMainSwitch() {
  int sensorValue = analogRead(A0);

  // Main Rotary Switch
  // 0-2        = 1
  // 1009-1010  = 2
  // 974-975    = 3
  // 928-929    = 4
  // 699-700    = 5

  if (sensorValue > 30 && sensorValue < 600) { return 1; }
  else if (sensorValue > 680 && sensorValue < 720) { return 5; }
  else if (sensorValue > 900 && sensorValue < 950) { return 4; }
  else if (sensorValue > 960 && sensorValue < 995) { return 3; }
  else if (sensorValue > 995 && sensorValue < 1020) { return 2; }

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

void loadPip(char *screen) {
  File pip = SD.open(screen);

  Serial.println();
  Serial.print("Loading PIP: ");
  Serial.print(screen);
  
  if (pip) {
    tft.fillScreen(ILI9340_BLACK);

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

    if (character == 13) {
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
    
    if (character == 13) {
      pip.read(); // Om nom nom
      return;
    }

    Serial.print(char(character));
    tft.print(char(character));
  }
}

void loadScreen(uint16_t screen) {
  switch (screen) {
    case 0:
      bmpDraw("0.bmp", 0, 0);
      break;

    case 1:
      bmpDraw("1.bmp", 0, 0);
      break;

    case 2:
      bmpDraw("2.bmp", 0, 0);
      break;

    case 3:
      bmpDraw("3.bmp", 0, 0);
      break;

    case 4:
      bmpDraw("4.bmp", 0, 0);
      break;
  }
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
