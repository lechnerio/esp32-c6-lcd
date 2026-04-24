# 2 — Bluetooth Radar

Continuously scans for nearby Bluetooth Low Energy devices and renders them as a live list on the LCD in landscape, sorted by signal strength.

> New to the board or toolchain? Read [`../docs/setup.md`](../docs/setup.md) and [`../docs/board.md`](../docs/board.md) first.

## What you'll see

- Top bar (cyan): `BLE radar  scan #N  showing X/Y`
- One row per device, sorted strongest first:
  - **White** — device advertises a friendly name (`iPhone`, `MX Keys`, `AirTag`...)
  - **Grey** — no name advertised → shows the BLE MAC address instead
  - RSSI color-coded: **green** ≥ −60 dBm, **yellow** ≥ −75 dBm, **orange** ≥ −90 dBm, **red** below.
- Bottom bar (blue) flashes during each 4 s scan.

## Why BLE and not classic Bluetooth

The ESP32-C6 supports **BLE only** — no classic Bluetooth. That turns out fine for a "radar", because almost every modern wireless thing (phones, AirPods, AirTags, smartwatches, fitness trackers, BLE beacons, IoT sensors) broadcasts BLE advertising packets. Phones with BLE MAC randomization still show up, just without a stable identifier.

## Pins used

Only the internal LCD pins (already wired — see [`../docs/board.md`](../docs/board.md)). No external wiring.

## Run it

```bash
pio run -t upload -t monitor
```

First build will be slow again because the BLE stack is added — expect a few minutes. After that, incremental builds are fast.

## Things to try

- Put a phone next to the board → its RSSI should shoot into green (< −60 dBm).
- Walk across the room → watch it degrade through yellow/orange.
- Flip a BLE-capable device in/out of airplane mode → appears/disappears between scans.

## Notes

- Uses `board_build.partitions = huge_app.csv` — BLE bloats the firmware past the default 1.3 MB app partition, so we allocate a bigger app slot. Data partition is correspondingly smaller, which is fine since we don't use filesystem storage yet.
- The scanner is set to **active** (`setActiveScan(true)`), which also requests scan responses — gets you friendly names for more devices at the cost of slightly higher power draw.
- The 4 s scan window is a balance between "responsive UI" and "catching devices that advertise slowly" (phones can be >1 s between advertisements when idle).
