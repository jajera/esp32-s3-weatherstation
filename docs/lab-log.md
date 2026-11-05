# Lab log

What actually passed on this bench. Detail lives in each [phase](phases/00-usb-erase.md); this page is the scoreboard.

## Bench

| Item | Value |
|------|-------|
| Host | Ubuntu 24.04 · `/dev/ttyACM0` (CH343) |
| Board | ESP32-S3 N16R8 · MAC `28:84:85:8C:D0:40` |
| Stack | Estardyn 2.0″ ST7789 · SHT40 + BMP390 · TF/SPI reader · breadboard 830 |
| On chip now | [`firmware/09-views`](firmware.md) |

## Checkpoints

| # | Result | Notes |
|---|--------|-------|
| 0 | **Pass** | COM `/dev/ttyACM0` · chip-id N16R8 · erase 2.3s |
| 1 | **Pass** | `01-hello` — flash 16MB / PSRAM 8MB · MAC `28:84:85:8C:D0:40` |
| 2 | **Pass** | TFT VDD+BL→3V3, GND; hello still on serial |
| 3 | **Pass** | SHT40 + BMP390 VCC/GND on 3V3 star; hello still on serial |
| 4 | **Pass** | I2C: SHT40 `0x44` · BMP390 `0x77` (SDA=8 SCL=18) |
| 5 | **Pass** | `05-climate` · ~25°C / 82% RH / 1014 hPa @ 1 Hz |
| 6 | **Pass** | TFT SPI R→G→B · I2C still ack `0x44`/`0x77` |
| 7 | **Pass** | Live TFT climate UI (carried into Phase 9 **Now**) |
| 8 | Partial | `SD ready` + `sd append`; TFT live on shared SPI — still confirm host `/climate.csv` |
| 9 | **Pass** | `09-views` · tact cycles [Now](assets/bench-phase9-now.jpg) / [Session](assets/bench-phase9-session.jpg) / [System](assets/bench-phase9-system.jpg) · LED self-test OK · partial redraw |

## Quirks worth remembering

- Use the **COM / UART** Type-C jack, not the other one, or nothing enumerates.
- Keep sensor and TFT logic on **3V3**.
- This Estardyn silk uses **`SCK`** for SPI clock (not `SCL`); same GPIO12 net.
