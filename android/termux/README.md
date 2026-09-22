# CYD + Android Codex CLI Automation

Target: **CYD / ESP32-WROOM-32E-N4, 4 MB flash**.

This path is intentionally CYD-only. It does not provision the ESP32-S3 or Flipper Zero.

## Architecture

```text
Android / Termux
    |
    +-- Codex CLI
    |     +-- inspect/repair repository source
    |     +-- run PlatformIO CYD build
    |     +-- create merged 4 MB factory image
    |
    +-- Android Downloads
    |     +-- cyd-wroom32e-n4-mermaid.factory.bin
    |     +-- SHA-256 manifest
    |
    +-- Chrome for Android
          +-- GitHub Pages Web Serial flasher
          +-- Android USB permission
          +-- ESP32 bootloader flash
```

Chrome's Web Serial path is the hardware boundary. Codex/Termux prepares the exact image and launches the browser, but it must not claim physical success until device evidence exists.

## First-time setup

```bash
bash android/termux/bootstrap.sh
codex login --device-auth
```

If Android asks for shared-storage permission, approve it and rerun the bootstrap.

## One-command Codex run

```bash
bash android/termux/cyd-codex.sh
```

The successful build/export sentinel is:

```text
MERMAID_CYD_ANDROID_ARTIFACT_OK
```

The output image is:

```text
~/storage/downloads/918-mermaid/cyd-wroom32e-n4-mermaid.factory.bin
```

## Open the browser flasher directly

```bash
bash android/termux/open-cyd-flasher.sh
```

In the page:

1. Select ESP32-WROOM-32E.
2. Choose **Merged / factory BIN**.
3. Select `cyd-wroom32e-n4-mermaid.factory.bin` from Android Downloads.
4. Keep offset `0x0`.
5. Connect the CYD by USB-C and grant Chrome USB permission.
6. Flash the image.
7. Hardware acceptance requires the serial identity:

```text
MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1
```

## Safety / fail-closed rules

The Android Codex path must not use full-chip erase, `esptool --force`, eFuse writes, arbitrary partition replacement, or claim hardware acceptance from compilation alone. USB permission remains user-controlled by Android/Chrome.


## Preferred Android physical flash path

For classic ESP32 CYD boards behind CP2102, CH340/CH9102, or FTDI USB-UART bridges, use the native Termux USB path instead of Chrome Web Serial.

Requirements:

- `termux-api` and `libusb` Termux packages
- the separate **Termux:API** Android app from F-Droid
- `nrflash==1.2.0`

After downloading the signed/hashed Mermaid factory image:

```bash
bash android/termux/cyd-flash-termux.sh
```

The script verifies the image SHA-256, probes the attached ESP32, writes the merged image at `0x0`, and requests device verification without doing a full-chip erase.

Success sentinel:

```text
MERMAID_CYD_TERMUX_FLASH_OK
```
