#pragma once

#include <Arduino.h>
#include <SdFat.h>
#include <RTClib.h>
#include <vector>

#ifndef ARDULOGGER_NO_PIN
#define ARDULOGGER_NO_PIN 255
#endif

class Ardulogger {
  public:
    Ardulogger(int chipSelectPin);

    bool begin();
    void datafile(const String& filename);
    void data(const String& label, float& variable);
    bool datalog();
    void comment(const String& text);
    bool fileExists();
    void setPrecision(uint8_t digits);
    String readLastLine();

    // Analog multiplexer support
    // selectPins must be ordered from least-significant select bit to most-significant
    // select bit. Example for a CD74HC4067: {S0, S1, S2, S3}.
    void muxBegin(uint8_t analogPin,
                  const uint8_t* selectPins,
                  uint8_t selectPinCount,
                  uint16_t settleTimeUs = 100,
                  uint8_t enablePin = ARDULOGGER_NO_PIN,
                  bool enableActiveLow = true);

    void muxDisable();
    void muxSelect(uint8_t channel);

    // Optional board-label mapping for mux inputs.
    // Example: muxMapAnalogRange(5, 8, 0) maps board labels A5-A12 to mux channels 0-7.
    void muxMapAnalogRange(uint8_t firstAnalogNumber,
                           uint8_t count,
                           uint8_t firstMuxChannel = 0);

    // Optional direct pin-style mapping. This lets calls like
    // value = logger.analogRead(A5);
    // select the correct mux channel first.
    void muxMapPin(uint8_t analogPin, uint8_t muxChannel);
    void muxMapPins(const uint8_t* analogPins,
                    uint8_t count,
                    uint8_t firstMuxChannel = 0);

    int muxChannelForAnalog(uint8_t analogNumber) const;
    int muxChannelForAnalog(const String& analogLabel) const;
    int muxChannelForPin(uint8_t analogPin) const;

    // Arduino-style analog reads. If analogPin/analogLabel is mapped to the mux,
    // the logger switches the mux and reads through the configured mux output pin.
    // If it is not mapped to the mux, this falls back to the normal Arduino ADC.
    int analogRead(uint8_t analogPin, uint8_t samples = 1);
    int analogRead(const String& analogLabel, uint8_t samples = 1);

    float analogReadVoltage(uint8_t analogPin,
                            float vref = 3.3,
                            uint16_t adcMax = 4095,
                            float scale = 1.0,
                            uint8_t samples = 1);

    float analogReadVoltage(const String& analogLabel,
                            float vref = 3.3,
                            uint16_t adcMax = 4095,
                            float scale = 1.0,
                            uint8_t samples = 1);

    float analogReadMilliVolts(uint8_t analogPin,
                               float scale = 1.0,
                               uint8_t samples = 1);

    float analogReadMilliVolts(const String& analogLabel,
                               float scale = 1.0,
                               uint8_t samples = 1);

    int muxReadRaw(uint8_t channel, uint8_t samples = 1);

    float muxReadVoltage(uint8_t channel,
                         float vref = 3.3,
                         uint16_t adcMax = 4095,
                         float scale = 1.0,
                         uint8_t samples = 1);

    float muxReadMilliVolts(uint8_t channel,
                            float scale = 1.0,
                            uint8_t samples = 1);

    float muxReadAnalogVoltage(const String& analogLabel,
                               float vref = 3.3,
                               uint16_t adcMax = 4095,
                               float scale = 1.0,
                               uint8_t samples = 1);

    float muxReadAnalogMilliVolts(const String& analogLabel,
                                  float scale = 1.0,
                                  uint8_t samples = 1);

    void muxData(const String& label,
                 float& variable,
                 uint8_t channel,
                 float vref = 3.3,
                 uint16_t adcMax = 4095,
                 float scale = 1.0,
                 uint8_t samples = 1);

    void muxDataMilliVolts(const String& label,
                           float& variable,
                           uint8_t channel,
                           float scale = 1.0,
                           uint8_t samples = 1);

    void muxDataAnalog(const String& label,
                       float& variable,
                       const String& analogLabel,
                       float vref = 3.3,
                       uint16_t adcMax = 4095,
                       float scale = 1.0,
                       uint8_t samples = 1);

    void muxDataAnalogMilliVolts(const String& label,
                                 float& variable,
                                 const String& analogLabel,
                                 float scale = 1.0,
                                 uint8_t samples = 1);

    bool updateMuxData();

  private:
    struct Binding {
      String label;
      float* ptr;
    };

    struct MuxBinding {
      String label;
      float* ptr;
      uint8_t channel;
      float vref;
      uint16_t adcMax;
      float scale;
      uint8_t samples;
      bool useMilliVolts;
    };

    struct MuxConfig {
      bool configured;
      uint8_t analogPin;
      std::vector<uint8_t> selectPins;
      uint16_t settleTimeUs;
      uint8_t enablePin;
      bool enableActiveLow;
      bool analogMapConfigured;
      uint8_t firstAnalogNumber;
      uint8_t analogCount;
      uint8_t firstMuxChannel;
    };

    struct MuxPinMap {
      uint8_t analogPin;
      uint8_t channel;
    };

    int _csPin;
    String _filename;
    RTC_DS3231 _rtc;
    std::vector<Binding> _bindings;
    std::vector<MuxBinding> _muxBindings;
    std::vector<MuxPinMap> _muxPinMap;
    MuxConfig _mux;
    uint8_t _precision;
    bool _headerWritten;
    SdFat _sd;
    File _file;

    String getTimestamp();
    void setMuxEnable(bool enabled);
    uint16_t muxChannelCount() const;
};
