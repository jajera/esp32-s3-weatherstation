# Firmware

PlatformIO Arduino apps for the ESP32-S3 N16R8. Each phase that needs new behaviour gets its **own folder** under [`firmware/`](../firmware/). That folder is a frozen snapshot — later phases add a new directory; they do not rewrite an old one.

Pin-level wiring lives on [Hardware](hardware.md). What actually passed on this bench is in the [lab log](lab-log.md).

```text
firmware/
  01-hello/          # Phase 1 (+ kept for Phases 2–3)
  04-i2c-scan/       # Phase 4
  05-climate/        # Phase 5
  06-tft-fill/       # Phase 6
  07-climate-ui/     # Phase 7
  08-sd-log/         # Phase 8
  09-views/          # Phase 9  ← on the chip now
```

Phase 0 has no app (erased chip). Phases 2–3 keep flashing `01-hello` unchanged while you only add power wires.

---

## On the board now

**[`firmware/09-views`](../firmware/09-views/)** — Phase 9.

| Behaviour | Detail |
|-----------|--------|
| TFT | Estardyn 2.0″ ST7789 · `init(240, 320)` · rotation 0 · partial redraw (no per-second `fillScreen`) |
| Views | **Now** / **Session** / **System** — tact on GPIO5 cycles them |
| Sensors | SHT40 + BMP390 @ ~1 Hz (same I2C as Phase 5) |
| CSV | `/climate.csv` append every **60 s** (`LOG_PERIOD_MS`) |
| LEDs | green GPIO4 pulses on SD append; red GPIO6 latches on fault |
| Boot | LED self-test green → red (~800 ms each), then UI |

Do **not** flash `init(240, 240, SPI_MODE3)` — that blanks this Estardyn panel.

Source: [`09-views/src/main.cpp`](../firmware/09-views/src/main.cpp). Previous stages stay on disk for rewind: `08-sd-log` · `07-climate-ui` · `06-tft-fill` · `05-climate` · `04-i2c-scan` · `01-hello`.

### Bench settings

| Setting | Value |
|---------|-------|
| Port | COM `/dev/ttyACM0` (CH343) |
| MAC | `28:84:85:8C:D0:40` |
| Flash / PSRAM | 16MB / 8MB (N16R8) |
| I2C | SDA **GPIO8**, SCL **GPIO18** · SHT40 `0x44` · BMP390 `0x77` |
| TFT SPI | SCK **12**, MOSI **11**, CS **10**, DC **9**, RST **7** · BL→3V3 |
| TFT silk | `GND VDD SCK SDA RES DC CS BL` (clock may print `SCL` on other packs) |
| SD SPI | CLK **12**, MOSI **11** (shared), MISO **14**, CS **13** · SdFat |
| Controls | tact **5**, LED OK **4**, LED fault **6** |

CDC is **off** (`ARDUINO_USB_CDC_ON_BOOT=0`) so `Serial` is on the COM jack, not native USB-OTG.

Phases **8–9** use **SdFat** on the shared SPI bus and `lib_ignore` the AVR `SD` / OLED transitive deps that break the link. Do not swap back to `arduino-libraries/SD` without fixing that first.

---

## Build and flash

Same pattern for every phase: `cd` into the phase folder, build with env `esp32-s3-n16r8`, then upload to the COM port. Stop any serial monitor before upload.

<div class="run" markdown>

```bash
cd firmware/09-views
pio run -e esp32-s3-n16r8
```

```text {.no-copy}
RAM:   [=         ]   7.7% (used 25092 bytes from 327680 bytes)
Flash: [=         ]   6.3% (used 410846 bytes from 6553600 bytes)
========================= [SUCCESS] Took 9.11 seconds =========================
```

</div>

<div class="run" markdown>

```bash
pio run -e esp32-s3-n16r8 -t upload --upload-port /dev/ttyACM0
```

```text {.no-copy}
Connected to ESP32-S3 on /dev/ttyACM0:
Chip type:          ESP32-S3 (QFN56) (revision v0.2)
Features:           Wi-Fi, BT 5 (LE), Dual Core + LP Core, 240MHz, Embedded PSRAM 8MB (AP_3v3)
Crystal frequency:  40MHz
MAC:                28:84:85:8c:d0:40
…
Auto-detected flash size: 16MB
…
Hash of data verified.
Hard resetting via RTS pin...
========================= [SUCCESS] Took 17.79 seconds =========================
```

</div>

Monitor:

```bash
pio device monitor -b 115200 --port /dev/ttyACM0
```

Expect something like:

```text
weather views — phase 9
BTN=5 LED_OK=4 LED_FAULT=6
SD ready
UI live — BTN cycles Now/Session/System
view=0 temp_c=… humidity=… pressure_hpa=…
sd append /climate.csv uptime_ms=…
```

Serial bring-up for the first hello image: [Phase 1 §1.3](phases/01-hello-flash.md#13-serial). Full Phase 9 walkthrough: [09 — TFT views](phases/09-views.md).

---

## Stage map

| Phase | Folder | What it proves |
|-------|--------|----------------|
| 0 | — | empty (erased) — [00](phases/00-usb-erase.md) |
| 1 | [`01-hello`](../firmware/01-hello/) | serial hello, flash/PSRAM — [01](phases/01-hello-flash.md) |
| 2–3 | `01-hello` | same image while TFT / sensor **power** only — [02](phases/02-tft-power.md) · [03](phases/03-sensor-power.md) |
| 4 | [`04-i2c-scan`](../firmware/04-i2c-scan/) | SHT40 + BMP390 ack — [04](phases/04-i2c-scan.md) |
| 5 | [`05-climate`](../firmware/05-climate/) | T/RH/P stream on serial — [05](phases/05-climate-read.md) |
| 6 | [`06-tft-fill`](../firmware/06-tft-fill/) | ST7789 SPI RGB fill — [06](phases/06-tft-fill.md) |
| 7 | [`07-climate-ui`](../firmware/07-climate-ui/) | live Climate screen — [07](phases/07-climate-ui.md) |
| 8 | [`08-sd-log`](../firmware/08-sd-log/) | UI + `/climate.csv` (SdFat) — [08](phases/08-sd-log.md) |
| **9** | **[`09-views`](../firmware/09-views/)** | **Now/Session/System + LEDs + CSV** — [09](phases/09-views.md) |
