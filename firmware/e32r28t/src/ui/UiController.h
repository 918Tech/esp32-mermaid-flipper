#pragma once
#include <Arduino.h>
#include "../../User_Setup.h"
#include <TFT_eSPI.h>
#include "../protocol/Proposal.h"
#include "../storage/UiPreferences.h"
#include "TouchController.h"
#include "UiState.h"

struct DirtyRegions {
  bool header{true};
  bool content{true};
  bool navigation{true};
  bool overlay{true};
};

class UiController {
 public:
  void begin();
  void loop();
  void setMode(UiMode mode);
  void setPage(UiPage page);
  void setProposal(const Proposal &proposal);
  void setLinkState(const char *name, LinkState state);
  void setLastCommand(const char *text);
  void setLastResponse(const char *text);
  void pushTerminalLine(const char *line);
  void setScanStats(uint8_t channel, uint16_t apCount, uint16_t staCount);
  void setCameraState(const char *captureState, const char *gatewayState);
  void emergencyStop();
 private:
  void render();
  void renderHeader();
  void renderContent();
  void renderNav();
  void renderCore();
  void renderCodex();
  void renderUart();
  void renderScan();
  void renderCam();
  void renderOverlay();
  void renderCalibration();
  void renderSettings();
  void applyBacklight(bool on);
  void drawBootScreen();
  bool touchToScreen(TouchPoint p, int16_t &x, int16_t &y) const;
  void handleTouch(const TouchPoint &tp);
  void handleRelease(int16_t x, int16_t y);
  bool handleNavHit(int16_t x, int16_t y);
  bool handleCodexHit(int16_t x, int16_t y);
  bool handleCoreHit(int16_t x, int16_t y);
  bool handleScanHit(int16_t x, int16_t y);
  bool handleCamHit(int16_t x, int16_t y);
  bool handleCalibrationHit(int16_t rawX, int16_t rawY);
  bool handleSettingsHit(int16_t x, int16_t y);
  void emitDecision(const char *decision, const char *reason);
  void dispatchApprovedCommand();
  void clearProposal(const char *modeLabel);
  uint16_t riskColor() const;
  const char *riskLabel() const;
  bool proposalExpired() const;
  void invalidateAll();
  void recordResetReason();
  bool codexButtonHit(int16_t x, int16_t y, bool &approve, bool &deny) const;

  static constexpr uint8_t PinLcdBacklight = 21;
  static constexpr uint8_t PinLcdCs = 15;
  static constexpr uint8_t PinTouchCs = 33;
  static constexpr uint8_t PinTouchIrq = 36;
  static constexpr uint8_t PinBoot = 0;
  static constexpr uint8_t PinSpeakerEnable = 4;
  static constexpr uint8_t PinSpeakerDac = 26;
  static constexpr uint8_t PinRed = 22;
  static constexpr uint8_t PinGreen = 16;
  static constexpr uint8_t PinBlue = 17;

  TFT_eSPI _display;
  TouchController _touch;
  UiPreferences _prefs;
  UiSettings _settings{};
  DirtyRegions _dirty{};
  UiMode _mode{UiMode::Booting};
  UiPage _page{UiPage::Core};
  LinkState _s3{LinkState::Connecting};
  LinkState _flipper{LinkState::Connecting};
  LinkState _marauder{LinkState::SafeMode};
  LinkState _gateway{LinkState::Connecting};
  Proposal _proposal{};
  char _lastCommand[161]{};
  char _lastResponse[161]{};
  char _scanState[32]{"IDLE"};
  char _captureState[32]{"IDLE"};
  char _gatewayState[32]{"IDLE"};
  uint8_t _scanChannel{0};
  uint16_t _apCount{0};
  uint16_t _staCount{0};
  uint32_t _lastHeaderDraw{0};
  uint32_t _lastContentDraw{0};
  uint32_t _lastNavDraw{0};
  uint32_t _lastLoopMs{0};
  uint32_t _pageLastChangedMs{0};
  uint32_t _proposalReceivedMs{0};
  uint32_t _displayRecoveryCount{0};
  uint32_t _fullScreenClears{0};
  bool _backlightOn{false};
  bool _bootRendered{false};
  bool _touchGestureActive{false};
  bool _touchGestureEligible{false};
  bool _touchPressed{false};
  bool _touchPresent{false};
  bool _touchStartedInside{false};
  bool _touchGestureConsumed{false};
  uint8_t _touchGestureTarget{0};
  int16_t _touchOriginX{-1};
  int16_t _touchOriginY{-1};
  int16_t _touchLastX{-1};
  int16_t _touchLastY{-1};
  int16_t _touchLastRawX{-1};
  int16_t _touchLastRawY{-1};
  TouchPhase _touchPhase{TouchPhase::Idle};
  uint32_t _touchReleaseMs{0};
  uint32_t _touchDownMs{0};
  uint32_t _bootButtonDownMs{0};
  uint32_t _renderBudgetUs{0};
  uint32_t _minFreeHeap{UINT32_MAX};
  uint32_t _largestFreeBlock{0};
  uint32_t _lastResetReason{0};
  uint32_t _lastActionMs{0};
  uint32_t _lastFpsUpdateMs{0};
  uint16_t _frameCount{0};
  uint16_t _renderFps{0};
  bool _settingsOverlay{false};
  bool _calibrationActive{false};
  uint8_t _calibrationStep{0};
  struct CalSample { int16_t x; int16_t y; bool valid; };
  CalSample _calSamples[5]{};
};
