# Phase 5 — Climate read

Read **SHT40** temp/humidity and **BMP390** pressure over the I2C already wired and print them on serial. No new wires.

## Firmware

**Changed.** New folder [`firmware/05-climate/`](../../firmware/05-climate/). Leave earlier phase folders untouched.

## Preconditions

- [Checkpoint 4](04-i2c-scan.md) passed (`0x44` + `0x77` on this bench)
- Phases 2–4 wiring still on

---

## 5.1 Flash

Stop any serial monitor on COM first.

<div class="run" markdown>

```bash
cd firmware/05-climate
pio run -e esp32-s3-n16r8 -t upload --upload-port /dev/ttyACM0
```

```text {.no-copy}
Connected to ESP32-S3 on /dev/ttyACM0:
…
Hash of data verified.
Hard resetting via RTS pin...
========================= [SUCCESS] Took 35.57 seconds =========================
```

</div>

<div class="run" markdown>

```bash
pio device monitor -b 115200 --port /dev/ttyACM0
```

```text {.no-copy}
weather climate — phase 5
SDA=GPIO8 SCL=GPIO18 SHT40~0x44 BMP390=0x77
sensors ok — printing ~1 Hz
temp_c=25.27  humidity=82.5  pressure_hpa=1014.12
temp_c=25.26  humidity=82.5  pressure_hpa=1014.13
```

</div>

Print ~1 Hz. Values should look like indoor comfort numbers. Pressure is in **hPa**.

## 5.2 Validate by hand

1. Resting → stable temp / humidity / pressure lines.
2. Breathe on the **SHT40** → humidity (and often temp) jumps.
3. Optional: gentle pressure change near the **BMP390** → `pressure_hpa` moves a little.

## Checkpoint 5

- [x] Continuous climate lines on serial
- [ ] Humidity reacts to breath (try it once)
- [x] Logged in [lab-log.md](../lab-log.md)

**Stop here until checkpoint passes.**

**Next:** [Phase 6 — TFT fill](06-tft-fill.md)
