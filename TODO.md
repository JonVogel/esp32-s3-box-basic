# TODO

Tracks open bugs and deferred work that spans sessions / machines. Session-level micro-tasks live in conversation; ideas / wishlist items live elsewhere. Check items off (`[ ]` → `[x]`) when resolved, and update the *Notes* line with the resolving commit. Remove fully-resolved items once they've shipped to `main`.

## Active branches

- `web-file-manager` — current, has the web file manager scaffolding + WiFi/HTTP server + /api/files endpoint + SD-init reorder + TI-faithful filename quoting. Not yet merged to main.
- `speech-stage-2b` — has the TMS5220 synth + volume controls + BLE scan policy. Already merged to main at commit `7af0c4d`; branch stays for the open O-block garble bug.
- `ble-scan-policy` — already merged to main at commit `604db0d`. Branch can be deleted.

## Submodule branches

- `TI_Extended_Basic_Interpreter` on `volume-commands` (HEAD `ae67462`) — adds CALL VOLUME/SPVOL, CALL WIFI hooks, and TI-faithful filename quoting.
- `ESP32_File_Handling` on `program-io` (HEAD `3f326de`) — adds `progio::parseTarget` + `progio::resolveExistingPath` + V9T9 binary `saveProgramBytes` / `loadProgramBytes` + reserved-name validation.
- `BleHidHost` on `main` at `b58376d` — `hasBondedPeers`, `stopScan`, `requestBackgroundScan` hooks.

## Open bugs

- [ ] **BLE: L75 keyboard doesn't reconnect after power-cycle.** After both the ESP32 and the L75 are turned off and back on, the ESP32 does not reconnect to the keyboard on its next boot — user must re-pair. Suspect the auto-reconnect-from-NVS-bonds path in `BleHidHost::begin()` isn't running, or the L75 isn't advertising in a way the scanner picks up. Reported 2026-05-13. Worth re-testing after the BLE two-mode scan policy (commit `e7a41fc`) — that work may have fixed it incidentally.
- [ ] **Speech: CALL SPGET truncates output at embedded 0x00 bytes.** `var_table` strings are null-terminated in the interpreter; real TI strings are length-prefixed. Some LPC byte sequences contain 0x00 internally, which silently truncates the SPGET result. Fix lives in the `TI_Extended_Basic_Interpreter` submodule, not this repo. Known limitation since stage 1 (commit d5b274f).
- [ ] **Speech: vocab entries with no silence preamble sound garbled.** Words whose ROM data starts directly on a voiced frame (E≥1, P≠0) sound unintelligible / noisy. Confirmed on "O" and "OH" (which share the same 61-byte data at offset 0x4B7D — TI aliased them). Multi-syllable words and any word whose ROM data starts with silence frames (HELLO does) sound fine. Three hypotheses ruled out 2026-05-15: (1) STOP-frame K-target interpolating toward stale extreme K values — zeroing them on STOP did nothing. (2) Lattice needs warmup from zero state — injecting 6 silence frames as preamble made it *worse*, not better. (3) The audible garble was end-of-word — user reports it's actually throughout, "more like random noise than speech." Real root cause unknown. Path forward: build a host-side MAME-equivalent reference (write a small C++ driver harness around MAME's original `tms5220.cpp` + `tms5110r.hxx` at `c:\tmp\mame_tms5220\`, feed it O's 61 bytes, render to WAV, diff sample-by-sample against our port's output) to find where we numerically diverge from the reference implementation. Frame parsing is confirmed byte-deterministic and matches expected bit layout (debug output captured in the conversation log). `TMS5220_DEBUG` flag in [TI_Speech/tms5220.h](TI_Speech/tms5220.h) gates the parsed-frame logging that helped reach this point.

## Deferred work

- [ ] **Adopt `progio::` library in the sibling `ti-extended-basic-esp32` project.** That project still carries inline copies of `cmdSave` / `cmdOld` / `cmdMerge` / `cmdDelete` from before the library existed. Replacing them with calls into `progio::parseTarget` / `progio::saveProgramBytes` / etc. removes ~250 lines of drift and means future bug fixes only happen once. Compile-only test from this machine since the 8048S043C hardware lives on the home setup.
- [ ] **`cmdDelete` device routing.** The host's `cmdDelete` was not migrated to `progio::parseTarget` in the file-routing batch (sibling uses `fio::parseSpec` which is a different design). Currently `DELETE` only works on FLASH and probably misroutes prefixed forms the same way SAVE used to. Should be a clean port now that the library API is settled.
- [ ] **Build out the web file manager UI past listing.** Currently `/api/files` and the HTML index page list files. Next steps: download (GET file as text/binary), upload (POST multipart, lands on the chosen device, blocked during BASIC RUN), delete, rename. Each is small on its own; sequence is roughly: download → upload → delete → rename.
- [ ] **Validate speech audio output against canonical TI recordings.** Most multi-syllable vocab words sound right but no formal A/B test has been done. Promotes `CALL SAY` / `CALL SPGET` in [KEYWORDS.md](KEYWORDS.md) from 🟡 to ✅ once it passes.

## Recently resolved

- [x] **SD card invisible despite mount succeeding.** Three layered bugs — initDisplay() killed SDMMC's VFS mount, openNextFile loops leaked File handles past the 5-slot pool, and unquoted filenames got dot-stripped by the tokenizer. Fix landed 2026-05-16 in commit `5d886da` (host) plus submodule bumps. SAVE/OLD/DIR on both FLASH and SDCARD now work end-to-end with quoted filenames.
- [x] **Speech mixer ceiling 2.0× → 5.0×.** Resolved 2026-05-15 in commit `c9c47fd` on `speech-stage-2b`. The synth was producing samples in the ±5000-16000 range; the 5× ceiling reliably saturates the int16 output. Confirmed via diagnostic instrumentation that briefly lived in audio.cpp.
- [x] **Web file manager scaffolding + WiFi.** Resolved 2026-05-15 in commit `8babb97` on `web-file-manager`. WiFi connects via CALL WIFI("ssid","pass"), HTTP server on :80, /api/status returns JSON.
