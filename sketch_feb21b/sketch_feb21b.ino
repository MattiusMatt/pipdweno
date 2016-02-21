const int PinCLK = 34;
const int PinDT = 36;
const int PinSW = 38;

volatile int virtualPosition = 0;

void isr() {
  if (digitalRead(PinDT)) {
    virtualPosition = virtualPosition + 1;
  } else {
    virtualPosition = virtualPosition - 1;
  }
}

void setup() {
  Serial.begin(9600);
  
  pinMode(PinCLK, INPUT);
  pinMode(PinDT, INPUT);
  pinMode(PinSW, INPUT);

  attachInterrupt(0, isr, FALLING);   // interrupt 0 is always connected to pin 2 on Arduino UNO

  Serial.println("Start");
}

void loop() {
  int lastCount = 0;

  while (true) {
    if (!(digitalRead(PinSW))) {        // check if pushbutton is pressed
      virtualPosition = 0;            // if YES, then reset counter to ZERO
      while (!digitalRead(PinSW)) {}  // wait til switch is released
      delay(10);                      // debounce
      Serial.println("Reset");        // Using the word RESET instead of COUNT here to find out a buggy encoder
    }
    
    if (virtualPosition != lastCount) {
      lastCount = virtualPosition;
      Serial.print("Count");
      Serial.println(virtualPosition);
    }
  } // while
}
