// fliptribe: parameter model for the EA-1 recreation.
// Everything the panel can touch is a Param with an integer value 0..max (knobs are 0..127, the hardware's
// NRPN resolution). The table drives the UI, motion sequencing, persistence and MIDI alike. The per-part
// list is exactly the EA-1's 14-byte synth block (docs/ea1-reference.md §2, §3.1, §4.3), in panel order.
#pragma once
#include <cstdint>

enum PartParam : uint8_t {
  P_PORTAMENTO,    // 0..127 glide time, always-on glide
  P_OSC_BALANCE,   // 0 = OSC1 only .. 127 = OSC2 (and OSC MOD output) only
  P_OSC2_PITCH,    // 0..127 = -1 octave .. +4 octaves relative to OSC1
  P_OSC1_WAVE,     // 0 saw, 1 square, 2 triangle (3 = audio in on the hardware; not offered yet)
  P_OSC2_WAVE,     // 0 saw, 1 square, 2 triangle
  P_OSC_MOD,       // 0 off, 1 ring, 2 sync, 3 decimator (original EA-1; the mkII swaps deci for cross)
  P_DISTORTION,    // 0/1, drive comes from LEVEL
  P_LEVEL,         // part level and distortion drive
  P_CUTOFF,
  P_RESONANCE,
  P_EG_INT,        // bipolar, 64 = none
  P_DECAY,         // the single decay envelope (cutoff only), 0 ms .. 10 s
  P_DELAY_DEPTH,   // tempo delay: wet level and feedback together
  P_DELAY_TIME,    // one of 16 tempo-synced step multiples
  P_CHORUS_DEPTH,  // chorus -> flanger as depth rises
  P_CHORUS_TIME,   // LFO rate 0.2 Hz .. 5 kHz
  P_MOTION_SEQ,    // 0 off, 1 smooth, 2 trig hold (per part on the EA-1)
  NUM_PART_PARAMS
};

enum GlobalParam : uint8_t {
  G_SWING,         // 0..25 -> 50..75 %
  G_BEAT,          // 0 = 16th, 1 = 32nd, 2 = triplet (12 steps per bar)
  G_LENGTH,        // bars - 1 (0..3)
  G_MASTER_VOLUME, // 0..127 (not stored in patterns on the hardware; we keep it in settings)
  NUM_GLOBAL_PARAMS
};

enum ParamKind : uint8_t { PK_KNOB, PK_BIPOLAR_KNOB, PK_TOGGLE, PK_ENUM };

struct ParamInfo {
  const char* id;        // short stable id for files
  const char* label;     // panel label
  ParamKind kind;
  uint8_t max;           // inclusive
  uint8_t def;           // default value
  bool motion;           // can be motion-sequenced
  const char* const* names; // for PK_ENUM: value names, else nullptr
};

static const char* const kWaveNames[]  = {"SAW", "SQU", "TRI"};
static const char* const kModNames[]   = {"OFF", "RING", "SYNC", "DECI"};
static const char* const kMseqNames[]  = {"OFF", "SMOOTH", "TRIG HOLD"};
static const char* const kBeatNames[]  = {"16TH", "32ND", "TRIPLET"};
static const char* const kLenNames[]   = {"1", "2", "3", "4"};

static const ParamInfo kPartParams[NUM_PART_PARAMS] = {
  {"portamento",   "PORTAMENTO",    PK_KNOB,         127, 0,   true,  nullptr},
  {"osc_balance",  "OSC BALANCE",   PK_KNOB,         127, 0,   true,  nullptr},
  {"osc2_pitch",   "OSC2 PITCH OFS",PK_KNOB,         127, 25,  true,  nullptr},
  {"osc1_wave",    "OSC1 WAVE",     PK_ENUM,         2,   0,   true,  kWaveNames},
  {"osc2_wave",    "OSC2 WAVE",     PK_ENUM,         2,   0,   true,  kWaveNames},
  {"osc_mod",      "OSC MOD",       PK_ENUM,         3,   0,   true,  kModNames},
  {"distortion",   "DISTORTION",    PK_TOGGLE,       1,   0,   true,  nullptr},
  {"level",        "LEVEL",         PK_KNOB,         127, 100, true,  nullptr},
  {"cutoff",       "CUTOFF",        PK_KNOB,         127, 70,  true,  nullptr},
  {"resonance",    "RESONANCE",     PK_KNOB,         127, 40,  true,  nullptr},
  {"eg_int",       "EG INT",        PK_BIPOLAR_KNOB, 127, 96,  true,  nullptr},
  {"decay",        "DECAY",         PK_KNOB,         127, 45,  true,  nullptr},
  {"delay_depth",  "DELAY DEPTH",   PK_KNOB,         127, 0,   true,  nullptr},
  {"delay_time",   "DELAY TIME",    PK_KNOB,         127, 72,  true,  nullptr},
  {"chorus_depth", "CHORUS DEPTH",  PK_KNOB,         127, 0,   true,  nullptr},
  {"chorus_time",  "CHORUS TIME",   PK_KNOB,         127, 30,  true,  nullptr},
  {"motion_seq",   "MOTION SEQ",    PK_ENUM,         2,   1,   false, kMseqNames},
};

static const ParamInfo kGlobalParams[NUM_GLOBAL_PARAMS] = {
  {"swing",        "SWING",         PK_KNOB,         25,  0,   false, nullptr},
  {"beat",         "BEAT",          PK_ENUM,         2,   0,   false, kBeatNames},
  {"length",       "LENGTH",        PK_ENUM,         3,   0,   false, kLenNames},
  {"master",       "MASTER VOLUME", PK_KNOB,         127, 100, false, nullptr},
};

// The 16 tempo delay times, in steps (docs §2.7).
static const float kDelaySteps[16] = {0.25f, 1.0f / 3, 0.5f, 2.0f / 3, 0.75f, 1.0f, 4.0f / 3, 1.5f, 2.0f, 2.5f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
static const char* const kDelayStepNames[16] = {"1/4", "1/3", "1/2", "2/3", "3/4", "1", "1.33", "1.5", "2", "2.5", "3", "4", "5", "6", "7", "8"};
inline int delayIndex(int v) { int i = v * 16 / 128; return i < 0 ? 0 : (i > 15 ? 15 : i); }

constexpr int kParts = 2;
constexpr int kMaxSteps = 64;
constexpr int kNumPatterns = 256;     // A01..d64
constexpr int kStepsPerPage = 16;

struct Step {
  uint8_t on = 0;        // trigger
  uint8_t note = 60;     // pitch, C4 = 60 (Clear Part resets pitches to C4)
  uint8_t gate4 = 3;     // gate time in quarter steps minus one: 3 = 1.00 step, 0 = 0.25, 255 = 64.0
};

struct MotionLane {
  uint8_t active = 0;              // lane recorded
  uint8_t v[kMaxSteps] = {};       // value per step
};

struct PartData {
  uint8_t p[NUM_PART_PARAMS];
  Step step[kMaxSteps];
  MotionLane motion[NUM_PART_PARAMS];   // the hardware keeps one lane per part; we allow one per knob
  uint8_t mute = 0;
  void init() {
    for (int i = 0; i < NUM_PART_PARAMS; i++) p[i] = kPartParams[i].def;
    for (auto& s : step) s = Step{};
    for (auto& m : motion) m = MotionLane{};
    mute = 0;
  }
};

struct Pattern {
  char name[17] = "INIT";
  uint16_t tempo10 = 1300;         // BPM * 10 (20.0 .. 300.0)
  uint8_t g[NUM_GLOBAL_PARAMS];
  PartData part[kParts];
  void init() {
    for (int i = 0; i < NUM_GLOBAL_PARAMS; i++) g[i] = kGlobalParams[i].def;
    tempo10 = 1300;
    for (auto& pt : part) pt.init();
    part[1].p[P_CUTOFF] = 55;
    for (int i = 0; i < 17; i++) name[i] = 0;
    name[0] = 'I'; name[1] = 'N'; name[2] = 'I'; name[3] = 'T';
  }
  int stepsPerBar() const { return g[G_BEAT] == 2 ? 12 : 16; }
  int steps() const { return (g[G_LENGTH] + 1) * stepsPerBar(); }
};
