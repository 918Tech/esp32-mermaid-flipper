# Mermaid HTML Master Flasher Design

## Goal

Create one canonical HTML-based master flasher for the 918 MERMAID CYD workflow on Android/Termux.

The user-facing experience is a single local web page. The physical flash transport remains CLI-only through `nrflash`. The browser must not use Web Serial, WebUSB, esptool-in-browser, or any direct USB flashing path.

## Target Hardware

- CYD / ESP32-WROOM-32E-N4
- Classic ESP32
- 4 MB flash
- 2.8-inch resistive-touch CYD hardware profile
- Factory image:
  `firmware/releases/cyd/cyd-wroom32e-n4-mermaid.factory.bin`
- Manifest:
  `firmware/releases/cyd/cyd-wroom32e-n4-mermaid.manifest.txt`

## Source of Truth

The checked-in repository firmware is authoritative.

Initial branch:

`feat/cyd-android-codex-cli-automation`

The master flasher must not depend on GitHub Releases.

## User Experience

The user runs one Termux command:

`bash android/termux/master-flasher/start.sh`

That command starts a local server and opens or prints a local URL such as:

`http://127.0.0.1:9181/`

The page presents one primary action:

**FLASH MERMAID CYD**

The page shows these stages:

1. Termux environment
2. Storage availability
3. Required packages
4. Repository firmware presence
5. Manifest validation
6. SHA-256 validation
7. Android USB availability
8. ESP32 probe
9. Flash
10. Verify
11. Completion sentinel

## Architecture

### 1. HTML Front End

Path:

`android/termux/master-flasher/index.html`

Responsibilities:

- display device and firmware status
- show manifest metadata
- show SHA-256
- start the automated flash flow
- stream status updates
- display actionable error messages
- show final success sentinel

The front end has no direct USB access.

### 2. Local Termux Backend

Path:

`android/termux/master-flasher/server.py`

Responsibilities:

- bind only to localhost by default
- expose status endpoint
- expose one flash-start endpoint
- launch the master shell orchestrator
- stream line-oriented progress back to the page
- reject concurrent flash jobs
- never accept arbitrary shell commands from the browser

### 3. Master Shell Orchestrator

Path:

`android/termux/master-flasher/master-flasher.sh`

Responsibilities:

- require Termux
- check shared storage
- install missing required Termux packages
- install/verify `nrflash==1.2.0`
- locate the checked-in firmware and manifest
- validate manifest fields
- calculate and compare SHA-256
- confirm USB API availability
- run `nrflash probe --chip esp32`
- run the canonical CLI write
- report exact stage failures
- emit `MERMAID_CYD_MASTER_FLASH_OK` only after write verification succeeds

Canonical write:

`nrflash write --chip esp32 --offset 0x0 <factory-image> --verify`

## Safety Constraints

The master flasher must never:

- erase the entire chip as a separate operation
- write eFuses
- use `esptool --force`
- use Web Serial
- use browser WebUSB
- flash any S3 target
- flash Flipper hardware
- execute arbitrary commands supplied by HTTP requests
- claim hardware acceptance before the CLI verify step succeeds

The backend should bind to `127.0.0.1` by default. LAN exposure is outside the master-flasher path and remains a separate transfer-server function.

## Firmware Validation

Before USB access begins, validate:

- manifest header = `MERMAID_CYD_FACTORY`
- hardware = `ESP32-WROOM-32E-N4`
- flash_size = `4MB`
- image filename matches the expected factory image
- SHA-256 is a 64-character hex digest
- calculated image SHA-256 matches the manifest

Expected application identity:

`MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1`

## Error Handling

Every stage returns a stable machine-readable status plus human-readable text.

Examples:

- `ERR_NOT_TERMUX`
- `ERR_STORAGE`
- `ERR_DEPENDENCY`
- `ERR_FIRMWARE_MISSING`
- `ERR_MANIFEST`
- `ERR_SHA256`
- `ERR_USB_API`
- `ERR_PROBE`
- `ERR_FLASH`
- `ERR_VERIFY`

No failure path may emit the success sentinel.

## HTTP Surface

Minimum endpoints:

- `GET /` — HTML UI
- `GET /api/status` — current readiness
- `POST /api/flash` — start one flash job
- `GET /api/events` — status stream

The HTTP API is intentionally narrow and does not expose a generic shell endpoint.

## Testing

### Static

- shell syntax validation
- Python syntax validation
- HTML loads without external dependencies

### Firmware

- checked-in firmware exists
- manifest parses
- SHA-256 matches

### Backend

- rejects concurrent jobs
- does not accept arbitrary command input
- reports every failure state
- binds to localhost by default

### Physical

Physical acceptance requires:

1. successful `nrflash probe --chip esp32`
2. successful `nrflash write ... --verify`
3. final sentinel:
   `MERMAID_CYD_MASTER_FLASH_OK`

A CI build alone is not physical acceptance.

## Canonical Entry Point

`bash android/termux/master-flasher/start.sh`

This becomes the single user-facing flasher entry point for the Mermaid CYD Android/Termux path.
