// Coffin Knocker Replacement Controller
// Adrian McCarthy 2023-10-04

// My Dragon Vane Cove friends acquired a second- or third-hand
// "Coffin Knocker" prop originally built by Spooky F/X, a company that
// appears to be out of business.  The prop has a single-action pneumatic
// cylinder that causes the lid of the coffin to bang.
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

int constexpr trigger_pin = 2;   // input, HIGH indicates motion
int constexpr solenoid_pin = 9;  // output, HIGH opens the valve

// These parameters control the action.
// The suspense_time is how long (in milliseconds) after motion is detected the
// action should begin.  A short delay after motion is detected may help in
// surprising victims, but this can be set to 0.
long constexpr suspense_time = 1000;

// After the suspense_time, the coffin will knock randomly for a total amount of
// time in this range.
long constexpr min_run_time =  3000;
long constexpr max_run_time = 10000;

// Each knock will engage the solenoid for a random time in this range.
long constexpr min_knock_time = 100;
long constexpr max_knock_time = 400;

// Amount of time between knocks.
long constexpr min_noknock_time = 200;
long constexpr max_noknock_time = 900;

// After the sequence plays, we lock out re-triggering to discourage folks
// lingering to see it again.
long constexpr lockout_time = 3000;


enum class State {
  waiting,
  suspense,
  playing,
  lockout
} state = State::waiting;

// timeout is the time the next state change should occur (millis).
unsigned long timeout = 0;

bool knock_on = false;
unsigned long knock_timeout = 0;


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
  int seed = analogRead(A0);
  Serial.print(F("Random Seed: "));
  Serial.println(seed);
  randomSeed(seed);
}

void loop() {
  // The onboard LED always shows the current state of the motion sensor.
  // This can help when setting up the sensor.
  int const trigger = digitalRead(trigger_pin);
  digitalWrite(LED_BUILTIN, trigger);

  // We set the solenoid output every time through the loop, not just when
  // it changes.
  digitalWrite(solenoid_pin, knock_on ? HIGH : LOW);

  unsigned long const now = millis();

  switch (state) {
    case State::waiting: {
      if (trigger == HIGH) {
        timeout = now + suspense_time;
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
          long const run_time = pick_time(min_run_time, max_run_time);
          timeout = now + run_time;
          knock_timeout = now;
          state = State::playing;
          Serial.print(now);
          Serial.println(F(" Animating!"));
        }
      }
      break;
    }

    case State::playing: {
      if (now >= timeout) {
        knock_on = false;
        timeout = now + lockout_time;
        state = State::lockout;
        Serial.print(now);
        Serial.println(F(" Lockout"));
        break;
      }
      if (now >= knock_timeout) {
        if (knock_on) {
          knock_on = false;
          long const noknock_time = pick_time(min_noknock_time, max_noknock_time);
          knock_timeout = now + noknock_time;
        } else {
          knock_on = true;
          long const knock_time = pick_time(min_knock_time, max_knock_time);
          knock_timeout = now + knock_time;
          Serial.print(now);
          Serial.println(F(" Bang!"));
        }
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
  }
}
