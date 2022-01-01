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
auto serial_for_lcd = SoftwareSerial(A2, A3);
auto lcd = make_LCD(serial_for_lcd);

constexpr int bar_segments[2] = { 9, 8 };

CommandBuffer<32> command;
auto parser = Parser(audio_board);

void Format(int x, char *buffer, size_t N) {
//  static_assert(N > 0, "cannot format to empty buffer");
  if (N == 0) return;
  size_t i = N;
  buffer[--i] = '\0';
  if (i == 0) return;
  if (x == 0) {
    buffer[--i] = '0';
  } else {
    const unsigned neg = x < 0 ? 1 : 0;
    if (neg) x = -x;
    while (i > neg && x != 0) {
      buffer[--i] = '0' + x%10;
      x /= 10;
    }
    if (neg) buffer[--i] = '-';
  }
  while (i > 0) buffer[--i] = ' ';
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Haunt Control by Hayward Haunter"));
  Serial.println(F("Copyright 2021 Adrian McCarthy"));

  lcd.begin();
  lcd.println(F("Haunt Control"));
  lcd.print(F("Initializing..."));

  audio_board.begin();
  frequency_analyzer.begin();
  command.begin();
  rotary_encoder.begin();

  for (auto i : bar_segments) {
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  
  lcd.moveTo(1, 0);
  lcd.print(F("Ready.          "));
}

void loop() {
  audio_board.update();

  frequency_analyzer.update();
  const auto value = frequency_analyzer[1];
  digitalWrite(bar_segments[0], value > 768 ? HIGH : LOW); // loud thunder
  digitalWrite(bar_segments[1], value > 384 ? HIGH : LOW); // thunder
  
  lcd.update();

  if (rotary_encoder.update()) {
    lcd.moveTo(1, 0);
    lcd.print("Knob: ");
    lcd.moveTo(1, 6);
    char buf[6];
    Format(rotary_encoder.count(), buf, sizeof(buf));
    lcd.print(buf);
  }

  if (command.available()) {
    if (!parser.parse(command)) {
      Serial.println("Command not recognized.");
    }
  }
}
