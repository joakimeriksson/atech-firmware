#!/usr/bin/env python3
"""Render a board variant header from its YAML descriptor.

The GPIO for a port, and which of a double-width module's two ports supplies which
line, are facts the Atech SDK already knows: they live in its board catalog and in
codegen's `_module_context`, which applies the left-column 180° rotation rule. So
this renders each module's instance line with the SDK's own template and context
rather than restating any of it here.

    tools/gen-board-header.py boards/atech14-synth.yaml lib/atech_board/variants/atech14-synth.h

I2C modules get two things more. The SDK's templates put every I2C module on `Wire`, each on
its own port's lines, so two of them would re-pin one bus under each other; the ESP32-S3 has two
controllers, so the second I2C module a board names is moved to `Wire1` (a third is an error).
And starting the bus needs the port's pins, which for some modules appear only in the SDK's
setup template, so that template is rendered too, as `<instance>_setup()` — after a bus recovery,
because a warm reset of the ESP32 leaves the modules powered and one can be left holding SDA.

A rotary encoder's CLK and DT pins are given names as well, `<instance>_pin_clk` and `_pin_dt`:
the SDK driver keeps them private, and an app that decodes the turn itself (see
modules/input/quadrature_knob.h in the glue) needs them without typing a GPIO number.

Needs the SDK (`make sdk`). The output is committed, so an ordinary build does not.
Anything the catalog cannot know — a module's measured physical layout — is kept in
a hand-written trailer below the marker and preserved across regeneration.
"""
import re
import sys
from pathlib import Path

from atech.catalog import get_board
from atech.codegen import _module_context, _render
from atech.project import Project

MARKER = "// ---- measured, not generated ---------------------------------------------------"

# The glue header each module is reached through, in the order the headers list them.
GLUE_HEADERS = {
    "speaker": "modules/audio/speaker.h",
    "st7735_tft": "modules/display/st7735_tft.h",
    "rotary_encoder": "modules/input/rotary_encoder.h",
    "button": "modules/input/button.h",
    "neopixel": "modules/led/neopixel.h",
    "icm40608": "modules/sensor/icm40608.h",
    "distance_sensor": "modules/sensor/vl53l5cx.h",
}
I2C_BUSES = ["Wire", "Wire1"]   # the ESP32-S3's two controllers, handed out in board-file order


def main(descriptor: Path, out: Path) -> int:
    project = Project.load(descriptor)
    project.validate()
    board = get_board(project.board)
    specs = project.module_specs()
    mcu = board.microcontroller
    if not isinstance(mcu, dict):   # older catalogs expose it as an object
        mcu = {k: getattr(mcu, k) for k in ('variant', 'flash_size_kb', 'ram_size_kb')}

    lines = [
        f"// Generated from {descriptor.name} by tools/gen-board-header.py — do not edit above the",
        "// marker; run `make board-headers` instead. Everything below the marker is hand-written.",
        "//",
        f"// Board: {board.name} ({mcu['variant']}, "
        f"{mcu['flash_size_kb'] // 1024} MB flash, {mcu['ram_size_kb']} KB RAM)",
        "//",
    ]
    for m in project.modules:
        ports = ", ".join(p.replace("port_", "") for p in m.ports)
        pins = " ".join(
            f"{p.replace('port_', '')}:{'/'.join(str(x.gpio) for x in board.port(p).pins)}"
            for p in m.ports
        )
        lines.append(f"//   port {ports:<5} {specs[m.module_id].name:<34} GPIO {pins}")
    present = {m.module_id for m in project.modules}
    missing = present - GLUE_HEADERS.keys()
    if missing:
        sys.exit(f"no glue header known for: {', '.join(sorted(missing))} (add it to GLUE_HEADERS)")
    lines += [""] + [f'#include "{h}"' for mid, h in GLUE_HEADERS.items() if mid in present]
    if any(specs[m.module_id].interface == "i2c" for m in project.modules):
        lines.append('#include "modules/shared/i2c_recover.h"')
    lines.append("")
    setups, buses = [], iter(I2C_BUSES)
    for m in project.modules:
        spec = specs[m.module_id]
        context = _module_context(m, board, spec)
        on_bus = lambda code: code
        if spec.interface == "i2c":
            bus = next(buses, None)
            if bus is None:
                sys.exit(f"{m.instance}: more I2C modules than the {len(I2C_BUSES)} buses the chip has")
            on_bus = lambda code, bus=bus: re.sub(r"\bWire\b", bus, code)
            body = on_bus(_render(spec.templates.setup, context)).rstrip().replace("\n", "\n    ")
            setups += [f"/// Start {m.instance} on {bus}: free the bus, then what the SDK's setup template does.",
                       f"static inline void {m.instance}_setup() {{",
                       f"    atechI2cRecover({context['pin_a']}, {context['pin_b']});", f"    {body}", "}"]
        if spec.templates.globals:
            lines.append(on_bus(_render(spec.templates.globals, context)).rstrip())
        if m.module_id == "rotary_encoder":      # the SDK's template: the primary port supplies CLK and DT
            lines.append(f"static const int {m.instance}_pin_clk = {context['pin_a']}, "
                         f"{m.instance}_pin_dt = {context['pin_b']};")
    lines += ([""] + setups if setups else []) + [""]

    trailer = ""
    if out.exists():
        existing = out.read_text()
        if MARKER in existing:
            trailer = existing[existing.index(MARKER):]
    if not trailer:
        trailer = MARKER + "\n// (nothing yet)\n"
    out.write_text("\n".join(lines) + trailer)
    print(f"wrote {out} ({len(project.modules)} modules)")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    sys.exit(main(Path(sys.argv[1]), Path(sys.argv[2])))
