#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.10"
# dependencies = ["bleak"]
# ///
"""Play the sid-theremin build from the MacBook's keyboard, over Bluetooth MIDI.

The build advertises as "SID Theremin". This finds it, connects, and turns typing into notes, laid
out like a piano the way music programs do it:

     W E   T Y U   O P          the black keys
    A S D F G H J K L ; '       the white keys, from C
    Z / X                       an octave down / up
    1 2 3 ... 9 0 -             the instrument (a MIDI program change, 1 = the first)
    left / right arrow          pitch bend down / up while held
    up / down arrow             the mod wheel (vibrato), a step at a time
    [ / ]                       softer / harder: the velocity the keys play at (a computer keyboard has none)
    space                       let every key go

    tools/midi-keys.py                     (or: make keys)
    tools/midi-keys.py --play "60 64 67"   no window: play these notes one after the other and leave
    tools/midi-keys.py --seconds 5         leave by itself after a while

A window, because a terminal does not say when a key is let go. It needs no MIDI setup on the Mac: it
writes BLE MIDI packets itself. Only one host can be connected to the board at a time, so not
together with a connection made in Audio MIDI Setup — which is the other way to play it, from any
MIDI program or keyboard: MIDI Studio > Bluetooth > SID Theremin > Connect.

Run through uv, which fetches bleak: the first line does that when the file is run directly. The
first use asks for Bluetooth permission for the terminal program.
"""
import argparse
import asyncio
import queue
import threading
import time

from bleak import BleakClient, BleakScanner

NAME = "SID Theremin"
SERVICE = "03b80e5a-ede8-4b33-a751-6ce34ec4c700"
CHARACTERISTIC = "7772e5db-3868-4112-a1a9-f2669d106bf3"

# semitones above C, by the character a key types with no modifier
KEY_NOTES = {"a": 0, "w": 1, "s": 2, "e": 3, "d": 4, "f": 5, "t": 6, "g": 7, "y": 8, "h": 9, "u": 10, "j": 11,
             "k": 12, "o": 13, "l": 14, "p": 15, "semicolon": 16, "apostrophe": 17, "quoteright": 17}
PROGRAM_KEYS = {"1": 0, "2": 1, "3": 2, "4": 3, "5": 4, "6": 5, "7": 6, "8": 7, "9": 8, "0": 9, "minus": 10}
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]


def packet(*messages):
    """One BLE MIDI packet: a header, then a timestamp byte in front of each message."""
    ms = int(time.monotonic() * 1000) & 0x1FFF
    out = bytearray([0x80 | (ms >> 7)])
    for message in messages:
        out += bytes([0x80 | (ms & 0x7F)]) + bytes(message)
    return bytes(out)


class Link:
    """The Bluetooth side, on a thread of its own: connects, then sends what is put in `out`."""

    def __init__(self):
        self.out = queue.Queue()
        self.state = "looking for the board..."
        self.done = threading.Event()
        self.thread = threading.Thread(target=lambda: asyncio.run(self.run()), daemon=True)

    def send(self, *message):
        self.out.put(bytes(message))

    async def run(self):
        while not self.done.is_set():
            device = await BleakScanner.find_device_by_filter(
                lambda d, ad: (ad.local_name or d.name) == NAME or SERVICE in [u.lower() for u in ad.service_uuids], timeout=10)
            if device is None:
                self.state = "no board found; still looking... (is another host connected to it?)"
                continue
            try:
                async with BleakClient(device) as client:
                    await client.start_notify(CHARACTERISTIC, lambda *_: None)     # a MIDI host subscribes before it plays
                    self.state = "connected"
                    while client.is_connected and not self.done.is_set():
                        try:
                            message = self.out.get_nowait()
                        except queue.Empty:
                            await asyncio.sleep(0.002)
                            continue
                        await client.write_gatt_char(CHARACTERISTIC, packet(message), response=False)
                    if client.is_connected:                                        # leaving: no key stays down
                        await client.write_gatt_char(CHARACTERISTIC, packet([0xB0, 123, 0]), response=False)
                        await asyncio.sleep(0.1)
            except Exception as error:             # the board reset, or went out of range: look for it again
                self.state = f"lost the board ({type(error).__name__}); looking again..."
                await asyncio.sleep(1.0)
        self.state = "closed"


def play(link, notes, seconds):
    """No window: the notes one after the other."""
    link.thread.start()
    end = time.time() + 20
    while link.state != "connected" and time.time() < end:
        time.sleep(0.1)
    print(link.state)
    if link.state != "connected":
        return 1
    for note in notes:
        link.send(0x90, note, 100)
        time.sleep(seconds)
        link.send(0x80, note, 0)
        time.sleep(0.05)
    time.sleep(0.2)
    link.done.set()
    link.thread.join(3)
    return 0


def window(link, seconds):
    import tkinter as tk

    root = tk.Tk()
    root.title("SID Theremin keys")
    root.geometry("560x190")
    text = tk.StringVar()
    tk.Label(root, textvariable=text, font=("Menlo", 13), justify="left", anchor="w", padx=14, pady=12).pack(fill="both", expand=True)

    state = {"octave": 4, "mod": 0, "program": 0, "velocity": 127}
    down = {}                                      # keysym -> the note it started, which an octave change must not lose
    releasing = {}                                 # keysym -> the pending release, for keyboards that repeat as up-down pairs

    def let_go(keysym):
        releasing.pop(keysym, None)
        note = down.pop(keysym, None)
        if note is not None:
            link.send(0x80, note, 0)

    def key_down(event):
        keysym = event.keysym if len(event.keysym) > 1 else event.keysym.lower()
        if keysym in releasing:                    # an auto-repeat's up-down pair: the key never left
            root.after_cancel(releasing.pop(keysym))
            return
        if keysym in down:
            return                                 # auto-repeat
        if keysym in KEY_NOTES:
            note = 12 * (state["octave"] + 1) + KEY_NOTES[keysym]
            if 0 <= note <= 127:
                down[keysym] = note
                link.send(0x90, note, state["velocity"])
        elif keysym in ("z", "x"):
            state["octave"] = max(0, min(8, state["octave"] + (1 if keysym == "x" else -1)))
        elif keysym in PROGRAM_KEYS:
            state["program"] = PROGRAM_KEYS[keysym]
            link.send(0xC0, state["program"])
        elif keysym in ("Left", "Right"):
            down[keysym] = None
            link.send(0xE0, 0x00 if keysym == "Left" else 0x7F, 0x00 if keysym == "Left" else 0x7F)
        elif keysym in ("Up", "Down"):
            state["mod"] = max(0, min(127, state["mod"] + (16 if keysym == "Up" else -16)))
            link.send(0xB0, 1, state["mod"])
        elif keysym in ("bracketleft", "bracketright"):
            state["velocity"] = max(1, min(127, state["velocity"] + (16 if keysym == "bracketright" else -16)))
        elif keysym == "space":
            down.clear()
            link.send(0xB0, 123, 0)

    def key_up(event):
        keysym = event.keysym if len(event.keysym) > 1 else event.keysym.lower()
        if keysym in ("Left", "Right"):
            down.pop(keysym, None)
            link.send(0xE0, 0x00, 0x40)            # the bend springs back to the middle
        elif keysym in down and keysym not in releasing:
            releasing[keysym] = root.after(25, let_go, keysym)

    def all_up(_event=None):
        for keysym in list(down):
            let_go(keysym)
        link.send(0xB0, 123, 0)

    def refresh():
        held = " ".join(f"{NOTE_NAMES[n % 12]}{n // 12 - 1}" for n in down.values() if n is not None)
        text.set(f"Bluetooth: {link.state}\n\n"
                 f"  W E   T Y U   O P        octave {state['octave']} (Z / X)\n"
                 f" A S D F G H J K L ; '      instrument {state['program'] + 1} (1..9 0 -)   vibrato {state['mod']}   velocity {state['velocity']} ([ ])\n\n"
                 f"sounding: {held or '-'}")
        root.after(80, refresh)

    def close():
        all_up()
        link.done.set()
        link.thread.join(3)
        root.destroy()

    root.bind("<KeyPress>", key_down)
    root.bind("<KeyRelease>", key_up)
    root.bind("<FocusOut>", all_up)                # a key let go in another window would never be heard of
    root.protocol("WM_DELETE_WINDOW", close)
    if seconds:
        root.after(int(seconds * 1000), close)
    link.thread.start()
    refresh()
    root.mainloop()
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--play", metavar="NOTES", help="no window: play these MIDI note numbers, one after the other")
    parser.add_argument("--note-seconds", type=float, default=0.4, help="how long each --play note lasts (default 0.4)")
    parser.add_argument("--seconds", type=float, default=0, help="close the window by itself after this long")
    args = parser.parse_args()
    link = Link()
    if args.play:
        return play(link, [int(n) for n in args.play.split()], args.note_seconds)
    return window(link, args.seconds)


if __name__ == "__main__":
    raise SystemExit(main())
