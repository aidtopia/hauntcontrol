// Haunt Control
// Adrian McCarthy 2021

// Using Arduino Pro Mini for now.

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "audiomodule.h"  // Catalex or DFPlayer Mini audio player
#include "commandbuffer.h"
#include "lcd_display.h"  // LCD character display
#include "motion.h"       // PIR motion sensor
#include "msgeq07.h"      // graphic equalizer chip
#include "parser.h"
#include "rotaryencoder.h"

// Devices
auto serial_for_audio = SoftwareSerial(11, 10);
auto audio_board = make_AudioModule(serial_for_audio);
auto frequency_analyzer = MSGEQ7(12, 13, A0);
auto rotary_encoder = RotaryEncoder(4, 5, 6, 2, 3);

constexpr int bar_segments[2] = { 9, 8 };

CommandBuffer<32> command;
auto parser = Parser(audio_board);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Haunt Control by Hayward Haunter"));
  Serial.println(F("Copyright 2021 Adrian McCarthy"));

  Serial.print(F("Clock frequency: "));
  Serial.println(F_CPU);

//  lcd.begin();
//  lcd.println(F("Haunt Control"));
//  lcd.print(F("Initializing..."));

  audio_board.begin();
  frequency_analyzer.begin();
  command.begin();
  rotary_encoder.begin();

  for (auto i : bar_segments) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  
//  lcd.moveTo(1, 0);
//  lcd.print(F("Ready.          "));
}

void loop() {
  audio_board.update();
  frequency_analyzer.update();

  const auto value = frequency_analyzer[1];
  digitalWrite(bar_segments[0], value > 768 ? HIGH : LOW); // loud thunder
  digitalWrite(bar_segments[1], value > 384 ? HIGH : LOW); // thunder
  
//  lcd.update();

  if (rotary_encoder.update()) {
    Serial.print("Turned: ");
    Serial.println(rotary_encoder.count());
  }

  if (command.available()) {
    if (!parser.parse(command)) {
      Serial.println("Command not recognized.");
    }
  }
}
