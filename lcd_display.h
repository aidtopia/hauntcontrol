// Class to control a SparkFun SerLCD display.
// Adrian McCarthy 2021

template <typename SerialType>
class LCD {
  public:
    explicit LCD(SerialType &serial) :
      m_serial(&serial),

      // We don't know the initial backlight brightness setting, so we'll
      // set this to 0, which will force us to acknowledge the first
      // brightness command the caller makes.
      m_brightness(0),

      // On powerup, the display has a splash screen that takes 500 ms.
      // We can't send any commands before that.
      m_block_until(millis() + 501)
    {}

    void begin(long baud = 9600) {
      m_serial->begin(baud);
      clear();
    }

    // Set the backlight brightness to a percentage from 0 to 100.
    void setBacklight(int percent) {
      if (percent < 0) percent = 0;
      if (percent > 100) percent = 100;
      const int setting = (29*percent / 100) + 128;
      if (m_brightness != setting) {
        m_brightness = setting;
        sendInterfaceCommand(m_brightness);
      }
    }

    // Clear the display and home the cursor.
    void clear() { sendCommand(1); }

    // Hide or show the cursor.
    void cursorOff() { sendCommand(0x0C); }
    void cursorOn() { sendCommand(0x0E); }

    // Move the cursor to a particular row and column (0-based).
    void moveTo(int row, int col) {
      if (row < 0) row = 0;
      if (row > 1) row = 1;
      if (col < 0) col = 0;
      if (col > 15) col = 15;
      const int pos = 64*row + col + 128;
      sendCommand(pos);
    }

    template <typename T>
    size_t print(T data) {
      waitForReady();
      return m_serial->print(data);
    }

    template <typename T>
    size_t println(T data) {
      waitForReady();
      const auto count = m_serial->print(data);
      moveTo(1, 0);
      return count;
    }

    void scrollRight() {
      sendCommand(0x1C);
    }

    void scrollLeft() {
      sendCommand(0x18);
    }

    bool update() { return false; }

    size_t write(const char *text) {
      waitForReady();
      return m_serial->write(text);
    }

    size_t write(char ch) {
      waitForReady();
      return m_serial->write(&ch, 1);
    }

  private:
    void sendCommand(char ch) {
      waitForReady();
      m_serial->write(0xFE);
      m_serial->write(ch);
    }

    void sendInterfaceCommand(char ch) {
      waitForReady();
      m_serial->write(0x7C);
      m_serial->write(ch);
      // This is not documented, but apparently you need a delay after
      // sending an interface command or the display can lock up.
      m_block_until = millis() + 500;
    }

    void waitForReady() {
      // We do not account for wraparound after 49 days.
      const unsigned long now = millis();
      if (m_block_until > now) {
        delay(m_block_until - now);
      }
    }

    SerialType * const m_serial;
    int m_brightness;
    unsigned long m_block_until;
};

template <typename SerialType>
LCD<SerialType> make_LCD(SerialType &serial) {
  return LCD<SerialType>(serial);
}
