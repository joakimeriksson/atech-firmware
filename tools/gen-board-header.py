#!/usr/bin/env python3
"""Render a board variant header from its YAML descriptor.

The GPIO for a port, and which of a double-width module's two ports supplies which
line, are facts the Atech SDK already knows: they live in its board catalog and in
codegen's `_module_context`, which applies the left-column 180° rotation rule. So
this renders each module's instance line with the SDK's own template and context
rather than restating any of it here.

    tools/gen-board-header.py boards/atech14-synth.yaml lib/atech_board/variants/atech14-synth.h

Needs the SDK (`make sdk`). The output is committed, so an ordinary build does not.
Anything the catalog cannot know — a module's measured physical layout — is kept in
a hand-written trailer below the marker and preserved across regeneration.
"""
import sys
from pathlib import Path

from atech.catalog import get_board
from atech.codegen import _module_context, _render
from atech.project import Project

MARKER = "// ---- measured, not generated ---------------------------------------------------"


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
    lines += ["", "#include \"modules/audio/speaker.h\"", "#include \"modules/display/st7735_tft.h\"",
              "#include \"modules/input/rotary_encoder.h\"", "#include \"modules/input/button.h\"",
              "#include \"modules/led/neopixel.h\"", ""]
    for m in project.modules:
        spec = specs[m.module_id]
        if spec.templates.globals:
            lines.append(_render(spec.templates.globals, _module_context(m, board, spec)).rstrip())
    lines.append("")

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
