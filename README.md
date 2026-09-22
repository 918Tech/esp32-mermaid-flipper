# esp32-flipper-codex

Production-oriented coordinator for a CYD / ESP32-WROOM-32E touchscreen board, an ESP32-S3-N16R8 camera/core board, and a Flipper Zero.

## Layout

- `gateway/`: Node.js service that calls the OpenAI Responses API
- `firmware/s3cam/`: ESP32-S3-N16R8 camera/core firmware
- `firmware/e32r28t/`: CYD ESP32-WROOM-32E-N4 touchscreen firmware
- `flipper/mermaid_mvp/`: benign Flipper Zero external provisioning app
- `tools/mermaid_flasher.py`: fail-closed host provisioner
- `shared/`: protocol, schema, CRC, threat model, test vectors

## Autonomous MVP flasher

The MVP provisions exactly three targets:

1. CYD: classic ESP32 with exactly 4 MB flash.
2. ESP32-S3-N16R8: ESP32-S3 with exactly 16 MB flash and the N16R8 QIO/OPI build.
3. One Flipper Zero, using `ufbt launch` for the external MVP app only.

The host provisioner identifies ESP targets with esptool before writes, refuses missing or ambiguous targets, builds all artifacts first, re-probes immediately before upload, verifies a repeatable `MERMAID_HELLO` serial identity after each ESP flash, then uploads/launches the Flipper app.

Linux/macOS:

```bash
bash scripts/mermaid-bootstrap.sh
bash scripts/mermaid-mvp-flash.sh
```

Windows PowerShell:

```powershell
.\scripts\mermaid-bootstrap.ps1
.\scripts\mermaid-mvp-flash.ps1
```

Successful end-to-end provisioning prints:

```text
MERMAID_MVP_PROVISIONING_OK
```

### Let Codex run the whole physical provisioning loop

Linux/macOS/WSL:

```bash
bash scripts/codex-mermaid-mvp.sh
```

Windows PowerShell:

```powershell
.\scripts\codex-mermaid-mvp.ps1
```

These launchers use Codex non-interactive `exec` with a workspace-write sandbox, on-request approvals, and automatic approval review. They do not use `--yolo`. Codex reads `AGENTS.md`, bootstraps the local toolchain if needed, runs the audited provisioner, and stops rather than taking destructive recovery shortcuts.

Run hardware discovery without writing:

```bash
.venv-mermaid/bin/python tools/mermaid_flasher.py doctor
```

Run post-flash identity verification:

```bash
bash scripts/mermaid-verify.sh
```

The flasher never uses esptool `--force`, does not erase flash in the normal path, and does not mutate eFuses or Flipper system firmware.

## Local configuration

Copy `.env.example` to `.env.local`. The S3 build generates `firmware/s3cam/src/wifi_credentials.h` from `WIFI_SSID` and `WIFI_PASSWORD`; the generated header is ignored by Git and must not be committed.

## Validation

```bash
node scripts/validate.mjs
```

## Notes

- The OpenAI API key stays on the gateway only.
- The E32 board never auto-approves a command.
- `loader list`, `help`, `device_info`, and `storage_info` are the initial Flipper allowlist entries.
- `help`, `status`, `scanap`, `scansta`, and `stopscan` are the initial Marauder allowlist entries.
- See `AGENTS.md` for the Codex hardware-write contract.
