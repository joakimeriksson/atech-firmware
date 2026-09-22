#pragma once
// SID 2: a rhythm and bass section under the lead, the way a C64 tune has one.
//
// A section is a four-bar phrase: a chord a bar, in the key of the lead's centre note, so the lead
// always fits; a bassline that moves with the chords; and a fourth bar that is a fill, with a
// bassline and drums of its own, before the phrase comes round again. Voice 1 is the bass. Voice 2
// plays the kick and the snare -- or, in the soft sections, a soft boom and a tick, and the fake
// echo C64 composers made from a spare voice: the arpeggio played again a few steps late and quieter.
// Voice 3 plays the hi-hats, or the arpeggio: the chord's notes, one a step. Drums are the C64's own,
// one voice changing waveform and pitch every frame, 50 times a second, from a table. The board's
// speaker has no low end, so the hard kick is a pulse, whose harmonics it can play, and the soft
// sections put their bass an octave higher. Steps are counted in samples, so the tempo has no jitter.
//
// Owned by the synth task: every call comes from it, except the event log, which loop() reads.
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "sid_chip.h"

namespace beat {

const int8_t R = -128, T = 127;                  // in a bassline: a rest; a tie, holding the note before

struct Chord { int8_t root; bool major; };        // semitones above the key; minor unless major
struct Section {
    const char* name;
    uint8_t bpm;                                  // the tempo it starts at when it is picked
    // per step, the plain bars (1 to 3) and the fill (bar 4):
    const char* drums[2];                         // voice 2: k kick, s snare, b soft boom, t tick, e echo, . nothing
    const char* tops[2];                          // voice 3: h hi-hat, o open hi-hat, a arpeggio note, . nothing
    int8_t bass[2][16];                           // semitones above the bar's chord; R rest, T tie
    Chord chords[4];
    // the bass: waveform, envelope, share of a step an untied note is held, octaves above the key's root.
    // No envelope rate of 0 on a note that is held: the chip is clocked in small steps while one is on (sid_chip.cpp)
    uint8_t bassWave, bassA, bassD, bassS, bassR; float bassHeld; int8_t bassOctave;
    // the arpeggio and its echo: waveform; envelopes; how long the gate is held (0: until it has decayed) --
    // a short gate on a slow attack never reaches full level, which is how a SID voice plays quietly
    uint8_t arpWave, arpA, arpD, arpGateMs, echoA, echoD, echoGateMs, echoSteps;
    // the filter: which voices go through it; resonance; the cutoff in octaves above the bass note, more
    // at each bass note, dying over envMs, and a slow sweep either side over lfoBars bars
    uint8_t routing, resonance; float cutoff, envOctaves; uint16_t envMs; float lfoOctaves; uint8_t lfoBars;
    uint8_t level;                                // SID 2's master volume, 0..15
};

#define SAWTOOTH SID_CTRL_SAWTOOTH
#define PULSE SID_CTRL_PULSE
#define TRIANGLE SID_CTRL_TRIANGLE
static const Section SECTIONS[] = {
    // a disco pump: root and octave, over i i VI VII
    { "FOUR", 120,
      { "k...s...k...s...", "k...s...k.s.ssss" }, { "..h...h...h...h.", "..h...h...h.o..." },
      { { 0, R, 12, R,   0, R, 12, R,   0, R, 12, R,   0, R, 12, R },
        { 0, R, 12, R,   0, R, 12, R,   0, 0, 12, 12,  7, 7, 12, R } },
      { { 0, false }, { 0, false }, { 8, true }, { 10, true } },
      PULSE, 0, 6, 10, 4, 0.7f, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0x01, 9, 3.0f, 2.0f, 70, 0.0f, 1,   15 },
    // syncopated, over i i iv iv
    { "FUNK", 104,
      { "k...s.k...k.s..s", "k...s.k...k.s.ss" }, { "hhhhhhhhhhhhhhho", "hhhhhhhhhhhhohho" },
      { { 0, R, R, 12,   R, R, 0, T,   10, R, 0, R,   7, R, 10, 12 },
        { 0, R, R, 12,   R, R, 0, T,   10, R, 12, R,  3, 5, 6, 7 } },
      { { 0, false }, { 0, false }, { 5, false }, { 5, false } },
      PULSE, 0, 6, 10, 4, 0.7f, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0x01, 9, 3.0f, 2.0f, 70, 0.0f, 1,   15 },
    // a running Hubbard bass in sixteenths, over i i iv v, walking down into the phrase's start
    { "HUBBARD", 128,
      { "k...s...k.k.s...", "k...s...k.k.ssss" }, { ".h.h.h.h.h.h.h.h", ".h.h.h.h.h.h.h.o" },
      { { 0, 0, 12, 0,   0, 12, 0, 12,   10, 10, 22, 10,  7, 7, 19, 7 },
        { 0, 0, 12, 0,   0, 12, 0, 12,   10, 10, 22, 10,  12, 10, 7, 5 } },
      { { 0, false }, { 0, false }, { 5, false }, { 7, false } },
      PULSE, 0, 6, 10, 4, 0.7f, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0x01, 9, 3.0f, 2.0f, 70, 0.0f, 1,   15 },
    // straight eighths, over i VII VI VII
    { "DRIVE", 140,
      { "k...s.k.k...s...", "k...s.k.k...s.ss" }, { "h.h.h.h.h.h.h.h.", "h.h.h.h.h.h.o..." },
      { { 0, R, 0, 12,   0, R, 0, 12,   0, R, 0, 12,   0, R, 7, R },
        { 0, R, 0, 12,   0, R, 0, 12,   7, R, 7, R,    12, R, 7, R } },
      { { 0, false }, { 10, true }, { 8, true }, { 10, true } },
      SAWTOOTH, 0, 6, 9, 4, 0.8f, 0,   0, 0, 0, 0, 0, 0, 0, 0,   0x01, 8, 3.5f, 1.5f, 90, 0.0f, 1,   14 },
    // soft, in Martin Galway's manner: a boom on one and a tick on three; an arpeggio in eighths through
    // the chord, its echo a sixteenth behind, quieter; a round bass held for half a bar; all of it
    // through a filter that sweeps slowly over the phrase. i VI iv VII; the fill runs the arpeggio in
    // sixteenths to lift into the next phrase
    { "GALWAY", 84,
      { "be.e.e.ete.e.e.e", "be.e.e.ete.e.e.e" }, { "a.a.a.a.a.a.a.a.", "a.a.a.a.aaaaaaaa" },
      { { 0, T, T, T,   T, T, T, T,   7, T, T, T,   T, T, T, T },
        { 0, T, T, T,   T, T, T, T,   7, T, T, T,   5, T, T, T } },
      { { 0, false }, { 8, true }, { 5, false }, { 10, true } },
      PULSE, 5, 1, 10, 9, 0.9f, 1,   PULSE, 1, 8, 0, 3, 8, 10, 1,   0x07, 6, 2.5f, 0.0f, 1, 0.8f, 4,   9 },
    // softer still: no drums at all; the arpeggio rippling in sixteenths with its echo three steps
    // behind, a dotted eighth, over a held bass. i iv VI v
    { "DRIFT", 70,
      { "eeeeeeeeeeeeeeee", "eeeeeeeeeeeeeeee" }, { "aaaaaaaaaaaaaaaa", "aaaaaaaaaaaaaaaa" },
      { { 0, T, T, T,   T, T, T, T,   T, T, T, T,   T, T, T, T },
        { 0, T, T, T,   T, T, T, T,   T, T, T, T,   7, T, T, T } },
      { { 0, false }, { 5, false }, { 8, true }, { 7, false } },
      TRIANGLE, 6, 1, 12, 10, 0.95f, 1,   TRIANGLE, 2, 7, 0, 4, 7, 12, 3,   0x07, 10, 3.0f, 0.0f, 1, 1.0f, 4,   9 },
};
#undef SAWTOOTH
#undef PULSE
#undef TRIANGLE
static const int SECTION_COUNT = sizeof(SECTIONS) / sizeof(SECTIONS[0]);

struct Frame { uint8_t wave; uint16_t hz; };      // one 50 Hz frame; a 0 wave ends the table, the last frame held
static const Frame KICK[]  = { { SID_CTRL_NOISE, 1600 }, { SID_CTRL_PULSE, 220 }, { SID_CTRL_PULSE, 160 }, { SID_CTRL_PULSE, 120 },
                               { SID_CTRL_PULSE, 95 }, { SID_CTRL_PULSE, 80 }, { SID_CTRL_PULSE, 70 }, { 0, 0 } };
static const Frame SNARE[] = { { SID_CTRL_PULSE, 240 }, { SID_CTRL_NOISE, 3800 }, { SID_CTRL_NOISE, 3000 }, { SID_CTRL_NOISE, 2600 }, { 0, 0 } };
static const Frame HAT[]   = { { SID_CTRL_NOISE, 3800 }, { 0, 0 } };
static const Frame BOOM[]  = { { SID_CTRL_TRIANGLE, 150 }, { SID_CTRL_TRIANGLE, 110 }, { SID_CTRL_TRIANGLE, 85 }, { SID_CTRL_TRIANGLE, 70 },
                               { SID_CTRL_TRIANGLE, 62 }, { SID_CTRL_TRIANGLE, 58 }, { 0, 0 } };
static const Frame TICK[]  = { { SID_CTRL_TRIANGLE, 1800 }, { SID_CTRL_TRIANGLE, 1500 }, { 0, 0 } };

/// What the rhythm section did on one step, for a host to check: the log is a ring loop() reads.
struct Event { uint32_t n; uint8_t bar, step; int8_t bass; char drum, top; int8_t drumNote, topNote; };

}  // namespace beat

class Rhythm {
public:
    bool begin(uint32_t sampleRate, int model) {
        _rate = sampleRate;
        _frameLen = sampleRate / 50;
        if (!_sid.begin(sampleRate, model)) { return false; }
        for (int v = 0; v < 3; v++) { _sid.setPulseWidth(v, 0.5f); }
        _sid.setPulseWidth(0, 0.35f);
        setTempo(120);
        return true;
    }
    void setModel(int model) { if (model != _sid.model()) { _sid.setModel(model); } }
    /// The section: taken at the next bar while it plays, at once otherwise.
    void setSection(int s) { _next = s < 0 ? 0 : s >= beat::SECTION_COUNT ? beat::SECTION_COUNT - 1 : s; if (!_running) { _section = _next; } }
    void setTempo(int bpm) { _stepLen = _rate * 15 / (uint32_t)(bpm < 30 ? 30 : bpm); }   // a sixteenth: a quarter of a beat
    void setKey(int midi) { _key = midi; }        // the key's root; the bass sits here or an octave up
    void setRunning(bool on) {
        if (on == _running) { return; }
        _running = on;
        if (on) { _section = _next; _step = 0; _bar = 0; _toStep = 0; _idle = false; _age = 0; for (int i = 0; i < 16; i++) { _arpAt[i] = -1; } }
        else { for (int v = 0; v < 3; v++) { gate(v, false); _gateOff[v] = 0; } }
    }
    bool running() const { return _running; }
    int bar() const { return _shownBar; }
    int step() const { return _shownStep; }
    bool active() const { return _running || !_idle; }      // playing, or its last notes still dying away
    uint8_t envelope(int voice) const { return _sid.envelope(voice); }

    /// What each voice last struck, for a display: a drum's letter, 'a' an arpeggio note or 'e' its echo, 'B'
    /// the bass; the note; where in the arpeggio's up-and-down it came (0..5); and a count of strikes, so a
    /// display that looks less often than a hi-hat lasts can still tell one came. Read by the synth task.
    struct Shown { char what; int8_t note; uint8_t pos; uint32_t strikes; };
    Shown shown[3] = { { '.', -1, 0, 0 }, { '.', -1, 0, 0 }, { '.', -1, 0, 0 } };

    beat::Event events[32];                      // the ring, written by the synth task
    volatile uint32_t eventCount = 0;

    /// SID 2's output for the next `frames` samples; false, and nothing written, when it is stopped and silent.
    bool render(int16_t* out, size_t frames) {
        if (!_running && _idle) { return false; }
        const beat::Section& sec = beat::SECTIONS[_section];
        while (frames > 0) {
            if (_running && _toStep == 0) { onStep(); _toStep = _stepLen; }
            for (int v = 1; v < 3; v++) { if (_drum[v] && _drumNext[v] == 0) { drumFrame(v); } }
            for (int v = 0; v < 3; v++) { if (_gateOff[v] == 1) { gate(v, false); _gateOff[v] = 0; } }

            uint32_t n = frames < 64 ? (uint32_t)frames : 64;
            if (_running && _toStep < n) { n = _toStep; }
            for (int v = 1; v < 3; v++) { if (_drum[v] && _drumNext[v] < n) { n = _drumNext[v]; } }
            for (int v = 0; v < 3; v++) { if (_gateOff[v] > 1 && _gateOff[v] - 1 < n) { n = _gateOff[v] - 1; } }

            // the filter: a strike at each bass note, dying away, and a slow sweep over lfoBars bars
            const float noteMs = _noteAge * 1000.0f / _rate;
            const float lfo = sec.lfoOctaves * sinf(6.2831853f * (float)_age / ((float)_stepLen * 16.0f * sec.lfoBars));
            _sid.setFilter(_bassHz * powf(2.0f, sec.cutoff + sec.envOctaves * expf(-noteMs / sec.envMs) + lfo),
                           sec.resonance, sec.routing, SID_FILT_LP | sec.level);
            _sid.render(out, n);

            out += n; frames -= n;
            if (_running) { _toStep -= n; }
            for (int v = 1; v < 3; v++) { if (_drum[v]) { _drumNext[v] -= n; } }
            for (int v = 0; v < 3; v++) { if (_gateOff[v] > 1) { _gateOff[v] -= n; } }
            _noteAge += n; _age += n;
        }
        if (!_running && _sid.envelope(0) == 0 && _sid.envelope(1) == 0 && _sid.envelope(2) == 0) { _idle = true; }
        return true;
    }

private:
    SidChip _sid;
    uint32_t _rate = 44100, _frameLen = 882, _stepLen = 5512;
    int _section = 0, _next = 0, _key = 36, _step = 0, _bar = 0, _shownStep = 0, _shownBar = 0;
    bool _running = false, _idle = true;
    uint32_t _toStep = 0, _age = 0, _noteAge = 0;
    const beat::Frame* _drum[3] = { nullptr, nullptr, nullptr };   // the frame table a voice plays, while it plays
    uint32_t _drumNext[3] = { 0, 0, 0 };        // samples until that voice's next frame
    uint32_t _gateOff[3] = { 0, 0, 0 };          // samples + 1 until that voice's gate closes; 0: nothing pending
    float _bassHz = 65.4f;
    int8_t _arpAt[16];                           // the arpeggio note played on each step of this bar and the last, -1 none
    uint8_t _arpPosAt[16];                       // and where in the arpeggio it came
    int _arpIndex = 0;

    static float noteHz(int note) { return 440.0f * powf(2.0f, (note - 69) / 12.0f); }
    uint32_t msToSamples(uint32_t ms) const { return ms * _rate / 1000; }

    void gate(int v, bool on) {
        uint8_t c = _sid.control(v);
        _sid.setControl(v, on ? (c | SID_CTRL_GATE) : (c & ~SID_CTRL_GATE));
    }

    /// Strike voice `v`: its envelope, then the gate closed and opened again, as a player retriggers one.
    void strike(int v, uint8_t wave, float hz, uint8_t a, uint8_t d, uint8_t s, uint8_t r, uint32_t heldSamples) {
        _sid.setAdsr(v, a, d, s, r);
        _sid.setFreqHz(v, hz);
        _sid.setControl(v, wave);
        _sid.setControl(v, wave | SID_CTRL_GATE);
        _gateOff[v] = heldSamples ? heldSamples + 1 : 0;
    }

    void drum(int v, const beat::Frame* frames, uint8_t decay) {
        strike(v, frames->wave, frames->hz, 0, decay, 0, 0, 0);
        _drum[v] = frames;
        _drumNext[v] = _frameLen;
    }

    void drumFrame(int v) {
        const beat::Frame* next = _drum[v] + 1;
        if (next->wave == 0) { _drum[v] = nullptr; return; }  // the last frame holds while the envelope dies
        _drum[v] = next;
        _sid.setFreqHz(v, next->hz);
        _sid.setControl(v, next->wave | (_sid.control(v) & SID_CTRL_GATE));
        _drumNext[v] = _frameLen;
    }

    /// The chord's notes, up and down again: root, third, fifth, octave, fifth, third.
    static int arpTone(const beat::Chord& c, int i) {
        static const int8_t MINOR[6] = { 0, 3, 7, 12, 7, 3 }, MAJOR[6] = { 0, 4, 7, 12, 7, 4 };
        return (c.major ? MAJOR : MINOR)[i % 6];
    }

    void show(int v, char what, int note, int pos) {
        shown[v].what = what; shown[v].note = (int8_t) note; shown[v].pos = (uint8_t) pos;
        shown[v].strikes = shown[v].strikes + 1;
    }

    void onStep() {
        if (_step == 0 && _next != _section) { _section = _next; }   // a new section starts on a bar
        const beat::Section& sec = beat::SECTIONS[_section];
        const int s = _step, variant = _bar == 3 ? 1 : 0;
        const beat::Chord& chord = sec.chords[_bar];
        const int chordRoot = _key + chord.root;
        if (s == 0) { _arpIndex = 0; }
        beat::Event ev = { eventCount, (uint8_t)_bar, (uint8_t)s, beat::R, '.', '.', -1, -1 };

        // voice 3: the hi-hats, or the arpeggio
        const char top = sec.tops[variant][s];
        _arpAt[s] = -1;
        if (top == 'h' || top == 'o') { drum(2, beat::HAT, top == 'o' ? 5 : 2); show(2, top, -1, shown[2].strikes % 3); }
        else if (top == 'a') {
            const int pos = _arpIndex % 6;
            const int note = chordRoot + 24 + arpTone(chord, _arpIndex++);
            strike(2, sec.arpWave, noteHz(note), sec.arpA, sec.arpD, 0, sec.arpD, msToSamples(sec.arpGateMs));
            _drum[2] = nullptr;
            _arpAt[s] = (int8_t)note;
            _arpPosAt[s] = (uint8_t)pos;
            ev.topNote = (int8_t)note;
            show(2, 'a', note, pos);
        }
        ev.top = top;

        // voice 2: the drums, or the echo of the arpeggio `echoSteps` back (across the bar line too)
        const char d = sec.drums[variant][s];
        if (d == 'k') { drum(1, beat::KICK, 7); show(1, d, -1, 0); }
        else if (d == 's') { drum(1, beat::SNARE, 6); show(1, d, -1, 0); }
        else if (d == 'b') { drum(1, beat::BOOM, 5); show(1, d, -1, 0); }
        else if (d == 't') { drum(1, beat::TICK, 1); show(1, d, -1, 0); }
        else if (d == 'e') {
            const int back = (s + 16 - sec.echoSteps) % 16;
            const int8_t note = _arpAt[back];
            if (note >= 0) {
                strike(1, sec.arpWave, noteHz(note), sec.echoA, sec.echoD, 0, sec.echoD, msToSamples(sec.echoGateMs));
                _drum[1] = nullptr;
                ev.drumNote = note;
                show(1, 'e', note, _arpPosAt[back]);
            }
        }
        ev.drum = d;

        // voice 1: the bass, held through its ties
        const int8_t b = sec.bass[variant][s];
        if (b != beat::R && b != beat::T) {
            int ties = 0;
            while (ties < 15 && sec.bass[variant][(s + 1 + ties) % 16] == beat::T && s + 1 + ties < 16) { ties++; }
            const int note = chordRoot + 12 * sec.bassOctave + b;
            _bassHz = noteHz(note);
            const float held = ties ? (1 + ties) - (1.0f - sec.bassHeld) : sec.bassHeld;
            strike(0, sec.bassWave, _bassHz, sec.bassA, sec.bassD, sec.bassS, sec.bassR, (uint32_t)(_stepLen * held));
            _noteAge = 0;
            ev.bass = (int8_t)note;
            show(0, 'B', note, 0);
        }

        events[eventCount % 32] = ev;
        eventCount = eventCount + 1;
        _shownStep = s; _shownBar = _bar;
        _step = (s + 1) % 16;
        if (_step == 0) { _bar = (_bar + 1) % 4; }
    }
};
