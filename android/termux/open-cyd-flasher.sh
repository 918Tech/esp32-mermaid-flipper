#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

URL="https://918tech.github.io/esp32-mermaid-flipper/wroom32e-n4-flipper.html"
OUT="$HOME/storage/downloads/918-mermaid/cyd-wroom32e-n4-mermaid.factory.bin"

[[ -f "$OUT" ]] || {
  echo "Factory image not found: $OUT" >&2
  echo "Run android/termux/cyd-build-export.sh first." >&2
  exit 2
}

if command -v termux-open-url >/dev/null 2>&1; then
  termux-open-url "$URL"
else
  echo "$URL"
fi

echo
echo "Select:"
echo "  $OUT"
echo "Use merged/factory mode at offset 0x0."
echo "Expected post-flash identity:"
echo "  MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1"
printf '%s
' "MERMAID_CYD_ANDROID_BROWSER_READY"
