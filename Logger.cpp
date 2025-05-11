#include "Logger.h"

Logger::Logger(int chipSelectPin)
  : _csPin(chipSelectPin), _filename("log.csv"), _precision(2) {}

bool Logger::begin() {
  if (!_rtc.begin()) {
    Serial.println("RTC not found");
    return false;
  }

  if (_rtc.lostPower()) {
    _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!SD.begin(_csPin)) {
    Serial.println("SD init failed");
    return false;
  }

  return true;
}

void Logger::datafile(const String& filename) {
  _filename = filename;
}

void Logger::header(const std::vector<String>& headers) {
  if (!fileExists()) {
    File file = SD.open(_filename, FILE_WRITE);
    if (file) {
      file.print("Timestamp");
      for (const auto& h : headers) {
        file.print(", ");
        file.print(h);
      }
      file.println();
      file.close();
    }
  }
}

void Logger::data(float value) {
  _dataBuffer.push_back(String(value, _precision));
}

void Logger::data(const String& value) {
  _dataBuffer.push_back(value);
}

bool Logger::datalog() {
  File file = SD.open(_filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file");
    return false;
  }

  file.print(getTimestamp());
  for (const auto& val : _dataBuffer) {
    file.print(", ");
    file.print(val);
  }
  file.println();
  file.close();
  _dataBuffer.clear();
  return true;
}

void Logger::comment(const String& text) {
  File file = SD.open(_filename, FILE_WRITE);
  if (file) {
    file.print("# ");
    file.print(getTimestamp());
    file.print(", ");
    file.println(text);
    file.close();
  }
}

bool Logger::fileExists() const {
  return SD.exists(_filename);
}

void Logger::setPrecision(uint8_t digits) {
  _precision = digits;
}

String Logger::readLastLine() {
  File file = SD.open(_filename, FILE_READ);
  if (!file) return "";

  String lastLine = "", currentLine = "";
  while (file.available()) {
    char c = file.read();
    if (c == '\n') {
      lastLine = currentLine;
      currentLine = "";
    } else {
      currentLine += c;
    }
  }
  file.close();
  return lastLine;
}

String Logger::getTimestamp() const {
  DateTime now = _rtc.now();
  char buffer[20];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(buffer);
}
