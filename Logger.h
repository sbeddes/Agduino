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
    void header(const std::vector<String>& headers);
    void data(float value);
    void data(const String& value);
    bool datalog();
    void comment(const String& text);
    bool fileExists() const;
    void setPrecision(uint8_t digits);
    String readLastLine();

  private:
    int _csPin;
    String _filename;
    RTC_DS3231 _rtc;
    std::vector<String> _dataBuffer;
    uint8_t _precision;

    String getTimestamp() const;
};

#endif
