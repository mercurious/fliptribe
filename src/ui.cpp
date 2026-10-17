#include "ui.h"
#include "store.h"
#include <cmath>
#include <algorithm>
#include <cstdio>

static const Color C_TEXT{40, 40, 44}, C_TEXT_DIM{95, 95, 100}, C_WHITE{250, 250, 250};
static const Color C_LED_RED{255, 40, 20}, C_LED_ORANGE{255, 140, 20}, C_LED_GREEN{60, 255, 80};
static const Color C_SEG_ON{255, 60, 30}, C_SEG_OFF{70, 22, 18}, C_DISPLAY_BG{28, 10, 8};
static const Color C_KEY{232, 232, 230}, C_KEY_DOWN{190, 190, 188};
static const Color C_BTN{58, 58, 62}, C_BTN_DOWN{40, 40, 44}, C_FOCUS{40, 160, 255};
static const int W = 1920, H = 1080;
static const char* kRowNames[NUM_ROWS] = {"PATTERN", "TEMPO", "STEP REC", "PITCH", "GATE TIME"};
static const char* kShiftNames[16] = {"LENGTH 1", "LENGTH 2", "LENGTH 3", "LENGTH 4", "16TH", "32ND", "TRIPLET", "SWING", "MOVE DATA", "COPY PART", "CLEAR MOTION", "CLEAR PART", "INSERT PTN", "DELETE PTN", "CLEAR EVENT", "CLEAR SONG"};
static const int kStepX0 = 262, kStepPitch = 102, kStepW = 92, kStepH = 200, kStepCY = 900;

// ---------- layout ----------
void Panel::layout() {
  ctls.clear();
  auto knob = [&](Scope sc, int id, int cx, int cy, int r, const char* label) { ctls.push_back({K_KNOB, sc, id, cx, cy, 2 * r + 30, 2 * r + 30, r, label, 0}); };
  auto btn = [&](Scope sc, int id, int cx, int cy, int w, int h, const char* label, int leds = 0) { ctls.push_back({K_BUTTON, sc, id, cx, cy, w, h, 0, label, leds}); };

  // ---- common section ----
  knob(S_GLOBAL, G_MASTER_VOLUME, 90, 178, 30, "MASTER VOL");
  ctls.push_back({K_DIAL, S_ACTION, 0, 585, 175, 150, 150, 60, "", 0});
  btn(S_ACTION, A_WRITE, 650, 82, 90, 40, "WRITE");
  btn(S_ACTION, A_CURSOR_UP, 120, 252, 54, 34, "");
  btn(S_ACTION, A_CURSOR_DOWN, 120, 294, 54, 34, "");
  for (int r = 0; r < NUM_ROWS; r++) ctls.push_back({K_ROW, S_ACTION, r, 300, 242 + r * 20, 260, 20, 0, kRowNames[r], 0});
  btn(S_ACTION, A_MODE_PATTERN, 500, 258, 80, 30, "PATTERN");
  btn(S_ACTION, A_MODE_SONG, 590, 258, 80, 30, "SONG");
  btn(S_ACTION, A_MODE_GLOBAL, 500, 300, 80, 30, "GLOBAL");
  btn(S_ACTION, A_MODE_MIDI, 590, 300, 80, 30, "MIDI");

  // ---- oscillator ----
  knob(S_PART, P_PORTAMENTO, 800, 175, 42, "PORTAMENTO");
  knob(S_PART, P_OSC_BALANCE, 935, 175, 42, "OSC BALANCE");
  knob(S_PART, P_OSC2_PITCH, 1070, 175, 42, "OSC2 PITCH");
  btn(S_PART, P_OSC1_WAVE, 1225, 92, 100, 34, "OSC1 WAVE", 3);
  btn(S_PART, P_OSC2_WAVE, 1225, 190, 100, 34, "OSC2 WAVE", 3);
  btn(S_PART, P_OSC_MOD, 1225, 288, 100, 34, "OSC MOD", 3);

  // ---- filter ----
  knob(S_PART, P_CUTOFF, 1445, 190, 62, "CUTOFF");
  knob(S_PART, P_RESONANCE, 1600, 175, 42, "RESONANCE");
  knob(S_PART, P_EG_INT, 1720, 175, 42, "EG INT");
  knob(S_PART, P_DECAY, 1838, 175, 42, "DECAY");

  // ---- sequence control ----
  btn(S_ACTION, A_REC, 95, 470, 110, 66, "REC");
  btn(S_ACTION, A_STOP, 225, 470, 110, 66, "STOP / CANCEL");
  btn(S_ACTION, A_PLAY, 355, 470, 110, 66, "PLAY / PAUSE");
  btn(S_ACTION, A_TAP, 485, 470, 110, 66, "TAP / REST-TIE");
  btn(S_ACTION, A_TRANSPOSE, 615, 470, 110, 66, "TRANSPOSE");
  btn(S_ACTION, A_PART_SELECT, 95, 572, 110, 40, "PART SELECT", 2);
  btn(S_PART, P_MOTION_SEQ, 262, 572, 110, 40, "MOTION SEQ", 3);
  btn(S_ACTION, A_MUTE1, 400, 572, 100, 40, "MUTE 1");
  btn(S_ACTION, A_MUTE2, 520, 572, 100, 40, "MUTE 2");

  // ---- amp ----
  btn(S_PART, P_DISTORTION, 800, 470, 110, 44, "DISTORTION");
  knob(S_PART, P_LEVEL, 930, 500, 44, "LEVEL");

  // ---- effect ----
  btn(S_ACTION, A_FX_TYPE, 1095, 470, 110, 44, "TYPE SELECT", 2);
  knob(S_PART, P_DELAY_DEPTH, 1270, 500, 44, "DEPTH");     // resolved to chorus when fxType = 1
  knob(S_PART, P_DELAY_TIME, 1410, 500, 44, "TIME");

  // ---- step key section ----
  btn(S_ACTION, A_SEL_PREV, 70, 800, 60, 50, "");
  btn(S_ACTION, A_SEL_NEXT, 140, 800, 60, 50, "");
  btn(S_ACTION, A_KEYBOARD, 105, 950, 130, 48, "KEYBOARD");
  btn(S_ACTION, A_SHIFT, 105, 1020, 130, 40, "SHIFT");
  for (int i = 0; i < 16; i++) ctls.push_back({K_STEP, S_ACTION, i, kStepX0 + i * kStepPitch, kStepCY, kStepW, kStepH, 0, "", 0});
}

bool Panel::init(int startSlot, int master) {
  layout();
  for (int i = 0; i < SDL_NumJoysticks(); i++) if (SDL_IsGameController(i)) { pad = SDL_GameControllerOpen(i); if (pad) { fprintf(stderr, "pad: %s\n", SDL_GameControllerName(pad)); break; } }
  for (int i = 0; i < 4; i++) spare.push_back(new Pattern);
  buildStatic();
  for (size_t i = 0; i < ctls.size(); i++) if (ctls[i].kind == K_KNOB && ctls[i].scope == S_PART && ctls[i].id == P_CUTOFF) focus = (int)i;
  slot = startSlot;
  setSlot(startSlot);
  sendGlobal(G_MASTER_VOLUME, std::clamp(master, 0, 127));
  dirty = false;
  return true;
}

void Panel::shutdown() {
  if (staticLayer) SDL_DestroyTexture(staticLayer);
  if (pad) SDL_GameControllerClose(pad);
  for (auto* p : spare) delete p;
}

void Panel::reclaim() { while (Pattern* p = eng.takeFreed()) spare.push_back(p); }

// ---------- static panel ----------
static void sectionBox(Gfx& g, int x, int y, int w, int h, const char* title) {
  g.roundRect(x, y, w, h, 10, Color{0, 0, 0, 28});
  g.roundRect(x + 1, y + 1, w - 2, h - 2, 9, Color{255, 255, 255, 40});
  g.roundRect(x + 2, y + 2, w - 4, h - 4, 8, Color{205, 205, 206, 255});
  g.roundRect(x + 2, y + 2, w - 4, 30, 8, Color{52, 52, 56, 255});
  g.rect(x + 2, y + 24, w - 4, 8, Color{52, 52, 56, 255});
  g.text(title, x + 14, y + 6, Color{235, 235, 235}, Gfx::F_LABEL);
}
static void screw(Gfx& g, int x, int y) {
  g.circle(x, y, 7, Color{120, 120, 124}); g.circle(x - 1, y - 1, 6, Color{165, 165, 168});
  g.line(x - 4, y - 4, x + 4, y + 4, Color{90, 90, 94}, 2);
}
static void flowBox(Gfx& g, int x, int y, int w, const char* t) {
  g.roundRect(x, y, w, 30, 4, Color{60, 60, 64}); g.text(t, x + w / 2, y + 6, C_WHITE, Gfx::F_SMALL, 1);
}

void Panel::buildStatic() {
  staticLayer = SDL_CreateTexture(gfx.ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, W, H);
  SDL_SetRenderTarget(gfx.ren, staticLayer);
  gfx.panelBackground(W, H);
  gfx.frame(0, 0, W, H, Color{120, 120, 124}, 2);
  gfx.frame(2, 2, W - 4, H - 4, Color{235, 235, 236, 160}, 1);
  screw(gfx, 24, 24); screw(gfx, W - 24, 24); screw(gfx, 24, H - 24); screw(gfx, W - 24, H - 24);

  // sections
  sectionBox(gfx, 20, 20, 690, 335, "COMMON");
  sectionBox(gfx, 730, 20, 600, 335, "OSCILLATOR");
  sectionBox(gfx, 1350, 20, 550, 335, "FILTER");
  sectionBox(gfx, 20, 375, 690, 325, "SEQUENCE CONTROL");
  sectionBox(gfx, 730, 375, 270, 325, "AMP");
  sectionBox(gfx, 1020, 375, 470, 325, "EFFECT");
  sectionBox(gfx, 1510, 375, 390, 325, "SIGNAL FLOW");
  sectionBox(gfx, 20, 720, 1880, 340, "STEP KEY");

  // logo
  gfx.text("ELECTRIBE·A", 40, 56, Color{30, 30, 34}, Gfx::F_LOGO);
  gfx.text("EA-1", 40 + gfx.textWidth("ELECTRIBE·A", Gfx::F_LOGO) + 14, 68, Color{200, 30, 20}, Gfx::F_BIG);
  gfx.text("ANALOG MODELING SYNTHESIZER  ·  fliptribe recreation", 42, 118, C_TEXT_DIM, Gfx::F_SMALL);
  // display window
  gfx.roundRect(170, 150, 300, 90, 8, Color{15, 15, 15});
  gfx.roundRect(174, 154, 292, 82, 6, C_DISPLAY_BG);
  gfx.text("BEAT", 500, 148, C_TEXT, Gfx::F_TINY, 1);
  // matrix
  gfx.text("PARAMETER", 180, 222, C_TEXT_DIM, Gfx::F_TINY);
  gfx.text("MODE", 460, 222, C_TEXT_DIM, Gfx::F_TINY);
  gfx.text("▲", 120, 251, C_WHITE, Gfx::F_LABEL, 1);
  gfx.text("▼", 120, 307, C_WHITE, Gfx::F_LABEL, 1);
  gfx.text("CURSOR", 120, 316, C_TEXT, Gfx::F_TINY, 1);
  gfx.text("DIAL", 585, 244, C_TEXT, Gfx::F_TINY, 1);

  // signal flow diagram
  flowBox(gfx, 1530, 440, 70, "OSC1"); flowBox(gfx, 1530, 480, 70, "OSC2");
  flowBox(gfx, 1620, 460, 80, "OSC MOD"); flowBox(gfx, 1720, 460, 70, "FILTER"); flowBox(gfx, 1810, 460, 70, "AMP");
  flowBox(gfx, 1530, 540, 110, "CHORUS/FLG"); flowBox(gfx, 1660, 540, 100, "TEMPO DLY"); flowBox(gfx, 1780, 540, 100, "OUTPUT");
  gfx.line(1600, 455, 1620, 470, Color{60, 60, 64}, 2); gfx.line(1600, 495, 1620, 480, Color{60, 60, 64}, 2);
  gfx.line(1700, 475, 1720, 475, Color{60, 60, 64}, 2); gfx.line(1790, 475, 1810, 475, Color{60, 60, 64}, 2);
  gfx.line(1845, 490, 1845, 520, Color{60, 60, 64}, 2); gfx.line(1845, 520, 1585, 520, Color{60, 60, 64}, 2); gfx.line(1585, 520, 1585, 540, Color{60, 60, 64}, 2);
  gfx.line(1640, 555, 1660, 555, Color{60, 60, 64}, 2); gfx.line(1760, 555, 1780, 555, Color{60, 60, 64}, 2);
  gfx.text("one decay EG, on cutoff only · the amp is a gate", 1530, 584, C_TEXT_DIM, Gfx::F_TINY);
  gfx.text("chorus/flanger and tempo delay are per part", 1530, 600, C_TEXT_DIM, Gfx::F_TINY);
  gfx.text("PART 1", 1560, 625, C_TEXT, Gfx::F_SMALL, 1); gfx.text("PART 2", 1700, 625, C_TEXT, Gfx::F_SMALL, 1);
  gfx.text("edit part", 1840, 625, C_TEXT_DIM, Gfx::F_SMALL, 1);

  // step overview label
  gfx.text("PATTERN OVERVIEW  (part 1 / part 2, 64 steps)", 40, 636, C_TEXT_DIM, Gfx::F_TINY);

  // select keys and LED grid labels
  gfx.text("◀", 70, 788, C_WHITE, Gfx::F_BOLD, 1); gfx.text("▶", 140, 788, C_WHITE, Gfx::F_BOLD, 1);
  gfx.text("SELECT", 105, 756, C_TEXT, Gfx::F_LABEL, 1);
  gfx.text("green: playing bar", 105, 884, C_TEXT_DIM, Gfx::F_TINY, 1); gfx.text("red: shown bar / octave", 105, 898, C_TEXT_DIM, Gfx::F_TINY, 1);

  // labels for knobs and buttons
  for (const Control& c : ctls) {
    if (c.kind == K_KNOB) {
      gfx.text(c.label, c.cx, c.cy + c.radius + 16, C_TEXT, c.radius >= 60 ? Gfx::F_LABEL : Gfx::F_KNOB, 1);
      for (int i = 0; i <= 10; i++) {
        float a = (-135.0f + 27.0f * i) * 3.14159265f / 180.0f;
        int r0 = c.radius + 8, r1 = c.radius + (i % 5 == 0 ? 16 : 12);
        gfx.line((int)(c.cx + std::sin(a) * r0), (int)(c.cy - std::cos(a) * r0), (int)(c.cx + std::sin(a) * r1), (int)(c.cy - std::cos(a) * r1), Color{60, 60, 64}, 2);
      }
      if (c.scope == S_PART && c.id == P_EG_INT) gfx.text("0", c.cx, c.cy - c.radius - 30, C_TEXT_DIM, Gfx::F_TINY, 1);
    } else if (c.kind == K_BUTTON) {
      bool inside = c.scope == S_ACTION && (c.id == A_WRITE || c.id == A_MODE_PATTERN || c.id == A_MODE_SONG || c.id == A_MODE_GLOBAL || c.id == A_MODE_MIDI);
      if (c.label[0] && !inside) gfx.text(c.label, c.cx, c.cy - c.h / 2 - 22, C_TEXT, c.w > 100 ? Gfx::F_KNOB : Gfx::F_TINY, 1);
      if (c.ledCount) {
        const char* const* names = c.scope == S_PART ? kPartParams[c.id].names : nullptr;
        static const char* partNames[] = {"PART 1", "PART 2"};
        static const char* fxNames[] = {"TEMPO DELAY", "CHORUS/FLANGER"};
        if (c.scope == S_ACTION && c.id == A_PART_SELECT) names = partNames;
        if (c.scope == S_ACTION && c.id == A_FX_TYPE) names = fxNames;
        int n = c.ledCount; int span = (c.scope == S_ACTION && c.id == A_FX_TYPE) ? 210 : (n == 2 ? 100 : (c.scope == S_PART && c.id == P_MOTION_SEQ ? 200 : 190));
        int off = (c.scope == S_PART && c.id == P_OSC_MOD) ? 1 : 0;   // OSC MOD LEDs are RING / SYNC / DECI; off = all dark
        for (int i = 0; i < n; i++) {
          int lx = c.cx - span / 2 + span * (i + 0.5f) / n;
          gfx.text(names ? names[i + off] : "", lx, c.cy + c.h / 2 + 20, C_TEXT, Gfx::F_TINY, 1);
        }
      }
    } else if (c.kind == K_STEP) {
      gfx.text(std::to_string(c.id + 1), c.cx, c.cy - c.h / 2 - 46, C_TEXT, Gfx::F_LABEL, 1);
      gfx.text(kShiftNames[c.id], c.cx, c.cy + c.h / 2 + 8, C_TEXT_DIM, Gfx::F_TINY, 1);
    } else if (c.kind == K_ROW) {
      gfx.text(c.label, c.cx - 100, c.cy - 8, C_TEXT, Gfx::F_TINY, 0);
    }
  }
  SDL_SetRenderTarget(gfx.ren, nullptr);
}

// ---------- values ----------
int Panel::partParamFor(const Control& c) const {
  if (c.scope != S_PART) return -1;
  if (fxType == 1 && c.id == P_DELAY_DEPTH) return P_CHORUS_DEPTH;
  if (fxType == 1 && c.id == P_DELAY_TIME) return P_CHORUS_TIME;
  return c.id;
}
int Panel::value(const Control& c) const {
  if (c.scope == S_PART) return s.pat.part[s.editPart].p[partParamFor(c)];
  if (c.scope == S_GLOBAL) return s.pat.g[c.id];
  return 0;
}
void Panel::sendPartParam(int part, int p, int v) { Command c; c.type = Command::SetPart; c.a = part; c.b = p; c.c = v; eng.send(c); s.pat.part[part].p[p] = (uint8_t)v; dirty = true; }
void Panel::sendGlobal(int g, int v) { Command c; c.type = Command::SetGlobal; c.a = g; c.b = v; eng.send(c); s.pat.g[g] = (uint8_t)v; if (g != G_MASTER_VOLUME) dirty = true; }

void Panel::setValue(const Control& c, int v) {
  if (c.scope == S_PART) { int p = partParamFor(c); v = std::clamp(v, 0, (int)kPartParams[p].max); sendPartParam(s.editPart, p, v); }
  else if (c.scope == S_GLOBAL) { v = std::clamp(v, 0, (int)kGlobalParams[c.id].max); sendGlobal(c.id, v); }
  showValue(c);
}

std::string Panel::noteName(int n) const {
  static const char* nm[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  char b[16]; snprintf(b, sizeof b, "%s%d", nm[n % 12], n / 12 - 1); return b;
}
std::string Panel::noteName3(int n) const {
  static const char* nm[] = {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G ", "G#", "A ", "A#", "B "};
  int oct = n / 12 - 1; char b[8];
  if (oct < 0) snprintf(b, sizeof b, "%c-1", nm[n % 12][0]); else snprintf(b, sizeof b, "%s%d", nm[n % 12], oct);
  return b;
}

void Panel::showValue(const Control& c) {
  char b[32];
  int p = partParamFor(c);
  const ParamInfo& pi = c.scope == S_PART ? kPartParams[p] : kGlobalParams[c.id];
  int v = value(c);
  std::string val;
  switch (pi.kind) {
    case PK_KNOB: snprintf(b, sizeof b, "%3d", v * 100 / 127); val = std::to_string(v * 100 / 127); break;
    case PK_BIPOLAR_KNOB: snprintf(b, sizeof b, "%3d", (v - 64) * 100 / 63); val = std::to_string((v - 64) * 100 / 63); break;
    case PK_TOGGLE: snprintf(b, sizeof b, "%s", v ? " on" : "oFF"); val = v ? "ON" : "OFF"; break;
    case PK_ENUM: snprintf(b, sizeof b, "%.3s", pi.names[v]); val = pi.names[v]; break;
  }
  if (c.scope == S_PART && p == P_DELAY_TIME) { snprintf(b, sizeof b, "%3.3s", kDelayStepNames[delayIndex(v)]); val = std::string(kDelayStepNames[delayIndex(v)]) + " steps"; }
  if (c.scope == S_PART && p == P_CHORUS_TIME) { float hz = 0.2f * std::pow(25000.0f, v / 127.0f); snprintf(b, sizeof b, "%3d", v * 100 / 127); char h[32]; if (hz < 10) snprintf(h, sizeof h, "%.2f Hz", hz); else if (hz < 1000) snprintf(h, sizeof h, "%.0f Hz", hz); else snprintf(h, sizeof h, "%.2f kHz", hz / 1000); val = h; }
  if (c.scope == S_PART && p == P_OSC2_PITCH) { float semis = -12.0f + v * 60.0f / 127.0f; if (std::fabs(semis) < 0.6f) semis = 0; snprintf(b, sizeof b, "%3d", (int)std::lround(semis)); char h[32]; snprintf(h, sizeof h, "%+.1f semitones", semis); val = h; }
  if (c.scope == S_PART && p == P_CUTOFF) { float hz = 18.0f * std::exp2(v / 127.0f * 10.3f); char h[32]; if (hz < 1000) snprintf(h, sizeof h, "%.0f Hz", hz); else snprintf(h, sizeof h, "%.1f kHz", hz / 1000); val = std::to_string(v * 100 / 127) + "  (" + h + ")"; }
  if (c.scope == S_PART && p == P_DECAY) { float sec = 0.004f + std::pow(v / 127.0f, 2.2f) * 10.0f; char h[32]; if (sec < 1) snprintf(h, sizeof h, "%.0f ms", sec * 1000); else snprintf(h, sizeof h, "%.2f s", sec); val = std::to_string(v * 100 / 127) + "  (" + h + ")"; }
  if (c.scope == S_GLOBAL && c.id == G_SWING) { snprintf(b, sizeof b, "%3d", 50 + v); val = std::to_string(50 + v) + " %"; }
  msg(b, std::string(c.scope == S_PART ? (s.editPart == 0 ? "PART 1  " : "PART 2  ") : "") + pi.label + "  " + val, 1.5f);
}

std::string Panel::slotText(int sl) const {
  static const char banks[] = {'A', 'b', 'C', 'd'};
  char b[8]; snprintf(b, sizeof b, "%c%02d", banks[(sl / 64) & 3], (sl % 64) + 1); return b;
}

void Panel::showRow() {
  const Step& st = s.pat.part[s.editPart].step[targetStep];
  char b[32];
  switch (row) {
    case R_PATTERN: msg(slotText(slot), std::string("PATTERN ") + slotText(slot) + "  " + s.pat.name, 1.0f); break;
    case R_TEMPO: snprintf(b, sizeof b, "%3d", s.pat.tempo10 / 10); msg(b, "TEMPO " + std::to_string(s.pat.tempo10 / 10) + "." + std::to_string(s.pat.tempo10 % 10) + " BPM", 1.5f, s.pat.tempo10 % 10 >= 5 ? 2 : -1); break;
    case R_STEP: { int spb = s.pat.stepsPerBar(); snprintf(b, sizeof b, "%d%02d", targetStep / spb + 1, targetStep % spb + 1); msg(b, "TARGET STEP " + std::to_string(targetStep / spb + 1) + "." + std::to_string(targetStep % spb + 1) + (st.on ? "  (trigger on)" : "  (no trigger)"), 1.5f, 0); break; }
    case R_PITCH: msg(noteName3(st.note), "STEP " + std::to_string(targetStep + 1) + "  PITCH " + noteName(st.note), 1.5f); break;
    case R_GATE: { float g = (st.gate4 + 1) / 4.0f; if (g < 10) { snprintf(b, sizeof b, "%03d", (int)std::lround(g * 100)); msg(b, "STEP " + std::to_string(targetStep + 1) + "  GATE " + std::to_string(g).substr(0, 4) + " steps", 1.5f, 0); } else { snprintf(b, sizeof b, "%03d", (int)std::lround(g * 10)); msg(b, "STEP " + std::to_string(targetStep + 1) + "  GATE " + std::to_string(g).substr(0, 4) + " steps", 1.5f, 1); } break; }
  }
}

// ---------- slots ----------
void Panel::setSlot(int sl) {
  slot = ((sl % kNumPatterns) + kNumPatterns) % kNumPatterns;
  reclaim();
  if (spare.empty()) spare.push_back(new Pattern);
  Pattern* p = spare.back(); spare.pop_back();
  if (!store::loadPattern(slot, *p)) p->init();
  Command c; c.type = Command::LoadPattern; c.pat = p; c.a = 0; eng.send(c);
  dirty = false;
  msg(slotText(slot), std::string("PATTERN ") + slotText(slot) + "  " + p->name + (s.playing ? "  (at end of pattern)" : ""), 1.2f);
  if (s.playing) dispSub += ""; else { page = 0; targetStep = 0; }
}

void Panel::writeSlot() {
  Snapshot fresh; eng.snapshot(fresh);
  Pattern p = fresh.pat;
  // length-1 writes copy bar 1 into the other bars, as the hardware does (docs §3.1)
  int spb = p.stepsPerBar(), bars = p.g[G_LENGTH] + 1;
  if (bars < 4) for (int pi = 0; pi < kParts; pi++) for (int b = bars; b < 4; b++) for (int i = 0; i < spb; i++) p.part[pi].step[b * spb + i] = p.part[pi].step[(b % bars) * spb + i];
  if (store::savePattern(writeDest, p)) { msg(slotText(writeDest), "WRITTEN TO " + slotText(writeDest), 1.2f); if (writeDest == slot) dirty = false; else { slot = writeDest; dirty = false; } }
  else msg("Er1", "WRITE FAILED: " + store::dataDir(), 2.0f);
  writeArmed = false;
}

// ---------- input ----------
int Panel::hit(int x, int y) const {
  int best = -1; long bestD = 1L << 60;
  for (size_t i = 0; i < ctls.size(); i++) {
    const Control& c = ctls[i];
    int hw = c.w / 2, hh = c.h / 2;
    if (c.kind == K_KNOB || c.kind == K_DIAL) { hw = c.radius + 18; hh = c.radius + 18; }
    if (x >= c.cx - hw && x <= c.cx + hw && y >= c.cy - hh && y <= c.cy + hh) {
      long d = (long)(x - c.cx) * (x - c.cx) + (long)(y - c.cy) * (y - c.cy);
      if (d < bestD) { bestD = d; best = (int)i; }
    }
  }
  return best;
}

void Panel::shiftStep(int k) {
  useShift();
  Command c;
  switch (k) {
    case 0: case 1: case 2: case 3:
      if (s.playing) { msg("---", "LENGTH: stop playback first", 1.2f); break; }
      sendGlobal(G_LENGTH, k); msg(std::to_string(k + 1), "LENGTH " + std::to_string(k + 1) + (k == 0 ? " BAR" : " BARS"), 1.2f); if (page > k) page = 0; break;
    case 4: case 5: case 6:
      if (s.playing) { msg("---", "BEAT: stop playback first", 1.2f); break; }
      sendGlobal(G_BEAT, k - 4); msg(k == 4 ? " 16" : (k == 5 ? " 32" : "tri"), std::string("BEAT ") + kBeatNames[k - 4], 1.2f); break;
    case 7: swingDial = 1; swingTimer = 3.0f; msg(std::to_string(50 + s.pat.g[G_SWING]), "SWING " + std::to_string(50 + s.pat.g[G_SWING]) + " %  (turn the dial)", 3.0f); break;
    case 9: c.type = Command::CopyPart; c.a = s.editPart; c.b = 1 - s.editPart; eng.send(c); msg("CPy", "PART " + std::to_string(s.editPart + 1) + " COPIED TO PART " + std::to_string(2 - s.editPart), 1.2f); dirty = true; break;
    case 10: c.type = Command::ClearMotion; c.a = s.editPart; eng.send(c); msg("CLr", "MOTION SEQUENCE CLEARED (PART " + std::to_string(s.editPart + 1) + ")", 1.2f); dirty = true; break;
    case 11: c.type = Command::ClearPart; c.a = s.editPart; eng.send(c); msg("CLr", "PART " + std::to_string(s.editPart + 1) + " CLEARED", 1.2f); dirty = true; break;
    default: msg("---", std::string(kShiftNames[k]) + ": not in this version", 1.2f); break;
  }
}

void Panel::stepKey(int k, bool down) {
  int spb = s.pat.stepsPerBar();
  if (!keyboard && k >= spb) { if (down) msg("---", "triplet scale: 12 steps per bar", 0.8f); return; }
  int stepIdx = page * spb + k;
  if (transposeArmed) {
    if (down) { int t = k - 3; Command c; c.type = Command::SetTranspose; c.a = t; eng.send(c); s.transpose = t; msg((t >= 0 ? " " : "") + std::to_string(t), "TRANSPOSE " + std::to_string(t), 1.2f); }
    return;
  }
  if (keyboard) {
    int note = std::clamp(21 + 12 * kbdOct + k, 0, 127);
    if (down) {
      heldKeys[k] = true;
      if (row == R_PITCH) {
        Command c; c.type = Command::SetStepNote; c.a = s.editPart; c.b = targetStep; c.c = note; eng.send(c);
        s.pat.part[s.editPart].step[targetStep].note = (uint8_t)note; dirty = true;
        msg(noteName3(note), "STEP " + std::to_string(targetStep + 1) + "  PITCH " + noteName(note), 1.5f);
        targetStep = (targetStep + 1) % s.pat.steps();
      } else msg(noteName3(note), "KEY " + noteName(note), 0.8f);
      Command c; c.type = Command::NoteOn; c.a = s.editPart; c.b = note; eng.send(c);
      if (s.playing && s.rec) dirty = true;
    } else { heldKeys[k] = false; bool any = false; for (bool h : heldKeys) any |= h; if (!any) { Command c; c.type = Command::NoteOff; c.a = s.editPart; eng.send(c); } }
    return;
  }
  if (!down) return;
  if (shift()) { shiftStep(k); return; }
  if (row == R_STEP || row == R_PITCH || row == R_GATE) { targetStep = stepIdx; showRow(); return; }
  Command c; c.type = Command::ToggleStep; c.a = s.editPart; c.b = stepIdx; eng.send(c);
  s.pat.part[s.editPart].step[stepIdx].on = !s.pat.part[s.editPart].step[stepIdx].on; dirty = true;
}

void Panel::action(Action a, bool down) {
  Command c;
  switch (a) {
    case A_PLAY: if (down) { if (shift()) { useShift(); c.type = Command::ResetPlay; } else c.type = Command::PlayStop; eng.send(c); } break;
    case A_STOP: if (down) { if (writeArmed) { writeArmed = false; msg("---", "WRITE CANCELLED", 1.0f); break; } if (transposeArmed) { transposeArmed = false; } if (shift()) { useShift(); c.type = Command::AllNotesOff; eng.send(c); msg("---", "ALL NOTES OFF", 1.0f); break; } c.type = Command::Stop; eng.send(c); if (s.rec) { c.type = Command::SetRec; c.a = 0; eng.send(c); s.rec = false; } } break;
    case A_REC:
      if (shift() && s.playing && s.rec) { c.type = Command::EraseHold; c.a = down ? 1 : 0; eng.send(c); if (down) { useShift(); msg("ErS", "ERASING PART " + std::to_string(s.editPart + 1) + " TRIGGERS WHILE HELD", 1.0f); dirty = true; } break; }
      if (down) { c.type = Command::SetRec; c.a = !s.rec; eng.send(c); s.rec = !s.rec; }
      break;
    case A_WRITE:
      if (!down) break;
      if (!writeArmed) { writeArmed = true; writeDest = slot; msg(slotText(writeDest), "WRITE TO " + slotText(writeDest) + "?  dial picks the slot, WRITE confirms, STOP cancels", 4.0f); }
      else writeSlot();
      break;
    case A_TAP: if (down) { c.type = Command::TapTempo; eng.send(c); } break;
    case A_SHIFT:
      if (down) { shiftDown = true; shiftUsed = false; }
      else { shiftDown = false; if (!shiftUsed) shiftLatch = !shiftLatch; }
      break;
    case A_PART_SELECT: if (down) { c.type = Command::SetEditPart; c.a = 1 - s.editPart; eng.send(c); s.editPart = 1 - s.editPart; heldKeys[0] = heldKeys[0]; } break;
    case A_MUTE1: if (down) { c.type = Command::SetMute; c.a = 0; c.b = !s.pat.part[0].mute; eng.send(c); s.pat.part[0].mute = !s.pat.part[0].mute; } break;
    case A_MUTE2: if (down) { c.type = Command::SetMute; c.a = 1; c.b = !s.pat.part[1].mute; eng.send(c); s.pat.part[1].mute = !s.pat.part[1].mute; } break;
    case A_SEL_PREV: case A_SEL_NEXT: {
      if (!down) break;
      int d = a == A_SEL_NEXT ? 1 : -1;
      if (shift()) { useShift(); targetStep = (targetStep + d + s.pat.steps()) % s.pat.steps(); if (row < R_STEP) row = R_STEP; showRow(); break; }
      if (keyboard) { kbdOct = std::clamp(kbdOct + d, 0, 5); msg("oc" + std::to_string(kbdOct + 1), "KEYBOARD " + noteName(21 + 12 * kbdOct) + " .. " + noteName(21 + 12 * kbdOct + 15), 1.0f); }
      else { int bars = s.pat.g[G_LENGTH] + 1; page = (page + d + bars) % bars; }
      break;
    }
    case A_KEYBOARD: if (down) { keyboard = !keyboard; if (!keyboard) { c.type = Command::NoteOff; c.a = s.editPart; eng.send(c); for (bool& h : heldKeys) h = false; } msg(keyboard ? "Kbd" : "StP", keyboard ? "KEYBOARD FUNCTION ON" : "KEYBOARD FUNCTION OFF", 1.0f); } break;
    case A_TRANSPOSE: if (down) { transposeArmed = !transposeArmed; if (transposeArmed) msg((s.transpose >= 0 ? " " : "") + std::to_string(s.transpose), "TRANSPOSE: dial or step keys (key 4 = C), TRANSPOSE again to leave", 3.0f); } break;
    case A_CURSOR_UP: if (down) { row = (row + NUM_ROWS - 1) % NUM_ROWS; swingDial = 0; showRow(); } break;
    case A_CURSOR_DOWN: if (down) { row = (row + 1) % NUM_ROWS; swingDial = 0; showRow(); } break;
    case A_FX_TYPE: if (down) { fxType = 1 - fxType; msg(fxType ? "CHo" : "dLy", fxType ? "DEPTH / TIME now edit CHORUS-FLANGER" : "DEPTH / TIME now edit TEMPO DELAY", 1.2f); } break;
    case A_MODE_PATTERN: if (down) msg("Ptn", "PATTERN MODE", 0.8f); break;
    case A_MODE_SONG: case A_MODE_GLOBAL: case A_MODE_MIDI: if (down) msg("---", a == A_MODE_SONG ? "SONG MODE: not in this version" : (a == A_MODE_GLOBAL ? "GLOBAL MODE: not in this version" : "MIDI MODE: not in this version"), 1.2f); break;
    default: break;
  }
}

void Panel::press(int ci, bool down, int64_t finger) {
  if (ci < 0) return;
  const Control& c = ctls[ci];
  if (down) focus = ci;
  switch (c.kind) {
    case K_BUTTON:
      if (c.scope == S_ACTION) action((Action)c.id, down);
      else if (down) {
        if (c.scope == S_PART && c.id == P_MOTION_SEQ && shift()) { useShift(); Command cm; cm.type = Command::ClearMotion; cm.a = s.editPart; eng.send(cm); msg("CLr", "MOTION SEQUENCE CLEARED", 1.2f); break; }
        int p = partParamFor(c);
        int mx = c.scope == S_PART ? kPartParams[p].max : kGlobalParams[c.id].max;
        setValue(c, (value(c) + 1) % (mx + 1));
      }
      break;
    case K_STEP: stepKey(c.id, down); break;
    case K_ROW: if (down) { row = c.id; swingDial = 0; showRow(); } break;
    case K_KNOB: case K_DIAL:
      if (down) { drags[finger] = Drag{ci, 0, false}; if (c.kind == K_KNOB) showValue(c); else if (!writeArmed && !transposeArmed && !swingDial) showRow(); }
      break;
  }
}

void Panel::dial(int delta) {
  if (delta == 0) return;
  Command c;
  if (writeArmed) { writeDest = ((writeDest + delta) % kNumPatterns + kNumPatterns) % kNumPatterns; msg(slotText(writeDest), "WRITE TO " + slotText(writeDest) + "?  WRITE confirms, STOP cancels", 4.0f); return; }
  if (transposeArmed) { int t = std::clamp(s.transpose + delta, -24, 24); c.type = Command::SetTranspose; c.a = t; eng.send(c); s.transpose = t; msg((t >= 0 ? " " : "") + std::to_string(t), "TRANSPOSE " + std::to_string(t) + " semitones (not saved)", 2.0f); return; }
  if (swingDial) { if (s.playing) { msg("---", "SWING: stop playback first", 1.0f); return; } int v = std::clamp((int)s.pat.g[G_SWING] + delta, 0, 25); sendGlobal(G_SWING, v); swingTimer = 3.0f; msg(std::to_string(50 + v), "SWING " + std::to_string(50 + v) + " %", 3.0f); return; }
  Step& st = s.pat.part[s.editPart].step[targetStep];
  switch (row) {
    case R_PATTERN: setSlot(slot + delta); break;
    case R_TEMPO: { int t = std::clamp((int)s.pat.tempo10 + delta * 10 * (shift() ? 10 : 1), 200, 3000); if (shift()) useShift(); c.type = Command::SetTempo10; c.a = t; eng.send(c); s.pat.tempo10 = (uint16_t)t; dirty = true; showRow(); break; }
    case R_STEP: targetStep = (targetStep + delta % s.pat.steps() + s.pat.steps()) % s.pat.steps(); page = targetStep / s.pat.stepsPerBar(); showRow(); break;
    case R_PITCH: { int n = std::clamp((int)st.note + delta * (shift() ? 12 : 1), 0, 127); if (shift()) useShift(); st.note = (uint8_t)n; c.type = Command::SetStepNote; c.a = s.editPart; c.b = targetStep; c.c = n; eng.send(c); dirty = true; showRow(); break; }
    case R_GATE: { int g = std::clamp((int)st.gate4 + delta * (shift() ? 4 : 1), 0, 255); if (shift()) useShift(); st.gate4 = (uint8_t)g; c.type = Command::SetStepGate; c.a = s.editPart; c.b = targetStep; c.c = g; eng.send(c); dirty = true; showRow(); break; }
  }
}

void Panel::nudge(int ci, int delta) {
  if (ci < 0 || delta == 0) return;
  const Control& c = ctls[ci];
  if (c.kind == K_KNOB) setValue(c, value(c) + delta);
  else if (c.kind == K_DIAL) dial(delta);
  else if (c.kind == K_BUTTON && c.scope != S_ACTION) { int p = partParamFor(c); int mx = c.scope == S_PART ? kPartParams[p].max : kGlobalParams[c.id].max; setValue(c, std::clamp(value(c) + delta, 0, mx)); }
  else if (c.kind == K_ROW) { row = c.id; dial(delta); }
}

void Panel::dragKnob(int ci, float dx, float dy, int64_t finger) {
  auto it = drags.find(finger); if (it == drags.end()) return;
  Drag& d = it->second;
  const Control& c = ctls[ci];
  float sens = c.kind == K_DIAL ? 1.0f / 22.0f : (shift() ? 0.12f : 0.5f);
  d.accum += (-dy + dx) * sens;
  int steps = (int)d.accum;
  if (steps != 0) { d.accum -= steps; nudge(ci, steps); d.moved = true; }
}

void Panel::moveFocus(int dx, int dy) {
  const Control& f = ctls[focus];
  int best = -1; float bestScore = 1e30f;
  for (size_t i = 0; i < ctls.size(); i++) {
    if ((int)i == focus) continue;
    const Control& c = ctls[i];
    float vx = (float)(c.cx - f.cx), vy = (float)(c.cy - f.cy);
    float along = vx * dx + vy * dy, across = std::fabs(vx * dy - vy * dx);
    if (along <= 10.0f) continue;
    float score = along + across * 2.2f;
    if (score < bestScore) { bestScore = score; best = (int)i; }
  }
  if (best >= 0) focus = best;
}

void Panel::handleEvent(const SDL_Event& ev) {
  auto beginAt = [&](int x, int y, int64_t id) {
    int ci = hit(x, y); drags.erase(id); press(ci, true, id);
    if (ci >= 0 && ctls[ci].kind != K_KNOB && ctls[ci].kind != K_DIAL) drags[id] = Drag{ci, 0, false};
  };
  auto endAt = [&](int64_t id) {
    auto it = drags.find(id); if (it == drags.end()) return;
    const Control& c = ctls[it->second.ctl];
    if (c.kind == K_STEP || (c.kind == K_BUTTON && c.scope == S_ACTION)) press(it->second.ctl, false, id);
    drags.erase(it);
  };
  auto moveBy = [&](float dx, float dy, int64_t id) {
    auto it = drags.find(id); if (it == drags.end()) return;
    const Control& c = ctls[it->second.ctl];
    if (c.kind == K_KNOB || c.kind == K_DIAL) dragKnob(it->second.ctl, dx, dy, id);
  };
  switch (ev.type) {
    case SDL_QUIT: quit = true; break;
    case SDL_FINGERDOWN: beginAt((int)(ev.tfinger.x * W), (int)(ev.tfinger.y * H), (int64_t)ev.tfinger.fingerId); break;
    case SDL_FINGERMOTION: moveBy(ev.tfinger.dx * W, ev.tfinger.dy * H, (int64_t)ev.tfinger.fingerId); break;
    case SDL_FINGERUP: endAt((int64_t)ev.tfinger.fingerId); break;
    case SDL_MOUSEBUTTONDOWN: if (ev.button.which != SDL_TOUCH_MOUSEID) beginAt(ev.button.x, ev.button.y, -1000 - ev.button.button); break;
    case SDL_MOUSEBUTTONUP: if (ev.button.which != SDL_TOUCH_MOUSEID) endAt(-1000 - ev.button.button); break;
    case SDL_MOUSEMOTION: if (ev.motion.which != SDL_TOUCH_MOUSEID) moveBy((float)ev.motion.xrel, (float)ev.motion.yrel, -1000 - SDL_BUTTON_LEFT); break;
    case SDL_MOUSEWHEEL: nudge(focus, ev.wheel.y * (shift() ? 1 : 4)); break;
    case SDL_CONTROLLERDEVICEADDED: if (!pad) { pad = SDL_GameControllerOpen(ev.cdevice.which); if (pad) fprintf(stderr, "pad: %s\n", SDL_GameControllerName(pad)); } break;
    case SDL_CONTROLLERDEVICEREMOVED: if (pad && ev.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pad))) { SDL_GameControllerClose(pad); pad = nullptr; } break;
    case SDL_CONTROLLERBUTTONDOWN: case SDL_CONTROLLERBUTTONUP: {
      bool down = ev.type == SDL_CONTROLLERBUTTONDOWN;
      switch (ev.cbutton.button) {
        case SDL_CONTROLLER_BUTTON_START: if (down) action(A_PLAY, true); break;
        case SDL_CONTROLLER_BUTTON_BACK: if (down) { if (shift()) quit = true; else action(A_REC, true); } else if (shift()) action(A_REC, false); break;
        case SDL_CONTROLLER_BUTTON_GUIDE: if (down) quit = true; break;
        case SDL_CONTROLLER_BUTTON_B: action(A_SHIFT, down); if (!down) shiftLatch = false; break;
        case SDL_CONTROLLER_BUTTON_X: if (down) action(A_KEYBOARD, true); break;
        case SDL_CONTROLLER_BUTTON_Y: if (down) action(A_PART_SELECT, true); break;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: if (down) action(A_SEL_PREV, true); break;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: if (down) action(A_SEL_NEXT, true); break;
        case SDL_CONTROLLER_BUTTON_LEFTSTICK: if (down) action(A_TRANSPOSE, true); break;
        case SDL_CONTROLLER_BUTTON_RIGHTSTICK: if (down) action(A_WRITE, true); break;
        case SDL_CONTROLLER_BUTTON_A: {
          const Control& c = ctls[focus];
          if (c.kind == K_KNOB) { if (down) showValue(c); }
          else if (c.kind == K_DIAL) { if (down) showRow(); }
          else press(focus, down, -1);
          break;
        }
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: if (down) { if (shift()) { useShift(); dial(-1); shiftLatch = false; } else moveFocus(-1, 0); } break;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: if (down) { if (shift()) { useShift(); dial(1); } else moveFocus(1, 0); } break;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: if (down) { if (shift()) { useShift(); action(A_CURSOR_UP, true); } else moveFocus(0, -1); } break;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: if (down) { if (shift()) { useShift(); action(A_CURSOR_DOWN, true); } else moveFocus(0, 1); } break;
      }
      break;
    }
    case SDL_CONTROLLERAXISMOTION: {
      float v = ev.caxis.value / 32767.0f;
      if (std::fabs(v) < 0.12f) v = 0.0f;
      switch (ev.caxis.axis) {
        case SDL_CONTROLLER_AXIS_LEFTY: stickL = -v; break;
        case SDL_CONTROLLER_AXIS_RIGHTX: stickRX = v; break;
        case SDL_CONTROLLER_AXIS_RIGHTY: stickRY = -v; break;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT: { static bool was = false; bool now = v > 0.5f; if (now && !was) action(s.editPart == 0 ? A_MUTE1 : A_MUTE2, true); was = now; break; }
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: { static bool was = false; bool now = v > 0.5f; if (now && !was) action(A_TAP, true); was = now; break; }
      }
      break;
    }
    case SDL_KEYDOWN: case SDL_KEYUP: {
      bool down = ev.type == SDL_KEYDOWN;
      if (ev.key.repeat) break;
      SDL_Keycode k = ev.key.keysym.sym;
      static const SDL_Keycode stepKeys[16] = {SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5, SDLK_6, SDLK_7, SDLK_8, SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i};
      for (int i = 0; i < 16; i++) if (k == stepKeys[i]) { stepKey(i, down); return; }
      if (k == SDLK_LSHIFT || k == SDLK_RSHIFT) { action(A_SHIFT, down); if (!down) shiftLatch = false; break; }
      if (!down) break;
      switch (k) {
        case SDLK_ESCAPE: quit = true; break;
        case SDLK_SPACE: action(A_PLAY, true); break;
        case SDLK_RETURN: action(A_REC, true); break;
        case SDLK_BACKSPACE: action(A_STOP, true); break;
        case SDLK_k: action(A_KEYBOARD, true); break;
        case SDLK_p: action(A_PART_SELECT, true); break;
        case SDLK_m: action(s.editPart == 0 ? A_MUTE1 : A_MUTE2, true); break;
        case SDLK_LEFTBRACKET: action(A_SEL_PREV, true); break;
        case SDLK_RIGHTBRACKET: action(A_SEL_NEXT, true); break;
        case SDLK_w: if (SDL_GetModState() & KMOD_CTRL) action(A_WRITE, true); break;
        case SDLK_PAGEUP: action(A_CURSOR_UP, true); break;
        case SDLK_PAGEDOWN: action(A_CURSOR_DOWN, true); break;
        case SDLK_COMMA: dial(-1); break;
        case SDLK_PERIOD: dial(1); break;
        case SDLK_LEFT: moveFocus(-1, 0); break;
        case SDLK_RIGHT: moveFocus(1, 0); break;
        case SDLK_UP: moveFocus(0, -1); break;
        case SDLK_DOWN: moveFocus(0, 1); break;
        case SDLK_EQUALS: case SDLK_PLUS: nudge(focus, 4); break;
        case SDLK_MINUS: nudge(focus, -4); break;
        case SDLK_a: press(focus, true, -1); press(focus, false, -1); break;
        case SDLK_TAB: action(A_TAP, true); break;
        case SDLK_f: action(A_FX_TYPE, true); break;
      }
      break;
    }
  }
}

void Panel::update(float dt) {
  eng.snapshot(s);
  reclaim();
  blink += dt;
  if (msgTimer > 0) msgTimer -= dt;
  if (swingDial && (swingTimer -= dt) <= 0) swingDial = 0;
  if (stickL != 0.0f) {
    float rate = (stickL < 0 ? -1.0f : 1.0f) * stickL * stickL * (shift() ? 25.0f : 90.0f);
    knobAccum += rate * dt; int st = (int)knobAccum;
    if (st != 0) { knobAccum -= st; nudge(focus, st); }
  } else knobAccum = 0.0f;
  auto rstick = [&](float v, float& acc, int param) {
    if (v == 0.0f) { acc = 0.0f; return; }
    float rate = (v < 0 ? -1.0f : 1.0f) * v * v * 110.0f;
    acc += rate * dt; int st = (int)acc;
    if (st != 0) { acc -= st; int cur = s.pat.part[s.editPart].p[param]; int nv = std::clamp(cur + st, 0, 127); if (nv != cur) { sendPartParam(s.editPart, param, nv); for (size_t i = 0; i < ctls.size(); i++) if (ctls[i].kind == K_KNOB && ctls[i].scope == S_PART && ctls[i].id == param) showValue(ctls[i]); } }
  };
  rstick(stickRY, ryAccum, P_CUTOFF);
  rstick(stickRX, rxAccum, P_RESONANCE);
  int bars = s.pat.g[G_LENGTH] + 1;
  if (page >= bars) page = 0;
  if (targetStep >= s.pat.steps()) targetStep = 0;
}

// ---------- drawing ----------
void Panel::drawKnob(const Control& c) {
  int p = partParamFor(c);
  const ParamInfo& pi = c.scope == S_PART ? kPartParams[p] : kGlobalParams[c.id];
  int v = value(c), mx = pi.max;
  float frac = (float)v / mx;
  float ang = -135.0f + 270.0f * frac;
  Color arcC{255, 120, 30, 230};
  float a0 = (-135.0f - 90.0f) * 3.14159265f / 180.0f, a1 = (ang - 90.0f) * 3.14159265f / 180.0f;
  if (pi.kind == PK_BIPOLAR_KNOB) a0 = -90.0f * 3.14159265f / 180.0f;
  gfx.arc(c.cx, c.cy, c.radius + 9, 3, std::min(a0, a1), std::max(a0, a1), arcC);
  gfx.blit(gfx.knobBody(c.radius), c.cx, c.cy);
  gfx.blit(gfx.knobPointer(c.radius), c.cx, c.cy, ang);
  if (c.scope == S_PART && (c.id == P_DELAY_DEPTH || c.id == P_DELAY_TIME)) {
    // which effect the knob is editing right now
    gfx.text(fxType ? "CHORUS" : "DELAY", c.cx, c.cy - c.radius - 36, Color{200, 30, 20}, Gfx::F_SMALL, 1);
  }
  if (pi.motion && c.scope == S_PART) {
    bool active = s.pat.part[s.editPart].motion[p].active;
    bool recing = s.motionRecLane[s.editPart][p];
    int lx = c.cx + c.radius + 10, ly = c.cy - c.radius - 4;
    if (recing && std::fmod(blink, 0.3f) < 0.15f) gfx.blit(gfx.led(5, C_LED_RED), lx, ly);
    else if (active) gfx.blit(gfx.led(5, C_LED_ORANGE), lx, ly);
    else gfx.blit(gfx.ledOff(5), lx, ly);
  }
}

void Panel::drawDial(const Control& c) {
  float ang = (slot % 64) * 360.0f / 64.0f;
  gfx.blit(gfx.knobBody(c.radius), c.cx, c.cy);
  gfx.circle(c.cx, c.cy, c.radius - 6, Color{44, 44, 48});
  gfx.circle(c.cx, c.cy, c.radius - 8, Color{38, 38, 42});
  float a = (ang - 90.0f) * 3.14159265f / 180.0f;
  gfx.circle((int)(c.cx + std::cos(a) * (c.radius - 20)), (int)(c.cy + std::sin(a) * (c.radius - 20)), 9, Color{22, 22, 24});
  gfx.circle((int)(c.cx + std::cos(a) * (c.radius - 20)) - 1, (int)(c.cy + std::sin(a) * (c.radius - 20)) - 1, 7, Color{70, 70, 74});
}

void Panel::drawButton(const Control& c) {
  int x = c.cx - c.w / 2, y = c.cy - c.h / 2;
  bool lit = false, down = false, big = false;
  Color led = C_LED_RED;
  Color body = C_BTN;
  int v = c.scope == S_ACTION ? 0 : value(c);
  if (c.scope == S_ACTION) {
    switch (c.id) {
      case A_PLAY: lit = s.playing; led = C_LED_GREEN; big = true; break;
      case A_STOP: big = true; break;
      case A_TAP: big = true; break;
      case A_REC: lit = s.rec && (!s.playing || std::fmod(blink, 0.5f) < 0.3f); body = Color{150, 30, 26}; big = true; break;
      case A_TRANSPOSE: lit = s.transpose != 0 || (transposeArmed && std::fmod(blink, 0.4f) < 0.2f); led = C_LED_ORANGE; big = true; break;
      case A_SHIFT: lit = down = shift(); led = C_LED_ORANGE; break;
      case A_MUTE1: lit = s.pat.part[0].mute; led = C_LED_ORANGE; break;
      case A_MUTE2: lit = s.pat.part[1].mute; led = C_LED_ORANGE; break;
      case A_KEYBOARD: lit = keyboard; led = C_LED_ORANGE; break;
      case A_WRITE: lit = writeArmed ? std::fmod(blink, 0.4f) < 0.2f : (dirty && std::fmod(blink, 1.0f) < 0.5f); led = C_LED_ORANGE; break;
      case A_MODE_PATTERN: lit = true; break;
      default: break;
    }
  } else if (c.ledCount == 0) lit = v != 0;
  gfx.roundRect(x, y + 3, c.w, c.h, 7, Color{0, 0, 0, 70});
  gfx.roundRect(x, y, c.w, c.h, 7, down ? C_BTN_DOWN : body);
  gfx.roundRect(x + 2, y + 2, c.w - 4, c.h / 2 - 2, 5, Color{255, 255, 255, down ? 10 : 22});
  if (c.ledCount == 0) {
    gfx.blit(lit ? gfx.led(5, led) : gfx.ledOff(5), x + 14, y + 13);
    if (c.scope == S_ACTION && big) {
      const char* sym = c.id == A_PLAY ? "▶ ‖" : (c.id == A_STOP ? "■" : (c.id == A_REC ? "●" : (c.id == A_TAP ? "TAP" : "TRANS")));
      gfx.text(sym, c.cx + 6, c.cy - 12, c.id == A_REC ? Color{255, 90, 70} : C_WHITE, Gfx::F_BOLD, 1);
    } else if (c.scope == S_ACTION && (c.id == A_MODE_PATTERN || c.id == A_MODE_SONG || c.id == A_MODE_GLOBAL || c.id == A_MODE_MIDI || c.id == A_WRITE)) {
      gfx.text(c.label, c.cx + 8, c.cy - 8, C_WHITE, Gfx::F_TINY, 1);
    }
  } else {
    int cur = v;
    if (c.scope == S_ACTION && c.id == A_PART_SELECT) cur = s.editPart;
    if (c.scope == S_ACTION && c.id == A_FX_TYPE) cur = fxType;
    int n = c.ledCount; int span = (c.scope == S_ACTION && c.id == A_FX_TYPE) ? 210 : (n == 2 ? 100 : (c.scope == S_PART && c.id == P_MOTION_SEQ ? 200 : 190));
    for (int i = 0; i < n; i++) {
      int lx = c.cx - span / 2 + span * (i + 0.5f) / n;
      bool on = i == cur;
      if (c.scope == S_PART && c.id == P_OSC_MOD) on = (i + 1 == cur);   // off = all dark, like the hardware
      gfx.blit(on ? gfx.led(4, C_LED_RED) : gfx.ledOff(4), lx, c.cy + c.h / 2 + 10);
    }
  }
}

void Panel::drawStep(const Control& c) {
  int x = c.cx - c.w / 2, y = c.cy - c.h / 2;
  int spb = s.pat.stepsPerBar();
  int idx = page * spb + c.id;
  bool unused = !keyboard && c.id >= spb;
  const PartData& pd = s.pat.part[s.editPart];
  const Step& st = pd.step[std::min(idx, kMaxSteps - 1)];
  bool isCur = s.playing && s.step == idx && !unused;
  bool pressed = heldKeys[c.id];
  gfx.roundRect(x, y + 4, c.w, c.h, 6, Color{0, 0, 0, 60});
  gfx.roundRect(x, y, c.w, c.h, 6, pressed ? C_KEY_DOWN : (unused ? Color{200, 200, 198} : C_KEY));
  gfx.rect(x + 4, y + c.h - 12, c.w - 8, 6, Color{0, 0, 0, 35});
  gfx.roundRect(x + 3, y + 3, c.w - 6, 24, 4, Color{255, 255, 255, 110});
  if (c.id % 4 == 0 && c.id) gfx.rect(x - 7, y + 10, 2, c.h - 20, Color{0, 0, 0, 40});
  bool on = false; Color col = C_LED_RED;
  if (keyboard) {
    int note = 21 + 12 * kbdOct + c.id; int pc = note % 12;
    bool black = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
    if (black) gfx.roundRect(x + 8, y + 34, c.w - 16, c.h - 90, 4, Color{40, 40, 44});
    gfx.text(noteName(note), c.cx, y + c.h - 40, C_TEXT, Gfx::F_SMALL, 1);
    on = pressed || (s.playing && s.soundingNote[s.editPart] == note);
  } else if (!unused) {
    on = st.on;
    if (st.on) gfx.text(noteName(st.note), c.cx, y + c.h - 40, C_TEXT, Gfx::F_SMALL, 1);
    if (st.on && st.gate4 >= 4) gfx.text("tie", c.cx, y + 36, C_TEXT_DIM, Gfx::F_SMALL, 1);
    bool isTarget = (row >= R_STEP) && targetStep == idx;
    if (isTarget) { gfx.frame(x - 3, y - 3, c.w + 6, c.h + 6, C_LED_ORANGE, 3); if (std::fmod(blink, 0.5f) < 0.25f) { on = true; col = C_LED_ORANGE; } }
  }
  if (shift() && !keyboard) {
    // shift held: keys 1-4 show length, 5-7 beat (hardware behaviour)
    if (c.id < 4) on = c.id == s.pat.g[G_LENGTH];
    else if (c.id < 7) on = c.id - 4 == s.pat.g[G_BEAT];
    else on = false;
    col = C_LED_ORANGE;
  }
  if (isCur) { col = on ? Color{255, 200, 60} : C_LED_ORANGE; on = true; }
  gfx.blit(on ? gfx.led(7, col) : gfx.ledOff(7), c.cx, y - 18);
  const Step& ot = s.pat.part[1 - s.editPart].step[std::min(idx, kMaxSteps - 1)];
  if (!unused && !keyboard && ot.on) gfx.circle(c.cx, y - 4, 2, Color{120, 60, 60});
}

void Panel::drawOverview() {
  int x0 = 40, y0 = 656, cw = 10, ch = 16;
  int len = s.pat.steps(), spb = s.pat.stepsPerBar();
  for (int p = 0; p < kParts; p++) {
    for (int i = 0; i < kMaxSteps; i++) {
      int x = x0 + i * cw, y = y0 + p * (ch + 4);
      bool in = i < len;
      Color bg = in ? Color{165, 165, 168} : Color{190, 190, 190};
      if (in && (i / spb) % 2 == 1) bg = Color{150, 150, 154};
      gfx.rect(x, y, cw - 1, ch, bg);
      const Step& st = s.pat.part[p].step[i];
      if (in && st.on) { int h = 4 + (st.note % 24) * (ch - 4) / 24; gfx.rect(x + 1, y + ch - h, cw - 3, h, p == s.editPart ? Color{220, 40, 20} : Color{120, 50, 40}); }
      if (s.playing && s.step == i) gfx.rect(x, y, cw - 1, ch, Color{255, 200, 60, 150});
    }
  }
  gfx.text(std::string("P") + std::to_string(s.editPart + 1), x0 + 64 * cw + 6, y0 + 4, Color{200, 30, 20}, Gfx::F_SMALL);
}

void Panel::drawDisplay() {
  std::string txt = msgTimer > 0 ? disp : slotText(slot);
  int dot = msgTimer > 0 ? dispDot : -1;
  bool blinkOff = s.pendingPattern && std::fmod(blink, 0.5f) < 0.25f && msgTimer <= 0;
  while (txt.size() < 3) txt = " " + txt;
  int x0 = 196, y0 = 162, h = 66;
  for (int i = 0; i < 3; i++) gfx.sevenSeg(blinkOff ? ' ' : txt[i], x0 + i * 54, y0, h, C_SEG_ON, C_SEG_OFF, i == dot);
  // dirty marker: the rightmost decimal point
  if (dirty && dot < 0 && !blinkOff) gfx.circle(x0 + 3 * 54 - 6, y0 + h - 4, 4, C_SEG_ON);
  // info strip under the display (not on the hardware, but a touchscreen needs it)
  std::string sub = msgTimer > 0 ? dispSub : (std::string("PATTERN ") + slotText(slot) + "  " + s.pat.name + (dirty ? "  (edited)" : ""));
  gfx.text(sub, 180, 332, Color{70, 70, 76}, Gfx::F_SMALL, 0);
  // beat LED
  bool beat = s.playing && (s.step % (s.pat.stepsPerBar() == 12 ? 3 : 4)) == 0;
  gfx.blit(beat ? gfx.led(6, C_LED_RED) : gfx.ledOff(6), 500, 175);
  // matrix LEDs
  for (int r = 0; r < NUM_ROWS; r++) gfx.blit(r == row ? gfx.led(4, C_LED_RED) : gfx.ledOff(4), 182, 242 + r * 20);
  // select LED grid: green = playing bar, red = shown bar (or keyboard octave pattern)
  int bars = s.pat.g[G_LENGTH] + 1;
  static const uint8_t octLeds[6] = {0x1, 0x3, 0x7, 0xE, 0xC, 0x8};
  for (int i = 0; i < 4; i++) {
    int lx = 60 + i * 36;
    bool green = s.playing && (s.step / s.pat.stepsPerBar()) == i;
    bool red = keyboard ? (octLeds[kbdOct] >> i) & 1 : (page == i);
    gfx.blit(green ? gfx.led(5, C_LED_GREEN) : gfx.ledOff(5), lx, 850);
    gfx.blit(red ? gfx.led(5, C_LED_RED) : (i < bars ? gfx.ledOff(5) : gfx.ledOff(3)), lx, 874);
  }
  // part meters + sounding notes in the signal-flow box
  for (int p = 0; p < kParts; p++) {
    int mx = 1560 + p * 140; float lv = std::min(1.0f, s.partLevel[p] * 1.4f);
    gfx.rect(mx - 50, 650, 100, 14, Color{60, 60, 64});
    int w = (int)(lv * 100); gfx.rect(mx - 50, 650, w, 14, lv > 0.9f ? Color{255, 60, 30} : Color{255, 140, 40});
    gfx.text(s.soundingNote[p] >= 0 ? noteName(s.soundingNote[p]) : (s.pat.part[p].mute ? "MUTE" : "-"), mx, 668, C_TEXT, Gfx::F_SMALL, 1);
  }
  gfx.blit(gfx.led(7, s.editPart == 0 ? C_LED_RED : C_LED_ORANGE), 1840, 660);
  gfx.text(s.editPart == 0 ? "1" : "2", 1840, 668, C_TEXT, Gfx::F_BOLD, 1);
  // tempo / status line in the sequence-control box
  char b[128]; snprintf(b, sizeof b, "%d.%d BPM   %s%s   LEN %d   %s   SWING %d%%   TRANSPOSE %+d", s.pat.tempo10 / 10, s.pat.tempo10 % 10, s.playing ? "PLAYING" : "STOPPED", s.rec ? " · REC" : "", bars, kBeatNames[s.pat.g[G_BEAT]], 50 + s.pat.g[G_SWING], s.transpose);
  gfx.text(b, 1880, 726, Color{235, 235, 235}, Gfx::F_LABEL, 2);
}

void Panel::draw() {
  SDL_RenderCopy(gfx.ren, staticLayer, nullptr, nullptr);
  for (const Control& c : ctls) {
    switch (c.kind) {
      case K_KNOB: drawKnob(c); break;
      case K_BUTTON: drawButton(c); break;
      case K_STEP: drawStep(c); break;
      case K_DIAL: drawDial(c); break;
      case K_ROW: break;
    }
  }
  drawOverview();
  drawDisplay();
  if (focus >= 0 && focus < (int)ctls.size()) {
    const Control& c = ctls[focus];
    int hw = c.w / 2 + 6, hh = c.h / 2 + 6;
    if (c.kind == K_KNOB || c.kind == K_DIAL) { hw = c.radius + 20; hh = c.radius + 20; }
    uint8_t a = (uint8_t)(150 + 100 * std::sin(blink * 6.0f));
    gfx.roundRect(c.cx - hw, c.cy - hh, 2 * hw, 2 * hh, 10, Color{C_FOCUS.r, C_FOCUS.g, C_FOCUS.b, (uint8_t)(a / 6)});
    for (int t = 0; t < 3; t++) { SDL_SetRenderDrawColor(gfx.ren, C_FOCUS.r, C_FOCUS.g, C_FOCUS.b, a); SDL_Rect r{c.cx - hw + t, c.cy - hh + t, 2 * hw - 2 * t, 2 * hh - 2 * t}; SDL_RenderDrawRect(gfx.ren, &r); }
  }
}
