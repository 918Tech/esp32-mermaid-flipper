#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PROJECT="$ROOT/firmware/e32r28t"
BUILD="$PROJECT/.pio/build/e32r28t"
NAME="cyd-wroom32e-n4-mermaid.factory.bin"
MANIFEST_NAME="cyd-wroom32e-n4-mermaid.manifest.txt"
RELEASE_BASE="https://github.com/918Tech/esp32-mermaid-flipper/releases/download/cyd-android-latest"

cd "$ROOT"

if [[ "${PREFIX:-}" == *"com.termux"* ]]; then
  OUTDIR="$HOME/storage/downloads/918-mermaid"
  OUT="$OUTDIR/$NAME"
  MANIFEST="$OUTDIR/$MANIFEST_NAME"

  [[ -d "$HOME/storage/downloads" ]] || {
    echo "Android Downloads is unavailable. Run termux-setup-storage." >&2
    exit 2
  }
  command -v curl >/dev/null || {
    echo "curl is missing. Run: pkg install -y curl" >&2
    exit 2
  }

  mkdir -p "$OUTDIR"
  wait_download() {
    local url="$1"
    local dest="$2"
    local label="$3"
    local attempts=24
    local delay=5

    for ((i=1; i<=attempts; i++)); do
      echo "$label (attempt $i/$attempts)"
      if curl -fL --connect-timeout 15 --max-time 120 "$url" -o "$dest"; then
        return 0
      fi
      rm -f "$dest"
      if (( i < attempts )); then
        echo "Release asset not available yet; waiting ${delay}s..."
        sleep "$delay"
      fi
    done

    echo "Timed out waiting for published CYD release asset: $url" >&2
    return 1
  }

  echo "[1/4] Downloading published CYD manifest"
  wait_download "$RELEASE_BASE/$MANIFEST_NAME" "$MANIFEST" "Fetching manifest"

  EXPECTED_SHA="$(sed -n 's/^sha256=//p' "$MANIFEST" | head -n1)"
  [[ "$EXPECTED_SHA" =~ ^[0-9a-fA-F]{64}$ ]] || {
    echo "Published manifest does not contain a valid SHA-256." >&2
    exit 3
  }

  echo "[2/4] Downloading Linux-built CYD factory image"
  wait_download "$RELEASE_BASE/$NAME" "$OUT" "Fetching factory image"

  echo "[3/4] Verifying SHA-256"
  ACTUAL_SHA="$(sha256sum "$OUT" | awk '{print $1}')"
  [[ "${ACTUAL_SHA,,}" == "${EXPECTED_SHA,,}" ]] || {
    echo "SHA-256 mismatch." >&2
    echo "expected=$EXPECTED_SHA" >&2
    echo "actual=$ACTUAL_SHA" >&2
    rm -f "$OUT"
    exit 4
  }

  echo "[4/4] Android artifact ready"
  echo "IMAGE: $OUT"
  echo "SHA256: $ACTUAL_SHA"
  echo "MANIFEST: $MANIFEST"
  printf '%s\n' "MERMAID_CYD_ANDROID_ARTIFACT_OK"
  exit 0
fi

command -v pio >/dev/null || {
  echo "PlatformIO is missing." >&2
  exit 2
}
python -c 'import esptool' >/dev/null 2>&1 || {
  echo "esptool is missing." >&2
  exit 2
}

OUTDIR="${MERMAID_CI_OUTDIR:-$ROOT/dist/cyd-android}"
OUT="$OUTDIR/$NAME"
MANIFEST="$OUTDIR/$MANIFEST_NAME"

echo "[1/4] Building CYD / ESP32-WROOM-32E-N4 firmware"
pio run -d "$PROJECT"

for f in bootloader.bin partitions.bin firmware.bin; do
  [[ -f "$BUILD/$f" ]] || {
    echo "Missing PlatformIO artifact: $BUILD/$f" >&2
    exit 3
  }
done

BOOT_APP0="$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin"
[[ -f "$BOOT_APP0" ]] || {
  echo "Missing Arduino boot_app0.bin: $BOOT_APP0" >&2
  exit 3
}

mkdir -p "$OUTDIR"

echo "[2/4] Creating 4 MB factory image"
python -m esptool --chip esp32 merge_bin   -o "$OUT"   --flash_mode dio   --flash_freq 40m   --flash_size 4MB   0x1000 "$BUILD/bootloader.bin"   0x8000 "$BUILD/partitions.bin"   0xE000 "$BOOT_APP0"   0x10000 "$BUILD/firmware.bin"

echo "[3/4] Writing manifest"
SHA="$(sha256sum "$OUT" | awk '{print $1}')"
SIZE="$(wc -c < "$OUT" | tr -d ' ')"
cat > "$MANIFEST" <<EOF
MERMAID_CYD_FACTORY
hardware=ESP32-WROOM-32E-N4
flash_size=4MB
image=$NAME
bytes=$SIZE
sha256=$SHA
expected_identity=MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1
source_branch=feat/cyd-android-codex-cli-automation
source_sha=${GITHUB_SHA:-local}
EOF

echo "[4/4] Export complete"
echo "IMAGE: $OUT"
echo "SHA256: $SHA"
echo "MANIFEST: $MANIFEST"
printf '%s\n' "MERMAID_CYD_ANDROID_ARTIFACT_OK"
