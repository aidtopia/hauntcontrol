// Coffin Knocker Replacement Controller
// Adrian McCarthy 2023-10-04

// My Dragon Vane Cove friends acquired a second- or third-hand
// "Coffin Knocker" prop originally built by Spooky F/X, a company that
// appears to be out of business.  The prop has a single-action pneumatic
// cylinder and a spring that causes the lid of the coffin to bang.
//
// The original control box required one 9-volt an eight AA batteries, which
// is a bit inconvenient.  Also, the microwave motion detector no longer
// functioned well.
//
// So I made a new controller to replace the original.
//
// * An air regulator from FrightProps.com with 1/4" NPT fittings.  The
//   original instructions suggested 20-25 psi.
// * A 12-volt 3-way solenoid valve with 1/4" NPT ports from U.S. Solid.
//   I crimped ferrule terminals to the wire ends of a DC barrel connector
//   pigtail along with a 1N4001 diode reverse biased to handle the back EMF
//   from the solenoid.  Once I determined the polarity of the LED soldered
//   into the housing, I wired it in so that the LED is on when the solenoid
//   is activated.
// * 1/4" tubing runs to the original cylinder in the prop.
// * A 12-VDC power supply connects to a protoboard with a DC barrel connector
// * An Arduino Pro Mini (clone) controls the show.
//   The clone has a crappy onboard voltage regulator, so a small generic
//   buck converter provides regulated 5 VDC to the VCC pin.
// * A MOSFET module acts as the switch for the solendoid power.  The module
//   is overkill, but cheap and simple.  The Arduino runs the solenoid from
//   digital pin 9.
// * The Arduino expects a logic high on a digital pin from any sort of motion
//   sensor.  I experimented with a microwave doppler radar sensor, which is
//   probably similar to the original motion detector.  But the detection area
//   is very large and hard to control, so I'll probably complete it with a
//   basic PIR motion sensor.

#include <EEPROM.h>

int constexpr trigger_pin  = 2;  // input, HIGH indicates motion
int constexpr solenoid_pin = 9;  // output, HIGH opens the valve

class Parameters {
  public:
    // The suspense time is how long (in milliseconds) after motion is detected
    // the action should begin.  A short delay after motion is detected may
    // help in surprising victims, but this can be set to 0.
    long suspense_time() const {
      return get(SUSPENSE_TIME);
    }

    // After the suspense time, the coffin will knock randomly for the
    // run time, which is selected randomly from a range.
    long random_run_time() const {
      return random(get(MIN_RUN_TIME), get(MAX_RUN_TIME));
    }

    // Each knock consists of a random on time...
    long random_knock_on_time() const {
      return random(get(MIN_KNOCK_ON_TIME), get(MAX_KNOCK_ON_TIME));
    }
    // ...followed by an off time.
    long random_knock_off_time() const {
      return random(get(MIN_KNOCK_OFF_TIME), get(MAX_KNOCK_OFF_TIME));
    }

    // After a sequence, motion triggers are ignored for the lockout time.
    long lockout_time() const {
      return get(LOCKOUT_TIME);
    }

    bool load_defaults() {
      m_version = current_version;
      set(SUSPENSE_TIME,         0);
      set(MIN_RUN_TIME,       2500);
      set(MAX_RUN_TIME,       6000);
      set(MIN_KNOCK_ON_TIME,   200);
      set(MAX_KNOCK_ON_TIME,   400);
      set(MIN_KNOCK_OFF_TIME,  200);
      set(MAX_KNOCK_OFF_TIME,  900);
      set(LOCKOUT_TIME,       6000);
      return true;
    }

    bool load_from_eeprom(int base_address = 0) {
      auto addr = base_address;
      int signature = 0;
      EEPROM.get(addr, signature); addr += sizeof(signature);
      if (signature != magic) return false;  // never been saved

      EEPROM.get(addr, m_version); addr += sizeof(m_version);
      if (m_version <= 0) return false;

      auto *p = reinterpret_cast<uint8_t *>(m_values);
      for (unsigned i = 0; i < sizeof(m_values); ++i) {
        *p++ = EEPROM.read(addr++);
      }

      return m_version == current_version;
    }

    bool save_to_eeprom(int base_address = 0) const {
      auto addr = base_address;
      auto const signature = magic;
      EEPROM.put(addr, signature); addr += sizeof(signature);
      EEPROM.put(addr, m_version); addr += sizeof(m_version);
      auto const *p = reinterpret_cast<uint8_t const *>(m_values);
      for (unsigned i = 0; i < sizeof(m_values); ++i) {
        EEPROM.update(addr++, *p++);
      }
      return true;
    }

    void clear_eeprom(int base_address = 0) {
      // All we have to do is wipe out the magic number.
      auto const signature = (magic == 0) ? 1 : 0;
      EEPROM.put(base_address, signature);
    }

    bool sane() const {
      unsigned long constexpr one_minute = 60 * 1000;
      if (m_version != current_version) return false;
      if (get(SUSPENSE_TIME) < 0 || one_minute < get(SUSPENSE_TIME)) return false;
      if (get(MIN_RUN_TIME) < 1000) return false;
      if (get(MAX_RUN_TIME) < get(MIN_RUN_TIME) || one_minute < get(MAX_RUN_TIME)) return false;
      if (get(MIN_KNOCK_ON_TIME) < 200) return false;
      if (get(MAX_KNOCK_ON_TIME) < get(MIN_KNOCK_ON_TIME) || one_minute < get(MAX_KNOCK_ON_TIME)) return false;
      if (get(MIN_KNOCK_OFF_TIME) < 200) return false;
      if (get(MAX_KNOCK_OFF_TIME) < get(MIN_KNOCK_OFF_TIME) || one_minute < get(MAX_KNOCK_OFF_TIME)) return false;
      if (get(LOCKOUT_TIME) < 0 || one_minute < get(LOCKOUT_TIME)) return false;
      return true;
    }

    void show() const {
      for (int i = 0; i < COUNT; ++i) {
        Serial.print(F(" "));
        Serial.print(i+1);
        Serial.print(F(". "));
        char buffer[32] = "";
        load_name(i, buffer);
        Serial.print(buffer);
        Serial.print(F(" = "));
        Serial.println(m_values[i]);
      }
    }

  private:
    enum Index {
      // VERSION 1
      SUSPENSE_TIME,
      MIN_RUN_TIME,
      MAX_RUN_TIME,
      MIN_KNOCK_ON_TIME,
      MAX_KNOCK_ON_TIME,
      MIN_KNOCK_OFF_TIME,
      MAX_KNOCK_OFF_TIME,
      LOCKOUT_TIME,

      // FUTURE VERSIONS ADD INDEXES HERE

      // TOTAL NUMBER OF PARAMETERS
      COUNT
    };

    long get(Index i) const {
      return (0 <= i && i < COUNT) ? m_values[i] : 0L;
    }

    bool set(Index i, long value) {
      if (0 <= i && i < COUNT) {
        m_values[i] = value;
        return true;
      }
      return false;
    }

    static void load_name(Index i, char buffer[]) {
      if (0 <= i && i < COUNT) {
        auto const source =
          reinterpret_cast<char const *>(pgm_read_word(m_names + i));
        strcpy_P(buffer, source);
      } else {
        strcpy(buffer, "OOB");
      }
    }

    int m_version;
    long m_values[COUNT];

    static char const * const m_names[COUNT];
    static int constexpr current_version = 1;
    static int constexpr magic = 307;
} params;

char const NAME_Suspense[] PROGMEM = "Suspense";
char const NAME_Min_Run[]  PROGMEM = "Min Run";
char const NAME_Max_Run[]  PROGMEM = "Max Run";
char const NAME_Min_Knock_On[] PROGMEM = "Min Knock On";
char const NAME_Max_Knock_On[] PROGMEM = "Max Knock On";
char const NAME_Min_Knock_Off[] PROGMEM = "Min Knock Off";
char const NAME_Max_Knock_Off[] PROGMEM = "Max Knock Off";
char const NAME_Lockout[] PROGMEM = "Lockout";
char const * const Parameters::m_names[Parameters::COUNT] PROGMEM = {
  NAME_Suspense,
  NAME_Min_Run, NAME_Max_Run,
  NAME_Min_Knock_On, NAME_Max_Knock_On,
  NAME_Min_Knock_Off, NAME_Max_Knock_Off,
  NAME_Lockout
};

enum class State {
  initializing,

  // The normal operating states...
  waiting,
  suspense,
  knock_on,
  knock_off,
  lockout,

  // Meta states...
  programming
} state = State::initializing;

// timeout is the time the next state change should occur (millis).
unsigned long timeout = 0;
// knock_timeout is the time the current knock_on or knock_off ends.
unsigned long knock_timeout = 0;

static long eat_input() {
  while (Serial.available()) Serial.read();
}

void setup() {
  pinMode(solenoid_pin, OUTPUT);
  digitalWrite(solenoid_pin, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(trigger_pin, INPUT);
  Serial.begin(115200);
  while (!Serial) {}  // not necessary for Pro Mini, but no harm
  Serial.println(F("\nCoffin Knocker"));

  EEPROM.begin();

  int const seed = analogRead(A0);
  Serial.print(F("Random Seed: "));
  Serial.println(seed);
  randomSeed(seed);
  state = State::initializing;
}

void loop() {
  // The onboard LED always shows the current state of the motion sensor.
  // This can help when setting up the sensor.
  int const trigger = digitalRead(trigger_pin);
  digitalWrite(LED_BUILTIN, trigger);

  // Serial input always takes us to the programming state.
  if (Serial.available() && state != State::programming) {
    Serial.println(F("Programming Mode"));
    state = State::programming;
  }

  // We set the solenoid output every time through the loop, not just when
  // it changes.  This ensures we de-energize the solenoid valve if there's
  // ever an unexpected state change.
  digitalWrite(solenoid_pin, state == State::knock_on ? HIGH : LOW);

  unsigned long const now = millis();

  switch (state) {
    case State::initializing: {
      if (params.load_from_eeprom(0) && params.sane()) {
        Serial.println(F("Timing parameters loaded from EEPROM."));
        params.show();
        state = State::waiting;
        Serial.print(now);
        Serial.println(F(" Waiting..."));
        break;
      }
      if (params.load_defaults() && params.sane()) {
        Serial.println(F("Using default timing parameters."));
        params.show();
        state = State::waiting;
        Serial.print(now);
        Serial.println(F(" Waiting..."));
        break;
      }
      Serial.println(F("Timing parameters not available."));
      state = State::programming;
      break;
    }

    case State::waiting: {
      if (trigger == HIGH) {
        timeout = now + params.suspense_time();
        state = State::suspense;
        Serial.print(now);
        Serial.println(F(" Triggered"));
      }
      break;
    }

    case State::suspense: {
      if (now >= timeout) {
        if (trigger == LOW) {
          // This won't ever happen if the suspense_time is less than the
          // minimum time the sensor will signal a motion event.
          Serial.print(now);
          Serial.println(F(" Canceled"));
          state = State::waiting;
        } else {
          long const run_time = params.random_run_time();
          timeout = now + run_time;
          knock_timeout = now;
          state = State::knock_off;
          Serial.print(now);
          Serial.println(F(" Animating!"));
        }
      }
      break;
    }

    case State::knock_on: {
      if (now >= timeout) {
        timeout = now + params.lockout_time();
        state = State::lockout;
        Serial.print(now);
        Serial.println(F(" Lockout"));
        break;
      }
      if (now >= knock_timeout) {
        long const knock_off_time = params.random_knock_off_time();
        knock_timeout = now + knock_off_time;
        state = State::knock_off;
      }
      break;
    }

    case State::knock_off: {
      if (now >= timeout) {
        timeout = now + params.lockout_time();
        state = State::lockout;
        Serial.print(now);
        Serial.println(F(" Lockout"));
        break;
      }
      if (now >= knock_timeout) {
        long const knock_time = params.random_knock_on_time();
        knock_timeout = now + knock_time;
        state = State::knock_on;
        Serial.print(now);
        Serial.println(F(" Bang!"));
      }
      break;
    }

    case State::lockout: {
      if (now >= timeout) {
        state = State::waiting;
        Serial.print(now);
        Serial.println(F(" Waiting..."));
      }
      break;
    }

    case State::programming: {
      if (!Serial.available()) break;
      int ch = Serial.read();
      if (ch == '\n' || ch == '\r' || ch == ' ' || ch == '\t') break;
      if ('a' <= ch && ch <= 'z') ch = ch - 'a' + 'A';
      if (' ' < ch && ch < 127) Serial.println((char) ch); else Serial.println(ch);

      switch (ch) {
        case 'C':
          params.clear_eeprom();
          break;
        case 'D':
          params.load_defaults();
          [[fallthrough]]
        case 'L':
          params.show();
          break;
        case 'S':
          if (params.sane() && params.save_to_eeprom()) {
            state = State::waiting;
          } else {
            Serial.println(F("Could not save settings to EEPROM."));
          }
          break;
        case 'R':
          state = params.sane() ? State::waiting : State::initializing;
          break;
      }
      eat_input();
      if (state == State::programming) {
        Serial.print(F("(L)ist, (D)efaults, (S)ave, (C)lear, (R)un: "));
        Serial.flush();
      }
      if (state == State::waiting) {
        Serial.println(F("Waiting..."));
        eat_input();  // seems redundant, but it makes a difference
      }
      break;
    }
  }
}
