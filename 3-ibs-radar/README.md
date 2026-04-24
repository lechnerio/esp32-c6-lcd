# 3 — iBS Radar

Continuously tracks Ingics **iBS03 / iBS05** BLE beacons and renders a live smoothed distance list. Each beacon keeps a 10-sample rolling window; the distance estimate comes from the **median** of that window so single outlier readings (e.g. from rotating the beacon 90°) don't throw the number around.

> New to the board or toolchain? Read [`../docs/setup.md`](../docs/setup.md) and [`../docs/board.md`](../docs/board.md) first.

## How it's different from `2-bluetooth-radar`

| Aspect | `2-bluetooth-radar` | `3-ibs-radar` |
| ------ | ------------------- | ------------- |
| Scope | All BLE devices | Only iBS03 / iBS05 |
| Scan model | 4 × 1 s chunks, results read at end | Continuous, **per-packet callback** |
| Storage | One scan's results | Rolling window per beacon |
| Distance | From single RSSI reading | From **median of last 10 readings** |
| Freshness | Fixed per scan | **Color-coded per row** (green / yellow / orange / red) |
| Persistence | None | None — RAM only, cleared on reboot |

## UI

- **Header**: `iBS radar  N tracked  M fresh  [iBS]`
- **Row**: `iBS0X  AABBCC    -NN dBm  ~NN cm/m`
- **Row color**: beacon freshness
  - **Green**: seen in last 2 s
  - **Yellow**: 2–5 s
  - **Orange**: 5–10 s
  - **Red** + `(lost)` in distance column: 10–30 s
  - Evicted from list after 30 s
- **Footer**: `Live  N tracked  (BOOT: reset)`

## Beacon settings (iBS05 configured via Ingics app)

- **Advertising interval**: 500 ms (→ 2 Hz sample rate per beacon)
- **TX power**: Mid (≈ 0 dBm)
- **PHY**: Legacy

With those settings, 10 samples ≈ 5 s of history. Distance estimate stabilizes within ~2 s of the beacon coming into range.

## Parameters you can tweak

All in `src/main.cpp`:

| Constant | Default | Effect |
| -------- | ------- | ------ |
| `WINDOW_SIZE` | 10 | Samples per median. Larger → smoother but laggier |
| `RSSI_AT_1M` | −59 dBm | Reference TX power for distance math. Calibrate per beacon for accuracy |
| `PATH_LOSS_EXP` | 2.5 | Environment exponent: 2.0 free space, 2.5–3.5 indoor |
| `FRESH_MS` / `STALE_MS` / `LOST_MS` / `EVICT_MS` | 2 / 5 / 10 / 30 s | Freshness thresholds |

## Planned future views (not implemented)

The single current view `[iBS]` is the first of a planned cycle. Machinery for `BOOT = cycle view` is intentionally left minimal right now (BOOT = reset). When more views are added, BOOT will cycle through:

- `[closest]` — big-numbers focus on the nearest beacon (distance rendered huge like the stats screen in project 2)
- `[spread]` — sort by RSSI variance over the window; surfaces beacons whose signal is *changing* (i.e. person is moving)
- `[radar]` — polar/radar visualization with each beacon drawn at a radius proportional to distance
- `[calibrate]` — capture a new `RSSI_AT_1M` per-beacon via a "place at 1 m, press BOOT" flow

## Architecture notes

- **Threading**: BLE callbacks fire on the BLE task; the main loop renders from the same `beacons` map. A FreeRTOS mutex (`beaconsMutex`) serializes access. Critical sections are microseconds long.
- **Why `setAdvertisedDeviceCallbacks(&cb, wantDuplicates=true)`**: without duplicates, the BLE stack dedupes same-MAC packets within a scan window — which means only one sample per beacon per scan chunk. We need every packet.
- **Why `scanner->start(1, true)` in a loop** instead of `start(0)`: `start(1, true)` is continuous from the radio's perspective (is_continue=true), but yields control to the main loop every second so we can render and poll the button. Net scan duty cycle stays near 100%.
- **Memory**: 10 beacons × 10 samples × 1 byte + per-beacon overhead (~80 bytes each) ≈ 1 KB total. Trivial.

## Run

```bash
pio run -t upload -t monitor
```

Bring an iBS03 / iBS05 near the board. It should appear within a couple of seconds with green freshness. Walk away — watch the distance climb, then the color fade green → yellow → orange. Cover the beacon with your body briefly to see the median resist spike noise that a single-reading distance would overreact to.
