# TODO

Tracks open bugs and deferred work that spans sessions / machines. Session-level micro-tasks live in conversation; ideas / wishlist items live elsewhere. Check items off (`[ ]` → `[x]`) when resolved, and update the *Notes* line with the resolving commit. Remove fully-resolved items once they've shipped to `main`.

## Open bugs

- [ ] **BLE: L75 keyboard doesn't reconnect after power-cycle.** After both the ESP32 and the L75 are turned off and back on, the ESP32 does not reconnect to the keyboard on its next boot — user must re-pair. Suspect the auto-reconnect-from-NVS-bonds path in `BleHidHost::begin()` isn't running, or the L75 isn't advertising in a way the scanner picks up. Reported 2026-05-13.
- [ ] **Speech: CALL SPGET truncates output at embedded 0x00 bytes.** `var_table` strings are null-terminated in the interpreter; real TI strings are length-prefixed. Some LPC byte sequences contain 0x00 internally, which silently truncates the SPGET result. Fix lives in the `TI_Extended_Basic_Interpreter` submodule, not this repo. Known limitation since stage 1 (commit d5b274f).

## Deferred work

- [ ] **Speech: audio output not yet validated.** Stage 2b code (commit `efcadf9`, on branch `speech-stage-2b`) compiles and flashes but has not been confirmed to produce intelligible speech. Side-by-side comparison with canonical TI Speech Synthesizer recordings needed before promoting `CALL SAY` / `CALL SPGET` from 🟡 to ✅ in [KEYWORDS.md](KEYWORDS.md). Merge `speech-stage-2b` → `main` only after validation passes.
- [ ] **Speech: add TMS5220_DEBUG instrumentation if audio quality is off.** Currently the synth has no debug logging (MAME's `LOGMASKED` calls were stripped during port). If validation reveals problems, add a `#define TMS5220_DEBUG 1` switch in [TI_Speech/tms5220.h](TI_Speech/tms5220.h) that gates `Serial.printf` calls for: parsed frame indices, lifecycle transitions (SPEN/TALK/TALKD), and periodic sample-time energy/pitch/lattice snapshots.
