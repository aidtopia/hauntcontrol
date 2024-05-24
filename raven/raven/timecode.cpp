#include "timecode.h"

#include "pins.h"

static DigitalOutputPin oscope_pink(49);
static DigitalOutputPin oscope_blue(45);

TimecodeReaderImpl TimecodeReader;

void TimecodeReaderImpl::begin(int pin) {
  oscope_pink.begin();
  oscope_blue.begin();

  auto const interrupt = digitalPinToInterrupt(pin);
  if (interrupt < 0) {
    Serial.print(F("Cannot set up TimecodeReader on pin "));
    Serial.print(pin);
    Serial.println(F(" because it doesn't support pin change interrupts."));
    return;
  }
  pinMode(pin, INPUT);
  attachInterrupt(interrupt, timecodeInterrupt, CHANGE);
}

bool TimecodeReaderImpl::update() {
  bool local_ready = false;
  noInterrupts();
  local_ready = ready;
  ready = false;
  interrupts();

  if (!local_ready) return false;
  
  if (data == sync_word) {
    bool const complete = count == 4;
    count = 0;
    return complete;
  }

  switch (count++) {
    case 0:
      buffer[10] = '0' +  (data & 0x000F);
      buffer[ 9] = '0' + ((data & 0x0300) >> 8);
      // When using "drop frame" counting, the convention is to use a semicolon
      // instead of a colon.
      buffer[ 8] = (data & 0x0400) ? ';' : ':';
      break;
    case 1:
      buffer[ 7] = '0' +  (data & 0x000F);
      buffer[ 6] = '0' + ((data & 0x0700) >> 8);
      break;
    case 2:
      buffer[ 4] = '0' +  (data & 0x000F);
      buffer[ 3] = '0' + ((data & 0x0700) >> 8);
      break;
    case 3:
      buffer[ 1] = '0' +  (data & 0x000F);
      buffer[ 0] = '0' + ((data & 0x0300) >> 8);
      break;
  }

  return false;
}

// This interrupt is called for each edge in the clock track.
void timecodeInterrupt() {
  // I tried `using` to pull these names into scope, but that didn't work.
  auto constexpr WORD_FULL   = TimecodeReaderImpl::WORD_FULL;
  auto constexpr SECOND_HALF = TimecodeReaderImpl::SECOND_HALF;
  auto constexpr FRAME_SYNC  = TimecodeReaderImpl::FRAME_SYNC;
  auto &tc = TimecodeReader;

  auto const now_us = micros();
  auto const delta_us = static_cast<uint16_t>(now_us - tc.last_us);
  if (delta_us <= 32) return;  // glitch shorter than the resolution of micros()
  tc.last_us = now_us;

  switch (delta_us/180) {
    case 1:  // A half bit period since last transition
      if (tc.status & SECOND_HALF) {  // 1-bit completed
        tc.bitbuffer >>= 1;
        tc.bitbuffer |= 0x8000;
        tc.status += 1;  // increments the bit count which will overflow into WORD_FULL
        if (tc.bitbuffer == TimecodeReaderImpl::sync_word) {
          tc.status = FRAME_SYNC | WORD_FULL;  // resets BIT_COUNT and SECOND_HALF
        }
        if (tc.status & WORD_FULL) {
          tc.data = tc.bitbuffer;
          tc.ready = (tc.status & FRAME_SYNC) != 0;
          tc.status &= FRAME_SYNC;  // clear all but FRAME_SYNC
        } else {
          tc.status &= ~SECOND_HALF;
        }
      } else { // Wait for the rest of this bit period.
        tc.status |= SECOND_HALF;
      }
      return;

    case 2:  // A full bit period since last transition
      if (tc.status & SECOND_HALF) break; // Expected the second half of a 1-bit.

      // We have a zero.
      tc.bitbuffer >>= 1;
      tc.status += 1;  // increments the bit count which will overflow into WORD_FULL
      if (tc.status & WORD_FULL) {
        tc.data  = tc.bitbuffer;
        tc.ready = (tc.status & FRAME_SYNC) != 0;
        tc.status &= FRAME_SYNC;  // clear all but FRAME_SYNC
      }
      return;
  }

  // We lost bit synchronization.
  tc.status = 0;
  tc.bitbuffer = 0;
}
