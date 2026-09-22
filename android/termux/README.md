# Mermaid CYD CLI Flasher

Target: **ESP32-WROOM-32E-N4 CYD, 4 MB flash, 2.8-inch ILI9341, XPT2046 resistive touch**.

## Canonical flashing rule

**All physical flashing is CLI-only.**

GitHub builds and hosts the firmware. Android/Termux downloads and verifies it. The only approved write path is `nrflash`.

No Web Serial. No browser flash button. No browser API that invokes flashing.

## Setup

```bash
bash android/termux/bootstrap.sh
```

Install the separate Termux:API Android app as required for USB access.

## Download and verify the current GitHub firmware

```bash
bash android/termux/cyd-build-export.sh
```

Expected artifact:

```text
~/storage/downloads/918-mermaid/cyd-wroom32e-n4-mermaid.factory.bin
```

The manifest SHA-256 must match before flashing is allowed.

## Flash from CLI

```bash
bash android/termux/cyd-flash-termux.sh
```

The script performs exactly this transport sequence:

```text
nrflash probe --chip esp32
nrflash write --chip esp32 --offset 0x0 <factory.bin> --verify
```

Target constraints:

- chip: ESP32
- hardware: ESP32-WROOM-32E-N4
- flash: 4 MB
- image offset: 0x0
- display: ILI9341 240 x 320
- touch: XPT2046 resistive
- full-chip erase: prohibited
- eFuse writes: prohibited
- forced chip mismatch: prohibited

Expected post-boot identity:

```text
MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1
```

A successful build or download is not physical acceptance. Physical acceptance requires a verified CLI write plus successful boot/display/touch behavior on the board.
