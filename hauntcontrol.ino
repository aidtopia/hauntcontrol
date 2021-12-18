// Haunt Control
// Adrian McCarthy 2021

// Currently requires an Arduino Mega 2560 for mulitple hardware serial devices.

#include <Arduino.h>

#include "audiomodule.h"  // Catalex or DFPlayer Mini audio player
#include "commandbuffer.h"
#include "lcd_display.h"  // 16x2 LCD character display
#include "motion.h"       // PIR motion sensor
#include "msgeq07.h"      // graphic equalizer chip
#include "parser.h"

// Devices
auto lcd = make_LCD(Serial1);
auto audio_board = make_AudioModule(&Serial2);
MotionSensor motion;

CommandBuffer<32> command;
auto parser = Parser(audio_board);

unsigned long timeout_time = ULONG_MAX;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Haunt Control by Hayward Haunter"));
  Serial.println(F("Copyright 2021 Adrian McCarthy"));

  Serial.print(F("Clock frequency: "));
  Serial.println(F_CPU);

  audio_board.begin();
  lcd.begin();
  command.begin();

  lcd.println(F("Haunt Control"));
  lcd.print(F("Ready."));
  Serial.println(F("Ready."));
}

void loop() {
  audio_board.update();
  lcd.update();

  if (command.available()) {
    parser.parse(command);
  }
}
