class Parser {
  public:
    using MyAudioModule = BasicAudioModule;
    explicit Parser(MyAudioModule &audio, Fogger *fogger = nullptr) :
      m_audio(audio), m_fogger(fogger), m_p(nullptr) {}

    bool parse(const char *buf) {
      m_p = buf;
      switch (parseKeyword()) {
        case KW_BASS:
          m_audio.selectEQ(MyAudioModule::EQ_BASS);
          return true;
        case KW_CLASSICAL:
          m_audio.selectEQ(MyAudioModule::EQ_CLASSICAL);
          return true;
        case KW_EQ:
          if (Accept('?')) {
            m_audio.queryEQ();
            return true;
          }
          if (Accept('=')) {
            auto eq = MyAudioModule::EQ_NORMAL;
            switch (parseKeyword()) {
              case KW_BASS:       eq = MyAudioModule::EQ_BASS;      break;
              case KW_CLASSICAL:  eq = MyAudioModule::EQ_CLASSICAL; break;
              case KW_JAZZ:       eq = MyAudioModule::EQ_NORMAL;    break;
              case KW_NORMAL:     eq = MyAudioModule::EQ_NORMAL;    break;
              case KW_POP:        eq = MyAudioModule::EQ_NORMAL;    break;
              case KW_ROCK:       eq = MyAudioModule::EQ_NORMAL;    break;
              default: return false;
            }
            m_audio.selectEQ(eq);
            return true;
          }
          break;
        case KW_FLASH: return parseDeviceQuery(MyAudioModule::DEV_FLASH);
        case KW_FOG: {
          auto duration = parseUnsigned();
          if (m_fogger) m_fogger->on(1000*duration);
          return true;
        }
        case KW_FOLDER:
          if (parseKeyword() == KW_COUNT && Accept('?')) {
            m_audio.queryFolderCount();
            return true;
          }
          break;
        case KW_JAZZ:
          m_audio.selectEQ(MyAudioModule::EQ_JAZZ);
          return true;
        case KW_NEXT:
          m_audio.playNextFile();
          return true;
        case KW_NORMAL:
          m_audio.selectEQ(MyAudioModule::EQ_NORMAL);
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
        case KW_POP:
          m_audio.selectEQ(MyAudioModule::EQ_POP);
          return true;
        case KW_PREVIOUS:
          m_audio.playPreviousFile();
          return true;
        case KW_RANDOM:
          m_audio.playFilesInRandomOrder();
          return true;
        case KW_RESET:
          m_audio.reset();
          return true;
        case KW_ROCK:
          m_audio.selectEQ(MyAudioModule::EQ_ROCK);
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
    bool parseDeviceQuery(typename MyAudioModule::Device device) {
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
      KW_UNKNOWN, KW_BASS, KW_CLASSICAL, KW_COUNT, KW_EQ, KW_FILE, KW_FLASH,
      KW_FOG, KW_FOLDER, KW_JAZZ, KW_LOOP, KW_NEXT, KW_NORMAL, KW_PAUSE,
      KW_PLAY, KW_POP, KW_PREVIOUS, KW_RANDOM, KW_RESET, KW_ROCK, KW_SDCARD,
      KW_SELECT, KW_SEQ, KW_STATUS, KW_STOP, KW_UNPAUSE, KW_USB, KW_VOLUME
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
      if (Accept('b')) return AllowSuffix(KW_BASS, 'a', 's', 's');
      if (Accept('c')) {
        if (Accept('l')) return AllowSuffix(KW_CLASSICAL, 'a', 's', 's', 'i', 'c', 'a', 'l');
        if (Accept('o')) return AllowSuffix(KW_COUNT, 'u', 'n', 't');
        return KW_UNKNOWN;
      }
      if (Accept('e')) return AllowSuffix(KW_EQ, 'q');
      if (Accept('f')) {
        if (Accept('i')) return AllowSuffix(KW_FILE, 'l', 'e');
        if (Accept('l')) return AllowSuffix(KW_FLASH, 'a', 's', 'h');
        if (Accept('o')) {
          if (Accept('g')) return KW_FOG;
          if (Accept('l')) return AllowSuffix(KW_FOLDER, 'd', 'e', 'r');
          return KW_UNKNOWN;
        }
        return KW_UNKNOWN;
      }
      if (Accept('j')) return AllowSuffix(KW_JAZZ, 'a', 'z', 'z');
      if (Accept('l')) return AllowSuffix(KW_LOOP, 'o', 'o', 'p');
      if (Accept('n')) {
        if (Accept('e')) return AllowSuffix(KW_NEXT, 'x', 't');
        if (Accept('o')) return AllowSuffix(KW_NORMAL, 'r', 'm', 'a', 'l');
        return KW_UNKNOWN;
      }
      if (Accept('p')) {
        if (Accept('a')) return AllowSuffix(KW_PAUSE, 'u', 's', 'e');
        if (Accept('l')) return AllowSuffix(KW_PLAY, 'a', 'y');
        if (Accept('o')) return AllowSuffix(KW_POP, 'p');
        if (Accept('r')) return AllowSuffix(KW_PREVIOUS, 'e', 'v', 'i', 'o', 'u', 's');
        return KW_UNKNOWN;
      }
      if (Accept('r')) {
        if (Accept('a')) return AllowSuffix(KW_RANDOM, 'n', 'd', 'o', 'm');
        if (Accept('e')) return AllowSuffix(KW_RESET, 's', 'e', 't');
        if (Accept('o')) return AllowSuffix(KW_ROCK, 'c', 'k');
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
    Fogger *m_fogger;
    const char *m_p;
};
