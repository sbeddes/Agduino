#pragma once

#include <Arduino.h>
#include <SdFat.h>
#include <RTClib.h>
#include <vector>

class Ardulogger {
  public:
    Ardulogger(int chipSelectPin);

    bool begin();
    void datafile(const String& filename);
    void data(const String& label, float& variable);
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
    SdFat _sd;
    File _file;

    String getTimestamp();
};
