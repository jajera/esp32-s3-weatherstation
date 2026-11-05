# Agent notes

Weather lab: ESP32-S3 N16R8 + climate sensors + ST7789 + SD. Docs in Zensical. Firmware in PlatformIO Arduino. Demo host: Ubuntu 24.04.

Read `.kiro/steering/` and `.cursor/rules/` first. Assume a clean machine until [docs/prerequisites.md](docs/prerequisites.md) is done (`pio` is not magic).

```bash
# after prerequisites — firmware
cd firmware/01-hello && pio run -e esp32-s3-n16r8

# walkthrough authors only (not lab steps)
/usr/bin/python3.14 -m venv .venv && .venv/bin/python -m pip install -r requirements.txt
.venv/bin/python -m zensical serve
```

One phase at a time. Pass the checkpoint before the next. New app behaviour = new `firmware/<nn>-…/` folder. Update `docs/firmware.md`. Don’t flash unless asked and a serial port is there.
