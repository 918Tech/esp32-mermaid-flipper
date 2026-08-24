#include "UiController.h"
#include "Icons.h"
#include "Theme.h"
#include "widgets/FlowGraph.h"

static const char *pageLabel(UiPage p) {
  switch (p) {
    case UiPage::Core: return "CORE";
    case UiPage::Codex: return "CODEX";
    case UiPage::Uart: return "UART";
    case UiPage::Scan: return "SCAN";
    case UiPage::Cam: return "CAM";
  }
  return "";
}

static const char *modeLabel(UiMode m) {
  switch (m) {
    case UiMode::Booting: return "BOOTING";
    case UiMode::CalibrationRequired: return "CALIBRATION REQUIRED";
    case UiMode::ConnectingS3: return "CONNECTING S3";
    case UiMode::ConnectingFlipper: return "CONNECTING FLIPPER";
    case UiMode::Ready: return "READY";
    case UiMode::Capturing: return "CAPTURING";
    case UiMode::CodexAnalyzing: return "CODEX ANALYZING";
    case UiMode::ProposalPending: return "PROPOSAL PENDING";
    case UiMode::CommandSending: return "COMMAND SENDING";
    case UiMode::CommandComplete: return "COMMAND COMPLETE";
    case UiMode::CommandBlocked: return "COMMAND BLOCKED";
    case UiMode::ProposalDenied: return "PROPOSAL DENIED";
    case UiMode::Degraded: return "DEGRADED";
    case UiMode::Error: return "ERROR";
    case UiMode::EmergencyStop: return "EMERGENCY STOP";
  }
  return "";
}

void UiController::recordResetReason() {
  _lastResetReason = esp_reset_reason();
  _prefs.rememberResetReason(_lastResetReason);
}

void UiController::begin() {
  recordResetReason();
  pinMode(PinLcdCs, OUTPUT);
  pinMode(PinTouchCs, OUTPUT);
  pinMode(PinLcdBacklight, OUTPUT);
  pinMode(PinSpeakerEnable, OUTPUT);
  pinMode(PinRed, OUTPUT);
  pinMode(PinGreen, OUTPUT);
  pinMode(PinBlue, OUTPUT);
  digitalWrite(PinLcdCs, HIGH);
  digitalWrite(PinTouchCs, HIGH);
  digitalWrite(PinLcdBacklight, LOW);
  digitalWrite(PinSpeakerEnable, HIGH);
  digitalWrite(PinRed, HIGH);
  digitalWrite(PinGreen, HIGH);
  digitalWrite(PinBlue, HIGH);
  pinMode(PinBoot, INPUT_PULLUP);
  _prefs.begin();
  _settings = _prefs.loadSettings();
  auto cal = _prefs.loadCalibration();
  _touch.begin();
  _touch.setCalibration(cal);
  _touch.forceRawMode(!cal.valid);
  _display.init();
  _display.setRotation(_settings.rotation);
  _display.fillScreen(Theme::Bg);
  _fullScreenClears++;
  drawBootScreen();
  applyBacklight(true);
  _mode = cal.valid ? UiMode::Ready : UiMode::CalibrationRequired;
  _page = UiPage::Core;
  invalidateAll();
}

void UiController::applyBacklight(bool on) {
  _backlightOn = on;
  digitalWrite(PinLcdBacklight, on ? HIGH : LOW);
}

void UiController::drawBootScreen() {
  _display.startWrite();
  _display.fillScreen(Theme::Bg);
  _display.fillRoundRect(20, 90, 200, 120, 12, Theme::Panel);
  _display.setTextColor(Theme::Text, Theme::Panel);
  _display.drawCentreString("918 TECH//ONE", 120, 116, 2);
  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawCentreString("Initializing display path", 120, 150, 1);
  _display.endWrite();
}

void UiController::invalidateAll() { _dirty.header = _dirty.content = _dirty.navigation = _dirty.overlay = true; }

void UiController::setMode(UiMode mode) { if (_mode != mode) { _mode = mode; _dirty.header = _dirty.content = _dirty.overlay = true; } }
void UiController::setPage(UiPage page) { if (_page != page) { _page = page; _pageLastChangedMs = millis(); _dirty.content = _dirty.navigation = true; } }
void UiController::setProposal(const Proposal &proposal) { _proposal = proposal; _proposalReceivedMs = millis(); setMode(UiMode::ProposalPending); setPage(UiPage::Codex); _dirty.content = _dirty.header = true; }
void UiController::setLinkState(const char *, LinkState state) { if (_s3 != state) { _s3 = state; _dirty.content = _dirty.header = true; } }
void UiController::setLastCommand(const char *text) { strlcpy(_lastCommand, text, sizeof(_lastCommand)); _dirty.content = true; }
void UiController::setLastResponse(const char *text) { strlcpy(_lastResponse, text, sizeof(_lastResponse)); _dirty.content = true; }
void UiController::pushTerminalLine(const char *line) { strlcpy(_lastResponse, line, sizeof(_lastResponse)); _dirty.content = true; }
void UiController::setScanStats(uint8_t channel, uint16_t apCount, uint16_t staCount) { _scanChannel = channel; _apCount = apCount; _staCount = staCount; _dirty.content = true; }
void UiController::setCameraState(const char *captureState, const char *gatewayState) { strlcpy(_captureState, captureState, sizeof(_captureState)); strlcpy(_gatewayState, gatewayState, sizeof(_gatewayState)); _dirty.content = true; }
void UiController::emergencyStop() { setMode(UiMode::EmergencyStop); _proposal.valid = false; strlcpy(_scanState, "STOPPED", sizeof(_scanState)); _dirty.content = true; }

void UiController::emitDecision(const char *decision, const char *reason) {
  char line[256];
  snprintf(line, sizeof(line),
           "{\"v\":1,\"t\":\"decision\",\"seq\":%lu,\"decision\":\"%s\",\"proposal_id\":\"%s\",\"reason\":\"%s\"}",
           static_cast<unsigned long>(millis()),
           decision,
           _proposal.id[0] ? _proposal.id : "",
           reason);
  Serial.println(line);
}

void UiController::dispatchApprovedCommand() {
  Serial.print("APPROVED: ");
  Serial.println(_proposal.command);
  setLastCommand(_proposal.command);
  setLastResponse("APPROVED // SENT TO LOCAL SYSTEM");
  _flipper = LinkState::Online;
}

void UiController::clearProposal(const char *modeLabel) {
  _proposal.valid = false;
  _proposal.handled = true;
  setMode(modeLabel && modeLabel[0] == 'D' ? UiMode::ProposalDenied : UiMode::CommandComplete);
  _dirty.content = true;
}

bool UiController::proposalExpired() const { return _proposal.valid && _proposal.expiresAtMs != 0 && millis() > _proposal.expiresAtMs; }

uint16_t UiController::riskColor() const {
  switch (_proposal.risk) {
    case ProposalRisk::ReadOnly: return Theme::Green;
    case ProposalRisk::StateChange: return Theme::Amber;
    case ProposalRisk::RadioTransmit: return Theme::Red;
    case ProposalRisk::FirmwareChange: return Theme::Red;
  }
  return Theme::Amber;
}
const char *UiController::riskLabel() const {
  switch (_proposal.risk) {
    case ProposalRisk::ReadOnly: return "read_only";
    case ProposalRisk::StateChange: return "state_change";
    case ProposalRisk::RadioTransmit: return "radio_transmit";
    case ProposalRisk::FirmwareChange: return "firmware_change";
  }
  return "";
}

void UiController::loop() {
  if ((millis() - _lastLoopMs) < 50) return;
  _lastLoopMs = millis();
  uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap < _minFreeHeap) _minFreeHeap = freeHeap;
  uint32_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
  if (largest > _largestFreeBlock) _largestFreeBlock = largest;
  if (_lastFpsUpdateMs == 0) _lastFpsUpdateMs = millis();
  _frameCount++;
  if (millis() - _lastFpsUpdateMs >= 1000) {
    _renderFps = _frameCount;
    _frameCount = 0;
    _lastFpsUpdateMs = millis();
    _dirty.header = true;
  }
  if (proposalExpired()) { setMode(UiMode::ProposalDenied); _proposal.valid = false; _dirty.content = true; }
  if (digitalRead(PinBoot) == LOW) {
    if (_bootButtonDownMs == 0) _bootButtonDownMs = millis();
    if (millis() - _bootButtonDownMs > 1200) emergencyStop();
  } else {
    _bootButtonDownMs = 0;
  }
  TouchPoint tp = _touch.sample();
  if (tp.valid) {
    handleTouch(tp);
  } else if (_touchPressed) {
    _touchPressed = false;
    _touchPhase = TouchPhase::Released;
    _touchReleaseMs = millis();
    handleRelease(_touchLastX, _touchLastY);
    if (millis() - _touchReleaseMs > 180) {
      _touchGestureConsumed = false;
      _touchStartedInside = false;
      _touchPhase = TouchPhase::Idle;
    }
  }
  render();
}

bool UiController::touchToScreen(TouchPoint p, int16_t &x, int16_t &y) const {
  if (!p.valid) return false;
  x = constrain(p.x, 0, 239);
  y = constrain(p.y, 0, 319);
  return true;
}

bool UiController::handleNavHit(int16_t x, int16_t y) {
  if (y < 272) return false;
  const uint8_t idx = constrain(x / 48, 0, 4);
  const UiPage next = static_cast<UiPage>(idx);
  setPage(next);
  return true;
}

bool UiController::handleCoreHit(int16_t x, int16_t y) {
  if (_page != UiPage::Core) return false;
  if (x >= 16 && x < 224 && y >= 106 && y < 190) {
    _settingsOverlay = true;
    _dirty.overlay = true;
    return true;
  }
  return false;
}

bool UiController::handleScanHit(int16_t x, int16_t y) {
  if (_page != UiPage::Scan) return false;
  if (x >= 16 && x < 224 && y >= 160 && y < 190) {
    strlcpy(_scanState, "SCANNING AP", sizeof(_scanState));
    setScanStats(1, 12, 2);
    return true;
  }
  if (x >= 16 && x < 224 && y >= 198 && y < 228) {
    strlcpy(_scanState, "STOPPED", sizeof(_scanState));
    setScanStats(0, 0, 0);
    return true;
  }
  return false;
}

bool UiController::handleCamHit(int16_t x, int16_t y) {
  if (_page != UiPage::Cam) return false;
  if (x >= 16 && x < 224 && y >= 198 && y < 228) {
    setMode(UiMode::Capturing);
    setCameraState("CAPTURING", "CODEX ANALYZING");
    return true;
  }
  return false;
}

bool UiController::handleCalibrationHit(int16_t x, int16_t y) {
  if (_mode != UiMode::CalibrationRequired) return false;
  const int16_t points[4][2] = {{24, 60}, {216, 60}, {216, 228}, {24, 228}};
  const int16_t px = points[_calibrationStep][0];
  const int16_t py = points[_calibrationStep][1];
  if (abs(x - px) > 24 || abs(y - py) > 24) return false;
  _calSamples[_calibrationStep] = {x, y, true};
  _calibrationStep++;
  if (_calibrationStep >= 4) {
    TouchCalibration cal{};
    cal.minX = min(min(_calSamples[0].x, _calSamples[3].x), min(_calSamples[1].x, _calSamples[2].x));
    cal.maxX = max(max(_calSamples[0].x, _calSamples[3].x), max(_calSamples[1].x, _calSamples[2].x));
    cal.minY = min(min(_calSamples[0].y, _calSamples[1].y), min(_calSamples[2].y, _calSamples[3].y));
    cal.maxY = max(max(_calSamples[0].y, _calSamples[1].y), max(_calSamples[2].y, _calSamples[3].y));
    cal.swapXY = false;
    cal.invX = false;
    cal.invY = false;
    cal.valid = true;
    _prefs.saveCalibration(cal);
    _touch.setCalibration(cal);
    _touch.forceRawMode(false);
    setMode(UiMode::Ready);
    _dirty.content = _dirty.header = _dirty.overlay = true;
  } else {
    _dirty.content = true;
  }
  return true;
}

bool UiController::handleSettingsHit(int16_t x, int16_t y) {
  if (!_settingsOverlay) return false;
  if (x >= 16 && x < 224 && y >= 150 && y < 178) {
    _settingsOverlay = false;
    _dirty.content = _dirty.overlay = true;
    return true;
  }
  return false;
}

bool UiController::handleCodexHit(int16_t x, int16_t y) {
  if (_page != UiPage::Codex || !_proposal.valid) return false;
  bool approve = false;
  bool deny = false;
  if (!codexButtonHit(x, y, approve, deny)) return false;
  if (approve) {
    if (proposalExpired() || _proposal.handled) {
      setMode(UiMode::CommandBlocked);
      setLastResponse("BLOCKED BY PROPOSAL STATE");
      emitDecision("blocked", "stale_or_handled");
      _dirty.content = true;
      return true;
    }
    if (!_proposal.command[0]) {
      setMode(UiMode::CommandBlocked);
      setLastResponse("BLOCKED BY EMPTY COMMAND");
      emitDecision("blocked", "empty_command");
      _dirty.content = true;
      return true;
    }
    _proposal.handled = true;
    setMode(UiMode::CommandSending);
    _lastActionMs = millis();
    emitDecision("approve", "local_approval");
    dispatchApprovedCommand();
    setMode(UiMode::CommandComplete);
    _dirty.content = true;
    return true;
  }
  if (deny) {
    clearProposal("deny");
    emitDecision("deny", "local_denial");
    setLastResponse("DENIED // PROPOSAL CLEARED");
    setMode(UiMode::ProposalDenied);
    _lastActionMs = millis();
    _dirty.content = true;
    return true;
  }
  return false;
}

bool UiController::codexButtonHit(int16_t x, int16_t y, bool &approve, bool &deny) const {
  approve = (x >= 16 && x < 108 && y >= 198 && y < 246);
  deny = (x >= 132 && x < 224 && y >= 198 && y < 246);
  return approve || deny;
}

void UiController::handleRelease(int16_t x, int16_t y) {
  if (_touchGestureConsumed || !(_touchStartedInside)) return;
  if (handleSettingsHit(x, y)) { _touchGestureConsumed = true; return; }
  if (handleCalibrationHit(x, y)) { _touchGestureConsumed = true; return; }
  if (handleCoreHit(x, y)) { _touchGestureConsumed = true; return; }
  if (handleNavHit(x, y)) { _touchGestureConsumed = true; return; }
  if (handleCodexHit(x, y)) { _touchGestureConsumed = true; return; }
  if (handleScanHit(x, y)) { _touchGestureConsumed = true; return; }
  if (handleCamHit(x, y)) { _touchGestureConsumed = true; return; }
}

void UiController::handleTouch(const TouchPoint &tp) {
  int16_t x = 0, y = 0;
  if (!touchToScreen(tp, x, y)) return;
  _touchPressed = true;
  _touchLastX = x;
  _touchLastY = y;
  if (_touchPhase == TouchPhase::Idle || _touchPhase == TouchPhase::Released) {
    _touchPhase = TouchPhase::PressCandidate;
    _touchOriginX = x;
    _touchOriginY = y;
    _touchDownMs = millis();
    bool approve = false, deny = false;
    _touchStartedInside = (y >= 272) || (_mode == UiMode::CalibrationRequired) || handleSettingsHit(x, y) || handleCoreHit(x, y) || handleScanHit(x, y) || handleCamHit(x, y) || (codexButtonHit(x, y, approve, deny));
    if (approve) _touchGestureTarget = 1;
    else if (deny) _touchGestureTarget = 2;
    else if (y >= 272) _touchGestureTarget = 3;
    else _touchGestureTarget = 0;
    _touchGestureConsumed = false;
  } else {
    _touchPhase = TouchPhase::Pressed;
  }
}

void UiController::renderHeader() {
  _display.startWrite();
  _display.fillRect(0, 0, 240, 40, Theme::Elevated);
  _display.fillRect(8, 6, 18, 10, Theme::Violet);
  _display.fillRect(26, 6, 18, 10, Theme::Cyan);
  _display.fillRect(8, 16, 36, 12, Theme::Amber);
  _display.setTextColor(Theme::Text, Theme::Elevated);
  _display.drawString("918", 10, 6, 2);
  _display.drawString("TECH//ONE", 56, 12, 2);
  _display.drawRightString(modeLabel(_mode), 232, 10, 2);
  _display.drawRightString("BAT 100%", 232, 22, 2);
  _display.endWrite();
  _dirty.header = false;
}

void UiController::renderNav() {
  _display.startWrite();
  _display.fillRect(0, 272, 240, 48, Theme::Panel);
  const char *labels[] = {"CORE", "CODEX", "UART", "SCAN", "CAM"};
  for (int i = 0; i < 5; ++i) {
    int x = i * 48;
    uint16_t c = (static_cast<int>(_page) == i) ? Theme::Cyan : Theme::Secondary;
    _display.fillRect(x + 1, 273, 46, 46, Theme::Panel);
    drawTabIcon(_display, i, x + 14, 278, c);
    _display.drawCentreString(labels[i], x + 24, 300, 1);
  }
  _display.endWrite();
  _dirty.navigation = false;
}

void UiController::renderCore() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  _display.drawString("UNIFIED COMMAND SURFACE", 16, 48, 1);
  _display.drawString("Systems alive.", 16, 66, 1);
  drawFlowGraph(_display, 16, 106, 208, 84, Theme::Cyan, Theme::Violet, Theme::Aqua);
  const char *labels[] = {"S3 Vision", "Flipper UART", "Marauder Safe Mode", "Codex Gateway"};
  const char *values[] = {(_s3 == LinkState::Online || _s3 == LinkState::Ready) ? "ONLINE" : "CONNECTING",
                          (_flipper == LinkState::Online || _flipper == LinkState::Ready) ? "ONLINE" : "CONNECTING",
                          "SAFE MODE",
                          (_gateway == LinkState::Online || _gateway == LinkState::Ready) ? "ONLINE" : "DEGRADED"};
  uint16_t colors[] = {Theme::Cyan, Theme::Aqua, Theme::Amber, Theme::Violet};
  for (int i = 0; i < 4; ++i) {
    int y = 196 + i * 18;
    _display.fillRoundRect(16, y, 208, 16, 4, Theme::Panel);
    _display.drawString(labels[i], 24, y + 4, 1);
    _display.drawRightString(values[i], 216, y + 4, 1);
    _display.fillCircle(210, y + 8, 3, colors[i]);
  }
  _display.endWrite();
}

void UiController::renderCodex() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  if (_mode == UiMode::CalibrationRequired) {
    renderCalibration();
    return;
  }
  _display.drawString("PHYSICAL APPROVAL REQUIRED", 16, 48, 1);
  _display.fillRoundRect(12, 76, 216, 114, 8, Theme::Panel);
  _display.drawString("Summary", 20, 84, 1);
  _display.drawString(_proposal.summary, 20, 98, 1);
  _display.drawString("Target", 20, 126, 1);
  _display.drawString(_proposal.target, 76, 124, 1);
  _display.drawString("Risk", 20, 146, 1);
  _display.drawString(riskLabel(), 76, 140, 1);
  _display.fillRect(20, 168, 200, 20, Theme::Bg);
  _display.drawString(_proposal.command, 20, 160, 1);
  _display.fillRoundRect(16, 198, 92, 48, 8, Theme::Green);
  _display.fillRoundRect(132, 198, 92, 48, 8, Theme::Red);
  _display.setTextColor(Theme::Text, Theme::Green);
  _display.drawCentreString("APPROVE", 62, 216, 1);
  _display.setTextColor(Theme::Text, Theme::Red);
  _display.drawCentreString("DENY", 178, 216, 1);
  if (_proposal.valid) {
    uint32_t remaining = (_proposal.expiresAtMs > millis()) ? ((_proposal.expiresAtMs - millis()) / 1000) : 0;
    _display.setTextColor(Theme::Amber, Theme::Bg);
    _display.drawString("Countdown", 20, 174, 1);
    char buf[16]; snprintf(buf, sizeof(buf), "%lus", static_cast<unsigned long>(remaining));
    _display.drawRightString(buf, 214, 176, 1);
  }
  _display.endWrite();
}

void UiController::renderUart() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  _display.drawString("UART // 115200 8N1", 16, 48, 1);
  _display.fillRoundRect(12, 72, 216, 176, 8, Theme::Panel);
  _display.drawString("Last transmitted", 20, 82, 1);
  _display.drawString(_lastCommand, 20, 94, 1);
  _display.drawString("Last response", 20, 122, 1);
  _display.drawString(_lastResponse, 20, 130, 1);
  _display.drawString("RX/TX bytes: bounded", 20, 162, 1);
  _display.drawString("Clear terminal", 20, 188, 1);
  _display.endWrite();
}

void UiController::renderScan() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  _display.drawString("AUTHORIZED NETWORKS ONLY", 16, 48, 1);
  _display.drawString("Marauder // Safe", 16, 66, 1);
  _display.drawString("SAFE MODE", 16, 102, 1);
  _display.drawString("RF TRANSMIT CONTROLS LOCKED", 16, 118, 1);
  _display.fillRoundRect(16, 160, 208, 30, 6, Theme::Amber);
  _display.drawCentreString("START PASSIVE AP SCAN", 120, 167, 1);
  _display.fillRoundRect(16, 198, 208, 30, 6, Theme::Amber);
  _display.drawCentreString("STOP SCAN", 120, 205, 1);
  char buf[32]; snprintf(buf, sizeof(buf), "CH %u  AP %u  STA %u", _scanChannel, _apCount, _staCount);
  _display.drawString(buf, 16, 236, 1);
  _display.endWrite();
}

void UiController::renderCam() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  _display.drawString("OV3660 // SECURE GATEWAY", 16, 48, 1);
  _display.drawString(_captureState, 16, 70, 1);
  _display.drawString(_gatewayState, 16, 86, 1);
  _display.drawRoundRect(60, 100, 120, 90, 8, Theme::Border);
  _display.drawLine(120, 108, 120, 182, Theme::Cyan);
  _display.drawLine(66, 145, 174, 145, Theme::Cyan);
  _display.fillRoundRect(16, 198, 208, 30, 6, Theme::Violet);
  _display.drawCentreString("CAPTURE + ANALYZE", 120, 205, 1);
  _display.endWrite();
}

void UiController::renderContent() {
  switch (_page) {
    case UiPage::Core: renderCore(); break;
    case UiPage::Codex: renderCodex(); break;
    case UiPage::Uart: renderUart(); break;
    case UiPage::Scan: renderScan(); break;
    case UiPage::Cam: renderCam(); break;
  }
  _dirty.content = false;
}

void UiController::renderOverlay() {
  if (_settingsOverlay) { renderSettings(); return; }
  if (_mode != UiMode::EmergencyStop) return;
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Red);
  _display.setTextColor(Theme::Text, Theme::Red);
  _display.drawCentreString("EMERGENCY STOP", 120, 126, 2);
  _display.drawCentreString("Touch to acknowledge", 120, 160, 1);
  _display.endWrite();
  _dirty.overlay = false;
}

void UiController::renderCalibration() {
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  _display.drawString("TOUCH CALIBRATION", 16, 48, 1);
  _display.drawString("Tap each target", 16, 64, 1);
  const int16_t points[4][2] = {{24, 60}, {216, 60}, {216, 228}, {24, 228}};
  for (uint8_t i = 0; i < 4; ++i) {
    uint16_t c = (i == _calibrationStep) ? Theme::Amber : Theme::Secondary;
    _display.drawCircle(points[i][0], points[i][1], 10, c);
    _display.drawLine(points[i][0] - 8, points[i][1], points[i][0] + 8, points[i][1], c);
    _display.drawLine(points[i][0], points[i][1] - 8, points[i][0], points[i][1] + 8, c);
  }
  char buf[24];
  snprintf(buf, sizeof(buf), "Step %u/4", static_cast<unsigned>(_calibrationStep + 1));
  _display.drawString(buf, 16, 252, 1);
}

void UiController::renderSettings() {
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  _display.drawString("SETTINGS", 16, 48, 1);
  char buf[64];
  snprintf(buf, sizeof(buf), "FPS %u", _renderFps);
  _display.drawString(buf, 16, 68, 1);
  snprintf(buf, sizeof(buf), "HEAP %lu", static_cast<unsigned long>(_minFreeHeap));
  _display.drawString(buf, 16, 84, 1);
  snprintf(buf, sizeof(buf), "LARGEST %lu", static_cast<unsigned long>(_largestFreeBlock));
  _display.drawString(buf, 16, 100, 1);
  snprintf(buf, sizeof(buf), "RESET %lu", static_cast<unsigned long>(_lastResetReason));
  _display.drawString(buf, 16, 116, 1);
  _display.fillRoundRect(16, 150, 208, 28, 6, Theme::Amber);
  _display.drawCentreString("CLOSE SETTINGS", 120, 156, 1);
  _display.drawString("Tap to return", 16, 186, 1);
}

void UiController::render() {
  if (_dirty.header) renderHeader();
  if (_dirty.content) renderContent();
  if (_dirty.navigation) renderNav();
  if (_dirty.overlay) renderOverlay();
}
