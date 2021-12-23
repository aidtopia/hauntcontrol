// Rotary Encoder
// Adrian McCarthy 2021

class RotaryEncoder {
  public:
    RotaryEncoder(int A_pin, int B_pin, int button_pin = 0, int red_pin = 0, int green_pin = 0) :
      m_a(A_pin), m_b(B_pin), m_button(button_pin), m_red(red_pin), m_green(green_pin),
      m_counts_per_detent(1), m_state(0), m_raw_count(0) {}

    void begin() {
      pinMode(m_a, INPUT_PULLUP);
      pinMode(m_b, INPUT_PULLUP);
      updateState();
      m_raw_count = 0;
      if (m_button != 0) {
        pinMode(m_button, INPUT_PULLUP);
      }
      if (m_red != 0) {
        pinMode(m_red, OUTPUT);
        digitalWrite(m_red, LOW);
      }
      if (m_green != 0) {
        pinMode(m_green, OUTPUT);
        digitalWrite(m_green, LOW);
      }
    }

    int count() const { return m_counts_per_detent * (m_raw_count + 2) / 4; }
    void reset() { m_raw_count = 0; }

    bool update() {
      enum { CW=1, CCW=-1, INVALID=0, NO_CHANGE=0 };
      // Decoder is indexed by four bits: old A, old B, new A, new B
      static constexpr int table[16] = {
        /* 0b0000 */ NO_CHANGE,
        /* 0b0001 */ CCW,
        /* 0b0010 */ CW,
        /* 0b0011 */ INVALID,
        /* 0b0100 */ CW,
        /* 0b0101 */ NO_CHANGE,
        /* 0b0110 */ INVALID,
        /* 0b0111 */ CCW,
        /* 0b1000 */ CCW,
        /* 0b1001 */ INVALID,
        /* 0b1010 */ NO_CHANGE,
        /* 0b1011 */ CW,
        /* 0b1100 */ INVALID,
        /* 0b1101 */ CW,
        /* 0b1110 */ CCW,
        /* 0b1111 */ NO_CHANGE
      };
      updateState();
      const auto delta = table[m_state];
      m_raw_count += delta;
      if (m_red   != 0) digitalWrite(m_red,   delta < 0 ? HIGH : LOW);
      if (m_green != 0) digitalWrite(m_green, delta > 0 ? HIGH : LOW);
      return (delta != 0) &&
        ((m_counts_per_detent >= 4) || (m_raw_count % (4/m_counts_per_detent)) == 0);
    }

    private:
      void updateState() {
        m_state = (m_state << 2) & 0x0F;
        m_state |= (digitalRead(m_a) == HIGH) ? 0b10 : 0;
        m_state |= (digitalRead(m_b) == HIGH) ? 0b01 : 0;
      }
      
      int m_a, m_b;
      int m_button;
      int m_red, m_green;
      int8_t m_counts_per_detent;
      uint8_t m_state;
      int m_raw_count;
};
