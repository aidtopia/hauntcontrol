// Experimenting with laser module
// Adrian McCarthy 2021

constexpr int laser_pin = 10;  // Pin 10 is OC2A, generated directly from Timer2

void setup() {
  Serial.begin(115200);
  Serial.println(F("Hello, Laser!"));
  pinMode(laser_pin, OUTPUT);
  digitalWrite(laser_pin, LOW);

#if 0
  // Set up Timer2 to generate a square wave on OC2A (pin 10).
  // Set the Compare On Match unit to Toggle mode (COM2A1=0, COM2A0=1).
  // Wave Generation Mode is Clear Timer on Compare (CTC).
  TCCR2A = bit(COM2A0) | bit(WGM21);
  // We want a frequency of 10kHz.
  // - Base clock is 16e6 Hz.
  // - Prescalar of 8 makes Timer 2 tick at 2e6 Hz.
  // - Count of 100 toggles the pin at 20 kHz.
  // - Two toggles per period gives a frequency of 10 kHz.
  // That can be acheived with a prescalar of 8 (CS22=0, CS21=1, CS20=0) ...
  TCCR2B = bit(CS21);
  // ... and a count of 100.
  OCR2A = 100 - 1;  // -1 because it's zero-based
#endif

  Serial1.begin(9600);
  delay(500);  // give the display time to boot up
  Serial1.write(0x12);  // reset to defaults
  delay(500);
  Serial1.write(0x7C);
  Serial1.write(0x04);  // 16 columns
  delay(500);
  Serial1.write(0x7C);
  Serial1.write(0x06);  // 2 rows
  delay(500);
  Serial1.write(0x7C);
  Serial1.write(0x90);  // backlight on about 50%
  delay(500);
  Serial1.write(0xFE);
  Serial1.write(0x01);  // clear and reset cursor
  Serial1.write("Hello, Laser!");
}

void loop() {
  delay(100);
}
