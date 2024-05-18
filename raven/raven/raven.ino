template <typename Element, size_t Count>
constexpr size_t arraySize(const Element (&)[Count]) noexcept {
  return Count;
}

auto constexpr maestro_reset_pin = 12;

struct ServoConfig {
  uint8_t pot_pin;
  uint8_t servo_number;
  int in0;
  int in1;
  int out0;
  int out1;
};

static const ServoConfig servos[] = {
  /* turn  */ { A0, 2, 1023, 0, 0, 254},
  /* nod   */ { A1, 3, 1023, 0, 0, 254},
  /* tilt  */ { A2, 5, 0, 1023, 0, 254},
  /* wings */ { A3, 4, 0, 1023, 0, 254}
};

// The Maestro Micro has 6 channels.  We'll have positions for all of them
// even though the raven uses only 5 servos.
static int positions[6] = { 127, 127, 127, 127, 127, 127 };

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
    Serial.print("Errors: ");
    if (error_code & 0x01) Serial.print("serial signal ");
    if (error_code & 0x02) Serial.print("serial overrun ");
    if (error_code & 0x04) Serial.print("buffer full ");
    if (error_code & 0x08) Serial.print("CRC error ");
    if (error_code & 0x10) Serial.print("protocol error ");
    if (error_code & 0x20) Serial.print("timeout ");
    if (error_code & 0x40) Serial.print("script stack error ");
    if (error_code & 0x80) Serial.print("call stack error ");
    if (error_code & 0x100) Serial.print("program counter error ");
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

  pinMode(maestro_reset_pin, OUTPUT);
  resetMaestro();
  Serial2.begin(115200);
  while (!Serial2) delay(10);
  Serial2.print('\xAA');  // causes Maestro to detect the baud rate
}

void loop() {
  for (auto const &servo : servos) {
    updateServo(servo, positions[servo.servo_number]);
  }
  delay(20);
}
