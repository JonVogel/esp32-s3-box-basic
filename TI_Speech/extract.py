#!/usr/bin/env python3
"""
Generate speech_rom.h from a user-supplied spchrom.bin.

The TI-99/4A Speech Synthesizer cartridge ROM is copyrighted TI material
and is NOT distributed with this project. The user must obtain their own
copy (e.g., from a MESS BIOS bundle) and run this script to bake it
into the firmware build as PROGMEM data.

Usage:
    python extract.py <path/to/spchrom.bin>

Writes speech_rom.h alongside this script. The .ino includes that header
when present; without it, CALL SAY / CALL SPGET fall back to stubs.
"""

import sys
import hashlib
from pathlib import Path

EXPECTED_SIZE = 32768
EXPECTED_SHA256 = "1A7481F3E7E2D464772B540F9B9E4D529DC61DF372F181ADF7432F0DF9876C16"


def main() -> int:
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <path/to/spchrom.bin>", file=sys.stderr)
        return 1

    rom_path = Path(sys.argv[1])
    if not rom_path.is_file():
        print(f"Not a file: {rom_path}", file=sys.stderr)
        return 1

    data = rom_path.read_bytes()
    if len(data) != EXPECTED_SIZE:
        print(f"WARNING: expected {EXPECTED_SIZE} bytes, got {len(data)}",
              file=sys.stderr)

    sha = hashlib.sha256(data).hexdigest().upper()
    if sha != EXPECTED_SHA256:
        print(f"WARNING: SHA256 mismatch (got {sha}, expected {EXPECTED_SHA256})",
              file=sys.stderr)
        print("Proceeding anyway — the format may still parse correctly.",
              file=sys.stderr)

    out_path = Path(__file__).parent / "speech_rom.h"
    with out_path.open("w", encoding="utf-8", newline="\n") as f:
        f.write(
            "// Generated from TI-99/4A Speech Synthesizer ROM (spchrom.bin).\n"
            f"// {len(data)} bytes. Copyrighted TI material — DO NOT commit this file.\n"
            "// Regenerate with TI_Speech/extract.py from a user-supplied spchrom.bin.\n"
            "//\n"
            f"// Source SHA256: {sha}\n"
            "//\n"
            "// Format (per Marc Rousseau ti99sim dumpspch.cpp):\n"
            "//   Offset 0:  0xAA marker\n"
            "//   Offset 1:  Root vocabulary node (binary tree)\n"
            "//   Node:      [1B nameLen][N bytes ASCII name][2B prev BE]\n"
            "//              [2B next BE][1B pad][2B LPC offset BE][1B LPC len]\n"
            "#pragma once\n"
            "\n"
            "#include <stdint.h>\n"
            "\n"
            f"#define SPEECH_ROM_SIZE {len(data)}\n"
            "\n"
            "static const uint8_t speechRom[SPEECH_ROM_SIZE] PROGMEM = {\n"
        )
        for i in range(0, len(data), 16):
            chunk = data[i:i + 16]
            f.write("  " + ",".join(f"0x{b:02X}" for b in chunk) + ",\n")
        f.write("};\n")

    print(f"Wrote {out_path} ({out_path.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
