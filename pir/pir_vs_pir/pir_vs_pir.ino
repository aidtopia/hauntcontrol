class PIR {
  public:
    PIR(int power, int signal, int ground) :
      m_power(power), m_signal(signal), m_ground(ground),
      m_motion_detected(false)
    { }

    void begin() {
      pinMode(m_ground, OUTPUT);
      digitalWrite(m_ground, LOW);
      pinMode(m_signal, INPUT);
      pinMode(m_power, OUTPUT);
      digitalWrite(m_power, HIGH);
    }

    // Returns true if the state has changed since last update.
    bool update() {
      const auto signal = digitalRead(m_signal);
      if (signal == HIGH && !m_motion_detected) {
        m_motion_detected = true;
        return true;
      } else if (signal == LOW && m_motion_detected) {
        m_motion_detected = false;
        return true;
      }
      return false;  // no change
    }

    bool motion_detected() const { return m_motion_detected; }

  private:
    int m_power;
    int m_signal;
    int m_ground;
    bool m_motion_detected;
};

PIR pir_sensors[] = { PIR(2, 3, 4) };
char const * const names[] = { "Orange" };
int const led_pins[] = {8, 9};

auto constexpr sensor_count = sizeof(pir_sensors) / sizeof(pir_sensors[0]);

void setup() {
  for (auto &sensor : pir_sensors) sensor.begin();
  for (auto &led_pin : led_pins) {
    pinMode(led_pin, OUTPUT);
    digitalWrite(led_pin, LOW);
  }

  Serial.begin(115200);
  Serial.println(F("Hello PIR!"));
}

void loop() {
  for (unsigned i = 0; i < sensor_count; ++i) {
    auto &sensor = pir_sensors[i];
    auto &led_pin = led_pins[i];
    if (sensor.update()) {
      Serial.print(millis());
      Serial.print(' ');
      Serial.print(names[i]);
      if (sensor.motion_detected()) {
        Serial.println(F(": motion"));
      } else {
        Serial.println(F(": no motion"));
      }
    }
    digitalWrite(led_pin, sensor.motion_detected() ? HIGH : LOW);
  }
}
