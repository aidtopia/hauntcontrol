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

// Devices
SoftwareSerial serial_for_audio(11, 10);
auto audio_board = make_AudioModule(serial_for_audio);
auto frequency_analyzer = MSGEQ7(12, 13, A0);

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
  command.begin();
  
//  lcd.moveTo(1, 0);
//  lcd.print(F("Ready.          "));
}

void loop() {
  audio_board.update();
  frequency_analyzer.update();
//  lcd.update();

  if (command.available()) {
    parser.parse(command);
  }
}
