#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
PACKAGE="$HOME/storage/downloads/mermaid-termux-complete.tar.gz"

if [[ "${PREFIX:-}" != *"com.termux"* ]]; then
  echo "ERROR: Run this in Termux." >&2
  exit 2
fi

[[ -f "$PACKAGE" ]] || {
  echo "Package not found:" >&2
  echo "  $PACKAGE" >&2
  exit 3
}

command -v python >/dev/null 2>&1 || {
  echo "Python is missing. Run: pkg install -y python" >&2
  exit 4
}

exec python "$ROOT/serve-mermaid-share.py"
