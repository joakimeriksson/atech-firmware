#include "sid_chip.h"

#include <math.h>
#include <stdlib.h>
#include <libcRSID.h>
#include "esp_attr.h"


namespace {
#include "C64/SID.h"                             // cRSID's tables; only the two cutoff curves are used here
const int CUTOFF_STEPS = 2048;
const unsigned short* cutoffCurve(int model) { return model == 8580 ? CutoffMul8580_44100Hz : CutoffMul6581_44100Hz; }
}

bool SidChip::begin(uint32_t sampleRate, int model) {
    // the C64 instance up to its memory banks: all the SID code reads of it is the clock ratio,
    // the attenuation and the digi-mode flag
    _c64 = (cRSID_C64instance*) calloc(1, offsetof(cRSID_C64instance, RAMbank));
    if (!_c64) { return false; }
    _sampleRate = sampleRate;
    _c64->SampleRate = (unsigned short) sampleRate;
    _c64->SampleClockRatio = (unsigned short) ((985248u << 4) / sampleRate);   // PAL, as cRSID_createC64 has it
    _c64->Attenuation = 0;                       // one chip and no digi channel to leave room for
    _sid = &_c64->SID[1];
    _sid->C64 = _c64;
    _sid->ChipModel = (unsigned short) model;
    _sid->BaseAddress = 0xD400;
    _sid->BasePtr = _regs;
    cRSID_initSIDchip(_sid);
    return true;
}

void SidChip::setModel(int model) { if (_sid) { _sid->ChipModel = (unsigned short)(model == 8580 ? 8580 : 6581); } }
int SidChip::model() const { return _sid ? _sid->ChipModel : 0; }

void SidChip::setFreqHz(int voice, float hz) {
    // cRSID steps a 28-bit accumulator by register * ratio each sample
    float reg = hz * 268435456.0f / ((float)_sampleRate * (float)_c64->SampleClockRatio);
    uint16_t r = reg <= 0.0f ? 0 : reg >= 65535.0f ? 65535 : (uint16_t)(reg + 0.5f);
    _regs[voice * 7] = (uint8_t) r;
    _regs[voice * 7 + 1] = (uint8_t)(r >> 8);
}

float SidChip::freqHz(int voice) const {
    uint16_t r = (uint16_t)(_regs[voice * 7] | (_regs[voice * 7 + 1] << 8));
    return (float)r * (float)_sampleRate * (float)_c64->SampleClockRatio / 268435456.0f;
}

void SidChip::setPulseWidth(int voice, float duty) {
    int pw = (int)(duty * 4095.0f + 0.5f);
    pw = pw < 0 ? 0 : pw > 4095 ? 4095 : pw;
    _regs[voice * 7 + 2] = (uint8_t) pw;
    _regs[voice * 7 + 3] = (uint8_t)(pw >> 8);
}

void SidChip::setControl(int voice, uint8_t control) {
    uint8_t before = _regs[voice * 7 + 4];
    _regs[voice * 7 + 4] = control;
    if ((before ^ control) & SID_CTRL_GATE) {
        if (control & SID_CTRL_GATE) { _sid->RateCounter[voice * 7] = 0; }   // the hard restart: see the header
        cRSID_emulateADSRs(_sid, 0);             // no clocks pass; the envelopes see the edge
    }
}

void SidChip::setAdsr(int voice, uint8_t a, uint8_t d, uint8_t s, uint8_t r) {
    _regs[voice * 7 + 5] = (uint8_t)((a << 4) | (d & 0x0f));
    _regs[voice * 7 + 6] = (uint8_t)((s << 4) | (r & 0x0f));
}

void SidChip::setFilter(float cutoffHz, uint8_t resonance, uint8_t routing, uint8_t modeVolume) {
    // the curves hold the state-variable filter's coefficient, 4096 * 2 sin(pi f / fs), per register value
    float limit = (float)_sampleRate * 0.45f;    // the curve itself stops lower: the search ends on its last step
    float hz = cutoffHz < 1.0f ? 1.0f : cutoffHz > limit ? limit : cutoffHz;
    unsigned want = (unsigned)(8192.0f * sinf((float)M_PI * hz / (float)_sampleRate) + 0.5f);
    const unsigned short* curve = cutoffCurve(model());
    int lo = 0, hi = CUTOFF_STEPS - 1;
    while (lo < hi) {                            // the first register value whose cutoff reaches it
        int mid = (lo + hi) / 2;
        if (curve[mid] < want) { lo = mid + 1; } else { hi = mid; }
    }
    _regs[0x15] = (uint8_t)(lo & 7);
    _regs[0x16] = (uint8_t)(lo >> 3);
    _regs[0x17] = (uint8_t)((resonance << 4) | (routing & 0x0f));
    _regs[0x18] = modeVolume;
}

float SidChip::cutoffHz() const {
    int fc = (_regs[0x16] << 3) | (_regs[0x15] & 7);
    float c = cutoffCurve(model())[fc] / 8192.0f;
    return asinf(c > 1.0f ? 1.0f : c) * (float)_sampleRate / (float)M_PI;
}

uint8_t SidChip::envelope(int voice) const { return _sid->EnvelopeCounter[voice * 7]; }

IRAM_ATTR void SidChip::render(int16_t* buf, size_t frames) {
    for (size_t i = 0; i < frames; i++) {
        // The envelopes run on SID clocks, ~22 a sample. cRSID's envelope counter fires when a step lands in
        // a window as wide as the step, which is exact only while a step is no longer than the rate's period:
        // longer, and the counter jumps the window and stalls until it wraps, 33 ms. The shortest period is
        // 9 clocks, for a rate of 0; the next is 32. So a whole sample is one step unless a voice is at rate
        // 0 now, when the sample goes in steps of 9 -- either way the same envelope, clock for clock, as the
        // player's steps of an instruction's few clocks, at a third of the calls.
        _cycles += _c64->SampleClockRatio;
        int clocks = (int)(_cycles >> 4);
        _cycles &= 15;
        int longest = clocks;
        for (int v = 0; v < 3; v++) {
            const uint8_t state = _sid->ADSRstate[v * 7], ad = _regs[v * 7 + 5], sr = _regs[v * 7 + 6];
            const uint8_t rate = (state & 0x80) ? (ad >> 4) : (state & 0x40) ? (ad & 0x0f) : (sr & 0x0f);
            if (rate == 0) { longest = 9; }
        }
        while (clocks > 0) {
            int step = clocks > longest ? longest : clocks;
            cRSID_emulateADSRs(_sid, (char) step);
            clocks -= step;
        }
        int out = cRSID_emulateWaves(_sid);
        buf[i] = (int16_t)(out > 32767 ? 32767 : out < -32768 ? -32768 : out);
    }
}
