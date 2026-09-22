#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."

PYTHON_BIN="${PYTHON_BIN:-python3}"
"$PYTHON_BIN" -m venv .venv-mermaid
.venv-mermaid/bin/python -m pip install --upgrade pip
.venv-mermaid/bin/python -m pip install -r requirements-mvp.txt
UFBT_HOME="$PWD/.ufbt" .venv-mermaid/bin/ufbt update --channel=release
printf '%s\n' "MERMAID_MVP_BOOTSTRAP_OK"
