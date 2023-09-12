// AudioModule
// Adrian McCarthy 2018-

// A library that works with various serial audio modules,
// like DFPlayer Mini, Catalex, etc.

#include "timeout.h"

class BasicAudioModule {
  public:
    explicit BasicAudioModule(Stream &stream) :
      m_stream(stream), m_in(), m_out(), m_state(nullptr), m_timeout() {}

    virtual void begin() { reset(); }

    // Call each time through `loop`.
    void update() {
      checkForIncomingMessage();
      checkForTimeout();
    }

    enum Device {
      DEV_USB,      // a storage device connected via USB
      DEV_SDCARD,   // an SD card in the TF slot
      DEV_AUX,      // typically a connection to a PC
      DEV_SLEEP,    // a pseudo-device to indicate the module is sleeping
      DEV_FLASH,    // internal flash memory
      // Synonyms used in the datasheets:
      DEV_TF  = DEV_SDCARD, // The SD card slot is sometimes called TF (True Flash)
      DEV_PC  = DEV_AUX,    // The AUX input is typically a PC connection
      DEV_SPI = DEV_FLASH   // The internal flash memory is an SPI device
    };

    enum Equalizer { EQ_NORMAL, EQ_POP, EQ_ROCK, EQ_JAZZ, EQ_CLASSICAL, EQ_BASS };
    enum ModuleState { MS_STOPPED, MS_PLAYING, MS_PAUSED, MS_ASLEEP };
    enum Sequence { SEQ_LOOPALL, SEQ_LOOPFOLDER, SEQ_LOOPTRACK, SEQ_RANDOM, SEQ_SINGLE };

    // Reset and re-initialize the audio module.
    // 
    // Resetting causes an unavoidable click on the output.
    void reset() {
      setState(&s_init_resetting_hardware);
    }

    // Select a Device to be the current source.
    //
    // Many modules select `DEV_SDCARD` by default, which is usually
    // appropriate, but it's good practice to select it yourself to be
    // certain.
    void selectSource(Device device) {
      switch (device) {
        case DEV_USB:    sendCommand(MID_SELECTSOURCE, 1); break;
        case DEV_SDCARD: sendCommand(MID_SELECTSOURCE, 2); break;
        case DEV_FLASH:  sendCommand(MID_SELECTSOURCE, 5); break;
        default: break;
      }
    }
    
    // Play a file selected by its file system index.
    //
    // If you don't know the file index of the track you want, you should
    // probably use `playTrack` instead.
    //
    // This command can play a track from any folder on the selected source
    // device.  You can use `queryFileCount` to find out how many
    // files are available.
    //
    // Corresponds to playback sequence `SEQ_SINGLE`.
    void playFile(uint16_t file_index) { sendCommand(MID_PLAYFILE, file_index); }

    // Play the next file based on the current file index.
    void playNextFile() { sendCommand(MID_PLAYNEXT); }

    // Play the previous file based on the current file index.
    void playPreviousFile() { sendCommand(MID_PLAYPREVIOUS); }

    // Play a single file repeatedly.
    //
    // Corresponds to playback sequence `SEQ_LOOPTRACK`.
    void loopFile(uint16_t file_index) { sendCommand(MID_LOOPFILE, file_index); }

    // Play all the files on the device, in file index order, repeatedly.
    //
    // Corresponds to playback sequence `SEQ_LOOPALL`.
    void loopAllFiles() { sendCommand(MID_LOOPALL, 1); }

    // Play all the files on the current device in a random order.
    //
    // TBD: Does it repeat once it has played all of them?
    //
    // Corresponds to playback sequence `SEQ_RANDOM`.
    void playFilesInRandomOrder() { sendCommand(MID_RANDOMPLAY); }
    
    // playTrack lets you specify a folder named with two decimal
    // digits, like "01" or "02", and a track whose name begins
    // with three decimal digits like "001.mp3" or
    // "014 Yankee Doodle.wav".
    void playTrack(uint16_t folder, uint16_t track) {
      // Under the hood, there are a couple different command
      // messages to achieve this. We'll automatically select the
      // most appropriate one based on the values.
      if (track < 256) {
        const uint16_t param = (folder << 8) | track;
        sendCommand(MID_PLAYFROMFOLDER, param);
      } else if (folder < 16 && track <= 3000) {
        // For folders with more than 255 tracks, we have this
        // alternative command.
        const uint16_t param = (folder << 12) | track;
        sendCommand(MID_PLAYFROMBIGFOLDER, param);
      }
    }

    // This overload lets you select a track whose file name begins
    // with a three or four decimal digit number, like "001" or "2432".
    // The file must be in a top-level folder named "MP3".  It's
    // recommended that you have fewer than 3000 files in this folder
    // in order to avoid long startup delays as the module searches
    // for the right file.
    //
    // Even though the folder is named "MP3", it may contain .wav
    // files as well.
    void playTrack(uint16_t track) { sendCommand(MID_PLAYFROMMP3, track); }

    // Insert an "advertisement."
    //
    // This interrupts a track to play a track from a folder named
    // "ADVERT".  The track must have a file name as described in
    // the playTrack(uint16_t) command above.  When the advert track
    // has completed, the interruped audio track resumes from where
    // it was.
    //
    // This is typically used with the regular audio in the "MP3"
    // folder described above, but it can interrupt any track
    // regardless of how you started playing it.
    //
    // If no track is currently playing (e.g., if the device is
    // stopped or paused), this will result in an "insertion error."
    //
    // You cannot insert while an inserted track is alrady playing.
    void insertAdvert(uint16_t track) { sendCommand(MID_INSERTADVERT, track); }

    // Stops a track that was inserted with `insertAdvert`.  The
    // interrupted track will resume from where it was.
    void stopAdvert() { sendCommand(MID_STOPADVERT); }

    // Stops any audio that's playing and resets the playback
    // sequence to `SEQ_SINGLE`.
    void stop() { sendCommand(MID_STOP); }

    // Pauses the current playback.
    void pause() { sendCommand(MID_PAUSE); }

    // Undoes a previous call to `pause`.
    //
    // Alternative use:  When a track finishes playing when the
    // playback sequence is `SEQ_SINGLE`, the next track (by file index)
    // is cued up and paused.  If you call this function about 100 ms
    // after an `onTrackFinished` notification, the cued track will
    // begin playing.
    void unpause() { sendCommand(MID_UNPAUSE); }

    // Set the volume to a level in the range of 0 - 30.
    void setVolume(int volume) {
      // Catalex effectively goes to 31, but it doesn't automatically
      // clamp values.  DF Player Mini goes to 30 and clamps there.
      // We'll make them behave the same way.
      if (volume < 0) volume = 0;
      if (30 < volume) volume = 30;
      sendCommand(MID_SETVOLUME, static_cast<uint16_t>(volume));
    }

    // Selecting an equalizer interrupts the current playback, so it's
    // best to select the EQ before starting playback.  Alternatively,
    // you can also pause, select the new EQ, and then unpause.
    void selectEQ(Equalizer eq) { sendCommand(MID_SELECTEQ, eq); }

    // Sleeping doesn't seem useful.  To lower the current draw, use
    // `disableDAC`.
    void sleep() { sendCommand(MID_SLEEP); }
    
    // Seems buggy.  Try reset() or selectSource().
    void wake() { sendCommand(MID_WAKE); }

    // Disabling the DACs when not in use saves a few milliamps.
    // Causes a click on the output.
    void disableDACs() { sendCommand(MID_DISABLEDAC, 1); }

    // To re-enable the DACs after they've been disabled.
    // Causes a click on the output.
    void enableDACs() { sendCommand(MID_DISABLEDAC, 0); }

    // Ask how many audio files (total) are on a source device, including
    // the root directory and any subfolders.  This is useful for knowing
    // the upper bound on a `playFile` call.  Hook `onDeviceFileCount`
    // for the result.
    void queryFileCount(Device device) {
      switch (device) {
        case DEV_USB:    sendQuery(MID_USBFILECOUNT);   break;
        case DEV_SDCARD: sendQuery(MID_SDFILECOUNT);    break;
        case DEV_FLASH:  sendQuery(MID_FLASHFILECOUNT); break;
        default: break;
      }
    }

    void queryCurrentFile(Device device) {
      switch (device) {
        case DEV_USB:    sendQuery(MID_CURRENTUSBFILE);   break;
        case DEV_SDCARD: sendQuery(MID_CURRENTSDFILE);    break;
        case DEV_FLASH:  sendQuery(MID_CURRENTFLASHFILE); break;
        default: break;
      }
    }

    // Ask how many folders there are under the root folder on the current
    // source device.
    void queryFolderCount() { sendQuery(MID_FOLDERCOUNT); }

    // Ask which device is currently selected as the source and whether
    // it's playing, paused, or stopped.  Can also indicate if the module
    // is asleep.  Hook `onStatus` for the result.  (Current device doesn't
    // seem to be reliable on DFPlayer Mini.)
    void queryStatus() { sendQuery(MID_STATUS); }

    // Query the current volume.
    //
    // Hook `onVolume` for the result.
    void queryVolume() { sendQuery(MID_VOLUME); }

    // Query the current equalizer setting.
    //
    // Hook `onEqualizer` for the result.
    void queryEQ() { sendQuery(MID_EQ); }

    // Query the current playback sequence.
    //
    // Hook `onPlaybackSequence` for the result.
    void queryPlaybackSequence() { sendQuery(MID_PLAYBACKSEQUENCE); }

    // Query the firmware version.
    //
    // Hook `onFirmwareVersion` for the result.  Catalex doesn't respond
    // to this query, so watch for a timeout error.
    void queryFirmwareVersion() { sendQuery(MID_FIRMWAREVERSION); }

    static constexpr uint16_t combine(uint8_t hi, uint8_t lo) {
      return static_cast<uint16_t>(hi << 8) | lo;
    }
    
    static constexpr uint8_t high(uint16_t x) { return x >> 8; }
    static constexpr uint8_t low(uint16_t x)  { return x & 0xFF; }

  protected:
    // These are the message IDs (sometimes called commands) for the
    // messages in the serial protocol for the YX5200 and YX5300 chips.
    enum MsgID : uint8_t {
      // First, the commands
      MID_PLAYNEXT          = 0x01,
      MID_PLAYPREVIOUS      = 0x02,
      MID_PLAYFILE          = 0x03,
      MID_VOLUMEUP          = 0x04,
      MID_VOLUMEDOWN        = 0x05,
      MID_SETVOLUME         = 0x06,
      MID_SELECTEQ          = 0X07,
      MID_LOOPFILE          = 0x08,
      MID_LOOPFLASHTRACK    = MID_LOOPFILE,  // Alternate msg not used
      MID_SELECTSOURCE      = 0x09,
      MID_SLEEP             = 0x0A,
      MID_WAKE              = 0x0B,
      MID_RESET             = 0x0C,
      MID_RESUME            = 0x0D,
      MID_UNPAUSE           = MID_RESUME,
      MID_PAUSE             = 0x0E,
      MID_PLAYFROMFOLDER    = 0x0F,
      MID_VOLUMEADJUST      = 0x10,  // Seems busted, use MID_SETVOLUME
      MID_LOOPALL           = 0x11,
      MID_PLAYFROMMP3       = 0x12,  // "MP3" here refers to name of folder
      MID_INSERTADVERT      = 0x13,
      MID_PLAYFROMBIGFOLDER = 0x14,
      MID_STOPADVERT        = 0x15,
      MID_STOP              = 0x16,
      MID_LOOPFOLDER        = 0x17,
      MID_RANDOMPLAY        = 0x18,
      MID_LOOPCURRENTFILE   = 0x19,
      MID_DISABLEDAC        = 0x1A,
      MID_PLAYLIST          = 0x1B,  // Might not work, unusual message length
      MID_PLAYWITHVOLUME    = 0x1C,  // seems redundant

      // Asynchronous messages from the module
      MID_DEVICEINSERTED    = 0x3A,
      MID_DEVICEREMOVED     = 0x3B,
      MID_FINISHEDUSBFILE   = 0x3C,
      MID_FINISHEDSDFILE    = 0x3D,
      MID_FINISHEDFLASHFILE = 0x3E,

      // Quasi-asynchronous
      MID_INITCOMPLETE      = 0x3F,

      // Basic replies
      MID_ERROR             = 0x40,
      MID_ACK               = 0x41,

      // Queries and their responses
      MID_STATUS            = 0x42,
      MID_VOLUME            = 0x43,
      MID_EQ                = 0x44,
      MID_PLAYBACKSEQUENCE  = 0x45,
      MID_FIRMWAREVERSION   = 0x46,
      MID_USBFILECOUNT      = 0x47,
      MID_SDFILECOUNT       = 0x48,
      MID_FLASHFILECOUNT    = 0x49,
      // no 0x4A?
      MID_CURRENTUSBFILE    = 0x4B,
      MID_CURRENTSDFILE     = 0x4C,
      MID_CURRENTFLASHFILE  = 0x4D,
      MID_FOLDERTRACKCOUNT  = 0x4E,
      MID_FOLDERCOUNT       = 0x4F,

      // We're going to steal an ID for our state machine's use.
      MID_ENTERSTATE        = 0x00
    };

    enum ErrorCode : uint16_t {
      EC_UNSUPPORTED        = 0x00,  // MsgID used is not supported
      EC_NOSOURCES          = 0x01,  // module busy or no sources installed
      EC_SLEEPING           = 0x02,  // module is sleeping
      EC_SERIALERROR        = 0x03,  // serial communication error
      EC_BADCHECKSUM        = 0x04,  // module received bad checksum
      EC_FILEOUTOFRANGE     = 0x05,  // this is the file index
      EC_TRACKNOTFOUND      = 0x06,  // couldn't find track by numeric prefix
      EC_INSERTIONERROR     = 0x07,  // couldn't start ADVERT track
      EC_SDCARDERROR        = 0x08,  // ??
      EC_ENTEREDSLEEP       = 0x0A,  // entered sleep mode??

      // And reserving one for our state machine
      EC_TIMEDOUT           = 0x0100
    };
  
    // Manages a buffered message with all the protocol details.
    class Message {
      public:
        enum { START = 0x7E, VERSION = 0xFF, LENGTH = 6, END = 0xEF };
        enum Feedback { NO_FEEDBACK = 0x00, FEEDBACK = 0x01 };

        Message() :
          m_buf{START, VERSION, LENGTH, 0, FEEDBACK, 0, 0, 0, 0, END},
          m_length(0) {}

        void set(MsgID msgid, uint16_t param, Feedback feedback = NO_FEEDBACK) {
          // Note that we're filling in just the bytes that change.  We rely
          // on the framing bytes set when the buffer was first initialized.
          m_buf[3] = msgid;
          m_buf[4] = feedback;
          m_buf[5] = (param >> 8) & 0xFF;
          m_buf[6] = (param     ) & 0xFF;
          const uint16_t checksum = ~sum() + 1;
          m_buf[7] = (checksum >> 8) & 0xFF;
          m_buf[8] = (checksum     ) & 0xFF;
          m_length = 10;
        }

        const uint8_t *getBuffer() const { return m_buf; }
        int getLength() const { return m_length; }

        bool isValid() const {
          if (m_length == 8 && m_buf[7] == END) return true;
          if (m_length != 10) return false;
          const uint16_t checksum = combine(m_buf[7], m_buf[8]);
          return sum() + checksum == 0;
        }

        MsgID getMessageID() const { return static_cast<MsgID>(m_buf[3]); }
        uint8_t getParamHi() const { return m_buf[5]; }
        uint8_t getParamLo() const { return m_buf[6]; }
        uint16_t getParam() const { return combine(m_buf[5], m_buf[6]); }

        // Returns true if the byte `b` completes a message.
        bool receive(uint8_t b) {
          switch (m_length) {
            default:
              // `m_length` is out of bounds, so start fresh.
              m_length = 0;
              /* FALLTHROUGH */
            case 0: case 1: case 2: case 9:
              // These bytes must always match the template.
              if (b == m_buf[m_length]) { ++m_length; return m_length == 10; }
              // No match; try to resync.
              if (b == START) { m_length = 1; return false; }
              m_length = 0;
              return false;
            case 7:
              // If there's no checksum, the message may end here.
              if (b == END) { m_length = 8; return true; }
              /* FALLTHROUGH */
            case 3: case 4: case 5: case 6: case 8:
              // These are the payload bytes we care about.
              m_buf[m_length++] = b;
              return false;
          }
        }

      private:
        // Sums the bytes used to compute the Message's checksum.
        uint16_t sum() const {
          uint16_t s = 0;
          for (int i = 1; i <= LENGTH; ++i) {
            s += m_buf[i];
          }
          return s;
        }

        uint8_t m_buf[10];
        int m_length;
    };

#if 0
    void onDeviceInserted(Device src) {}
    void onDeviceRemoved(Device src) {}
    void onError(uint16_t code) {}
    void onFinishedFile(Device device, uint16_t file_index) {}
    void onMessageInvalid() {}
    void onMessageReceived(const Message &msg) {}
    void onMessageSent(const uint8_t *buf, int len) {}
#else
    void onAck() {
      Serial.println(F("ACK"));
    }

    void onCurrentTrack(Device device, uint16_t file_index) {
      printDeviceName(device);
      Serial.print(F(" current file index: "));
      Serial.println(file_index);
    }

    void onDeviceInserted(Device src) {
      Serial.print(F("Device inserted: "));
      printDeviceName(src);
      Serial.println();
    }

    void onDeviceRemoved(Device src) {
      printDeviceName(src);
      Serial.println(F(" removed."));
    }

    void onEqualizer(Equalizer eq) {
      Serial.print(F("Equalizer: "));
      printEqualizerName(eq);
      Serial.println();
    }

    void onError(uint16_t code) {
      Serial.print(F("Error "));
      Serial.print(code);
      Serial.print(F(": "));
      switch (code) {
        case 0x00: Serial.println(F("Unsupported command")); break;
        case 0x01: Serial.println(F("Module busy or no sources available")); break;
        case 0x02: Serial.println(F("Module sleeping")); break;
        case 0x03: Serial.println(F("Serial communication error")); break;
        case 0x04: Serial.println(F("Bad checksum")); break;
        case 0x05: Serial.println(F("File index out of range")); break;
        case 0x06: Serial.println(F("Track not found")); break;
        case 0x07: Serial.println(F("Insertion error")); break;
        case 0x08: Serial.println(F("SD card error")); break;
        case 0x0A: Serial.println(F("Entered sleep mode")); break;
        case 0x100: Serial.println(F("Timed out")); break;
        default:   Serial.println(F("Unknown error code")); break;
      }
    }

    void onDeviceFileCount(Device device, uint16_t count) {
      printDeviceName(device);
      Serial.print(F(" file count: "));
      Serial.println(count);
    }

    // Note that this hook receives a file index, even if the track
    // was initialized using something other than its file index.
    //
    // The module sometimes sends these multiple times in quick
    // succession.
    //
    // This hook does not trigger when the playback is stopped, only
    // when a track finishes playing on its own.
    //
    // This hook does not trigger when an inserted track finishes.
    // If you need to know that, you can try watching for a brief
    // blink on the BUSY pin of the DF Player Mini.
    void onFinishedFile(Device device, uint16_t file_index) {
      Serial.print(F("Finished playing file: "));
      printDeviceName(device);
      Serial.print(F(" "));
      Serial.println(file_index);
    }

    void onFirmwareVersion(uint16_t version) {
      Serial.print(F("Firmware Version: "));
      Serial.println(version);
    }

    void onFolderCount(uint16_t count) {
      Serial.print(F("Folder count: "));
      Serial.println(count);
    }

    void onFolderTrackCount(uint16_t count) {
      Serial.print(F("Folder track count: "));
      Serial.println(count);
    }

    void onInitComplete(uint8_t devices) {
      Serial.print(F("Hardware initialization complete.  Device(s) online:"));
      if (devices & (1u << DEV_SDCARD)) Serial.print(F(" SD Card"));
      if (devices & (1u << DEV_USB))    Serial.print(F(" USB"));
      if (devices & (1u << DEV_AUX))    Serial.print(F(" AUX"));
      if (devices & (1u << DEV_FLASH))  Serial.print(F(" Flash"));
      Serial.println();
    }

    void onMessageInvalid() {
      Serial.println(F("Invalid message received."));
    }

    void onMessageReceived(const Message &msg) {
#if 0
      const auto *buf = msg.getBuffer();
      const auto len = msg.getLength();
      Serial.print(F("Received:"));
      for (int i = 0; i < len; ++i) {
        Serial.print(F(" "));
        Serial.print(buf[i], HEX);
      }
      Serial.println();
#endif

      switch (msg.getMessageID()) {
        case 0x3A: {
          const auto mask = msg.getParamLo();
          if (mask & 0x01) onDeviceInserted(DEV_USB);
          if (mask & 0x02) onDeviceInserted(DEV_SDCARD);
          if (mask & 0x04) onDeviceInserted(DEV_AUX);
          return;
        }
        case 0x3B: {
          const auto mask = msg.getParamLo();
          if (mask & 0x01) onDeviceRemoved(DEV_USB);
          if (mask & 0x02) onDeviceRemoved(DEV_SDCARD);
          if (mask & 0x04) onDeviceRemoved(DEV_AUX);
          return;
        }
        case 0x3C: return onFinishedFile(DEV_USB, msg.getParam());
        case 0x3D: return onFinishedFile(DEV_SDCARD, msg.getParam());
        case 0x3E: return onFinishedFile(DEV_FLASH, msg.getParam());

        // Initialization complete
        case 0x3F: {
          uint16_t devices = 0;
          const auto mask = msg.getParamLo();
          if (mask & 0x01) devices = devices | (1u << DEV_USB);
          if (mask & 0x02) devices = devices | (1u << DEV_SDCARD);
          if (mask & 0x04) devices = devices | (1u << DEV_AUX);
          if (mask & 0x10) devices = devices | (1u << DEV_FLASH);
          return onInitComplete(devices);
        }

        case 0x40: return onError(msg.getParamLo());
        
        // ACK
        case 0x41:
          return onAck();

        // Query responses
        case 0x42: {
          // Only Flyron documents this response to the status query.
          // The DFPlayer Mini always seems to report SDCARD even when
          // the selected and active device is USB, so maybe it uses
          // the high byte to signal something else?  Catalex also
          // always reports the SDCARD, but it only has an SDCARD.
          Device device = DEV_SLEEP;
          switch (msg.getParamHi()) {
            case 0x01: device = DEV_USB;     break;
            case 0x02: device = DEV_SDCARD;  break;
          }
          ModuleState state = MS_ASLEEP;
          switch (msg.getParamLo()) {
            case 0x00: state = MS_STOPPED;  break;
            case 0x01: state = MS_PLAYING;  break;
            case 0x02: state = MS_PAUSED;   break;
          }
          return onStatus(device, state);
        }
        case 0x43: return onVolume(msg.getParamLo());
        case 0x44: return onEqualizer(static_cast<Equalizer>(msg.getParamLo()));
        case 0x45: return onPlaybackSequence(static_cast<Sequence>(msg.getParamLo()));
        case 0x46: return onFirmwareVersion(msg.getParam());
        case 0x47: return onDeviceFileCount(DEV_USB, msg.getParam());
        case 0x48: return onDeviceFileCount(DEV_SDCARD, msg.getParam());
        case 0x49: return onDeviceFileCount(DEV_FLASH, msg.getParam());
        case 0x4B: return onCurrentTrack(DEV_USB, msg.getParam());
        case 0x4C: return onCurrentTrack(DEV_SDCARD, msg.getParam());
        case 0x4D: return onCurrentTrack(DEV_FLASH, msg.getParam());
        case 0x4E: return onFolderTrackCount(msg.getParam());
        case 0x4F: return onFolderCount(msg.getParam());
        default: break;
      }
    }

    void onMessageSent(const uint8_t * /* buf */, int /* len */) {
#if 0
      Serial.print(F("Sent:    "));
      for (int i = 0; i < len; ++i) {
        Serial.print(F(" "));
        Serial.print(buf[i], HEX);
      }
      Serial.println();
#endif
    }

    void onPlaybackSequence(Sequence seq) {
      Serial.print(F("Playback Sequence: "));
      printSequenceName(seq);
      Serial.println();
    }
    
    void onStatus(Device device, ModuleState state) {
      Serial.print(F("State: "));
      if (device != DEV_SLEEP) {
        printDeviceName(device);
        Serial.print(F(" "));
      }
      printModuleStateName(state);
      Serial.println();
    }

    void onVolume(uint8_t volume) {
      Serial.print(F("Volume: "));
      Serial.println(volume);
    }

    void printDeviceName(Device src) {
      switch (src) {
        case DEV_USB:    Serial.print(F("USB")); break;
        case DEV_SDCARD: Serial.print(F("SD Card")); break;
        case DEV_AUX:    Serial.print(F("AUX")); break;
        case DEV_SLEEP:  Serial.print(F("SLEEP (does this make sense)")); break;
        case DEV_FLASH:  Serial.print(F("FLASH")); break;
        default:         Serial.print(F("Unknown Device")); break;
      }
    }

    void printEqualizerName(Equalizer eq) {
      switch (eq) {
        case EQ_NORMAL:    Serial.print(F("Normal"));    break;
        case EQ_POP:       Serial.print(F("Pop"));       break;
        case EQ_ROCK:      Serial.print(F("Rock"));      break;
        case EQ_JAZZ:      Serial.print(F("Jazz"));      break;
        case EQ_CLASSICAL: Serial.print(F("Classical")); break;
        case EQ_BASS:      Serial.print(F("Bass"));      break;
        default:           Serial.print(F("Unknown EQ")); break;
      }
    }

    void printModuleStateName(ModuleState state) {
      switch (state) {
        case MS_STOPPED: Serial.print(F("Stopped")); break;
        case MS_PLAYING: Serial.print(F("Playing")); break;
        case MS_PAUSED:  Serial.print(F("Paused"));  break;
        case MS_ASLEEP:  Serial.print(F("Asleep"));  break;
        default:         Serial.print(F("???"));     break;
      }
    }

    void printSequenceName(Sequence seq) {
      switch (seq) {
        case SEQ_LOOPALL:    Serial.print(F("Loop All")); break;
        case SEQ_LOOPFOLDER: Serial.print(F("Loop Folder")); break;
        case SEQ_LOOPTRACK:  Serial.print(F("Loop Track")); break;
        case SEQ_RANDOM:     Serial.print(F("Random")); break;
        case SEQ_SINGLE:     Serial.print(F("Single")); break;
        default:             Serial.print(F("???")); break;
      }
    }
#endif

  private:
    // The State tells us how to handle messages received from the module.
    // The intent is to have one static instance of each concrete State in
    // order to minimize RAM and ROM usage.
    struct State {
        // The return value is a pointer to the new State.  You can return
        // `this` to remain in the same State, `nullptr` to indicate that
        // the operation is complete and the user can make a new request,
        // or a pointer to another State in order to chain a series of
        // operations together.
        virtual State *onEvent(
          BasicAudioModule * /*module*/,
          MsgID /*msgid*/,
          uint8_t /*paramHi*/,
          uint8_t /*paramLo*/
        ) {
          return this;
        }
    };

    struct InitResettingHardware : public State {
      State *onEvent(
          BasicAudioModule *module,
          MsgID msgid,
          uint8_t paramHi,
          uint8_t paramLo
      ) override {
        switch (msgid) {
          case MID_ENTERSTATE:
            Serial.println(F("Resetting hardware."));
            module->sendCommand(MID_RESET, 0, false);
            // The default timeout is probably too short for a reset.
            module->m_timeout.set(10000);
            return this;
          case MID_INITCOMPLETE:
            return &BasicAudioModule::s_init_getting_version;
          case MID_ERROR:
            if (combine(paramHi, paramLo) == EC_TIMEDOUT) {
              Serial.println(F("No response from audio module"));
            }
            return nullptr;
          default: break;
        }
        return this;
      }
    };
    static InitResettingHardware s_init_resetting_hardware;

    struct InitGettingVersion : public State {
      State *onEvent(BasicAudioModule *module, MsgID msgid, uint8_t paramHi, uint8_t paramLo) override {
        switch (msgid) {
          case MID_ENTERSTATE:
            module->queryFirmwareVersion();
            return this;
          case MID_FIRMWAREVERSION:
            return &s_init_checking_usb_file_count;
          case MID_ERROR:
            if (combine(paramHi, paramLo) == EC_TIMEDOUT) {
              return &s_init_checking_usb_file_count;
            }
            break;
          default: break;
        }
        return this;
      }
    };
    static InitGettingVersion s_init_getting_version;

    struct InitCheckingUSBFileCount : public State {
      State *onEvent(BasicAudioModule *module, MsgID msgid, uint8_t paramHi, uint8_t paramLo) override {
        switch (msgid) {
          case MID_ENTERSTATE:
            module->queryFileCount(BasicAudioModule::DEV_USB);
            return this;
          case MID_USBFILECOUNT: {
            const auto count = (static_cast<uint16_t>(paramHi) << 8) | paramLo;
            module->m_files = count;
            if (count > 0) return &s_init_selecting_usb;
            return &s_init_checking_sd_file_count;
          }
          case MID_ERROR:
            return &s_init_checking_sd_file_count;
          default: break;
        }
        return this;
      }
    };
    static InitCheckingUSBFileCount s_init_checking_usb_file_count;
    
    struct InitCheckingSDFileCount : public State {
      State *onEvent(BasicAudioModule *module, MsgID msgid, uint8_t paramHi, uint8_t paramLo) override {
        switch (msgid) {
          case MID_ENTERSTATE:
            module->queryFileCount(BasicAudioModule::DEV_SDCARD);
            return this;
          case MID_SDFILECOUNT: {
            const auto count = (static_cast<uint16_t>(paramHi) << 8) | paramLo;
            module->m_files = count;
            if (count > 0) return &s_init_selecting_sd;
            return nullptr;
          }
          case MID_ERROR:
            return nullptr;
          default: break;
        }
        return this;
      }
    };
    static InitCheckingSDFileCount s_init_checking_sd_file_count;

    struct InitSelectingUSB : public State {
      State *onEvent(
        BasicAudioModule *module,
        MsgID msgid,
        uint8_t /*paramHi*/,
        uint8_t /*paramLo*/
      ) override {
        switch (msgid) {
          case MID_ENTERSTATE:
            module->selectSource(BasicAudioModule::DEV_USB);
            return this;
          case MID_ACK:
            module->m_source = BasicAudioModule::DEV_USB;
            return &s_init_checking_folder_count;
          default: break;
        }
        return this;
      }
    };
    static InitSelectingUSB s_init_selecting_usb;

    struct InitSelectingSD : public State {
      State *onEvent(
        BasicAudioModule *module,
        MsgID msgid,
        uint8_t /*paramHi*/,
        uint8_t /*paramLo*/
      ) override {
        switch (msgid) {
          case MID_ENTERSTATE:
            module->selectSource(BasicAudioModule::DEV_SDCARD);
            return this;
          case MID_ACK:
            module->m_source = BasicAudioModule::DEV_SDCARD;
            return &s_init_checking_folder_count;
          default: break;
        }
        return this;
      }
    };
    static InitSelectingSD s_init_selecting_sd;

    struct InitCheckingFolderCount : public State {
      State *onEvent(
        BasicAudioModule *module,
        MsgID msgid,
        uint8_t /*paramHi*/,
        uint8_t paramLo
      ) override {
        switch (msgid) {
          case MID_ENTERSTATE:
            module->queryFolderCount();
            return this;
          case MID_FOLDERCOUNT:
            module->m_folders = paramLo;
            Serial.print(F("Audio module initialized.\nSelected: "));
            module->printDeviceName(module->m_source);
            Serial.print(F(" with "));
            Serial.print(module->m_files);
            Serial.print(F(" files and "));
            Serial.print(module->m_folders);
            Serial.println(F(" folders"));
            return nullptr;
          default: break;
        }
        return this;
      }
    };
    static InitCheckingFolderCount s_init_checking_folder_count;

    struct InitStartPlaying : public State {
      State *onEvent(
        BasicAudioModule *module,
        MsgID msgid,
        uint8_t /*paramHi*/,
        uint8_t /*paramLo*/
      ) override {
        switch (msgid) {
          case MID_ENTERSTATE:
            module->sendCommand(MID_LOOPFOLDER, 1);
            return this;
          case MID_ACK:
            return nullptr;
          default: break;
        }
        return this;
      }
    };
    static InitStartPlaying s_init_start_playing;

    void checkForIncomingMessage() {
      while (m_stream.available() > 0) {
        if (m_in.receive(m_stream.read())) {
          receiveMessage(m_in);
        }
      }
    }

    void checkForTimeout() {
      if (m_timeout.expired()) {
        m_timeout.cancel();
        if (m_state) {
          setState(m_state->onEvent(this, MID_ERROR, high(EC_TIMEDOUT), low(EC_TIMEDOUT)));
        }
      }
    }

    void receiveMessage(const Message &msg) {
      onMessageReceived(msg);
      if (!msg.isValid()) return onMessageInvalid();
      if (!m_state) return;
      setState(m_state->onEvent(this, msg.getMessageID(), msg.getParamHi(), msg.getParamLo()));
    }

    void sendMessage(const Message &msg) {
      const auto buf = msg.getBuffer();
      const auto len = msg.getLength();
      m_stream.write(buf, len);
      m_timeout.set(200);
      onMessageSent(buf, len);
    }

    void sendCommand(MsgID msgid, uint16_t param = 0, bool feedback = true) {
      m_out.set(msgid, param, feedback ? Message::FEEDBACK : Message::NO_FEEDBACK);
      sendMessage(m_out);
    }

    void sendQuery(MsgID msgid, uint16_t param = 0) {
      // Since queries naturally have a response, we won't ask for feedback.
      sendCommand(msgid, param, false);
    }

    void setState(State *new_state, uint8_t arg1 = 0, uint8_t arg2 = 0) {
      const auto original_state = m_state;
      while (m_state != new_state) {
        m_state = new_state;
        if (m_state) {
          new_state = m_state->onEvent(this, MID_ENTERSTATE, arg1, arg2);
        }
        // break out of a cycle
        if (m_state == original_state) return;
      }
    }

    // TODO:  Guess the module type.
    enum Module { MOD_UNKNOWN, MOD_CATALEX, MOD_DFPLAYERMINI };

    Stream  &m_stream;
    Message  m_in;
    Message  m_out;
    State   *m_state;
    Timeout<MillisClock> m_timeout;
    Device   m_source;   // the currently selected device
    uint16_t m_files;    // the number of files on the selected device
    uint8_t  m_folders;  // the number of folders on the selected device
};

template <typename SerialType>
class AudioModule : public BasicAudioModule {
  public:
    explicit AudioModule(SerialType &serial) :
      BasicAudioModule(serial), m_serial(serial) {}

    // Initialization to be done during `setup`.
    void begin() override {
      m_serial.begin(9600);
      BasicAudioModule::begin();
    }

  private:
    SerialType &m_serial;
};

template <typename SerialType>
AudioModule<SerialType> make_AudioModule(SerialType &serial) {
  return AudioModule<SerialType>(serial);
}

BasicAudioModule::InitResettingHardware    BasicAudioModule::s_init_resetting_hardware;
BasicAudioModule::InitGettingVersion       BasicAudioModule::s_init_getting_version;
BasicAudioModule::InitCheckingUSBFileCount BasicAudioModule::s_init_checking_usb_file_count;
BasicAudioModule::InitCheckingSDFileCount  BasicAudioModule::s_init_checking_sd_file_count;
BasicAudioModule::InitSelectingUSB         BasicAudioModule::s_init_selecting_usb;
BasicAudioModule::InitSelectingSD          BasicAudioModule::s_init_selecting_sd;
BasicAudioModule::InitCheckingFolderCount  BasicAudioModule::s_init_checking_folder_count;
BasicAudioModule::InitStartPlaying         BasicAudioModule::s_init_start_playing;
