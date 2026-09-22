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


static void drawWaveBand(TFT_eSPI &t, int y, uint16_t a, uint16_t b) {
  for (int row = 0; row < 4; ++row) {
    int yy = y + row * 6;
    uint16_t c = (row & 1) ? b : a;
    t.drawLine(0, yy + 2, 40, yy - 2, c);
    t.drawLine(40, yy - 2, 80, yy + 3, c);
    t.drawLine(80, yy + 3, 120, yy - 1, c);
    t.drawLine(120, yy - 1, 160, yy + 2, c);
    t.drawLine(160, yy + 2, 200, yy - 3, c);
    t.drawLine(200, yy - 3, 239, yy + 1, c);
  }
}

static void drawScaleField(TFT_eSPI &t, int x, int y, int w, int h, uint16_t c) {
  for (int yy = y; yy < y + h; yy += 12) {
    int phase = ((yy - y) / 12) & 1 ? 6 : 0;
    for (int xx = x - phase; xx < x + w; xx += 12) {
      t.drawCircle(xx, yy, 6, c);
    }
  }
}

static void drawStatusPill(TFT_eSPI &t, int x, int y, int w, const char *label, uint16_t c) {
  t.fillRoundRect(x, y, w, 18, 9, Theme::Panel);
  t.drawRoundRect(x, y, w, 18, 9, c);
  t.fillCircle(x + 10, y + 9, 3, c);
  t.setTextColor(c, Theme::Panel);
  t.drawString(label, x + 18, y + 5, 1);
}

static void drawSectionTitle(TFT_eSPI &t, const char *eyebrow, const char *title) {
  t.setTextColor(Theme::Aqua, Theme::Bg);
  t.drawString(eyebrow, 14, 48, 1);
  t.setTextColor(Theme::Text, Theme::Bg);
  t.drawString(title, 14, 62, 2);
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
  _page = cal.valid ? UiPage::Core : UiPage::Codex;
  invalidateAll();
}

void UiController::applyBacklight(bool on) {
  _backlightOn = on;
  digitalWrite(PinLcdBacklight, on ? HIGH : LOW);
}

void UiController::drawBootScreen() {
  _display.startWrite();
  _display.fillScreen(Theme::Bg);

  drawScaleField(_display, 0, 18, 240, 86, Theme::DeepSea);
  drawWaveBand(_display, 220, Theme::Aqua, Theme::Cyan);

  _display.fillRoundRect(20, 54, 200, 154, 18, Theme::Panel);
  _display.drawRoundRect(20, 54, 200, 154, 18, Theme::Cyan);

  _display.fillRoundRect(34, 70, 54, 42, 8, Theme::Violet);
  _display.setTextColor(Theme::Text, Theme::Violet);
  _display.drawCentreString("918", 61, 79, 4);

  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawString("TECHNOLOGIES", 100, 72, 1);
  _display.setTextColor(Theme::Aqua, Theme::Panel);
  _display.drawString("MERMAID", 100, 87, 2);

  _display.drawCircle(120, 142, 28, Theme::Cyan);
  _display.drawCircle(120, 142, 20, Theme::Aqua);
  _display.drawCircle(120, 142, 4, Theme::Pearl);
  _display.drawLine(92, 142, 148, 142, Theme::Border);
  _display.drawLine(120, 114, 120, 170, Theme::Border);

  _display.setTextColor(Theme::Text, Theme::Panel);
  _display.drawCentreString("M3RMA1D S1R3N", 120, 177, 1);
  _display.setTextColor(Theme::Aqua, Theme::Panel);
  _display.drawCentreString("UI R2", 120, 190, 1);
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

bool UiController::handleCalibrationHit(int16_t rawX, int16_t rawY) {
  if (_mode != UiMode::CalibrationRequired) return false;
  if (rawX <= 0 || rawY <= 0 || _calibrationStep >= 4) return false;

  _calSamples[_calibrationStep] = {rawX, rawY, true};
  _calibrationStep++;

  if (_calibrationStep >= 4) {
    // Targets are TL, TR, BR, BL. Infer whether raw axes are swapped.
    const int32_t horizontalRawX =
        abs(_calSamples[1].x - _calSamples[0].x) +
        abs(_calSamples[2].x - _calSamples[3].x);
    const int32_t horizontalRawY =
        abs(_calSamples[1].y - _calSamples[0].y) +
        abs(_calSamples[2].y - _calSamples[3].y);

    TouchCalibration cal{};
    cal.swapXY = horizontalRawY > horizontalRawX;

    int16_t sx[4];
    int16_t sy[4];
    for (int i = 0; i < 4; ++i) {
      sx[i] = cal.swapXY ? _calSamples[i].y : _calSamples[i].x;
      sy[i] = cal.swapXY ? _calSamples[i].x : _calSamples[i].y;
    }

    const int16_t left = (sx[0] + sx[3]) / 2;
    const int16_t right = (sx[1] + sx[2]) / 2;
    const int16_t top = (sy[0] + sy[1]) / 2;
    const int16_t bottom = (sy[2] + sy[3]) / 2;

    cal.invX = left > right;
    cal.invY = top > bottom;

    cal.minX = min(min(sx[0], sx[1]), min(sx[2], sx[3]));
    cal.maxX = max(max(sx[0], sx[1]), max(sx[2], sx[3]));
    cal.minY = min(min(sy[0], sy[1]), min(sy[2], sy[3]));
    cal.maxY = max(max(sy[0], sy[1]), max(sy[2], sy[3]));
    cal.valid = (cal.maxX - cal.minX > 500) && (cal.maxY - cal.minY > 500);

    if (!cal.valid) {
      _calibrationStep = 0;
      _dirty.content = true;
      Serial.println("MERMAID_TOUCH_CAL_RETRY");
      return true;
    }

    _prefs.saveCalibration(cal);
    _touch.setCalibration(cal);
    _touch.forceRawMode(false);
    setMode(UiMode::Ready);
    setPage(UiPage::Core);
    _dirty.content = _dirty.header = _dirty.navigation = _dirty.overlay = true;
    Serial.println("MERMAID_TOUCH_CAL_OK");
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
  if (_mode == UiMode::CalibrationRequired) {
    if (handleCalibrationHit(_touchLastRawX, _touchLastRawY)) { _touchGestureConsumed = true; return; }
  }
  if (handleSettingsHit(x, y)) { _touchGestureConsumed = true; return; }
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
  _touchLastRawX = tp.rawX;
  _touchLastRawY = tp.rawY;
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

  _display.fillRoundRect(4, 4, 34, 30, 7, Theme::Violet);
  _display.setTextColor(Theme::Text, Theme::Violet);
  _display.drawCentreString("918", 21, 11, 2);

  _display.setTextColor(Theme::Text, Theme::Elevated);
  _display.drawString("TECH", 45, 5, 1);
  _display.setTextColor(Theme::Aqua, Theme::Elevated);
  _display.drawString("MERMAID", 45, 18, 1);

  _display.fillRoundRect(103, 6, 43, 14, 7, Theme::Panel);
  _display.setTextColor(Theme::Cyan, Theme::Panel);
  _display.drawCentreString("UI R2", 124, 10, 1);

  _display.setTextColor(Theme::Pearl, Theme::Elevated);
  _display.drawRightString(pageLabel(_page), 234, 5, 1);
  _display.setTextColor(Theme::Secondary, Theme::Elevated);
  _display.drawRightString("BAT 100%", 234, 19, 1);

  _display.drawFastHLine(0, 38, 240, Theme::Cyan);
  _display.endWrite();
  _dirty.header = false;
}

void UiController::renderNav() {
  _display.startWrite();
  _display.fillRect(0, 272, 240, 48, Theme::Panel);
  _display.drawFastHLine(0, 272, 240, Theme::Border);

  const char *labels[] = {"CORE", "CODEX", "UART", "SCAN", "CAM"};
  for (int i = 0; i < 5; ++i) {
    int x = i * 48;
    const bool active = (static_cast<int>(_page) == i);
    uint16_t fg = active ? Theme::Pearl : Theme::Secondary;
    uint16_t bg = active ? Theme::Tide : Theme::Panel;

    if (active) {
      _display.fillRoundRect(x + 3, 275, 42, 40, 10, bg);
      _display.drawRoundRect(x + 3, 275, 42, 40, 10, Theme::Cyan);
      _display.fillRoundRect(x + 14, 276, 20, 3, 2, Theme::Aqua);
    }

    drawTabIcon(_display, i, x + 14, 280, active ? Theme::Cyan : fg);
    _display.setTextColor(fg, bg);
    _display.drawCentreString(labels[i], x + 24, 301, 1);
  }
  _display.endWrite();
  _dirty.navigation = false;
}

void UiController::renderCore() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  drawScaleField(_display, 172, 46, 68, 62, Theme::DeepSea);
  drawSectionTitle(_display, "918 TECHNOLOGIES", "MERMAID CORE");

  _display.fillRoundRect(14, 92, 212, 96, 14, Theme::Panel);
  _display.drawRoundRect(14, 92, 212, 96, 14, Theme::Border);

  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawString("SYSTEM MATRIX", 26, 102, 1);

  _display.drawCircle(120, 141, 31, Theme::Border);
  _display.drawCircle(120, 141, 22, Theme::Cyan);
  _display.drawCircle(120, 141, 13, Theme::Aqua);
  _display.fillCircle(120, 141, 5, Theme::Pearl);
  _display.drawLine(89, 141, 151, 141, Theme::Border);
  _display.drawLine(120, 110, 120, 172, Theme::Border);

  _display.setTextColor(Theme::Cyan, Theme::Panel);
  _display.drawString("S3", 30, 126, 1);
  _display.setTextColor(Theme::Aqua, Theme::Panel);
  _display.drawRightString("CODEX", 210, 126, 1);
  _display.setTextColor(Theme::Pearl, Theme::Panel);
  _display.drawString("FLIPPER", 30, 156, 1);
  _display.drawRightString("SAFE", 210, 156, 1);

  drawStatusPill(_display, 14, 198, 100, "S3 VISION", (_s3 == LinkState::Online || _s3 == LinkState::Ready) ? Theme::Green : Theme::Cyan);
  drawStatusPill(_display, 126, 198, 100, "FLIPPER", (_flipper == LinkState::Online || _flipper == LinkState::Ready) ? Theme::Green : Theme::Aqua);
  drawStatusPill(_display, 14, 224, 100, "SAFE MODE", Theme::Amber);
  drawStatusPill(_display, 126, 224, 100, "CODEX", (_gateway == LinkState::Online || _gateway == LinkState::Ready) ? Theme::Green : Theme::Violet);

  _display.setTextColor(Theme::Secondary, Theme::Bg);
  _display.drawCentreString("tap matrix for diagnostics", 120, 252, 1);
  _display.endWrite();
}

void UiController::renderCodex() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);

  if (_mode == UiMode::CalibrationRequired) {
    renderCalibration();
    _display.endWrite();
    return;
  }

  drawScaleField(_display, 178, 46, 62, 56, Theme::DeepSea);
  drawSectionTitle(_display, "918 MERMAID // CODEX", "PHYSICAL GATE");

  _display.fillRoundRect(12, 82, 216, 104, 14, Theme::Panel);
  _display.drawRoundRect(12, 82, 216, 104, 14, Theme::Border);
  _display.fillRoundRect(12, 82, 5, 104, 3, riskColor());

  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawString("PROPOSAL", 26, 92, 1);
  _display.setTextColor(Theme::Text, Theme::Panel);
  _display.drawString(_proposal.summary[0] ? _proposal.summary : "Local approval request", 26, 108, 1);

  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawString("TARGET", 26, 130, 1);
  _display.setTextColor(Theme::Pearl, Theme::Panel);
  _display.drawString(_proposal.target[0] ? _proposal.target : "local_device", 72, 130, 1);

  _display.fillRoundRect(26, 151, 92, 20, 10, Theme::Bg);
  _display.drawRoundRect(26, 151, 92, 20, 10, riskColor());
  _display.setTextColor(riskColor(), Theme::Bg);
  _display.drawCentreString(riskLabel(), 72, 157, 1);

  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawRightString("LOCAL ONLY", 212, 157, 1);

  _display.fillRoundRect(16, 198, 92, 48, 12, Theme::Green);
  _display.drawRoundRect(16, 198, 92, 48, 12, Theme::Pearl);
  _display.setTextColor(Theme::Text, Theme::Green);
  _display.drawCentreString("AUTHORIZE", 62, 213, 1);
  _display.drawCentreString("+", 62, 226, 1);

  _display.fillRoundRect(132, 198, 92, 48, 12, Theme::Red);
  _display.drawRoundRect(132, 198, 92, 48, 12, Theme::Pearl);
  _display.setTextColor(Theme::Text, Theme::Red);
  _display.drawCentreString("DENY", 178, 213, 1);
  _display.drawCentreString("X", 178, 226, 1);

  _display.setTextColor(Theme::Secondary, Theme::Bg);
  _display.drawCentreString("physical approval boundary", 120, 254, 1);
  _display.endWrite();
}

void UiController::renderUart() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  drawSectionTitle(_display, "MERMAID LINK", "UART CONSOLE");

  _display.fillRoundRect(150, 48, 76, 18, 9, Theme::Panel);
  _display.setTextColor(Theme::Cyan, Theme::Panel);
  _display.drawCentreString("115200 8N1", 188, 53, 1);

  _display.fillRoundRect(12, 84, 216, 160, 12, 0x0000);
  _display.drawRoundRect(12, 84, 216, 160, 12, Theme::Border);
  for (int y = 96; y < 236; y += 12) _display.drawFastHLine(20, y, 200, Theme::DeepSea);

  _display.setTextColor(Theme::Aqua, 0x0000);
  _display.drawString("> TX", 22, 94, 1);
  _display.setTextColor(Theme::Text, 0x0000);
  _display.drawString(_lastCommand[0] ? _lastCommand : "idle", 22, 110, 1);

  _display.setTextColor(Theme::Cyan, 0x0000);
  _display.drawString("< RX", 22, 138, 1);
  _display.setTextColor(Theme::Pearl, 0x0000);
  _display.drawString(_lastResponse[0] ? _lastResponse : "awaiting frame", 22, 154, 1);

  _display.setTextColor(Theme::Secondary, 0x0000);
  _display.drawString("MERMAIDLINK // BOUNDED IO", 22, 214, 1);
  _display.endWrite();
}

void UiController::renderScan() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  drawSectionTitle(_display, "AUTHORIZED PASSIVE MODE", "MERMAID SONAR");

  _display.drawCircle(120, 118, 42, Theme::Border);
  _display.drawCircle(120, 118, 29, Theme::DeepSea);
  _display.drawCircle(120, 118, 16, Theme::Tide);
  _display.drawFastHLine(78, 118, 84, Theme::Cyan);
  _display.drawFastVLine(120, 76, 84, Theme::Cyan);
  _display.drawLine(120, 118, 151, 92, Theme::Aqua);
  _display.fillCircle(149, 94, 3, Theme::Pearl);
  _display.fillCircle(98, 131, 2, Theme::Aqua);
  _display.fillCircle(136, 142, 2, Theme::Cyan);

  _display.fillRoundRect(16, 160, 208, 30, 8, Theme::Aqua);
  _display.setTextColor(Theme::Bg, Theme::Aqua);
  _display.drawCentreString("START PASSIVE SCAN", 120, 168, 1);

  _display.fillRoundRect(16, 198, 208, 30, 8, Theme::Panel);
  _display.drawRoundRect(16, 198, 208, 30, 8, Theme::Amber);
  _display.setTextColor(Theme::Amber, Theme::Panel);
  _display.drawCentreString("STOP SCAN", 120, 206, 1);

  char buf[36];
  snprintf(buf, sizeof(buf), "CH %u  AP %u  STA %u", _scanChannel, _apCount, _staCount);
  _display.setTextColor(Theme::Secondary, Theme::Bg);
  _display.drawCentreString(buf, 120, 242, 1);
  _display.drawCentreString("RF transmit locked", 120, 256, 1);
  _display.endWrite();
}

void UiController::renderCam() {
  _display.startWrite();
  _display.fillRect(0, 40, 240, 232, Theme::Bg);
  drawSectionTitle(_display, "S3 VISION GATEWAY", "SIREN CAM");

  _display.fillRoundRect(24, 86, 192, 102, 14, 0x0000);
  _display.drawRoundRect(24, 86, 192, 102, 14, Theme::Border);

  _display.drawRect(42, 99, 18, 18, Theme::Cyan);
  _display.drawRect(180, 99, 18, 18, Theme::Cyan);
  _display.drawRect(42, 157, 18, 18, Theme::Cyan);
  _display.drawRect(180, 157, 18, 18, Theme::Cyan);
  _display.drawCircle(120, 137, 26, Theme::Aqua);
  _display.drawCircle(120, 137, 5, Theme::Pearl);
  _display.drawFastHLine(86, 137, 68, Theme::Cyan);
  _display.drawFastVLine(120, 103, 68, Theme::Cyan);

  _display.setTextColor(Theme::Secondary, 0x0000);
  _display.drawString(_captureState, 34, 94, 1);
  _display.drawRightString(_gatewayState, 206, 94, 1);

  _display.fillRoundRect(16, 198, 208, 30, 8, Theme::Violet);
  _display.drawRoundRect(16, 198, 208, 30, 8, Theme::Cyan);
  _display.setTextColor(Theme::Text, Theme::Violet);
  _display.drawCentreString("CAPTURE + ANALYZE", 120, 206, 1);

  _display.setTextColor(Theme::Secondary, Theme::Bg);
  _display.drawCentreString("OV3660 // LOCAL APPROVAL", 120, 244, 1);
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
  _display.setTextColor(Theme::Aqua, Theme::Bg);
  _display.drawString("918 MERMAID // TOUCH", 16, 48, 1);
  _display.setTextColor(Theme::Text, Theme::Bg);
  _display.drawString("TOUCH CALIBRATION", 16, 64, 1);
  _display.setTextColor(Theme::Secondary, Theme::Bg);
  _display.drawString("Tap each target", 16, 80, 1);
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
  drawSectionTitle(_display, "918 TECHNOLOGIES", "MERMAID SYSTEM");

  const int xs[2] = {14, 124};
  const int ys[2] = {88, 122};
  for (int row = 0; row < 2; ++row) {
    for (int col = 0; col < 2; ++col) {
      _display.fillRoundRect(xs[col], ys[row], 102, 28, 7, Theme::Panel);
      _display.drawRoundRect(xs[col], ys[row], 102, 28, 7, Theme::Border);
    }
  }

  char buf[32];
  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawString("FPS", 22, 94, 1);
  snprintf(buf, sizeof(buf), "%u", _renderFps);
  _display.setTextColor(Theme::Cyan, Theme::Panel);
  _display.drawRightString(buf, 108, 94, 1);

  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawString("HEAP", 132, 94, 1);
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(_minFreeHeap));
  _display.setTextColor(Theme::Aqua, Theme::Panel);
  _display.drawRightString(buf, 218, 94, 1);

  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawString("BLOCK", 22, 128, 1);
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(_largestFreeBlock));
  _display.setTextColor(Theme::Pearl, Theme::Panel);
  _display.drawRightString(buf, 108, 128, 1);

  _display.setTextColor(Theme::Secondary, Theme::Panel);
  _display.drawString("RESET", 132, 128, 1);
  snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(_lastResetReason));
  _display.setTextColor(Theme::Violet, Theme::Panel);
  _display.drawRightString(buf, 218, 128, 1);

  _display.fillRoundRect(16, 150, 208, 28, 8, Theme::Aqua);
  _display.setTextColor(Theme::Bg, Theme::Aqua);
  _display.drawCentreString("CLOSE DIAGNOSTICS", 120, 158, 1);

  drawWaveBand(_display, 202, Theme::Aqua, Theme::Cyan);
  _display.setTextColor(Theme::Secondary, Theme::Bg);
  _display.drawCentreString("M3RMA1D S1R3N // UI R2", 120, 250, 1);
}

void UiController::render() {
  if (_dirty.header) renderHeader();
  if (_dirty.content) renderContent();
  if (_dirty.navigation) renderNav();
  if (_dirty.overlay) renderOverlay();
}
