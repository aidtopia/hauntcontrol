#include "audiomodule.h"
#include "timecode.h"

auto constexpr maestro_reset_pin = 12;
auto constexpr gray_button_pin = 53;

struct ServoConfig {
  uint8_t pot_pin;
  uint8_t servo_number;
  int in0;
  int in1;
  int out0;
  int out1;
};

static ServoConfig const servos[] = {
  /* turn  */ { A0, 2, 1023, 0, 0, 254},
  /* nod   */ { A1, 3, 1023, 0, 0, 254},
  /* tilt  */ { A2, 5, 0, 1023, 0, 254},
  /* wings */ { A3, 4, 0, 1023, 0, 254}
};

// The Maestro Micro has 6 channels.  We'll have positions for all of them
// even though the raven uses only 5 servos.
static int positions[6] = { 127, 127, 127, 127, 127, 127 };

static auto audio = make_AudioModule(Serial1);

static void resetMaestro() {
  Serial.println("Resetting the Maestro");
  digitalWrite(maestro_reset_pin, LOW);
  delay(500);
  digitalWrite(maestro_reset_pin, HIGH);
  delay(1000);
}

static void checkMaestroErrors() {
  Serial2.print("\xAA\x0C\x21");
  while (!Serial2.available()) {}
  uint8_t const lo = Serial2.read();
  while (!Serial2.available()) {}
  uint8_t const hi = Serial2.read();
  uint16_t const error_code = (static_cast<uint16_t>(hi) << 7) | lo;
  if (error_code != 0) {
    Serial.print(F("Maestro errors: "));
    if (error_code & 0x01) Serial.print(F("serial signal "));
    if (error_code & 0x02) Serial.print(F("serial overrun "));
    if (error_code & 0x04) Serial.print(F("buffer full "));
    if (error_code & 0x08) Serial.print(F("CRC error "));
    if (error_code & 0x10) Serial.print(F("protocol error "));
    if (error_code & 0x20) Serial.print(F("timeout "));
    if (error_code & 0x40) Serial.print(F("script stack error "));
    if (error_code & 0x80) Serial.print(F("call stack error "));
    if (error_code & 0x100) Serial.print(F("program counter error "));
    Serial.println();
  }
}

static void setServo(uint8_t number, uint8_t position) {
  uint8_t buffer[3] = {static_cast<uint8_t>('\xFF'), number, position};
  Serial2.write(buffer, 3);
  checkMaestroErrors();
}

static void updateServo(ServoConfig const &servo, int &pos) {
  auto const value = analogRead(servo.pot_pin);
  auto const target = map(value, servo.in0, servo.in1, servo.out0, servo.out1);
  pos = constrain((3*pos + target) / 4, 0, 254);
  setServo(servo.servo_number, static_cast<uint8_t>(pos));
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nHello, Raven!\n");

  TimecodeReader.begin(3);

  pinMode(gray_button_pin, INPUT_PULLUP);

  pinMode(maestro_reset_pin, OUTPUT);
  resetMaestro();
  Serial2.begin(115200);
  while (!Serial2) delay(10);
  Serial2.print('\xAA');  // causes Maestro to detect the baud rate

  audio.begin();
}

void loop() {
  audio.update();
  if (digitalRead(gray_button_pin) == LOW) {
    audio.playTrack(2, 3);
    while (digitalRead(gray_button_pin) == LOW) {}
  }

  #if 0
  for (auto const &servo : servos) {
    updateServo(servo, positions[servo.servo_number]);
  }
  #endif

  if (TimecodeReader.update()) {
    Serial.println(TimecodeReader.as_string());
  }
}
