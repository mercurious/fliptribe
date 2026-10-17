#!/usr/bin/env python3
"""Writes the shipped demo patterns (patterns/A01.json ...). All original material written in the EA-1 idiom;
nothing here is transcribed from Korg's factory set. Run from the repo root: python3 tools/make_patterns.py"""
import json, math, os, re

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "patterns")
NOTE = {"c": 0, "d": 2, "e": 4, "f": 5, "g": 7, "a": 9, "b": 11}

def midi(tok):
    m = re.fullmatch(r"([a-g])([#b]?)(-?\d)", tok)
    n = NOTE[m.group(1)] + (1 if m.group(2) == "#" else -1 if m.group(2) == "b" else 0)
    return n + 12 * (int(m.group(3)) + 1)          # C4 = 60, as the EA-1 counts

def seq(text, gate=0.75):
    """'.' rest, 'c2' note, suffixes: '~' tie into next step (1.25), '_' long (2.0), '!' short (0.5), '-' half-step (0.5)."""
    on, note, gates = [], [], []
    for tok in text.split():
        if tok == ".":
            on.append(0); note.append(60); gates.append(1.0); continue
        g = gate
        if tok.endswith("~"): g = 1.25; tok = tok[:-1]
        elif tok.endswith("_"): g = 2.0; tok = tok[:-1]
        elif tok.endswith("!"): g = 0.5; tok = tok[:-1]
        on.append(1); note.append(midi(tok)); gates.append(g)
    return on, note, gates

def pad(a, fill):
    a = list(a)
    while len(a) < 64: a += a[: 64 - len(a)] if a else [fill]
    return a[:64]

def pct(x): return max(0, min(127, round(x * 1.27)))

DEFAULT = dict(portamento=0, osc_balance=0, osc2_pitch=25, osc1_wave=0, osc2_wave=0, osc_mod=0, distortion=0, level=pct(85),
               cutoff=pct(55), resonance=pct(30), eg_int=64 + 30, decay=pct(35), delay_depth=0, delay_time=72, chorus_depth=0, chorus_time=30, motion_seq=1)

def part(line, gate=0.75, motion=None, **params):
    on, note, gates = seq(line, gate)
    p = dict(DEFAULT); p.update(params)
    d = {"params": p, "mute": 0, "on": pad(on, 0), "note": pad(note, 60), "gate": pad(gates, 1.0), "motion": {}}
    if motion:
        for k, v in motion.items(): d["motion"][k] = pad(v, v[0])
    return d

def pattern(slot, name, tempo, p1, p2, length=1, beat=0, swing=0):
    data = {"format": "fliptribe-ea1-pattern", "version": 1, "name": name, "tempo": tempo,
            "global": {"swing": swing, "beat": beat, "length": length - 1, "master": 100}, "parts": [p1, p2]}
    with open(os.path.join(OUT, slot + ".json"), "w") as f:
        json.dump(data, f, indent=1); f.write("\n")

def sweep(lo, hi, n=16, shape="tri"):
    out = []
    for i in range(n):
        t = i / n
        if shape == "tri": t = 1 - abs(2 * t - 1)
        elif shape == "down": t = 1 - t
        elif shape == "sine": t = 0.5 - 0.5 * math.cos(2 * math.pi * t)
        out.append(round(lo + (hi - lo) * t))
    return out

# eg_int is bipolar: 64 = 0, 127 = +100
def egi(x): return max(0, min(127, 64 + round(x * 0.63)))
# osc2 pitch: 0..127 = -12..+48 semitones
def o2(semis): return max(0, min(127, round((semis + 12) * 127 / 60)))

os.makedirs(OUT, exist_ok=True)

# A01 classic acid line: ties + portamento give the slides, EG int and resonance do the squelch
pattern("A01", "ACID LINE 1", 130,
  part("c2 . c2 c3~ c2 . eb2 c2 . c2~ g2 . c2 . bb2! c3", gate=0.6, portamento=pct(18), cutoff=pct(38), resonance=pct(78), eg_int=egi(70), decay=pct(32),
       delay_depth=pct(22), delay_time=24, motion={"cutoff": sweep(pct(28), pct(62), 16, "sine")}),
  part(". . . c1 . . . . . . . c1 . . c1 .", gate=0.5, osc1_wave=1, osc2_wave=2, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(30), resonance=pct(20), eg_int=egi(40), decay=pct(40), level=pct(90)))

# A02 techno bass under a hard-synced lead with a pitch sweep recorded as a motion sequence
pattern("A02", "TEK BASS SYNC", 135,
  part("c1 . c1 . c1 . c1 c1 c1 . c1 . c1 . c1 .", gate=0.55, osc1_wave=0, osc2_wave=1, osc_balance=pct(45), osc2_pitch=o2(0.15), cutoff=pct(45), resonance=pct(35), eg_int=egi(55), decay=pct(28), distortion=0),
  part(". . c3 . . . c3 . . . eb3 . . g3 . .", gate=1.25, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(20), decay=pct(60), level=pct(70),
       delay_depth=pct(35), delay_time=40, motion={"osc2_pitch": sweep(o2(0), o2(19), 16, "tri")}))

# A03 decimator bass with ring-modulated blips echoing in 1/3 steps
pattern("A03", "DECI BASS", 128,
  part("c1~ c1 . c1 . . c1 . bb0~ bb0 . c1 . . eb1 .", gate=0.75, osc_mod=3, osc_balance=pct(80), osc2_pitch=o2(31), cutoff=pct(42), resonance=pct(40), eg_int=egi(45), decay=pct(38), level=pct(90)),
  part(". . . . c4 . . . . . . c5 . g4 . .", gate=0.3, osc_mod=1, osc_balance=pct(90), osc2_pitch=o2(7), cutoff=pct(65), resonance=pct(55), eg_int=egi(60), decay=pct(18), level=pct(60), delay_depth=pct(55), delay_time=8))

# A04 deep house: triangle sub with swing, chorused mono "chord" arpeggio on part 2
pattern("A04", "DEEP HOUSE", 123,
  part("f1 . . f1 . . f1 . ab1 . . f1 . . eb1 .", gate=0.7, osc1_wave=2, osc2_wave=2, osc_balance=pct(30), osc2_pitch=o2(12), cutoff=pct(32), resonance=pct(10), eg_int=egi(25), decay=pct(45), level=pct(95)),
  part(". f3 ab3 c4 . f3 ab3 c4 . eb3 g3 bb3 . eb3 g3 bb3", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0.08), cutoff=pct(48), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(55), chorus_depth=pct(45), chorus_time=pct(22), delay_depth=pct(30), delay_time=56),
  swing=8)

# A05 electro at 32nd scale: a 16-key bar is two beats, so the pattern is 2 bars long
pattern("A05", "ELECTRO 32", 112,
  part("c1 . . c1 . . c1 . . . c1 . eb1 . . .  c1 . . c1 . . c1 . . . g1 . f1 . eb1 .", gate=0.6, osc1_wave=1, osc2_wave=0, osc_balance=pct(35), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(45), eg_int=egi(50), decay=pct(30), distortion=1, level=pct(60)),
  part(". . . . c5 . . . . . . . c6 . . .  . . . . c5 . . . . . c4 . . . c6 .", gate=0.25, cutoff=pct(60), resonance=pct(70), eg_int=egi(100), decay=pct(12), level=pct(55), delay_depth=pct(40), delay_time=16),
  length=2, beat=1)

# A06 triplet scale: 12 steps per bar, a lilting bass and flanged long notes
pattern("A06", "TRIPLET ROLL", 118,
  part("a1 . a1 c2 . a1 e2 . a1 g2 . e2", gate=0.7, osc1_wave=0, osc2_wave=2, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(44), resonance=pct(28), eg_int=egi(40), decay=pct(35)),
  part("a3_ . . . . . g3_ . . . . .", gate=3.0, osc1_wave=0, osc2_wave=0, osc_balance=pct(50), osc2_pitch=o2(0.12), cutoff=pct(52), resonance=pct(15), eg_int=egi(-30), decay=pct(70), level=pct(50), chorus_depth=pct(85), chorus_time=pct(18)),
  beat=2)

# A07 dub stepper: long bass gates, 3/4-step echoes on the skank
pattern("A07", "DUB STEPPER", 140,
  part("g1_ . . . . . g1 . bb1_ . . . f1 . . .", gate=1.0, osc1_wave=2, osc2_wave=0, osc_balance=pct(25), osc2_pitch=o2(0), cutoff=pct(30), resonance=pct(20), eg_int=egi(30), decay=pct(55), level=pct(95)),
  part(". . g3! . . . bb3! . . . g3! . . . f3! .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(7), cutoff=pct(58), resonance=pct(40), eg_int=egi(40), decay=pct(25), level=pct(55), delay_depth=pct(65), delay_time=40))

# A08 sync lead sweep over two bars with a walking bass
pattern("A08", "SYNC SWEEP", 126,
  part("e1 . e1 . g1 . e1 . a1 . e1 . b1 . d2 .  e1 . e1 . g1 . e1 . a1 . g1 . e1 . d1 .", gate=0.6, osc_balance=pct(35), osc2_pitch=o2(12), cutoff=pct(40), resonance=pct(35), eg_int=egi(45), decay=pct(30)),
  part("e3~ e3~ e3~ e3 . . . . e3~ e3~ e3~ e3 . . . .  g3~ g3~ g3~ g3 . . . . a3~ a3~ a3~ a3 . . . .", gate=1.25, osc_mod=2, osc_balance=pct(100), cutoff=pct(75), resonance=pct(20), eg_int=egi(10), decay=pct(50), level=pct(65), delay_depth=pct(30), delay_time=56,
       motion={"osc2_pitch": sweep(o2(0), o2(24), 32, "tri")}),
  length=2)

# A09 drone: resonance at the edge of self-oscillation, chorus LFO pushed to audio rate on a motion lane
pattern("A09", "DRONE FLANGE", 90,
  part("c2_ . . . . . . . c2_ . . . . . . .", gate=8.0, osc1_wave=2, osc2_wave=2, osc_balance=pct(50), osc2_pitch=o2(7.05), cutoff=pct(34), resonance=pct(92), eg_int=egi(0), decay=pct(80), level=pct(70),
       chorus_depth=pct(70), chorus_time=pct(30), motion={"chorus_time": sweep(pct(20), pct(78), 16, "sine")}),
  part(". . . . . . . . . . . . . . . .", osc1_wave=1, level=pct(0)))

# A10 ring-mod bells against a distorted square bass
pattern("A10", "RING TONES", 132,
  part("c1 . . c1 . c1 . . c1 . . eb1 . f1 . .", gate=0.55, osc1_wave=1, osc2_wave=1, osc_balance=pct(40), osc2_pitch=o2(0.1), cutoff=pct(38), resonance=pct(30), eg_int=egi(40), decay=pct(30), distortion=1, level=pct(55)),
  part("c4 . eb4 . . g4 . . bb4 . . g4 . eb4 . .", gate=0.35, osc_mod=1, osc_balance=pct(100), osc1_wave=2, osc2_wave=2, osc2_pitch=o2(19), cutoff=pct(70), resonance=pct(20), eg_int=egi(30), decay=pct(35), level=pct(60), delay_depth=pct(45), delay_time=56, chorus_depth=pct(30), chorus_time=pct(15)))

# A11 two-bar distorted acid, octave jumps and a slow filter opening
pattern("A11", "ACID LINE 2", 138,
  part("a1 a1 a2 a1 . a1 c2~ a1 a1 a2 . a1 g1 a1 e2 a1  a1 a1 a2 a1 . a1 c2~ a1 a1 a2 . a1 g2~ a2 e2 d2", gate=0.55, portamento=pct(10), osc1_wave=0, osc2_wave=0, osc_balance=pct(20), osc2_pitch=o2(0), cutoff=pct(30), resonance=pct(75), eg_int=egi(80), decay=pct(28), distortion=1, level=pct(45), delay_depth=pct(20), delay_time=24,
       motion={"cutoff": sweep(pct(22), pct(58), 32, "up")}),
  part("a0 . . . . . . . . . . . . . . .  a0 . . . . . . . . . . . g0 . . .", gate=1.5, osc1_wave=2, osc2_wave=2, osc_balance=pct(0), cutoff=pct(28), resonance=pct(5), eg_int=egi(15), decay=pct(60), level=pct(95)),
  length=2)

# A12 fast decimator chirps at 32nd scale, a nod to the box's IDM use
pattern("A12", "IDM CHIRPS", 150,
  part("c3! . eb3! . . g3! . . c4! . . bb3! . g3! . .  c3! . eb3! . . f3! . . c4! . . ab3! . g3! . eb3!", gate=0.5, osc_mod=3, osc_balance=pct(95), osc2_pitch=o2(24), cutoff=pct(55), resonance=pct(50), eg_int=egi(60), decay=pct(15), level=pct(70), delay_depth=pct(50), delay_time=24,
       motion={"osc2_pitch": sweep(o2(5), o2(40), 32, "sine")}),
  part("c1 . . . . . . c1 . . . . c1 . . .  c1 . . . . . . c1 . . . . eb1 . f1 .", gate=0.6, osc1_wave=1, osc2_wave=2, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(36), resonance=pct(25), eg_int=egi(35), decay=pct(30), level=pct(90)),
  length=2, beat=1)

print("wrote", sorted(f for f in os.listdir(OUT) if f.endswith(".json")))
