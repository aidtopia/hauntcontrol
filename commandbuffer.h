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
        if (ch != '\n') {
          buf[buflen++] = ch;
          continue;
        }

        buf[buflen] = '\0';
        buflen = 0;
        Serial.print(F("> "));
        Serial.println(buf);
        return true;
      }
      return false;
    }

    operator const char *() const { return buf; }

  private:
    char buf[BUFFER_SIZE];
    int buflen = 0;
};
