---
inclusion: always
---

# Lab safety

Don’t upload or erase flash unless asked and you can see a serial port (`/dev/ttyACM*` or `/dev/ttyUSB*`). Build-only (`pio run`) is fine anytime.

Power modules from **3V3**, never 5V on sensor/TFT/SD logic. Unplug USB while rewiring when you can. After power wiring, check for heat and ~3.3 V on VDD.

No Wi‑Fi passwords or certs in git. Don’t tick a checkpoint Done without something real in `docs/lab-log.md`.
