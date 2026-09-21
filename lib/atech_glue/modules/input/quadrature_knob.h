#pragma once
#include <Arduino.h>

// The turn of a rotary encoder, decoded from both of its lines.
//
// The SDK's RotaryEncoder takes a step on each falling edge of CLK and reads the direction off
// DT at that moment, ignoring edges within 3 ms of the last step. A contact that bounces later
// than that — CLK rising again at the end of a slow click does — is a falling edge with DT the
// other way round, so a click counts +1 and then -1 and the knob goes nowhere. Measured on the
// board in the sid-theremin build: 25 steps reported, a net movement of 3.
//
// A quadrature decoder does not have that problem: the two lines go through four states a click,
// a bounce only ever moves between two neighbouring states and back, and a click counts when the
// cycle completes. begin() takes over the CLK interrupt the SDK driver attached, so call it after
// RotaryEncoder::begin(); the driver's switch and ring go on working, its getPosition() stops.
// The sign is the driver's: DT high as CLK falls is +1.
//
// The interrupt handler is a plain static function, not a member: the Xtensa linker cannot place
// the literals of an IRAM_ATTR function that is inline in a class ("l32r: literal placed after use").
struct QuadratureKnob {
    int clk = -1, dt = -1;
    volatile uint8_t state = 3;                  // CLK << 1 | DT; 3, both high, is the rest between clicks
    volatile int8_t quarters = 0;
    volatile bool moved = false;
    volatile int32_t clicks = 0;
    volatile uint32_t bounces = 0;

    void begin(int pinClk, int pinDt);
    int32_t position() const { return clicks; }
    uint32_t bouncesIgnored() const { return bounces; }      // movements that went back where they came from
};

static void IRAM_ATTR quadratureKnobIsr(void* arg) {
    // quarter steps by (previous state << 2 | state); +1 runs 3, 1, 0, 2, 3
    static const int8_t STEP[16] = { 0, -1, +1, 0,   +1, 0, 0, -1,   -1, 0, 0, +1,   0, +1, -1, 0 };
    QuadratureKnob* k = (QuadratureKnob*)arg;
    uint8_t now = (uint8_t)((digitalRead(k->clk) << 1) | digitalRead(k->dt));
    int8_t step = STEP[(k->state << 2) | now];
    k->state = now;
    if (step) { k->quarters = k->quarters + step; k->moved = true; }
    if (now == 3 && k->moved) {                              // at rest again: the click is over
        if (k->quarters >= 2) { k->clicks = k->clicks + 1; }             // 4 of 4 quarters, or 2 with an edge missed
        else if (k->quarters <= -2) { k->clicks = k->clicks - 1; }
        else { k->bounces = k->bounces + 1; }
        k->quarters = 0; k->moved = false;
    }
}

inline void QuadratureKnob::begin(int pinClk, int pinDt) {
    clk = pinClk; dt = pinDt;
    state = (uint8_t)((digitalRead(clk) << 1) | digitalRead(dt));
    attachInterruptArg(digitalPinToInterrupt(clk), quadratureKnobIsr, this, CHANGE);
    attachInterruptArg(digitalPinToInterrupt(dt), quadratureKnobIsr, this, CHANGE);
}
