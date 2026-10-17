// fliptribe: Korg Electribe EA-1 recreation for the Retroid Pocket Flip 2 (ROCKNIX). Entry point.
//   fliptribe                      run the panel full screen
//   fliptribe --window             run in a 1920x1080 window (desktop testing)
//   fliptribe --play               start the sequencer right away (testing over ssh)
//   fliptribe --render out.wav --seconds 8 [--slot A01]   render a pattern offline, no window, no audio device
//   fliptribe --screenshot out.bmp [--slot A01]           draw one frame offscreen and save it
// Env: FLIPTRIBE_DATA (user patterns + settings), FLIPTRIBE_ASSETS, FLIPTRIBE_PATTERNS, FLIPTRIBE_RATE (default 39062.5,
// the hardware's rate; SDL resamples to the device).
#include <SDL.h>
#include "engine.h"
#include "gfx.h"
#include "ui.h"
#include "store.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <cmath>

static Engine* gEngine = nullptr;
static void audioCallback(void*, Uint8* stream, int len) {
  float* out = (float*)stream;
  int frames = len / (int)(sizeof(float) * 2);
  gEngine->render(out, frames);
}

static int parseSlot(const char* s) {
  if (!s || strlen(s) < 2) return 0;
  char b = toupper((unsigned char)s[0]); int n = atoi(s + 1);
  int bank = b == 'A' ? 0 : (b == 'B' ? 1 : (b == 'C' ? 2 : 3));
  return bank * 64 + std::max(0, std::min(63, n - 1));
}

static bool writeWav(const std::string& path, const std::vector<float>& inter, int rate) {
  FILE* f = fopen(path.c_str(), "wb"); if (!f) return false;
  uint32_t n = (uint32_t)inter.size(); uint32_t dataBytes = n * 2; uint32_t riff = 36 + dataBytes;
  uint16_t ch = 2, bits = 16, blockAlign = 4; uint32_t byteRate = rate * blockAlign; uint32_t fmtLen = 16; uint16_t fmt = 1;
  fwrite("RIFF", 1, 4, f); fwrite(&riff, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f); fwrite(&fmtLen, 4, 1, f); fwrite(&fmt, 2, 1, f); fwrite(&ch, 2, 1, f);
  fwrite(&rate, 4, 1, f); fwrite(&byteRate, 4, 1, f); fwrite(&blockAlign, 2, 1, f); fwrite(&bits, 2, 1, f); fwrite("data", 1, 4, f); fwrite(&dataBytes, 4, 1, f);
  for (float v : inter) { int16_t s = (int16_t)std::lround(std::max(-1.0f, std::min(1.0f, v)) * 32767.0f); fwrite(&s, 2, 1, f); }
  fclose(f); return true;
}

int main(int argc, char** argv) {
  std::string renderPath, shotPath; float seconds = 8.0f; int slot = -1; bool windowed = false, autoplay = false;
  for (int i = 1; i < argc; i++) {
    std::string a = argv[i];
    if (a == "--render" && i + 1 < argc) renderPath = argv[++i];
    else if (a == "--screenshot" && i + 1 < argc) shotPath = argv[++i];
    else if (a == "--seconds" && i + 1 < argc) seconds = (float)atof(argv[++i]);
    else if (a == "--slot" && i + 1 < argc) slot = parseSlot(argv[++i]);
    else if (a == "--window") windowed = true;
    else if (a == "--play") autoplay = true;
    else if (a == "--help" || a == "-h") { printf("fliptribe [--window] [--render out.wav --seconds N] [--screenshot out.bmp] [--slot A01]\n"); return 0; }
  }
  float rate = 39062.5f;
  if (const char* r = getenv("FLIPTRIBE_RATE")) rate = (float)atof(r);

  json::Value settings;
  store::loadSettings(settings);
  if (slot < 0) slot = (int)settings.numOr("slot", 0);
  int master = (int)settings.numOr("master", 100);
  store::seedFromShipped();

  Engine engine; gEngine = &engine;
  engine.init(rate);

  if (!renderPath.empty()) {
    // offline render: load the slot, play, write a WAV at the engine rate
    Pattern* p = new Pattern; if (!store::loadPattern(slot, *p)) p->init();
    Command c; c.type = Command::LoadPattern; c.pat = p; c.a = 1; engine.send(c);
    Command g; g.type = Command::SetGlobal; g.a = G_MASTER_VOLUME; g.b = master; engine.send(g);
    Command pl; pl.type = Command::Play; engine.send(pl);
    int frames = (int)(seconds * rate);
    std::vector<float> buf((size_t)frames * 2);
    for (int done = 0; done < frames; ) { int n = std::min(256, frames - done); engine.render(&buf[(size_t)done * 2], n); done += n; }
    bool ok = writeWav(renderPath, buf, (int)std::lround(rate));
    printf("%s %s (%.1f s of %s at %d Hz)\n", ok ? "wrote" : "FAILED", renderPath.c_str(), seconds, store::slotName(slot).c_str(), (int)rate);
    return ok ? 0 : 1;
  }

  bool offscreen = !shotPath.empty();
  if (offscreen) SDL_SetHint(SDL_HINT_VIDEODRIVER, "offscreen");
  SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");
  SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | (offscreen ? 0 : SDL_INIT_AUDIO)) != 0) { fprintf(stderr, "SDL_Init: %s\n", SDL_GetError()); return 1; }

  Uint32 flags = offscreen ? SDL_WINDOW_HIDDEN : (windowed ? SDL_WINDOW_SHOWN : SDL_WINDOW_FULLSCREEN_DESKTOP);
  SDL_Window* win = SDL_CreateWindow("fliptribe EA-1", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1920, 1080, flags);
  if (!win) { fprintf(stderr, "window: %s\n", SDL_GetError()); return 1; }
  SDL_Renderer* ren = SDL_CreateRenderer(win, -1, offscreen ? SDL_RENDERER_SOFTWARE : (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
  if (!ren) ren = SDL_CreateRenderer(win, -1, 0);
  if (!ren) { fprintf(stderr, "renderer: %s\n", SDL_GetError()); return 1; }
  SDL_RenderSetLogicalSize(ren, 1920, 1080);
  SDL_ShowCursor(windowed ? SDL_ENABLE : SDL_DISABLE);

  Gfx gfx;
  if (!gfx.init(ren, store::assetDir() + "/fonts")) return 1;
  Panel panel(engine, gfx);
  panel.init(slot, master);

  SDL_AudioDeviceID dev = 0;
  if (!offscreen) {
    SDL_AudioSpec want{}, have{};
    want.freq = (int)std::lround(rate); want.format = AUDIO_F32SYS; want.channels = 2; want.samples = 256; want.callback = audioCallback;
    dev = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);
    if (!dev) { fprintf(stderr, "audio: %s\n", SDL_GetError()); return 1; }
    fprintf(stderr, "audio: %d Hz, %d frames per callback, driver %s\n", have.freq, have.samples, SDL_GetCurrentAudioDriver());
    SDL_PauseAudioDevice(dev, 0);
    if (autoplay) { Command c; c.type = Command::Play; engine.send(c); }
  }

  Uint64 last = SDL_GetPerformanceCounter();
  bool running = true;
  while (running) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) panel.handleEvent(ev);
    Uint64 now = SDL_GetPerformanceCounter();
    float dt = (float)(now - last) / (float)SDL_GetPerformanceFrequency(); last = now;
    if (dt > 0.1f) dt = 0.1f;
    if (offscreen) { static float dummy[512 * 2]; for (int i = 0; i < 4; i++) engine.render(dummy, 512); }   // no audio thread: pump the engine so the snapshot reflects the loaded pattern
    panel.update(dt);
    SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
    SDL_RenderClear(ren);
    panel.draw();
    if (offscreen) {
      SDL_Surface* s = SDL_CreateRGBSurfaceWithFormat(0, 1920, 1080, 32, SDL_PIXELFORMAT_ARGB8888);
      SDL_RenderReadPixels(ren, nullptr, SDL_PIXELFORMAT_ARGB8888, s->pixels, s->pitch);
      SDL_SaveBMP(s, shotPath.c_str()); SDL_FreeSurface(s);
      printf("wrote %s\n", shotPath.c_str());
      running = false;
    } else SDL_RenderPresent(ren);
    if (panel.wantsQuit()) running = false;
  }

  if (dev) { SDL_PauseAudioDevice(dev, 1); SDL_CloseAudioDevice(dev); }
  json::Value out = json::Value::obj();
  out["slot"] = json::Value::num(panel.currentSlot());
  out["master"] = json::Value::num(panel.masterVolume());
  store::saveSettings(out);
  panel.shutdown();
  gfx.shutdown();
  SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
  return 0;
}
