// fliptribe: drawing helpers on top of SDL_Renderer. Sprites are rasterised once in software with
// anti-aliasing (knob bodies, LEDs, keys) and blitted per frame; text goes through a texture cache.
#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include <string>
#include <map>
#include <cstdint>

struct Color { uint8_t r, g, b, a = 255; };

class Gfx {
public:
  bool init(SDL_Renderer* r, const std::string& fontDir);
  void shutdown();
  SDL_Renderer* ren = nullptr;

  // text
  enum Font { F_SMALL, F_LABEL, F_BOLD, F_BIG, F_LOGO, F_TINY, F_KNOB, NUM_FONTS };
  void text(const std::string& s, int x, int y, Color c, Font f = F_LABEL, int align = 0); // align 0 left, 1 centre, 2 right
  int textWidth(const std::string& s, Font f = F_LABEL);

  // primitives
  void rect(int x, int y, int w, int h, Color c);
  void frame(int x, int y, int w, int h, Color c, int t = 1);
  void roundRect(int x, int y, int w, int h, int rad, Color c);
  void circle(int cx, int cy, int r, Color c);
  void ring(int cx, int cy, int r, int t, Color c);
  void line(int x0, int y0, int x1, int y1, Color c, int t = 1);
  void arc(int cx, int cy, int r, int t, float a0, float a1, Color c, int segs = 48);

  // sprites
  SDL_Texture* knobBody(int radius);        // brushed black knob with grey skirt
  SDL_Texture* knobPointer(int radius);     // white pointer line, rotate with RenderCopyEx
  SDL_Texture* led(int radius, Color on);   // glowing LED
  SDL_Texture* ledOff(int radius);
  SDL_Texture* glow(int radius, Color c);   // soft halo

  void blit(SDL_Texture* t, int cx, int cy, double angleDeg = 0.0, uint8_t alpha = 255);
  void panelBackground(int w, int h);       // brushed silver panel

  // 7 segment digit, size h, at x,y (top-left). ch in "0-9A-Z-. " plus '~' for blank
  void sevenSeg(char ch, int x, int y, int h, Color on, Color off, bool dot = false);

private:
  TTF_Font* fonts[NUM_FONTS] = {};
  struct Key { std::string s; int f; uint32_t c; bool operator<(const Key& o) const { return c != o.c ? c < o.c : (f != o.f ? f < o.f : s < o.s); } };
  struct Entry { SDL_Texture* t; int w, h; };
  std::map<Key, Entry> cache;
  std::map<std::string, SDL_Texture*> sprites;
  SDL_Texture* fromSurface(SDL_Surface* s);
};
