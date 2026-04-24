# 1 — Hello World

First project. Initializes the ST7789 display and renders colored "Hello, World!" text. Goal: confirm the toolchain, board, and LCD all work end-to-end.

> New to the board or toolchain? Read [`../docs/setup.md`](../docs/setup.md) and [`../docs/board.md`](../docs/board.md) first.

## Pins used

Only the internal LCD pins (already wired on the board — see [`../docs/board.md`](../docs/board.md) for the full reference):

| Signal          | GPIO |
| --------------- | ---- |
| SPI MOSI        | 6    |
| SPI SCLK        | 7    |
| LCD CS          | 14   |
| LCD DC          | 15   |
| LCD RST         | 21   |
| LCD BL          | 22   |

## Run it

From this folder, in a PlatformIO Core CLI terminal:

```bash
pio run -t upload -t monitor
```

First build downloads the toolchain + Arduino-ESP32 core + GFX library (2–5 min, one-time). If upload fails, see [`../docs/troubleshooting.md`](../docs/troubleshooting.md#upload-fails-failed-to-connect--no-serial-data-received).

## Expected result

- Serial monitor (115200 baud) prints `ESP32-C6 Hello World booting...` then `tick` once per second.
- Screen shows: white **Hello,**, cyan **World!**, yellow **ESP32-C6**, magenta **Waveshare 1.47"**.

If colors, rotation, or edge artifacts are off, see the display section of [`../docs/troubleshooting.md`](../docs/troubleshooting.md).

## What the code does

`src/main.cpp` — ~35 lines:

1. Constructs an `Arduino_ESP32SPI` bus on the internal SPI pins.
2. Wraps it in an `Arduino_ST7789` driver configured for the 172×320 panel (with `34, 0, 34, 0` column/row offsets specific to this panel).
3. Drives the backlight pin HIGH.
4. Calls `gfx->begin()` and draws four lines of text with different sizes and colors.
5. Loops printing `tick` so you can verify serial is alive.
