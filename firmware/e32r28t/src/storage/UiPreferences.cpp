#include "UiPreferences.h"
static uint32_t checksum(const TouchCalibration &c) {
  const uint8_t *p = reinterpret_cast<const uint8_t *>(&c);
  uint32_t v = 0x12345678u;
  for (size_t i = 0; i < sizeof(TouchCalibration) - sizeof(uint32_t) - sizeof(bool); ++i) v = (v * 33u) ^ p[i];
  return v;
}
void UiPreferences::begin() { prefs.begin("918ui", false); }
TouchCalibration UiPreferences::loadCalibration() {
  TouchCalibration c;
  c.version = prefs.getUShort("cver", 0);
  c.checksum = prefs.getUInt("cck", 0);
  c.minX = prefs.getShort("minx", 0);
  c.maxX = prefs.getShort("maxx", 0);
  c.minY = prefs.getShort("miny", 0);
  c.maxY = prefs.getShort("maxy", 0);
  c.swapXY = prefs.getBool("swap", false);
  c.invX = prefs.getBool("invx", false);
  c.invY = prefs.getBool("invy", false);
  c.valid = (c.version == 2 && c.minX < c.maxX && c.minY < c.maxY);
  TouchCalibration t = c; t.checksum = 0;
  c.valid = c.valid && c.checksum == checksum(t);
  return c;
}
void UiPreferences::saveCalibration(const TouchCalibration &cal) {
  TouchCalibration t = cal;
  t.version = 2;
  t.checksum = 0;
  prefs.putUShort("cver", 2);
  prefs.putShort("minx", cal.minX);
  prefs.putShort("maxx", cal.maxX);
  prefs.putShort("miny", cal.minY);
  prefs.putShort("maxy", cal.maxY);
  prefs.putBool("swap", cal.swapXY);
  prefs.putBool("invx", cal.invX);
  prefs.putBool("invy", cal.invY);
  prefs.putUInt("cck", checksum(t));
}
UiSettings UiPreferences::loadSettings() {
  UiSettings s;
  s.brightness = prefs.getUChar("bri", 255);
  s.soundEnabled = prefs.getBool("snd", true);
  s.approvalTimeoutS = prefs.getUShort("ato", 20);
  s.rotation = prefs.getUChar("rot", 0);
  return s;
}
void UiPreferences::saveSettings(const UiSettings &s) {
  prefs.putUChar("bri", s.brightness);
  prefs.putBool("snd", s.soundEnabled);
  prefs.putUShort("ato", s.approvalTimeoutS);
  prefs.putUChar("rot", s.rotation);
}
void UiPreferences::rememberResetReason(uint32_t reason){ prefs.putUInt("rr", reason); }
uint32_t UiPreferences::resetReason() { return prefs.getUInt("rr", 0); }
void UiPreferences::clear() { prefs.clear(); }
