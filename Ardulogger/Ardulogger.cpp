#include "Ardulogger.h"
#include <math.h>

Ardulogger::Ardulogger(int chipSelectPin)
  : _csPin(chipSelectPin),
    _filename("log.csv"),
    _precision(2),
    _headerWritten(false) {
  _mux.configured = false;
  _mux.analogPin = ARDULOGGER_NO_PIN;
  _mux.settleTimeUs = 100;
  _mux.enablePin = ARDULOGGER_NO_PIN;
  _mux.enableActiveLow = true;
  _mux.analogMapConfigured = false;
  _mux.firstAnalogNumber = 0;
  _mux.analogCount = 0;
  _mux.firstMuxChannel = 0;
}

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
  if (!updateMuxData()) {
    return false;
  }

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

bool Ardulogger::fileExists() {
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

void Ardulogger::muxBegin(uint8_t analogPin,
                          const uint8_t* selectPins,
                          uint8_t selectPinCount,
                          uint16_t settleTimeUs,
                          uint8_t enablePin,
                          bool enableActiveLow) {
  _mux.analogPin = analogPin;
  _mux.selectPins.clear();
  _mux.settleTimeUs = settleTimeUs;
  _mux.enablePin = enablePin;
  _mux.enableActiveLow = enableActiveLow;

  pinMode(_mux.analogPin, INPUT);

  for (uint8_t i = 0; i < selectPinCount; i++) {
    _mux.selectPins.push_back(selectPins[i]);
    pinMode(selectPins[i], OUTPUT);
    digitalWrite(selectPins[i], LOW);
  }

  if (_mux.enablePin != ARDULOGGER_NO_PIN) {
    pinMode(_mux.enablePin, OUTPUT);
  }

  _mux.configured = true;
  setMuxEnable(true);
  muxSelect(0);
}

void Ardulogger::muxDisable() {
  setMuxEnable(false);
}

void Ardulogger::setMuxEnable(bool enabled) {
  if (_mux.enablePin == ARDULOGGER_NO_PIN) return;

  bool level;
  if (_mux.enableActiveLow) {
    level = enabled ? LOW : HIGH;
  } else {
    level = enabled ? HIGH : LOW;
  }

  digitalWrite(_mux.enablePin, level);
}

uint16_t Ardulogger::muxChannelCount() const {
  if (_mux.selectPins.size() >= 16) return 65535;
  return 1U << _mux.selectPins.size();
}

void Ardulogger::muxMapAnalogRange(uint8_t firstAnalogNumber,
                                   uint8_t count,
                                   uint8_t firstMuxChannel) {
  _mux.firstAnalogNumber = firstAnalogNumber;
  _mux.analogCount = count;
  _mux.firstMuxChannel = firstMuxChannel;
  _mux.analogMapConfigured = (count > 0);

  if (_mux.configured && (firstMuxChannel + count > muxChannelCount())) {
    Serial.println("Mux analog range is larger than available mux channels.");
  }
}

void Ardulogger::muxMapPin(uint8_t analogPin, uint8_t muxChannel) {
  if (_mux.configured && muxChannel >= muxChannelCount()) {
    Serial.println("Mux pin map channel is out of range.");
    return;
  }

  for (auto& m : _muxPinMap) {
    if (m.analogPin == analogPin) {
      m.channel = muxChannel;
      return;
    }
  }

  _muxPinMap.push_back({ analogPin, muxChannel });
}

void Ardulogger::muxMapPins(const uint8_t* analogPins,
                            uint8_t count,
                            uint8_t firstMuxChannel) {
  for (uint8_t i = 0; i < count; i++) {
    muxMapPin(analogPins[i], firstMuxChannel + i);
  }
}

int Ardulogger::muxChannelForAnalog(uint8_t analogNumber) const {
  if (!_mux.analogMapConfigured) return -1;

  if (analogNumber < _mux.firstAnalogNumber) return -1;

  uint8_t offset = analogNumber - _mux.firstAnalogNumber;
  if (offset >= _mux.analogCount) return -1;

  uint16_t channel = (uint16_t)_mux.firstMuxChannel + offset;
  if (_mux.configured && channel >= muxChannelCount()) return -1;

  return (int)channel;
}

int Ardulogger::muxChannelForAnalog(const String& analogLabel) const {
  String s = analogLabel;
  s.trim();
  s.toUpperCase();

  if (s.length() < 2 || s[0] != 'A') return -1;

  uint16_t analogNumber = 0;
  for (uint8_t i = 1; i < s.length(); i++) {
    char c = s[i];
    if (c < '0' || c > '9') return -1;
    analogNumber = analogNumber * 10 + (uint16_t)(c - '0');
    if (analogNumber > 255) return -1;
  }

  return muxChannelForAnalog((uint8_t)analogNumber);
}

int Ardulogger::muxChannelForPin(uint8_t analogPin) const {
  for (const auto& m : _muxPinMap) {
    if (m.analogPin == analogPin) return m.channel;
  }

  // This allows logger.analogRead(5) to mean logical board input A5
  // when muxMapAnalogRange(5, 8, 0) has been called.
  int channel = muxChannelForAnalog(analogPin);
  if (channel >= 0) return channel;

  // If the Arduino A5 macro is not numerically equal to 5, but A5 is also
  // the mux output pin, this makes logger.analogRead(A5) select the first
  // mapped mux channel instead of reading the mux output directly.
  if (_mux.configured && _mux.analogMapConfigured && analogPin == _mux.analogPin) {
    return muxChannelForAnalog(_mux.firstAnalogNumber);
  }

  return -1;
}

int Ardulogger::analogRead(uint8_t analogPin, uint8_t samples) {
  if (samples == 0) samples = 1;

  int channel = muxChannelForPin(analogPin);
  if (channel >= 0) {
    return muxReadRaw((uint8_t)channel, samples);
  }

  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; i++) {
    sum += ::analogRead(analogPin);
    if (i < samples - 1 && _mux.configured) delayMicroseconds(_mux.settleTimeUs);
  }

  return sum / samples;
}

int Ardulogger::analogRead(const String& analogLabel, uint8_t samples) {
  int channel = muxChannelForAnalog(analogLabel);
  if (channel < 0) {
    Serial.print("Analog label not mapped to mux channel: ");
    Serial.println(analogLabel);
    return -1;
  }

  return muxReadRaw((uint8_t)channel, samples);
}

float Ardulogger::analogReadVoltage(uint8_t analogPin,
                                    float vref,
                                    uint16_t adcMax,
                                    float scale,
                                    uint8_t samples) {
  int raw = analogRead(analogPin, samples);
  if (raw < 0 || adcMax == 0) return NAN;

  return ((float)raw * vref / (float)adcMax) * scale;
}

float Ardulogger::analogReadVoltage(const String& analogLabel,
                                    float vref,
                                    uint16_t adcMax,
                                    float scale,
                                    uint8_t samples) {
  int channel = muxChannelForAnalog(analogLabel);
  if (channel < 0) {
    Serial.print("Analog label not mapped to mux channel: ");
    Serial.println(analogLabel);
    return NAN;
  }

  return muxReadVoltage((uint8_t)channel, vref, adcMax, scale, samples);
}

float Ardulogger::analogReadMilliVolts(uint8_t analogPin,
                                       float scale,
                                       uint8_t samples) {
  if (samples == 0) samples = 1;

  int channel = muxChannelForPin(analogPin);
  if (channel >= 0) {
    return muxReadMilliVolts((uint8_t)channel, scale, samples);
  }

#if defined(ESP32)
  ::analogReadMilliVolts(analogPin);
  if (_mux.configured) delayMicroseconds(_mux.settleTimeUs);

  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; i++) {
    sum += ::analogReadMilliVolts(analogPin);
    if (i < samples - 1 && _mux.configured) delayMicroseconds(_mux.settleTimeUs);
  }

  return ((float)sum / (float)samples) * scale;
#else
  return analogReadVoltage(analogPin, 5.0, 1023, 1.0, samples) * 1000.0 * scale;
#endif
}

float Ardulogger::analogReadMilliVolts(const String& analogLabel,
                                       float scale,
                                       uint8_t samples) {
  int channel = muxChannelForAnalog(analogLabel);
  if (channel < 0) {
    Serial.print("Analog label not mapped to mux channel: ");
    Serial.println(analogLabel);
    return NAN;
  }

  return muxReadMilliVolts((uint8_t)channel, scale, samples);
}

void Ardulogger::muxSelect(uint8_t channel) {
  if (!_mux.configured) {
    Serial.println("Mux not configured. Call muxBegin() first.");
    return;
  }

  if (channel >= muxChannelCount()) {
    Serial.println("Mux channel out of range.");
    return;
  }

  setMuxEnable(true);

  for (uint8_t i = 0; i < _mux.selectPins.size(); i++) {
    digitalWrite(_mux.selectPins[i], (channel >> i) & 0x01);
  }

  delayMicroseconds(_mux.settleTimeUs);
}

int Ardulogger::muxReadRaw(uint8_t channel, uint8_t samples) {
  if (!_mux.configured) {
    Serial.println("Mux not configured. Call muxBegin() first.");
    return -1;
  }

  if (channel >= muxChannelCount()) {
    Serial.println("Mux channel out of range.");
    return -1;
  }

  if (samples == 0) samples = 1;

  muxSelect(channel);

  // Throw away the first reading after switching channels so the ADC sample
  // capacitor has a chance to settle to the new mux channel voltage.
  ::analogRead(_mux.analogPin);
  delayMicroseconds(_mux.settleTimeUs);

  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; i++) {
    sum += ::analogRead(_mux.analogPin);
    if (i < samples - 1) delayMicroseconds(_mux.settleTimeUs);
  }

  return sum / samples;
}

float Ardulogger::muxReadVoltage(uint8_t channel,
                                 float vref,
                                 uint16_t adcMax,
                                 float scale,
                                 uint8_t samples) {
  int raw = muxReadRaw(channel, samples);
  if (raw < 0 || adcMax == 0) return NAN;

  return ((float)raw * vref / (float)adcMax) * scale;
}

float Ardulogger::muxReadMilliVolts(uint8_t channel, float scale, uint8_t samples) {
  if (!_mux.configured) {
    Serial.println("Mux not configured. Call muxBegin() first.");
    return NAN;
  }

  if (channel >= muxChannelCount()) {
    Serial.println("Mux channel out of range.");
    return NAN;
  }

  if (samples == 0) samples = 1;

  muxSelect(channel);

#if defined(ESP32)
  // ESP32 Arduino core provides calibrated millivolt readings.
  ::analogReadMilliVolts(_mux.analogPin);
  delayMicroseconds(_mux.settleTimeUs);

  uint32_t sum = 0;
  for (uint8_t i = 0; i < samples; i++) {
    sum += ::analogReadMilliVolts(_mux.analogPin);
    if (i < samples - 1) delayMicroseconds(_mux.settleTimeUs);
  }

  return ((float)sum / (float)samples) * scale;
#else
  // Fallback for non-ESP32 boards. Adjust vref/adcMax with muxReadVoltage()
  // if your board does not use 5V and 10-bit ADC readings.
  return muxReadVoltage(channel, 5.0, 1023, scale, samples) * 1000.0;
#endif
}

float Ardulogger::muxReadAnalogVoltage(const String& analogLabel,
                                       float vref,
                                       uint16_t adcMax,
                                       float scale,
                                       uint8_t samples) {
  int channel = muxChannelForAnalog(analogLabel);
  if (channel < 0) {
    Serial.print("Analog label not mapped to mux channel: ");
    Serial.println(analogLabel);
    return NAN;
  }

  return muxReadVoltage((uint8_t)channel, vref, adcMax, scale, samples);
}

float Ardulogger::muxReadAnalogMilliVolts(const String& analogLabel,
                                          float scale,
                                          uint8_t samples) {
  int channel = muxChannelForAnalog(analogLabel);
  if (channel < 0) {
    Serial.print("Analog label not mapped to mux channel: ");
    Serial.println(analogLabel);
    return NAN;
  }

  return muxReadMilliVolts((uint8_t)channel, scale, samples);
}

void Ardulogger::muxData(const String& label,
                         float& variable,
                         uint8_t channel,
                         float vref,
                         uint16_t adcMax,
                         float scale,
                         uint8_t samples) {
  data(label, variable);

  for (auto& b : _muxBindings) {
    if (b.label == label) {
      b.ptr = &variable;
      b.channel = channel;
      b.vref = vref;
      b.adcMax = adcMax;
      b.scale = scale;
      b.samples = samples;
      b.useMilliVolts = false;
      return;
    }
  }

  _muxBindings.push_back({ label, &variable, channel, vref, adcMax, scale, samples, false });
}

void Ardulogger::muxDataMilliVolts(const String& label,
                                   float& variable,
                                   uint8_t channel,
                                   float scale,
                                   uint8_t samples) {
  data(label, variable);

  for (auto& b : _muxBindings) {
    if (b.label == label) {
      b.ptr = &variable;
      b.channel = channel;
      b.vref = 3.3;
      b.adcMax = 4095;
      b.scale = scale;
      b.samples = samples;
      b.useMilliVolts = true;
      return;
    }
  }

  _muxBindings.push_back({ label, &variable, channel, 3.3, 4095, scale, samples, true });
}

void Ardulogger::muxDataAnalog(const String& label,
                               float& variable,
                               const String& analogLabel,
                               float vref,
                               uint16_t adcMax,
                               float scale,
                               uint8_t samples) {
  int channel = muxChannelForAnalog(analogLabel);
  if (channel < 0) {
    Serial.print("Analog label not mapped to mux channel: ");
    Serial.println(analogLabel);
    variable = NAN;
    return;
  }

  muxData(label, variable, (uint8_t)channel, vref, adcMax, scale, samples);
}

void Ardulogger::muxDataAnalogMilliVolts(const String& label,
                                         float& variable,
                                         const String& analogLabel,
                                         float scale,
                                         uint8_t samples) {
  int channel = muxChannelForAnalog(analogLabel);
  if (channel < 0) {
    Serial.print("Analog label not mapped to mux channel: ");
    Serial.println(analogLabel);
    variable = NAN;
    return;
  }

  muxDataMilliVolts(label, variable, (uint8_t)channel, scale, samples);
}

bool Ardulogger::updateMuxData() {
  if (_muxBindings.empty()) return true;

  if (!_mux.configured) {
    Serial.println("Mux data registered, but muxBegin() was not called.");
    return false;
  }

  for (auto& b : _muxBindings) {
    if (b.useMilliVolts) {
      *(b.ptr) = muxReadMilliVolts(b.channel, b.scale, b.samples);
    } else {
      *(b.ptr) = muxReadVoltage(b.channel, b.vref, b.adcMax, b.scale, b.samples);
    }
  }

  return true;
}

String Ardulogger::getTimestamp() {
  DateTime now = _rtc.now();
  char buffer[20];
  sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
          now.year(), now.month(), now.day(),
          now.hour(), now.minute(), now.second());
  return String(buffer);
}
