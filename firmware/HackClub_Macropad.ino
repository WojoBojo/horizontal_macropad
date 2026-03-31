#include <Arduino.h>
#include "USBHIDKeyboard.h"

USBHIDKeyboard Keyboard;

//Mux address pins
const uint8_t S0 = 4;
const uint8_t S1 = 5;
const uint8_t S2 = 6;
const uint8_t S3 = 7;

//Mux common output pin (the pin the COM of the mux is connected to)
const uint8_t MUX_OUT = 1;     // <-- change if you used a different pin

// default keymap
char keyMap[16] = {'1','2','3','4','Q','W','E','R','A','S','D','F','Z','X','C','V'};

bool wasPressed[16] = {false};

void setup() {
  Keyboard.begin();
  Serial.begin(115200);

  //Set mux address pins as outputs
  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);

  pinMode(MUX_OUT, INPUT_PULLUP);

  Serial.println("macropad ready");
  Serial.println("type help if you want");
}

void setMux(int ch) {
  digitalWrite(S0, ch & 1);
  digitalWrite(S1, (ch >> 1) & 1);
  digitalWrite(S2, (ch >> 2) & 1);
  digitalWrite(S3, (ch >> 3) & 1);
  delayMicroseconds(50);
}

void loop() {
  for (int i = 0; i < 16; i++) {
    setMux(i);
    
    bool pressed = (digitalRead(MUX_OUT) == LOW);

    if (pressed && !wasPressed[i]) {
      Keyboard.press(keyMap[i]);
      wasPressed[i] = true;
    } 
    else if (!pressed && wasPressed[i]) {
      Keyboard.release(keyMap[i]);
      wasPressed[i] = false;
    }
  }

  delay(5);
}