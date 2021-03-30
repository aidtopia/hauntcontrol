// Haunt Control
// Adrian McCarthy

#include <Arduino.h>
#include <SoftwareSerial.h>

#include "audiomodule.h"

const int red_button = 2;
const int green_button = 3;
const int blue_button = 4;
const int yellow_button = 5;
const int tx_to_audio = 10;
const int rx_from_audio = 11;
const int eq_output = A0;
const int eq_reset = 12;
const int eq_strobe = 13;
const int eq_display[] = { 6, 7, 8, 9 };

SoftwareSerial audio_serial(rx_from_audio, tx_to_audio);

typedef AudioModule MyAudioModule;
MyAudioModule audio_board(&audio_serial);

char buf[32];
int buflen = 0;

class MSGEQ7 {
  public:
    void begin(int reset_pin, int strobe_pin, int output_pin, const int eq_display[4]) {
      m_reset_pin = reset_pin;
      pinMode(m_reset_pin, OUTPUT);
      digitalWrite(m_reset_pin, LOW);
      m_strobe_pin = strobe_pin;
      pinMode(m_strobe_pin, OUTPUT);
      digitalWrite(m_strobe_pin, LOW);
      m_output_pin = output_pin;
      for (int i = 0; i < 4; ++i) {
        m_eq_display[i] = eq_display[i];
        pinMode(m_eq_display[i], OUTPUT);
        digitalWrite(m_eq_display[i], LOW);
      }      
    }

    void update() {
      digitalWrite(m_reset_pin, HIGH);
      digitalWrite(m_strobe_pin, HIGH);
      delayMicroseconds(18);
      digitalWrite(m_strobe_pin, LOW);
      delayMicroseconds(18);
      digitalWrite(m_strobe_pin, HIGH);
      digitalWrite(m_reset_pin, LOW);
      delayMicroseconds(18);
      for (int i = 0; i < 7; ++i) {
        digitalWrite(m_strobe_pin, LOW);
        delayMicroseconds(36);
        m_channels[i] = analogRead(m_output_pin);
        digitalWrite(m_strobe_pin, HIGH);
        delayMicroseconds(36);
      }

      // Show channel 1 (160 Hz) in the eq display.
      digitalWrite(m_eq_display[0], m_channels[1] >   0 ? HIGH : LOW);
      digitalWrite(m_eq_display[1], m_channels[1] > 255 ? HIGH : LOW);
      digitalWrite(m_eq_display[2], m_channels[1] > 511 ? HIGH : LOW);
      digitalWrite(m_eq_display[3], m_channels[1] > 767 ? HIGH : LOW);
    }

  private:
    int m_reset_pin;
    int m_strobe_pin;
    int m_output_pin;
    int m_channels[7];
    int m_eq_display[4];
} msgeq7;

class Parser {
  public:
    explicit Parser(MyAudioModule &audio) : m_audio(audio), m_p(nullptr) {}

    bool parse(const char *buf) {
      m_p = buf;
      switch (parseKeyword()) {
        case KW_EQ:
          if (Accept('?')) {
            m_audio.queryEQ();
            return true;
          }
          break;
        case KW_FLASH: return parseDeviceQuery(MyAudioModule::DEV_FLASH);
        case KW_FOLDER:
          if (parseKeyword() == KW_COUNT && Accept('?')) {
            m_audio.queryFolderCount();
            return true;
          }
          break;
        case KW_NEXT:
          m_audio.playNextFile();
          return true;
        case KW_PAUSE:
          m_audio.pause();
          return true;
        case KW_PLAY: {
          auto index = parseUnsigned();
          if (index != 0) {
            if (Accept('/')) {
              auto track = parseUnsigned();
              m_audio.playTrack(/* folder */index, track);
              return true;
            }
            m_audio.playFile(index);
            return true;
          }
          switch (parseKeyword()) {
            case KW_NEXT:   m_audio.playNextFile(); return true;
            case KW_PREVIOUS: m_audio.playPreviousFile(); return true;
            case KW_RANDOM: m_audio.playFilesInRandomOrder(); return true;
            default: break;
          }
          return false;
        }
        case KW_PREVIOUS:
          m_audio.playPreviousFile();
          return true;
        case KW_RANDOM:
          m_audio.playFilesInRandomOrder();
          return true;
        case KW_RESET:
          m_audio.reset();
          return true;
        case KW_SDCARD: return parseDeviceQuery(MyAudioModule::DEV_SDCARD);
        case KW_SELECT:
          switch(parseKeyword()) {
            case KW_FLASH:
              m_audio.selectSource(MyAudioModule::DEV_FLASH);
              return true;
            case KW_SDCARD:
              m_audio.selectSource(MyAudioModule::DEV_SDCARD);
              return true;
            case KW_USB:
              m_audio.selectSource(MyAudioModule::DEV_USB);
              return true;
            default:
              return false;
          }
          break;
        case KW_SEQ:
          Accept('?');
          m_audio.queryPlaybackSequence();
          return true;
        case KW_STATUS:
          Accept('?');
          m_audio.queryStatus();
          return true;
        case KW_STOP:
          m_audio.stop();
          return true;
        case KW_UNPAUSE:
          m_audio.unpause();
          return true;
        case KW_USB: return parseDeviceQuery(MyAudioModule::DEV_USB);
        case KW_VOLUME:
          if (Accept('?')) {
              m_audio.queryVolume();
              return true;
          }
          if (Accept('=')) {
            m_audio.setVolume(parseInteger());
            return true;
          }
          break;
        default: break;
      }
      return false;
    }

  private:
    bool parseDeviceQuery(MyAudioModule::Device device) {
      switch (parseKeyword()) {
        case KW_FILE:
          if (Accept('?')) {
            m_audio.queryCurrentFile(device);
            return true;
          }
          if (parseKeyword() == KW_COUNT && Accept('?')) {
            m_audio.queryFileCount(device);
            return true;
          }
          return false;
        case KW_FOLDER:
          Serial.println(F("You can only query the folder count of the selected device."));
          /* FALLTHROUGH */
        default:
          return false;
      }
      return false;
    }
  
    enum Keyword {
      KW_UNKNOWN, KW_COUNT, KW_EQ, KW_FILE, KW_FLASH, KW_FOLDER,
      KW_LOOP, KW_NEXT, KW_PAUSE, KW_PLAY, KW_PREVIOUS, KW_RANDOM,
      KW_RESET, KW_SDCARD, KW_SELECT, KW_SEQ, KW_STATUS, KW_STOP,
      KW_UNPAUSE, KW_USB, KW_VOLUME
    };

    Keyword parseKeyword() {
      while (Accept(' ') || Accept('\t')) {}
      // A hand-rolled TRIE.  This may seem crazy, but many
      // microcontrollers have very limited RAM.  A data-driven
      // solution would require a table in RAM, so that's a
      // non-starter.  Instead, we "unroll" what the data-driven
      // solution would do so that it gets coded directly into
      // program memory.  Believe it or not, the variadic template
      // for AllowSuffix actually reduces the amount of code
      // generated versus a completely hand-rolled expression.
      if (Accept('c')) return AllowSuffix(KW_COUNT, 'o', 'u', 'n', 't');
      if (Accept('e')) return AllowSuffix(KW_EQ, 'q');
      if (Accept('f')) {
        if (Accept('i')) return AllowSuffix(KW_FILE, 'l', 'e');
        if (Accept('l')) return AllowSuffix(KW_FLASH, 'a', 's', 'h');
        if (Accept('o')) return AllowSuffix(KW_FOLDER, 'l', 'd', 'e', 'r');
        return KW_UNKNOWN;
      }
      if (Accept('l')) return AllowSuffix(KW_LOOP, 'o', 'o', 'p');
      if (Accept('n')) return AllowSuffix(KW_NEXT, 'e', 'x', 't');
      if (Accept('p')) {
        if (Accept('a')) return AllowSuffix(KW_PAUSE, 'u', 's', 'e');
        if (Accept('l')) return AllowSuffix(KW_PLAY, 'a', 'y');
        if (Accept('r')) return AllowSuffix(KW_PREVIOUS, 'e', 'v', 'i', 'o', 'u', 's');
        return KW_UNKNOWN;
      }
      if (Accept('r')) {
        if (Accept('a')) return AllowSuffix(KW_RANDOM, 'n', 'd', 'o', 'm');
        if (Accept('e')) return AllowSuffix(KW_RESET, 's', 'e', 't');
        return KW_UNKNOWN;
      }
      if (Accept('s')) {
        if (Accept('d')) return AllowSuffix(KW_SDCARD, 'c', 'a', 'r', 'd');
        if (Accept('e')) {
          if (Accept('q')) return AllowSuffix(KW_SEQ);
          if (Accept('l')) return AllowSuffix(KW_SELECT, 'e', 'c', 't');
          return KW_UNKNOWN;
        }
        if (Accept('t')) {
          if (Accept('a')) return AllowSuffix(KW_STATUS, 't', 'u', 's');
          if (Accept('o')) return AllowSuffix(KW_STOP, 'p');
          return KW_UNKNOWN;
        }
        return KW_UNKNOWN;
      }
      if (Accept('u')) {
        if (Accept('n')) return AllowSuffix(KW_UNPAUSE, 'p', 'a', 'u', 's', 'e');
        if (Accept('s')) return AllowSuffix(KW_USB, 'b');
        return KW_UNKNOWN;
      }
      if (Accept('v')) return AllowSuffix(KW_VOLUME, 'o', 'l', 'u', 'm', 'e');
      return KW_UNKNOWN;
    }

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
    bool MatchLetter() const { return 'a' <= *m_p && *m_p <= 'z'; }

    bool AllowSuffix() { return !MatchLetter(); }

    template <typename ... T>
    bool AllowSuffix(char head, T... tail) {
      return !MatchLetter() || (Accept(head) && AllowSuffix(tail...));
    }

    template <typename ... T>
    Keyword AllowSuffix(Keyword kw, T... suffix) {
      return AllowSuffix(suffix...) ? kw : KW_UNKNOWN;
    }

    MyAudioModule &m_audio;
    const char *m_p;
} parser(audio_board);

void setup() {
  Serial.begin(115200);
  Serial.println(F("Hello Audio Module"));
  pinMode(red_button, INPUT_PULLUP);
  pinMode(green_button, INPUT_PULLUP);
  pinMode(blue_button, INPUT_PULLUP);
  pinMode(yellow_button, INPUT_PULLUP);
  pinMode(rx_from_audio, INPUT);  // Will SoftwareSerial::begin()
  pinMode(tx_to_audio, OUTPUT);   // handle these pinModes?

  audio_serial.begin(9600);
  audio_board.begin();

  msgeq7.begin(eq_reset, eq_strobe, eq_output, eq_display);
}

void loop() {
  if (digitalRead(red_button) == LOW) {
    Serial.println(F("red button pressed"));
    audio_board.selectSource(MyAudioModule::DEV_SDCARD);
    while (digitalRead(red_button) == LOW) {}
  }
  if (digitalRead(green_button) == LOW) {
    Serial.println(F("green button pressed"));
    audio_board.selectSource(MyAudioModule::DEV_USB);
    while (digitalRead(green_button) == LOW) {}
  }
  if (digitalRead(blue_button) == LOW) {
    Serial.println(F("blue button pressed"));
    audio_board.playPreviousFile();
    while (digitalRead(blue_button) == LOW) {}
  }
  if (digitalRead(yellow_button) == LOW) {
    Serial.println(F("yellow button pressed"));
    audio_board.playNextFile();
    while (digitalRead(yellow_button) == LOW) {}
  }
  audio_board.update();

  msgeq7.update();

  while (Serial.available()) {
    char ch = Serial.read();
    if (buflen == sizeof(buf)) buflen = 0;
    if (ch != '\n') {
      buf[buflen++] = ch;
      continue;
    }

    buf[buflen] = '\0';
    buflen = 0;
    Serial.print(F("> "));
    Serial.println(buf);
    parser.parse(buf);
  }
}
