#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
IMAGE="$HOME/storage/downloads/918-mermaid/cyd-wroom32e-n4-mermaid.factory.bin"
MANIFEST="$HOME/storage/downloads/918-mermaid/cyd-wroom32e-n4-mermaid.manifest.txt"

cd "$ROOT"

if [[ "${PREFIX:-}" != *"com.termux"* ]]; then
  echo "ERROR: Run this inside Termux on Android." >&2
  exit 2
fi

command -v nrflash >/dev/null 2>&1 || {
  echo "nrflash is missing. Run: bash android/termux/bootstrap.sh" >&2
  exit 2
}
command -v termux-usb >/dev/null 2>&1 || {
  echo "termux-usb is missing. Run: pkg install -y termux-api" >&2
  echo "Also install the Termux:API Android app from F-Droid." >&2
  exit 2
}

[[ -f "$IMAGE" ]] || {
  echo "Factory image missing: $IMAGE" >&2
  echo "Run: bash android/termux/cyd-build-export.sh" >&2
  exit 2
}
[[ -f "$MANIFEST" ]] || {
  echo "Manifest missing: $MANIFEST" >&2
  exit 2
}

EXPECTED_SHA="$(sed -n 's/^sha256=//p' "$MANIFEST" | head -n1)"
ACTUAL_SHA="$(sha256sum "$IMAGE" | awk '{print $1}')"
[[ "${EXPECTED_SHA,,}" == "${ACTUAL_SHA,,}" ]] || {
  echo "Factory image SHA-256 mismatch." >&2
  echo "expected=$EXPECTED_SHA" >&2
  echo "actual=$ACTUAL_SHA" >&2
  exit 3
}

echo "[1/3] Checking Android USB access"
if ! termux-usb -l >/dev/null 2>&1; then
  echo "Termux USB API is unavailable." >&2
  echo "Install/open the Termux:API Android app from F-Droid, then retry." >&2
  exit 4
fi

echo "[2/3] Probing CYD / classic ESP32"
echo "Connect the CYD with a data-capable USB-C cable."
echo "If auto-reset does not enter download mode: hold BOOT, tap EN/RST, release EN/RST, then release BOOT."
nrflash probe --chip esp32

echo "[3/3] Flashing merged 4 MB Mermaid image at 0x0 and verifying"
nrflash write --chip esp32 --offset 0x0 "$IMAGE" --verify

echo
echo "Expected application identity after reboot:"
echo "MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1"
printf '%s\n' "MERMAID_CYD_TERMUX_FLASH_OK"
