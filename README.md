# Atech firmware

Firmware for the Atech modular boards: an ESP32-S3 motherboard whose ports take snap-in modules
(display, knob, buttons, speaker, LED grids), programmed against the open `atech` SDK. One repo,
several builds, because the hardware is a kit and the interesting part is which modules are in
which ports.

A build is an **app** plus a **board**:

| Build | App | Board | What it is |
| --- | --- | --- | --- |
| `pocket-synth` | `src/pocket-synth` | atech14-synth | The Pocket Synth: a SID-chip synth and C64 tune player, TFT UI, knob and buttons, per-voice VU on the light grids |
| `grid-selftest` | `src/grid-selftest` | atech14-synth | Light Grid bring-up: walks the chain one LED at a time and names where each should appear |

```sh
make list                      # the builds this repo defines
make build APP=pocket-synth    # build one
make flash APP=grid-selftest   # build and flash a connected board
make monitor                   # serial console
make dist  APP=pocket-synth    # bootloader + partition table + app into dist/, with provenance
```

First time: `make sdk` installs the open SDK into a virtualenv, `make sync-sdk` copies the real
module drivers into `lib/atech_*`. Those drivers are not redistributed here.

## Layout

```
src/<app>/             one directory per app; main.cpp is what the Atech platform generates
lib/atech_board/       board builds: variants/<name>.h holds the module instances for a layout
lib/atech_glue/        the hosted-platform glue the SDK does not ship (AtechSerial, UI helpers)
lib/atech_*/           the real module drivers, synced from the SDK (gitignored)
lib/crsid, sid, sidtunes   third party, see THIRD-PARTY.md
idf-minimal/           an ESP-IDF sample: different framework, its own project
dist/<build>/          what `make dist` collects for flashing or for the emulator
```

**Adding an app**: a directory under `src/`, then an `[env:...]` block that filters to it.

**Adding a board**: a header under `lib/atech_board/variants/` with the module instances for that
port layout, a matching `ATECH_BOARD_<NAME>` branch in `atech_board.h`, and an environment that
defines the flag. Apps never name a board; they include `<atech_board.h>` and use the instances.

## The physical layout of a module is a board fact

A module's wiring is not always what its API suggests. The Light Grid V1.1 is addressed as a
nine-LED chain, but the chain is not row-major on the glass: with the module's ESP32 connector
edge down it runs down the left column, up the middle, then down the right. That was measured on
the board with the `grid-selftest` build, one LED at a time, and it lives in the board variant as
`GRID_GLASS` so any app can draw by glass geometry. The esp32sim emulator carries the same map
from the other side, so what the page draws is what the module shows.

## The emulator

These builds run unmodified in [esp32sim](https://github.com/joakimeriksson/esp32sim), which
models the motherboard and its modules. `make dist` produces exactly the three binaries the
emulator's demo manifests load, and the emulator's golden tests pin the console output, the audio
and the instruction count for the Pocket Synth build.
