#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

mode="${1:-validate}"

case "$mode" in
  sync)
    bash scripts/sync-all-mermaid.sh
    ;;
  validate)
    python3 tools/mermaid_flasher.py self-test
    node scripts/check-secrets.mjs
    node --test test/gateway.test.mjs test/protocol.test.mjs
    pio run -d firmware/e32r28t
    pio run -d firmware/s3cam
    bash scripts/sync-all-mermaid.sh

    required=(
      "feat__m3rma1d-s1r3n-integrated-firmware"
      "feat__m3rma1d-s1r3n-android-companion"
      "feat__m3rma1d-s1r3n-flipper-firmware"
      "feat__m3rma1d-s1r3n-single-s3-cam"
      "feat__m3rma1d-safe-transport-rpc"
      "feat__m3rma1d-typed-flipper-rpc"
      "feat__m3rma1d-s1r3n-embedded-ut"
    )

    for snapshot in "${required[@]}"; do
      test -d "legacy/branch-snapshots/$snapshot/m3rma1d-s1r3n" ||         test -d "legacy/branch-snapshots/$snapshot" || {
          echo "missing Mermaid snapshot: $snapshot" >&2
          exit 3
        }
    done

    printf '%s\n' "ALL_MERMAID_VALIDATION_OK"
    ;;
  *)
    echo "usage: $0 [sync|validate]" >&2
    exit 2
    ;;
esac
