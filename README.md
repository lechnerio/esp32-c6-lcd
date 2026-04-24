# ESP32-C6 LCD — Learning Projects

Incremental projects for the **Waveshare ESP32-C6 1.47" LCD** dev board — starting from a hello-world and adding a feature at a time. Language: C++ on the Arduino framework via PlatformIO (switching to ESP-IDF later for deeper features).

## Shared docs

- [`docs/board.md`](./docs/board.md) — board layout, buttons, pinout, internal wiring
- [`docs/setup.md`](./docs/setup.md) — PlatformIO install, open/build/upload, PATH setup
- [`docs/troubleshooting.md`](./docs/troubleshooting.md) — upload failures, IntelliSense, display issues

## Projects

| # | Project | Status | Focus |
| - | ------- | ------ | ----- |
| 1 | [`1-hello-world/`](./1-hello-world/) | 🟢 in progress | LCD init, draw colored text |

More coming. Each project folder has its own `README.md` with goal, pins used, and expected result — shared setup / board details live in [`docs/`](./docs/).

## Quick start

1. Read [`docs/setup.md`](./docs/setup.md) once, install VS Code + PlatformIO.
2. Open the **project folder** (e.g. `1-hello-world/`) directly in VS Code — **not** this repo root.
3. Build & upload. See each project's README for what to expect.
