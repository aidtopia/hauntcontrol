// timecode.h
// Adrian McCarthy 2024

// This class reads a SMPTE Linear Time Code (LTC) track using a digital
// pin capable of pin change interrupts (typically pin 2 or pin 3).
// Using a low-power amplifier on the DAC output from one channel of an
// audio player, this class can recognize and report the frame time.

#ifndef TIMECODE_H
#define TIMECODE_H

#include "Arduino.h"

void timecodeInterrupt();

// SMTPE linear time code has 80 bits per frame.  Frame rates vary from
// 24 to 30 f/s.  There can be one or two transitions per bit.
//   Frame Rate, Bit Frequency, Full Bit Time
//   24 fps,     1920.0 Hz,     520.83 us
//   25 fps,     2000.0 Hz,     500.00 us
//   29.97 fps,  2397.6 Hz,     417.08 us
//   30 fps,     2400.0 Hz,     416.67 us
// On 16 MHz Arduino boards, micros() has a resolution of 4 microseconds.
class TimecodeReaderImpl {
  public:
    void begin(int pin);

    // Returns true if the frame time is available.
    bool update();

    char const *as_string() const { return buffer; }

  private:
    // This looks backwards, but it's correct given how we load bitbuffer.
    static uint16_t constexpr sync_word = 0b1011111111111100;

    // These are accessed by only the interrupt handler.
    unsigned long last_us = 0;
    uint16_t bitbuffer = 0;  // We collect 16-bits at a time into this buffer.
    enum StatusMasks : uint8_t {
      BIT_COUNT   = 0x0F,
      WORD_FULL   = 0x10,
      SECOND_HALF = 0x20,
      FRAME_SYNC  = 0x40,
      RESERVED    = 0x80
    };
    uint8_t status = 0;

    // These are accessed both inside and outside the interrupt handler.
    volatile uint16_t data = 0;  // copy of most recently completed bitbuffer
    volatile bool ready = false;

    // These are accessed only outside the interrupt handler.
    uint8_t count = 0;
    char buffer[12] = "??:??:??:??";

    friend void timecodeInterrupt();
};

extern TimecodeReaderImpl TimecodeReader;

#endif
