# Phase 2 — Power the TFT

Three wires to the display only. Backlight is the quick check. Leave the climate sensors alone.

## Firmware

**Unchanged** — [`firmware/01-hello/`](../../firmware/01-hello/).

## Preconditions

Checkpoint 1 done. TFT 8-pin header on. Unplug USB while seating jumpers.

!!! warning
    Power the TFT from **3V3 only** — 5V on the logic can kill the module. After wiring, check for heat and ~3.3 V on VDD before trusting it.

---

## Wires

![Phase 2 — TFT power](../assets/wiring-phase2-tft.svg)

| From S3 | To |
|---------|-----|
| **3V3** | TFT **VDD**, TFT **BL** |
| **GND** | TFT **GND** |

S3 underside photo — match **3V3** / **GND** by silk. Star on the breadboard rails ([how the board is tied](../hardware.md#breadboard)). TFT SPI pins stay open.

Silk left→right: `GND` · `VDD` · `SCL` · `SDA` · `RES` · `DC` · `CS` · `BL`.

---

## Check

Plug COM. Module cool. Meter ≈ 3.3 V on TFT VDD. Backlight on.

<div class="run" markdown>

```bash
pio device monitor -b 115200 --port /dev/ttyACM0
```

```text {.no-copy}
weather hello MAC=28:84:85:8C:D0:40 uptime_ms=6620
```

</div>

## Checkpoint 2

- [x] Those three TFT wires only
- [x] ~3.3 V, backlight on, no heat
- [x] Hello still prints
- [x] [Lab log](../lab-log.md)

**Next:** [Phase 3 — Sensor power](03-sensor-power.md)
