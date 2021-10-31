// Haunt Control
// Adrian McCarthy

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "audiomodule.h"  // Catalex or DFPlayer Mini audio player
#include "commandbuffer.h"
#include "motion.h"
#include "msgeq07.h"      // graphic equalizer chip
#include "parser.h"

// Pin assignments
const int red_button = 2;
const int green_button = 3;
const int blue_button = 4;
const int yellow_button = 5;
const int tx_to_audio = 10;
const int rx_from_audio = 11;
const int eq_output = A0;
const int eq_reset = 12;
const int eq_strobe = 13;
const int eq_display[] = { 6, 7, 8, 9 };
constexpr int spit_valve = 23;
SoftwareSerial audio_serial(rx_from_audio, tx_to_audio);
typedef AudioModule MyAudioModule;
MyAudioModule audio_board(&audio_serial);
MotionSensor motion;

CommandBuffer<32> command;
auto parser = Parser<MyAudioModule>(audio_board);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Haunt Control by Hayward Haunter"));
  Serial.println(F("Copyright 2021 Adrian McCarthy"));
  command.begin();

  pinMode(red_button, INPUT_PULLUP);
  pinMode(green_button, INPUT_PULLUP);
  pinMode(blue_button, INPUT_PULLUP);
  pinMode(yellow_button, INPUT_PULLUP);

#if 0
  pinMode(rx_from_audio, INPUT);  // Will SoftwareSerial::begin()
  pinMode(tx_to_audio, OUTPUT);   // handle these pinModes?
  audio_serial.begin(9600);
  audio_board.begin();

  msgeq7.begin(eq_reset, eq_strobe, eq_output, eq_display);
#endif

  pinMode(spit_valve, OUTPUT);
  digitalWrite(spit_valve, HIGH);
  motion.begin(50, 51);
  Serial.println(F("Ready."));
}

void loop() {
#if 0
  if (digitalRead(red_button) == LOW) {
    Serial.println(F("red button pressed"));
    audio_board.selectSource(MyAudioModule::DEV_SDCARD);
    while (digitalRead(red_button) == LOW) {}
  }
  if (digitalRead(green_button) == LOW) {
    Serial.println(F("green button pressed"));
    audio_board.selectSource(MyAudioModule::DEV_USB);
    while (digitalRead(green_button) == LOW) {}
  }
  if (digitalRead(blue_button) == LOW) {
    Serial.println(F("blue button pressed"));
    audio_board.playPreviousFile();
    while (digitalRead(blue_button) == LOW) {}
  }
  if (digitalRead(yellow_button) == LOW) {
    Serial.println(F("yellow button pressed"));
    audio_board.playNextFile();
    while (digitalRead(yellow_button) == LOW) {}
  }
  audio_board.update();

  msgeq7.update();
#endif

  if (motion.update()) {
    switch (motion.getState()) {
      case MotionSensor::idle:
        digitalWrite(spit_valve, HIGH);
        Serial.println(F("Motion ceased."));
        break;
      case MotionSensor::triggered:
        Serial.println(F("Motion detected."));
        digitalWrite(spit_valve, LOW);
        break;
    }
  }

  if (command.available()) {
    parser.parse(command);
  }
}
