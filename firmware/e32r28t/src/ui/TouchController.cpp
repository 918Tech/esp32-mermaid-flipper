#include "TouchController.h"

static int16_t median3(int16_t a, int16_t b, int16_t c) {
  if (a > b) { int16_t t = a; a = b; b = t; }
  if (b > c) { int16_t t = b; b = c; c = t; }
  if (a > b) { int16_t t = a; a = b; b = t; }
  return b;
}

bool TouchController::begin() {
  pinMode(TouchCs, OUTPUT);
  digitalWrite(TouchCs, HIGH);

  // GPIO36 is input-only and has no reliable internal pull-up on ESP32.
  // Do not gate touch reads on IRQ; SPI pressure/raw samples are authoritative.
  pinMode(TouchIrq, INPUT);

  _bus.begin(TouchSck, TouchMiso, TouchMosi, TouchCs);
  _ts.begin(_bus);
  _ts.setRotation(0);
  _rawMode = true;
  return true;
}

TouchPoint TouchController::sample() {
  TouchPoint out{};

  if (micros() - _lastSampleUs < 5000) return out;
  _lastSampleUs = micros();

  TS_Point p1 = _ts.getPoint();
  delayMicroseconds(250);
  TS_Point p2 = _ts.getPoint();
  delayMicroseconds(250);
  TS_Point p3 = _ts.getPoint();

  const int16_t rawX = median3(p1.x, p2.x, p3.x);
  const int16_t rawY = median3(p1.y, p2.y, p3.y);
  const int16_t rawZ = median3(p1.z, p2.z, p3.z);

  // XPT2046 idle/no-touch reads collapse toward the rails.
  if (rawZ < 80 || rawX < 80 || rawX > 4015 || rawY < 80 || rawY > 4015) return out;

  out.rawX = rawX;
  out.rawY = rawY;

  if (_cal.valid) {
    long a = rawX;
    long b = rawY;

    if (_cal.swapXY) {
      long t = a;
      a = b;
      b = t;
    }

    long x = map(a, _cal.minX, _cal.maxX, 0, 239);
    long y = map(b, _cal.minY, _cal.maxY, 0, 319);

    if (_cal.invX) x = 239 - x;
    if (_cal.invY) y = 319 - y;

    out.x = constrain((int16_t)x, (int16_t)0, (int16_t)239);
    out.y = constrain((int16_t)y, (int16_t)0, (int16_t)319);
    out.valid = true;
    out.down = true;
    return out;
  }

  if (!_rawMode) return out;

  // Provisional mapping is only for the calibration interaction.
  out.x = constrain((int16_t)map(rawX, 200, 3900, 0, 239), (int16_t)0, (int16_t)239);
  out.y = constrain((int16_t)map(rawY, 200, 3900, 0, 319), (int16_t)0, (int16_t)319);
  out.valid = true;
  out.down = true;
  return out;
}
