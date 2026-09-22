#!/data/data/com.termux/files/usr/bin/bash
set -euo pipefail

printf '%s
' "[918 MERMAID] Android / Termux bootstrap"

if [[ "${PREFIX:-}" != *"com.termux"* ]]; then
  echo "ERROR: Run this script inside Termux on Android." >&2
  exit 2
fi

pkg update -y
pkg install -y git nodejs python clang make pkg-config openssh termux-tools

python -m pip install --upgrade pip
python -m pip install "platformio==6.2.0" "esptool==4.12.0" "pyserial==3.5"

if ! command -v codex >/dev/null 2>&1; then
  npm install -g @openai/codex
fi

if [[ ! -d "$HOME/storage/downloads" ]]; then
  echo
  echo "Android shared storage is not available yet."
  echo "Running termux-setup-storage; approve the Android storage prompt, then rerun this script."
  termux-setup-storage || true
  exit 3
fi

printf '%s
' "Codex: $(codex --version 2>/dev/null || echo installed)"
printf '%s
' "PlatformIO: $(pio --version)"
printf '%s
' "esptool: $(python -m esptool version 2>/dev/null | head -n1)"
printf '%s
' "MERMAID_CYD_ANDROID_BOOTSTRAP_OK"
