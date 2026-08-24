#pragma once
#include <Arduino.h>
struct TouchButton {
  int16_t x{}, y{}, w{}, h{};
  const char *label{};
  uint16_t fill{}, border{}, text{};
  bool enabled{true};
  bool pressed{false};
  bool hit(int16_t px, int16_t py) const { return px >= x && px < x + w && py >= y && py < y + h; }
};
