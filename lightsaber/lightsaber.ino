#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

// Sound Properties
#define SFX_TX 5
#define SFX_RX 6
#define SFX_RST 4
#define SFX_ACT 7

SoftwareSerial audioSerial = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&audioSerial, NULL, SFX_RST);

// Accelerometer Properties
#define ACC_X A0
#define ACC_Y A1
#define ACC_Z A2

#define SWING_TOL 50
#define CLASH_TOL 170

// Main App
void setup() {
  // Console
  Serial.begin(115200);

  // Sound
  audioSerial.begin(9600);

  pinMode(SFX_ACT, INPUT);

  if (!sfx.reset()) {
    Serial.println("Audio not found");
    while (1);
  }
}

bool processingMovement = false;

void loop() {
  processMovement();

  loopAudio(4);

  delay(100);
}

// Accelerometer
int previous_x = 0;
int previous_y = 0;
int previous_z = 0;

void processMovement() {
  int x = analogRead(ACC_X);
  int y = analogRead(ACC_Y);
  int z = analogRead(ACC_Z);

  int x_diff = x - previous_x;
  int y_diff = y - previous_y;
  int z_diff = z - previous_z;

  // Stop initialisation detection
  if (previous_x + previous_y + previous_z > 0) {
    if (detectMovement(x_diff, y_diff, z_diff, CLASH_TOL)) {
      clash();
    } else if (detectMovement(x_diff, y_diff, z_diff, SWING_TOL)) {
      swing();
    }
  }

  previous_x = x;
  previous_y = y;
  previous_z = z;
}

bool detectMovement(int x_diff, int y_diff, int z_diff, int tolerence) {
  if (x_diff >= tolerence || x_diff * -1 >= tolerence) {
    Serial.print("X: ");
    Serial.println(x_diff);

    return true;
  }
  
  if (y_diff >= tolerence || y_diff * -1 >= tolerence) {
    Serial.print("Y: ");
    Serial.println(y_diff);

    return true;
  }
  
  if (z_diff >= tolerence || z_diff * -1 >= tolerence) {
    Serial.print("Z: ");
    Serial.println(z_diff);

    return true;
  }

  return false;
}

void swing() {
  if (!processingMovement) {
    Serial.println("Swing detected");
    
    processingMovement = true;
    
    playAudio(0);
  }
  
  delay(500);
}

void clash() {
  if (!processingMovement) {
    Serial.println("Clash detected");
    
    processingMovement = true;
    
    playAudio(1);
  }
  
  delay(1000);
}

// Sound
bool audioPlaying() {
  int actValue = digitalRead(SFX_ACT);

  return actValue == 0;
}

void playAudio(uint8_t track) {
  stopAudio();
  if (!sfx.playTrack(track)) {
    Serial.println("Failed to play Audio?");
  }
}

void stopAudio() {
  if (audioPlaying()) {
    if (!sfx.stop()) {
      Serial.println("Failed to stop");
    }
  }
}

void loopAudio(uint8_t track) {
  if (!audioPlaying()) {
    processingMovement = false;
    //playAudio(track);
  }
}

void flushInput() {
  // Read all available serial input to flush pending data.
  uint16_t timeoutloop = 0;
  
  while (timeoutloop++ < 40) {
    while(audioSerial.available()) {
      audioSerial.read();
      timeoutloop = 0;  // If char was received reset the timer
    }
    delay(1);
  }
}

