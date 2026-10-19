// fliptribe: the audio engine. Owns the live Pattern, the two voices with their effects and the sequencer.
// The UI thread never touches engine state directly: it sends Commands through a lock-free queue and reads
// a snapshot the engine refreshes after every audio callback (try-lock, so audio never blocks). Wi-Fi sync reads
// the transport through a seqlocked SyncClock instead, which is lighter and carries a timestamp.
#pragma once
#include "params.h"
#include "dsp.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <cstring>

// steady clock in microseconds; the engine stamps its SyncClock with it and Wi-Fi sync polls with it
inline int64_t monoMicros() { return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

struct Command {
  enum Type : uint8_t {
    None, SetPart, SetGlobal, SetTempo10, ToggleStep, SetStep, SetStepNote, SetStepGate,
    Play, Stop, PlayStop, ResetPlay, SetRec, SetMute, NoteOn, NoteOff, LoadPattern, ClearMotion, ClearPart,
    CopyPart, SetEditPart, TapTempo, SetTranspose, EraseHold, AllNotesOff,
    SetRate, PlayFrom, Jump   // Wi-Fi sync: x = speed factor / start position / phase jump, positions in 16th notes
  } type = None;
  int a = 0, b = 0, c = 0, d = 0;
  double x = 0.0;
  Pattern* pat = nullptr;   // for LoadPattern: engine copies from it, then returns it through takeFreed()
};

template <int N> struct Ring {
  Command buf[N];
  std::atomic<int> head{0}, tail{0};
  bool push(const Command& c) {
    int h = head.load(std::memory_order_relaxed), n = (h + 1) % N;
    if (n == tail.load(std::memory_order_acquire)) return false;
    buf[h] = c; head.store(n, std::memory_order_release); return true;
  }
  bool pop(Command& c) {
    int t = tail.load(std::memory_order_relaxed);
    if (t == head.load(std::memory_order_acquire)) return false;
    c = buf[t]; tail.store((t + 1) % N, std::memory_order_release); return true;
  }
};

// What the UI sees. Refreshed from the audio thread after each callback.
struct Snapshot {
  Pattern pat;
  bool playing = false, rec = false, pendingPattern = false;
  int step = 0;             // current sequencer step 0..63
  int editPart = 0;
  int transpose = 0;
  float partLevel[kParts] = {0, 0};
  float tempoBpm = 130.0f;
  uint8_t motionRecLane[kParts][NUM_PART_PARAMS] = {};
  int soundingNote[kParts] = {-1, -1};   // pitch currently gated per part, -1 = silent
};

// Transport as Wi-Fi sync sees it. pos counts 16th notes since play started whatever the BEAT scale (a 32nd
// step is half of one, a triplet step 4/3), unwrapped; perSec is how fast it moves, rate trim included.
struct SyncClock {
  bool playing = false;
  int tempo10 = 1300;
  double pos = 0.0, perSec = 0.0;
  int64_t stampUs = 0;
};

class Engine {
public:
  Engine();
  void init(float sampleRate);
  void render(float* out, int frames);          // interleaved stereo float
  bool send(const Command& c) { return q.push(c); }
  void snapshot(Snapshot& out) { std::lock_guard<std::mutex> lk(snapMutex); out = snap; }
  Pattern* takeFreed();
  float sampleRate() const { return sr; }
  SyncClock syncClock(int64_t nowUs) const;     // any thread; pos is extrapolated from the last callback to nowUs

private:
  struct Voice {
    dsp::Osc o1, o2;
    dsp::Ladder flt;
    dsp::DecayEnv env;
    dsp::Gate gate;
    dsp::Glide glide;
    dsp::Decimator dec;
    dsp::Chorus chorus;
    dsp::MonoDelay delay;
    float cut = 1000.0f, res = 0.0f;
    int gateLeft = 0;                  // samples until note off (0 = off)
    bool held = false;                 // live key held (keyboard mode / MIDI)
    float meter = 0.0f;
    float live[NUM_PART_PARAMS];       // knob positions with motion applied, as floats
    float motionFrom[NUM_PART_PARAMS], motionTo[NUM_PART_PARAMS];
    int overrideLeft[NUM_PART_PARAMS]; // samples during which a touched knob beats the motion lane
    int recLeft[NUM_PART_PARAMS];      // steps left while recording a motion lane (0 = not recording)
    int note = 60;
    float noteF = 60.0f;
  };

  void handle(const Command& c);
  void trigger(int part, int note, int gateSamples, bool legato);
  void release(int part);
  void onStep();
  void applyMotionTargets(int stepIdx);
  float stepSamples() const;
  float swingFrac() const;
  double sixteenthsPerStep() const;
  double linFrac() const;
  void moveBy(double sixteenths);
  void publish();
  void publishClock();
  void startPlay(double sixteenths = 0.0);
  void stopPlay();
  void adoptPattern(const Pattern& p, bool keepTempo);

  Pattern pat;
  Pattern* pending = nullptr;   // pattern waiting for the end of the current one
  Voice voice[kParts];
  float gLive[NUM_GLOBAL_PARAMS];

  float sr = 39062.5f;
  bool playing = false, rec = false, eraseHold = false;
  int step = 0;
  double stepPos = 0.0, stepLen = 1.0;
  double rate = 1.0;            // Wi-Fi sync speed trim, 0.9 .. 1.1
  double clockAtStep = 0.0;     // 16th notes since play started, at the unswung start of the current step
  int editPart = 0, transpose = 0;
  double tapLast = -1.0; int tapCount = 0; double tapAvg = 0; double clockSamples = 0;
  float lim = 1.0f;

  Ring<1024> q;
  Ring<32> freed;
  std::mutex snapMutex;
  Snapshot snap;
  // SyncClock behind a seqlock: odd while the audio thread writes
  std::atomic<uint32_t> clkSeq{0};
  std::atomic<bool> clkPlaying{false};
  std::atomic<int> clkTempo10{1300};
  std::atomic<double> clkPos{0.0}, clkPerSec{0.0};
  std::atomic<int64_t> clkStamp{0};
};
