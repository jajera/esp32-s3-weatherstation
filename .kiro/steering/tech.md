---
inclusion: always
---

# Tech

Demo is written for Ubuntu 24.04. Fresh machine: follow `docs/prerequisites.md` before any phase (apt → PlatformIO → `PATH`). Don’t assume `pio` exists.

Docs: Zensical + patina. Firmware: PlatformIO Arduino, env `esp32-s3-n16r8` (16MB flash, 8MB PSRAM).

Plug the **COM** port (CH343 → `/dev/ttyACM0`), not USB-OTG. Keep `ARDUINO_USB_CDC_ON_BOOT=0` so Serial hits that UART.

Pins we use:

- I2C: SDA **8**, SCL **18** (SHT40 `0x44` + BMP390 `0x77` on this bench)
- TFT SPI: SCK **12**, MOSI **11**, CS **10**, DC **9**, RST **7**; **BL → 3V3** (Estardyn 2.0″ / 240×320; silk `GND VDD SCK SDA RES DC CS BL`)
- TFT bring-up: `init(240, 320)` rotation 0 (SPI_MODE0) — never `init(240, 240, SPI_MODE3)` on this panel
- SD SPI (silk `3V3 CS MOSI CLK MISO GND`): CLK **12**, MOSI **11** (shared with TFT), MISO **14**, CS **13** — do not share TFT CS; SdFat on phases 8–9
- Controls: tact **5** (pull-up to GND), LED OK **4**, LED fault **6** — no buzzer in v1
- Don’t tie TFT MOSI net to I2C SDA
- Leave alone: 0, 19/20, 38/48, 45/46, and 35–37 on this Octal board

```bash
.venv/bin/python -m zensical serve
cd firmware/01-hello && pio run -e esp32-s3-n16r8
pio run -e esp32-s3-n16r8 -t upload --upload-port /dev/ttyACM0
```
