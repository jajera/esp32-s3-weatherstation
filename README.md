# ESP32-S3 weather station

ESP32-S3 weather station with live display and data logging

A desk climate station on Ubuntu 24.04: ESP32-S3 N16R8 + climate sensors + ST7789 + SD log — wire it, flash phase folders, watch readings grow.

## Docs locally

```bash
cd ~/workspace/jajera/esp32-s3-weatherstation
/usr/bin/python3.14 -m venv .venv
.venv/bin/python -m pip install -r requirements.txt
.venv/bin/python -m zensical serve
```

Open the URL printed by the server (usually `http://127.0.0.1:8000`).

Use `python -m` (not `.venv/bin/zensical` alone) so a broken script shebang cannot bite. Pin `/usr/bin/python3.14` (newest on this host) — bare `python3` may be the distro 3.12.

## Start here

1. [docs/prerequisites.md](docs/prerequisites.md) — git, Python, PlatformIO
2. [docs/index.md](docs/index.md) — story + phases
3. [docs/firmware.md](docs/firmware.md) — frozen `firmware/<nn>-…/` apps

**Current:** Phases 0–9 passed on the bench (`firmware/09-views` — Now/Session/System + SD CSV + status LEDs). See the [lab log](docs/lab-log.md). Hosted docs: https://esp32-s3-weatherstation.johna.kiwi/
