# Mermaid MVP Flasher

The MVP flasher is a host-side state machine. It does not trust serial-port ordering and does not write until all local artifacts build successfully.

## State machine

`SELF_TEST -> BUILD -> DISCOVER -> CLASSIFY -> FLASH_CYD -> REDISCOVER -> FLASH_S3 -> REDISCOVER -> VERIFY_IDENTITIES -> UFBT_LAUNCH -> DONE`

Any transition failure moves to `STOP` with a non-zero exit status.

## Role classification

- `CYD`: classic ESP32 family plus exactly 4 MB detected flash.
- `S3_N16R8`: ESP32-S3 plus exactly 16 MB detected flash.
- Other Espressif devices remain unclassified and are never written by the MVP path.

The S3 build uses `default_16MB.csv`, quad flash (`qio`), and octal PSRAM (`qio_opi` / `opi`) for the N16R8 module.

## Verification contract

The flashed firmware emits an identity every two seconds at 115200 baud:

- `MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1`
- `MERMAID_HELLO role=S3_N16R8 hw=ESP32-S3-N16R8 proto=MVP1 camera=<ready|fault>`

A camera fault does not falsify hardware identity, but it remains visible in the S3 beacon for device acceptance testing.

## Flipper scope

The MVP deploys a normal external FAP with `ufbt launch`. It intentionally does not replace Flipper system firmware. This makes the Flipper step independently removable and recoverable.

## Codex entry point

Codex should invoke only the repository scripts documented in `AGENTS.md`. The scripts set the local virtualenv, uFBT SDK directory, and call the audited Python provisioner.
