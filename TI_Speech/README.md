# TI_Speech

Build-time bake of the TI-99/4A Speech Synthesizer cartridge ROM (`spchrom.bin`) into a `PROGMEM` C array so `CALL SAY` / `CALL SPGET` can do real vocabulary lookups against authentic TI speech data.

## Why this exists

`spchrom.bin` is copyrighted TI material. We don't redistribute it. The user supplies their own copy (e.g., from a MESS BIOS bundle), runs `extract.py`, and gets `speech_rom.h` written here. The `.ino` includes that header when it's present and falls back to stubs when it isn't — so a fresh clone still builds, it just doesn't have real speech.

## Generate

```sh
python extract.py path/to/spchrom.bin
```

The expected ROM is 32 768 bytes, SHA-256 `1A7481F3E7E2D464772B540F9B9E4D529DC61DF372F181ADF7432F0DF9876C16` (the canonical TI Speech Synthesizer cartridge dump shipped with MESS / MAME). Other dumps may work; the script warns on mismatch but proceeds.

## ROM format

Documented inline in the generated header and in `extract.py`. Briefly:

```
Offset 0:  0xAA marker (skip)
Offset 1:  Root vocabulary node (binary tree)
Node:      [1B nameLen][N bytes ASCII name][2B prev BE]
           [2B next BE][1B pad][2B LPC offset BE][1B LPC len]
```

Source: Marc Rousseau's `ti99sim/src/util/dumpspch.cpp`, function `ReadNode`.

The tree contains roughly 373 vocabulary words (digits, letters, color names, numbers, common verbs, etc.) — every word `CALL SAY` recognizes on a real TI.

## Files

| File | Purpose |
|---|---|
| `extract.py` | Reads a user `spchrom.bin`, writes `speech_rom.h` |
| `speech_rom.h` | Generated. Not committed — see `.gitignore` |
| `README.md` | This file |
