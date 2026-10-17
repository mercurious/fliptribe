// fliptribe: a tiny JSON writer and reader, enough for our own pattern files. Not a general parser.
#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <cstdio>

namespace json {

struct Value {
  enum Type { Null, Bool, Number, String, Array, Object } type = Null;
  bool b = false;
  double n = 0;
  std::string s;
  std::vector<Value> a;
  std::map<std::string, Value> o;

  static Value num(double v) { Value x; x.type = Number; x.n = v; return x; }
  static Value str(const std::string& v) { Value x; x.type = String; x.s = v; return x; }
  static Value arr() { Value x; x.type = Array; return x; }
  static Value obj() { Value x; x.type = Object; return x; }
  const Value* get(const char* k) const { if (type != Object) return nullptr; auto it = o.find(k); return it == o.end() ? nullptr : &it->second; }
  double numOr(const char* k, double d) const { const Value* v = get(k); return (v && v->type == Number) ? v->n : d; }
  std::string strOr(const char* k, const std::string& d) const { const Value* v = get(k); return (v && v->type == String) ? v->s : d; }
  Value& operator[](const char* k) { type = Object; return o[k]; }
  void push(Value v) { type = Array; a.push_back(std::move(v)); }
};

inline void writeStr(std::string& out, const std::string& s) {
  out += '"';
  for (char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      default: if ((unsigned char)c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); out += b; } else out += c;
    }
  }
  out += '"';
}

inline void write(std::string& out, const Value& v, int indent = 0) {
  switch (v.type) {
    case Value::Null: out += "null"; break;
    case Value::Bool: out += v.b ? "true" : "false"; break;
    case Value::Number: {
      char b[32];
      if (v.n == (long long)v.n) snprintf(b, sizeof b, "%lld", (long long)v.n); else snprintf(b, sizeof b, "%.6g", v.n);
      out += b; break;
    }
    case Value::String: writeStr(out, v.s); break;
    case Value::Array: {
      // compact arrays of numbers on one line
      bool flat = true; for (auto& e : v.a) if (e.type == Value::Array || e.type == Value::Object) { flat = false; break; }
      out += '[';
      for (size_t i = 0; i < v.a.size(); i++) {
        if (!flat) { out += '\n'; out.append(indent + 2, ' '); }
        write(out, v.a[i], indent + 2);
        if (i + 1 < v.a.size()) out += flat ? "," : ",";
      }
      if (!flat && !v.a.empty()) { out += '\n'; out.append(indent, ' '); }
      out += ']'; break;
    }
    case Value::Object: {
      out += "{";
      size_t i = 0;
      for (auto& kv : v.o) {
        out += '\n'; out.append(indent + 2, ' ');
        writeStr(out, kv.first); out += ": ";
        write(out, kv.second, indent + 2);
        if (++i < v.o.size()) out += ',';
      }
      if (!v.o.empty()) { out += '\n'; out.append(indent, ' '); }
      out += '}'; break;
    }
  }
}

struct Parser {
  const char* p; const char* end;
  Parser(const std::string& s) : p(s.data()), end(s.data() + s.size()) {}
  void ws() { while (p < end && (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t')) p++; }
  bool lit(const char* s) { size_t n = strlen(s); if ((size_t)(end - p) >= n && !strncmp(p, s, n)) { p += n; return true; } return false; }
  bool parse(Value& out) {
    ws(); if (p >= end) return false;
    if (*p == '{') {
      p++; out = Value::obj();
      ws(); if (p < end && *p == '}') { p++; return true; }
      while (p < end) {
        ws(); Value k; if (p >= end || *p != '"' || !parse(k)) return false;
        ws(); if (p >= end || *p != ':') return false; p++;
        Value v; if (!parse(v)) return false;
        out.o[k.s] = std::move(v);
        ws(); if (p < end && *p == ',') { p++; continue; }
        if (p < end && *p == '}') { p++; return true; }
        return false;
      }
      return false;
    }
    if (*p == '[') {
      p++; out = Value::arr();
      ws(); if (p < end && *p == ']') { p++; return true; }
      while (p < end) {
        Value v; if (!parse(v)) return false;
        out.a.push_back(std::move(v));
        ws(); if (p < end && *p == ',') { p++; continue; }
        if (p < end && *p == ']') { p++; return true; }
        return false;
      }
      return false;
    }
    if (*p == '"') {
      p++; out.type = Value::String; out.s.clear();
      while (p < end && *p != '"') {
        if (*p == '\\' && p + 1 < end) {
          p++;
          switch (*p) { case 'n': out.s += '\n'; break; case 't': out.s += '\t'; break; case 'u': { if (end - p >= 5) { out.s += (char)strtol(std::string(p + 1, 4).c_str(), nullptr, 16); p += 4; } break; } default: out.s += *p; }
          p++;
        } else out.s += *p++;
      }
      if (p >= end) return false;
      p++; return true;
    }
    if (lit("true")) { out.type = Value::Bool; out.b = true; return true; }
    if (lit("false")) { out.type = Value::Bool; out.b = false; return true; }
    if (lit("null")) { out.type = Value::Null; return true; }
    char* e = nullptr; double d = strtod(p, &e);
    if (e == p) return false;
    p = e; out = Value::num(d); return true;
  }
};

inline bool parse(const std::string& text, Value& out) { Parser ps(text); return ps.parse(out); }

inline bool readFile(const std::string& path, std::string& out) {
  FILE* f = fopen(path.c_str(), "rb"); if (!f) return false;
  char buf[65536]; size_t n; out.clear();
  while ((n = fread(buf, 1, sizeof buf, f)) > 0) out.append(buf, n);
  fclose(f); return true;
}
inline bool writeFile(const std::string& path, const std::string& data) {
  std::string tmp = path + ".tmp";
  FILE* f = fopen(tmp.c_str(), "wb"); if (!f) return false;
  bool ok = fwrite(data.data(), 1, data.size(), f) == data.size();
  fclose(f);
  if (!ok) { remove(tmp.c_str()); return false; }
  return rename(tmp.c_str(), path.c_str()) == 0;
}

} // namespace json
