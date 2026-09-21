#!/usr/bin/env python3
"""Show what the distance sensor sees: the 8x8 frames the sid-theremin build prints over serial.

The build's `grid_log` action prints every new frame as `[grid] <ms> d0 .. d63`, distances in
mm, in the order the driver keeps them. This turns the log on, draws each frame as a heat map
in the terminal, near = bright, and turns it off again on the way out.

    tools/grid-view.py                       live view, ctrl-C to leave
    tools/grid-view.py --max 1600            scale the colours to 1.6 m instead of the hand range
    tools/grid-view.py --record hands.grid   also keep the raw lines
    tools/grid-view.py --replay hands.grid   draw a recording instead of a board, at its own pace
    tools/grid-view.py --frames 3 --plain    print three frames one after the other and stop

The rows and columns are the driver's, not the room's: which way they face in port 13 is what
this is for finding out. Below the map is the hand as the firmware works it out — the four
nearest zones in range, the nearest dropped — so the map and the pitch can be compared.

Needs pyserial, which the SDK's virtualenv has (`make sdk`). The port is opened the way pyserial
does by default, DTR and RTS both asserted, which leaves the board running — measured: its uptime
counts on across opens. Opening with both lines low does reset it, because pyserial drops DTR
before RTS and RTS alone is the ESP32's reset. Only one program can hold the port: not together
with `make monitor`.
"""
import argparse
import glob
import sys
import time

HAND_FAR_MM, HAND_MIN_ZONES = 450, 3            # as in src/sid-theremin/main.cpp
GRID_LOG_ON, GRID_LOG_OFF = b'{"action":"grid_log","value":"on"}\n', b'{"action":"grid_log","value":"off"}\n'
RAMP = [16, 17, 18, 19, 20, 26, 32, 38, 44, 80, 116, 152, 188, 224, 220, 214, 208, 202, 196]   # far .. near


def cell(mm, max_mm, plain):
    if plain:
        return f"{mm:5d}"
    if mm <= 0 or mm > max_mm:
        return "\x1b[48;5;233m\x1b[38;5;240m" + f"{min(mm, 9999):5d}" + "\x1b[0m"
    level = int((1.0 - mm / max_mm) * (len(RAMP) - 1) + 0.5)
    ink = 16 if level > 8 else 250
    return f"\x1b[48;5;{RAMP[level]}m\x1b[38;5;{ink}m{mm:5d}\x1b[0m"


def hand(grid):
    near = sorted(d for d in grid if 20 <= d <= HAND_FAR_MM)
    if len(near) < HAND_MIN_ZONES:
        return None, len(near)
    picked = near[1:4] if len(near) >= 4 else near[1:3]
    return sum(picked) // len(picked), len(near)


def draw(ms, grid, args, fps, count):
    out = [] if args.plain else ["\x1b[H"]
    out.append(f"frame {count}  t={ms / 1000:8.2f} s  {fps:4.1f} frames/s   colours: 0..{args.max} mm\x1b[K")
    out.append("      " + "".join(f"  c{c}  " for c in range(8)) + "\x1b[K")
    for r in range(8):
        out.append(f"  r{r}  " + " ".join(cell(grid[r * 8 + c], args.max, args.plain) for c in range(8)) + "\x1b[K")
    mm, zones = hand(grid)
    nearest = min((d for d in grid if d > 0), default=0)
    where = grid.index(nearest) if nearest else 0
    out.append(f"nearest zone {nearest} mm at r{where // 8} c{where % 8};  "
               + (f"hand {mm} mm from {zones} zones in range" if mm else f"no hand ({zones} zones within {HAND_FAR_MM} mm)")
               + "\x1b[K")
    text = "\n".join(out) + "\n"
    sys.stdout.write(text.replace("\x1b[K", "") if args.plain else text)
    sys.stdout.flush()


def frames_from(lines):
    for line in lines:
        if isinstance(line, bytes):
            line = line.decode("utf-8", "replace")
        parts = line.split()
        if len(parts) == 66 and parts[0] == "[grid]":
            try:
                yield line, int(parts[1]), [int(p) for p in parts[2:]]
            except ValueError:
                pass                                # a line torn by another print


def serial_lines(port, idle_s=8.0):
    """Lines from the board, until it has sent no frame for a while. The log is asked for again
    every two seconds until frames come: a board that is still starting misses the first ask."""
    last_frame = last_ask = time.time()
    port.write(GRID_LOG_ON)
    while time.time() - last_frame < idle_s:
        line = port.readline()
        if line.startswith(b"[grid]"):
            last_frame = time.time()
        elif time.time() - last_frame > 2.0 and time.time() - last_ask > 2.0:
            port.write(GRID_LOG_ON)
            last_ask = time.time()
        if line:
            yield line


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--port", default=None, help="serial port (default: the first /dev/cu.usbmodem*)")
    ap.add_argument("--max", type=int, default=600, help="distance in mm that maps to the darkest colour (default 600)")
    ap.add_argument("--frames", type=int, default=0, help="stop after this many frames")
    ap.add_argument("--plain", action="store_true", help="no colours or cursor movement: frames one after the other")
    ap.add_argument("--record", metavar="FILE", help="also append the raw [grid] lines to FILE")
    ap.add_argument("--replay", metavar="FILE", help="draw a recording instead of reading a board")
    args = ap.parse_args()

    port = None
    if args.replay:
        source = open(args.replay)
    else:
        import serial
        name = args.port or next(iter(sorted(glob.glob("/dev/cu.usbmodem*"))), None)
        if not name:
            sys.exit("no board on USB (looked for /dev/cu.usbmodem*)")
        port = serial.Serial(name, 115200, timeout=0.5)     # pyserial's default DTR/RTS: the board runs on
        source = serial_lines(port)

    record = open(args.record, "a") if args.record else None
    if not args.plain:
        sys.stdout.write("\x1b[2J\x1b[?25l")
    count, fps, last_ms, started = 0, 0.0, None, time.time()
    try:
        for line, ms, grid in frames_from(source):
            if last_ms is not None and ms > last_ms:
                fps = 0.8 * fps + 0.2 * 1000.0 / (ms - last_ms) if fps else 1000.0 / (ms - last_ms)
                if args.replay:
                    time.sleep(min((ms - last_ms) / 1000.0, 0.5))
            last_ms = ms
            count += 1
            if record:
                record.write(line if line.endswith("\n") else line + "\n")
            draw(ms, grid, args, fps, count)
            if args.frames and count >= args.frames:
                break
        if not count:
            print("no [grid] lines: is the sid-theremin build on the board, with the distance sensor found?")
    except KeyboardInterrupt:
        pass
    finally:
        if port:
            port.write(GRID_LOG_OFF)
            port.flush()
            port.close()
        if record:
            record.close()
        if not args.plain:
            sys.stdout.write("\x1b[?25h\n")
        if count:
            print(f"{count} frames in {time.time() - started:.1f} s")


if __name__ == "__main__":
    main()
