# Troubleshooting

## Upload fails: "Failed to connect" / "No serial data received"

Force the chip into USB download mode:

1. Hold the **BOOT** button (left, from the front with USB at bottom — see [`board.md`](./board.md))
2. Tap **RST** (right)
3. Release **BOOT**
4. Re-run upload

After the first successful upload the auto-reset usually works from then on.

If it still fails:

- Try a different **USB-C cable** — many cheap cables are power-only and will power the board (so the screen lights up) without enumerating a COM port.
- Check Device Manager → _Ports (COM & LPT)_ — do you see `USB JTAG/serial debug unit (COMx)`? If not, the cable/port is the problem, not the firmware.
- Force the port explicitly: `pio run -t upload --upload-port COM5`.

## IntelliSense: `could not open source file "Arduino.h"`

VS Code's C/C++ extension shows squiggles like _"#include errors detected. Please update your includePath"_ **before the first successful build** — the include paths are only generated once PlatformIO compiles the project.

Fixes, in order:

1. **Run a build first** (`pio run` or the ✓ button). PlatformIO writes `.vscode/c_cpp_properties.json` afterwards and the squiggles disappear.
2. If still broken after a build: command palette → **PlatformIO: Rebuild IntelliSense Index**.
3. Make sure you opened the **project folder** directly in VS Code, not the parent `esp32-c6-lcd/`. PlatformIO keys off `platformio.ini` at the workspace root.
4. Reload window: command palette → _Developer: Reload Window_.

## Screen stays blank after upload

- **Backlight off** — check the `LCD_BL` pin in the code is driven HIGH.
- **Wrong SPI pins** — verify MOSI=6, SCLK=7, CS=14, DC=15, RST=21, BL=22 match the code.
- **Display init missing** — serial should print from `setup()`; if not, the board is crashing before init (check Serial Monitor).

## Colors look wrong (red appears blue, etc.)

Flip the BGR flag in the `Arduino_ST7789(...)` constructor — the `true` argument. Swap it to `false` (or vice versa).

## Text is rotated wrong

Change the rotation argument (3rd parameter to `Arduino_ST7789`): `0`, `1`, `2`, or `3` — one of them will be right-way-up for your orientation.

## Colored strip at the edge of the display

The column/row offsets in the `Arduino_ST7789(...)` constructor (`34, 0, 34, 0` for the 172 × 320 panel) are off. Tweak the first and third numbers until the strip disappears. Different panel batches occasionally need different offsets.
