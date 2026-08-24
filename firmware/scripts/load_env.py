Import("env")
from pathlib import Path

root = Path(env.subst("$PROJECT_DIR")).parent.parent
env_file = root / ".env.local"
generated = Path(env.subst("$PROJECT_DIR")) / "src" / "wifi_credentials.h"

ssid = ""
password = ""

def esc(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')

if env_file.exists():
    for line in env_file.read_text(encoding="utf-8").splitlines():
        if "=" not in line or line.lstrip().startswith("#"):
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if key == "WIFI_SSID":
            ssid = value
        elif key == "WIFI_PASSWORD":
            password = value

generated.write_text(
    '#pragma once\n'
    f'#define WIFI_SSID "{esc(ssid)}"\n'
    f'#define WIFI_PASSWORD "{esc(password)}"\n',
    encoding="utf-8",
)
