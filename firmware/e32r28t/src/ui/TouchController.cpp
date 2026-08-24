#include "TouchController.h"
static int16_t median3(int16_t a, int16_t b, int16_t c) {
  if (a > b) { int16_t t = a; a = b; b = t; }
  if (b > c) { int16_t t = b; b = c; c = t; }
  if (a > b) { int16_t t = a; a = b; b = t; }
  return b;
}
bool TouchController::begin() {
  pinMode(TouchCs, OUTPUT); digitalWrite(TouchCs, HIGH);
  pinMode(TouchIrq, INPUT_PULLUP);
  _bus.begin(TouchSck, TouchMiso, TouchMosi, TouchCs);
  _ts.begin(_bus);
  _ts.setRotation(0);
  _rawMode = true;
  return true;
}
TouchPoint TouchController::sample() {
  TouchPoint out{};
  if (digitalRead(TouchIrq) != LOW) return out;
  if (micros() - _lastSampleUs < 5000) return out;
  _lastSampleUs = micros();
  if (!_ts.touched()) return out;
  TS_Point p1 = _ts.getPoint();
  delayMicroseconds(300);
  TS_Point p2 = _ts.getPoint();
  delayMicroseconds(300);
  TS_Point p3 = _ts.getPoint();
  int16_t rawX = median3(p1.x, p2.x, p3.x);
  int16_t rawY = median3(p1.y, p2.y, p3.y);
  if (rawX <= 0 || rawY <= 0) return out;
  if (_cal.valid) {
    if (rawX < _cal.minX || rawX > _cal.maxX || rawY < _cal.minY || rawY > _cal.maxY) return out;
    long x = map(rawX, _cal.minX, _cal.maxX, 0, 239);
    long y = map(rawY, _cal.minY, _cal.maxY, 0, 319);
    if (_cal.swapXY) { long t = x; x = y; y = t; }
    if (_cal.invX) x = 239 - x;
    if (_cal.invY) y = 319 - y;
    out.x = constrain((int16_t)x, (int16_t)0, (int16_t)239);
    out.y = constrain((int16_t)y, (int16_t)0, (int16_t)319);
    out.valid = true;
    out.down = true;
    return out;
  }
  if (!_rawMode) return out;
  out.x = map(rawX, 0, 4095, 0, 239);
  out.y = map(rawY, 0, 4095, 0, 319);
  out.x = constrain(out.x, 0, 239);
  out.y = constrain(out.y, 0, 319);
  out.down = true;
  out.valid = true;
  return out;
}
