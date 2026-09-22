# M3rMa1dS1r3n — Mermaid Consolidation

Canonical integration branch: `integrate/all-mermaid`

## Native 918Tech/esp32-mermaid-flipper lineage

- `feat/918-mermaid-production` — production baseline
- `feat/mermaid-mvp-autoflasher` — merged into this branch via PR #3
- `fix/display-stability-mermaid-theme` — currently identical to production baseline

## Legacy BlockChain-BailBonds/BlockChainBailbonds lineage

Baseline:
- `feat/m3rma1d-s1r3n-integrated-firmware`

Known branch deltas:
- `feat/m3rma1d-s1r3n-android-companion` — Android companion foundation
- `feat/m3rma1d-s1r3n-flipper-firmware` — full Flipper firmware overlay
- `feat/m3rma1d-s1r3n-single-s3-cam` — single S3-CAM + embedded hardware roles
- `feat/m3rma1d-safe-transport-rpc` — safe transport/RPC bridge changes
- `feat/m3rma1d-typed-flipper-rpc` — identical to integrated-firmware at inspected tip
- `feat/m3rma1d-s1r3n-embedded-ut` — embedded firmware/unit-test lineage

## Consolidation policy

The current production/MVP tree remains authoritative at repository root.
Legacy Mermaid source is preserved under `legacy/m3rma1d-s1r3n/` and must not silently overwrite current production firmware.
Run `scripts/sync-all-mermaid.sh` to materialize all legacy branch snapshots locally for comparison, migration, or selective promotion.

Physical-device acceptance remains evidence-gated; source consolidation does not imply hardware acceptance.
