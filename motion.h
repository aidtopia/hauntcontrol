class MotionSensor {
  public:
    void begin(int sensor_pin, int led_pin = 0) {
      m_sensor_pin = sensor_pin;
      m_led_pin = led_pin;

      pinMode(m_sensor_pin, INPUT);

      if (m_led_pin != 0) {
        pinMode(m_led_pin, OUTPUT);
        digitalWrite(m_led_pin, HIGH);
        delay(500);
        digitalWrite(m_led_pin, LOW);
      }
      m_state = init;
    }

    enum State { disabled, init, idle, triggered };

    // Returns true if the state has changed since the last update.
    bool update() {
      if (m_sensor_pin == 0) return false;

      // Read the sensor pin and set the LED to give feedback.
      const int reading = digitalRead(m_sensor_pin);
      if (m_led_pin != 0) digitalWrite(m_led_pin, reading);

      switch (m_state) {
        case disabled:
          return false;
        case init:
          m_state = (reading == HIGH) ? triggered : idle;
          return true;
        case idle:
          if (reading == HIGH) {
            m_state = triggered;
            return true;
          }
          return false;
        case triggered:
          if (reading == LOW) {
            m_state = idle;
            return true;
          }
          return false;
      }
      Serial.println(F("Motion sensor in unknown state. Disabling."));
      m_state = disabled;
      m_sensor_pin = 0;
      return true;
    }

    State getState() const { return m_state; }

  private:
    int m_sensor_pin = 0;
    int m_led_pin = 0;
    State m_state = disabled;
};
