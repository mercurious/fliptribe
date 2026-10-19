// fliptribe: the skeuomorphic EA-1 panel. Handles touch, mouse, gamepad and keyboard input, draws the panel
// and talks to the Engine only through Commands and Snapshots.
#pragma once
#include "engine.h"
#include "gfx.h"
#include "sync.h"
#include <vector>
#include <map>
#include <string>

enum CtlKind : uint8_t { K_KNOB, K_BUTTON, K_STEP, K_DIAL, K_ROW };
enum Scope : uint8_t { S_PART, S_GLOBAL, S_ACTION };
enum Action : uint8_t { A_NONE, A_PLAY, A_STOP, A_REC, A_WRITE, A_TAP, A_SHIFT, A_PART_SELECT, A_MUTE1, A_MUTE2,
                        A_SEL_PREV, A_SEL_NEXT, A_KEYBOARD, A_TRANSPOSE, A_CURSOR_UP, A_CURSOR_DOWN, A_FX_TYPE,
                        A_MODE_PATTERN, A_MODE_SONG, A_MODE_GLOBAL, A_MODE_MIDI, A_SYNC };
enum Row : uint8_t { R_PATTERN, R_TEMPO, R_STEP, R_PITCH, R_GATE, NUM_ROWS };

struct Control {
  CtlKind kind; Scope scope; int id;
  int cx, cy, w, h;          // centre and hit size
  int radius;                // knob radius
  const char* label;
  int ledCount;              // enum buttons: LED row under the key
};

class Panel {
public:
  Panel(Engine& e, Gfx& g) : eng(e), gfx(g) {}
  bool init(int slot, int master);
  void setSync(WifiSync* w) { sync = w; }     // before init; null = no Wi-Fi sync key action
  void handleEvent(const SDL_Event& ev);
  void update(float dt);
  void draw();
  bool wantsQuit() const { return quit; }
  void shutdown();
  int currentSlot() const { return slot; }
  int masterVolume() const { return s.pat.g[G_MASTER_VOLUME]; }

private:
  Engine& eng; Gfx& gfx;
  WifiSync* sync = nullptr;
  Snapshot s;
  std::vector<Control> ctls;
  SDL_Texture* staticLayer = nullptr;
  int slot = 0, page = 0, focus = 0;
  int row = R_PATTERN, targetStep = 0;
  bool keyboard = false, quit = false, dirty = false;
  bool shiftDown = false, shiftLatch = false, shiftUsed = false;
  bool transposeArmed = false, writeArmed = false; int writeDest = 0;
  int fxType = 0;               // 0 tempo delay, 1 chorus/flanger: which pair the DEPTH/TIME knobs edit
  int kbdOct = 3;               // 0..5 -> A0..C7 (table in docs §1.4)
  int swingDial = 0;            // >0 while the dial edits swing (after Shift+8)
  float swingTimer = 0.0f;
  float msgTimer = 0.0f; std::string disp, dispSub; int dispDot = -1;
  float blink = 0.0f;
  float stickL = 0.0f, stickRX = 0.0f, stickRY = 0.0f, knobAccum = 0.0f, rxAccum = 0.0f, ryAccum = 0.0f;
  bool heldKeys[16] = {};
  struct Drag { int ctl; float accum; bool moved; };
  std::map<int64_t, Drag> drags;
  SDL_GameController* pad = nullptr;
  std::vector<Pattern*> spare;

  bool shift() const { return shiftDown || shiftLatch; }
  void useShift() { shiftUsed = true; shiftLatch = false; }
  void layout();
  void buildStatic();
  int hit(int x, int y) const;
  void press(int ci, bool down, int64_t finger);
  void dragKnob(int ci, float dx, float dy, int64_t finger);
  void nudge(int ci, int delta);
  void dial(int delta);
  int value(const Control& c) const;
  int partParamFor(const Control& c) const;   // resolves DEPTH/TIME to delay or chorus
  void setValue(const Control& c, int v);
  void showValue(const Control& c);
  void showRow();
  void action(Action a, bool down);
  void stepKey(int k, bool down);
  void shiftStep(int k);
  void setSlot(int sl);
  void writeSlot();
  void moveFocus(int dx, int dy);
  void reclaim();
  void drawKnob(const Control& c);
  void drawButton(const Control& c);
  void drawStep(const Control& c);
  void drawDisplay();
  void drawDial(const Control& c);
  void drawOverview();
  std::string slotText(int sl) const;
  std::string noteName(int n) const;
  std::string noteName3(int n) const;
  void sendPartParam(int part, int p, int v);
  void sendGlobal(int g, int v);
  void msg(const std::string& d, const std::string& sub, float t = 1.2f, int dot = -1) { disp = d; dispSub = sub; msgTimer = t; dispDot = dot; }
};
