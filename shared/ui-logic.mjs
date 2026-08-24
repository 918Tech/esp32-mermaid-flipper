export const pages = ["CORE", "CODEX", "UART", "SCAN", "CAM"];

export function navIndexFromTouch(x, y) {
  if (y < 272 || y >= 320 || x < 0 || x >= 240) return -1;
  return Math.min(4, Math.max(0, Math.floor(x / 48)));
}

export function codexHit(x, y) {
  const approve = x >= 16 && x < 108 && y >= 198 && y < 246;
  const deny = x >= 132 && x < 224 && y >= 198 && y < 246;
  return approve ? "approve" : deny ? "deny" : null;
}

export function proposalGate({ valid, handled, expiresAtMs, nowMs }) {
  if (!valid) return "invalid";
  if (handled) return "handled";
  if (expiresAtMs && nowMs > expiresAtMs) return "expired";
  return "allowed";
}

export function calibrationStepHit(x, y, step) {
  const points = [
    [24, 60],
    [216, 60],
    [216, 228],
    [24, 228]
  ];
  if (step < 0 || step > 3) return false;
  const [px, py] = points[step];
  return Math.abs(x - px) <= 24 && Math.abs(y - py) <= 24;
}

export function settingsCloseHit(x, y) {
  return x >= 16 && x < 224 && y >= 150 && y < 178;
}

export function camCaptureHit(x, y) {
  return x >= 16 && x < 224 && y >= 198 && y < 228;
}
