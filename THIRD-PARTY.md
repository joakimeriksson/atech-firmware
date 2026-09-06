# Third-party code

Vendored under `lib/`, each keeping its own terms.

## cRSID — `lib/crsid`, `lib/sidtunes`

A full C64 in software (6502 CPU, SID, CIA, VIC) by **Hermit (Mihaly Horvath)**, 2022.
WTFPL: "do what the fuck you want with the code, but please mention me as the original
author" — hence this file. `lib/crsid/host` and `cRSID_free` are the ESP32 port's additions.

`lib/sidtunes` embeds `.sid` music files for the player to run. The tunes are the work of
their original C64 composers and are included here only as demo material; they are not
covered by the cRSID licence.

## `lib/sid`

A small 3-voice SID-style synthesis core written for this firmware — the synth mode, as
opposed to cRSID's full chip emulation. Same licence as this repository.

## Atech SDK drivers — `lib/atech_*` (not in this repository)

Fetched by `make sync-sdk` from the installed open `atech` Python SDK and deliberately not
redistributed here. `lib/atech_board` and `lib/atech_glue` are this repository's own.
