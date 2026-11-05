---
inclusion: always
---

# Firmware

One frozen folder per phase under `firmware/<nn>-name/`. Env `esp32-s3-n16r8`, COM `/dev/ttyACM0`, CDC off (`ARDUINO_USB_CDC_ON_BOOT=0`).

Pins: I2C 8/18; TFT SPI 12/11/10/9/7 (BL→3V3); SD MISO 14 / CS 13 (shared SCK/MOSI); tact 5, LED OK 4, LED fault 6. Estardyn: `init(240, 320)` rotation 0 — not 240×240 MODE3. Phases 8–9 use SdFat + `lib_ignore` AVR `SD`.

Don’t edit an old phase tree for a new stage — add a directory. Update `docs/firmware.md` and the phase blurb.

Build anytime. Flash only when asked and the serial port is present.
