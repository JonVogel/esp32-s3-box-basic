# CLAUDE.md

TI Extended BASIC simulator for the **ESP32-S3-Box-3**. The Arduino sketch in this repo is the Box-3 *host*; the BASIC language layer is a git submodule (`TI_Extended_Basic_Interpreter/`) shared with sibling builds.

## Architecture

The codebase has a deliberate split between **language** (portable, in submodules) and **hardware** (Box-3-specific, in the top-level sketch):

| Layer | Lives in | Concerns |
|---|---|---|
| Top-level sketch ([esp32-s3-box-basic.ino](esp32-s3-box-basic.ino)) | this repo | LCD init (LovyanGFX), audio (ES8311 codec), input plumbing, immediate-mode shell, pin assignments, boot screen |
| Audio engine ([audio.h](audio.h) / [audio.cpp](audio.cpp)) | this repo | SN76489-faithful tone/noise generator over I²S DMA, PolyBLEP anti-aliasing, tanh soft-limit |
| Language layer | [TI_Extended_Basic_Interpreter/](TI_Extended_Basic_Interpreter/) submodule | Tokenizer, expression parser, exec manager, var table, line editor, sprite data structures — header-only, host-agnostic |
| File I/O | [ESP32_File_Handling/](ESP32_File_Handling/) submodule | LittleFS + SD + V9T9 .DSK image routing; hardware-agnostic SD (host passes in `fs::FS&`) |
| BLE HID host | [BleHidHost/](BleHidHost/) submodule | NimBLE multi-peer keyboard/gamepad host with NVS bond persistence |
| Font / logo | [TI_Font/](TI_Font/) submodule | 8×8 TI ROM font + TI logo glyphs |

### Weak-symbol platform pattern

The interpreter consumes hardware via the hooks declared in [ti_platform.h](TI_Extended_Basic_Interpreter/ti_platform.h). Each hook has a **weak no-op default** in `ti_platform.cpp`; the host sketch overrides by defining a **strong** function with the same signature. The linker prefers strong over weak, so the override wins automatically — no callback registration needed.

```cpp
// In esp32-s3-box-basic.ino — overrides the weak default:
void tiPrintChar(char c) { /* draw to LCD */ }
```

Exceptions (still use explicit `setCallbacks()` on `TokenParser`): file I/O (FLASH/SD/DSK\<n\>), shell commands (NEW/RUN/SAVE/OLD/etc.), throttle, IMAGE lookup, DATA/READ. Those are language-layer concerns, not hardware.

### Sibling builds

The interpreter, file-I/O, BLE-host, and font submodules are shared with:
- `ti-extended-basic-esp32` — 800×480 RGB-panel reference build
- `ti-basic-otg` — ESP32-S3-USB-OTG variant
- `ti-scott-adams-esp32` — Scott Adams Adventure interpreter (shares file/BLE/font only)

When changing BASIC language behavior, the change usually belongs in the submodule (affects all hosts). Box-3-specific I/O — pins, codec, panel, partition — stays in this repo.

## Build & flash

`build.bat` wraps `arduino-cli`. Default port is `COM19`; override as the last arg.

```bat
build.bat                  REM compile + upload
build.bat compile          REM compile only
build.bat upload COM23     REM upload to a specific port
build.bat run              REM upload + monitor (skip compile)
build.bat all              REM compile + upload + monitor
build.bat monitor          REM serial monitor (115200)
build.bat killmonitor      REM free a stuck COM handle
```

**FQBN:** `esp32:esp32:esp32s3:PSRAM=opi,PartitionScheme=custom,FlashSize=16M,CDCOnBoot=cdc`
- `PSRAM=opi` — Box-3 has 16 MB octal PSRAM at 1.8 V; required for `heap_caps_malloc(MALLOC_CAP_SPIRAM)` (sprites use this).
- Custom partition table in [partitions.csv](partitions.csv) — 16 MB flash split into dual-OTA app slots (3.1 MB each) + SPIFFS (1.5 MB) + 64 KB coredump.
- `CDCOnBoot=cdc` — `Serial.print()` routes over USB-CDC; the Box-3 has no UART bridge.

`build.bat upload` auto-kills any running `arduino-cli.exe` or `serial-monitor.exe` first so the COM port is free.

### Box-3 upload quirk — auto-reset sometimes fails

The Box-3 uses native USB-Serial-JTAG, not a CP210x/CH340 bridge, so esptool's DTR/RTS auto-reset isn't always reliable. If an upload fails with `Wrong boot mode detected (0xNN)`, force download mode manually:

1. Hold **BOOT** (front button — also the pairing trigger).
2. Briefly press **RESET** (small button on the back).
3. Release RESET, then BOOT.
4. Retry the upload (the COM port may renumerate).

A varying boot-mode value across attempts (e.g. `0x2a` → `0xa`) means the strapping pins are being sampled while floating — confirms auto-reset isn't driving them, not a code bug.

## Hardware

ESP32-S3-Box-3 with WROOM-1U-N16R16V (16 MB flash, 16 MB octal PSRAM @ 1.8 V).

### Display
- 320×240 ST7789 over SPI, LovyanGFX with `LGFX_AUTODETECT`
- 32 cols × 24 rows of 8×8 chars, centered with 32px / 24px offsets, status bar below
- LCD pins: DC=4, CS=5, SCLK=7, MOSI=6, RST=48 (through inverting level shifter — drive HIGH for reset), BL=47

### Buttons
- `BTN_OK = 0` (BOOT) — also doubles as the BLE pairing trigger
- `BTN_MUTE = 1` (MUTE switch, active-low, gated through D-flip-flop)
- There is no third mainboard button; RESET is a recessed pinhole on the back

### SD card (SENSOR add-on)
- Native SD-MMC, 1-bit mode
- Pins: CLK=11, CMD=14, D0=9, PWR=43 (drive HIGH to power the slot)
- Per esp-bsp/esp-box-3 conventions

### Audio
- ES8311 codec over I²C + I²S DMA
- Recent commits added SN76489-style synthesis for `CALL SOUND`, TI beeps, HONK; PolyBLEP anti-aliasing; tanh soft-limiter; LFSR noise voice
- Power-amp enable on GPIO46

## Working from BASIC

[KEYWORDS.md](KEYWORDS.md) is the authoritative status table for which TI Extended BASIC keywords are implemented and tested. Treat it as the source of truth when adding or testing features.

[TEST_PROGRAMS.txt](TEST_PROGRAMS.txt) has BASIC programs used for verification.

### Non-TI immediate-mode commands (Box-3 extensions)

These are not real TI commands — they're host-specific additions parsed in the .ino's `processInput()`:

| Command | Purpose |
|---|---|
| `BLE` | List bonded BLE HID peers + pairing-mode state ([esp32-s3-box-basic.ino:2515](esp32-s3-box-basic.ino#L2515) → `BleHidHost::describePeers`) |
| `DIR` / `CAT` / `CATALOG` | List files on a device (defaults to FLASH) |
| `MOUNT` / `MOUNT DSK<n> <image>` | Show / mount V9T9 .DSK images |
| `UNMOUNT` | Detach a mounted disk |

`BLE` is **shell-only** — it won't parse inside a numbered program line. From a program, use `CALL PAIR` / `CALL UNPAIR` (added in commit c1f1a15, in [token_parser.h:2644](TI_Extended_Basic_Interpreter/token_parser.h#L2644)).

### BLE peer-name caveat

`describePeers` shows `(no name)` when `Peer::savedName` is empty. The name is captured from the **advertising packet** at scan time ([BleHidHost.h:492](BleHidHost/BleHidHost.h#L492)). Some HID peripherals put their name in the scan-response packet rather than the primary ad, or only advertise it during a full re-pair (long-hold the channel button), not a reconnect. If the name comes back empty, do `CALL UNPAIR` then put the peripheral into full pairing mode and `CALL PAIR` again.

## File layout (top-level)

```
esp32-s3-box-basic.ino       main sketch (~3000 lines: setup, loop, immediate-mode shell,
                             strong overrides for ti_platform hooks)
audio.h / audio.cpp          ES8311 + SN76489-style synth
ble_keyboard.h               BLE keyboard HID-report → CALL KEY plumbing
ble_gamepad.h                BLE gamepad HID-report → CALL JOYST plumbing
build.bat                    arduino-cli wrapper
partitions.csv               16 MB custom partition table
KEYWORDS.md                  TI Extended BASIC implementation status table
TEST_PROGRAMS.txt            BASIC test programs
char_editor.py               Host-side tool for designing CALL CHAR patterns
tilogo.json / tologo.json    Boot-screen logo data
SCH_ESP32-S3-BOX-3-*.pdf     Box-3 schematics (reference)

TI_Extended_Basic_Interpreter/   submodule — language layer
ESP32_File_Handling/             submodule — file/SD/DSK
BleHidHost/                      submodule — NimBLE HID host
TI_Font/                         submodule — 8×8 TI font + logo
```

## Conventions

- **No new files when an edit will do.** The sketch is large but coherent; prefer editing existing sections to creating new headers.
- **Box-3 specifics stay in the .ino.** If a change would affect every sibling build, it goes in the relevant submodule.
- **Weak hooks are how the language layer reaches hardware.** Don't add a new callback registration when a weak override will work.
- **`Serial.printf` is fine for diagnostics** — `CDCOnBoot=cdc` makes it land on the USB serial monitor.
- **PSRAM is available** via `heap_caps_malloc(size, MALLOC_CAP_SPIRAM)`. Use it for anything bigger than a few KB.
