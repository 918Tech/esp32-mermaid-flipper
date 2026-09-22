#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
DEFAULT_IMAGE="$HOME/storage/downloads/918-mermaid/ad5x-esp32s3-n16r8-camera.factory.bin"
IMAGE="${1:-$DEFAULT_IMAGE}"
MANIFEST="${2:-}"

cd "$ROOT"

usage() {
  cat <<'EOF'
Usage:
  bash android/termux/ad5x-s3-cam-flash-termux.sh [IMAGE] [MANIFEST]

Defaults:
  IMAGE=~/storage/downloads/918-mermaid/ad5x-esp32s3-n16r8-camera.factory.bin

The image must be a merged/factory ESP32-S3 image intended for offset 0x0.
If MANIFEST is supplied, it may contain either:
  sha256=<64-hex-digest>
or a standard sha256sum-style first field.
EOF
}

if [[ "${IMAGE:-}" == "-h" || "${IMAGE:-}" == "--help" ]]; then
  usage
  exit 0
fi

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
  echo "AD5X ESP32-S3 camera factory image missing: $IMAGE" >&2
  echo "Pass the merged image path as the first argument, for example:" >&2
  echo "  bash android/termux/ad5x-s3-cam-flash-termux.sh /sdcard/Download/camera.factory.bin" >&2
  exit 2
}

if [[ -z "$MANIFEST" ]]; then
  BASE_NO_BIN="${IMAGE%.bin}"
  if [[ -f "${BASE_NO_BIN}.manifest.txt" ]]; then
    MANIFEST="${BASE_NO_BIN}.manifest.txt"
  elif [[ -f "${IMAGE}.sha256" ]]; then
    MANIFEST="${IMAGE}.sha256"
  fi
fi

if [[ -n "$MANIFEST" ]]; then
  [[ -f "$MANIFEST" ]] || {
    echo "Manifest not found: $MANIFEST" >&2
    exit 2
  }

  EXPECTED_SHA="$(sed -n 's/^sha256=//p' "$MANIFEST" | head -n1)"
  if [[ -z "$EXPECTED_SHA" ]]; then
    EXPECTED_SHA="$(awk 'NR==1 {print $1}' "$MANIFEST")"
  fi

  if [[ ! "$EXPECTED_SHA" =~ ^[0-9A-Fa-f]{64}$ ]]; then
    echo "Manifest does not contain a valid SHA-256 digest: $MANIFEST" >&2
    exit 3
  fi

  ACTUAL_SHA="$(sha256sum "$IMAGE" | awk '{print $1}')"
  if [[ "${EXPECTED_SHA,,}" != "${ACTUAL_SHA,,}" ]]; then
    echo "Factory image SHA-256 mismatch." >&2
    echo "expected=$EXPECTED_SHA" >&2
    echo "actual=$ACTUAL_SHA" >&2
    exit 3
  fi
  echo "SHA-256 verified: $ACTUAL_SHA"
else
  echo "NOTICE: no SHA-256 manifest supplied or found."
  echo "Image SHA-256: $(sha256sum "$IMAGE" | awk '{print $1}')"
fi

echo "[1/3] Checking Android USB access"
if ! termux-usb -l >/dev/null 2>&1; then
  echo "Termux USB API is unavailable." >&2
  echo "Install/open the Termux:API Android app from F-Droid, then retry." >&2
  exit 4
fi

echo "[2/3] Probing attached Espressif target"
echo "Connect the ESP32-S3 N16R8 camera board with a data-capable USB cable."
echo "If download mode does not engage: hold BOOT, tap RESET, release RESET, then release BOOT."
nrflash probe

echo "[3/3] Flashing merged AD5X camera image at 0x0 and verifying"
nrflash write --chip esp32s3 --offset 0x0 "$IMAGE" --verify

echo
echo "nrflash write + device verification completed."
echo "Application-level camera acceptance still requires boot/video evidence from the ESP32-S3 camera firmware."
printf '%s\n' "AD5X_ESP32S3_CAM_TERMUX_FLASH_OK"
