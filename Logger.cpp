#include "Logger.h"

Logger::Logger(int chipSelectPin)
  : _csPin(chipSelectPin), _filename("log.csv"), _precision(2), _headerWritten(false) {}

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

  if (SD.exists(_filename)) {
    _headerWritten = true;
  }

  return true;
}

void Logger::datafile(const String& filename) {
  _filename = filename;
}

void Logger::data(const String& label, float& variable) {
  // Only bind unique labels
  for (const auto& b : _bindings) {
    if (b.label == label) return;
  }
  _bindings.push_back({ label, &variable });
}

bool Logger::datalog() {
  File file = SD.open(_filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file");
    return false;
  }

  if (!_headerWritten) {
    file.print("Timestamp");
    for (const auto& b : _bindings) {
      file.print(", ");
      file.print(b.label);
    }
    file.println();
    _headerWritten = true;
  }

  file.print(getTimestamp());
  for (const auto& b : _bindings) {
    file.print(", ");
    file.print(*(b.ptr), _precision);
  }
  file.println();
  file.close();
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
