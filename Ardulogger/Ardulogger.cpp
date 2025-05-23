#include "Ardulogger.h"

Ardulogger::Ardulogger(int chipSelectPin)
  : _csPin(chipSelectPin), _filename("log.csv"), _precision(2), _headerWritten(false) {}

bool Ardulogger::begin() {
  if (!_rtc.begin()) {
    Serial.println("RTC not found");
    return false;
  }

  if (_rtc.lostPower()) {
    _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  if (!_sd.begin(_csPin, SD_SCK_MHZ(8))) {
    Serial.println("SD init failed");
    return false;
  }

  if (_sd.exists(_filename.c_str())) {
    _headerWritten = true;
  }

  return true;
}

void Ardulogger::datafile(const String& filename) {
  _filename = filename;
}

void Ardulogger::data(const String& label, float& variable) {
  for (const auto& b : _bindings) {
    if (b.label == label) return;
  }
  _bindings.push_back({ label, &variable });
}

bool Ardulogger::datalog() {
  _file = _sd.open(_filename.c_str(), FILE_WRITE);
  if (!_file) {
    Serial.print("Failed to open file: ");
    Serial.println(_filename);
    return false;
  }

  if (!_headerWritten) {
    _file.print("Timestamp");
    for (const auto& b : _bindings) {
      _file.print(", ");
      _file.print(b.label);
    }
    _file.println();
    _headerWritten = true;
  }

  _file.print(getTimestamp());
  for (const auto& b : _bindings) {
    _file.print(", ");
    _file.print(*(b.ptr), _precision);
  }
  _file.println();
  _file.close();
  return true;
}

void Ardulogger::comment(const String& text) {
  _file = _sd.open(_filename.c_str(), FILE_WRITE);
  if (_file) {
    _file.print("# ");
    _file.print(getTimestamp());
    _file.print(", ");
    _file.println(text);
    _file.close();
  }
}

bool Ardulogger::fileExists() const {
  return _sd.exists(_filename.c_str());
}

void Ardulogger::setPrecision(uint8_t digits) {
  _precision = digits;
}

String Ardulogger::readLastLine() {
  _file = _sd.open(_filename.c_str(), FILE_READ);
  if (!_file) return "";

  String lastLine = "", currentLine = "";
  while (_file.available()) {
    char c = _file.read();
    if (c == '\n') {
      lastLine = currentLine;
      currentLine = "";
    } else {
      currentLine += c;
    }
  }
  _file.close();
  return lastLine;
}

String Ardulogger::getTimestamp() {
  DateTime now = _rtc.now();
  char buffer[20];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(buffer);
}
