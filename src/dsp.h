// fliptribe: DSP building blocks for the EA-1 voice. Everything is per-sample, float, 48 kHz-agnostic.
#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace dsp {

constexpr float kPi = 3.14159265358979f;

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float midiToHz(float note) { return 440.0f * std::exp2((note - 69.0f) / 12.0f); }

// polyBLEP residual for a discontinuity at phase t with increment dt.
inline float polyBlep(float t, float dt) {
  if (t < dt) { t /= dt; return t + t - t * t - 1.0f; }
  if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
  return 0.0f;
}

enum Wave : uint8_t { W_SAW = 0, W_SQUARE = 1, W_TRI = 2 };

// Band-limited oscillator with hard sync support. Phase in [0,1).
struct Osc {
  float phase = 0.0f;
  float tri = 0.0f;       // leaky integrator state for the triangle
  void reset(float ph = 0.0f) { phase = ph; }
  // Advances one sample. Returns the waveform. *wrapped is set when the phase wrapped this sample,
  // and wrapFrac receives the sub-sample position of the wrap (for syncing a slave).
  inline float tick(float dt, Wave w, bool* wrapped = nullptr, float* wrapFrac = nullptr) {
    phase += dt;
    bool wr = false; float wf = 0.0f;
    if (phase >= 1.0f) { phase -= 1.0f; wr = true; wf = phase / dt; }
    if (wrapped) *wrapped = wr;
    if (wrapFrac) *wrapFrac = wf;
    return shape(dt, w);
  }
  inline float shape(float dt, Wave w) {
    switch (w) {
      case W_SAW: {
        float v = 2.0f * phase - 1.0f;
        return v - polyBlep(phase, dt);
      }
      case W_SQUARE: {
        float v = phase < 0.5f ? 1.0f : -1.0f;
        v += polyBlep(phase, dt);
        float t2 = phase + 0.5f; if (t2 >= 1.0f) t2 -= 1.0f;
        v -= polyBlep(t2, dt);
        return v;
      }
      default: {
        // triangle: integrate the blep'd square; leaky to kill DC. Scale so peak ~1.
        float sq = phase < 0.5f ? 1.0f : -1.0f;
        sq += polyBlep(phase, dt);
        float t2 = phase + 0.5f; if (t2 >= 1.0f) t2 -= 1.0f;
        sq -= polyBlep(t2, dt);
        tri = tri * 0.9995f + sq * dt * 4.0f;
        return clampf(tri, -1.0f, 1.0f);
      }
    }
  }
  // Hard sync: restart at the master's wrap position (sub-sample accurate enough for our purposes).
  inline void sync(float dt, float wrapFrac) { phase = wrapFrac * dt; }
};

// 4-pole transistor-ladder lowpass (Zavalishin TPT form), resonance 0..1 reaching self-oscillation.
struct Ladder {
  float s[4] = {0, 0, 0, 0};
  float g = 0.1f, k = 0.0f;
  void set(float cutoffHz, float res, float sr) {
    float fc = clampf(cutoffHz, 10.0f, sr * 0.45f);
    g = std::tan(kPi * fc / sr);
    k = 4.0f * clampf(res, 0.0f, 1.1f);   // 4 = oscillation threshold
  }
  inline float tick(float x) {
    float G = g / (1.0f + g);
    float G2 = G * G, G4 = G2 * G2;
    // feedback estimate
    float S = (G2 * G * s[0] + G2 * s[1] + G * s[2] + s[3]) / (1.0f + g);
    float u = (x - k * S) / (1.0f + k * G4);
    u = std::tanh(u);   // saturate the input, like the transistor ladder
    // four one-pole stages
    float v, y = u;
    for (int i = 0; i < 4; i++) {
      v = (y - s[i]) * G;
      y = v + s[i];
      s[i] = y + v;
    }
    return y;
  }
  void reset() { s[0] = s[1] = s[2] = s[3] = 0.0f; }
};

// Exponential decay envelope: jump to 1 on trigger, fall towards 0 with a time constant.
struct DecayEnv {
  float v = 0.0f;
  float coef = 0.999f;
  bool gate = false;
  void setDecay(float seconds, float sr) {
    float n = std::max(1.0f, seconds * sr);
    coef = std::exp(-6.9f / n);     // reaches ~0.1% at 'seconds'
  }
  inline void trigger() { v = 1.0f; }
  inline float tick() { v *= coef; return v; }
};

// Linear-ish amp gate with short attack and release so clicks stay away.
struct Gate {
  float v = 0.0f;
  float target = 0.0f;
  float up = 0.01f, down = 0.002f;
  void setRates(float sr) { up = 1.0f / (0.0015f * sr); down = 1.0f / (0.004f * sr); }
  inline void on() { target = 1.0f; }
  inline void off() { target = 0.0f; }
  inline float tick() {
    if (v < target) { v += up; if (v > target) v = target; }
    else if (v > target) { v -= down; if (v < target) v = target; }
    return v;
  }
};

// Glide between pitches (one-pole in the log domain).
struct Glide {
  float cur = 48.0f, target = 48.0f, coef = 0.0f;
  void setTime(float seconds, float sr) { coef = seconds <= 0.0005f ? 0.0f : std::exp(-1.0f / (seconds * sr * 0.25f)); }
  inline void jump(float n) { cur = target = n; }
  inline void to(float n) { target = n; }
  inline float tick() { cur = target + (cur - target) * coef; return cur; }
};

// Mono tempo delay, one per part. Depth sets wet level and feedback together; the feedback path soft-clips
// so excessive depth "distorts" rather than blows up, as the manual warns.
struct MonoDelay {
  static constexpr int kMax = 1 << 17;   // 3.3 s at 39 kHz; the hardware clamps long times too
  float* buf = nullptr;
  int w = 0;
  float timeSamples = 10000.0f, curTime = 10000.0f;
  float fb = 0.0f, wet = 0.0f;
  void alloc() { if (!buf) { buf = new float[kMax]; clear(); } }
  ~MonoDelay() { delete[] buf; }
  void clear() { for (int i = 0; i < kMax; i++) buf[i] = 0.0f; }
  inline float tick(float in) {
    curTime += (timeSamples - curTime) * 0.0004f;
    float rp = (float)w - curTime;
    while (rp < 0) rp += kMax;
    int i0 = (int)rp; float fr = rp - i0;
    int i1 = i0 + 1; if (i1 >= kMax) i1 -= kMax;
    float d = buf[i0] + (buf[i1] - buf[i0]) * fr;
    float f = d * fb;
    f = f / (1.0f + std::fabs(f) * 0.35f);   // gentle saturation in the loop
    buf[w] = in + f;
    if (++w >= kMax) w = 0;
    return d * wet;
  }
};

// Chorus that turns into a flanger as depth rises (shorter delay, feedback added). LFO goes to audio rate.
struct Chorus {
  static constexpr int kMax = 2048;
  float buf[kMax] = {};
  int w = 0;
  float lfo = 0.0f;
  inline float tick(float in, float depth, float rateHz, float sr) {
    if (depth <= 0.002f) { buf[w] = in; if (++w >= kMax) w = 0; return in; }
    lfo += rateHz / sr; if (lfo >= 1.0f) lfo -= 1.0f;
    float s = std::sin(lfo * 2.0f * kPi);
    float flange = clampf((depth - 0.35f) / 0.65f, 0.0f, 1.0f);
    float baseMs = 9.0f - 7.5f * flange;                   // chorus ~9 ms -> flanger ~1.5 ms
    float modMs = baseMs * (0.35f + 0.5f * depth);
    float dSamp = clampf((baseMs + modMs * s) * 0.001f * sr, 1.0f, kMax - 2.0f);
    float rp = (float)w - dSamp; while (rp < 0) rp += kMax;
    int i0 = (int)rp; float fr = rp - i0; int i1 = i0 + 1; if (i1 >= kMax) i1 -= kMax;
    float d = buf[i0] + (buf[i1] - buf[i0]) * fr;
    float fb = flange * 0.8f;
    float x = in + d * fb;
    buf[w] = x / (1.0f + std::fabs(x) * 0.2f);
    if (++w >= kMax) w = 0;
    float wetAmt = 0.5f * std::min(1.0f, depth * 3.0f);
    return in + d * wetAmt;
  }
};

// EA-1 style distortion: a lot of gain into a soft clipper, then trimmed back.
inline float distort(float x, float drive) {
  float y = x * drive;
  y = y / (1.0f + std::fabs(y));          // soft clip
  return y * 1.6f;
}

// Decimator: sample-and-hold at a rate given in cycles/sample.
struct Decimator {
  float phase = 0.0f, held = 0.0f;
  inline float tick(float x, float rate) {
    phase += rate;
    if (phase >= 1.0f) { phase -= 1.0f; held = x; }
    return held;
  }
};

} // namespace dsp
