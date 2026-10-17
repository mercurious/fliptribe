#include "engine.h"
#include <cmath>
#include <algorithm>

using namespace dsp;

Engine::Engine() { pat.init(); }

void Engine::init(float sampleRate) {
  sr = sampleRate;
  for (int p = 0; p < kParts; p++) {
    Voice& v = voice[p];
    v.gate.setRates(sr);
    v.delay.alloc();
    for (int i = 0; i < NUM_PART_PARAMS; i++) {
      v.live[i] = v.motionFrom[i] = v.motionTo[i] = pat.part[p].p[i];
      v.overrideLeft[i] = 0; v.recLeft[i] = 0;
    }
    v.glide.jump(60.0f);
    v.o1.reset(0.0f); v.o2.reset(0.37f);   // free-running, start apart
  }
  for (int i = 0; i < NUM_GLOBAL_PARAMS; i++) gLive[i] = pat.g[i];
  publish();
}

Pattern* Engine::takeFreed() { Command c; if (freed.pop(c)) return c.pat; return nullptr; }

float Engine::stepSamples() const {
  float bpm = pat.tempo10 / 10.0f;
  float beat = 60.0f / bpm * sr;          // quarter note
  switch (pat.g[G_BEAT]) {
    case 1: return beat / 8.0f;            // 32nd
    case 2: return beat / 3.0f;            // 8th-note triplet, 12 per bar
    default: return beat / 4.0f;           // 16th
  }
}

void Engine::adoptPattern(const Pattern& p, bool keepTempo) {
  uint16_t t = pat.tempo10;
  pat = p;
  if (keepTempo) pat.tempo10 = t;
  for (int pi = 0; pi < kParts; pi++) {
    Voice& v = voice[pi];
    for (int i = 0; i < NUM_PART_PARAMS; i++) { v.live[i] = v.motionFrom[i] = v.motionTo[i] = pat.part[pi].p[i]; v.overrideLeft[i] = 0; v.recLeft[i] = 0; }
  }
  for (int i = 0; i < NUM_GLOBAL_PARAMS; i++) gLive[i] = pat.g[i];
}

void Engine::startPlay() {
  playing = true; step = 0; stepPos = 0.0;
  stepLen = stepSamples();
  for (int p = 0; p < kParts; p++) voice[p].gateLeft = 0;
  onStep();
}

void Engine::stopPlay() {
  playing = false; step = 0; stepPos = 0.0;
  for (int p = 0; p < kParts; p++) {
    if (!voice[p].held) release(p);
    for (int i = 0; i < NUM_PART_PARAMS; i++) voice[p].recLeft[i] = 0;
  }
  if (pending) { adoptPattern(*pending, false); Command f; f.type = Command::LoadPattern; f.pat = pending; freed.push(f); pending = nullptr; }
}

void Engine::release(int part) { voice[part].gate.off(); voice[part].gateLeft = 0; }

void Engine::trigger(int part, int note, int gateSamples, bool legato) {
  Voice& v = voice[part];
  float porta = v.live[P_PORTAMENTO] / 127.0f;
  // portamento is always-on glide; constant-time law, up to ~1.5 s at full
  v.glide.setTime(porta < 0.005f ? 0.0f : 0.004f + porta * porta * 1.5f, sr);
  v.glide.to((float)note);
  if (!legato) {
    v.env.trigger();
    v.gate.on();
    // trig hold motion samples the lane at the trigger
    if (pat.part[part].p[P_MOTION_SEQ] == 2) {
      for (int i = 0; i < NUM_PART_PARAMS; i++) {
        const MotionLane& m = pat.part[part].motion[i];
        if (m.active && kPartParams[i].motion && v.recLeft[i] == 0 && v.overrideLeft[i] == 0) v.live[i] = v.motionFrom[i] = v.motionTo[i] = m.v[step];
      }
    }
  }
  v.gateLeft = gateSamples;
  v.note = note;
}

void Engine::applyMotionTargets(int idx) {
  int len = pat.steps();
  int nxt = (idx + 1) % len;
  for (int p = 0; p < kParts; p++) {
    Voice& v = voice[p];
    int mode = pat.part[p].p[P_MOTION_SEQ];
    if (mode != 1) continue;                            // smooth only; trig hold samples at triggers
    for (int i = 0; i < NUM_PART_PARAMS; i++) {
      const MotionLane& m = pat.part[p].motion[i];
      if (!m.active || !kPartParams[i].motion) continue;
      if (v.recLeft[i] > 0 || v.overrideLeft[i] > 0) continue;
      float from = m.v[idx], to = m.v[nxt];
      if (kPartParams[i].kind == PK_TOGGLE || kPartParams[i].kind == PK_ENUM) to = from;
      v.motionFrom[i] = from; v.motionTo[i] = to; v.live[i] = from;
    }
  }
}

void Engine::onStep() {
  int len = pat.steps();
  if (step >= len) step = 0;
  // pattern change takes effect at the top of the pattern
  if (step == 0 && pending) {
    adoptPattern(*pending, true);
    Command f; f.type = Command::LoadPattern; f.pat = pending; freed.push(f); pending = nullptr;
    len = pat.steps();
  }
  // motion recording: store the live knob value into the lane for this step
  for (int p = 0; p < kParts; p++) {
    Voice& v = voice[p];
    for (int i = 0; i < NUM_PART_PARAMS; i++) {
      if (v.recLeft[i] > 0) {
        MotionLane& m = pat.part[p].motion[i];
        m.active = 1; m.v[step] = (uint8_t)std::lround(clampf(v.live[i], 0, kPartParams[i].max));
        if (--v.recLeft[i] == 0) v.overrideLeft[i] = 0;
      }
    }
  }
  applyMotionTargets(step);
  float base = stepSamples();
  for (int p = 0; p < kParts; p++) {
    PartData& pd = pat.part[p];
    Step& s = pd.step[step];
    if (eraseHold && rec && p == editPart) s.on = 0;
    if (s.on && !pd.mute && !voice[p].held) {
      int gateSamples = std::max(1, (int)(base * (s.gate4 + 1) / 4.0f));
      bool legato = voice[p].gateLeft > (int)(base * 0.1f);   // previous gate still running: no EG retrigger
      trigger(p, std::clamp((int)s.note + transpose, 0, 127), gateSamples, legato);
    }
  }
}

void Engine::handle(const Command& c) {
  switch (c.type) {
    case Command::SetPart: {
      int part = c.a, i = c.b; if (part < 0 || part >= kParts || i < 0 || i >= NUM_PART_PARAMS) break;
      int val = std::clamp(c.c, 0, (int)kPartParams[i].max);
      pat.part[part].p[i] = (uint8_t)val;
      Voice& v = voice[part];
      v.live[i] = v.motionFrom[i] = v.motionTo[i] = val;
      if (kPartParams[i].motion) {
        v.overrideLeft[i] = (int)(stepLen * 1.5);
        if (playing && rec && pat.part[part].p[P_MOTION_SEQ] != 0 && v.recLeft[i] == 0) {
          v.recLeft[i] = pat.steps();
          MotionLane& m = pat.part[part].motion[i];
          m.active = 1; m.v[step] = (uint8_t)val;
        }
      }
      break;
    }
    case Command::SetGlobal: {
      int i = c.a; if (i < 0 || i >= NUM_GLOBAL_PARAMS) break;
      int val = std::clamp(c.b, 0, (int)kGlobalParams[i].max);
      pat.g[i] = (uint8_t)val; gLive[i] = val;
      if ((i == G_LENGTH || i == G_BEAT) && step >= pat.steps()) step = 0;
      break;
    }
    case Command::SetTempo10: pat.tempo10 = (uint16_t)std::clamp(c.a, 200, 3000); break;
    case Command::ToggleStep: {
      int part = c.a, s = c.b; if (part < 0 || part >= kParts || s < 0 || s >= kMaxSteps) break;
      pat.part[part].step[s].on = !pat.part[part].step[s].on;
      break;
    }
    case Command::SetStep: {
      int part = c.a, s = c.b; if (part < 0 || part >= kParts || s < 0 || s >= kMaxSteps) break;
      pat.part[part].step[s].on = c.c ? 1 : 0;
      if (c.d >= 0) pat.part[part].step[s].note = (uint8_t)std::clamp(c.d, 0, 127);
      break;
    }
    case Command::SetStepNote: { int part = c.a, s = c.b; if (part < 0 || part >= kParts || s < 0 || s >= kMaxSteps) break; pat.part[part].step[s].note = (uint8_t)std::clamp(c.c, 0, 127); break; }
    case Command::SetStepGate: { int part = c.a, s = c.b; if (part < 0 || part >= kParts || s < 0 || s >= kMaxSteps) break; pat.part[part].step[s].gate4 = (uint8_t)std::clamp(c.c, 0, 255); break; }
    case Command::Play: if (!playing) startPlay(); break;
    case Command::Stop: stopPlay(); break;
    case Command::PlayStop: if (playing) stopPlay(); else startPlay(); break;
    case Command::ResetPlay: startPlay(); break;
    case Command::SetRec: rec = c.a != 0; if (!rec) { for (int p = 0; p < kParts; p++) for (int i = 0; i < NUM_PART_PARAMS; i++) voice[p].recLeft[i] = 0; eraseHold = false; } break;
    case Command::EraseHold: eraseHold = c.a != 0; break;
    case Command::SetMute: if (c.a >= 0 && c.a < kParts) { pat.part[c.a].mute = c.b ? 1 : 0; if (c.b && !voice[c.a].held) release(c.a); } break;
    case Command::NoteOn: {
      int part = c.a; if (part < 0 || part >= kParts) break;
      int note = std::clamp(c.b, 0, 127);
      bool legato = voice[part].held;      // key already down: slide (no EG retrigger), like overlapping gates
      voice[part].held = true;
      trigger(part, note, 0, legato);
      if (playing && rec) {
        int len = pat.steps();
        int target = (stepPos > stepLen * 0.5) ? (step + 1) % len : step;
        Step& st = pat.part[part].step[target];
        st.on = 1; st.note = (uint8_t)note;
      }
      break;
    }
    case Command::NoteOff: { int part = c.a; if (part < 0 || part >= kParts) break; voice[part].held = false; release(part); break; }
    case Command::AllNotesOff: for (int p = 0; p < kParts; p++) { voice[p].held = false; release(p); } break;
    case Command::LoadPattern: {
      if (!c.pat) break;
      if (playing && c.a == 0) {
        if (pending) { Command f; f.type = Command::LoadPattern; f.pat = pending; freed.push(f); }
        pending = c.pat;
      } else {
        adoptPattern(*c.pat, playing);
        if (playing) { step = step % pat.steps(); applyMotionTargets(step); }
        Command f; f.type = Command::LoadPattern; f.pat = c.pat; freed.push(f);
      }
      break;
    }
    case Command::ClearMotion: if (c.a >= 0 && c.a < kParts) { for (auto& m : pat.part[c.a].motion) m = MotionLane{}; for (int i = 0; i < NUM_PART_PARAMS; i++) { voice[c.a].live[i] = voice[c.a].motionFrom[i] = voice[c.a].motionTo[i] = pat.part[c.a].p[i]; voice[c.a].recLeft[i] = 0; } } break;
    case Command::ClearPart: if (c.a >= 0 && c.a < kParts) { for (auto& s : pat.part[c.a].step) s = Step{}; } break;
    case Command::CopyPart: { int from = c.a, to = c.b; if (from < 0 || from >= kParts || to < 0 || to >= kParts || from == to) break; uint8_t mute = pat.part[to].mute; pat.part[to] = pat.part[from]; pat.part[to].mute = mute; for (int i = 0; i < NUM_PART_PARAMS; i++) { voice[to].live[i] = voice[to].motionFrom[i] = voice[to].motionTo[i] = pat.part[to].p[i]; voice[to].recLeft[i] = 0; voice[to].overrideLeft[i] = 0; } break; }
    case Command::SetEditPart: editPart = std::clamp(c.a, 0, kParts - 1); break;
    case Command::SetTranspose: transpose = std::clamp(c.a, -24, 24); break;
    case Command::TapTempo: {
      double now = clockSamples / sr;
      if (tapLast >= 0 && now - tapLast < 2.0) {
        double iv = now - tapLast;
        tapAvg = tapCount == 0 ? iv : tapAvg * 0.6 + iv * 0.4;
        tapCount++;
        if (tapCount >= 2) pat.tempo10 = (uint16_t)std::clamp((int)std::lround(600.0 / tapAvg), 200, 3000);
      } else tapCount = 0;
      tapLast = now;
      break;
    }
    default: break;
  }
}

void Engine::render(float* out, int frames) {
  Command c;
  while (q.pop(c)) handle(c);

  float swingFrac = (gLive[G_SWING] / 25.0f) * 0.5f;      // 75 % = even steps half a step late
  if (pat.g[G_BEAT] == 2) swingFrac = 0.0f;               // no swing in triplet scale
  float baseStep = stepSamples();

  // per-part delay / chorus settings, once per block
  for (int p = 0; p < kParts; p++) {
    Voice& v = voice[p];
    float depth = v.live[P_DELAY_DEPTH] / 127.0f;
    float t = baseStep * kDelaySteps[delayIndex((int)v.live[P_DELAY_TIME])];
    v.delay.timeSamples = std::min(t, (float)(MonoDelay::kMax - 4));
    v.delay.fb = depth * 0.95f;
    v.delay.wet = depth * 0.9f;
  }

  int len = pat.steps();
  for (int n = 0; n < frames; n++) {
    clockSamples += 1.0;
    if (playing) {
      stepPos += 1.0;
      if (stepPos >= stepLen) {
        stepPos -= stepLen;
        step = (step + 1) % len;
        float swingDelay = baseStep * swingFrac;
        stepLen = (step % 2 == 0) ? baseStep + swingDelay : baseStep - swingDelay;
        onStep();
        len = pat.steps();
      }
    }
    float frac = playing ? (float)(stepPos / stepLen) : 0.0f;

    float mix = 0.0f;
    for (int p = 0; p < kParts; p++) {
      Voice& v = voice[p];
      PartData& pd = pat.part[p];
      // motion lanes (smooth mode interpolates within the step)
      if (pd.p[P_MOTION_SEQ] == 1) {
        for (int i = 0; i < NUM_PART_PARAMS; i++) {
          if (v.overrideLeft[i] > 0) { v.overrideLeft[i]--; continue; }
          if (pd.motion[i].active && kPartParams[i].motion && v.recLeft[i] == 0)
            v.live[i] = v.motionFrom[i] + (v.motionTo[i] - v.motionFrom[i]) * frac;
        }
      } else {
        for (int i = 0; i < NUM_PART_PARAMS; i++) if (v.overrideLeft[i] > 0) v.overrideLeft[i]--;
      }
      if (v.gateLeft > 0 && --v.gateLeft == 0 && !v.held) v.gate.off();

      float env = v.env.tick();
      float g = v.gate.tick();
      float y = 0.0f;
      if (g > 0.0f || v.gate.target > 0.5f) {
        float dec = v.live[P_DECAY] / 127.0f;
        v.env.setDecay(0.004f + std::pow(dec, 2.2f) * 10.0f, sr);

        float note = v.glide.tick();
        v.noteF = note;
        float f1 = midiToHz(note);
        // OSC2 pitch offset: -12 .. +48 semitones, with a detent at 0
        float semis = -12.0f + v.live[P_OSC2_PITCH] * 60.0f / 127.0f;
        if (std::fabs(semis) < 0.6f) semis = 0.0f;
        float f2 = midiToHz(note + semis);
        float dt1 = std::min(f1 / sr, 0.45f), dt2 = std::min(f2 / sr, 0.45f);
        Wave w1 = (Wave)std::min(2, (int)pd.p[P_OSC1_WAVE]), w2 = (Wave)std::min(2, (int)pd.p[P_OSC2_WAVE]);
        int mod = (int)std::lround(v.live[P_OSC_MOD]);
        bool wrapped = false; float wf = 0.0f;
        float a = v.o1.tick(dt1, w1, &wrapped, &wf);
        // OSC1 waves carry a slightly heavier fundamental (manual)
        a += 0.22f * std::sin(v.o1.phase * 2.0f * kPi);
        if (mod == 2 && wrapped) v.o2.sync(dt2, wf);
        float b = v.o2.tick(dt2, w2);
        float modOut;
        switch (mod) {
          case 1: modOut = a * b * 1.3f; break;                                        // ring
          case 2: modOut = b; break;                                                   // sync (b is slaved)
          case 3: modOut = v.dec.tick(a, std::min(0.98f, dt2 * 8.0f)); break;          // decimator
          default: modOut = b; break;
        }
        float bal = v.live[P_OSC_BALANCE] / 127.0f;
        float osc = a * (1.0f - bal) + modOut * bal;

        float cutK = v.live[P_CUTOFF] / 127.0f;
        float egi = (v.live[P_EG_INT] - 64.0f) / 63.0f;
        float octs = cutK * 10.3f + egi * 6.0f * env;
        float fc = 18.0f * std::exp2(clampf(octs, 0.0f, 10.3f));
        float rk = v.live[P_RESONANCE] / 127.0f;
        v.cut += (fc - v.cut) * 0.03f;
        v.res += (rk - v.res) * 0.02f;
        v.flt.set(v.cut, v.res * 1.04f, sr);
        y = v.flt.tick(osc * 0.7f);
        y *= 1.0f + v.res * 0.5f;
        float lvl = v.live[P_LEVEL] / 127.0f;
        if (pd.p[P_DISTORTION] || v.live[P_DISTORTION] > 0.5f) {
          float d = y * (2.0f + lvl * 30.0f);
          y = clampf(d, -1.0f, 1.0f) * 0.55f * (0.35f + 0.65f * lvl);
        } else y *= lvl * lvl * 1.1f;
        y *= g;
      }
      // effects run even when the voice is silent so tails ring out
      float cd = v.live[P_CHORUS_DEPTH] / 127.0f;
      float rate = 0.2f * std::pow(25000.0f, v.live[P_CHORUS_TIME] / 127.0f);
      y = v.chorus.tick(y, cd, rate, sr);
      y += v.delay.tick(y);
      float ay = std::fabs(y);
      v.meter = ay > v.meter ? ay : v.meter * 0.9996f;
      mix += y;
    }
    float master = gLive[G_MASTER_VOLUME] / 127.0f;
    float o = mix * master * 0.9f;
    float peak = std::fabs(o);
    if (peak * lim > 0.98f) lim = 0.98f / peak; else lim += (1.0f - lim) * 0.0003f;
    o = std::tanh(o * lim * 1.15f);
    out[2 * n] = o; out[2 * n + 1] = o;
  }
  publish();
}

void Engine::publish() {
  if (!snapMutex.try_lock()) return;
  snap.pat = pat;
  snap.playing = playing; snap.rec = rec; snap.step = step; snap.editPart = editPart; snap.transpose = transpose;
  snap.pendingPattern = pending != nullptr;
  snap.tempoBpm = pat.tempo10 / 10.0f;
  for (int p = 0; p < kParts; p++) {
    snap.partLevel[p] = voice[p].meter;
    snap.soundingNote[p] = (voice[p].gate.target > 0.5f) ? voice[p].note : -1;
    for (int i = 0; i < NUM_PART_PARAMS; i++) snap.motionRecLane[p][i] = voice[p].recLeft[i] > 0;
  }
  snapMutex.unlock();
}
