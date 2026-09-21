// SID Theremin. Hold the board flat in one hand, USB-C towards you, and play the air above the
// distance sensor with the other.
//
//   hand over the sensor      pitch: nearer is higher, an octave either side of the centre note
//   tilt left / right         filter, dark to bright (the cutoff follows the note)
//   tilt away / towards you   volume, louder away (in ARPEGGIO: the speed, faster away)
//   button 1                  tap: next mode. Hold: sound without a hand, at the pitch last played
//   button 2                  next instrument, of eleven: see INSTRUMENTS
//   knob press                which setting the knob turns
//   knob turn                 that setting, a step a click. While the knob is in use the whole screen
//                             is the setting — its name, its value, a bar for where in its range it
//                             is — and the ring turns orange and shows the same; otherwise the
//                             screen's bottom line names the setting the knob is on
//                               NOTE    the centre note, C3..C5
//                               SCALE   what the pitch snaps to: FREE, CHROMA, MAJOR, MINOR, PENTA
//                               RANGE   semitones either side of the centre note: 6, 12, 18, 24
//                               the mode's own —
//                                 THEREMIN  VIBR   vibrato depth
//                                 HARP, FRETS  RING  how long a string rings
//                                 ARPEGGIO  SPEED  steps a second with the board level
//                                           ARP    the pattern: UP, DOWN, UPDN, RAND
//                                           CHORD  TRIAD, 7TH, OCT (octaves), 5TH (fifths and octaves)
//                               SWEEP, DEPTH   with a synced instrument (LASER HARP, RAT RACE): how fast its
//                                       sync sweep moves and how far, either side of the instrument's own;
//                                       the screen shows the time and the octaves that come of it
//                               RESO    filter resonance, 0..15: 8 leaves each instrument its own
//                               CHIP    6581, the C64's first SID with its dark, distorting filter, or 8580
//                               ECHO    how much echo, 0 (off) to 10: louder repeats, and more of them
//                               TIME    the echo's delay, 60..480 ms; 300 is three ARPEGGIO steps at SPEED 10
//                               VOL     volume
//   knob hold                 take the pose the board is in now as neutral
//
// Four modes, four ways for the same hand to play the same SID:
//   THEREMIN   the pitch follows the hand continuously; a hand in range sounds the note, taking it
//              away lets it die. Vibrato comes in on a held note. Voices 1 and 2, slightly detuned
//   HARP       the scale's notes are strings across the range, and the hand plucks each one it
//              crosses: a hit that fades away over a second and a half. The three voices take the
//              plucks in turn, so three strings ring at once. FREE has no strings: every semitone is one
//   FRETS      one string on a fretted neck. A hand arriving plucks it; the pitch is the fret below
//              the hand — truncated, not rounded to the nearest — and moving along the neck changes
//              the ringing note without plucking again, a hammer-on, until the string has all but
//              died, when the next fret plucks it afresh. The frets are the scale's notes
//   ARPEGGIO   the hand picks the root and the SID runs through a chord from it, in the scale's own
//              steps (in semitones in FREE and CHROMA, gliding with the hand in FREE), without
//              retriggering, the way C64 tunes fake a chord on one voice
//
// Keys. The board is also a MIDI port over Bluetooth LE, "SID Theremin": connect it in the Mac's Audio
// MIDI Setup (MIDI Studio > Bluetooth) and any MIDI program or keyboard plays it. While keys are
// held, and for two seconds after, they are the pitch and the hand is not; each mode takes a key as
// it takes the hand —
//   THEREMIN   one voice, the last key down sounds, letting it go gives the note back to the one
//              before; a key pressed while another is held slides to it without a new attack
//   HARP       every key plucks a string, three ringing at once
//   FRETS      a key plucks the string; a key pressed while another is held is a hammer-on
//   ARPEGGIO   the last key is the root (taken to the scale's nearest note, where there is a scale)
// Keys are not rounded to the SCALE otherwise: a keyboard has its own notes. Every channel is heard.
// Pitch bend is two semitones; the mod wheel (CC 1) adds vibrato; CC 74 moves the filter as the
// tilt does; a program change picks the instrument; CC 120 and 123 let every key go. Velocity is
// not used: a SID voice has no level of its own but its envelope.
//
// The SID is cRSID's, the emulation the Pocket Synth's tune player runs, driven register by register
// as a C64 program drives the chip (sid_chip.h): what an instrument sets is what a tune would set.
//
// Either sensor may be missing. Without the distance sensor the tilt away/towards is the pitch
// instead of the volume and button 1 is the only gate; without the IMU the filter and the volume
// stay put. The pose at boot is neutral, so it plays from however it is held when it starts.
// The screen shows the note, the hand and the tilt; the Light Grids are a meter: a bar as high as
// the pitch, blue low to red high, as bright as the note is loud, leaning with the tilt; the knob
// ring points at the pitch within the range.
//
//   {"action":"recenter","value":""}         neutral = the current pose
//   {"action":"set_instrument","value":"1"}  0..9        {"action":"set_scale","value":"2"}   0..4
//   {"action":"set_center","value":"60"}     MIDI 48..72 {"action":"set_volume","value":"30"} 0..100
//   {"action":"set_mode","value":"1"}        0 THEREMIN, 1 HARP, 2 ARPEGGIO, 3 FRETS
//   {"action":"set_arp","value":"2"}         0..3 pattern {"action":"set_chord","value":"1"}  0..3
//   {"action":"set_speed","value":"4"}       arpeggio steps a second, 4..20
//   {"action":"set_echo","value":"4"}        0..10        {"action":"set_echo_ms","value":"300"}  60..480
//   {"action":"set_chip","value":"8580"}     6581 or 8580
//   {"action":"set_sweep","value":"-3"}      the SWEEP setting, -6..6   {"action":"set_depth","value":"2"}  DEPTH, -4..4
//   {"action":"screen_dump","value":"play"}  what the firmware draws, as 80 rows of RGB565 hex: play, knob, sound,
//                                            or empty for whichever view is up now
//   {"action":"note_on","value":"60"}        a key, as if from MIDI; note_off lets it go
//   {"action":"midi","value":"e0 00 60"}     one MIDI message in hex
//   {"action":"ble_packet","value":"80 80 90 3c 64"}   a BLE MIDI packet in hex, through the packet parser
//   {"action":"status","value":""}           the controls as the firmware sees them now, and the settings
//   {"action":"gate","value":"on"}           button 1 held, from the host: sounds the note with no hand there
//   {"action":"imu_log","value":"on"}        print the hand distance, the angles, the voices and the timing at
//                                            5 Hz; a number is the period in ms, and under 200 the screen
//                                            stands still to make room
//   {"action":"grid_log","value":"on"}       print every new 8x8 frame of distances, row by row as the
//                                            driver orders them; the screen stands still meanwhile
//
// Two tasks. The display is bit-banged SPI and a full frame takes 207 ms, measured, against the
// 46 ms of audio the I2S DMA holds, so the screen cannot share a loop with the sound the way it
// does in the Pocket Synth, whose screen is still while a note plays. Here the synth task owns the
// SID, the controls and the sensors, paced by the I2S writes, and refreshes the Light Grids and the
// knob ring at 20 Hz as the Pocket Synth's audio loop does; loop() has the screen and the serial
// link, and the screen shows what that task reports at the five frames a second it can manage.
#include <Arduino.h>
#include <math.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#include "modules/shared/atech_helpers.h"
#include "modules/connectivity/serial_templates.h"
#include "modules/input/quadrature_knob.h"
#include <atech_board.h>
#include "sid_chip.h"
#include "midi_in.h"
#include "ble_midi.h"

AtechSerial serialLink(115200);

// Not 0. In Arduino-ESP32 2.0.17, HWCDC::write() counts its retries down from this number, and from
// 0 the count wraps to four billion: once a host has opened the port and closed it again, the
// first write to find the transmit buffer full never returns. Measured here: with the port closed
// and the log on, loop() drew 3 frames in 20 s — the screen froze while the synth task played on.
// From 5 the write gives up after 5 ms and the core takes the host for gone until it reads again.
static const uint32_t SERIAL_TX_TIMEOUT_MS = 5;
QuadratureKnob knobTurn;                         // the knob's turn; the SDK driver keeps its switch and its ring

#define UI_BG     ST7735_TFT::COLOR_BLACK
#define UI_LABEL  ST7735_TFT::COLOR_WHITE
#define UI_VALUE  ST7735_TFT::COLOR_CYAN
#define UI_ACCENT ST7735_TFT::COLOR_ORANGE

static const uint32_t SAMPLE_RATE = 44100;
static const int CHUNK = 256;                 // one I2S write: 5.8 ms, the control rate
static const int BLOCK = 64;                  // the glide advances this often, so pitch never steps

static const float PITCH_SPAN_DEG   = 45.0f;  // tilt this far for either end of the range
static const float ROLL_SPAN_DEG    = 45.0f;
static const int   CENTRE_MIN = 48, CENTRE_MAX = 72;
static const float TILT_SMOOTH_MS   = 40.0f;  // the accelerometer also feels the hand move; this is what calms it
static const int   HAND_NEAR_MM = 60, HAND_FAR_MM = 450;   // the playing range above the sensor: 16 mm a semitone at RANGE 12
static const int   HAND_MIN_ZONES   = 3;      // of the 8x8: fewer than this in range is not a hand
// The SDK driver sets the sensor to 15 Hz but delivers 8 frames a second, measured with grid_log:
// it reads each frame in 32-byte I2C chunks with all nine outputs on, then sleeps 50 ms.
static const float HAND_SMOOTH_MS   = 70.0f;  // turns those steps into a line
static const uint32_t HAND_GONE_MS  = 150;    // two missed frames, so one dropped frame does not end the note
static const float SWELL_OCTAVES    = 1.5f;   // tilt away or towards scales the volume by up to 2^this
static const float GLIDE_FREE_MS    = 25.0f;
static const float GLIDE_SNAP_MS    = 40.0f;  // snapped notes slide into each other rather than click over
static const float SNAP_HYSTERESIS  = 0.25f;  // semitones the tilt must favour a new note by, so an edge does not flutter
static const float VIBRATO_HZ = 5.5f;
static const uint32_t VIBRATO_RAMP_MS = 500;  // it comes in over this long on a held note, like a player's
static const float SYNC_TILT_OCTAVES = 0.9f;  // tilting left or right moves the synced voice this far either way
static const uint32_t KEYS_HOLD_MS = 2000;   // after the last key, how long the keys keep the pitch from the hand
static const float BEND_SEMITONES = 2.0f, MOD_WHEEL_SEMITONES = 0.6f;
static const uint32_t STRIKE_MS = 20;         // one frame of a 50 Hz C64 player: how long an instrument's strike waveform lasts
static const uint32_t KNOB_HOLD_MS = 800;
static const uint32_t ADJUST_SHOW_MS = 1500;  // how long the ring shows a setting after the knob last touched it
static const uint32_t ADJUST_SCREEN_MS = 2500; // and the screen, which is a frame or two behind
static const uint32_t BUTTON_HOLD_MS = 350;   // button 1: a shorter press is "next mode", a longer one sounds the note
static const float ARP_RATE_OCTAVES = 1.3f;   // tilting away or towards: from 0.4 to 2.5 times SPEED

enum Mode { MODE_THEREMIN, MODE_HARP, MODE_ARP, MODE_FRETS, MODE_COUNT };
static const char* MODE_NAMES[MODE_COUNT] = { "THEREMIN", "HARP", "ARPEGGIO", "FRETS" };
static const uint8_t FRET_REPLUCK_BELOW = 40; // of 255: a string quieter than this is plucked by the next fret, not hammered

// What the knob turns, in the order a press steps through them. `modes` is the playing modes a
// setting belongs to, a bit each: a press skips the settings the mode does not have.
#define IN_MODE(m) (uint8_t)(1u << (m))
#define IN_ALL 0x7f
#define IN_SYNC 0xff                             // every mode, but only with an instrument whose voice is synced: see paramShown()
enum Param { PARAM_NOTE, PARAM_SCALE, PARAM_RANGE, PARAM_VIBRATO, PARAM_RING, PARAM_SPEED, PARAM_PATTERN, PARAM_CHORD,
             PARAM_SWEEP, PARAM_DEPTH, PARAM_RESO, PARAM_ECHO, PARAM_ECHO_TIME, PARAM_CHIP, PARAM_VOL, PARAM_COUNT };
static const struct { const char* name; uint8_t modes; } PARAMS[PARAM_COUNT] = {
    { "NOTE", IN_ALL }, { "SCALE", IN_ALL }, { "RANGE", IN_ALL }, { "VIBR", IN_MODE(MODE_THEREMIN) },
    { "RING", IN_MODE(MODE_HARP) | IN_MODE(MODE_FRETS) },
    { "SPEED", IN_MODE(MODE_ARP) }, { "ARP", IN_MODE(MODE_ARP) }, { "CHORD", IN_MODE(MODE_ARP) },
    { "SWEEP", IN_SYNC }, { "DEPTH", IN_SYNC },
    { "RESO", IN_ALL }, { "ECHO", IN_ALL }, { "TIME", IN_ALL }, { "CHIP", IN_ALL }, { "VOL", IN_ALL },
};

enum ArpPattern { ARP_UP, ARP_DOWN, ARP_UPDOWN, ARP_RANDOM, ARP_PATTERN_COUNT };
static const char* ARP_PATTERN_NAMES[ARP_PATTERN_COUNT] = { "UP", "DOWN", "UPDN", "RAND" };

// A chord is tones above the root: `steps` of the scale plus whole `octaves`, so it is a chord of
// whatever scale is on; `semis` is the same chord where the scale has no thirds of its own.
struct Chord { const char* name; uint8_t count; int8_t steps[4]; int8_t octaves[4]; int8_t semis[4]; };
static const Chord CHORDS[] = {
    { "TRIAD", 4, { 0, 2, 4, 0 }, { 0, 0, 0, 1 }, { 0, 4, 7, 12 } },
    { "7TH",   4, { 0, 2, 4, 6 }, { 0, 0, 0, 0 }, { 0, 4, 7, 10 } },
    { "OCT",   3, { 0, 0, 0, 0 }, { 0, 1, 2, 0 }, { 0, 12, 24, 0 } },
    { "5TH",   4, { 0, 4, 0, 4 }, { 0, 0, 1, 1 }, { 0, 7, 12, 19 } },
};
static const int CHORD_COUNT = sizeof(CHORDS) / sizeof(CHORDS[0]);
static const float HIGHEST_NOTE = 105.0f;     // A7: the SID's frequency register ends just above it
static const char* RING_TIMES[7] = { "0.24s", "0.3s", "0.75s", "1.5s", "2.4s", "3s", "9s" };   // SID decay 7..13

// ---- Instruments ----------------------------------------------------------------------------------
// What button 2 steps through: not the SID's four waveforms but sounds built from them with the
// devices C64 composers built theirs from — a pulse width that never stands still, one frame of
// noise in front of a note to make it twang, vibrato that waits for the note to settle, a second
// voice an octave away, a filter that closes after the strike, ring modulation, hard sync. In the
// manner of Rob Hubbard's and Martin Galway's instruments; not taken from any tune's data.
struct Instrument {
    const char* name;
    uint8_t wave, strike;             // voice 1's waveform, and the one its first STRIKE_MS has instead (0 = the same)
    uint8_t a, d, s, r;               // voice 1's envelope; HARP keeps the attack and lets RING set the rest
    float pulse, pwmDepth, pwmHz;     // pulse width, and its sweep either side
    float vibrato; uint16_t vibratoAfterMs;   // semitones with VIBR at 4; how long the note is held before it starts
    float second; uint8_t secondWave, secondLevel;   // voice 2: frequency against voice 1's (0 = silent), waveform, sustain
    uint8_t mod; float modRatio, modSweep; uint16_t modSweepMs; float modLfo, modLfoHz; uint8_t modTriangle;
                                              // SID_CTRL_RING or _SYNC on voice 1 from voice 3, unheard: their ratio.
                                              // A synced voice is the one whose frequency moves, and the movement is
                                              // the sound. Two movements, both counted from the start of the note:
                                              // a strike, modSweep octaves off the ratio and back over modSweepMs;
                                              // and an LFO at modLfoHz — a sine, modLfo octaves either side of the
                                              // ratio, that comes in as the strike dies; or (modTriangle) a triangle
                                              // that starts at the ratio and goes modLfo octaves up and back, so that
                                              // every note goes up first and then down. The tilt moves it too
    uint8_t filter; float cutoff, sweep; uint16_t sweepMs;   // SID_FILT_*; octaves above the note; more (or, negative, less) at
                                                             // the strike, dying away over sweepMs
    float wah, wahHz;                 // a slow sweep of the cutoff that never stops: octaves either side, and its rate
    uint8_t resonance;
    uint8_t level;                    // SID master volume, 0..15: two voices through a ringing filter need room one triangle does not
    uint8_t glideMs;                  // portamento: how long the pitch takes to reach a new note, arpeggio steps too (0 = at once)
};
#define TRI SID_CTRL_TRIANGLE
#define SAW SID_CTRL_SAWTOOTH
#define PUL SID_CTRL_PULSE
#define NOI SID_CTRL_NOISE
static const Instrument INSTRUMENTS[] = {
    //              wave strike a   d   s   r  pulse  pwm    Hz     vib  after   2nd   wave lvl  mod            ratio  sweep  ms    lfo   Hz  tri filter       cut   sweep   ms   wah   Hz    res lvl glide
    { "PWM LEAD",   PUL, 0,    0,  9, 11,  6, 0.45f, 0.35f, 1.1f, 0.25f, 250, 1.006f, PUL,  8,  0,             0.0f,  0.0f,   0, 0.0f,  0.0f, 0, SID_FILT_LP, 2.5f,  0.0f,   0, 0.0f, 0.0f,   5,  8,  0 },   // Hubbard: the wide, restless pulse
    { "TWANG",      PUL, NOI,  0,  8,  5,  7, 0.25f, 0.15f, 3.0f, 0.20f, 300, 2.0f,   PUL,  3,  0,             0.0f,  0.0f,   0, 0.0f,  0.0f, 0, SID_FILT_LP, 2.0f,  2.0f, 120, 0.0f, 0.0f,   8,  9,  0 },   // Hubbard: a frame of noise, then the note
    { "HUB BASS",   PUL, NOI,  0,  7,  7,  5, 0.35f, 0.25f, 0.8f, 0.0f,    0, 0.5f,   SAW,  9,  0,             0.0f,  0.0f,   0, 0.0f,  0.0f, 0, SID_FILT_LP, 1.2f,  1.5f, 200, 0.0f, 0.0f,  11,  7,  0 },   // an octave of saw under it, the filter closing
    { "SAW BRASS",  SAW, 0,    2,  8, 10,  6, 0.5f,  0.0f,  0.0f, 0.20f, 350, 1.005f, SAW,  8,  0,             0.0f,  0.0f,   0, 0.0f,  0.0f, 0, SID_FILT_LP, 1.5f,  2.5f, 350, 0.0f, 0.0f,   7, 11,  0 },   // the filter opens on the strike and settles
    { "FLUTE",      TRI, 0,    4,  0, 15,  8, 0.5f,  0.0f,  0.0f, 0.35f, 400, 0.0f,   0,    0,  0,             0.0f,  0.0f,   0, 0.0f,  0.0f, 0, 0,           0.0f,  0.0f,   0, 0.0f, 0.0f,   0, 15,  0 },   // Galway: a bare triangle and a late, deep vibrato
    { "GLASS PAD",  PUL, 0,    9,  0, 15, 11, 0.5f,  0.4f,  0.3f, 0.15f, 600, 1.008f, PUL, 12,  0,             0.0f,  0.0f,   0, 0.0f,  0.0f, 0, SID_FILT_LP, 2.0f,  0.0f,   0, 0.0f, 0.0f,   3,  7,  0 },   // Galway: slow attack, slow pulse sweep, two voices beating
    { "RING BELL",  TRI, 0,    0, 10,  4, 10, 0.5f,  0.0f,  0.0f, 0.0f,    0, 0.0f,   0,    0,  SID_CTRL_RING, 2.76f, 0.0f,   0, 0.0f,  0.0f, 0, 0,           0.0f,  0.0f,   0, 0.0f, 0.0f,   0, 15,  0 },   // ring modulation at a ratio no harmonic has
    // After the Elka Synthex patch Jarre's laser harp plays: a hard-synced sawtooth whose pitch starts more than two octaves
    // up and falls for most of a second, so every note opens as a bright tear and settles into the note; once it has
    // settled a slow LFO takes over and the note keeps turning. A second sawtooth beats against it where the Synthex had
    // its chorus. Made for HARP mode, with ECHO up and a long RING.
    { "LASER HARP", SAW, 0,    0, 11,  6, 10, 0.5f,  0.0f,  0.0f, 0.15f, 500, 1.005f, SAW,  5,  SID_CTRL_SYNC, 1.25f, 2.2f, 800, 0.22f, 0.3f, 0, SID_FILT_LP, 4.0f,  0.5f, 600, 0.0f, 0.0f,   3, 10, 45 },
    { "ORGAN",      PUL, 0,    1,  0, 15,  4, 0.5f,  0.0f,  0.0f, 0.12f, 500, 2.0f,   TRI, 10,  0,             0.0f,  0.0f,   0, 0.0f,  0.0f, 0, SID_FILT_LP, 3.0f,  0.0f,   0, 0.0f, 0.0f,   2, 12,  0 },   // a square and a triangle an octave over it
    { "WIND",       NOI, 0,    8,  0, 15,  9, 0.5f,  0.0f,  0.0f, 0.0f,    0, 0.0f,   0,    0,  0,             0.0f,  0.0f,   0, 0.0f,  0.0f, 0, SID_FILT_BP, 1.0f,  0.0f,   0, 0.0f, 0.0f,  14, 12,  0 },   // noise through a ringing band-pass that follows the note
    // In the manner of the lead in Galway's Roland's Rat Race, from memory of the record, not from its data: a sync
    // sweep. Voice 1, a pulse, is hard-synced to the note and its own frequency is what moves — from just above the note
    // to three octaves over it, the SID's 3.8 kHz ceiling allowing — as a triangle that starts
    // again with every note: up first, then down, slowly — over a second each way — and round again while the note lasts. A plain pulse sits
    // under it for the note's body; notes slide into each other and the vibrato comes late and deep.
    { "RAT RACE",   PUL, 0,    1,  9, 12,  8, 0.5f,  0.2f,  0.5f, 0.30f, 350, 1.004f, PUL,  6,  SID_CTRL_SYNC, 1.06f, 0.0f,   0, 3.0f, 0.4f, 1, SID_FILT_LP, 3.5f,  0.0f,   0, 0.0f, 0.0f,   6,  8, 70 },
};
#undef TRI
#undef SAW
#undef PUL
#undef NOI
static const int INSTRUMENT_COUNT = sizeof(INSTRUMENTS) / sizeof(INSTRUMENTS[0]);

static const char* NOTE_NAMES[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

struct Scale { const char* name; uint8_t count; int8_t steps[12]; };
static const Scale SCALES[] = {
    { "FREE",   0,  {} },                                       // no snapping: the theremin proper
    { "CHROMA", 12, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
    { "MAJOR",  7,  { 0, 2, 4, 5, 7, 9, 11 } },
    { "MINOR",  7,  { 0, 2, 3, 5, 7, 8, 10 } },
    { "PENTA",  5,  { 0, 3, 5, 7, 10 } },                       // minor pentatonic: no wrong notes
};
static const int SCALE_COUNT = sizeof(SCALES) / sizeof(SCALES[0]);

// ---- Shared between the two tasks ---------------------------------------------------------------
// Each is one word with one writer, so none needs a lock. The first group is what the player or
// the serial link sets; the second is what the synth task reports for loop() to draw.
static volatile int  mode = MODE_THEREMIN;
static volatile int  instrument = 0;
static volatile int  scaleIndex = 0;
static volatile int  centreNote = 60;         // MIDI; the scales are rooted here too
static volatile int  rangeSemis = 12;         // either side of the centre note
static volatile int  vibratoSteps = 4;        // quarters of the instrument's own vibrato depth, 0..10
static volatile int  harpDecay = 10;          // SID decay and release value, 7..13: see RING_TIMES
static volatile int  arpSpeed = 10;           // steps a second with the board level, 4..20
static volatile int  arpPattern = ARP_UP;
static volatile int  arpChord = 0;
static volatile int  echoLevel = 4;           // 0 is off
static volatile int  echoMs = 300;
static volatile int  chipModel = 6581;        // or 8580: the later chip, with a cleaner, stronger filter
static volatile int  sweepSpeed = 0;          // thirds of an octave of speed on a synced voice's movement, -6..+6; 0 is the instrument's own
static volatile int  sweepDepth = 0;          // quarters of an octave of depth, -4..+4
static volatile int  resonance = 8;           // 8 is the instrument's own; either side adds or takes away
static volatile int  knobParam = PARAM_NOTE;
static volatile uint32_t adjustedMs = 0;      // when the knob last chose or changed a setting
static volatile uint32_t knobTurns = 0, knobPresses = 0, button1Presses = 0, button2Presses = 0;   // since boot, for `status`
static volatile bool recentreRequest = true;  // the first pose read becomes neutral
static volatile float baseVolume = 0.3f;
static volatile bool remoteGate = false;
static volatile bool    keysOwnPitch = false;                // the keys have the pitch, not the hand
static volatile int     keysDown = 0;
static volatile uint32_t midiMessages = 0, blePackets = 0;   // since boot, for `status`
static bool bleStarted = false;
static volatile uint32_t imuLogMs = 0;        // the period; 0 is off
static volatile bool gridLog = false;

static volatile bool    imuPresent = false;
static volatile bool    handSensor = false;                  // the distance sensor has delivered a frame
static volatile bool    handSeen = false;                    // a hand is in the playing range now
static volatile int     handMm = 0;
static volatile bool    gateOpen = false;
static volatile float   pitchPos = 0.0f;                     // -1..+1 of the range, from the hand or the tilt
static volatile float   tiltAway = 0.0f, tiltRight = 0.0f;   // -1..+1 of the span, smoothed
static volatile float   rawPitch = 0.0f, rawRoll = 0.0f;     // the driver's angles, for imu_log
static volatile float   shownNote = 60.0f;                   // MIDI, fractional: the glide without the vibrato
static volatile uint8_t envelope = 0;
static volatile int     outputPeak = 0;                      // largest sample sent to the speaker since the log last asked, of 32767
static volatile uint32_t audioMaxGapMs = 0;                  // longest wait between two I2S writes; over 46 is a dropout

static SidChip sid;                              // cRSID's SID, the one the tune player runs: see sid_chip.h

static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float noteToHz(float note) { return 440.0f * powf(2.0f, (note - 69.0f) / 12.0f); }
static inline float smoothing(float stepMs, float tauMs) { return 1.0f - expf(-stepMs / tauMs); }

/// The note of scale `s` nearest to `note`, keeping `held` until another is clearly nearer.
static float snapToScale(float note, float held, const Scale& s) {
    if (s.count == 0) { return note; }
    int root = centreNote;
    int octave = (int) floorf((note - root) / 12.0f);
    float best = held, bestDist = 1e9f;
    for (int o = octave - 1; o <= octave + 1; o++) {
        for (uint8_t i = 0; i < s.count; i++) {
            float candidate = root + 12 * o + s.steps[i];
            float dist = fabsf(note - candidate);
            if (dist < bestDist) { bestDist = dist; best = candidate; }
        }
    }
    if (best != held && bestDist + SNAP_HYSTERESIS > fabsf(note - held)) { return held; }
    return best;
}

/// The fret below `note`: the highest note of scale `s` not above it. A fret counts as soon as the
/// hand reaches it; going back down, `held` is kept until the hand is clearly below it, so a hand
/// on a fret does not flutter.
static float fretBelow(float note, float held, const Scale& s) {
    int root = centreNote;
    int octave = (int) floorf((note - root) / 12.0f);
    float below = -1000.0f;
    for (int o = octave - 1; o <= octave; o++) {
        for (uint8_t i = 0; i < s.count; i++) {
            float fret = root + 12 * o + s.steps[i];
            if (fret <= note && fret > below) { below = fret; }
        }
    }
    if (below < held && note > held - SNAP_HYSTERESIS) { return held; }
    return below;
}

/// The note `steps` scale steps above `from`, itself a note of scale `s` rooted at the centre note.
static float scaleStepsAbove(float from, int steps, const Scale& s) {
    int root = centreNote, rel = (int) lroundf(from) - root;
    int octave = (int) floorf(rel / 12.0f), index = 0;
    for (uint8_t i = 0; i < s.count; i++) { if (s.steps[i] <= rel - 12 * octave) { index = i; } }
    index += steps;
    return root + 12 * (octave + index / s.count) + s.steps[index % s.count];
}

/// A reading that has to hold for three polls (17 ms) before it counts: the SDK's button and
/// encoder switch have no debounce of their own, and a bounce here would retrigger the envelope.
struct Debounced {
    bool state = false;
    uint8_t run = 0;
    bool changed(bool raw) {
        if (raw == state) { run = 0; return false; }
        if (++run < 3) { return false; }
        state = raw; run = 0;
        return true;
    }
};

static void drawGrids();                                     // defined with the screen below
static void paramValue(int param, char* value, size_t size);

// ---- The synth task: controls, sensors, sound and lights, paced by the I2S writes ------------------
static void readTilt() {
    static float pitch0 = 0.0f, roll0 = 0.0f, away = 0.0f, right = 0.0f;
    if (!imuPresent) { return; }                 // no sensor: tilt stays at zero and the centre note plays
    float p = imu_1.getPitch(), r = imu_1.getRoll();
#if IMU_SWAP_PITCH_ROLL
    float t = p; p = r; r = t;
#endif
    rawPitch = p; rawRoll = r;
    if (recentreRequest) { pitch0 = p; roll0 = r; away = right = 0.0f; recentreRequest = false; }
    float dp = p - pitch0, dr = r - roll0;
    if (dr > 180.0f) { dr -= 360.0f; }           // roll is -180..+180 and wraps
    if (dr < -180.0f) { dr += 360.0f; }
    static const float k = smoothing(1000.0f * CHUNK / SAMPLE_RATE, TILT_SMOOTH_MS);
    away  += k * (clampf(IMU_AWAY_SIGN  * dp / PITCH_SPAN_DEG, -1.0f, 1.0f) - away);
    right += k * (clampf(IMU_RIGHT_SIGN * dr / ROLL_SPAN_DEG,  -1.0f, 1.0f) - right);
    tiltAway = away; tiltRight = right;
}

/// How far the hand is, from the 8x8 frame the driver caches. The driver copies distances without
/// their target status, so a lone zone can be junk: take the four nearest zones in range, drop the
/// nearest, and average the rest.
static float handPos = 0.0f;                     // -1..+1 of the range; the synth task's own, as pitchPos is what it shows

static void readHand() {
    static float pos = 0.0f;
    static uint32_t lastSeenMs = 0;
    handSensor = distance_sensor_1.isReady();
    if (!handSensor) { return; }
    int16_t grid[VL53L5CX_Sensor::GRID_SIZE];
    distance_sensor_1.getDistanceGrid(grid);
    int nearest[4] = { 32767, 32767, 32767, 32767 }, zones = 0;
    for (int16_t d : grid) {
        if (d < 20 || d > HAND_FAR_MM) { continue; }        // 0 is no target; under 20 mm is the cover glass
        zones++;
        for (int i = 0, v = d; i < 4; i++) { if (v < nearest[i]) { int t = nearest[i]; nearest[i] = v; v = t; } }
    }
    bool wasSeen = handSeen;
    if (zones >= HAND_MIN_ZONES) {
        int mm = zones >= 4 ? (nearest[1] + nearest[2] + nearest[3]) / 3 : (nearest[1] + nearest[2]) / 2;
        float target = clampf(1.0f - 2.0f * (mm - HAND_NEAR_MM) / (float)(HAND_FAR_MM - HAND_NEAR_MM), -1.0f, 1.0f);
        static const float k = smoothing(1000.0f * CHUNK / SAMPLE_RATE, HAND_SMOOTH_MS);
        pos = wasSeen ? pos + k * (target - pos) : target;   // a hand arriving starts where it is, not where the last one left
        handMm = mm;
        lastSeenMs = millis();
    }
    handSeen = zones >= HAND_MIN_ZONES || (wasSeen && millis() - lastSeenMs < HAND_GONE_MS);
    handPos = pos;                                           // held when the hand leaves, so the note dies at its pitch
}

// ---- Voices ---------------------------------------------------------------------------------------
// THEREMIN and ARPEGGIO play one note on the instrument's voices: voice 1 the lead, voice 2 its
// second, voice 3 unheard as the ring or sync source. HARP gives each voice a string of its own —
// except with an instrument that rings or syncs, which needs voice 3 beside voice 1 and so has the
// one string, on the same voices as the other modes: `strings` below is the first case.
static bool voiceGate[3] = { false, false, false };
static uint32_t voiceOnMs[3] = { 0, 0, 0 };      // when each gate last opened: the strike and the filter sweep count from it
static bool droneHeld = false;                   // button 1 held past BUTTON_HOLD_MS

static void gateVoice(int v, bool open) {
    if (open && !voiceGate[v]) { voiceOnMs[v] = millis(); }
    voiceGate[v] = open;
}

/// Close a voice's gate on the chip now, not at the next pass over the controls: what follows — a
/// pluck, another instrument's envelope — has to find the voice released. On a SID an envelope
/// settles only when it *equals* the sustain level: give a held note a higher sustain than the
/// level it is at and it counts down to zero and stays there, gate open, until the gate is closed
/// and opened again. So nothing here changes an envelope under an open gate.
static void releaseVoice(int v) {
    voiceGate[v] = false;
    sid.setControl(v, sid.control(v) & ~SID_CTRL_GATE);
}

/// The control register voice `v` should have now. Writing it is what opens and closes the gate.
static uint8_t controlFor(int v, const Instrument& inst, bool strings) {
    bool striking = inst.strike && voiceGate[v] && millis() - voiceOnMs[v] < STRIKE_MS;
    uint8_t bits;
    if (strings)     { bits = striking ? inst.strike : inst.wave; }         // a string's neighbour is another string
    else if (v == 0) { bits = striking ? inst.strike : (inst.wave | inst.mod); }
    else if (v == 1) { bits = inst.secondWave; }
    else             { bits = inst.mod ? SID_CTRL_TRIANGLE : 0; }           // its oscillator is all that is wanted of it
    return bits | (voiceGate[v] ? SID_CTRL_GATE : 0);
}

/// A string, struck from wherever its envelope is and left to fade: the next voice in turn, or, where
/// there is the one string, voices 1 and 2, whose pitch the synth task then sets.
static void pluck(float note, const Instrument& inst, bool strings) {
    static int next = 0;
    int first = 0, count = inst.second > 0.0f ? 2 : 1;
    if (strings) {
        first = next; count = 1;
        next = (next + 1) % 3;
        sid.setFreqHz(first, noteToHz(note));
    }
    for (int v = first; v < first + count; v++) {
        releaseVoice(v);                         // the gate has to close to open again
        gateVoice(v, true);
    }
}

static void setEnvelopes(const Instrument& inst, bool plucked) {
    if (plucked) {
        for (int v = 0; v < 3; v++) { sid.setAdsr(v, min((int)inst.a, 4), harpDecay, 0, harpDecay); }   // up, then RING down to nothing
    } else {
        sid.setAdsr(0, inst.a, inst.d, inst.s, inst.r);
        sid.setAdsr(1, inst.a, inst.d, min(inst.s, inst.secondLevel), inst.r);
    }
}

/// One click of the knob on the setting it is on. Each stops at its ends rather than wrapping.
static void adjustParam(int clicks) {
    switch (knobParam) {
    case PARAM_NOTE:    centreNote = constrain(centreNote + clicks, CENTRE_MIN, CENTRE_MAX); break;
    case PARAM_SCALE:   scaleIndex = constrain(scaleIndex + clicks, 0, SCALE_COUNT - 1); break;
    case PARAM_RANGE:   rangeSemis = constrain(rangeSemis + 6 * clicks, 6, 24); break;
    case PARAM_VIBRATO: vibratoSteps = constrain(vibratoSteps + clicks, 0, 10); break;
    case PARAM_RING:    harpDecay = constrain(harpDecay + clicks, 7, 13); break;
    case PARAM_SPEED:   arpSpeed = constrain(arpSpeed + clicks, 4, 20); break;
    case PARAM_PATTERN: arpPattern = constrain(arpPattern + clicks, 0, ARP_PATTERN_COUNT - 1); break;
    case PARAM_CHORD:   arpChord = constrain(arpChord + clicks, 0, CHORD_COUNT - 1); break;
    case PARAM_RESO:    resonance = constrain(resonance + clicks, 0, 15); break;
    case PARAM_ECHO:    echoLevel = constrain(echoLevel + clicks, 0, 10); break;
    case PARAM_ECHO_TIME: echoMs = constrain(echoMs + 30 * clicks, 60, 480); break;
    case PARAM_CHIP:    chipModel = clicks > 0 ? 8580 : 6581; break;
    case PARAM_SWEEP:   sweepSpeed = constrain(sweepSpeed + clicks, -6, 6); break;
    case PARAM_DEPTH:   sweepDepth = constrain(sweepDepth + clicks, -4, 4); break;
    case PARAM_VOL:     baseVolume = constrain(baseVolume + 0.05f * clicks, 0.0f, 1.0f); break;
    }
}

/// Where the knob's setting is within its range, 0..1, for the ring.
static float paramFraction() {
    switch (knobParam) {
    case PARAM_NOTE:    return (centreNote - CENTRE_MIN) / (float)(CENTRE_MAX - CENTRE_MIN);
    case PARAM_SCALE:   return scaleIndex / (float)(SCALE_COUNT - 1);
    case PARAM_RANGE:   return (rangeSemis - 6) / 18.0f;
    case PARAM_VIBRATO: return vibratoSteps / 10.0f;
    case PARAM_RING:    return (harpDecay - 7) / 6.0f;
    case PARAM_SPEED:   return (arpSpeed - 4) / 16.0f;
    case PARAM_PATTERN: return arpPattern / (float)(ARP_PATTERN_COUNT - 1);
    case PARAM_CHORD:   return arpChord / (float)(CHORD_COUNT - 1);
    case PARAM_RESO:    return resonance / 15.0f;
    case PARAM_ECHO:    return echoLevel / 10.0f;
    case PARAM_ECHO_TIME: return (echoMs - 60) / 420.0f;
    case PARAM_CHIP:    return chipModel == 8580 ? 1.0f : 0.0f;
    case PARAM_SWEEP:   return (sweepSpeed + 6) / 12.0f;
    case PARAM_DEPTH:   return (sweepDepth + 4) / 8.0f;
    default:            return baseVolume;
    }
}

/// The setting after `from` that the playing mode has; `from` itself counts when `inclusive`.
static inline float sweepSpeedFactor() { return powf(2.0f, sweepSpeed / 3.0f); }     // 0.25 .. 4
static inline float sweepDepthFactor() { return powf(2.0f, sweepDepth / 4.0f); }     // 0.5 .. 2

/// Whether the knob's menu has setting `i` now: the mode's own, and a synced instrument's own.
static bool paramShown(int i) {
    if (!(PARAMS[i].modes & IN_MODE(mode))) { return false; }
    return PARAMS[i].modes != IN_SYNC || INSTRUMENTS[instrument].mod == SID_CTRL_SYNC;
}

static int nextParam(int from, bool inclusive) {
    for (int i = inclusive ? 0 : 1; i <= PARAM_COUNT; i++) {
        int candidate = (from + i) % PARAM_COUNT;
        if (paramShown(candidate)) { return candidate; }
    }
    return PARAM_NOTE;
}

/// Which of a chord's `n` tones sounds at `step` of the pattern; RAND never repeats the one before.
static int arpToneAt(int pattern, uint32_t step, int n, int previous) {
    switch (pattern) {
    case ARP_DOWN:   return n - 1 - (int)(step % n);
    case ARP_UPDOWN: { int period = 2 * n - 2, k = step % period; return k < n ? k : period - k; }
    case ARP_RANDOM: { int t = esp_random() % (n - 1); return t >= previous ? t + 1 : t; }
    default:         return step % n;
    }
}

static void readControls() {
    static Debounced b1, wave, knob;
    static int32_t lastKnob = 0;
    static uint32_t b1DownMs = 0, knobDownMs = 0;
    static bool knobHeld = false;

    rotary_encoder_1.update(); button_1.update(); button_2.update();

    // button 1: a press that ends before the hold time is "next mode"; one that lasts sounds the note
    if (b1.changed(button_1.isPressed())) {
        if (b1.state) { b1DownMs = millis(); button1Presses = button1Presses + 1; }
        else if (millis() - b1DownMs < BUTTON_HOLD_MS) {
            mode = (mode + 1) % MODE_COUNT;
            knobParam = nextParam(knobParam, true);          // off a setting the new mode does not have
        }
    }
    droneHeld = (b1.state && millis() - b1DownMs >= BUTTON_HOLD_MS) || remoteGate;
    if (wave.changed(button_2.isPressed()) && wave.state) { instrument = (instrument + 1) % INSTRUMENT_COUNT; button2Presses = button2Presses + 1; }

    int32_t pos = knobTurn.position();
    if (pos != lastKnob) {
        adjustParam((int)(pos - lastKnob));
        lastKnob = pos;
        adjustedMs = millis();
        knobTurns = knobTurns + 1;
    }
    // a press that ends before the hold time moves the knob to the next setting; one that lasts recentres, once
    if (knob.changed(rotary_encoder_1.isPressed())) {
        if (knob.state) { knobDownMs = millis(); knobHeld = false; knobPresses = knobPresses + 1; }
        else if (!knobHeld) { knobParam = nextParam(knobParam, false); adjustedMs = millis(); }
    }
    if (knob.state && !knobHeld && millis() - knobDownMs >= KNOB_HOLD_MS) { knobHeld = true; recentreRequest = true; }
    rotary_encoder_1.wasPressed();               // the edge is handled above
}

// ---- Keys: MIDI in -----------------------------------------------------------------------------------
// Messages come from the Bluetooth host's task and from loop() (the serial actions), and are played
// by the synth task: a queue between them. Everything below the queue belongs to the synth task.
struct MidiMessage { uint8_t status, d1, d2; };
static QueueHandle_t midiQueue = nullptr;

static void midiReceive(void*, uint8_t status, uint8_t d1, uint8_t d2) {
    MidiMessage m = { status, d1, d2 };
    if (midiQueue && xQueueSend(midiQueue, &m, 0) == pdTRUE) { midiMessages = midiMessages + 1; }
}
static void blePacketReceive(const uint8_t* packet, size_t length) {
    blePackets = blePackets + 1;
    bleMidiParse(packet, length, midiReceive, nullptr);
}

static KeyStack keys;
static struct { uint8_t note; bool legato; } struck[6];      // the keys that went down since the last look
static int struckCount = 0;
static bool keysRetrigger = false;               // a key went down with none held: a new note, not a slide
static uint32_t keysLastMs = 0;
static float bendSemis = 0.0f, modWheel = 0.0f, ccBright = 0.0f;

static void readKeys() {
    struckCount = 0;
    keysRetrigger = false;
    MidiMessage m;
    while (midiQueue && xQueueReceive(midiQueue, &m, 0) == pdTRUE) {
        const uint8_t type = m.status & 0xF0;    // omni: the channel is not looked at
        if (type == 0x90 && m.d2 > 0) {
            if (keys.count == 0) { keysRetrigger = true; }
            if (struckCount < 6) { struck[struckCount].note = m.d1; struck[struckCount].legato = keys.count > 0; struckCount++; }
            keys.press(m.d1);
            keysLastMs = millis();
        } else if (type == 0x80 || type == 0x90) {           // a note-on of velocity 0 is a note-off
            keys.release(m.d1);
            keysLastMs = millis();
        } else if (type == 0xE0) {
            bendSemis = ((int)((m.d2 << 7) | m.d1) - 8192) / 8192.0f * BEND_SEMITONES;
        } else if (type == 0xB0) {
            if (m.d1 == 1)        { modWheel = m.d2 / 127.0f; }
            else if (m.d1 == 74)  { ccBright = clampf((m.d2 - 64) / 63.0f, -1.0f, 1.0f); }
            else if (m.d1 == 120 || m.d1 == 123) { keys.clear(); }
        } else if (type == 0xC0) {
            instrument = m.d1 % INSTRUMENT_COUNT;
        }
    }
    keysDown = keys.count;
    keysOwnPitch = keys.count > 0 || (keysLastMs != 0 && millis() - keysLastMs < KEYS_HOLD_MS);
}

// ---- Echo -------------------------------------------------------------------------------------------
// A feedback delay on the mix, after the SID: the one thing here a C64 could not do, which is why
// its composers faked it with a second voice playing the tune late and quiet. Each repeat goes
// round through a gentle low-pass, so it comes back duller as well as quieter, as a tape echo's do.
// ECHO turns up the repeats' level and their number together.
static const int ECHO_LINE = SAMPLE_RATE / 2;    // half a second: 43 KB
static int16_t echoLine[ECHO_LINE];

static void echo(int16_t* buf, int frames) {
    static int at = 0;
    static float dull = 0.0f;
    const int level = echoLevel;
    const float wet = 0.07f * level, feedback = level ? 0.2f + 0.045f * level : 0.0f;   // 10: 0.7 and 0.65
    const int delay = constrain((int)((int64_t)echoMs * SAMPLE_RATE / 1000), 1, ECHO_LINE - 1);
    for (int i = 0; i < frames; i++) {
        int from = at - delay;
        if (from < 0) { from += ECHO_LINE; }
        float repeat = echoLine[from];
        dull += 0.45f * (repeat - dull);
        float dry = buf[i];
        echoLine[at] = (int16_t) clampf(dry + feedback * dull, -32768.0f, 32767.0f);
        buf[i] = (int16_t) clampf(dry + wet * repeat, -32768.0f, 32767.0f);
        if (++at == ECHO_LINE) { at = 0; }
    }
}

static void synthTask(void*) {
    static int16_t buf[CHUNK];
    const float blockMs = 1000.0f * BLOCK / SAMPLE_RATE, chunkMs = 1000.0f * CHUNK / SAMPLE_RATE;
    const float TWO_PI_F = 2.0f * (float)M_PI;
    float note = centreNote, held = centreNote, lfo = 0.0f, pwmLfo = 0.0f, wahLfo = 0.0f;
    float lastString = -1.0f, arpClockMs = 0.0f, slid = centreNote;
    uint32_t arpStep = 0, stepMs = 0;           // stepMs: the latest arpeggio step, hammered fret or key, which a synced voice's strike sweeps into
    uint32_t noteMs = 0;                        // the same without the arpeggio's steps: where a synced voice's LFO starts from
    int arpTone = 0;
    bool wasPresent = false, wasDrone = false;
    uint32_t lastWriteMs = millis();
    int appliedInstrument = -1, appliedMode = -1, appliedDecay = harpDecay;
    bool ringShowsSetting = false;
    uint32_t chunks = 0;

    for (;;) {
        readTilt();
        readHand();
        readControls();
        readKeys();
        const bool keysOwn = keysOwnPitch;
        const float bright = clampf(tiltRight + ccBright, -1.0f, 1.0f);   // the tilt to the right, and CC 74
        const float position = handSensor ? handPos : tiltAway;   // no distance sensor: the tilt is the pitch

        if (chipModel != sid.model()) { sid.setModel(chipModel); }
        if (mode != appliedMode) {               // button 1 or the serial link changed it
            appliedMode = mode;
            appliedInstrument = -1;              // the envelopes are the mode's as much as the instrument's
            gateOpen = false; wasPresent = false; lastString = -1.0f; arpStep = 0; arpClockMs = 0.0f;
        }
        const bool harp = appliedMode == MODE_HARP, frets = appliedMode == MODE_FRETS;
        const bool plucked = harp || frets;      // notes that are struck and fade, however long the hand stays
        const Instrument& inst = INSTRUMENTS[instrument];
        const bool strings = harp && !inst.mod;  // three strings of their own; see Voices
        if (instrument != appliedInstrument || harpDecay != appliedDecay) {     // button 2, RING, or the serial link
            // another instrument, or another mode: a note that is held starts afresh with the new envelope,
            // and strings stop, since the voices may change roles. RING alone leaves strings ringing: their
            // sustain stays 0, which every level can reach
            if (instrument != appliedInstrument) { for (int v = 0; v < 3; v++) { releaseVoice(v); } }
            if (instrument != appliedInstrument) { sweepSpeed = 0; sweepDepth = 0; knobParam = nextParam(knobParam, true); }
            appliedInstrument = instrument;
            appliedDecay = harpDecay;
            setEnvelopes(inst, plucked);         // strings already sounding fade at the new rate too
        }
        // with a hand for the pitch, tilting away or towards is the theremin's other antenna: the
        // volume, except in ARPEGGIO, where it is the speed
        bool tiltIsFree = handSensor || keysOwn;
        float swell = tiltIsFree && appliedMode != MODE_ARP ? powf(2.0f, SWELL_OCTAVES * tiltAway) : 1.0f;
        speaker_1.setVolume(clampf(baseVolume * swell, 0.0f, 0.9f));

        // strings and frets are notes, so FREE there is every semitone
        const Scale& scale = SCALES[plucked && scaleIndex == 0 ? 1 : scaleIndex];
        if (keysOwn) {
            // a keyboard has its own notes; only an arpeggio's root is taken to the scale, which its chord is built in
            if (keys.count > 0) { held = appliedMode == MODE_ARP && scale.count ? snapToScale(keys.top(), held, scale) : (float)keys.top(); }
        } else {
            held = frets ? fretBelow(centreNote + position * rangeSemis, held, scale)
                         : snapToScale(centreNote + position * rangeSemis, held, scale);
        }
        // written once a pass: loop() draws from it on the other core, and must not see a value on its way
        pitchPos = keysOwn ? clampf((held - centreNote) / rangeSemis, -1.0f, 1.0f) : position;
        const float glide = smoothing(blockMs, scale.count ? GLIDE_SNAP_MS : GLIDE_FREE_MS);
        bool playing = droneHeld || (keysOwn ? keys.count > 0 : handSeen);
        if (keysRetrigger) { note = held; }      // a new note starts on its pitch

        float vibrato = 0.0f, arpNote = held;
        bool arpGlides = false;
        if (plucked && keysOwn) {
            // every key plucks; in FRETS a key pressed while another is held is a hammer-on, and letting
            // it go, a pull-off back to the key still held
            for (int k = 0; k < struckCount; k++) {
                bool hammer = frets && struck[k].legato && sid.envelope(0) >= FRET_REPLUCK_BELOW;
                if (hammer) { stepMs = noteMs = millis(); } else { pluck(struck[k].note, inst, strings); }
                lastString = struck[k].note;
            }
            if (frets && keys.count > 0 && lastString != (float)keys.top()) { lastString = keys.top(); stepMs = noteMs = millis(); }
            if (droneHeld && !wasDrone) { pluck(held, inst, strings); }
            wasPresent = false;                  // the hand, when it has the pitch back, arrives afresh
        } else if (harp) {
            // the hand plucks each string it arrives at; without a distance sensor the tilt always does
            bool present = handSensor ? handSeen : true;
            if ((present && (held != lastString || !wasPresent)) || (droneHeld && !wasDrone)) { pluck(held, inst, strings); }
            if (present) { lastString = held; }
            wasPresent = present;
        } else if (frets) {
            // a hand arriving plucks the string; a new fret on a ringing string is a hammer-on — the pitch
            // changes, a synced voice and the filter sweep into it, the envelope fades on — and on a string
            // that has all but died it is a pluck
            bool present = handSensor ? handSeen : true;
            bool moved = present && held != lastString, quiet = sid.envelope(0) < FRET_REPLUCK_BELOW;
            if ((present && !wasPresent) || (moved && quiet) || (droneHeld && !wasDrone)) { pluck(held, inst, false); }
            else if (moved) { stepMs = noteMs = millis(); }
            if (present) { lastString = held; }
            wasPresent = present;
        } else {
            if (keysRetrigger && voiceGate[0]) { releaseVoice(0); releaseVoice(1); }   // key up and key down inside one look
            if (struckCount) { stepMs = noteMs = millis(); } // a synced voice sweeps into every key
            gateVoice(0, playing);
            gateVoice(1, playing && inst.second > 0.0f);
            gateVoice(2, false);
            if (appliedMode == MODE_ARP) {
                const Chord& chord = CHORDS[arpChord];
                float stepsPerS = arpSpeed * (tiltIsFree ? powf(2.0f, ARP_RATE_OCTAVES * tiltAway) : 1.0f);
                arpClockMs += chunkMs;
                if (!playing) {                              // every note starts at the top of its pattern
                    arpClockMs = 0.0f; arpStep = 0;
                    arpTone = arpToneAt(arpPattern, 0, chord.count, arpTone);
                } else if (arpClockMs >= 1000.0f / stepsPerS) {
                    arpClockMs = 0.0f;
                    arpTone = arpToneAt(arpPattern, ++arpStep, chord.count, arpTone);
                    stepMs = millis();                    // a synced voice sweeps into every step
                }
                arpTone = min(arpTone, (int)chord.count - 1);            // CHORD may just have changed to a shorter one
                arpGlides = scale.count == 0 || scale.count == 12;
                arpNote = arpGlides ? chord.semis[arpTone]                   // added to the gliding note below
                                    : scaleStepsAbove(held, chord.steps[arpTone], scale) + 12 * chord.octaves[arpTone];
            } else if (playing) {
                float heldMs = (float)(millis() - voiceOnMs[0]) - inst.vibratoAfterMs;
                vibrato = inst.vibrato * vibratoSteps / 4.0f * clampf(heldMs / VIBRATO_RAMP_MS, 0.0f, 1.0f);
            }
        }
        gateOpen = playing;
        wasDrone = droneHeld;
        for (int v = 0; v < 3; v++) {            // the gates, the strike running out, a new instrument's waveform
            uint8_t control = controlFor(v, inst, strings);
            if (control != sid.control(v)) { sid.setControl(v, control); }
        }

        // ring and sync: voice 3 runs unheard beside voice 1. Ringing, it is the one off at a ratio; syncing,
        // it holds the note and voice 1 is the one off at a ratio, torn back to the note's period each cycle.
        // A fixed ratio is a fixed timbre, and a dead one: the synced voice is kept moving — see Instrument
        float syncOctaves = 0.0f;
        if (inst.mod == SID_CTRL_SYNC) {
            const uint32_t now = millis();
            const float sinceStrike = (float)(now - max(voiceOnMs[0], stepMs)), sinceNote = (float)(now - max(voiceOnMs[0], noteMs));
            const float faster = sweepSpeedFactor(), deeper = sweepDepthFactor();     // the SWEEP and DEPTH settings
            const float strike = inst.modSweepMs ? inst.modSweep * deeper * expf(-sinceStrike * faster / inst.modSweepMs) : 0.0f;
            const float turn = sinceNote / 1000.0f * inst.modLfoHz * (inst.modTriangle ? faster : 1.0f), phase = turn - floorf(turn);
            const float wave = inst.modTriangle ? 1.0f - fabsf(2.0f * phase - 1.0f)       // 0 up to 1 and back: from the ratio, up first
                                                : sinf(TWO_PI_F * phase);
            const float arrived = inst.modSweepMs ? 1.0f - expf(-sinceNote * faster / inst.modSweepMs) : 1.0f;   // the LFO comes in as the strike dies
            syncOctaves = strike + inst.modLfo * (inst.modTriangle ? deeper : 1.0f) * wave * arrived + SYNC_TILT_OCTAVES * bright;
        }

        for (int b = 0; b < CHUNK; b += BLOCK) {
            note += glide * (held - note);
            lfo += TWO_PI_F * VIBRATO_HZ * blockMs / 1000.0f;
            if (lfo > TWO_PI_F) { lfo -= TWO_PI_F; }
            if (!strings) {                      // a harp's strings keep the pitch they were plucked at
                float sounding = plucked ? (lastString >= 0.0f ? lastString : held)
                               : appliedMode == MODE_ARP ? (arpGlides ? note + arpNote : arpNote) : note;
                // portamento, where the instrument has it; a note after a silence starts on its pitch, and so does a string
                if (inst.glideMs && playing && !plucked && !keysRetrigger) { slid += smoothing(blockMs, inst.glideMs) * (sounding - slid); } else { slid = sounding; }
                float hz = noteToHz(min(slid + bendSemis + (vibrato + MOD_WHEEL_SEMITONES * modWheel) * sinf(lfo), HIGHEST_NOTE));
                // under 1 the sync has nothing to tear
                const float modRatio = inst.mod == SID_CTRL_SYNC ? clampf(inst.modRatio * powf(2.0f, syncOctaves), 1.02f, 16.0f) : inst.modRatio;
                sid.setFreqHz(0, inst.mod == SID_CTRL_SYNC ? hz * modRatio : hz);
                sid.setFreqHz(1, hz * inst.second);
                sid.setFreqHz(2, inst.mod == SID_CTRL_RING ? hz * modRatio : hz);
            }
            sid.render(buf + b, BLOCK);
        }

        // the filter follows the note, so a tilt means the same brightness anywhere in the range: the
        // instrument's own cutoff, two octaves either way with the tilt, and its sweep dying away
        // from the latest strike
        float follow = plucked ? (lastString >= 0.0f ? lastString : held) : note;
        uint32_t struckMs = max(max(voiceOnMs[0], frets ? stepMs : 0), max(voiceOnMs[1], voiceOnMs[2]));
        float sweep = inst.sweepMs ? inst.sweep * expf(-(float)(millis() - struckMs) / inst.sweepMs) : 0.0f;
        wahLfo += TWO_PI_F * inst.wahHz * chunkMs / 1000.0f;
        if (wahLfo > TWO_PI_F) { wahLfo -= TWO_PI_F; }
        sweep += inst.wah * sinf(wahLfo);
        // voice 3 as a ring or sync source is kept out of the mix the way C64 tunes keep it out: off the
        // filter's input and behind the 3OFF bit. On a 6581 a silent voice is not quite silent
        const bool voice3Unheard = inst.mod && !strings;
        sid.setFilter(noteToHz(follow) * powf(2.0f, inst.cutoff + 2.0f * bright + sweep),
                      (uint8_t)constrain((int)inst.resonance + resonance - 8, 0, 15),
                      inst.filter ? (voice3Unheard ? 0x03 : 0x07) : 0x00,
                      inst.filter | inst.level | (voice3Unheard ? SID_FILT_3OFF : 0));
        pwmLfo += TWO_PI_F * inst.pwmHz * chunkMs / 1000.0f;
        if (pwmLfo > TWO_PI_F) { pwmLfo -= TWO_PI_F; }
        for (int v = 0; v < 3; v++) { sid.setPulseWidth(v, clampf(inst.pulse + inst.pwmDepth * sinf(pwmLfo + 0.9f * v), 0.05f, 0.95f)); }

        shownNote = follow;
        envelope = max(sid.envelope(0), max(sid.envelope(1), sid.envelope(2)));
        if ((++chunks & 7) == 0) {               // 20 Hz
            // the ring points at the pitch, 12 o'clock = the centre note; for a moment after the knob
            // is touched it is orange and shows the setting instead, 7 o'clock lowest to 5 o'clock highest
            bool showSetting = millis() - adjustedMs < ADJUST_SHOW_MS && adjustedMs != 0;
            if (showSetting != ringShowsSetting) {
                ringShowsSetting = showSetting;
                if (showSetting) { rotary_encoder_1.setRingColor(255, 110, 0); } else { rotary_encoder_1.setRingColor(0, 200, 255); }
            }
            float hours = showSetting ? (paramFraction() - 0.5f) * 10.0f                 // +-5, clockwise is up
                                      : (follow - centreNote) / rangeSemis * 5.5f;       // +-5.5
            rotary_encoder_1.setRingPosition(fmodf(12.0f - hours + 12.0f, 12.0f));   // the ring counts anticlockwise
            drawGrids();                         // four short strips: the I2S DMA absorbs it
        }

        // cRSID mixes one chip at a quarter of full scale a voice; twice that still leaves the echo room
        for (int i = 0; i < CHUNK; i++) { buf[i] = (int16_t) constrain(2 * (int)buf[i], -32768, 32767); }
        echo(buf, CHUNK);
        int peak = outputPeak;
        for (int i = 0; i < CHUNK; i++) { peak = max(peak, abs((int)buf[i])); }
        outputPeak = peak;
        speaker_1.writeSamples(buf, CHUNK);      // blocks on the I2S DMA — that is what paces this task
        uint32_t now = millis();
        if (now - lastWriteMs > audioMaxGapMs) { audioMaxGapMs = now - lastWriteMs; }
        lastWriteMs = now;
    }
}

// ---- Drawing: the grids from the synth task, the screen from loop() -------------------------------
static void hueToRgb(float hue, float value, uint8_t* r, uint8_t* g, uint8_t* b) {
    float h = hue * 6.0f;
    int sector = (int) h % 6;
    float f = h - floorf(h), q = 1.0f - f;
    float rgb[6][3] = { {1, f, 0}, {q, 1, 0}, {0, 1, f}, {0, q, 1}, {f, 0, 1}, {1, 0, q} };
    *r = (uint8_t)(rgb[sector][0] * value * 255.0f);
    *g = (uint8_t)(rgb[sector][1] * value * 255.0f);
    *b = (uint8_t)(rgb[sector][2] * value * 255.0f);
}

/// Both grids as one meter of what is sounding, drawn by glass geometry so both show it the same
/// way round. The bar's height is the pitch within the range, filling from the bottom row with
/// the top lit row fading in, as the Pocket Synth's VU does; its colour runs from blue at the
/// bottom of the range to red at the top; its brightness is the envelope, so it lights when a
/// note sounds and dies away with it. Tilting left or right leans the bar to that side's columns.
static volatile uint32_t gridFrames = 0;

static void drawGrids() {
    float range = (pitchPos + 1.0f) * 0.5f;                  // 0..1 of the playing range
    float height = 0.5f + 2.5f * range;                      // rows lit: the lowest note still shows half a row
    // a trace when silent, so the board does not look off. The grid driver scales everything by its
    // brightness, 15%: under 0.07 here the trace rounds to nothing, which is how the emulator found it
    float value = 0.08f + 0.55f * (float)envelope / 255.0f;
    float lean = tiltRight;
    for (uint8_t row = 0; row < 3; row++) {
        float lit = clampf(height - (float)(2 - row), 0.0f, 1.0f);              // row 2, the bottom, lights first
        for (uint8_t col = 0; col < 3; col++) {
            float side = clampf(1.0f - fabsf(lean) * fabsf(1.0f + lean - col) * 0.5f, 0.0f, 1.0f);
            uint8_t r, g, b;
            hueToRgb(0.66f * (1.0f - range), value * lit * side, &r, &g, &b);
            gridSetGlassXY(light_grid_7,  GRID_GLASS_PORT7,  row, col, r, g, b);
            gridSetGlassXY(light_grid_11, GRID_GLASS_PORT11, row, col, r, g, b);
        }
    }
    light_grid_7.show();
    light_grid_11.show();
    gridFrames = gridFrames + 1;
}

static void paramValue(int param, char* value, size_t size) {
    switch (param) {
    case PARAM_NOTE:    snprintf(value, size, "%s%d", NOTE_NAMES[centreNote % 12], centreNote / 12 - 1); break;
    case PARAM_SCALE:   snprintf(value, size, "%s", SCALES[scaleIndex].name); break;
    case PARAM_RANGE:   snprintf(value, size, "+-%d", (int)rangeSemis); break;
    case PARAM_VIBRATO: snprintf(value, size, "x%.2f", vibratoSteps / 4.0f); break;
    case PARAM_RING:    snprintf(value, size, "%s", RING_TIMES[harpDecay - 7]); break;
    case PARAM_SPEED:   snprintf(value, size, "%d/s", (int)arpSpeed); break;
    case PARAM_PATTERN: snprintf(value, size, "%s", ARP_PATTERN_NAMES[arpPattern]); break;
    case PARAM_CHORD:   snprintf(value, size, "%s", CHORDS[arpChord].name); break;
    case PARAM_RESO:    snprintf(value, size, "%+d", (int)resonance - 8); break;
    case PARAM_ECHO:    if (echoLevel) { snprintf(value, size, "%d", (int)echoLevel); } else { snprintf(value, size, "OFF"); } break;
    case PARAM_ECHO_TIME: snprintf(value, size, "%dms", (int)echoMs); break;
    case PARAM_CHIP:    snprintf(value, size, "%d", (int)chipModel); break;
    case PARAM_SWEEP: {
        const Instrument& inst = INSTRUMENTS[instrument];
        float seconds = inst.modTriangle ? 0.5f / (inst.modLfoHz * sweepSpeedFactor()) : inst.modSweepMs / 1000.0f / sweepSpeedFactor();
        snprintf(value, size, "%.2fs", seconds);
        break;
    }
    case PARAM_DEPTH: {
        const Instrument& inst = INSTRUMENTS[instrument];
        snprintf(value, size, "%.1foct", (inst.modTriangle ? inst.modLfo : fabsf(inst.modSweep)) * sweepDepthFactor());
        break;
    }
    default:            snprintf(value, size, "%d%%", (int)lroundf(baseVolume * 100.0f)); break;
    }
}

/// Push the frame, with a square in the top right corner that changes every frame: a screen that
/// has stopped can be told from one with nothing new to show.
static void showFrame() {
    static bool beat = false;
    beat = !beat;
    st7735_tft_1.fillRect(156, 0, 4, 4, beat ? UI_ACCENT : UI_BG);
    st7735_tft_1.display();
}

/// How wide `text` comes out in `font`, in pixels: the display wrapper has no getTextBounds().
static int textWidth(const GFXfont* font, const char* text) {
    int width = 0;
    for (; *text; text++) {
        uint8_t c = (uint8_t)*text;
        if (c >= font->first && c <= font->last) { width += font->glyph[c - font->first].xAdvance; }
    }
    return width;
}

// The views are templates over what they draw on: the display, or a canvas of the same size that
// `screen_dump` sends to the host, so what the firmware draws can be looked at from there.

/// The whole screen as one setting: its name, which of how many, its value, where in its range.
template <class G> static void drawSetting(G& g, const char* name, const char* value, int index, int total, float fraction) {
    g.fillScreen(UI_BG);
    g.setFont(&FreeSansBold9pt7b);
    g.setTextColor(UI_ACCENT);
    g.setCursor(6, 16);
    g.print(name);
    g.setFont(nullptr);
    g.setTextColor(UI_LABEL);
    g.setCursor(124, 8);
    g.print(index); g.print("/"); g.print(total);
    const GFXfont* fonts[3] = { &FreeSansBold18pt7b, &FreeSansBold12pt7b, &FreeSansBold9pt7b };   // the largest the value fits in
    int f = 0;
    while (f < 2 && textWidth(fonts[f], value) > 148) { f++; }
    g.setFont(fonts[f]);
    g.setTextColor(UI_VALUE);
    g.setCursor(8, f == 0 ? 54 : f == 1 ? 50 : 46);
    g.print(value);
    g.drawRect(6, 66, 148, 9, UI_LABEL);
    int fill = (int)(144.0f * clampf(fraction, 0.0f, 1.0f) + 0.5f);
    if (fill > 0) { g.fillRect(8, 68, fill, 5, UI_ACCENT); }
}

/// The setting the knob is on: shown while the knob is in use.
template <class G> static void drawKnobSetting(G& g) {
    int param = knobParam, index = 0, total = 0;
    char value[12];
    paramValue(param, value, sizeof(value));
    for (int i = 0; i < PARAM_COUNT; i++) {
        if (!paramShown(i)) { continue; }
        total++;
        if (i <= param) { index = total; }
    }
    drawSetting(g, PARAMS[param].name, value, index, total, paramFraction());
}

static volatile uint32_t framesDrawn = 0;                    // since boot, for `status`
static volatile uint32_t instrumentChangedMs = 0;            // set by loop(), which is where a change is noticed

/// The playing screen: the mode and the instrument, the note, the hand and the tilt, the knob's setting.
template <class G> static void drawPlaying(G& g) {
    const int bx = 118, by = 24, bs = 36;                    // the tilt box
    int nearest = (int) lroundf(shownNote);
    int cents = (int) lroundf((shownNote - nearest) * 100.0f);

    g.fillScreen(UI_BG);
    g.setFont(&FreeSansBold9pt7b);
    g.setTextColor(UI_ACCENT);
    g.setCursor(6, 16);
    g.print(MODE_NAMES[mode]);
    // the instrument, top right in the built-in 6x8 font, clear of the frame square in the corner: on
    // one line where the mode's name leaves room, and its two words on two lines where it does not
    const char* sound = INSTRUMENTS[instrument].name;
    const char* space = strchr(sound, ' ');
    const int right = 153, modeEnds = 6 + textWidth(&FreeSansBold9pt7b, MODE_NAMES[mode]);
    g.setFont(nullptr);
    g.setTextColor(UI_LABEL);
    if (!space || right - 6 * (int)strlen(sound) >= modeEnds + 4) {
        g.setCursor(right - 6 * (int)strlen(sound), 6);
        g.print(sound);
    } else {
        char first[12];
        snprintf(first, sizeof(first), "%.*s", (int)(space - sound), sound);
        g.setCursor(right - 6 * (int)strlen(first), 2);
        g.print(first);
        g.setCursor(right - 6 * (int)strlen(space + 1), 11);
        g.print(space + 1);
    }

    g.setFont(&FreeSansBold18pt7b);
    g.setTextColor(gateOpen ? UI_VALUE : UI_LABEL);
    g.setCursor(8, 58);
    g.print(NOTE_NAMES[((nearest % 12) + 12) % 12]);
    g.print(nearest / 12 - 1);
    // how far off the named note the pitch is: a tick either side of a centre mark, +-50 cents
    g.drawLine(58, 58, 58, 61, UI_LABEL);                   // clear of the bottom line's capitals, which start at y 64
    g.fillRect(cents >= 0 ? 58 : 58 + cents, 59, abs(cents) + 1, 2, UI_ACCENT);

    const bool keysShown = keysOwnPitch;
    if (handSensor || keysShown) {                           // the hand, or the key: a marker on a bar, top is highest
        const int hx = 104, hw = 7;
        g.drawRect(hx, by, hw, bs, UI_LABEL);
        int my = by + bs / 2 - (int)(pitchPos * (bs / 2 - 3));
        g.fillRect(hx + 1, my - 1, hw - 2, 3, (keysShown ? keysDown > 0 : handSeen) ? UI_ACCENT : 0x39E7);
    }
    g.drawRect(bx, by, bs, bs, UI_LABEL);
    g.drawLine(bx + bs / 2, by + 1, bx + bs / 2, by + bs - 2, 0x39E7);    // dim crosshair
    g.drawLine(bx + 1, by + bs / 2, bx + bs - 2, by + bs / 2, 0x39E7);
    int dx = bx + bs / 2 + (int)(tiltRight * (bs / 2 - 4)), dy = by + bs / 2 - (int)(tiltAway * (bs / 2 - 4));
    g.fillCircle(dx, dy, 3, gateOpen ? UI_ACCENT : UI_LABEL);

    g.setFont(&FreeSans9pt7b);
    g.setCursor(6, 77);
    if (imuPresent || handSensor || keysShown) {
        // the setting the knob is on, and its value
        int param = knobParam;
        char value[12];
        paramValue(param, value, sizeof(value));
        g.setTextColor(UI_ACCENT);
        g.print(PARAMS[param].name);
        g.print(" ");
        g.setTextColor(UI_LABEL);
        g.print(value);
        g.setFont(nullptr);                       // in the built-in 6x8 font: Bluetooth, lit when a host is connected...
        if (bleStarted) {
            g.setTextColor(bleMidiConnected() ? UI_VALUE : 0x39E7);
            g.setCursor(136, 62);
            g.print("BLE");
        }
        g.setTextColor(UI_ACCENT);                // ...and what the pitch follows
        g.setCursor(130, 71);
        g.print(keysShown ? "KEYS" : handSensor ? "HAND" : "TILT");
    } else {
        g.setTextColor(ST7735_TFT::COLOR_RED);
        g.print("NO SENSORS: 13, 14");
    }
}

enum View { VIEW_AUTO, VIEW_PLAY, VIEW_KNOB, VIEW_SOUND };

template <class G> static void drawView(G& g, int view) {
    if (view == VIEW_AUTO) {                     // whichever was touched last, the knob or the instrument button, has the screen for a moment
        bool knobShown = adjustedMs != 0 && millis() - adjustedMs < ADJUST_SCREEN_MS;
        bool soundShown = instrumentChangedMs != 0 && millis() - instrumentChangedMs < ADJUST_SCREEN_MS;
        view = soundShown && (!knobShown || instrumentChangedMs >= adjustedMs) ? VIEW_SOUND : knobShown ? VIEW_KNOB : VIEW_PLAY;
    }
    if (view == VIEW_SOUND) {
        int i = instrument;
        drawSetting(g, "SOUND", INSTRUMENTS[i].name, i + 1, INSTRUMENT_COUNT, i / (float)(INSTRUMENT_COUNT - 1));
    } else if (view == VIEW_KNOB) {
        drawKnobSetting(g);
    } else {
        drawPlaying(g);
    }
}

static void drawScreen() {
    drawView(st7735_tft_1, VIEW_AUTO);
    showFrame();
    framesDrawn = framesDrawn + 1;
}

/// Hex bytes, "90 3c 64", into `out`; how many there were.
static size_t hexBytes(const char* text, uint8_t* out, size_t max) {
    size_t count = 0;
    while (*text && count < max) {
        char* end;
        long byte = strtol(text, &end, 16);
        if (end == text) { break; }
        out[count++] = (uint8_t) byte;
        text = end;
    }
    return count;
}

static void handleMessage(const char* action, const char* value) {
    int n = atoi(value);
    if (strcmp(action, "recenter") == 0)                                  { recentreRequest = true; }
    else if (strcmp(action, "set_instrument") == 0 && n >= 0 && n < INSTRUMENT_COUNT) { instrument = n; }
    else if (strcmp(action, "set_mode") == 0 && n >= 0 && n < MODE_COUNT)   { mode = n; knobParam = nextParam(knobParam, true); }
    else if (strcmp(action, "set_arp") == 0 && n >= 0 && n < ARP_PATTERN_COUNT) { arpPattern = n; }
    else if (strcmp(action, "set_chord") == 0 && n >= 0 && n < CHORD_COUNT) { arpChord = n; }
    else if (strcmp(action, "set_scale") == 0 && n >= 0 && n < SCALE_COUNT) { scaleIndex = n; }
    else if (strcmp(action, "set_center") == 0 && n >= CENTRE_MIN && n <= CENTRE_MAX) { centreNote = n; }
    else if (strcmp(action, "set_volume") == 0 && n >= 0 && n <= 100)     { baseVolume = n / 100.0f; }
    else if (strcmp(action, "status") == 0) {
        char value[12];
        paramValue(knobParam, value, sizeof(value));
        Serial.printf("[status] up %lu ms, %u screen frames; knob position %ld (%u bounces ignored), switch %s, turns seen %u, presses seen %u; button 1 %s (%u presses), button 2 %s (%u presses)\n",
                      (unsigned long)millis(), (unsigned)framesDrawn, (long)knobTurn.position(), (unsigned)knobTurn.bouncesIgnored(), rotary_encoder_1.isPressed() ? "DOWN" : "up",
                      (unsigned)knobTurns, (unsigned)knobPresses, button_1.isPressed() ? "DOWN" : "up", (unsigned)button1Presses,
                      button_2.isPressed() ? "DOWN" : "up", (unsigned)button2Presses);
        Serial.printf("[status] Bluetooth MIDI %s; %u packets, %u MIDI messages taken; %d keys down, the pitch is the %s; free heap %u\n",
                      !bleStarted ? "not started" : bleMidiConnected() ? "connected" : "advertising", (unsigned)blePackets, (unsigned)midiMessages,
                      (int)keysDown, keysOwnPitch ? "keys'" : "hand's", (unsigned)ESP.getFreeHeap());
        Serial.printf("[status] mode %s, knob on %s = %s, instrument %s, last knob use %lu ms ago\n", MODE_NAMES[mode], PARAMS[knobParam].name, value,
                      INSTRUMENTS[instrument].name, adjustedMs ? (unsigned long)(millis() - adjustedMs) : 0UL);
        return;
    }
    else if (strcmp(action, "screen_dump") == 0) {
        int view = !strcmp(value, "play") ? VIEW_PLAY : !strcmp(value, "knob") ? VIEW_KNOB : !strcmp(value, "sound") ? VIEW_SOUND : VIEW_AUTO;
        GFXcanvas16* shot = new GFXcanvas16(160, 80);
        drawView(*shot, view);
        Serial.setTxTimeoutMs(250);              // 51 KB: let the prints wait for the host rather than drop
        Serial.println("[screen] begin 160 80");
        for (int y = 0; y < 80; y++) {
            Serial.print("[screen] ");
            for (int x = 0; x < 160; x++) { Serial.printf("%04x", shot->getPixel(x, y)); }
            Serial.println();
        }
        Serial.println("[screen] end");
        Serial.setTxTimeoutMs(SERIAL_TX_TIMEOUT_MS);
        delete shot;
        return;
    }
    else if (strcmp(action, "note_on") == 0 && n >= 0 && n <= 127)        { midiReceive(nullptr, 0x90, (uint8_t)n, 100); }
    else if (strcmp(action, "note_off") == 0 && n >= 0 && n <= 127)       { midiReceive(nullptr, 0x80, (uint8_t)n, 0); }
    else if (strcmp(action, "midi") == 0 || strcmp(action, "ble_packet") == 0) {
        uint8_t bytes[32];
        size_t count = hexBytes(value, bytes, sizeof(bytes));
        if (action[0] == 'b') { bleMidiParse(bytes, count, midiReceive, nullptr); }
        else if (count >= 1 && (bytes[0] & 0x80)) { midiReceive(nullptr, bytes[0], count > 1 ? bytes[1] : 0, count > 2 ? bytes[2] : 0); }
    }
    else if (strcmp(action, "gate") == 0)                                 { remoteGate = strcmp(value, "off") != 0; }
    else if (strcmp(action, "imu_log") == 0)                              { imuLogMs = strcmp(value, "off") == 0 ? 0 : constrain(n ? n : 200, 20, 5000); }
    else if (strcmp(action, "set_speed") == 0 && n >= 4 && n <= 20)       { arpSpeed = n; }
    else if (strcmp(action, "set_echo") == 0 && n >= 0 && n <= 10)        { echoLevel = n; }
    else if (strcmp(action, "set_echo_ms") == 0 && n >= 60 && n <= 480)   { echoMs = n; }
    else if (strcmp(action, "set_sweep") == 0 && n >= -6 && n <= 6)       { sweepSpeed = n; }
    else if (strcmp(action, "set_depth") == 0 && n >= -4 && n <= 4)       { sweepDepth = n; }
    else if (strcmp(action, "set_chip") == 0 && (n == 6581 || n == 8580)) { chipModel = n; }
    else if (strcmp(action, "grid_log") == 0)                             { gridLog = strcmp(value, "off") != 0; }
    else { Serial.printf("[theremin] ignored %s=%s\n", action, value); return; }
    Serial.printf("[theremin] %s=%s\n", action, value);
}

void setup() {
    // AtechSerial::connect() is a no-op — the generated firmware starts Serial itself, so
    // an app must too.
    Serial.setRxBufferSize(8192);
    Serial.begin(115200);
    Serial.setTxTimeoutMs(SERIAL_TX_TIMEOUT_MS);
    serialLink.connect();
    serialLink.onMessage(handleMessage);

    speaker_1.begin(SAMPLE_RATE);
    speaker_1.setVolume(baseVolume);
    st7735_tft_1.begin();
    delay(150);                                  // allow the display to stabilise
    rotary_encoder_1.begin();
    knobTurn.begin(rotary_encoder_1_pin_clk, rotary_encoder_1_pin_dt);   // after the driver's begin(): see quadrature_knob.h
    rotary_encoder_1.setRingColor(0, 200, 255);
    rotary_encoder_1.setRingBrightness(80);
    button_1.begin(); button_2.begin();
    light_grid_7.begin(); light_grid_11.begin();

    // the distance sensor takes its firmware over I2C at every start: a few seconds when it is there
    imu_1_setup();
    distance_sensor_1_setup();
    imuPresent = imu_1.isConnected();
    Serial.printf("[theremin] IMU in port 14: %s\n", imuPresent ? imu_1.getChipName() : "not found, so no filter or volume from the tilt");
    // the SDK's setup template drops begin()'s result; a first frame within half a second says the same
    for (uint32_t t0 = millis(); !distance_sensor_1.isReady() && millis() - t0 < 500; ) { delay(10); }
    Serial.printf("[theremin] distance sensor in port 13: %s\n",
                  distance_sensor_1.isReady() ? "ranging" : "not found, so the tilt away/towards is the pitch");

    if (!sid.begin(SAMPLE_RATE, chipModel)) { Serial.println("[theremin] no memory for the SID: no sound"); for (;;) { delay(1000); } }
    // the synth task sets the envelopes and the waveform when it enters the first mode

    uint32_t t0 = millis();
    drawScreen();
    Serial.printf("[theremin] one screen frame takes %u ms; the I2S DMA holds %u ms of sound\n",
                  (unsigned)(millis() - t0), (unsigned)(8 * 256 * 1000 / SAMPLE_RATE));

    // Bluetooth last of all that allocates: its host and controller take their memory from what is left
    midiQueue = xQueueCreate(64, sizeof(MidiMessage));
    uint32_t heapBefore = ESP.getFreeHeap();
    bleStarted = bleMidiBegin("SID Theremin", blePacketReceive);
    Serial.printf("[theremin] Bluetooth MIDI %s; it took %u KB of heap, %u KB are free\n", bleStarted ? "advertising as \"SID Theremin\"" : "did not start",
                  (unsigned)((heapBefore - ESP.getFreeHeap()) / 1024), (unsigned)(ESP.getFreeHeap() / 1024));

    // Core 0, above the IMU's poll task: it sleeps in the I2S write most of the time
    xTaskCreatePinnedToCore(synthTask, "Synth", 4096, NULL, 5, NULL, 0);
    Serial.println("[theremin] hold the board flat, USB-C towards you; a hand over the sensor plays, button 1 drones");
}

void loop() {
    static uint32_t lastLogMs = 0, frames = 0;
    static int postedWave = -1, postedScale = -1, postedGate = -1, postedMode = -1, postedParam = -1, postedBle = -1;
    static int32_t postedKnob = 0;

    serialLink.maintain();
    if (postedKnob != knobTurn.position()) {
        postedKnob = knobTurn.position();
        char value[12];
        paramValue(knobParam, value, sizeof(value));
        Serial.printf("[knob] position %ld: %s = %s\n", (long)postedKnob, PARAMS[knobParam].name, value);
    }
    if (bleStarted && postedBle != (int)bleMidiConnected()) { postedBle = bleMidiConnected(); serialLink.postStateEvent("bluetooth_midi", postedBle ? "connected" : "advertising"); }
    if (postedParam != knobParam) { postedParam = knobParam;   serialLink.postStateEvent("knob_setting", PARAMS[postedParam].name); }
    if (postedMode != mode)       { postedMode = mode;         serialLink.postStateEvent("mode", MODE_NAMES[postedMode]); }
    if (postedWave != instrument) {
        if (postedWave >= 0) { instrumentChangedMs = millis(); }            // not at boot
        postedWave = instrument;
        serialLink.postStateEvent("instrument", INSTRUMENTS[postedWave].name);
    }
    if (postedScale != scaleIndex) { postedScale = scaleIndex; serialLink.postStateEvent("scale", SCALES[postedScale].name); }
    if (postedGate != (int)gateOpen) { postedGate = gateOpen;  serialLink.postButtonEvent("gate", postedGate); }

    uint32_t logMs = imuLogMs;
    if (gridLog) {                               // the driver has no frame counter: a frame that differs is a new one
        static int16_t last[VL53L5CX_Sensor::GRID_SIZE];
        int16_t grid[VL53L5CX_Sensor::GRID_SIZE];
        distance_sensor_1.getDistanceGrid(grid);
        if (memcmp(grid, last, sizeof(grid)) != 0) {
            memcpy(last, grid, sizeof(grid));
            Serial.printf("[grid] %lu", (unsigned long)millis());
            for (int16_t d : grid) { Serial.printf(" %d", d); }
            Serial.println();
        }
    } else if (logMs == 0 || logMs >= 200) {
        drawScreen();                            // 207 ms a frame: the serial link above is polled between frames
        frames++;
    }
    if (logMs && millis() - lastLogMs >= logMs) {
        Serial.printf("[imu] hand %s %4d mm  pitch %6.1f roll %6.1f -> pos %+.2f away %+.2f right %+.2f note %.2f keys %d env %3u/%3u/%3u  voices %.1f/%.1f/%.1f Hz, cutoff %.0f  ctl %02x/%02x/%02x  peak %d\n",
                      handSensor ? (handSeen ? "seen" : "none") : "n/a ", handMm, rawPitch, rawRoll,
                      pitchPos, tiltAway, tiltRight, shownNote, (int)keysDown,
                      sid.envelope(0), sid.envelope(1), sid.envelope(2),
                      sid.freqHz(0), sid.freqHz(1), sid.freqHz(2), sid.cutoffHz(),
                      sid.control(0), sid.control(1), sid.control(2), (int)outputPeak);
        outputPeak = 0;
        static uint32_t lastGridFrames = 0;
        uint32_t grids = gridFrames;
        Serial.printf("[time] screen %.1f fps, grids %.1f Hz, longest gap between I2S writes %u ms (the DMA holds 46)\n",
                      frames * 1000.0f / (millis() - lastLogMs), (grids - lastGridFrames) * 1000.0f / (millis() - lastLogMs),
                      (unsigned)audioMaxGapMs);
        lastLogMs = millis(); frames = 0; audioMaxGapMs = 0; lastGridFrames = grids;
    }
    delay(2);
}
