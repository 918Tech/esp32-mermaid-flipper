# Codex Contract: Mermaid MVP Hardware Provisioning

## Objective

Bring one Mermaid MVP stack to a verified state using exactly:

- one CYD / ESP32-WROOM-32E-N4 target with 4 MB flash,
- one ESP32-S3-N16R8 target with 16 MB flash and 8 MB OPI PSRAM,
- one Flipper Zero receiving only the Mermaid external FAP.

Codex owns the host-side build/provision/verification loop. The device identity checks in `tools/mermaid_flasher.py` are authoritative.

## Canonical commands

For a Codex-owned end-to-end run, use `scripts/codex-mermaid-mvp.sh` or `scripts/codex-mermaid-mvp.ps1`. The launcher uses auto-reviewed on-request escalation for operations that need access outside the workspace, including attached USB/serial hardware.

For direct operator execution, use:

Linux, macOS, or WSL:

```bash
bash scripts/mermaid-bootstrap.sh
bash scripts/mermaid-mvp-flash.sh
```

Windows PowerShell:

```powershell
.\scripts\mermaid-bootstrap.ps1
.\scripts\mermaid-mvp-flash.ps1
```

## Required execution order

1. Run `tools/mermaid_flasher.py self-test`.
2. Run `tools/mermaid_flasher.py doctor` and require exactly one CYD and one S3_N16R8 classification.
3. Build all three artifacts before any hardware write.
4. Re-probe immediately before each ESP upload; never bind a role to COM/tty ordering.
5. Flash CYD through its PlatformIO project.
6. Re-probe, then flash S3_N16R8 through its PlatformIO project.
7. Re-probe and require both `MERMAID_HELLO` serial identities.
8. Build/upload/launch the Flipper `mermaid_mvp` FAP using `ufbt launch`.
9. Do not report success unless the host prints `MERMAID_MVP_PROVISIONING_OK`.

## Fail-closed hardware policy

Stop instead of guessing when:

- zero or multiple candidate boards map to a role,
- a classic ESP32 is not exactly 4 MB flash,
- the S3 is not exactly 16 MB flash,
- the flashed S3 does not report exactly 8 MB PSRAM at runtime,
- the chip family does not match the expected role,
- a post-flash identity beacon is missing,
- multiple Flipper-like USB serial devices are present,
- build or upload verification fails.

## Prohibited recovery shortcuts

Do not use these in the autonomous path:

- `esptool erase_flash`,
- `esptool --force`,
- eFuse writes,
- arbitrary bootloader/partition replacement outside PlatformIO's target image,
- Flipper system-firmware flashing,
- Marauder firmware installation or RF/Wi-Fi attack automation,
- disabling post-flash verification to obtain a green result.

If a device needs destructive recovery, stop and print the exact observed state and the proposed manual recovery command for operator review.

## Allowed autonomous repair

Codex may repair source/build issues, dependency pins, serial re-enumeration handling, and benign FAP compatibility. After a repair it must rerun self-test and build before touching hardware again.

## Completion evidence

Preserve `dist/mvp-logs/inventory.json`. A complete MVP run must show:

- CYD chip family + 4 MB classification,
- S3 chip family + 16 MB classification,
- CYD `MERMAID_HELLO` identity,
- S3_N16R8 `MERMAID_HELLO` identity,
- successful `ufbt launch`,
- final sentinel `MERMAID_MVP_PROVISIONING_OK`.


## Consolidated Mermaid source policy

The canonical consolidation branch is `integrate/all-mermaid`.

- Treat repository-root production/MVP firmware as authoritative.
- Treat `legacy/m3rma1d-s1r3n/` and `legacy/branch-snapshots/` as migration/reference sources unless a change is explicitly promoted.
- Read `MERMAID_CONSOLIDATION.md` before migrating legacy capabilities.
- Use `scripts/sync-all-mermaid.sh` to materialize all known legacy Mermaid branch snapshots for comparison.
- Do not overwrite current production firmware with a legacy snapshot wholesale.
- Preserve physical acceptance gates; merged source is not equivalent to tested hardware.
