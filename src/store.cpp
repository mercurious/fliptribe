#include "store.h"
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <climits>
#include <cstring>

namespace store {

static void mkdirs(const std::string& path) {
  std::string cur;
  for (size_t i = 0; i < path.size(); i++) {
    cur += path[i];
    if (path[i] == '/' && i > 0) mkdir(cur.c_str(), 0755);
  }
  mkdir(path.c_str(), 0755);
}

std::string dataDir() {
  static std::string d;
  if (d.empty()) {
    const char* e = getenv("FLIPTRIBE_DATA");
    if (e && *e) d = e; else { const char* h = getenv("HOME"); d = std::string(h ? h : ".") + "/.config/fliptribe"; }
    mkdirs(d + "/patterns");
  }
  return d;
}

static std::string exeDir() {
  char buf[PATH_MAX]; ssize_t n = readlink("/proc/self/exe", buf, sizeof buf - 1);
  if (n <= 0) return ".";
  buf[n] = 0; std::string s(buf); size_t k = s.rfind('/'); return k == std::string::npos ? "." : s.substr(0, k);
}

std::string assetDir() {
  static std::string d;
  if (d.empty()) {
    const char* e = getenv("FLIPTRIBE_ASSETS");
    if (e && *e) d = e; else {
      std::string x = exeDir();
      struct stat st;
      if (stat((x + "/assets").c_str(), &st) == 0) d = x + "/assets";
      else if (stat((x + "/../assets").c_str(), &st) == 0) d = x + "/../assets";
      else d = "assets";
    }
  }
  return d;
}

std::string patternDir() {
  const char* e = getenv("FLIPTRIBE_PATTERNS");
  if (e && *e) return e;
  return assetDir() + "/../patterns";
}

std::string slotName(int slot) {
  char b[8]; snprintf(b, sizeof b, "%c%02d", 'A' + (slot / 64), (slot % 64) + 1); return b;
}

json::Value toJson(const Pattern& p) {
  json::Value v = json::Value::obj();
  v["format"] = json::Value::str("fliptribe-ea1-pattern");
  v["version"] = json::Value::num(1);
  v["name"] = json::Value::str(p.name);
  v["tempo"] = json::Value::num(p.tempo10 / 10.0);
  json::Value g = json::Value::obj();
  for (int i = 0; i < NUM_GLOBAL_PARAMS; i++) g[kGlobalParams[i].id] = json::Value::num(p.g[i]);
  v["global"] = g;
  json::Value parts = json::Value::arr();
  for (int pi = 0; pi < kParts; pi++) {
    const PartData& pd = p.part[pi];
    json::Value pv = json::Value::obj();
    json::Value pp = json::Value::obj();
    for (int i = 0; i < NUM_PART_PARAMS; i++) pp[kPartParams[i].id] = json::Value::num(pd.p[i]);
    pv["params"] = pp;
    pv["mute"] = json::Value::num(pd.mute);
    json::Value on = json::Value::arr(), note = json::Value::arr(), gate = json::Value::arr();
    for (int s = 0; s < kMaxSteps; s++) { on.push(json::Value::num(pd.step[s].on)); note.push(json::Value::num(pd.step[s].note)); gate.push(json::Value::num((pd.step[s].gate4 + 1) / 4.0)); }
    pv["on"] = on; pv["note"] = note; pv["gate"] = gate;   // gate in steps (0.25 .. 64.0)
    json::Value m = json::Value::obj();
    for (int i = 0; i < NUM_PART_PARAMS; i++) if (pd.motion[i].active) { json::Value a = json::Value::arr(); for (int s = 0; s < kMaxSteps; s++) a.push(json::Value::num(pd.motion[i].v[s])); m[kPartParams[i].id] = a; }
    pv["motion"] = m;
    parts.push(pv);
  }
  v["parts"] = parts;
  return v;
}

static int findPart(const char* id) { for (int i = 0; i < NUM_PART_PARAMS; i++) if (!strcmp(kPartParams[i].id, id)) return i; return -1; }
static int findGlobal(const char* id) { for (int i = 0; i < NUM_GLOBAL_PARAMS; i++) if (!strcmp(kGlobalParams[i].id, id)) return i; return -1; }

bool fromJson(const json::Value& v, Pattern& p) {
  if (v.type != json::Value::Object) return false;
  p.init();
  std::string name = v.strOr("name", "INIT");
  strncpy(p.name, name.c_str(), 16); p.name[16] = 0;
  p.tempo10 = (uint16_t)std::max(200.0, std::min(3000.0, v.numOr("tempo", 130.0) * 10.0 + 0.5));
  if (const json::Value* g = v.get("global")) for (auto& kv : g->o) { int i = findGlobal(kv.first.c_str()); if (i >= 0 && kv.second.type == json::Value::Number) p.g[i] = (uint8_t)std::max(0.0, std::min((double)kGlobalParams[i].max, kv.second.n)); }
  if (const json::Value* parts = v.get("parts")) {
    for (size_t pi = 0; pi < parts->a.size() && pi < (size_t)kParts; pi++) {
      const json::Value& pv = parts->a[pi]; PartData& pd = p.part[pi];
      if (const json::Value* pp = pv.get("params")) for (auto& kv : pp->o) { int i = findPart(kv.first.c_str()); if (i >= 0 && kv.second.type == json::Value::Number) pd.p[i] = (uint8_t)std::max(0.0, std::min((double)kPartParams[i].max, kv.second.n)); }
      pd.mute = (uint8_t)pv.numOr("mute", 0);
      auto arr = [&](const char* k, auto fn) { if (const json::Value* a = pv.get(k)) for (size_t s = 0; s < a->a.size() && s < kMaxSteps; s++) fn(pd.step[s], a->a[s].n); };
      arr("on", [](Step& s, double x) { s.on = x != 0 ? 1 : 0; });
      arr("note", [](Step& s, double x) { s.note = (uint8_t)std::max(0.0, std::min(127.0, x)); });
      arr("gate", [](Step& s, double x) { s.gate4 = (uint8_t)std::max(0.0, std::min(255.0, std::lround(x * 4.0) - 1.0)); });
      if (const json::Value* m = pv.get("motion")) for (auto& kv : m->o) { int i = findPart(kv.first.c_str()); if (i >= 0 && kv.second.type == json::Value::Array) { pd.motion[i].active = 1; for (size_t s = 0; s < kv.second.a.size() && s < kMaxSteps; s++) pd.motion[i].v[s] = (uint8_t)kv.second.a[s].n; } }
    }
  }
  return true;
}

static std::string userPath(int slot) { return dataDir() + "/patterns/" + slotName(slot) + ".json"; }
static std::string shippedPath(int slot) { return patternDir() + "/" + slotName(slot) + ".json"; }

bool savePattern(int slot, const Pattern& p) {
  std::string out; json::write(out, toJson(p)); out += '\n';
  return json::writeFile(userPath(slot), out);
}

bool loadPattern(int slot, Pattern& p) {
  std::string text; json::Value v;
  if (json::readFile(userPath(slot), text) && json::parse(text, v) && fromJson(v, p)) return true;
  if (json::readFile(shippedPath(slot), text) && json::parse(text, v) && fromJson(v, p)) return true;
  return false;
}

void seedFromShipped() {
  for (int s = 0; s < kNumPatterns; s++) {
    struct stat st;
    if (stat(userPath(s).c_str(), &st) == 0) continue;
    std::string text;
    if (json::readFile(shippedPath(s), text)) json::writeFile(userPath(s), text);
  }
}

bool saveSettings(const json::Value& v) { std::string out; json::write(out, v); return json::writeFile(dataDir() + "/settings.json", out); }
bool loadSettings(json::Value& v) { std::string t; return json::readFile(dataDir() + "/settings.json", t) && json::parse(t, v); }

} // namespace store
