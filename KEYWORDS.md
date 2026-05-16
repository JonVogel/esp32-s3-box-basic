# TI Extended BASIC Keyword Status

Status legend:
- **Impl** — implementation in place in the simulator
- **Test** — verified working on device via [TEST_PROGRAMS.txt](TEST_PROGRAMS.txt) or recent commit
- *(blank)* — not yet implemented / not yet tested

---

## Immediate-mode commands

| Keyword        | Impl | Test | Notes                                          |
|----------------|:----:|:----:|------------------------------------------------|
| NEW            |  ✅  |  ✅  | Clears program and screen                      |
| RUN            |  ✅  |  ✅  |                                                |
| LIST           |  ✅  |  ✅  | Range supported: `LIST n`, `LIST n-`, `LIST -m`, `LIST n-m` |
| OLD            |  ✅  |  ✅  | Loads from LittleFS or `DSK<n>.NAME` (PROGRAM)  |
| SAVE           |  ✅  |  ✅  | Saves to LittleFS or `DSK<n>.NAME` (PROGRAM)    |
| MERGE          |  ✅  |  ✅  | Fold file into current program; collisions win |
| BYE            |  ✅  |  ✅  | Restarts the ESP32                             |
| DIR            |  ✅  |  ✅  | Lists files on a device (default FLASH)        |
| SIZE           |  ✅  |      | Prints free heap + estimated program space     |
| NUMBER / NUM   |  ✅  |  ✅  | Auto line-number input mode                    |
| RESEQUENCE/RES |  ✅  |  ✅  | Renumbers lines + GOTO/GOSUB/THEN/ELSE targets |
| BREAK          |  ✅  |  ✅  | Set breakpoint line list                       |
| UNBREAK        |  ✅  |  ✅  | Clear breakpoints                              |
| TRACE          |  ✅  |  ✅  | Prints `<lineN>` before each line              |
| UNTRACE        |  ✅  |  ✅  |                                                |
| CON/CONTINUE   |  ✅  |  ✅  | Resumes after STOP / breakpoint                |
| DELETE (file)  |  ✅  |  ✅  | `DELETE "FILE"` removes a file                 |

## Non-TI immediate-mode commands (ESP32-S3-Box-3 extensions)

| Command        | Impl | Test | Notes                                            |
|----------------|:----:|:----:|--------------------------------------------------|
| BLE            |  ✅  |  ✅  | Lists bonded BLE HID peers + pairing-mode state  |
| CAT / CATALOG  |  ✅  |  ✅  | Alias for DIR                                    |
| MOUNT          |  ✅  |  ✅  | Bare = list mounts; `MOUNT DSK<n> <image>` to attach |
| UNMOUNT        |  ✅  |  ✅  | `UNMOUNT DSK<n>`                                 |
| NEWDISK        |  ✅  |  ✅  | `NEWDISK <device.name> ["VOLNAME"] [SSSD\|DSSD\|DSDD]` — create a blank V9T9 .DSK image. Spec like `FLASH.WORK.DSK` or `SDCARD.GAMES.DSK`. VOLNAME quoted, 1-10 chars, defaults to `NEWDISK`. Size defaults to SSSD (360 sec / 90 KB); DSSD = 720 / 180 KB; DSDD = 1440 / 360 KB. Separators can be spaces or commas |
| COPY           |  ✅  |  ✅  | `COPY <src> <dst>` across FLASH / SDCARD / DSK<n> |

## Program control flow

| Keyword        | Impl | Test | Notes                                      |
|----------------|:----:|:----:|--------------------------------------------|
| IF / THEN      |  ✅  |  ✅  |                                            |
| ELSE           |  ✅  |  ✅  |                                            |
| FOR / TO       |  ✅  |  ✅  |                                            |
| STEP           |  ✅  |  ✅  |                                            |
| NEXT           |  ✅  |  ✅  |                                            |
| GOTO / GO TO   |  ✅  |  ✅  |                                            |
| GOSUB          |  ✅  |  ✅  |                                            |
| RETURN         |  ✅  |  ✅  |                                            |
| ON ... GOTO    |  ✅  |  ✅  |                                            |
| ON ... GOSUB   |  ✅  |  ✅  |                                            |
| END            |  ✅  |  ✅  |                                            |
| STOP           |  ✅  |  ✅  |                                            |
| REM            |  ✅  |  ✅  |                                            |
| `!` (comment)  |  ✅  |  ✅  | Extended BASIC tail-comment                |
| ON BREAK       |  ✅  |  ✅  | STOP / NEXT modes                          |
| ON ERROR       |  ✅  |  ✅  | `<line>` / STOP / NEXT; disarms inside handler |
| ON WARNING     |  ✅  |  ✅  | PRINT / STOP / NEXT modes                  |

## Variables, assignment, I/O

| Keyword        | Impl | Test | Notes                                        |
|----------------|:----:|:----:|----------------------------------------------|
| LET (implicit) |  ✅  |  ✅  | Bare `VAR = expr` supported                  |
| DIM            |  ✅  |  ✅  | 1D/2D/3D arrays, numeric and string          |
| OPTION BASE    |  ✅  |  ✅  | 0 or 1                                       |
| PRINT          |  ✅  |  ✅  | `;` `,` `TAB()` supported                    |
| INPUT          |  ✅  |  ✅  |                                              |
| LINPUT         |  ✅  |  ✅  |                                              |
| DISPLAY AT     |  ✅  |  ✅  |                                              |
| ACCEPT AT      |  ✅  |  ✅  |                                              |
| READ           |  ✅  |  ✅  |                                              |
| DATA           |  ✅  |  ✅  |                                              |
| RESTORE        |  ✅  |  ✅  |                                              |
| RANDOMIZE      |  ✅  |  ✅  |                                              |
| DEF            |  ✅  |  ✅  | Numeric (`DEF FNX(X)=...`) and string (`DEF FNX$(X$)=...`) |
| SUB            |  ✅  |  ✅  | Pass-by-value-result for bare-var args       |
| SUBEND         |  ✅  |  ✅  |                                              |
| SUBEXIT        |  ✅  |  ✅  |                                              |
| OPEN           |  ✅  |  ✅  | INPUT/OUTPUT/APPEND/UPDATE × DISPLAY/INTERNAL × SEQUENTIAL/RELATIVE × FIXED N |
| CLOSE          |  ✅  |  ✅  |                                              |
| PRINT #        |  ✅  |  ✅  | `REC k` for relative records on FIXED files  |
| INPUT #        |  ✅  |  ✅  | `REC k` supported; comma-separated parse     |
| LINPUT #       |  ✅  |  ✅  | Whole-line read; no comma split              |
| RESTORE #      |  ✅  |  ✅  | Rewind a file unit                           |
| IMAGE          |  ✅  |  ✅  | Format string for PRINT USING                |
| PRINT USING    |  ✅  |  ✅  | `PRINT USING <imageLine>:` or inline `"fmt":` |
| DISPLAY USING  |  ✅  |  ✅  | `DISPLAY AT(r,c) USING <imageLine>:`         |

## Operators

| Operator       | Impl | Test | Notes                                      |
|----------------|:----:|:----:|--------------------------------------------|
| `+ - * / ^`    |  ✅  |  ✅  |                                            |
| `=`, `<>`      |  ✅  |  ✅  |                                            |
| `<`, `<=`      |  ✅  |  ✅  |                                            |
| `>`, `>=`      |  ✅  |  ✅  |                                            |
| `AND`, `OR`    |  ✅  |  ✅  |                                            |
| `NOT`          |  ✅  |  ✅  |                                            |
| `&` concat     |  ✅  |  ✅  |                                            |
| `XOR`          |  ✅  |  ✅  | Extended BASIC; precedence NOT > XOR > AND > OR |

## String functions

| Function       | Impl | Test | Notes                                      |
|----------------|:----:|:----:|--------------------------------------------|
| ASC            |  ✅  |  ✅  |                                            |
| CHR$           |  ✅  |  ✅  |                                            |
| LEN            |  ✅  |  ✅  |                                            |
| POS            |  ✅  |  ✅  |                                            |
| SEG$           |  ✅  |  ✅  |                                            |
| STR$           |  ✅  |  ✅  |                                            |
| VAL            |  ✅  |  ✅  |                                            |
| RPT$           |  ✅  |  ✅  |                                            |

## Numeric functions

| Function       | Impl | Test | Notes                                      |
|----------------|:----:|:----:|--------------------------------------------|
| ABS            |  ✅  |  ✅  |                                            |
| ATN            |  ✅  |  ✅  |                                            |
| COS            |  ✅  |  ✅  |                                            |
| SIN            |  ✅  |  ✅  |                                            |
| TAN            |  ✅  |  ✅  |                                            |
| EXP            |  ✅  |  ✅  |                                            |
| LOG            |  ✅  |  ✅  |                                            |
| INT            |  ✅  |  ✅  |                                            |
| SGN            |  ✅  |  ✅  |                                            |
| SQR            |  ✅  |  ✅  |                                            |
| RND            |  ✅  |  ✅  | Zero-arg form works without parens         |
| PI             |  ✅  |  ✅  | Zero-arg constant                          |
| MAX            |  ✅  |  ✅  | Extended BASIC                             |
| MIN            |  ✅  |  ✅  | Extended BASIC                             |
| EOF            |  ✅  |  ✅  | `EOF(unit)` for INPUT loops                |

## CALL subprograms — graphics

| Subprogram     | Impl | Test | Notes                                      |
|----------------|:----:|:----:|--------------------------------------------|
| CALL CLEAR     |  ✅  |  ✅  |                                            |
| CALL SCREEN    |  ✅  |  ✅  |                                            |
| CALL COLOR     |  ✅  |  ✅  |                                            |
| CALL CHAR      |  ✅  |  ✅  | 8-byte hex pattern                         |
| CALL HCHAR     |  ✅  |  ✅  |                                            |
| CALL VCHAR     |  ✅  |  ✅  |                                            |
| CALL GCHAR     |  ✅  |  ✅  |                                            |
| CALL CHARSET   |  ✅  |  ✅  | Resets chars 32-127 to ROM defaults; optional `("PC")` / `("TI")` switches font ROM (V9T9 authentic), persisted to NVS |
| CALL CHARPAT   |  ✅  |  ✅  | `CALL CHARPAT(code, A$)` — 16-char hex     |

## CALL subprograms — sprites (Extended BASIC)

| Subprogram     | Impl | Test | Notes                                      |
|----------------|:----:|:----:|--------------------------------------------|
| CALL SPRITE    |  ✅  |  ✅  | `(#n, char, color, row, col [, rVel, cVel])` |
| CALL MOTION    |  ✅  |  ✅  | Velocity in 1/8-pixel/frame units; 60 Hz tick |
| CALL POSITION  |  ✅  |  ✅  | Reads snapshot coords (coherent per frame) |
| CALL LOCATE    |  ✅  |  ✅  | Relocate sprite                            |
| CALL COINC     |  ✅  |  ✅  | Sprite-pair, sprite-vs-point, and `ALL` modes; respects MAGNIFY footprint |
| CALL DISTANCE  |  ✅  |  ✅  | Sprite-to-sprite and sprite-to-point       |
| CALL DELSPRITE |  ✅  |  ✅  | Single sprite or `ALL`                     |
| CALL MAGNIFY   |  ✅  |  ✅  | Modes 1-4 (8×8, 8×8×2, 16×16, 16×16×2)     |
| CALL PATTERN   |  ✅  |  ✅  | Change sprite pattern                      |

## CALL subprograms — I/O & system

| Subprogram     | Impl | Test | Notes                                      |
|----------------|:----:|:----:|--------------------------------------------|
| CALL KEY       |  ✅  |  ✅  | Mode 0; other modes treated same           |
| CALL VERSION   |  ✅  |  ✅  | Returns 110                                |
| CALL JOYST     |  ✅  |  ✅  | BLE gamepad (unit 1); -4/0/+4 per axis     |
| CALL SOUND     |  ✅  |  ✅  | SN76489-style; 3 tone + 1 noise; PolyBLEP + tanh limit |
| CALL ERR       |  ✅  |  ✅  | Line is real; severity/code/err-num always 0 (stub) |
| CALL SAY       |  ✅  |  🟡  | TMS5220 LPC-10 synth (MAME port) + spchrom.bin vocab lookup + greedy multi-word matching + UHOH on miss. Some vocab entries with no silence preamble still sound garbled (see TODO.md) |
| CALL SPGET     |  ✅  |  🟡  | Binary-tree lookup in spchrom.bin returns the raw LPC byte string. Known limit: output truncates at the first embedded 0x00 byte (var_table strings are null-terminated; real TI strings are length-prefixed) |
| CALL INIT      |      |      | Memory Expansion init — no 9900 CPU emulation |
| CALL LINK      |      |      | Assembly linkage — out of scope            |
| CALL LOAD      |      |      | Memory poke — out of scope                 |
| CALL PEEK      |      |      | Memory read — out of scope                 |

## CALL subprograms — non-TI ESP32 extensions

| Subprogram     | Impl | Test | Notes                                            |
|----------------|:----:|:----:|--------------------------------------------------|
| CALL TIMER     |  ✅  |      | `CALL TIMER(N)` writes `millis()` into N         |
| CALL DELAY     |  ✅  |      | `CALL DELAY(ms)` blocks but yields to BLE/sprite tasks |
| CALL SPEED     |  ✅  |  ✅  | µs/statement throttle: 0 native, ~285 XB, ~666 TI BASIC |
| CALL PAIR      |  ✅  |  ✅  | Opens 30s BLE-HID pairing window                 |
| CALL UNPAIR    |  ✅  |  ✅  | Forgets every bonded BLE peer                    |
| CALL WIFI      |  ✅  |  ✅  | `CALL WIFI` prints status; `CALL WIFI(ssid$,pass$)` stores creds + connects; `CALL WIFI("forget"\|"on"\|"off")`. Creds persisted to NVS; host provides WiFi STA + HTTP file-manager on `web-file-manager` branch |
| CALL VOLUME    |  🟡  |      | Master output level 0..30 (SN76489 scale: 0 loudest, 30 silent). Host strong overrides + NVS persistence implemented in `audio.cpp`; interpreter parser/dispatch not yet wired |
| CALL SPVOL     |  🟡  |      | Speech-only level 0..30, separate from CALL VOLUME (lets users boost speech vs SN76489 voices). Same host/interpreter status as CALL VOLUME |
| CALL GETVOLUME |  🟡  |      | Read current master volume into a numeric variable. Host done, interpreter pending |
| CALL GETSPVOL  |  🟡  |      | Read current speech volume. Host done, interpreter pending |

---

## Storage / device layer

| Feature                                | Impl | Test | Notes                                |
|----------------------------------------|:----:|:----:|--------------------------------------|
| FLASH device (LittleFS)                |  ✅  |  ✅  | `FLASH.NAME`                         |
| SDCARD device (raw SD root)            |  ✅  |  ✅  | `SDCARD.NAME`; SD-MMC 1-bit on Box-3 |
| DSK\<n\> mount table                   |  ✅  |  ✅  | `MOUNT DSK1 image`; persists across reboot |
| V9T9 .dsk read (Phase 1)               |  ✅  |  ✅  | DIS/VAR, SSSD/DSSD/DSDD              |
| V9T9 .dsk write (Phase 2)              |  ✅  |  ✅  | NEWDISK + OPEN…OUTPUT for DIS/VAR    |
| PROGRAM format on .dsk (Phase 3)       |  ✅  |  ✅  | SAVE / OLD work with `DSK<n>.NAME`   |
| INTERNAL file type                     |  ⚠️  |  ✅  | Parsed; stored as DISPLAY (no radix-100). Not interoperable with real TI |
| FIXED N records                        |  ✅  |  ✅  | N-byte fixed records, no LF terminator |
| RELATIVE access (`REC k`)              |  ✅  |  ✅  | Random-access read/write             |

## Editor / environment features

| Feature                            | Impl | Test | Notes                                   |
|------------------------------------|:----:|:----:|-----------------------------------------|
| Line editor — DEL / INS / ERASE    |  ✅  |  ✅  | FCTN+1/2/3                              |
| Line editor — arrows (L/R/U/D)     |  ✅  |  ✅  | FCTN+S/D/E/X                            |
| REDO recall (FCTN+8)               |  ✅  |  ✅  | Recalls last submitted line             |
| Line-number recall (`<N>` + UP/DN) |  ✅  |  ✅  |                                         |
| UP/DOWN browse in EDIT mode        |  ✅  |  ✅  | Commits current line before navigating  |
| CLEAR breaks running program       |  ✅  |  ✅  | FCTN+4 or Ctrl+C or ESC                 |
| PC-style backspace                 |  ✅  |  ✅  | 0x7F deletes char to the left           |
| BLE keyboard input                 |  ✅  |  ✅  | F12 or BOOT button = pairing            |
| BLE gamepad input                  |  ✅  |  ✅  | Routes to CALL JOYST (unit 1)           |
| Serial paste                       |  ✅  |  ✅  | 16 KB decoupled buffer                  |
| Title / menu screen                |  ✅  |  ✅  | Color stripes, 3×3 logo, © char         |
| Error format (blank + msg + BEL)   |  ✅  |  ✅  | TI-style "* MSG IN nn"                  |
| Audio output                       |  ✅  |  ✅  | ES8311 codec over I²S DMA               |
