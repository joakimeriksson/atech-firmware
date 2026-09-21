#!/usr/bin/env python3
"""Save what the sid-theremin build draws on its screen, as PNGs: the display cannot be seen from the host.

The build's `screen_dump` action renders a view into a canvas the size of the display and prints it
as 80 rows of RGB565 hex. This asks for each view and writes screen-<view>.png, scaled x4.

    tools/screen-shot.py                     play, knob and sound, into the current directory
    tools/screen-shot.py play,auto shots/    chosen views (auto = whichever is up now), into shots/

It shows what the firmware draws, not what the panel shows: a panel that has stopped taking frames
looks fine here. Needs pyserial, which the SDK's virtualenv has (`make sdk`).
"""
import sys, time, glob, json, struct, zlib, serial
def png(path, w, h, rows):
    raw = b"".join(b"\x00" + bytes(c for px in row for c in px) for row in rows)
    def chunk(t, d): return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d))
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(raw)) + chunk(b"IEND", b""))
views = sys.argv[1] if len(sys.argv) > 1 else "play,knob,sound"
out = sys.argv[2] if len(sys.argv) > 2 else "."
s = serial.Serial(glob.glob("/dev/cu.usbmodem*")[0], 115200, timeout=1.0)     # pyserial's default DTR/RTS: the board runs on
time.sleep(0.2)
for view in views.split(","):
    s.reset_input_buffer()
    s.write((json.dumps({"action": "screen_dump", "value": view}) + "\n").encode())
    rows, end = [], time.time() + 15
    while time.time() < end:
        line = s.readline().decode("ascii", "replace").strip()
        if line == "[screen] end": break
        if line.startswith("[screen] ") and len(line) == 9 + 640:
            px = [int(line[9 + 4 * i: 13 + 4 * i], 16) for i in range(160)]
            rows.append([((p >> 11) * 255 // 31, ((p >> 5) & 63) * 255 // 63, (p & 31) * 255 // 31) for p in px])
    print(view or "auto", len(rows), "rows")
    if len(rows) == 80:
        big = [[px for px in row for _ in range(4)] for row in rows for _ in range(4)]
        png(f"{out}/screen-{view or 'auto'}.png", 640, 320, big)
s.close()
