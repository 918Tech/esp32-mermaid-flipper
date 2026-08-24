#pragma once
#include <Arduino.h>
#include <SPI.h>
#include "../storage/UiPreferences.h"
#include "../vendor/XPT2046_Touchscreen.h"
struct TouchPoint { int16_t x{0}; int16_t y{0}; bool down{false}; bool valid{false}; };
class TouchController {
 public:
  bool begin();
  void setCalibration(const TouchCalibration &cal) { _cal = cal; }
  bool calibrationValid() const { return _cal.valid; }
  bool hasCalibration() const { return _cal.valid; }
  TouchPoint sample();
  void forceRawMode(bool enabled) { _rawMode = enabled; }
  uint32_t lastSampleUs() const { return _lastSampleUs; }
  SPIClass &bus() { return _bus; }
 private:
  static constexpr uint8_t TouchSck = 25;
  static constexpr uint8_t TouchMiso = 39;
  static constexpr uint8_t TouchMosi = 32;
  static constexpr uint8_t TouchCs = 33;
  static constexpr uint8_t TouchIrq = 36;
  SPIClass _bus{VSPI};
  XPT2046_Touchscreen _ts{TouchCs, TouchIrq};
  TouchCalibration _cal{};
  uint32_t _lastSampleUs{0};
  bool _rawMode{false};
};
