#pragma once
#include <Arduino.h>
#include <SPI.h>

struct TS_Point {
  int16_t x;
  int16_t y;
  int16_t z;
};

class XPT2046_Touchscreen {
 public:
  XPT2046_Touchscreen(uint8_t csPin, uint8_t irqPin = 255)
      : _csPin(csPin), _irqPin(irqPin) {}

  void begin(SPIClass &bus) {
    _bus = &bus;
    pinMode(_csPin, OUTPUT);
    digitalWrite(_csPin, HIGH);
    if (_irqPin != 255) pinMode(_irqPin, INPUT_PULLUP);
  }

  void setRotation(uint8_t) {}
  bool touched() const { return _irqPin != 255 && digitalRead(_irqPin) == LOW; }

  TS_Point getPoint() {
    if (_bus == nullptr) return {0, 0, 0};
    _bus->beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
    digitalWrite(_csPin, LOW);
    TS_Point p{static_cast<int16_t>(read12(0xD0)),
               static_cast<int16_t>(read12(0x90)),
               static_cast<int16_t>(read12(0xB0))};
    digitalWrite(_csPin, HIGH);
    _bus->endTransaction();
    return p;
  }

 private:
  uint16_t read12(uint8_t command) {
    _bus->transfer(command);
    uint16_t hi = _bus->transfer(0x00);
    uint16_t lo = _bus->transfer(0x00);
    return ((hi << 8) | lo) >> 3;
  }

  SPIClass *_bus{nullptr};
  uint8_t _csPin;
  uint8_t _irqPin;
};
