#pragma once
// The SID the theremin plays: cRSID's — the emulation the Pocket Synth's tune player runs, with its
// 6581 and 8580 filter curves, the 6581's filter distortion and envelope DAC, its combined
// waveforms — without the C64 around it. cRSID's SID reads its registers through a pointer and
// its clock ratio from the C64 instance, and nothing else, so this gives it 32 bytes of registers
// and the head of a C64 instance: 263 KB of memory banks are not allocated, which matters on a
// board with no PSRAM. The build defines CRSID_SID_ONLY, which takes out the one place the SID
// code writes into those banks (OSC3/ENV3 read-back, for tunes that read them).
//
// Two things a register file cannot do as the chip does, done here:
//   - cRSID sees a gate edge when its envelope code next runs, so a gate closed and opened again
//     between two runs — a pluck — would not retrigger. setControl() latches the edge at once.
//   - cRSID emulates the ADSR delay bug: after a gate-on the envelope can wait up to 33 ms for its
//     rate counter to wrap. C64 players hard-restart a voice two frames ahead to get round that;
//     an instrument played by hand cannot know two frames ahead, so a gate-on clears the counter.
#include <stddef.h>
#include <stdint.h>

// As on the chip: the control register of a voice ($D404 + 7 * voice)...
enum { SID_CTRL_GATE = 0x01, SID_CTRL_SYNC = 0x02, SID_CTRL_RING = 0x04, SID_CTRL_TEST = 0x08,
       SID_CTRL_TRIANGLE = 0x10, SID_CTRL_SAWTOOTH = 0x20, SID_CTRL_PULSE = 0x40, SID_CTRL_NOISE = 0x80 };
// ...and the filter mode bits of $D418, beside the volume
enum { SID_FILT_LP = 0x10, SID_FILT_BP = 0x20, SID_FILT_HP = 0x40, SID_FILT_3OFF = 0x80 };

struct cRSID_C64instance;
struct cRSID_SIDinstance;

class SidChip {
public:
    bool begin(uint32_t sampleRate, int model);  // 6581 or 8580; false if there is no memory for it
    void setModel(int model);
    int model() const;

    void setFreqHz(int voice, float hz);         // exact for cRSID's clock ratio, which is an integer
    void setPulseWidth(int voice, float duty);   // 0..1
    void setControl(int voice, uint8_t control);
    void setAdsr(int voice, uint8_t a, uint8_t d, uint8_t s, uint8_t r);
    /// The cutoff is in Hz, through the inverse of the chip model's own curve: the 6581's starts at
    /// 220 Hz and is far from even. `modeVolume` is $D418: SID_FILT_* and the volume, 0..15.
    void setFilter(float cutoffHz, uint8_t resonance, uint8_t routing, uint8_t modeVolume);
    void render(int16_t* buf, size_t frames);

    uint8_t control(int voice) const { return _regs[voice * 7 + 4]; }
    uint8_t envelope(int voice) const;           // 0..255, the envelope counter
    float freqHz(int voice) const;
    float cutoffHz() const;                      // what the cutoff register means in the model's curve
    uint8_t reg(uint8_t r) const { return _regs[r & 0x1f]; }

private:
    uint8_t _regs[32] = {};
    cRSID_C64instance* _c64 = nullptr;
    cRSID_SIDinstance* _sid = nullptr;
    uint32_t _sampleRate = 44100;
    uint32_t _cycles = 0;                        // 28.4: SID clocks owed to the envelopes
};
