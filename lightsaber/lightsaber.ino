#include <SoftwareSerial.h>
#include "Adafruit_Soundboard.h"

// RGB Properties
#define LED_R 9
#define LED_G 10
#define LED_B 11

// Sound Properties
#define SFX_TX 5
#define SFX_RX 6
#define SFX_RST 4
#define SFX_ACT 7

// Fonts
#define SND_ON 0
#define SND_HUM 1
#define SND_CLASH_0 2
#define SND_CLASH_1 3
#define SND_CLASH_2 4
#define SND_SWING_0 5
#define SND_SWING_1 6
#define SND_SWING_2 7

SoftwareSerial audioSerial = SoftwareSerial(SFX_TX, SFX_RX);
Adafruit_Soundboard sfx = Adafruit_Soundboard(&audioSerial, NULL, SFX_RST);

// Accelerometer Properties
#define ACC_X A0
#define ACC_Y A1
#define ACC_Z A2

#define SWING_TOL 50
#define CLASH_TOL 170

#define SWING_DEL 0.5
#define CLASH_DEL 0.5

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

  // LED Reset
  analogWrite(LED_R, 0);
  analogWrite(LED_G, 0);
  analogWrite(LED_B, 0);

  // TEMP
  analogWrite(LED_R, 50);
  analogWrite(LED_G, 50);
  analogWrite(LED_B, 50);
  // TEMP

  // Sound On
  delay(1000);
  Serial.println("Boot");
  playAudio(SND_ON);
}

bool processingSwing = false;
bool processingClash = false;
uint32_t swing_detected = millis();
uint32_t clash_detected = millis();

void loop() {
  processMovement();

  if (processingSwing && (millis() - swing_detected) > (SWING_DEL * 1000)) {
    processingSwing = false;
  }

  if (processingClash && (millis() - clash_detected) > (CLASH_DEL * 1000)) {
    processingClash = false;
  }

  if (!processingSwing && !processingClash) {
    loopAudio(SND_HUM);
  }

  delay(10);

  // TEMP
  //Serial.println("LED High");
  //analogWrite(LED_R, 150);
  //delay(3000);
  //analogWrite(LED_R, 0);
  //Serial.println("LED Low");
  //delay(3000);
  // TEMP
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
  if (!processingSwing && !processingClash) {
    Serial.println("Swing detected");
    swing_detected = millis();
    
    processingSwing = true;

    int swing = random(SND_SWING_0, SND_SWING_2 + 1);
    
    Serial.print("Swing ");
    Serial.println(swing);
    playAudio(swing);
  }
}

void clash() {
  if (!processingClash) {
    Serial.println("Clash detected");
    clash_detected = millis();
    
    processingClash = true;

    int clash = random(SND_CLASH_0, SND_CLASH_2 + 1);
    
    Serial.print("Clash ");
    Serial.println(clash);
    playAudio(clash);
  }
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
    playAudio(track);
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

