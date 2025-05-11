#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>
#include <RTClib.h>
#include <SD.h>
#include <vector>

class Logger {
  public:
    Logger(int chipSelectPin);

    bool begin();
    void datafile(const String& filename);
    void data(const String& label, float& variable);  // Automatic reference binding
    bool datalog();
    void comment(const String& text);
    bool fileExists() const;
    void setPrecision(uint8_t digits);
    String readLastLine();

  private:
    struct Binding {
      String label;
      float* ptr;
    };

    int _csPin;
    String _filename;
    RTC_DS3231 _rtc;
    std::vector<Binding> _bindings;
    uint8_t _precision;
    bool _headerWritten;

    String getTimestamp() const;
};

#endif
