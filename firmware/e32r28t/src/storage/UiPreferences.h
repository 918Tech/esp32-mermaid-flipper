#pragma once
#include <Arduino.h>
#include <Preferences.h>
struct TouchCalibration { uint16_t version{0}; uint32_t checksum{0}; int16_t minX{0}, maxX{0}, minY{0}, maxY{0}; bool swapXY{false}, invX{false}, invY{false}; bool valid{false}; };
struct UiSettings { uint8_t brightness{255}; bool soundEnabled{true}; uint16_t approvalTimeoutS{20}; uint8_t rotation{0}; };
class UiPreferences {
 public:
  void begin();
  TouchCalibration loadCalibration();
  void saveCalibration(const TouchCalibration &cal);
  UiSettings loadSettings();
  void saveSettings(const UiSettings &settings);
  void rememberResetReason(uint32_t reason);
  uint32_t resetReason();
  void clear();
 private:
  Preferences prefs;
};
