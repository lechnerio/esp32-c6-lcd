# Setup — PlatformIO on Windows

## Why PlatformIO (and not Arduino IDE or ESP-IDF directly)

PlatformIO manages the toolchain, framework, and libraries per project in `platformio.ini`. It works from either VS Code or the terminal, and it supports all the frameworks this board can run (Arduino, ESP-IDF, etc.). Arduino IDE works too, but per-project config is harder. ESP-IDF directly is more powerful but steeper — we'll use it in later projects.

All projects in this repo use the [`pioarduino`](https://github.com/pioarduino/platform-espressif32) community fork of the PlatformIO espressif32 platform, because Arduino-ESP32 **3.x** is required for ESP32-C6 and the stock platform hasn't updated yet.

## Prerequisites

1. **VS Code** — <https://code.visualstudio.com/>
2. **PlatformIO IDE extension** — in VS Code, Extensions (`Ctrl+Shift+X`) → search _PlatformIO IDE_ → Install.
3. **USB-C data cable** (not a power-only cable — if Windows doesn't chime when you plug in, swap it).

No USB driver install needed: ESP32-C6 has a native USB-Serial/JTAG. Windows 11 recognizes it automatically as `USB JTAG/serial debug unit (COMx)` in Device Manager → _Ports (COM & LPT)_.

## Open a project

VS Code → **File → Open Folder** → pick the project folder (e.g. `1-hello-world/`). PlatformIO detects `platformio.ini` and starts indexing. First time per project: it downloads the toolchain + Arduino-ESP32 core + libraries (2–5 min, cached afterwards).

> Open the **project folder directly**, not the parent `esp32-c6-lcd/` folder. PlatformIO keys off `platformio.ini` sitting at the workspace root.

## Build / upload — GUI

The PlatformIO status bar at the bottom of VS Code has:

- ✓ Build
- → Upload
- 🗑 Clean
- 🔌 Serial Monitor

## Build / upload — terminal (CLI)

Open the **PlatformIO Core CLI** terminal: VS Code command palette (`Ctrl+Shift+P`) → _PlatformIO: New Terminal_. This terminal has `pio` on `PATH`. Then:

```bash
pio run                      # build
pio run -t upload            # build + upload
pio device monitor           # open serial monitor (115200)
pio run -t upload -t monitor # upload then monitor in one go
pio run -t clean             # clean build artifacts
pio device list              # list COM ports
```

Force a specific port if auto-detect picks wrong: `pio run -t upload --upload-port COM5`.

## Using `pio` outside VS Code

To use `pio` from any terminal (regular PowerShell / Windows Terminal), append `%USERPROFILE%\.platformio\penv\Scripts` to your Windows user `PATH`. Run this in PowerShell — it reads the current user PATH and only appends if the entry isn't already there (safe to re-run, no truncation):

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

Close and reopen the terminal afterwards, then verify with `pio --version`.

> Avoid `setx PATH "..."` — it truncates at 1024 chars and can corrupt long PATHs. The `[Environment]` API used above has no length limit.
