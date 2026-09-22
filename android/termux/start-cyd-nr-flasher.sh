#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
IMAGE="$HOME/storage/downloads/918-mermaid/cyd-wroom32e-n4-mermaid.factory.bin"
MANIFEST="$HOME/storage/downloads/918-mermaid/cyd-wroom32e-n4-mermaid.manifest.txt"
PORT="\${MERMAID_CYD_NR_PORT:-9180}"

if [[ "\${PREFIX:-}" != *"com.termux"* ]]; then
  echo "ERROR: Run this inside Termux on Android." >&2
  exit 2
fi
command -v nrflash >/dev/null 2>&1 || { echo "nrflash is missing. Run: bash android/termux/bootstrap.sh" >&2; exit 2; }
command -v termux-usb >/dev/null 2>&1 || { echo "termux-usb is missing. Install termux-api and the Termux:API Android app from F-Droid." >&2; exit 2; }
[[ -f "$IMAGE" && -f "$MANIFEST" ]] || { echo "Mermaid CYD release image/manifest missing." >&2; echo "Run: bash android/termux/cyd-build-export.sh" >&2; exit 2; }

URL="http://127.0.0.1:$PORT/"
echo "[918 MERMAID] Starting CYD NR Flasher at $URL"
( sleep 1; if command -v termux-open-url >/dev/null 2>&1; then termux-open-url "$URL" || true; else echo "Open $URL in your browser."; fi ) &
exec python "$ROOT/android/termux/cyd-nr-server.py" --image "$IMAGE" --manifest "$MANIFEST" --port "$PORT"
