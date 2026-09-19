#!/usr/bin/env python3
"""Mermaid MVP hardware provisioner.

Fail-closed host orchestrator for:
- CYD / ESP32-WROOM-32E-N4 (classic ESP32, 4 MB flash)
- ESP32-S3-N16R8 (ESP32-S3, 16 MB flash)
- Flipper Zero external Mermaid MVP app

The normal provisioning path never erases flash wholesale, never passes esptool
--force, never writes eFuses, and never flashes Flipper system firmware.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
import time
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Iterable, Sequence

ROOT = Path(__file__).resolve().parents[1]
CYD_DIR = ROOT / "firmware" / "e32r28t"
S3_DIR = ROOT / "firmware" / "s3cam"
FLIPPER_DIR = ROOT / "flipper" / "mermaid_mvp"
LOG_DIR = ROOT / "dist" / "mvp-logs"

CYD_ROLE = "CYD"
S3_ROLE = "S3_N16R8"
EXPECTED_CYD_HELLO = "MERMAID_HELLO role=CYD hw=ESP32-WROOM-32E-N4 proto=MVP1"
EXPECTED_S3_HELLO_PREFIX = "MERMAID_HELLO role=S3_N16R8 hw=ESP32-S3-N16R8 proto=MVP1 flash=16777216 psram=8388608"


@dataclass(frozen=True)
class Probe:
    port: str
    description: str
    hwid: str
    chip: str
    flash_mb: int
    role: str | None


def run(
    cmd: Sequence[str],
    *,
    cwd: Path | None = None,
    timeout: int = 120,
    check: bool = True,
    capture: bool = False,
) -> subprocess.CompletedProcess[str]:
    print("+", " ".join(str(part) for part in cmd))
    result = subprocess.run(
        list(cmd),
        cwd=str(cwd) if cwd else None,
        text=True,
        capture_output=capture,
        timeout=timeout,
        check=False,
    )
    if capture:
        if result.stdout:
            print(result.stdout.rstrip())
        if result.stderr:
            print(result.stderr.rstrip(), file=sys.stderr)
    if check and result.returncode != 0:
        raise RuntimeError(f"command failed ({result.returncode}): {' '.join(cmd)}")
    return result


def require_tool(name: str) -> str:
    path = shutil.which(name)
    if not path:
        raise RuntimeError(
            f"required tool '{name}' is missing; run scripts/mermaid-bootstrap.sh "
            "or scripts/mermaid-bootstrap.ps1 first"
        )
    return path


def serial_ports() -> list[tuple[str, str, str]]:
    try:
        from serial.tools import list_ports
    except ImportError as exc:
        raise RuntimeError("pyserial is missing; run the Mermaid bootstrap script") from exc

    found: list[tuple[str, str, str]] = []
    for port in list_ports.comports():
        found.append((port.device, port.description or "", port.hwid or ""))
    return found


def parse_chip(text: str) -> str:
    patterns = [
        r"Chip is\s+([^\r\n(]+)",
        r"Detecting chip type\.\.\.\s*([^\r\n]+)",
    ]
    for pattern in patterns:
        match = re.search(pattern, text, flags=re.IGNORECASE)
        if match:
            return match.group(1).strip().upper()
    return ""


def parse_flash_mb(text: str) -> int:
    match = re.search(r"Detected flash size:\s*(\d+)\s*MB", text, flags=re.IGNORECASE)
    if match:
        return int(match.group(1))
    match = re.search(r"Flash size:\s*(\d+)\s*MB", text, flags=re.IGNORECASE)
    if match:
        return int(match.group(1))
    return 0


def classify_role(chip: str, flash_mb: int) -> str | None:
    normalized = chip.upper().replace("-", "")
    if "ESP32S3" in normalized and flash_mb == 16:
        return S3_ROLE

    classic_esp32 = (
        normalized == "ESP32"
        or normalized.startswith("ESP32D0WD")
        or normalized.startswith("ESP32U4WDH")
        or normalized.startswith("ESP32PICO")
    )
    if classic_esp32 and flash_mb == 4:
        return CYD_ROLE
    return None


def esptool_probe(port: str, description: str, hwid: str) -> Probe | None:
    cmd_base = [sys.executable, "-m", "esptool", "--port", port, "--baud", "115200"]
    try:
        chip_result = run(cmd_base + ["chip_id"], timeout=10, check=False, capture=True)
    except (subprocess.TimeoutExpired, OSError):
        return None
    chip_text = (chip_result.stdout or "") + "\n" + (chip_result.stderr or "")
    chip = parse_chip(chip_text)
    if chip_result.returncode != 0 or not chip:
        return None

    try:
        flash_result = run(cmd_base + ["flash_id"], timeout=10, check=False, capture=True)
    except (subprocess.TimeoutExpired, OSError):
        return None
    flash_text = (flash_result.stdout or "") + "\n" + (flash_result.stderr or "")
    flash_mb = parse_flash_mb(flash_text)
    if flash_result.returncode != 0 or flash_mb <= 0:
        return None

    return Probe(
        port=port,
        description=description,
        hwid=hwid,
        chip=chip,
        flash_mb=flash_mb,
        role=classify_role(chip, flash_mb),
    )


def discover() -> list[Probe]:
    probes: list[Probe] = []
    for port, description, hwid in serial_ports():
        print(f"probe candidate: {port} | {description} | {hwid}")
        probe = esptool_probe(port, description, hwid)
        if probe:
            probes.append(probe)
            print(
                f"  -> chip={probe.chip} flash={probe.flash_mb}MB "
                f"role={probe.role or 'UNCLASSIFIED'}"
            )
    return probes


def unique_targets(probes: Iterable[Probe]) -> dict[str, Probe]:
    by_role: dict[str, list[Probe]] = {CYD_ROLE: [], S3_ROLE: []}
    for probe in probes:
        if probe.role in by_role:
            by_role[probe.role].append(probe)

    errors: list[str] = []
    for role, matches in by_role.items():
        if len(matches) != 1:
            errors.append(f"{role}: expected exactly 1 target, found {len(matches)}")
    if errors:
        details = "\n".join(
            f"- {p.port}: chip={p.chip}, flash={p.flash_mb}MB, role={p.role or 'UNCLASSIFIED'}"
            for p in probes
        ) or "- no Espressif bootloader targets detected"
        raise RuntimeError("target classification failed:\n" + "\n".join(errors) + "\nDetected:\n" + details)

    return {role: matches[0] for role, matches in by_role.items()}


def flipper_candidates() -> list[tuple[str, str, str]]:
    return [
        item
        for item in serial_ports()
        if "flipper" in (item[1] + " " + item[2]).lower()
    ]


def write_inventory(probes: list[Probe]) -> None:
    LOG_DIR.mkdir(parents=True, exist_ok=True)
    payload = {
        "generated_at_unix": int(time.time()),
        "esp": [asdict(p) for p in probes],
        "flipper_candidates": [
            {"port": p, "description": d, "hwid": h} for p, d, h in flipper_candidates()
        ],
    }
    (LOG_DIR / "inventory.json").write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")


def ensure_local_env() -> None:
    env_file = ROOT / ".env.local"
    if env_file.exists():
        return
    env_file.write_text("WIFI_SSID=\nWIFI_PASSWORD=\n", encoding="utf-8")
    print("created .env.local with blank Wi-Fi values; S3 will boot with Wi-Fi unprovisioned")


def build_all() -> None:
    require_tool("pio")
    require_tool("ufbt")
    ensure_local_env()
    run(["pio", "run"], cwd=CYD_DIR, timeout=900)
    run(["pio", "run"], cwd=S3_DIR, timeout=900)
    run(["ufbt"], cwd=FLIPPER_DIR, timeout=900)


def flash_esp(role: str, target: Probe) -> None:
    project_dir = CYD_DIR if role == CYD_ROLE else S3_DIR
    print(f"flashing {role} on {target.port}")
    run(
        ["pio", "run", "-t", "upload", "--upload-port", target.port],
        cwd=project_dir,
        timeout=300,
    )


def verify_serial(port: str, expected: str, *, prefix: bool = False, timeout_s: int = 15) -> str:
    try:
        import serial
    except ImportError as exc:
        raise RuntimeError("pyserial is missing; run the Mermaid bootstrap script") from exc

    deadline = time.monotonic() + timeout_s
    last_lines: list[str] = []
    last_error: Exception | None = None
    while time.monotonic() < deadline:
        try:
            with serial.Serial(port, 115200, timeout=0.5) as ser:
                try:
                    ser.dtr = False
                    ser.rts = False
                except Exception:
                    pass
                while time.monotonic() < deadline:
                    raw = ser.readline()
                    if not raw:
                        continue
                    line = raw.decode("utf-8", errors="replace").strip()
                    if not line:
                        continue
                    print(f"[{port}] {line}")
                    last_lines.append(line)
                    last_lines = last_lines[-20:]
                    matched = line.startswith(expected) if prefix else line == expected
                    if matched:
                        return line
        except Exception as exc:
            last_error = exc
            time.sleep(0.4)

    detail = " | ".join(last_lines[-5:]) if last_lines else "no serial identity lines received"
    if last_error:
        detail += f"; last serial error: {last_error}"
    raise RuntimeError(f"post-flash verification failed on {port}: {detail}")


def verify_targets(targets: dict[str, Probe]) -> None:
    verify_serial(targets[CYD_ROLE].port, EXPECTED_CYD_HELLO)
    verify_serial(targets[S3_ROLE].port, EXPECTED_S3_HELLO_PREFIX, prefix=True)


def deploy_flipper() -> None:
    require_tool("ufbt")
    candidates = flipper_candidates()
    if len(candidates) > 1:
        raise RuntimeError(
            "multiple Flipper-like serial devices detected; disconnect all but the target Flipper Zero"
        )
    if len(candidates) == 0:
        print("warning: Flipper serial identity was not enumerable; ufbt launch will perform final USB detection")
    run(["ufbt", "launch"], cwd=FLIPPER_DIR, timeout=300)


def doctor() -> dict[str, Probe]:
    require_tool("pio")
    require_tool("ufbt")
    probes = discover()
    write_inventory(probes)
    targets = unique_targets(probes)
    print("\nclassified targets:")
    for role in (CYD_ROLE, S3_ROLE):
        p = targets[role]
        print(f"  {role}: {p.port} | {p.chip} | {p.flash_mb}MB")
    flip = flipper_candidates()
    print(f"  FLIPPER: {len(flip)} enumerable candidate(s); ufbt is authoritative at launch")
    print("MERMAID_MVP_DOCTOR_OK")
    return targets


def provision() -> None:
    build_all()
    doctor()

    targets = unique_targets(discover())
    flash_esp(CYD_ROLE, targets[CYD_ROLE])

    targets = unique_targets(discover())
    flash_esp(S3_ROLE, targets[S3_ROLE])

    targets = unique_targets(discover())
    verify_targets(targets)

    deploy_flipper()
    print("MERMAID_MVP_PROVISIONING_OK")


def self_test() -> None:
    fixtures = [
        ("ESP32-D0WD-V3", 4, CYD_ROLE),
        ("ESP32-S3 (QFN56) (revision v0.2)", 16, S3_ROLE),
        ("ESP32-S3", 8, None),
        ("ESP32-C3", 4, None),
    ]
    for chip, flash_mb, expected in fixtures:
        actual = classify_role(chip, flash_mb)
        assert actual == expected, (chip, flash_mb, actual, expected)

    assert parse_chip("Chip is ESP32-S3 (QFN56) (revision v0.2)\n") == "ESP32-S3"
    assert parse_flash_mb("Detected flash size: 16MB\n") == 16

    good = [
        Probe("/dev/ttyA", "", "", "ESP32-D0WD-V3", 4, CYD_ROLE),
        Probe("/dev/ttyB", "", "", "ESP32-S3", 16, S3_ROLE),
    ]
    chosen = unique_targets(good)
    assert chosen[CYD_ROLE].port == "/dev/ttyA"
    assert chosen[S3_ROLE].port == "/dev/ttyB"

    try:
        unique_targets(good + [Probe("/dev/ttyC", "", "", "ESP32-S3", 16, S3_ROLE)])
    except RuntimeError:
        pass
    else:
        raise AssertionError("duplicate S3 target must fail closed")

    print("MERMAID_MVP_SELF_TEST_OK")


def main() -> int:
    parser = argparse.ArgumentParser(description="918 Technologies Mermaid MVP flasher")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("self-test", help="run host policy tests without hardware")
    sub.add_parser("doctor", help="detect and classify attached targets without writing")
    sub.add_parser("build", help="build both ESP firmware images and the Flipper FAP")
    sub.add_parser("verify", help="verify already-flashed ESP role identity beacons")
    sub.add_parser("provision", help="build, detect, flash, verify, and launch the Flipper app")
    args = parser.parse_args()

    try:
        if args.command == "self-test":
            self_test()
        elif args.command == "doctor":
            doctor()
        elif args.command == "build":
            build_all()
            print("MERMAID_MVP_BUILD_OK")
        elif args.command == "verify":
            targets = doctor()
            verify_targets(targets)
            print("MERMAID_MVP_VERIFY_OK")
        elif args.command == "provision":
            provision()
        return 0
    except (RuntimeError, subprocess.TimeoutExpired) as exc:
        print(f"MERMAID_MVP_ERROR: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
