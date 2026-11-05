# Phase 1 — Hello flash

Prove the toolchain: build `01-hello`, upload, see MAC and uptime on serial.

## Firmware

[`firmware/01-hello`](../firmware.md) — frozen snapshot.

---

## 1.1 Build

<div class="run" markdown>

```bash
cd firmware/01-hello
pio run -e esp32-s3-n16r8
```

```text {.no-copy}
RAM:   [=         ]  13.5% (used 44360 bytes from 327680 bytes)
Flash: [=         ]  13.4% (used 875406 bytes from 6553600 bytes)
========================= [SUCCESS] Took 11.33 seconds =========================
```

</div>

---

## 1.2 Upload

<div class="run" markdown>

```bash
pio run -e esp32-s3-n16r8 -t upload --upload-port /dev/ttyACM0
```

```text {.no-copy}
Connected to ESP32-S3 on /dev/ttyACM0:
…
Auto-detected flash size: 16MB
…
Hash of data verified.
Hard resetting via RTS pin...
========================= [SUCCESS] Took 19.60 seconds =========================
```

</div>

---

## 1.3 Serial

<div class="run" markdown>

```bash
pio device monitor --port /dev/ttyACM0 --baud 115200
```

```text {.no-copy}
weather hello — phase 1
MAC=28:84:85:8C:D0:40
chip=ESP32-S3 flash=16MB psram=8388608 bytes
weather hello MAC=28:84:85:8C:D0:40 uptime_ms=601
weather hello MAC=28:84:85:8C:D0:40 uptime_ms=1612
```

</div>

---

## Checkpoint

- [x] Build and upload succeed
- [x] Serial shows `weather hello` and a stable MAC
- [x] [Lab log](../lab-log.md)

**Next:** [Phase 2 — TFT power](02-tft-power.md)
