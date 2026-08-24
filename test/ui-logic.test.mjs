import test from "node:test";
import assert from "node:assert/strict";
import { navIndexFromTouch, codexHit, proposalGate, calibrationStepHit, settingsCloseHit, camCaptureHit } from "../shared/ui-logic.mjs";

test("navigation hit testing", () => {
  assert.equal(navIndexFromTouch(10, 280), 0);
  assert.equal(navIndexFromTouch(60, 280), 1);
  assert.equal(navIndexFromTouch(120, 280), 2);
  assert.equal(navIndexFromTouch(180, 280), 3);
  assert.equal(navIndexFromTouch(230, 280), 4);
  assert.equal(navIndexFromTouch(10, 200), -1);
});

test("codex approve deny regions", () => {
  assert.equal(codexHit(20, 210), "approve");
  assert.equal(codexHit(150, 210), "deny");
  assert.equal(codexHit(120, 210), null);
});

test("proposal gating", () => {
  assert.equal(proposalGate({ valid: false, handled: false, expiresAtMs: 0, nowMs: 0 }), "invalid");
  assert.equal(proposalGate({ valid: true, handled: true, expiresAtMs: 0, nowMs: 0 }), "handled");
  assert.equal(proposalGate({ valid: true, handled: false, expiresAtMs: 10, nowMs: 11 }), "expired");
  assert.equal(proposalGate({ valid: true, handled: false, expiresAtMs: 10, nowMs: 9 }), "allowed");
});

test("calibration steps", () => {
  assert.equal(calibrationStepHit(24, 60, 0), true);
  assert.equal(calibrationStepHit(216, 60, 1), true);
  assert.equal(calibrationStepHit(216, 228, 2), true);
  assert.equal(calibrationStepHit(24, 228, 3), true);
  assert.equal(calibrationStepHit(120, 120, 0), false);
});

test("settings close", () => {
  assert.equal(settingsCloseHit(20, 160), true);
  assert.equal(settingsCloseHit(230, 160), false);
});

test("cam capture", () => {
  assert.equal(camCaptureHit(20, 210), true);
  assert.equal(camCaptureHit(10, 210), false);
});
