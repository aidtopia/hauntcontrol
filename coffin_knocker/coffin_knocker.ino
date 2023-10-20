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

int constexpr motion_pin  = 2;  // input, HIGH indicates motion
int constexpr solenoid_pin = 9;  // output, HIGH opens the valve

template <int BUFFER_SIZE>
class CommandBuffer {
  public:
    void begin() {
      memset(buf, '\0', sizeof(buf));
      buflen = 0;
    }

    bool available() {
      while (Serial.available()) {
        char ch = Serial.read();
        if (buflen == sizeof(buf)) buflen = 0;
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
    int buflen = 0;
};

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

    int parseInteger() {
      while (Accept(' ')) {}
      const bool neg = Accept('-');
      if (!neg) Accept('+');
      const auto result = static_cast<int>(parseUnsigned());
      return neg ? -result : result;
    }

    unsigned parseUnsigned() {
      while (Accept(' ')) {}
      unsigned result = 0;
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

    bool AllowSuffix() { return Accept(' ') || !MatchLetter(); }

    template <typename ... T>
    bool AllowSuffix(char head, T... tail) {
      return !MatchLetter() || (Accept(head) && AllowSuffix(tail...));
    }

    const char *m_p;
};

enum class Keyword {
  UNKNOWN, CLEAR, DEFAULTS, EEPROM, HELP, LIST, LOAD, RUN, SAVE, TRIGGER
};

class CoffinKnockerParser : public Parser {
  public:
    CoffinKnockerParser(char const *buffer) : Parser(buffer) {}

    Keyword parseKeyword() {
      // A hand-rolled TRIE.  This may seem crazy, but many
      // microcontrollers have very limited RAM.  A data-driven
      // solution would require a table in RAM, so that's a
      // non-starter.  Instead, we "unroll" what the data-driven
      // solution would do so that it gets coded directly into
      // program memory.  Believe it or not, the variadic template
      // for AllowSuffix actually reduces the amount of code
      // generated versus a completely hand-rolled expression.
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
      if (Accept('S')) return AllowSuffix(Keyword::SAVE, 'A', 'V', 'E');
      if (Accept('T')) return AllowSuffix(Keyword::TRIGGER, 'R', 'I', 'G', 'G', 'E', 'R');
      return Keyword::UNKNOWN;
    }

  private:
    template <typename ... T>
    Keyword AllowSuffix(Keyword kw, T... suffix) {
      return Parser::AllowSuffix(suffix...) ? kw : Keyword::UNKNOWN;
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
  Serial.println(F(" TRIGGER       - starts the sequence as if motion detected"));
  if (!params.sane()) {
    Serial.println(F("\n* There is a problem with the current settings. The RUN"));
    Serial.println(F("command will not work until the problem is solved."));
    Serial.println(F("If you don't know how to solve the problem, try this:"));
    Serial.println(F(" LOAD DEFAULTS\n SAVE\n RUN"));
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
      }
      break;
    case Keyword::RUN:
      if (!run(millis())) break;
      return;
    case Keyword::SAVE:
      if (parser.parseKeyword() != Keyword::EEPROM) break;
      if (!save_to_eeprom()) break;
      return;
    case Keyword::TRIGGER:
      if (!trigger(millis())) break;
      return;
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
  Serial.println(F("\nCoffin Knocker"));

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
