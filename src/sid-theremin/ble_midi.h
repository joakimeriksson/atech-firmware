#pragma once
// MIDI over Bluetooth LE, as a peripheral: the MacBook (Audio MIDI Setup > MIDI Studio > Bluetooth), an
// iPad or a phone connects to "SID Theremin" and it shows up there as a MIDI port. Input only: what
// is written to the characteristic is handed on, packet by packet, from the Bluetooth host's task.
#include <stddef.h>
#include <stdint.h>

typedef void (*BleMidiPacket)(const uint8_t* packet, size_t length);

// C linkage so that the name is the symbol: esp32sim has no Bluetooth controller, and runs this
// build with `--stub bleMidiBegin`.
extern "C" bool bleMidiBegin(const char* name, BleMidiPacket onPacket);
bool bleMidiConnected();
