// Coffin Knocker Replacement Controller
// Adrian McCarthy 2023-10-04

// My Dragon Vane Cove friends acquired a second- or third-hand
// "Coffin Knocker" prop originally built by Spooky F/X, a company that
// appears to be out of business.  The prop has a single-action pneumatic
// cylinder and a spring that causes the lid of the coffin to bang.
//
// The original control box required one 9-volt and eight AA batteries, which
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

int constexpr motion_pin  = 2;  // input, HIGH indicates motion
int constexpr solenoid_pin = 9;  // output, HIGH opens the valve

template <uint8_t BUFFER_SIZE>
class CommandBuffer {
  public:
    void begin() {
      memset(buf, '\0', BUFFER_SIZE);
      buflen = 0;
    }

    bool available() {
      while (Serial.available()) {
        char ch = Serial.read();
        if (buflen == BUFFER_SIZE) buflen = 0;
        switch (ch) {
          case '\r': break;
          case '\n':
            buf[buflen] = '\0';
            buflen = 0;
            return true;
          case ' ': case '\t':
            if (buflen == 0) break;
            if (buf[buflen - 1] == ' ') break;
            buf[buflen++] = ' ';
            break;
          default:
            if (ch < ' ') break;
            if ('a' <= ch && ch <= 'z') ch = ch - 'a' + 'A';
            buf[buflen++] = ch;
            break;
        }
      }
      return false;
    }

    operator const char *() const { return buf; }

  private:
    char buf[BUFFER_SIZE];
    uint8_t buflen = 0;
};

template <uint8_t COUNT, int MAGIC, int VERSION>
class Parameters {
  public:
    bool load_defaults() {
      memset(m_values, 0, sizeof(m_values));
      if (!do_load_defaults()) {
        return false;
      }
      m_version = VERSION;
      return true;
    }

    bool sane() const {
      return (m_version == VERSION) && do_sane();
    }

    bool load_from_eeprom(int base_address = 0) {
      auto addr = base_address;
      int signature = 0;
      EEPROM.get(addr, signature); addr += sizeof(signature);
      if (signature != MAGIC) return false;  // never been saved

      EEPROM.get(addr, m_version); addr += sizeof(m_version);
      if (m_version <= 0) return false;

      auto *p = reinterpret_cast<uint8_t *>(m_values);
      for (unsigned i = 0; i < sizeof(m_values); ++i) {
        *p++ = EEPROM.read(addr++);
      }

      return m_version == VERSION;
    }

    bool save_to_eeprom(int base_address = 0) const {
      auto addr = base_address;
      auto const signature = MAGIC;
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
      int const signature = !MAGIC;
      EEPROM.put(base_address, signature);
    }

    void print_name(uint8_t i) {
      if (i >= COUNT) return;
      do_print_name(i);
    }

    void show() const {
      for (uint8_t i = 0; i < COUNT; ++i) {
        Serial.print(F(" "));
        do_print_name(i);
        Serial.print(F(" = "));
        Serial.println(m_values[i]);
      }
    }

  protected:
    long get_by_index(uint8_t i) const {
      return (0 <= i && i < COUNT) ? m_values[i] : 0L;
    }

    bool set_by_index(uint8_t i, long value) {
      if (i < COUNT) {
        m_values[i] = value;
        return true;
      }
      return false;
    }

  private:
    virtual bool do_load_defaults() = 0;
    virtual bool do_sane() const = 0;
    virtual void do_print_name(uint8_t i) const = 0;

    int m_version;
    long m_values[COUNT];
};

enum class Parameter : uint8_t {
  // VERSION 1
  SUSPENSE_TIME,
  MIN_RUN_TIME,
  MAX_RUN_TIME,
  MIN_KNOCK_ON_TIME,
  MAX_KNOCK_ON_TIME,
  MIN_KNOCK_OFF_TIME,
  MAX_KNOCK_OFF_TIME,
  LOCKOUT_TIME,

  COUNT
};

class CoffinKnockerParameters :
  public Parameters<static_cast<uint8_t>(Parameter::COUNT), 307, 1>
{
  public:
    // The suspense time is how long (in milliseconds) after motion is detected
    // the action should begin.  A short delay after motion is detected may
    // help in surprising victims, but this can be set to 0.
    long suspense_time() const {
      return get(Parameter::SUSPENSE_TIME);
    }

    // After the suspense time, the coffin will knock randomly for the
    // run time, which is selected randomly from a range.
    long random_run_time() const {
      return random(get(Parameter::MIN_RUN_TIME), get(Parameter::MAX_RUN_TIME));
    }

    // Each knock consists of a random on time ...
    long random_knock_on_time() const {
      return random(get(Parameter::MIN_KNOCK_ON_TIME), get(Parameter::MAX_KNOCK_ON_TIME));
    }
    // ... followed by an off time.
    long random_knock_off_time() const {
      return random(get(Parameter::MIN_KNOCK_OFF_TIME), get(Parameter::MAX_KNOCK_OFF_TIME));
    }

    // After a sequence, motion triggers are ignored for the lockout time.
    long lockout_time() const {
      return get(Parameter::LOCKOUT_TIME);
    }

    long get(Parameter p) const { return get_by_index(static_cast<uint8_t>(p)); }
    bool set(Parameter p, long value) { return set_by_index(static_cast<uint8_t>(p), value); }

  private:
    bool do_load_defaults() override {
      set(Parameter::SUSPENSE_TIME,         0);
      set(Parameter::MIN_RUN_TIME,       2500);
      set(Parameter::MAX_RUN_TIME,       6000);
      set(Parameter::MIN_KNOCK_ON_TIME,   200);
      set(Parameter::MAX_KNOCK_ON_TIME,   400);
      set(Parameter::MIN_KNOCK_OFF_TIME,  200);
      set(Parameter::MAX_KNOCK_OFF_TIME,  900);
      set(Parameter::LOCKOUT_TIME,       6000);
      return true;
    }

    bool do_sane() const override {
      long constexpr one_minute = 60 * 1000L;
      if (get(Parameter::SUSPENSE_TIME) < 0 || one_minute < get(Parameter::SUSPENSE_TIME)) return false;
      if (get(Parameter::MIN_RUN_TIME) < 1000) return false;
      if (get(Parameter::MAX_RUN_TIME) < get(Parameter::MIN_RUN_TIME) || one_minute < get(Parameter::MAX_RUN_TIME)) return false;
      if (get(Parameter::MIN_KNOCK_ON_TIME) < 200) return false;
      if (get(Parameter::MAX_KNOCK_ON_TIME) < get(Parameter::MIN_KNOCK_ON_TIME) || one_minute < get(Parameter::MAX_KNOCK_ON_TIME)) return false;
      if (get(Parameter::MIN_KNOCK_OFF_TIME) < 200) return false;
      if (get(Parameter::MAX_KNOCK_OFF_TIME) < get(Parameter::MIN_KNOCK_OFF_TIME) || one_minute < get(Parameter::MAX_KNOCK_OFF_TIME)) return false;
      if (get(Parameter::LOCKOUT_TIME) < 0 || one_minute < get(Parameter::LOCKOUT_TIME)) return false;
      return true;
    }

    void do_print_name(uint8_t i) const override {
      auto source =
        reinterpret_cast<char const *>(pgm_read_word(m_names + i));
      for (auto ch = pgm_read_byte_near(source++); ch != '\0'; ch = pgm_read_byte_near(source++)) {
        Serial.print(static_cast<char>(ch));
      }
    }

    static char const * const m_names[];
} params;

char const NAME_SUSPENSE[] PROGMEM = "SUSPENSE";
char const NAME_MIN_RUN[]  PROGMEM = "MIN_RUN";
char const NAME_MAX_RUN[]  PROGMEM = "MAX_RUN";
char const NAME_MIN_KNOCK_ON[] PROGMEM = "MIN_KNOCK_ON";
char const NAME_MAX_KNOCK_ON[] PROGMEM = "MAX_KNOCK_ON";
char const NAME_MIN_KNOCK_OFF[] PROGMEM = "MIN_KNOCK_OFF";
char const NAME_MAX_KNOCK_OFF[] PROGMEM = "MAX_KNOCK_OFF";
char const NAME_LOCKOUT[] PROGMEM = "LOCKOUT";
char const * const CoffinKnockerParameters::m_names[] PROGMEM = {
  NAME_SUSPENSE,
  NAME_MIN_RUN, NAME_MAX_RUN,
  NAME_MIN_KNOCK_ON, NAME_MAX_KNOCK_ON,
  NAME_MIN_KNOCK_OFF, NAME_MAX_KNOCK_OFF,
  NAME_LOCKOUT
};

enum class State : uint8_t {
  // The normal operating states...
  waiting,
  suspense,
  knock_on,
  knock_off,
  lockout,

  // Meta states...
  programming
} state = State::programming;

// timeout is the time the next state change should occur (millis).
unsigned long timeout = 0;
// knock_timeout is the time the current knock_on or knock_off ends.
unsigned long knock_timeout = 0;

CommandBuffer<32> command;

class Parser {
  public:
    explicit Parser(char const * buffer) : m_p(buffer) {}

    long parseLong() {
      while (Accept(' ')) {}
      const bool neg = Accept('-');
      if (!neg) Accept('+');
      const auto result = static_cast<long>(parseUnsignedLong());
      return neg ? -result : result;
    }

    unsigned long parseUnsignedLong() {
      while (Accept(' ')) {}
      unsigned long result = 0;
      while (MatchDigit()) {
        result *= 10;
        result += *m_p - '0';
        Advance();
      }
      return result;
    }

    bool Advance() { ++m_p; return true; }

    bool Accept(char ch) { return (*m_p == ch) ? Advance() : false; }
    bool MatchDigit() const { return '0' <= *m_p && *m_p <= '9'; }
    bool MatchLetter() const {
        return ('A' <= *m_p && *m_p <= 'Z') ||
               ('a' <= *m_p && *m_p <= 'z');
    }


    bool Expect() { return true; }

    template <typename ... T>
    bool Expect(char head, T... tail) { return Accept(head) && Expect(tail...); }

    bool AllowSuffix() { return Accept(' ') || !MatchLetter(); }

    template <typename ... T>
    bool AllowSuffix(char head, T... tail) {
      if (Accept(head)) return AllowSuffix(tail...);
      return Accept(' ') || !MatchLetter();
    }

    const char *m_p;
};

enum class Keyword : uint8_t {
  UNKNOWN, CLEAR, DEFAULTS, EEPROM, HELP, LIST, LOAD, RUN, SAVE, SET
};

class CoffinKnockerParser : public Parser {
  public:
    CoffinKnockerParser(char const *buffer) : Parser(buffer) {}

    Keyword parseKeyword() {
      if (Accept('C')) return AllowSuffix(Keyword::CLEAR, 'L', 'E', 'A', 'R');
      if (Accept('D')) return AllowSuffix(Keyword::DEFAULTS, 'E', 'F', 'A', 'U', 'L', 'T', 'S');
      if (Accept('E')) {
        Accept('E');  // Allow "EEPROM" or "EPROM"
        return AllowSuffix(Keyword::EEPROM, 'P', 'R', 'O', 'M');
      }
      if (Accept('H')) return AllowSuffix(Keyword::HELP, 'E', 'L', 'P');
      if (Accept('L')) {
        if (Accept('I')) return AllowSuffix(Keyword::LIST, 'S', 'T');
        if (Accept('O')) return AllowSuffix(Keyword::LOAD, 'A', 'D');
        return Keyword::UNKNOWN;
      }
      if (Accept('R')) return AllowSuffix(Keyword::RUN, 'U', 'N');
      if (Accept('S')) {
        if (Accept('A')) return AllowSuffix(Keyword::SAVE, 'V', 'E');
        if (Accept('E')) return AllowSuffix(Keyword::SET, 'T');
        return Keyword::UNKNOWN;
      }
      return Keyword::UNKNOWN;
    }

    Parameter parseParameter() {
      if (Accept('L')) return AllowSuffix(Parameter::LOCKOUT_TIME, 'O', 'C', 'K', 'O', 'U', 'T');
      if (Accept('M')) {
        if (Accept('A')) {
          if (Expect('X', '_')) {
            if (Accept('K')) {
              if (Expect('N', 'O', 'C', 'K', '_', 'O')) {
                if (Accept('F')) return AllowSuffix(Parameter::MAX_KNOCK_OFF_TIME, 'F');
                if (Accept('N')) return AllowSuffix(Parameter::MAX_KNOCK_ON_TIME);
                return Parameter::COUNT;
              }
              return Parameter::COUNT;
            }
            if (Accept('R')) return AllowSuffix(Parameter::MAX_RUN_TIME, 'U', 'N');
            return Parameter::COUNT;
          }
          return Parameter::COUNT;
        }
        if (Accept('I')) {
          if (Expect('N', '_')) {
            if (Accept('K')) {
              if (Expect('N', 'O', 'C', 'K', '_', 'O')) {
                if (Accept('F')) return AllowSuffix(Parameter::MIN_KNOCK_OFF_TIME, 'F');
                if (Accept('N')) return AllowSuffix(Parameter::MIN_KNOCK_ON_TIME);
                return Parameter::COUNT;
              }
              return Parameter::COUNT;
            }
            if (Accept('R')) return AllowSuffix(Parameter::MIN_RUN_TIME, 'U', 'N');
            return Parameter::COUNT;
          }
          return Parameter::COUNT;
        }
        return Parameter::COUNT;
      }
      if (Accept('S')) return AllowSuffix(Parameter::SUSPENSE_TIME, 'U', 'S', 'P', 'E', 'N', 'S', 'E');
      return Parameter::COUNT;
    }

  private:
    template <typename ... T>
    Keyword AllowSuffix(Keyword kw, T... suffix) {
      return Parser::AllowSuffix(suffix...) ? kw : Keyword::UNKNOWN;
    }

    template <typename ... T>
    Parameter AllowSuffix(Parameter p, T... suffix) {
      return Parser::AllowSuffix(suffix...) ? p : Parameter::COUNT;
    }
};

static void help() {
  if (state == State::programming) {
    Serial.println(F("The prop is paused so that you can change the settings."));
  }
  Serial.println(F("\nCommands:"));
  Serial.println(F(" CLEAR EEPROM  - clears any settings currently saved in EEPROM"));
  Serial.println(F(" HELP          - shows these instructions"));
  Serial.println(F(" LIST          - shows current settings"));
  Serial.println(F(" LOAD DEFAULTS - sets current settings to factory defaults"));
  Serial.println(F(" LOAD EEPROM   - loads previously saved settings"));
  Serial.println(F(" RUN           - runs the prop with the current settings"));
  Serial.println(F(" SAVE EEPROM   - saves current settings to EEPROM"));
  Serial.println(F(" SET <setting>=<value>"));
  if (!params.sane()) {
    Serial.println(F("\n* There is a problem with the current settings. The RUN"));
    Serial.println(F("command will not work until the problem is solved."));
    Serial.println(F("If you don't know how to solve the problem, try this:"));
    Serial.println(F(" LOAD DEFAULTS\n SAVE EEPROM\n RUN"));
  }
}

static void clear() {
  params.clear_eeprom();
  Serial.println(F("Settings stored in EEPROM have been cleared."));
}

static void list() {
  params.show();
  if (!params.sane()) {
    Serial.println(F("\n* Cannot run with these settings."));
  }
}

static bool load_defaults() {
  if (!params.load_defaults()) {
    Serial.println(F("Could not load defaults."));
    return false;
  }
  Serial.println(F("Loaded default settings."));
  list();
  return true;
}

static bool load_from_eeprom() {
  if (!params.load_from_eeprom()) {
    Serial.println(F("Could not load settings from EEPROM."));
    return false;
  }
  Serial.println(F("Loaded settings from EEPROM."));
  list();
  return true;
}

static bool run(unsigned long now) {
  if (!params.sane()) {
    Serial.println(F("Cannot run with current settings."));
    state = State::programming;
    return false;
  }

  state = State::waiting;
  Serial.print(now);
  Serial.println(F(" Waiting..."));
  return true;
}

static bool save_to_eeprom() {
  if (!(params.sane() && params.save_to_eeprom())) {
    Serial.println(F("Could not save settings to EEPROM."));
    return false;
  }
  return true;  
}

static bool trigger(unsigned long now) {
  if (!params.sane()) {
    Serial.println(F("Cannot run with current settings."));
    state = State::programming;
    return false;
  }
  timeout = now + params.suspense_time();
  state = State::suspense;
  Serial.print(now);
  Serial.println(F(" Triggered"));
  return true;
}

static void execute_command(char const *command) {
  CoffinKnockerParser parser(command);
  switch (parser.parseKeyword()) {
    case Keyword::HELP:  help();  return;
    case Keyword::CLEAR:
      if (parser.parseKeyword() != Keyword::EEPROM) break;
      clear();
      return;
    case Keyword::LIST:  list();  return;
    case Keyword::LOAD:
      switch (parser.parseKeyword()) {
        case Keyword::DEFAULTS:
          if (!load_defaults()) break;
          return;
        case Keyword::EEPROM:
          if (!load_from_eeprom()) break;
          return;
        default:
          break;
      }
      break;
    case Keyword::RUN:
      if (!run(millis())) break;
      return;
    case Keyword::SAVE:
      if (parser.parseKeyword() != Keyword::EEPROM) break;
      if (!save_to_eeprom()) break;
      return;
    case Keyword::SET: {
      Parameter const p = parser.parseParameter();
      if (p == Parameter::COUNT) break;
      parser.Accept(':'); parser.Accept('='); parser.Accept(' ');
      if (!parser.MatchDigit()) break;
      long value = parser.parseLong();
      if (!params.set(p, value)) break;
      list();
      return;
    }
    default: break;
  }
  Serial.println(F("Type HELP for detailed instructions."));
}

void setup() {
  pinMode(solenoid_pin, OUTPUT);
  digitalWrite(solenoid_pin, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(motion_pin, INPUT);
  Serial.begin(115200);
  while (!Serial) {}  // not necessary for Pro Mini, but no harm
  Serial.println(F("\nCoffin Knocker 2025"));

  EEPROM.begin();

  int const seed = analogRead(A0);
  Serial.print(F("Random Seed: "));
  Serial.println(seed);
  randomSeed(seed);

  if (!((load_from_eeprom() || load_defaults()) && run(millis()))) {
    help();
  }
}

void loop() {
  // The onboard LED always shows the current state of the motion sensor.
  // This can help when setting up the sensor.
  int const motion = digitalRead(motion_pin);
  digitalWrite(LED_BUILTIN, motion);

  if (command.available()) {
    if (state != State::programming) {
      state = State::programming;
      Serial.print(F("> "));
    }
    Serial.println(command);
    execute_command(command);
    if (state == State::programming) {
      Serial.print(F("> "));
      Serial.flush();
    }
  }

  // We set the solenoid output every time through the loop, not just when
  // it changes.  This ensures we de-energize the solenoid valve if there's
  // ever an unexpected state change.
  digitalWrite(solenoid_pin, state == State::knock_on ? HIGH : LOW);

  unsigned long const now = millis();

  switch (state) {
    case State::waiting: {
      if (motion == HIGH) trigger(now);
      break;
    }

    case State::suspense: {
      if (now >= timeout) {
        if (motion == LOW) {
          // This won't ever happen if the suspense_time is less than the
          // minimum time the sensor will signal a motion event.
          state = State::waiting;
          Serial.print(now);
          Serial.println(F(" Canceled"));
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
      break;
    }
  }
}
