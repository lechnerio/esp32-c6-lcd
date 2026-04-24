# 1 — Hello World

First project on the **Waveshare ESP32-C6 1.47" LCD** board. Displays colored "Hello, World!" text on the built-in 172×320 ST7789 screen using the Arduino framework + `GFX_Library_for_Arduino`.

## Hardware

- **Board:** Waveshare ESP32-C6 1.47" LCD (ESP32-C6, Wi-Fi 6, 8 MB flash)
- **Display:** 1.47" IPS, 172×320, ST7789 controller, SPI
- **Pin mapping used in this project:**

| Signal         | GPIO |
| -------------- | ---- |
| MOSI           | 6    |
| SCLK           | 7    |
| CS             | 14   |
| DC             | 15   |
| RST            | 21   |
| BL (backlight) | 22   |

## Board layout

Held with the **screen facing you** and the **USB-C cable entering at the bottom**:

```
        ┌───────────────────────┐
        │  1.47" LCD            │
        │  172 × 320            │
        │                       │
        │                       │
        │                       │
        │                       │
        │                       │
        ├───────────────────────┤
 BOOT ──┤ ●   ┌───────────┐   ● ├── RST
 GPIO9  │     │ SD (back) │     │  (EN / hardware reset)
        │     ├───────────┤     │
        │     │   USB-C   │     │
        └─────┴───────────┴─────┘
                  USB-C
```

- **Left button → `BOOT`** (GPIO 9). Hold during reset to enter the USB download/flash mode.
- **Right button → `RST`** (EN pin, hardware reset — not a GPIO).
- **USB-C** on the bottom edge.
- **Onboard RGB LED (WS2812)** on **GPIO 8**.
- **microSD / TF slot** on the side (unused in this project).

Flash recovery sequence if upload fails: hold **BOOT** (left) → tap **RST** (right) → release **BOOT** → re-run upload.

## Framework choice

ESP32-C6 supports several frameworks — Arduino, ESP-IDF (Espressif native), MicroPython, Rust, Zephyr. This project uses **Arduino** for simplicity. Arduino on ESP32 is itself a thin layer over ESP-IDF, so you can mix them later.

Because Arduino-ESP32 3.x is required for the C6, the PlatformIO `platform` points at the [`pioarduino`](https://github.com/pioarduino/platform-espressif32) community fork rather than the stock one.

## Prerequisites

1. **VS Code** — https://code.visualstudio.com/
2. **PlatformIO IDE extension** — in VS Code, Extensions (`Ctrl+Shift+X`) → search _PlatformIO IDE_ → Install.
3. **USB-C data cable** (not a power-only cable — if Windows doesn't chime when you plug in, swap it).

No USB driver install needed: ESP32-C6 has a native USB-Serial/JTAG, and Windows 11 recognizes it out of the box. Check Device Manager → _Ports (COM & LPT)_ → you should see `USB JTAG/serial debug unit (COMx)`.

## Open the project

VS Code → **File → Open Folder** → pick this `1-hello-world` folder. PlatformIO detects `platformio.ini` and starts indexing. First time: it downloads the toolchain + Arduino-ESP32 core + GFX library (2–5 min, one-time).

## Build / upload — GUI

The PlatformIO status bar at the bottom of VS Code has:

- ✓ Build
- → Upload
- 🗑 Clean
- 🔌 Serial Monitor

## Build / upload — terminal (CLI)

Open the **PlatformIO Core CLI** terminal: VS Code command palette (`Ctrl+Shift+P`) → _PlatformIO: New Terminal_. This terminal has `pio` on `PATH`. Then:

```bash
cd 1-hello-world/
pio run                      # build
pio run -t upload            # build + upload
pio device monitor           # open serial monitor (115200)
pio run -t upload -t monitor # upload then monitor in one go
pio run -t clean             # clean build artifacts
pio device list              # list COM ports
```

Force a specific port if auto-detect picks wrong: `pio run -t upload --upload-port COM5`.

If you want `pio` in a regular terminal, append `%USERPROFILE%\.platformio\penv\Scripts` to your Windows user `PATH`. Run this in PowerShell — it reads the current user PATH and only appends if the entry isn't already there (safe to re-run, no truncation):

```powershell
$add = "$env:USERPROFILE\.platformio\penv\Scripts"
$cur = [Environment]::GetEnvironmentVariable('Path','User')
if (($cur -split ';') -notcontains $add) {
  [Environment]::SetEnvironmentVariable('Path', ($cur.TrimEnd(';') + ';' + $add), 'User')
  Write-Host "Added: $add"
} else {
  Write-Host "Already on PATH"
}
```

Close and reopen the terminal afterwards, then verify with `pio --version`. Avoid `setx PATH "..."` — it truncates at 1024 chars and can corrupt long PATHs.

## Upload troubleshooting

If upload fails with _"Failed to connect"_ or _"No serial data received"_:

1. Hold the **BOOT** button on the board
2. Tap **RESET** (sometimes labeled EN)
3. Release BOOT
4. Re-run upload

This forces the chip into download mode. After the first successful upload, auto-reset usually works from then on.

## IntelliSense: `could not open source file "Arduino.h"`

VS Code's C/C++ extension shows squiggles like _"#include errors detected. Please update your includePath"_ **before the first successful build** — the include paths are only generated once PlatformIO compiles the project.

Fixes, in order:

1. **Run a build first** (`pio run` or the ✓ button). PlatformIO writes `.vscode/c_cpp_properties.json` afterward and the squiggles disappear.
2. If still broken after a build: command palette → **PlatformIO: Rebuild IntelliSense Index**.
3. Make sure you opened the **`1-hello-world/` folder** directly in VS Code, not the parent `esp32-c6-lcd/`. PlatformIO keys off `platformio.ini` sitting at the workspace root.
4. Reload window: command palette → _Developer: Reload Window_.

## Expected result

- Serial monitor (115200 baud) prints `ESP32-C6 Hello World booting...` then `tick` once per second.
- Screen shows white _"Hello,"_, cyan _"World!"_, yellow _"ESP32-C6"_, magenta _"Waveshare 1.47\""_.

If colors look wrong (e.g. red appears blue), flip the BGR flag — the `true` argument in the `Arduino_ST7789(...)` constructor in `src/main.cpp`. If orientation is wrong, change the rotation argument (`0` → `1`/`2`/`3`). If you see a colored strip at the edges, the column/row offsets (`34, 0, 34, 0`) need tweaking for your panel.
