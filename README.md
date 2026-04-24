# ESP32-C6 LCD — Learning Projects

Incremental projects for the **Waveshare ESP32-C6 1.47" LCD** dev board — starting from a hello-world and adding a feature at a time. Language: C++ on the Arduino framework via PlatformIO (switching to ESP-IDF later for deeper features).

## Shared docs

- [`docs/board.md`](./docs/board.md) — board layout, buttons, pinout, internal wiring
- [`docs/setup.md`](./docs/setup.md) — PlatformIO install, open/build/upload, PATH setup
- [`docs/troubleshooting.md`](./docs/troubleshooting.md) — upload failures, IntelliSense, display issues

## Projects

| # | Project | Status | Focus |
| - | ------- | ------ | ----- |
| 1 | [`1-hello-world/`](./1-hello-world/) | ✅ done | LCD init, draw colored text |
| 2 | [`2-bluetooth-radar/`](./2-bluetooth-radar/) | ✅ done | BLE scan, sorted by RSSI, distance estimate, landscape UI, BOOT-button filter |

More coming. Each project folder has its own `README.md` with goal, pins used, and expected result — shared setup / board details live in [`docs/`](./docs/).

## Quick start

1. Read [`docs/setup.md`](./docs/setup.md) once, install VS Code + PlatformIO.
2. Open the **project folder** (e.g. `1-hello-world/`) directly in VS Code — **not** this repo root.
3. Build & upload. See each project's README for what to expect.

## Common commands

From within a project folder (e.g. `2-bluetooth-radar/`), in a PlatformIO Core CLI terminal:

```bash
pio run                         # build only
pio run -t upload               # build + flash
pio run -t upload -t monitor    # build + flash + open serial monitor (115200)
pio device monitor              # just open serial monitor
pio device list                 # list COM ports
pio run -t clean                # clean build artifacts
```

Full setup (install, PATH, troubleshooting): [`docs/setup.md`](./docs/setup.md).
