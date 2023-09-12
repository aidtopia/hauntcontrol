// Class to control a fog machine via a relay.
// Adrian McCarthy 2023

class Fogger {
  public:
    Fogger(int pin, int trigger_level = HIGH) :
      m_pin(pin), m_level(trigger_level), m_timeout(0) {}

    void begin() {
      pinMode(m_pin, OUTPUT);
      off();
    }

    void update() {
      if (m_timeout == 0) return;
      auto const now = millis();
      if (m_timeout <= now) {
        if (now - m_timeout <= MAX_BURST) off();  // legit expiration
      } else {
        if (m_timeout - now > MAX_BURST) off();  // clock wrapped
      }
    }

    void on(unsigned long duration = 1000) {
      if (m_timeout == 0) {
        digitalWrite(m_pin, m_level);
        duration = (duration > MAX_BURST) ? MAX_BURST : duration;
        m_timeout = millis() + duration;
        if (m_timeout == 0) m_timeout = 1;  // because 0 means "off"
      }
    }

    void off() {
      digitalWrite(m_pin, m_level == LOW ? HIGH : LOW);
      m_timeout = 0;
    }

  private:
    int const m_pin;
    int const m_level;
    unsigned long m_timeout;

    static unsigned long constexpr MAX_BURST = 60000;
};