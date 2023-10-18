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

int constexpr trigger_pin = 2;   // input, HIGH indicates motion
int constexpr solenoid_pin = 9;  // output, HIGH opens the valve

// These parameters control the action.
struct TimingParameters {
  int version;

  // VERSION 1

  // The suspense_time is how long (in milliseconds) after motion is detected
  // the action should begin.  A short delay after motion is detected may help
  // in surprising victims, but this can be set to 0.
  long suspense_time;

  // After the suspense_time, the coffin will knock randomly for a total
  // amount of time in this range.
  long min_run_time;
  long max_run_time;

  // Each knock will engage the solenoid for a random time in this range.
  long min_knock_time;
  long max_knock_time;

  // Amount of time between knocks.
  long min_noknock_time;
  long max_noknock_time;

  // After the sequence plays, we lock out re-triggering to discourage folks
  // from lingering to see it again.
  long lockout_time;

  static int constexpr current_version = 1;
  static int constexpr magic = 307;

  bool load_defaults() {
    version = current_version;
    suspense_time    =     0;
    min_run_time     =  2500;
    max_run_time     =  6000;
    min_knock_time   =   200;
    max_knock_time   =   400;
    min_noknock_time =   200;
    max_noknock_time =   900;
    lockout_time     =  6000;
    return true;
  }

  bool load_from_eeprom(int base_address = 0) {
    int signature = 0;
    EEPROM.get(base_address, signature);
    if (signature != magic) return false;  // never been saved
    auto address = base_address + sizeof(signature);
    EEPROM.get(address, version); address += sizeof(version);
    if (version == 0 || version > current_version) return false;
    EEPROM.get(address, suspense_time); address += sizeof(suspense_time);
    EEPROM.get(address, min_run_time); address += sizeof(min_run_time);
    EEPROM.get(address, max_run_time); address += sizeof(max_run_time);
    EEPROM.get(address, min_knock_time); address += sizeof(min_knock_time);
    EEPROM.get(address, max_knock_time); address += sizeof(max_knock_time);
    EEPROM.get(address, min_noknock_time); address += sizeof(min_noknock_time);
    EEPROM.get(address, max_noknock_time); address += sizeof(max_noknock_time);
    EEPROM.get(address, lockout_time); address += sizeof(lockout_time);

    return version == current_version;
  }

  bool save_to_eeprom(int base_address = 0) {
    auto const signature = magic;
    auto address = base_address + sizeof(signature);
    EEPROM.put(address, version); address += sizeof(version);
    EEPROM.put(address, suspense_time); address += sizeof(suspense_time);
    EEPROM.put(address, min_run_time); address += sizeof(min_run_time);
    EEPROM.put(address, max_run_time); address += sizeof(max_run_time);
    EEPROM.put(address, min_knock_time); address += sizeof(min_knock_time);
    EEPROM.put(address, max_knock_time); address += sizeof(max_knock_time);
    EEPROM.put(address, min_noknock_time); address += sizeof(min_noknock_time);
    EEPROM.put(address, max_noknock_time); address += sizeof(max_noknock_time);
    EEPROM.put(address, lockout_time); address += sizeof(lockout_time);
    EEPROM.put(base_address, signature);
    return true;
  }

  bool sane() const {
    if (version != current_version) return false;
    if (suspense_time < 0 || 60000 < suspense_time) return false;
    if (min_run_time < 1000) return false;
    if (max_run_time < min_run_time || 60000 < max_run_time) return false;
    if (min_knock_time < 200) return false;
    if (max_knock_time < min_knock_time || 60000 < max_knock_time) return false;
    if (min_noknock_time < 200) return false;
    if (max_noknock_time < min_noknock_time || 60000 < max_noknock_time) return false;
    if (lockout_time < 0 || 60000 < lockout_time) return false;
    return true;
  }

  void show() const {
    Serial.print(F(" Version:  ")); Serial.println(version);
    Serial.print(F(" Suspense: ")); Serial.println(suspense_time);
    Serial.print(F(" Run:      ")); Serial.print(min_run_time); Serial.print(F(" - ")); Serial.println(max_run_time);
    Serial.print(F(" Knock:    ")); Serial.print(min_knock_time); Serial.print(F(" - ")); Serial.println(max_knock_time);
    Serial.print(F(" No Knock: ")); Serial.print(min_noknock_time); Serial.print(F(" - ")); Serial.println(max_noknock_time);
    Serial.print(F(" Lockout:  ")); Serial.println(lockout_time);
  }
} timing;

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

unsigned long knock_timeout = 0;

static long eat_input() {
  while (Serial.available()) Serial.read();
}

static long pick_time(long low, long high) {
  return random(low, high);
}

void setup() {
  Serial.begin(115200);
  pinMode(solenoid_pin, OUTPUT);
  digitalWrite(solenoid_pin, LOW);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(trigger_pin, INPUT);
  while (!Serial) {}  // not necessary for Pro Mini, but no harm
  Serial.println(F("\nCoffin Knocker"));
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
      if (timing.load_from_eeprom(0) && timing.sane()) {
        Serial.println(F("Timing parameters loaded from EEPROM."));
        timing.show();
        state = State::waiting;
        Serial.print(now);
        Serial.println(F(" Waiting..."));
        break;
      }
      if (timing.load_defaults() && timing.sane()) {
        Serial.println(F("Using default timing parameters."));
        timing.show();
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
        timeout = now + timing.suspense_time;
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
          long const run_time = pick_time(timing.min_run_time, timing.max_run_time);
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
        timeout = now + timing.lockout_time;
        state = State::lockout;
        Serial.print(now);
        Serial.println(F(" Lockout"));
        break;
      }
      if (now >= knock_timeout) {
        long const noknock_time = pick_time(timing.min_noknock_time, timing.max_noknock_time);
        knock_timeout = now + noknock_time;
        state = State::knock_off;
      }
      break;
    }

    case State::knock_off: {
      if (now >= timeout) {
        timeout = now + timing.lockout_time;
        state = State::lockout;
        Serial.print(now);
        Serial.println(F(" Lockout"));
        break;
      }
      if (now >= knock_timeout) {
        long const knock_time = pick_time(timing.min_knock_time, timing.max_knock_time);
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
        case 'D':
          timing.load_defaults();
          [[fallthrough]]
        case 'L':
          timing.show();
          break;
        case 'S':
          if (timing.sane() && timing.save_to_eeprom()) {
            state = State::waiting;
          } else {
            Serial.println(F("Could not save settings to EEPROM."));
          }
          break;
        case 'R':
          state = timing.sane() ? State::waiting : State::initializing;
          break;
      }
      eat_input();
      if (state == State::programming) {
        Serial.print(F("(L)ist, restore (D)efaults, (S)ave, (R)un: "));
      }
      if (state == State::waiting) {
        Serial.println(F("Waiting..."));
        eat_input();  // seems redundant, but it makes a difference
      }
      break;
    }
  }
}
