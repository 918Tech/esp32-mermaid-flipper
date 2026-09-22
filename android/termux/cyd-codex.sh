#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

command -v codex >/dev/null || {
  echo "Codex CLI is missing. Run android/termux/bootstrap.sh first." >&2
  exit 2
}

if ! codex login status >/dev/null 2>&1; then
  echo "Codex is not authenticated in this Termux environment." >&2
  echo "Run: codex login --device-auth" >&2
  echo "Then rerun: bash android/termux/cyd-codex.sh" >&2
  exit 4
fi

PROMPT='You are operating the 918 MERMAID CYD Android automation path. Read AGENTS.md and android/termux/README.md first. Target exactly one ESP32-WROOM-32E-N4 / CYD with 4 MB flash. Do not modify or provision the S3 or Flipper paths. Run the CYD build checks, repair only source/build issues that stay within this repository, then run android/termux/cyd-build-export.sh. Require MERMAID_CYD_ANDROID_ARTIFACT_OK before continuing. Do not claim a physical flash or hardware acceptance from a successful build. After the factory image is exported to Android Downloads, open the public CYD Web Serial page with termux-open-url if available. Stop after reporting the exported image path, SHA-256, and the exact expected MERMAID_HELLO identity. Never erase flash, write eFuses, use esptool --force, or bypass browser USB permission.'

codex exec --sandbox workspace-write "$PROMPT"
