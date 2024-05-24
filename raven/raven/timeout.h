#include <limits.h>

struct MillisClock { static decltype(millis()) now() { return millis(); } };
struct MicrosClock { static decltype(micros()) now() { return micros(); } };

template <class Clock = MillisClock>
class Timeout {
  public:
    // Implementation assumes that the type used by the clock is
    // an unsigned integral type.
    typedef decltype(Clock::now()) TimeRep;
  
    Timeout() : m_expires(0) {}

    void cancel() { m_expires = 0; }

    bool expired() const {
      if (m_expires == 0) return false;
      const auto now = Clock::now();
      if (now < m_expires) return false;
      // It looks like the timer had expired, but we have to
      // check for rollover first.  If MSB of m_expires is 0
      // and the MSB of now is 1, then we need to wait for the
      // clock to roll over.  The conditions above mean we
      // already know now >= m_expires, so this boils down to
      // whether the MSBs are different, thus the exclusive-or.
      return !((now ^ m_expires) & MSB_MASK);
    }
    
    // `delta` must be less than half of the range of a TimeRep.
    void set(TimeRep delta) {
      const auto now = Clock::now();
      m_expires = now + delta;
      if (m_expires == 0) {
        // Since we use `m_expires == 0` to mean "no timeout,"
        // we'll fudge it by one clock tick when rollover
        // would cause the expiration time to be exactly 0.
        m_expires = 1;
      }
    }

  private:
    TimeRep m_expires;
    static constexpr TimeRep MSB_MASK =
      static_cast<TimeRep>(1) << (CHAR_BIT * sizeof(TimeRep) - 1);
};
