# Atech firmware

Firmware for the Atech modular boards: an ESP32-S3 motherboard whose ports take snap-in modules
(display, knob, buttons, speaker, LED grids, sensors), programmed against the open `atech` SDK. One repo,
several builds, because the hardware is a kit and the interesting part is which modules are in
which ports.

A build is an **app** plus a **board**:

| Build | App | Board | What it is |
| --- | --- | --- | --- |
| `pocket-synth` | `src/pocket-synth` | atech14-synth | The Pocket Synth: a SID-chip synth and C64 tune player, TFT UI, knob and buttons, per-voice VU on the light grids |
| `grid-selftest` | `src/grid-selftest` | atech14-synth | Light Grid bring-up: walks the chain one LED at a time and names where each should appear |
| `sid-theremin` | `src/sid-theremin` | atech14-theremin | The SID Theremin: a hand over the distance sensor (port 13) is the pitch, the tilt of the board (IMU, port 14) the filter and the volume. Four modes: theremin, harp (the hand plucks the scale's strings), frets (one string, the pitch the fret below the hand), arpeggio; eleven instruments built the way C64 composers built theirs, a Synthex-style laser harp among them, on cRSID's SID (6581 or 8580). Also a Bluetooth MIDI port, "SID Theremin": keys play it in every mode (`make keys` is the MacBook's keyboard). Plays with either sensor missing |

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
boards/<name>.yaml     which module sits in which port — the source of truth for wiring
lib/atech_board/       board builds: variants/<name>.h, rendered from boards/<name>.yaml;
                       measured/ holds what was measured on a board and is shared by variants
lib/atech_glue/        the hosted-platform glue the SDK does not ship (AtechSerial, UI helpers)
lib/atech_*/           the real module drivers, synced from the SDK (gitignored)
lib/crsid, sidtunes       third party, see THIRD-PARTY.md
lib/sid/               the synth-mode SID core, this repository's own
idf-minimal/           an ESP-IDF sample: different framework, its own project
dist/<build>/          what `make dist` collects for flashing or for the emulator
```

**Adding an app**: a directory under `src/`, then an `[env:...]` block that filters to it.

**Adding a board**: a descriptor under `boards/` naming the modules and their ports, then
`make board-headers` to render `lib/atech_board/variants/<name>.h`, a matching
`ATECH_BOARD_<NAME>` branch in `atech_board.h`, and an environment that defines the flag. Apps
never name a board; they include `<atech_board.h>` and use the instances.

GPIO numbers are never hand-typed. `make board-headers` resolves them through the Atech SDK's own
board catalog and codegen, so the rule that a module snaps into the left column rotated 180° — and
therefore which port supplies which line of a double-width module — stays the SDK's to define. The
rendered headers are committed, so an ordinary build needs no SDK; only regenerating does.
Anything the catalog cannot know, such as a module's measured physical layout, is hand-written
below the `measured, not generated` marker and preserved across regeneration.

## The physical layout of a module is a board fact

A module's wiring is not always what its API suggests. The Light Grid V1.1 is addressed as a
nine-LED chain, but the chain is not row-major on the glass: with the module's ESP32 connector edge
down it runs down the left column, up the middle, then down the right. That was measured on the
board with the `grid-selftest` build, one LED at a time, and it reaches an app through the board
variant as `GRID_GLASS` so any app can draw by glass geometry. Both boards have their grids in ports
7 and 11, so the map is one file under `lib/atech_board/measured/` that both variants include. The
esp32sim emulator carries the same map from the other side, so what the page draws is what the
module shows.

## The emulator

These builds run unmodified in [esp32sim](https://github.com/joakimeriksson/esp32sim), which
models the motherboard and its modules. `make dist` produces exactly the three binaries the
emulator's demo manifests load, and the emulator's golden tests pin the console output, the audio
and the instruction count for the Pocket Synth build.

The `sid-theremin` build is the exception to "unmodified" in one respect: the emulator models
neither of its two sensors nor a Bluetooth controller. Without the sensors it runs in its
no-sensor fallback; starting Bluetooth would hang it, so it is run with the start stubbed out,
`--elf .pio/build/sid-theremin/firmware.elf --stub bleMidiBegin=0`, and keys are sent as the
`note_on` / `note_off` / `ble_packet` serial actions, which go through the same code.

## License

BSD 3-Clause, see [LICENSE](LICENSE). Vendored code under `lib/` keeps its own terms, listed in
[THIRD-PARTY.md](THIRD-PARTY.md).
