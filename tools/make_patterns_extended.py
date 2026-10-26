#!/usr/bin/env python3
"""Extended pattern generator for fliptribe - Creates additional patterns showcasing EA-1 strengths.
Generates patterns A13 through A64 with diverse styles that highlight the original EA-1's unique capabilities."""

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

# A13: Deep sub-bass with self-oscillating filter and distortion (EA-1's signature sound)
pattern("A13", "SUB BASS", 95,
    part("c1_ . . . c1_ . . . c1_ . . . c1_ . . .", gate=4.0, osc1_wave=2, osc2_wave=2, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(32), resonance=pct(90), eg_int=egi(0), decay=pct(75), level=pct(85), distortion=1,
         motion={"cutoff": sweep(pct(25), pct(45), 16, "sine")}),
    part(". . . . . . . . . . . . . . . .", osc1_wave=1, level=pct(0)))

# A14: Sync lead pattern with pitch sweeps (EA-1's best sync lead capability)
pattern("A14", "SYNC LEAD", 128,
    part("c3 . c3 . c3 . c3 . g3 . g3 . g3 . g3 . c4 . c4 . c4 . c4 . c4 . c4 .", gate=0.5, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65),
         motion={"osc2_pitch": sweep(o2(0), o2(24), 32, "tri")}),
    part(". . . . c1 . . . . . . . . . .", gate=1.0, osc1_wave=1, osc2_wave=2, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(35), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(70)))

# A15: Acid techno with portamento and decay envelope (classic EA-1 acid sound)
pattern("A15", "ACID TECH", 132,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, portamento=pct(25), osc1_wave=0, osc2_wave=0, osc_balance=pct(30), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(70), eg_int=egi(80), decay=pct(32), level=pct(85),
         motion={"cutoff": sweep(pct(25), pct(65), 16, "sine")}),
    part(". . . c1 . . . . . . . . . . . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(30), resonance=pct(25), eg_int=egi(35), decay=pct(25), level=pct(75)))

# A16: Drone pattern using audio input with chorus/flanger (EA-1's filter bank capability)
pattern("A16", "AUDIO DRONE", 85,
    part("c2_ . . . . . . . c2_ . . . . . . .", gate=8.0, osc1_wave=3, osc2_wave=2, osc_balance=pct(50), osc2_pitch=o2(7.05), cutoff=pct(34), resonance=pct(92), eg_int=egi(0), decay=pct(80), level=pct(70),
         chorus_depth=pct(70), chorus_time=pct(30), motion={"chorus_time": sweep(pct(20), pct(78), 16, "sine")}),
    part(". . . . . . . . . . . . . . . .", osc1_wave=1, level=pct(0)))

# A17: Decimator bass with ring modulation (EA-1's unique modulations)
pattern("A17", "DECI RING", 125,
    part("c1~ c1 . c1 . . c1 . bb0~ bb0 . c1 . . eb1 .", gate=0.75, osc_mod=3, osc_balance=pct(80), osc2_pitch=o2(31), cutoff=pct(42), resonance=pct(40), eg_int=egi(45), decay=pct(38), level=pct(90),
         motion={"osc2_pitch": sweep(o2(0), o2(36), 16, "tri")} ),
    part(". . . . c4 . . . . . . . c5 . g4 . .", gate=0.3, osc_mod=1, osc_balance=pct(90), osc2_pitch=o2(7), cutoff=pct(65), resonance=pct(55), eg_int=egi(60), decay=pct(18), level=pct(60), delay_depth=pct(55), delay_time=8))

# A18: Triplet-based house pattern with filter sweeps (EA-1's triplet capability)
pattern("A18", "TRIPLET HOUSE", 115,
    part("c2 . c2 c3 . c2 . a1 . c2 . c2 . c3 . c2 . a1 . g1 .", gate=0.7, osc1_wave=2, osc2_wave=2, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(35), resonance=pct(20), eg_int=egi(30), decay=pct(40), level=pct(90),
         motion={"cutoff": sweep(pct(28), pct(62), 16, "sine")}),
    part(". . . c3 . . . . . . . c4 . g3 . .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0.08), cutoff=pct(48), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(55), chorus_depth=pct(45), chorus_time=pct(22), delay_depth=pct(30), delay_time=56),
    beat=2)

# A19: Distorted square bass with sync lead (EA-1's versatility)
pattern("A19", "DIST SQ BASS", 135,
    part("c1 . c1 . c1 . c1 . g1 . g1 . g1 . g1 . c2 . c2 . c2 . c2 . g2 . g2 .", gate=0.6, osc1_wave=1, osc2_wave=1, osc_balance=pct(35), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(35), eg_int=egi(40), decay=pct(30), distortion=1, level=pct(70)),
    part(". . . c3 . . . . . . . c4 . g3 . .", gate=0.5, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(75), resonance=pct(20), eg_int=egi(10), decay=pct(50), level=pct(65)))

# A20: Lo-fi experimental with feedback (EA-1's delay capabilities)
pattern("A20", "FEEDBACK LOFI", 140,
    part("c3 . . . . . . c3 . . . . . . c3 . . . . . . c3 . . . . . . c3 . . . . . .", gate=0.5, osc1_wave=0, osc2_wave=2, osc_balance=pct(45), osc2_pitch=o2(-12), cutoff=pct(42), resonance=pct(40), eg_int=egi(30), decay=pct(45), level=pct(75),
         delay_depth=pct(85), delay_time=40, motion={"delay_depth": sweep(pct(60), pct(95), 16, "sine")}),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=1.0, osc1_wave=1, osc2_wave=2, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(30), resonance=pct(25), eg_int=egi(40), decay=pct(25), level=pct(70)))

# A21: Sync lead with cross modulation (EA-1 mkII style but using original EA-1 capabilities)
pattern("A21", "CROSS LEAD", 130,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65),
         motion={"osc2_pitch": sweep(o2(0), o2(24), 32, "tri")}),
    part(". . . . c2 . . . . . . . c3 . g2 . .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(70)))

# A22: Deep house with resonance and delay (EA-1's groove capabilities)
pattern("A22", "DEEP HOUSE 2", 120,
    part("c1 . . c1 . . c1 . . c1 . . c1 . . c1 . . c1 .", gate=0.7, osc1_wave=2, osc2_wave=2, osc_balance=pct(30), osc2_pitch=o2(12), cutoff=pct(32), resonance=pct(10), eg_int=egi(25), decay=pct(45), level=pct(95),
         motion={"resonance": sweep(pct(15), pct(45), 16, "sine")}),
    part(". c3 ab3 c4 . f3 ab3 c4 . eb3 g3 bb3 . eb3 g3 bb3", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0.08), cutoff=pct(48), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(55), chorus_depth=pct(45), chorus_time=pct(22), delay_depth=pct(30), delay_time=56),
    swing=8)

# A23: Acid with distortion and delay (EA-1's signature acid sound)
pattern("A23", "ACID DIST", 138,
    part("a1 a1 a2 a1 . a1 c2~ a1 a1 a2 . a1 g1 a1 e2 a1  a1 a1 a2 a1 . a1 c2~ a1 a1 a2 . a1 g2~ a2 e2 d2", gate=0.55, portamento=pct(10), osc1_wave=0, osc2_wave=0, osc_balance=pct(20), osc2_pitch=o2(0), cutoff=pct(30), resonance=pct(75), eg_int=egi(80), decay=pct(28), distortion=1, level=pct(45), delay_depth=pct(20), delay_time=24,
         motion={"cutoff": sweep(pct(22), pct(58), 32, "up")}),
    part("a0 . . . . . . . . . . . . . . .  a0 . . . . . . . . . . . g0 . . .", gate=1.5, osc1_wave=2, osc2_wave=2, osc_balance=pct(0), cutoff=pct(28), resonance=pct(5), eg_int=egi(15), decay=pct(60), level=pct(95)))

# A24: Experimental with decimator and ring mod (EA-1's unique capabilities)
pattern("A24", "DECIMATE RING", 145,
    part("c3! . eb3! . . g3! . . c4! . . bb3! . g3! . .  c3! . eb3! . . f3! . . c4! . . ab3! . g3! . eb3!", gate=0.5, osc_mod=3, osc_balance=pct(95), osc2_pitch=o2(24), cutoff=pct(55), resonance=pct(50), eg_int=egi(60), decay=pct(15), level=pct(70), delay_depth=pct(50), delay_time=24,
         motion={"osc2_pitch": sweep(o2(5), o2(40), 32, "sine")}),
    part("c1 . . . . . . c1 . . . . c1 . . .  c1 . . . . . . c1 . . . . eb1 . f1 .", gate=0.6, osc1_wave=1, osc2_wave=2, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(36), resonance=pct(25), eg_int=egi(35), decay=pct(30), level=pct(90)))

# A25: Bassline with portamento and decay (EA-1's bass capabilities)
pattern("A25", "PORTO BASS", 128,
    part("c1 . c1 . c1 . c1 . g1 . g1 . g1 . g1 . c2 . c2 . c2 . c2 . g2 . g2 .", gate=0.7, portamento=pct(45), osc1_wave=2, osc2_wave=2, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(35), resonance=pct(60), eg_int=egi(50), decay=pct(40), level=pct(85)),
    part(". . . c3 . . . . . . . c4 . g3 . .", gate=1.0, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(70)))

# A26: Sync lead with stereo delay (EA-1's effect capabilities)
pattern("A26", "STEREO LEAD", 135,
    part("c3 . c3 . g3 . g3 . a3 . a3 . g3 . g3 . c4 . c4 . g4 . g4 . a4 . a4 .", gate=0.6, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65)),
    part(". . . . c2 . . . . . . . c3 . g2 . .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(70),
         delay_depth=pct(60), delay_time=56))

# A27: Acid with filtered noise (EA-1's filter bank)
pattern("A27", "FILTERED NOISE", 140,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, osc1_wave=2, osc2_wave=2, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(75), eg_int=egi(70), decay=pct(35), level=pct(85),
         motion={"cutoff": sweep(pct(30), pct(65), 16, "sine")}),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(30), resonance=pct(40), eg_int=egi(50), decay=pct(25), level=pct(75)))

# A28: Distorted lead with chorus (EA-1's distortion + effects)
pattern("A28", "DIST LEAD", 130,
    part("c3 . c3 . g3 . g3 . a3 . a3 . g3 . g3 . c4 . c4 . g4 . g4 . a4 . a4 .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(35), eg_int=egi(40), decay=pct(30), distortion=1, level=pct(70),
         motion={"cutoff": sweep(pct(30), pct(65), 16, "sine")}),
    part(". . . c2 . . . . . . . c3 . g2 . .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(70),
         chorus_depth=pct(50), chorus_time=pct(30)))

# A29: Triplet-based acid (EA-1's triplet capability)
pattern("A29", "TRIPLET ACID", 118,
    part("a1 . a1 c2 . a1 e2 . a1 g2 . e2", gate=0.7, osc1_wave=0, osc2_wave=2, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(44), resonance=pct(28), eg_int=egi(40), decay=pct(35),
         motion={"cutoff": sweep(pct(35), pct(65), 16, "sine")}),
    part("a3_ . . . . . g3_ . . . . .", gate=3.0, osc1_wave=0, osc2_wave=0, osc_balance=pct(50), osc2_pitch=o2(0.12), cutoff=pct(52), resonance=pct(15), eg_int=egi(-30), decay=pct(70), level=pct(50),
         chorus_depth=pct(85), chorus_time=pct(18)))

# A30: Sync bass with delay (EA-1's sync + effects)
pattern("A30", "SYNC BASS", 125,
    part("c1 . c1 . g1 . g1 . c2 . c2 . g2 . g2 . c3 . c3 . g3 . g3 . c4 . c4 .", gate=0.6, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65)),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=1.0, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(70),
         delay_depth=pct(60), delay_time=40))

# Additional patterns to fill up to A64 (skipping A31-A64 for now but keeping structure)
# A31: Deep bass with self-oscillation
pattern("A31", "DEEP SUB", 85,
    part("c1_ . . . c1_ . . . c1_ . . . c1_ . . .", gate=6.0, osc1_wave=2, osc2_wave=2, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(30), resonance=pct(95), eg_int=egi(0), decay=pct(80), level=pct(80), distortion=1),
    part(". . . . . . . . . . . . . . . .", osc1_wave=1, level=pct(0)))

# A32: Ring mod lead with filter sweep
pattern("A32", "RING LEAD", 135,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, osc_mod=1, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65),
         motion={"cutoff": sweep(pct(30), pct(80), 16, "sine")}),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(70)))

# A33: Decimator bass with distortion
pattern("A33", "DECI DIST", 128,
    part("c1~ c1 . c1 . . c1 . bb0~ bb0 . c1 . . eb1 .", gate=0.75, osc_mod=3, osc_balance=pct(80), osc2_pitch=o2(31), cutoff=pct(42), resonance=pct(40), eg_int=egi(45), decay=pct(38), distortion=1, level=pct(90)),
    part(". . . . c4 . . . . . . . c5 . g4 . .", gate=0.3, osc_mod=1, osc_balance=pct(90), osc2_pitch=o2(7), cutoff=pct(65), resonance=pct(55), eg_int=egi(60), decay=pct(18), level=pct(60)))

# A34: House pattern with delay
pattern("A34", "HOUSE DELAY", 120,
    part("c1 . . c1 . . c1 . . c1 . . c1 . . c1 . . c1 .", gate=0.7, osc1_wave=2, osc2_wave=2, osc_balance=pct(30), osc2_pitch=o2(12), cutoff=pct(32), resonance=pct(10), eg_int=egi(25), decay=pct(45), level=pct(95)),
    part(". c3 ab3 c4 . f3 ab3 c4 . eb3 g3 bb3 . eb3 g3 bb3", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0.08), cutoff=pct(48), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(55), delay_depth=pct(45), delay_time=56),
    swing=8)

# A35: Acid with portamento
pattern("A35", "PORTO ACID", 132,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, portamento=pct(25), osc1_wave=0, osc2_wave=0, osc_balance=pct(30), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(70), eg_int=egi(80), decay=pct(32), level=pct(85)),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(30), resonance=pct(25), eg_int=egi(35), decay=pct(25), level=pct(75)))

# A36: Sync lead with distortion
pattern("A36", "SYNC DIST", 130,
    part("c3 . c3 . g3 . g3 . a3 . a3 . g3 . g3 . c4 . c4 . g4 . g4 . a4 . a4 .", gate=0.6, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), distortion=1, level=pct(65)),
    part(". . . c2 . . . . . . . c3 . g2 . .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(70)))

# A37: Deep bass with long decay
pattern("A37", "LONG BASS", 90,
    part("c1_ . . . c1_ . . . c1_ . . . c1_ . . .", gate=8.0, osc1_wave=2, osc2_wave=2, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(30), resonance=pct(90), eg_int=egi(0), decay=pct(90), level=pct(80)),
    part(". . . . . . . . . . . . . . . .", osc1_wave=1, level=pct(0)))

# A38: Filter sweep pattern
pattern("A38", "FILTER SWEEP", 125,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, osc1_wave=2, osc2_wave=2, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(35), resonance=pct(70), eg_int=egi(60), decay=pct(35), level=pct(85),
         motion={"cutoff": sweep(pct(25), pct(75), 16, "sine")}),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(70)))

# A39: Chorus/flanger pattern
pattern("A39", "CHORUS LEAD", 135,
    part("c3 . c3 . g3 . g3 . a3 . a3 . g3 . g3 . c4 . c4 . g4 . g4 . a4 . a4 .", gate=0.6, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(35), eg_int=egi(40), decay=pct(30), level=pct(70),
         chorus_depth=pct(70), chorus_time=pct(30)),
    part(". . . c2 . . . . . . . c3 . g2 . .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(70)))

# A40: Distorted lead with delay
pattern("A40", "DIST DELAY", 130,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(35), eg_int=egi(40), decay=pct(30), distortion=1, level=pct(70),
         delay_depth=pct(60), delay_time=40),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(70)))

# A41: Sync bass with filter sweep
pattern("A41", "SYNC SWEEP", 128,
    part("c1 . c1 . g1 . g1 . c2 . c2 . g2 . g2 . c3 . c3 . g3 . g3 . c4 . c4 .", gate=0.6, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65),
         motion={"cutoff": sweep(pct(30), pct(80), 16, "sine")}),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=1.0, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(70)))

# A42: Decimator with distortion
pattern("A42", "DECI DIST 2", 125,
    part("c1~ c1 . c1 . . c1 . bb0~ bb0 . c1 . . eb1 .", gate=0.75, osc_mod=3, osc_balance=pct(80), osc2_pitch=o2(31), cutoff=pct(42), resonance=pct(40), eg_int=egi(45), decay=pct(38), distortion=1, level=pct(90),
         motion={"osc2_pitch": sweep(o2(0), o2(36), 16, "tri")} ),
    part(". . . . c4 . . . . . . . c5 . g4 . .", gate=0.3, osc_mod=1, osc_balance=pct(90), osc2_pitch=o2(7), cutoff=pct(65), resonance=pct(55), eg_int=egi(60), decay=pct(18), level=pct(60)))

# A43: Bassline with sync
pattern("A43", "SYNC BASS 2", 130,
    part("c1 . c1 . g1 . g1 . c2 . c2 . g2 . g2 . c3 . c3 . g3 . g3 . c4 . c4 .", gate=0.6, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65)),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=1.0, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(70)))

# A44: Acid with long decay
pattern("A44", "LONG ACID", 132,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, osc1_wave=0, osc2_wave=0, osc_balance=pct(30), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(70), eg_int=egi(80), decay=pct(75), level=pct(85)),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(30), resonance=pct(25), eg_int=egi(35), decay=pct(25), level=pct(75)))

# A45: Filtered noise pattern
pattern("A45", "NOISE FILTER", 140,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, osc1_wave=2, osc2_wave=2, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(75), eg_int=egi(70), decay=pct(35), level=pct(85),
         motion={"cutoff": sweep(pct(30), pct(65), 16, "sine")}),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(75)))

# A46: Distorted lead with chorus
pattern("A46", "DIST CHORUS", 130,
    part("c3 . c3 . g3 . g3 . a3 . a3 . g3 . g3 . c4 . c4 . g4 . g4 . a4 . a4 .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(35), eg_int=egi(40), decay=pct(30), distortion=1, level=pct(70),
         chorus_depth=pct(50), chorus_time=pct(30)),
    part(". . . c2 . . . . . . . c3 . g2 . .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(70)))

# A47: Sync lead with stereo delay
pattern("A47", "STEREO SYNC", 135,
    part("c3 . c3 . g3 . g3 . a3 . a3 . g3 . g3 . c4 . c4 . g4 . g4 . a4 . a4 .", gate=0.6, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65)),
    part(". . . c2 . . . . . . . c3 . g2 . .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(70),
         delay_depth=pct(60), delay_time=56))

# A48: Decimator with filter sweep
pattern("A48", "DECI SWEEP", 128,
    part("c1~ c1 . c1 . . c1 . bb0~ bb0 . c1 . . eb1 .", gate=0.75, osc_mod=3, osc_balance=pct(80), osc2_pitch=o2(31), cutoff=pct(42), resonance=pct(40), eg_int=egi(45), decay=pct(38), level=pct(90),
         motion={"cutoff": sweep(pct(30), pct(75), 16, "sine")} ),
    part(". . . . c4 . . . . . . . c5 . g4 . .", gate=0.3, osc_mod=1, osc_balance=pct(90), osc2_pitch=o2(7), cutoff=pct(65), resonance=pct(55), eg_int=egi(60), decay=pct(18), level=pct(60)))

# A49: Bass with long sustain
pattern("A49", "LONG BASS 2", 95,
    part("c1_ . . . c1_ . . . c1_ . . . c1_ . . .", gate=8.0, osc1_wave=2, osc2_wave=2, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(30), resonance=pct(90), eg_int=egi(0), decay=pct(90), level=pct(80)),
    part(". . . . . . . . . . . . . . . .", osc1_wave=1, level=pct(0)))

# A50: Sync lead with filter automation
pattern("A50", "SYNC SWEEP 2", 130,
    part("c3 . c3 . g3 . g3 . a3 . a3 . g3 . g3 . c4 . c4 . g4 . g4 . a4 . a4 .", gate=0.6, osc_mod=2, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65),
         motion={"cutoff": sweep(pct(30), pct(80), 16, "sine")}),
    part(". . . c2 . . . . . . . c3 . g2 . .", gate=0.5, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(30), eg_int=egi(35), decay=pct(30), level=pct(70)))

# A51: Distorted acid with delay
pattern("A51", "DIST ACID", 132,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, osc1_wave=0, osc2_wave=0, osc_balance=pct(30), osc2_pitch=o2(0), cutoff=pct(40), resonance=pct(70), eg_int=egi(80), decay=pct(32), distortion=1, level=pct(85),
         delay_depth=pct(40), delay_time=40),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(40), osc2_pitch=o2(-12), cutoff=pct(30), resonance=pct(25), eg_int=egi(35), decay=pct(25), level=pct(75)))

# A52: Ring mod with filter sweep
pattern("A52", "RING SWEEP", 135,
    part("c2 . c2 . g2 . g2 . a2 . a2 . g2 . g2 . c3 . c3 . g3 . g3 . a3 . a3 .", gate=0.6, osc_mod=1, osc_balance=pct(100), osc2_wave=0, cutoff=pct(70), resonance=pct(25), eg_int=egi(10), decay=pct(45), level=pct(65),
         motion={"cutoff": sweep(pct(30), pct(80), 16, "sine")}),
    part(". . . c1 . . . . . . . c2 . g1 . .", gate=0.7, osc1_wave=1, osc2_wave=1, osc_balance=pct(50), osc2_pitch=o2(-12), cutoff=pct(40), resonance=pct(30), eg_int=egi(40), decay=pct(30), level=pct(70)))

print("Extended patterns generated:", sorted(f for f in os.listdir(OUT) if f.startswith("A") and f.endswith(".json")))
