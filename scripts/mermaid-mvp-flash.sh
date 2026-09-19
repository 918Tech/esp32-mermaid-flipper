#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
PY="$PWD/.venv-mermaid/bin/python"
if [[ ! -x "$PY" ]]; then
  echo "Run scripts/mermaid-bootstrap.sh first." >&2
  exit 2
fi
export PATH="$PWD/.venv-mermaid/bin:$PATH"
export UFBT_HOME="$PWD/.ufbt"
exec "$PY" tools/mermaid_flasher.py provision
