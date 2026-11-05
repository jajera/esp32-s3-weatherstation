# Phase 0 — USB, detect, erase

Get the PC talking to the board, prove it’s an N16R8, wipe flash so Phase 1 starts clean.

**Before this:** [Prerequisites](../prerequisites.md) — packages + PlatformIO installed (`pio` / `esptool` on your `PATH`).

## Firmware

Chip is empty after erase. No phase folder under `firmware/` yet — Phase 1 adds `01-hello/`. Tracker: [Firmware](../firmware.md).

```mermaid
flowchart LR
  PC[Linux PC] -->|data USB-C| COM[COM CH343]
  COM --> S3[ESP32-S3]
  S3 --> Flash["16MB flash"]
```

---

## 0.1 Serial permissions (once per machine)

From the **repo root**:

<div class="run" markdown>

```bash
sudo ./scripts/setup-linux-usb.sh
```

```text {.no-copy}
Installing udev rules → /etc/udev/rules.d/99-platformio-udev.rules
…
Setup complete.
```

</div>

Confirm:

<div class="run" markdown>

```bash
groups | grep dialout
test -f /etc/udev/rules.d/99-platformio-udev.rules && echo udev_ok
```

```text {.no-copy}
dialout
udev_ok
```

</div>

Log out/in (or reboot) if `dialout` was just added, then unplug/replug the board.

---

## 0.2 Detect the board

Plug the **COM** Type-C (UART/CH343), not the USB-OTG jack.

<div class="run" markdown>

```bash
lsusb
ls -la /dev/ttyACM* /dev/serial/by-id/
pio device list
```

```text {.no-copy}
Bus 001 Device 103: ID 1a86:55d3 QinHeng Electronics USB Single Serial
crw-rw-rw- 1 root dialout … /dev/ttyACM0
…/by-id/usb-1a86_USB_Single_Serial_5CE5196417-if00 -> ../../ttyACM0

/dev/ttyACM0
------------
Hardware ID: USB VID:PID=1A86:55D3 SER=5CE5196417 LOCATION=1-1:1.0
Description: USB Single Serial
```

</div>

---

## 0.3 Chip and flash size

<div class="run" markdown>

```bash
~/.platformio/penv/bin/esptool --chip esp32s3 --port /dev/ttyACM0 chip-id
~/.platformio/penv/bin/esptool --chip esp32s3 --port /dev/ttyACM0 flash-id
```

```text {.no-copy}
Connected to ESP32-S3 on /dev/ttyACM0:
Chip type:          ESP32-S3 (QFN56) (revision v0.2)
Features:           … Embedded PSRAM 8MB (AP_3v3)
Crystal frequency:  40MHz
MAC:                28:84:85:8c:d0:40
…
Detected flash size: 16MB
Flash type set in eFuse: quad (4 data lines)
Flash voltage set by eFuse: 3.3V
```

</div>

| Item | This board (N16R8) |
|------|--------------------|
| Chip | ESP32-S3 rev v0.2 |
| Flash | 16MB |
| PSRAM | 8MB |
| MAC | `28:84:85:8C:D0:40` |

---

## 0.4 Erase flash

!!! warning
    `erase-flash` wipes everything on the chip. Only run it once `/dev/ttyACM0` is confirmed above.

<div class="run" markdown>

```bash
~/.platformio/penv/bin/esptool --chip esp32s3 --port /dev/ttyACM0 erase-flash
```

```text {.no-copy}
Erasing flash memory (this may take a while)...
Flash memory erased successfully in 2.3 seconds.
Hard resetting via RTS pin...
```

</div>

---

## Checkpoint

- [x] `dialout` + udev rules in place
- [x] `/dev/ttyACM0` appears (`1a86:55d3`)
- [x] chip-id / flash-id say ESP32-S3, 16MB flash, 8MB PSRAM
- [x] erase-flash finishes cleanly
- [x] [Lab log](../lab-log.md)

**Next:** [Phase 1 — Hello flash](01-hello-flash.md)
