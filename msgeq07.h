class MSGEQ7 {
  public:
    void begin(int reset_pin, int strobe_pin, int output_pin, const int eq_display[4]) {
      m_reset_pin = reset_pin;
      pinMode(m_reset_pin, OUTPUT);
      digitalWrite(m_reset_pin, LOW);
      m_strobe_pin = strobe_pin;
      pinMode(m_strobe_pin, OUTPUT);
      digitalWrite(m_strobe_pin, LOW);
      m_output_pin = output_pin;
      for (int i = 0; i < 4; ++i) {
        m_eq_display[i] = eq_display[i];
        pinMode(m_eq_display[i], OUTPUT);
        digitalWrite(m_eq_display[i], LOW);
      }      
    }

    void update() {
      digitalWrite(m_reset_pin, HIGH);
      digitalWrite(m_strobe_pin, HIGH);
      delayMicroseconds(18);
      digitalWrite(m_strobe_pin, LOW);
      delayMicroseconds(18);
      digitalWrite(m_strobe_pin, HIGH);
      digitalWrite(m_reset_pin, LOW);
      delayMicroseconds(18);
      for (int i = 0; i < 7; ++i) {
        digitalWrite(m_strobe_pin, LOW);
        delayMicroseconds(36);
        m_channels[i] = analogRead(m_output_pin);
        digitalWrite(m_strobe_pin, HIGH);
        delayMicroseconds(36);
      }

      // Show channel 1 (160 Hz) in the eq display.
      digitalWrite(m_eq_display[0], m_channels[1] >   0 ? HIGH : LOW);
      digitalWrite(m_eq_display[1], m_channels[1] > 255 ? HIGH : LOW);
      digitalWrite(m_eq_display[2], m_channels[1] > 511 ? HIGH : LOW);
      digitalWrite(m_eq_display[3], m_channels[1] > 767 ? HIGH : LOW);
    }

  private:
    int m_reset_pin;
    int m_strobe_pin;
    int m_output_pin;
    int m_channels[7];
    int m_eq_display[4];
} msgeq7;
