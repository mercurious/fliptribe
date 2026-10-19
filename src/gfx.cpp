#include "gfx.h"
#include <cmath>
#include <vector>
#include <cstdio>

static inline uint32_t pack(Color c) { return (c.r << 24) | (c.g << 16) | (c.b << 8) | c.a; }

bool Gfx::init(SDL_Renderer* r, const std::string& fontDir) {
  ren = r;
  if (TTF_Init() != 0) { fprintf(stderr, "TTF_Init: %s\n", TTF_GetError()); return false; }
  std::string reg = fontDir + "/LiberationSans-Regular.ttf", bold = fontDir + "/LiberationSans-Bold.ttf";
  fonts[F_SMALL] = TTF_OpenFont(reg.c_str(), 17);
  fonts[F_LABEL] = TTF_OpenFont(bold.c_str(), 19);
  fonts[F_BOLD] = TTF_OpenFont(bold.c_str(), 26);
  fonts[F_BIG] = TTF_OpenFont(bold.c_str(), 40);
  fonts[F_LOGO] = TTF_OpenFont(bold.c_str(), 54);
  fonts[F_TINY] = TTF_OpenFont(reg.c_str(), 13);
  fonts[F_KNOB] = TTF_OpenFont(bold.c_str(), 16);
  for (int i = 0; i < NUM_FONTS; i++) if (!fonts[i]) { fprintf(stderr, "font load failed in %s: %s\n", fontDir.c_str(), TTF_GetError()); return false; }
  SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
  return true;
}

void Gfx::shutdown() {
  for (auto& kv : cache) SDL_DestroyTexture(kv.second.t);
  for (auto& kv : sprites) SDL_DestroyTexture(kv.second);
  for (int i = 0; i < NUM_FONTS; i++) if (fonts[i]) TTF_CloseFont(fonts[i]);
  TTF_Quit();
}

SDL_Texture* Gfx::fromSurface(SDL_Surface* s) {
  SDL_Texture* t = SDL_CreateTextureFromSurface(ren, s);
  SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
  SDL_FreeSurface(s);
  return t;
}

void Gfx::text(const std::string& s, int x, int y, Color c, Font f, int align) {
  if (s.empty()) return;
  Key k{s, f, pack(c)};
  auto it = cache.find(k);
  if (it == cache.end()) {
    SDL_Color sc{c.r, c.g, c.b, c.a};
    SDL_Surface* surf = TTF_RenderUTF8_Blended(fonts[f], s.c_str(), sc);
    if (!surf) return;
    Entry e{nullptr, surf->w, surf->h};
    e.t = fromSurface(surf);
    if (cache.size() > 4000) { for (auto& kv : cache) SDL_DestroyTexture(kv.second.t); cache.clear(); }
    it = cache.emplace(k, e).first;
  }
  const Entry& e = it->second;
  int dx = align == 1 ? x - e.w / 2 : (align == 2 ? x - e.w : x);
  SDL_Rect d{dx, y, e.w, e.h};
  SDL_RenderCopy(ren, e.t, nullptr, &d);
}

int Gfx::textWidth(const std::string& s, Font f) { int w = 0, h = 0; TTF_SizeUTF8(fonts[f], s.c_str(), &w, &h); return w; }

void Gfx::rect(int x, int y, int w, int h, Color c) { SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a); SDL_Rect r{x, y, w, h}; SDL_RenderFillRect(ren, &r); }
void Gfx::frame(int x, int y, int w, int h, Color c, int t) { for (int i = 0; i < t; i++) { SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a); SDL_Rect r{x + i, y + i, w - 2 * i, h - 2 * i}; SDL_RenderDrawRect(ren, &r); } }

void Gfx::roundRect(int x, int y, int w, int h, int rad, Color c) {
  SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
  if (rad * 2 > w) rad = w / 2;
  if (rad * 2 > h) rad = h / 2;
  SDL_Rect mid{x, y + rad, w, h - 2 * rad}; SDL_RenderFillRect(ren, &mid);
  for (int i = 0; i < rad; i++) {
    float dy = rad - i - 0.5f;
    int dx = (int)std::sqrt(std::max(0.0f, (float)rad * rad - dy * dy));
    SDL_Rect a{x + rad - dx, y + i, w - 2 * (rad - dx), 1}; SDL_RenderFillRect(ren, &a);
    SDL_Rect b{x + rad - dx, y + h - 1 - i, w - 2 * (rad - dx), 1}; SDL_RenderFillRect(ren, &b);
  }
}

void Gfx::circle(int cx, int cy, int r, Color c) {
  SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
  for (int dy = -r; dy <= r; dy++) {
    int dx = (int)std::sqrt((float)(r * r - dy * dy));
    SDL_Rect row{cx - dx, cy + dy, 2 * dx + 1, 1}; SDL_RenderFillRect(ren, &row);
  }
}

void Gfx::ring(int cx, int cy, int r, int t, Color c) { arc(cx, cy, r, t, 0, 2 * 3.14159265f, c, 96); }

void Gfx::line(int x0, int y0, int x1, int y1, Color c, int t) {
  SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
  if (t <= 1) { SDL_RenderDrawLine(ren, x0, y0, x1, y1); return; }
  float dx = (float)(x1 - x0), dy = (float)(y1 - y0), len = std::sqrt(dx * dx + dy * dy);
  if (len < 0.5f) return;
  float nx = -dy / len, ny = dx / len;
  for (int i = 0; i < t; i++) {
    float o = i - (t - 1) / 2.0f;
    SDL_RenderDrawLine(ren, (int)std::lround(x0 + nx * o), (int)std::lround(y0 + ny * o), (int)std::lround(x1 + nx * o), (int)std::lround(y1 + ny * o));
  }
}

void Gfx::arc(int cx, int cy, int r, int t, float a0, float a1, Color c, int segs) {
  if (a1 < a0) std::swap(a0, a1);
  int n = std::max(2, (int)(segs * (a1 - a0) / (2 * 3.14159265f)) + 1);
  float px = cx + std::cos(a0) * r, py = cy + std::sin(a0) * r;
  for (int i = 1; i <= n; i++) {
    float a = a0 + (a1 - a0) * i / n;
    float x = cx + std::cos(a) * r, y = cy + std::sin(a) * r;
    line((int)std::lround(px), (int)std::lround(py), (int)std::lround(x), (int)std::lround(y), c, t);
    px = x; py = y;
  }
}

// --- software rasterised sprites ---
static SDL_Surface* newSurface(int w, int h) { return SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32); }
static inline void put(SDL_Surface* s, int x, int y, float r, float g, float b, float a) {
  if (x < 0 || y < 0 || x >= s->w || y >= s->h || a <= 0.0f) return;
  uint8_t* p = (uint8_t*)s->pixels + y * s->pitch + x * 4;
  float da = p[3] / 255.0f;
  float oa = a + da * (1 - a);
  if (oa <= 0) return;
  p[0] = (uint8_t)std::min(255.0f, (r * a + p[0] * da * (1 - a)) / oa);
  p[1] = (uint8_t)std::min(255.0f, (g * a + p[1] * da * (1 - a)) / oa);
  p[2] = (uint8_t)std::min(255.0f, (b * a + p[2] * da * (1 - a)) / oa);
  p[3] = (uint8_t)std::min(255.0f, oa * 255.0f);
}
static inline float cover(float d, float edge) { // 1 inside, anti-aliased edge of 1px
  return std::max(0.0f, std::min(1.0f, edge - d + 0.5f));
}

SDL_Texture* Gfx::knobBody(int radius) {
  std::string key = "knob" + std::to_string(radius);
  auto it = sprites.find(key); if (it != sprites.end()) return it->second;
  int R = radius + 6, W = 2 * R + 2;
  SDL_Surface* s = newSurface(W, W);
  float cx = W / 2.0f, cy = W / 2.0f;
  for (int y = 0; y < W; y++) for (int x = 0; x < W; x++) {
    float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = std::sqrt(dx * dx + dy * dy);
    // drop shadow
    float ds = std::sqrt((dx - 2) * (dx - 2) + (dy - 4) * (dy - 4));
    float sh = cover(ds, radius + 4) * 0.45f * (1.0f - std::max(0.0f, (ds - radius) / 5.0f));
    if (sh > 0) put(s, x, y, 0, 0, 0, sh);
    // skirt: light grey ring with a tick scale look
    float skirt = cover(d, radius + 3) - cover(d, radius - 1);
    if (skirt > 0) { float ang = std::atan2(dy, dx); float tick = std::fmod(ang * 12.0f / 3.14159f + 100.0f, 1.0f) < 0.12f ? 0.65f : 1.0f; float sg = 150 * tick; put(s, x, y, sg, sg, sg + 4, skirt); }
    // cap: dark rubbery black with a highlight top-left and a lighter rim
    float cap = cover(d, radius - 1);
    if (cap > 0) {
      float ang = std::atan2(dy, dx);
      float rimLight = 0.5f + 0.5f * std::cos(ang + 2.3f);      // top-left bright
      float rim = std::max(0.0f, 1.0f - (radius - 1 - d) / 3.0f);
      float base = 34.0f + rimLight * 20.0f * rim + (1.0f - d / radius) * 10.0f;
      // subtle radial brushing
      base += 3.0f * std::sin(ang * 60.0f);
      put(s, x, y, base, base, base + 3, cap);
    }
    // inner ring groove
    float groove = cover(d, radius * 0.72f) - cover(d, radius * 0.66f);
    if (groove > 0) put(s, x, y, 20, 20, 22, groove * 0.8f);
  }
  SDL_Texture* t = fromSurface(s); sprites[key] = t; return t;
}

SDL_Texture* Gfx::knobPointer(int radius) {
  std::string key = "ptr" + std::to_string(radius);
  auto it = sprites.find(key); if (it != sprites.end()) return it->second;
  int R = radius + 6, W = 2 * R + 2;
  SDL_Surface* s = newSurface(W, W);
  float cx = W / 2.0f, cy = W / 2.0f;
  // pointer along -Y (pointing up) from 0.25r to 0.95r, width 4px, rounded
  for (int y = 0; y < W; y++) for (int x = 0; x < W; x++) {
    float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
    float py = -dy;
    float a = 0.0f;
    if (py > radius * 0.28f && py < radius * 0.95f) a = cover(std::fabs(dx), 2.4f);
    else if (py >= radius * 0.95f) a = cover(std::sqrt(dx * dx + (py - radius * 0.95f) * (py - radius * 0.95f)), 2.4f);
    else if (py <= radius * 0.28f) a = cover(std::sqrt(dx * dx + (py - radius * 0.28f) * (py - radius * 0.28f)), 2.4f);
    if (a > 0) put(s, x, y, 245, 245, 240, a);
  }
  SDL_Texture* t = fromSurface(s); sprites[key] = t; return t;
}

SDL_Texture* Gfx::led(int radius, Color on) {
  char kb[64]; snprintf(kb, sizeof kb, "led%d_%08x", radius, pack(on));
  auto it = sprites.find(kb); if (it != sprites.end()) return it->second;
  int R = radius * 3, W = 2 * R + 2;
  SDL_Surface* s = newSurface(W, W);
  float cx = W / 2.0f, cy = W / 2.0f;
  for (int y = 0; y < W; y++) for (int x = 0; x < W; x++) {
    float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = std::sqrt(dx * dx + dy * dy);
    float halo = std::max(0.0f, 1.0f - d / R); halo = halo * halo * 0.55f;
    if (halo > 0) put(s, x, y, on.r, on.g, on.b, halo);
    float body = cover(d, radius);
    if (body > 0) {
      float hot = std::max(0.0f, 1.0f - d / radius);
      put(s, x, y, std::min(255.0f, on.r + 120 * hot), std::min(255.0f, on.g + 120 * hot), std::min(255.0f, on.b + 120 * hot), body);
    }
  }
  SDL_Texture* t = fromSurface(s); sprites[kb] = t; return t;
}

SDL_Texture* Gfx::ledOff(int radius) {
  std::string key = "ledoff" + std::to_string(radius);
  auto it = sprites.find(key); if (it != sprites.end()) return it->second;
  int W = 2 * radius + 4;
  SDL_Surface* s = newSurface(W, W);
  float cx = W / 2.0f, cy = W / 2.0f;
  for (int y = 0; y < W; y++) for (int x = 0; x < W; x++) {
    float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = std::sqrt(dx * dx + dy * dy);
    float body = cover(d, radius);
    if (body > 0) { float hl = std::max(0.0f, 1.0f - std::sqrt((dx + radius * 0.3f) * (dx + radius * 0.3f) + (dy + radius * 0.3f) * (dy + radius * 0.3f)) / radius); put(s, x, y, 90 + 40 * hl, 30 + 20 * hl, 30 + 20 * hl, body); }
    float rim = cover(d, radius + 1) - cover(d, radius - 0.5f);
    if (rim > 0) put(s, x, y, 40, 40, 40, rim * 0.6f);
  }
  SDL_Texture* t = fromSurface(s); sprites[key] = t; return t;
}

SDL_Texture* Gfx::glow(int radius, Color c) {
  char kb[64]; snprintf(kb, sizeof kb, "glow%d_%08x", radius, pack(c));
  auto it = sprites.find(kb); if (it != sprites.end()) return it->second;
  int W = 2 * radius + 2;
  SDL_Surface* s = newSurface(W, W);
  float cx = W / 2.0f, cy = W / 2.0f;
  for (int y = 0; y < W; y++) for (int x = 0; x < W; x++) {
    float dx = x + 0.5f - cx, dy = y + 0.5f - cy, d = std::sqrt(dx * dx + dy * dy);
    float a = std::max(0.0f, 1.0f - d / radius); a = a * a;
    if (a > 0) put(s, x, y, c.r, c.g, c.b, a * c.a / 255.0f);
  }
  SDL_Texture* t = fromSurface(s); sprites[kb] = t; return t;
}

void Gfx::blit(SDL_Texture* t, int cx, int cy, double angleDeg, uint8_t alpha) {
  int w, h; SDL_QueryTexture(t, nullptr, nullptr, &w, &h);
  SDL_Rect d{cx - w / 2, cy - h / 2, w, h};
  SDL_SetTextureAlphaMod(t, alpha);
  SDL_RenderCopyEx(ren, t, nullptr, &d, angleDeg, nullptr, SDL_FLIP_NONE);
}

void Gfx::panelBackground(int w, int h) {
  // brushed aluminium: horizontal grain with a soft vertical gradient
  uint32_t seed = 12345;
  for (int y = 0; y < h; y++) {
    seed = seed * 1664525u + 1013904223u;
    float grain = ((seed >> 8) & 0xff) / 255.0f - 0.5f;
    float grad = 1.0f - 0.12f * (float)y / h;
    int base = (int)((196 + grain * 9.0f) * grad);
    SDL_SetRenderDrawColor(ren, base, base, base + 3, 255);
    SDL_RenderDrawLine(ren, 0, y, w, y);
  }
  // second finer grain pass for texture
  for (int y = 0; y < h; y += 2) {
    seed = seed * 1664525u + 1013904223u;
    int a = (seed >> 9) & 0x1f;
    SDL_SetRenderDrawColor(ren, 255, 255, 255, a / 2);
    int x0 = (seed >> 14) % w;
    SDL_RenderDrawLine(ren, x0, y, std::min(w, x0 + 400 + (int)((seed >> 20) % 900)), y);
  }
}

// 7-segment: segments a b c d e f g = bits 0..6
static uint8_t segFor(char ch) {
  switch (ch) {
    case '0': return 0x3F; case '1': return 0x06; case '2': return 0x5B; case '3': return 0x4F; case '4': return 0x66;
    case '5': return 0x6D; case '6': return 0x7D; case '7': return 0x07; case '8': return 0x7F; case '9': return 0x6F;
    case 'A': return 0x77; case 'b': case 'B': return 0x7C; case 'C': return 0x39; case 'c': return 0x58; case 'd': case 'D': return 0x5E;
    case 'E': return 0x79; case 'F': return 0x71; case 'G': return 0x3D; case 'H': return 0x76; case 'h': return 0x74; case 'I': return 0x30;
    case 'J': return 0x1E; case 'L': return 0x38; case 'n': case 'N': return 0x54; case 'o': case 'O': return 0x5C; case 'P': return 0x73;
    case 'q': case 'Q': return 0x67; case 'r': case 'R': return 0x50; case 'S': return 0x6D; case 't': case 'T': return 0x78; case 'U': return 0x3E;
    case 'u': return 0x1C; case 'y': case 'Y': return 0x6E; case '-': return 0x40; case '_': return 0x08; case '=': return 0x48;
    case 'K': return 0x75; case 'M': return 0x37; case 'W': return 0x7E; case 'V': return 0x3E; case 'X': return 0x76; case 'Z': return 0x5B;
    default: return 0x00;
  }
}

void Gfx::sevenSeg(char ch, int x, int y, int h, Color on, Color off, bool dot) {
  uint8_t m = segFor(ch);
  int w = h * 11 / 20, t = std::max(3, h / 9), g = 2;
  int xm = x + t, xr = x + w - t;
  int ym = y + h / 2;
  // slanted look: shift top by 6% of h
  int sl = h / 16;
  auto seg = [&](bool onb, int x0, int y0, int x1, int y1, bool horiz) {
    Color c = onb ? on : off;
    if (horiz) {
      roundRect(x0 + g, y0 - t / 2, x1 - x0 - 2 * g, t, t / 2, c);
    } else {
      roundRect(x0 - t / 2, y0 + g, t, y1 - y0 - 2 * g, t / 2, c);
    }
    if (onb) { Color gl = on; gl.a = 70; roundRect(horiz ? x0 + g - 2 : x0 - t / 2 - 2, horiz ? y0 - t / 2 - 2 : y0 + g - 2, horiz ? x1 - x0 - 2 * g + 4 : t + 4, horiz ? t + 4 : y1 - y0 - 2 * g + 4, t / 2 + 2, gl); }
  };
  seg(m & 0x01, xm + sl, y + t / 2, xr + sl, y + t / 2, true);              // a top
  seg(m & 0x02, xr + sl / 2, y + t / 2, xr + sl / 2, ym, false);            // b top-right
  seg(m & 0x04, xr - sl / 2, ym, xr - sl / 2, y + h - t / 2, false);        // c bottom-right
  seg(m & 0x08, xm - sl, y + h - t / 2, xr - sl, y + h - t / 2, true);      // d bottom
  seg(m & 0x10, xm - sl / 2, ym, xm - sl / 2, y + h - t / 2, false);        // e bottom-left
  seg(m & 0x20, xm + sl / 2, y + t / 2, xm + sl / 2, ym, false);            // f top-left
  seg(m & 0x40, xm, ym, xr, ym, true);                                      // g middle
  if (dot) circle(x + w + t, y + h - t / 2, t / 2, on);
}
