# Third-party code

Vendored under `lib/`, each keeping its own terms.

## cRSID — `lib/crsid`, `lib/sidtunes`

A full C64 in software (6502 CPU, SID, CIA, VIC) by **Hermit (Mihaly Horvath)**, 2022.
WTFPL: "do what the fuck you want with the code, but please mention me as the original
author" — hence this file. `lib/crsid/host` and `cRSID_free` are the ESP32 port's additions.

The `sid-theremin` build plays cRSID's SID on its own, register by register, with no C64 behind
it (`src/sid-theremin/sid_chip.h`). `CRSID_SID_ONLY` in `C64/SID.c` is this repository's addition
for that: it takes out the SID code's one write into C64 memory. `CRSID_IRAM`, also this
repository's, puts the SID's two per-sample functions in the ESP32's internal RAM, where the flash
cache cannot evict them. Builds that define neither compile exactly as before.

`lib/sidtunes` embeds `.sid` music files for the player to run. The tunes are the work of
their original C64 composers and are included here only as demo material; they are not
covered by the cRSID licence.

## `lib/sid`

A small 3-voice SID-style synthesis core written for this firmware — the synth mode, as
opposed to cRSID's full chip emulation. BSD 3-Clause, same as the rest of this repository
(see `LICENSE`). Its envelope rate-counter periods are the 6581 hardware measurements
published by **Dag Lem** in reSID; the same sixteen values appear in cRSID and in every
accurate SID emulator, and are credited here as a courtesy.

## Atech SDK drivers — `lib/atech_*` (not in this repository)

Fetched by `make sync-sdk` from the installed open `atech` Python SDK and deliberately not
redistributed here. `lib/atech_board` and `lib/atech_glue` are this repository's own.
