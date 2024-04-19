// Class to work with MSGEQ7
// Adrian McCarthy 2019-

// This chip measures output power in seven frequency bands (e.g., to
// produce a display like a graphic equalizer).

class MSGEQ7 {
  public:
    // Driving the chip requires two digital output pins, reset and strobe.
    // Data is collected from a single analog input pin, data.
    MSGEQ7(int reset_pin, int strobe_pin, int data_pin) :
      m_reset_pin(reset_pin),
      m_strobe_pin(strobe_pin),
      m_data_pin(data_pin) {}
 
    void begin() {
      pinMode(m_reset_pin, OUTPUT);
      digitalWrite(m_reset_pin, LOW);
      pinMode(m_strobe_pin, OUTPUT);
      digitalWrite(m_strobe_pin, LOW);
      pinMode(m_data_pin, INPUT);
      for (auto &c : m_channels) c = 0;
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
        m_channels[i] = analogRead(m_data_pin);
        digitalWrite(m_strobe_pin, HIGH);
        delayMicroseconds(36);
      }
    }

    int operator[] (int n) const {
      if (n < 0) return 0;
      if (7 <= n) return 0;
      return m_channels[n];
    }

  private:
    int m_reset_pin;
    int m_strobe_pin;
    int m_data_pin;
    int m_channels[7];
};
