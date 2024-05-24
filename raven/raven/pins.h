// Pins.h
// Adrian McCarthy 2022

// These classes attempt to make reading and writing digital pins a bit
// faster by pre-caching the register and the bit mask (which are normally
// looked up each time you call digitalRead or digitalWrite).  The
// resulting speed increase improved the stability of the animation by
// doing less work during the interrupts.

#ifndef PINS_H
#define PINS_H

class DigitalPin {
  public:
    explicit DigitalPin(int8_t p) : m_pin(p) {}

    operator int() const { return m_pin; }

  protected:
    void mode(int mode) const { pinMode(m_pin, mode); }
    uint8_t mask() const { return digitalPinToBitMask(m_pin); }
    uint8_t volatile * outputReg() const { return portOutputRegister(digitalPinToPort(m_pin)); }
    uint8_t const volatile * inputReg() const { return portInputRegister(digitalPinToPort(m_pin)); }

  private:
    int8_t m_pin;
};

class DigitalInputPin : public DigitalPin {
  public:
    explicit DigitalInputPin(int8_t p) :
      DigitalPin(p),
      m_mask(mask()),
      m_input(inputReg()) {}

    void begin() const { mode(INPUT); }
    void begin(int x) const {
      mode(x == INPUT_PULLUP ? INPUT_PULLUP : INPUT);
    }
    
    int read() const { return (*m_input & m_mask) ? HIGH : LOW; }

  private:
      uint8_t m_mask;
      uint8_t const volatile *m_input;
};

class DigitalOutputPin : public DigitalPin {
  public:
    explicit DigitalOutputPin(int8_t p) :
      DigitalPin(p),
      m_mask(mask()),
      m_output(outputReg()) {}
  
      void begin(int initial=LOW) const {
        mode(OUTPUT);
        write(initial);
      }
  
      void clear()  const { *m_output &= ~m_mask; }
      void set()    const { *m_output |=  m_mask; }
      void toggle() const { *m_output ^=  m_mask; }
      void write(int value) const {
        if (value == LOW) clear(); else set();
      }

  private:
      uint8_t m_mask;
      uint8_t volatile *m_output;
};

#endif
