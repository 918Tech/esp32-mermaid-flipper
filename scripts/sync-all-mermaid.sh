#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/legacy/branch-snapshots"
REMOTE="https://github.com/BlockChain-BailBonds/BlockChainBailbonds.git"

branches=(
  "feat/m3rma1d-s1r3n-integrated-firmware"
  "feat/m3rma1d-s1r3n-android-companion"
  "feat/m3rma1d-s1r3n-flipper-firmware"
  "feat/m3rma1d-s1r3n-single-s3-cam"
  "feat/m3rma1d-safe-transport-rpc"
  "feat/m3rma1d-typed-flipper-rpc"
  "feat/m3rma1d-s1r3n-embedded-ut"
)

mkdir -p "$DEST"

for ref in "${branches[@]}"; do
  slug="${ref//\//__}"
  tmp="$(mktemp -d)"
  git clone --depth 1 --single-branch --branch "$ref" "$REMOTE" "$tmp/repo"
  rm -rf "$DEST/$slug"
  mkdir -p "$DEST/$slug"
  cp -a "$tmp/repo/m3rma1d-s1r3n/." "$DEST/$slug/"
  rm -rf "$tmp"
  printf 'synced %s -> %s\n' "$ref" "$DEST/$slug"
done

printf 'ALL_MERMAID_SOURCES_SYNCED\n'
