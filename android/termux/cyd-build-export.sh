#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
PROJECT="$ROOT/firmware/e32r28t"
BUILD="$PROJECT/.pio/build/e32r28t"
OUTDIR="$HOME/storage/downloads/918-mermaid"
OUT="$OUTDIR/cyd-wroom32e-n4-mermaid.factory.bin"
MANIFEST="$OUTDIR/cyd-wroom32e-n4-mermaid.manifest.txt"

cd "$ROOT"

command -v pio >/dev/null || {
  echo "PlatformIO is missing. Run android/termux/bootstrap.sh first." >&2
  exit 2
}
python -c 'import esptool' >/dev/null 2>&1 || {
  echo "esptool is missing. Run android/termux/bootstrap.sh first." >&2
  exit 2
}
[[ -d "$HOME/storage/downloads" ]] || {
  echo "Android Downloads is unavailable. Run termux-setup-storage." >&2
  exit 2
}

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
image=$(basename "$OUT")
bytes=$SIZE
sha256=$SHA
expected_identity=MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1
source_branch=feat/cyd-android-codex-cli-automation
EOF

echo "[4/4] Export complete"
echo "IMAGE: $OUT"
echo "SHA256: $SHA"
echo "MANIFEST: $MANIFEST"
printf '%s
' "MERMAID_CYD_ANDROID_ARTIFACT_OK"
