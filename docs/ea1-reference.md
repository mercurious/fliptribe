# Korg Electribe·A EA-1 (1999, original silver/blue model) — Reference for a software recreation

Scope: the ORIGINAL EA-1 (model code X-829B, released 1999), not the EA-1 mkII (2003). Where a
source blurs the two, it is flagged. Every section lists the URLs it was built from. Items the
sources do not settle are marked **UNCONFIRMED**.

Primary sources used throughout (downloaded and read in full):

- Korg EA-1 Owner's Manual, English, `EA1_OM_E.pdf` (52 pp., ©1999 KORG, "E 2" edition):
  https://www.korg.com/us/support/download/manual/1/95/1724/
  (direct file: https://cdn.korg.com/us/support/download/files/20bbceb7746568bb82fed9080350fca2.pdf)
- Korg EA-1 MIDI Implementation (text file `EA1_MIDIimp.TXT` inside `EA1_ER1_MIDIimp.zip`, which also
  holds the mkII version for comparison): https://www.korg.com/us/support/download/manual/1/95/2664/
  (direct file: https://cdn.korg.com/us/support/download/files/368dc4d240a7d0aed11b61ccfe63d1e4.zip)
- Korg EA-1 EasyStart guide: https://www.korg.com/us/support/download/manual/1/95/2071/
- Korg EA-1/ER-1 FAQ (knob → NRPN chart): https://www.korg.com/us/support/download/manual/1/95/3308/
- Korg EA-1/ER-1 Service Manual (mk1, rev 1.00, Aug 1999; block diagram, OCR text):
  https://archive.org/details/ea-1-er-1-service-manual
- Sound On Sound review, Chris Carter, July 1999: https://www.soundonsound.com/reviews/korg-electribe-ea1-er1
- Vintage Synth Explorer: https://www.vintagesynth.com/korg/electribe-ea-1
- Sweetwater SweetNotes, Fall 1999: https://www.sweetwater.com/publications/sweetnotes/sn-fall99/page08.html
- Synthtopia review (2003): https://www.synthtopia.com/?p=389
- sequencer.de Electribe wiki (German): https://www.sequencer.de/synth/index.php/Electribe
- getlofi.com teardown notes (2005): https://getlofi.com/page/97/
- Audiofanzine user reviews (EA-1 and EA-1 mkII): https://en.audiofanzine.com/groove-box/korg/electribe-ea-1/user_reviews/
  and https://en.audiofanzine.com/groove-box/korg/ElecTribe-Ea-1-MkII/user_reviews/r.82233.html

---

## 0. Hardware facts relevant to an emulation (service manual block diagram)

Source: https://archive.org/details/ea-1-er-1-service-manual (EA1_ER1_ServiceManual, block diagram sheet
KOD-B40194, dated '99.2.22). OCR is noisy; chip names below were legible.

- Master clock 20 MHz; the block diagram states **Fs = 39.0625 kHz** (20 MHz / 512; 256Fs clock derived
  with a 74HC4040). Nyquist is therefore ~19.5 kHz. This is the system sample rate for the DSP and the
  codec.
- DSP: Texas Instruments **TMS57070FFT** (the same family Korg used in other late-1990s DSP products).
  getlofi.com reports "two DSP chips" on the board; the block diagram OCR shows the TMS57070 label once.
  Count **UNCONFIRMED**; sample rate is not in doubt.
- Host CPU: Hitachi/Renesas **H8S/2350** (runs the panel, sequencer and MIDI; talks to the DSP over a
  serial line).
- Codec: Philips **UDA1309H** (stereo ADC/DAC; the EA-1 uses one input channel for AUDIO IN and two
  output channels for PART1/MIX and PART2). Headphone amps TDA1308.
- Memory: 8 Mbit flash (MBM29F800BA-70, system ROM + stored patterns), 4 Mbit DRAM
  (DSP delay memory), 4 Mbit system RAM.
- The EA-1 and ER-1 share the same main PCB (KLM-2092) with a model jumper; only the panel PCB differs
  (KLM-2095 for EA-1). getlofi.com relays an unverified claim that the engine is related to the MS2000's
  and notes the EA-1 delay "sounds virtually identical" to the MS2000 delay. Treat as anecdote.
- Outputs: two mono 1/4" jacks (PART1/MIX, PART2), -10 dBu nominal, 1 kΩ. PART1/MIX carries both parts
  when nothing is plugged into PART2 (manual: "If you wish to output the sound of Parts 1 and 2 together,
  make connections to the PART1/MIX jack"). Headphone output is **monaural** (both parts summed).
  There is no stereo bus and no pan control anywhere on the instrument.
- Input: one mono 1/4" AUDIO IN, -10 dBu nominal, 47 kΩ; line level only.
- Power 9 V DC adapter, 8 W; 300 × 222.5 × 53.4 mm; 1.25 kg. Mains only (SOS: no batteries).

---

## 1. Front panel inventory

Sources: Owner's Manual ch. 2 "Front and rear panel" (pp. 8–11) and ch. 4–7; SOS review;
EasyStart. The panel is divided by Korg into four groups: Common, Synthesizer, Sequence Control, Step
Key (plus the rear Connector section).

### 1.1 Common section (top left)

| Control | Type | Range / states | Function |
|---|---|---|---|
| MASTER VOLUME | knob | – | Output level of both part jacks and headphones. |
| AUDIO IN THRU | key + LED | on/off | Passes the external input straight to the outputs. While on, "Audio In" as OSC1 has no effect. |
| Display | 3-character 7-segment LED | – | Shows value of the selected matrix parameter and messages. |
| Dial | endless rotary encoder | – | Edits the displayed value; Shift+dial moves in steps of 10. |
| Peak LED | LED | – | Input peak indicator for AUDIO IN. |
| Beat LED | LED | – | Blinks on quarter notes at the current tempo. |
| Cursor ▲ / ▼ | 2 keys | – | Move the lit "parameter select" LED up/down the matrix column of the current mode. |
| Matrix | printed 5 × 4 grid with LEDs | – | Rows = parameters, columns = modes (see 1.6). |
| Mode keys | 4 keys with LEDs: PATTERN, SONG, GLOBAL, MIDI | – | Select mode. Global can be entered during playback; MIDI mode cannot. **There is no rotary mode selector on the EA-1** (that arrived with the 2nd-generation Electribes, e.g. EMX/ESX); mode is chosen with these four keys. |
| WRITE | key + LED | – | Save pattern / song / global / MIDI / Pattern Set data (press, choose destination, press again). |

Display behaviour (3 characters, 7-segment, with decimal points):

- Pattern numbers: `A01`…`A64`, `b01`…`b64`, `C01`…`C64`, `d01`…`d64` (lower-case b and d because of the
  7-segment font). Songs: `S01`…`S16`. Position: `001`…`256`, then `End`.
- Target step: `1.01`…`4.16` (bar.step; EasyStart shows the form "Step 5(37)" = length 3, step 5).
- Pitch: note name + octave, e.g. A#3, B4, D#-1.
- Tempo: `20`…`300`. Swing: `50`…`75`. Transpose / pitch shift: `-24`…`24`.
- MIDI filter shows three characters `PCE`, each `O` or `-`.
- Messages: `PSt` (writing Pattern Set), `PLd` (factory preload), `---` (writing Global/MIDI),
  `Full` (song event memory full), `Er.1` (write failed), `Er.2` (event limit), `Er.9` (protect on).
- When a new pattern is selected during playback the displayed number **blinks** until the change
  takes effect at the end of the current pattern.
- The "Original Value" LED (in the Synthesizer section) lights when a knob being moved passes through
  the value stored in the pattern (SOS: works until you save).

### 1.2 Synthesizer section (right)

Ordered as printed on the panel, left to right. All continuous knobs are 0–127 internally (NRPN / SysEx
resolution 7-bit); the panel legends below are the ranges the manual prints.

OSCILLATOR

| Control | Type | Range | Function |
|---|---|---|---|
| Portamento | knob | 0…100 | Glide time between pitches; further right = slower. |
| OSC Balance | knob | OSC1 … OSC2 | Crossfade between the two oscillators; at either extreme the other oscillator is at 0. OSC Mod results come out of the OSC2 side, so set toward OSC2 to hear them. |
| OSC2 Pitch Ofs | knob | –1 octave … +4 octaves (relative to OSC1) | Offset of OSC2 pitch. SOS: has a centre detent, very sensitive, hard to fine-tune; detent "sometimes drifts". |
| OSC1 Wave | key, 4 LEDs | Saw, Square, Triangle, Audio In | Cycles the OSC1 waveform. |
| OSC2 Wave | key, 3 LEDs | Saw, Square, Triangle | Cycles the OSC2 waveform (ignored when OSC Mod = Deci). |
| OSC Mod | key, 3 LEDs | off (all dark), Ring, Sync, Deci | Oscillator modulation type; OSC2 is always the slave. |

Panel also carries: Original Value LED; **Motion Seq** key with 2 LEDs (cycles Off → Smooth → Trig Hold);
**Part Select** key with Part 1 / Part 2 LEDs (both blink when "both parts" is chosen inside Move Data /
Pitch Shift); and a printed synthesis block diagram (OSC1/OSC2/Audio In → OSC Mod → Filter → Amp
(Distortion, Level) → Chorus/Flanger → Delay → Output).

FILTER

| Control | Type | Range | Function |
|---|---|---|---|
| Cutoff | knob | 0 Hz … 20,000 Hz (printed) | Low-pass cutoff. "If Cutoff and EG Int are set to 0, you will hear virtually no sound." |
| Resonance | knob | 0…100 | Peak at cutoff; manual promises the "meow-ing" analog sound; warns it "may be distorted depending on cutoff or pitch". |
| EG Int | knob, centre = 0 | –100 … 0 … +100 | Bipolar amount of the (single) envelope on cutoff. |
| Decay | knob | 0 msec … 10 sec | Decay time of that envelope. With negative EG Int it acts as a pseudo-attack. |

AMP

| Control | Type | Range | Function |
|---|---|---|---|
| Distortion | key + LED | On / Off | Hard clipping-style distortion, no amount control (the NRPN table calls Level "Distortion Level", i.e. Level is effectively the drive). |
| Level | knob | 0…100 | Part volume. |

EFFECT

| Control | Type | Range | Function |
|---|---|---|---|
| Type Select | key, 2 LEDs | Tempo Delay / Chorus-Flanger | Chooses which effect the two knobs below edit. Both effects keep their own stored Depth/Time (separate NRPNs and SysEx bytes), so both can be set at once. |
| Depth | knob | 0…100 | Delay: wet level and feedback together. Chorus/Flanger: depth; towards the right it "changes from a chorus effect to a flanger effect" (adds feedback). |
| Time | knob | Delay: 1/4 … 8 (16 tempo-synced step multiples); Chorus/Flanger LFO: 0.2 Hz … 5,000 Hz | Delay time or modulation rate. |

### 1.3 Sequence Control section

| Control | Function |
|---|---|
| REC key (large, lit red) | Enter record-ready; during recording, pressing it ends recording but keeps playing. Shift+Rec while playing = erase triggers of the selected part while held. |
| STOP/CANCEL key | Stop / cancel an operation / Shift+Stop = MIDI reset (all notes off). |
| PLAY/PAUSE key | Play / pause (LED blinks while paused). Shift+Play = Reset & Play (restart from step 1; transmits only MIDI Start). |
| TAP / REST-TIE key | Tap tempo (3+ taps, works stopped or playing). In step record: rest (press) or tie (hold step key + press). |
| TRANSPOSE key + LED | Hold and turn dial (or press step keys, key 4 = C, or Select keys) to transpose the whole pattern –24…+24 semitones live. LED lit when ≠ 0. Not saved; resets to 0 at power-on. Also used (lit) to switch Move Data into Pitch Shift mode, and held with WRITE at power-on to factory-reset. |

### 1.4 Step Key section

| Control | Function |
|---|---|
| SELECT ◀ / ▶ keys | Choose which of the 4 bars (lengths 1–4) the 16 step keys show; in Keyboard mode they switch octave; in Song mode rewind / fast-forward; in Pattern Set mode they switch groups 1–4. Shift+Select = step the target step by one. |
| Select LEDs: 2 rows × 4 | Upper row (green): the bar currently **playing**. Lower row (red): the bar currently **displayed** on the step keys / the Keyboard octave / the Pattern Set group. |
| Step keys 1–16 (lit rubber keys) | Trigger on/off per step (lit = trigger). While playing, the current step's key also lights. In Keyboard mode: a 16-note keyboard. In Pattern Set: 16 pattern slots. With Shift: the functions printed under each key. |
| KEYBOARD key (lit) | Toggles Keyboard function. Hold + step key = Pattern Set recall; Shift+Keyboard = Pattern Set "hold" (key blinks). |
| SHIFT key | Modifier. Shift+step key 1–4 Length, 5–7 Beat/Scale, 8 Swing, 9 Move Data / Pitch Shift, 10 Copy Part, 11 Clear Motion, 12 Clear Part, 13 Insert Pattern, 14 Delete Pattern, 15 Clear Event, 16 Clear Song. Holding Shift lights the keys showing current length and beat. |

Step-key LED behaviour summary (manual p. 23, 27–28):

- Keyboard off: keys show the trigger map of the selected part for the displayed bar; the key for the
  target step blinks (in Step Rec / Pitch / Gate Time pages); the playing step lights as it passes.
- Keyboard on while playing: keys light in succession to show the **pitch** the selected part is
  sounding (within the displayed octave) — this is also how incoming MIDI notes are shown (SOS).
- Shift held: keys 1–4 show length, 5–7 show beat/scale.
- Shift + Motion Seq: keys 1–4 light if the selected part has motion data; in Song mode keys 13–16
  light if the song has event data.
- Triplet scale: keys 13–16 are unused (12 steps per bar).

Keyboard-mode layout (EasyStart): key 1 = A, keys 4 and 16 = C, black keys are printed as such, so
the 16 keys cover A…C (16 semitones). Octave is chosen with Select and shown by the red LEDs:

| Red LEDs lit | 1 | 1,2 | 1,2,3 | 2,3,4 | 3,4 | 4 |
|---|---|---|---|---|---|---|
| Range | A0…C2 | A1…C3 | A2…C4 | A3…C5 | A4…C6 | A5…C7 |

### 1.5 Connector section (rear)

Power switch; DC 9 V; adapter cable holder; MIDI IN / OUT / THRU; AUDIO IN (mono); PART1/MIX and PART2
outputs; stereo-jack headphone output (mono signal).

### 1.6 The parameter matrix (what the cursor keys select, per mode)

| Row | PATTERN | SONG | GLOBAL | MIDI |
|---|---|---|---|---|
| 1 | Pattern (A01…d64) | Song (S01…S16) | Metronome (oFF, r-0, r-1, r-2, on) | MIDI ch (1…16, part 1) |
| 2 | Tempo (20…300) | Tempo (20…300) | Input Gain (0…100) | P2 ch (1…16, part 2) |
| 3 | Step Rec (target step 1.01…4.16) | Position (001…256) | Clock (int, Ext) | Dump (Ptn, SnG, ALL) |
| 4 | Pitch (C-1…G9) | Pattern (A01…d64) | Protect (on, oFF) | MIDI Filter (PCE) |
| 5 | Gate Time (0.25…64.0) | Pitch Offset (–24…24) | – | Bend Range (–12…12) |

Defaults: MIDI ch 1, P2 ch 2, Protect on, Clock int, Filter all `O`, Metronome oFF at every power-on
(metronome setting is not stored).

---

## 2. Synth architecture per part

Sources: Owner's Manual pp. 6, 9, 23–25, 46; MIDI implementation TABLE1/TABLE3; SOS review;
Sweetwater; getlofi; service manual.

### 2.1 Overview

- Two **parts**, each a strictly **monophonic** synth voice ("cannot play two or more notes
  simultaneously"), each with its own output jack and its own effects. Total polyphony = 2.
  Note priority within a part is **UNCONFIRMED** (not documented).
- Signal flow per part (manual's panel diagram): OSC1 (saw / square / tri / Audio In) and OSC2
  (saw / square / tri) → OSC Mod (off / Ring / Sync / Deci, output on the OSC2 side) → OSC Balance mix
  → resonant low-pass Filter with one decay-only EG → Amp (Level, Distortion on/off) → Chorus/Flanger
  → Tempo Delay → part output.
- Korg calls it "DSP synthesis" / "analog modeling"; Sweetwater's 1999 copy says it uses "the same
  tone generators" as the Prophecy and Z1. SOS likewise says "based on Korg Z1 analogue modelling".
  Treat as marketing lineage, not an implementation statement.
- No LFO, no velocity, no accent, no pan, no amp envelope, no second filter type, no noise source
  (SOS; sequencer.de: "no velocity, no accent … no roll/flam").

### 2.2 Oscillators

- OSC1 waveforms: **Sawtooth, Square, Triangle, Audio In** (NRPN values 0–3 in that order per the
  official MIDI implementation; the Korg FAQ chart lists them as Triangle/Square/Saw/Audio In — the
  MIDI implementation document is the authoritative one).
- OSC2 waveforms: **Sawtooth, Square, Triangle** (0–2).
- Manual on the waves: saw "rich in overtones … bass or synth lead"; square "contains only the
  odd-numbered harmonics" (i.e. a fixed 50 % pulse — there is no pulse-width control; the manual's
  example sound called "Pulse Width Mod" is achieved with two squares detuned); triangle "more mellow
  … suitable for sub-bass".
- Important for modelling: "So that they will be particularly suitable for bass sounds, **the waveforms
  of OSC1 have a slightly heavier sound (fundamental) than the waveforms of OSC2**." OSC1 and OSC2 are
  therefore not identical wavetables; OSC1 has more fundamental weight.
- SOS: OSC1's frequency range "is narrower than Osc 2's" (OSC2 can be offset up to +4 octaves).
- OSC2 Pitch Offset: –1 octave … +4 octaves relative to OSC1, 0–127 over MIDI (so roughly 60 semitones
  across 128 steps, i.e. not quantised to semitones; SOS complains it is hard to tune precisely).
  Whether the mapping is linear in semitones is **UNCONFIRMED**; a centre detent exists (SOS).
- OSC Balance: 0–127, OSC1 … OSC2, equal-power vs linear **UNCONFIRMED**.
- Phase: the manual notes "the tone and volume of Ring Modulation may vary depending on the phase
  difference between OSC1 and OSC2", which implies the oscillators are **free-running** (not reset on
  note-on).
- Portamento 0–100: "smoothly connects changes in pitch … further right, the pitch will change more
  slowly … if raised excessively, a certain amount of time will elapse before the destination pitch
  is reached." The manual describes it as applying to every pitch change (no legato-only mode is
  mentioned). Whether it is constant-time or constant-rate is **UNCONFIRMED**.

### 2.3 OSC Mod types (original EA-1: off, Ring, Sync, Deci — confirmed)

Confirmed by the owner's manual (p. 24), the NRPN table ("Osc Mode 0~3 : Off/Ring/Sync/Deci") and the
SysEx pattern format. The **mkII** replaces Deci with **Cross** ("Off/Ring/Sync/Cross" in
`EA1mkII_MIDIimp.txt`; the only differences between the two MIDI implementation files are those three
lines). So: the original has Decimator and NO cross-modulation; the mkII has cross modulation and NO
decimator. Any page listing "ring, sync, cross" is describing the mkII.

- **Off**: no interaction; detune with OSC2 Pitch Ofs for "spacious detune"; one octave down/up for
  "thick synth-bass".
- **Ring**: "generates a frequency consisting of the sum and difference of the OSC1 and OSC2
  frequencies"; metallic sounds / effects when OSC2 pitch is moved; output on OSC2 side; result depends
  on relative phase.
- **Sync**: "forcibly resets the OSC2 waveform by synchronizing its pitch to the pitch of OSC1" (hard
  sync, OSC1 = master, OSC2 = slave). Sweeping OSC2 pitch gives the "sync lead". "Differences in the
  OSC1 waveform will not affect the output of OSC2."
- **Deci (Decimator)**: "reproduces the OSC1 waveform at the sampling frequency of OSC2. The digitized
  OSC1 waveform will be output from OSC2" — i.e. sample-and-hold resampling of OSC1 at a rate set by
  the OSC2 pitch (OSC2 Pitch Ofs becomes "Sampling Freq"). OSC2's own waveform selection is ignored.
  "If OSC1 is a square wave, Decimator will have little effect." "Effective when creating electric
  bass sounds." SOS: "bit reduction … down-sampled, grungy edginess" (SOS calls it bit reduction, the
  manual describes sample-rate reduction; implement as sample-rate reduction with OSC2 frequency as
  the hold clock, which is what produces aliasing sidebands).
- All three leave OSC1's own output untouched on the OSC1 side of the balance knob.
- With OSC1 = Audio In, Ring and Deci operate on the external signal (SOS confirms external audio
  can be ring-modulated and decimated).

### 2.4 Filter

- One resonant **low-pass** filter per part. Panel range 0 Hz – 20 kHz (exceeds Nyquist at
  Fs = 39.0625 kHz), Resonance 0–100.
- **Slope: UNCONFIRMED.** Neither the manual, the MIDI implementation, SOS nor any other located
  source states 12 or 24 dB/oct. (Do not infer from the MS2000, which is switchable 12/24; the
  MS2000 relationship is itself anecdotal.)
- Self-oscillates: SOS — "although it can be persuaded to self-oscillate, when doing so it displays
  some uneven peaks and troughs while sweeping". Manual warns high resonance "may be distorted depending
  on the cutoff frequency or on the pitch that you play" (suggests internal clipping at high resonance).
- Character (SOS): "a good approximation of an analogue low-pass type that sounds more 'Japanese' than
  American", "more Korg/Roland than ARP/Moog", "not a bad filter".
- Audio In goes through the filter, so the EA-1 is marketed as a "filter bank with powerful
  resonance" (manual p. 6).

### 2.5 Envelope and amplitude (gate vs. trigger)

- There is exactly **one envelope per part** and it modulates **cutoff only**. It is a one-stage
  **decay** envelope: instantaneous attack at trigger, then exponential/linear (shape **UNCONFIRMED**)
  decay to zero over 0 ms – 10 s. EG Int sets depth and polarity (–100…+100, centre = none).
- The amplifier has **no envelope**: SOS — "the amp has no envelope. It is simply on or off, which
  causes a slight blip at the start of each sound." The note is on for the step's **gate time**, then
  off. So the amp is a pure gate; the "decay" knob never shortens notes — gate time does.
- Retrigger rule (manual p. 25 and p. 28): "If the gate time length of a step extends beyond the point
  at which the next step is sounded, the EG will not be re-triggered for the next step." I.e. overlapping
  gates = legato: the pitch changes (through portamento if set) but the filter EG keeps decaying. This is
  how 303-style slides/accents are approximated: tie (gate ≥ step) + portamento → slide without
  re-attack; separate gates → re-attack.
- Pseudo-attack: negative EG Int with a long Decay (manual, SOS).
- Velocity: none. Keyboard keys and the sequencer always send/use velocity 100 (0x64); incoming velocity
  is ignored. Accent: none. Level changes must be done with motion sequences (sequencer.de).

### 2.6 Amp, distortion

- Level 0–100 (0–127 internal) per part.
- Distortion: on/off only; the NRPN table labels Level as "Distortion Level", so Level is the drive
  into the distortion stage and also the output level. Manual: "produce a hard sound even from a
  mellow waveform. It is highly effective to raise the resonance and use distortion." SOS: "could hardly
  be described as subtle." Audiofanzine users: "frighteningly" harsh and "not adjustable".
- Distortion precedes the chorus/flanger and delay (panel block diagram: Amp → Chorus/Flanger → Delay),
  so the "famous" sound of a resonant, distorted line feeding a tempo delay is simply the default
  topology — the delay repeats are of the already-distorted signal and are clean (the feedback path is
  inside the delay, after distortion).

### 2.7 Effects (per part, independent on each part)

Both effects store their own Depth and Time (SysEx bytes 10–13 of each part: Delay Depth, Delay Time,
Chorus Depth, Chorus Time). The Type Select key only picks which pair the two knobs edit. The panel
diagram shows Chorus/Flanger and Delay in series; both being active simultaneously is strongly implied
by the data format and the "3 Effects per part" wording, though the manual never states it in words.

Tempo Delay

- Depth 0–100: "the depth of the delay and the amount of feedback (the number of delay repeats)" —
  one knob sets both wet level and feedback. "Raising the Depth excessively may cause the sound to
  distort" (feedback can exceed unity / clip).
- Time: 16 quantised tempo-synced values expressed in **steps** (a step is a 16th at the default
  scale): **1/4, 1/3, 1/2, 2/3, 3/4, 1, 1.33, 1.5, 2, 2.5, 3, 4, 5, 6, 7, 8**. Synchronises to the
  pattern tempo, and to external MIDI clock when Clock = Ext.
- Memory limit: "Depending on the tempo setting, it may be impossible to set the delay time. In such
  cases, set the delay time to half the desired value." SOS: at minimum time it "behaves like a
  flanger"; at maximum "delays of a second or more"; the maximum reachable time shortens at high
  tempos. (Example: 8 steps at 120 BPM = 1.0 s; at 20 BPM it would be 6 s, which the 4 Mbit DRAM cannot
  hold, hence the clamp.)
- Timbre: no explicit "lo-fi" statement in primary sources; getlofi says it sounds virtually identical
  to the MS2000 delay; SampleScience's EA-1 sample library markets the overall EA-1 sound as "lofi"
  and "gritty" (https://rekkerd.org/?p=92093). Whether the delay line has reduced bandwidth is
  **UNCONFIRMED**; a plain digital delay at 39 kHz with feedback saturation is the defensible model.
- Each part's delay is separate, enabling "syncopated echoes" (SOS).

Chorus/Flanger

- Depth 0–100: chorus at low settings, flanger (feedback added) at high; can distort when excessive.
- Time = LFO rate **0.2 Hz … 5,000 Hz**: the modulation LFO runs into audio rate; SOS calls the
  results "beautifully crazed and over-the-top". Delay-line length and waveform of the LFO
  **UNCONFIRMED**.

### 2.8 Audio In

- OSC1 = Audio In replaces OSC1 with the external signal (mono, line level). Global Input Gain
  0–100. Audio In Thru key bypasses everything.
- The external signal then passes through OSC Mod (as OSC1 in Ring/Deci), the filter, distortion and
  effects, and the amp **gate** — Korg's German product page describes external audio being "routed
  through the filters and effects and triggered by the sequencer", i.e. the sequencer's triggers gate
  and envelope the incoming audio. Each part can have its own treatment (SOS).
- Factory patterns whose style carries an 'A' use Audio In for OSC1 (e.g. A05, A09, A42, A63, B01, B09,
  B40, C59, C60).

### 2.9 Tune / keyboard / misc

- There is no master tune parameter. Global pitch control is the live Transpose (–24…+24, not saved)
  and, in songs, a per-position Pitch Offset (–24…+24).
- Pitch bend is received (not sent) with range ±12 semitones ("on some sounds it may not be possible
  to bend the pitch upward a full octave").
- MIDI note range: notes 0–127 received; the sequencer stores pitches C-1…G9.
- Stuck notes: Shift + Stop/Cancel performs a MIDI reset.

### 2.10 Original EA-1 vs EA-1 mkII (engine)

- Confirmed difference: OSC Mod slot 3 is **Decimator** on the original and **Cross Modulation** on
  the mkII (Korg MIDI implementation files; Synthtopia launch report
  https://www.synthtopia.com/?p=599 calls cross modulation "new"). Retail copy for the mkII: "the
  frequency (pitch) of oscillator 1 modulate[s] the waveform of oscillator 2".
- Otherwise the two MIDI implementations are byte-for-byte identical (same NRPNs, same pattern SysEx
  layout, same 256 patterns / 16 songs / 20–300 BPM), supporting owner reports (Audiofanzine forums,
  sequencer.de) that the mkII is the same engine in the new X-series-style case with different knobs.
- The sequencer.de claim that mk2 Electribes "replace the wah with a mod delay" refers to the
  ES-1/ER-1 mkII, not the EA-1, which never had a wah.

---

## 3. Sequencer

Sources: Owner's Manual ch. 3–5 (pp. 12–38), Specifications p. 45; MIDI implementation TABLE2/3/6/7;
SOS; EasyStart.

### 3.1 Pattern structure

- 256 patterns, four banks of 64: A01–A64, b01–b64, C01–C64, d01–d64.
- A pattern = tempo + length + beat/scale + swing + two Parts. Each Part = synth settings (14 bytes) +
  phrase pattern (64 steps of trigger/pitch/gate) + one motion sequence (64 values + type +
  destination). Part data is 208 bytes; pattern data 420 bytes (SysEx TABLE2/3).
- Length 1–4 bars; bar = 16 steps (or 12 in triplet scale). Maximum 64 steps per part.

| Length | 16th / 32nd scale | Triplet scale |
|---|---|---|
| 1 | 16 | 12 |
| 2 | 32 | 24 |
| 3 | 48 | 36 |
| 4 | 64 | 48 |

- Beat/Scale (Shift + step 5/6/7; SysEx value 0–2): **16th note** (16 steps = one 4/4 bar),
  **32nd note** (16 steps = 32nd notes, so a "bar" of 16 steps lasts two beats), **triplet** (12 steps
  of 8th-note triplets, i.e. 3 × 4 per bar; keys 13–16 unused). SOS phrases the same thing as "4/4, 3/4
  or triplet timing" and "12 to 64 steps".
- Length and Beat/Scale cannot be viewed or changed during playback, recording or Pattern Set play.
- The 16 step keys show one bar at a time; the **Select** keys choose the bar (red LEDs) while the
  green LEDs follow the playing bar.
- Writing a length-1 pattern automatically copies bar 1 into bars 2–4 ("Data Copy within a pattern":
  A — — — → A A A A; length 2: A B → A B A B; length 3: A B C → A B C C), so extending the length later
  starts from repeated material. If the length is already 4, nothing is copied.
- Per-step data: Trigger (on/off), Pitch (C-1…G9), Gate Time 0.25…64.0 steps in 0.25-step units
  (SysEx 0–255, or 0–191 = 0.25…48.0 for triplet). Gate > 1.0 overlaps the next step (tie / legato, no
  EG retrigger). Clear Part resets pitches to C4 and gates to 1.00. There is **no per-step accent,
  velocity, roll/flam or slide flag**; slides are done with gate overlap + portamento.
- Tempo 20.0–300.0 BPM (stored per pattern; SysEx 2 bytes). Tempo changes made while playing revert
  unless written. When switching patterns during playback the previous tempo is kept; stop and restart
  to adopt the new pattern's tempo.
- Swing 50–75 % (Shift + step 8; SysEx 0–25), delays **even-numbered steps**; 50 = straight, 66 ≈
  shuffle; no effect in triplet scale; cannot be edited during playback.

### 3.2 Recording / editing

- **Realtime recording** (default, SOS: "easy to use in the extreme"): Keyboard on → Rec → Play,
  loop-records step keys into the selected part; quantised to the step grid (no sub-step resolution
  is stored, see data format). Shift + Rec held = erase triggers of the selected part as it passes
  (pitch and gate untouched). Metronome options oFF / r-0 (only while recording) / r-1 (1-bar count-in)
  / r-2 (2-bar count-in) / on, quarter notes.
- **Step recording**: matrix row "Step Rec" shows the target step (1.01…4.16); Rec; press keyboard
  keys to enter pitch + trigger and auto-advance; TAP = rest; hold key + TAP = tie; dial moves the
  target step; Shift + step key jumps to it; Shift + Select steps by one. Gate time cannot be set
  during step record (fix it with event editing).
- **Event editing**: with Keyboard off, the step keys toggle triggers; the Pitch and Gate Time matrix
  rows let you select a step (it blinks, and auditions) and dial its value.
- Utilities (Shift + step key, playback stopped): Move Data –16…+16 steps with wrap-around; Pitch
  Shift –24…+24 per part or both; Copy Part (sound + phrase + motion from any pattern/part); Clear
  Part; Clear Motion.
- Transpose key: live transposition –24…+24 of the whole pattern, shown while held, not saved.
- Write: stop first; Write → choose destination → Write. Memory Protect (Global) defaults **on**.
  Edits are discarded when another pattern is selected without writing.

### 3.3 Motion sequence

- Per part: **one** knob or switch, 64 values (one per step), recorded in real time over one full
  pattern cycle after the first knob movement, then recording auto-stops. Recording another knob on the
  same part replaces the previous motion. Cannot be edited after the fact, only cleared/re-recorded.
- Recordable destinations (SysEx "ds" table): Portamento, Osc Balance, Osc2 Pitch, Osc1 Wave,
  Osc2 Wave, Osc Mode, Distortion on/off, Level, Cutoff, Resonance, EG Int, EG Decay, Delay Depth,
  Delay Time, Chorus Depth, Chorus Time — i.e. everything in the Synthesizer section except the
  Motion Seq key itself and Part Select.
- Playback types (Motion Seq key): **Off** (data retained but ignored), **Smooth** ("knob values will
  be connected smoothly" — interpolate between the 64 stored values), **Trig Hold** ("the value of the
  motion sequence knob will be held at the note timing of that part" — sample the motion value when the
  part triggers and hold it until the next trigger; SOS: "quantises to the nearest 16th").
- Stored values 0–127; MSB set = "no data at this step". Spec sheet: "one parameter for each part,
  64 events".
- The EasyStart's line "Two knob rotations can be recorded for the Delay Motion Sequence" and its
  "Reverse key" step are ER-1 text pasted into the EA-1 guide; the EA-1 manual and data format allow
  exactly one destination per part.

### 3.4 Keyboard function

- 16 step keys become a 16-note keyboard (A…C), 6 octave positions A0…C7 (table in 1.4); Part
  Select picks which part sounds; velocity fixed at 100; monophonic per part.
- Pressing the keys transmits Note On/Off on the part's MIDI channel.

### 3.5 Mute / solo

- The original EA-1 has **no dedicated mute or solo keys**. Parts are silenced by turning Level
  down, by clearing/turning off triggers, or by Shift+Rec erase while playing. (sequencer.de notes
  "mutes are ignored when switching patterns" for the Electribe family; for the EA-1 this is moot.)

### 3.6 Pattern change, chaining, Pattern Set

- Selecting a new pattern (dial, Pattern Set key, MIDI Program Change) while playing takes effect
  "when the currently playing pattern finishes its last step"; the display blinks until then. The
  previous tempo is retained across the change.
- There is no "next pattern" field in the pattern data; chaining is done live (dial or Pattern Set)
  or by a Song.
- **Pattern Set**: 4 groups × 16 keys = 64 registered patterns (stored in Global data, bytes 64–127).
  Hold Keyboard key + step key to recall; Keyboard + Select to change group (red LEDs); Shift +
  Keyboard = hold mode (key blinks). Not available while recording. Factory: "the first pattern of each
  style is assigned to step keys 1…16".
- Reset & Play (Shift + Play) restarts from step 1; "By using this function in conjunction with
  setting the tempo by the Tap key, you can synchronize the playback without using MIDI" (DJ-style
  beat-matching to vinyl).

### 3.7 Songs

- 16 songs (S01–S16), each up to **256 positions**; each position = pattern number + Pitch Offset
  (–24…+24). Song tempo 20–300.
- Event recording (Song mode Rec + Play): records keyboard performance (note, length in steps +
  ticks, tie-to-next-measure flag, part), knob movements of the selected part, and tempo; events are
  time-stamped by position / measure (1–4) / step (1–16) plus a tick count for note length. Replace
  (not overdub) recording. Max ≈ **65,500 events** total. Song events play back as "Smooth" motion.
  Manual caveat: "After rewinding a song, it may not be possible to playback exactly according to the
  event data." SOS found realtime event recording unreliable and recommends step entry of positions.
- Insert Pattern / Delete Pattern / Clear Event / Clear Song utilities (Shift + step 13–16). Select
  keys = rewind / fast-forward; song cannot be switched during playback (queued instead).
- Song playback stops automatically at End.

### 3.8 Tap tempo and sync

- Tap: 3 or more taps, stopped or playing. External: MIDI Clock in (Global Clock = Ext; falls back to
  internal if no clock arrives; tempo knob disabled while slaved) or out (Clock = int). Start/Continue/
  Stop and Song Position Pointer / Song Select handled in Song mode. A Start message received while
  already slaved restarts the current pattern from its beginning.

---

## 4. MIDI

Sources: `EA1_MIDIimp.TXT` (Korg, in the zip above); Owner's Manual pp. 41–43 and MIDI
Implementation Chart p. 51 (dated 1999.2.3); Korg FAQ PDF.

### 4.1 Channels and basic behaviour

- Part 1 channel "MIDI ch" 1–16 (default 1), Part 2 "P2 ch" 1–16 (default 2); same channel for
  transmit and receive. Program Change, Bank Select, Song Select and SysEx use the part 1 channel. If
  both parts share a channel, both sound together.
- Default mode 3 (Omni off, Poly) per the chart; each part is mono in practice.
- Note On 9n kk vv (vv 1–127 received, any value plays at full level; transmitted vv fixed at 0x64),
  Note Off 8n or 9n with vv = 0. Note-off velocity not sent.
- Velocity: ignored (× in chart). Aftertouch: no. Pitch Bend: received only, range per Bend Range
  (–12…+12). Modulation wheel (CC1): not recognised.
- CC 121 Reset All Controllers, 123 All Notes Off; 122/124/125 treated as All Notes Off.
- Active Sensing sent and received.
- MIDI Filter: P (Program/Bank/Song Select), C (control change = NRPN/Data Entry), E (SysEx) each
  on/off. SysEx always works on the Dump page regardless of E.

### 4.2 Pattern / song selection

- Bank Select MSB CC0 = 0; LSB CC32 = 0 → Program 0–127 = A01…b64; LSB = 1 → Program 0–127 =
  C01…d64. A Program Change alone stays within the current 128-pattern group. Transmitted when
  patterns are switched on the panel (Pattern mode only).
- Song Select F3 ss (ss 0–15) and Song Position Pointer F2 in Song mode (SPP sent only when stopped
  and the position is changed; because pattern lengths differ, master and slave are "not necessarily
  in the same location").
- Realtime: F8 clock, FA start, FB continue, FC stop (sent when Clock = int, received when Ext).

### 4.3 NRPN map (official; TABLE1 of the MIDI implementation)

Send CC99 (NRPN MSB) = 3, CC98 (NRPN LSB) = parameter, then CC6 (Data Entry MSB) = value. Only the
MSB of Data Entry is used (128 steps; CC38 ignored). All panel knobs and keys except Motion Seq
transmit these when moved; received values edit the current pattern's part.

| NRPN LSB Part 1 | NRPN LSB Part 2 | Parameter | Data Entry value |
|---|---|---|---|
| 0x00 | 0x20 | Portamento | 0–127 |
| 0x01 | 0x21 | Osc Balance | 0–127 |
| 0x02 | 0x22 | Osc2 Pitch (offset) | 0–127 |
| 0x03 | 0x23 | Osc1 Wave | 0 Saw, 1 Square, 2 Tri, 3 Audio In |
| 0x04 | 0x24 | Osc2 Wave | 0 Saw, 1 Square, 2 Tri |
| 0x05 | 0x25 | Osc Mode | 0 Off, 1 Ring, 2 Sync, 3 Deci (mkII: 3 = Cross) |
| 0x06 | 0x26 | Distortion | 0–63 Off, 64–127 On |
| 0x07 | 0x27 | "Distortion Level" (= Level) | 0–127 |
| 0x08 | 0x28 | Cutoff | 0–127 |
| 0x09 | 0x29 | Resonance | 0–127 |
| 0x0A | 0x2A | EG Int | 0–127 (64 = centre/none) |
| 0x0B | 0x2B | EG Decay | 0–127 |
| 0x0C | 0x2C | Delay Depth | 0–127 |
| 0x0D | 0x2D | Delay Time | 0–127 (quantised to the 16 multiples) |
| 0x0E | 0x2E | Chorus Depth | 0–127 |
| 0x0F | 0x2F | Chorus Time | 0–127 |
| 0x10 | 0x30 | Motion Seq Type | 0 Off, 1 Smooth, 2 TrigHold |
| 0x41 | – | Output Gain | 0–100 |
| – | – | Tempo | song event only (destination 0x44) |

Note: because each part has its own channel, Korg's FAQ says the Part 1 numbers are valid for both
parts when addressed on each part's own channel; the Part 2 numbers (0x20–0x30) are the values used
inside motion-sequence / song-event data and when both parts share one channel. The FAQ's enumeration
of wave/mode values differs from the MIDI implementation (it lists Triangle first and omits "Off");
the MIDI implementation document is taken as correct here. A 2003 MIDIbox forum post
(https://forum.midibox.org/t/big-tips-for-controlling-ext-machine/831) independently reports the
CC99=3 / CC98=n / CC6=v scheme and that Part 2 numbers start at 0x20.

### 4.4 System Exclusive

- Header `F0 42 3n 52` (n = global channel 0–15; family ID 0x52 = EA-1; the manual's "About MIDI"
  text prints `51`, which contradicts both the implementation file and the Device Inquiry reply —
  treat `52` as correct). Device Inquiry (F0 7E nn 06 01 F7) is answered.
- Dump requests: 10 current pattern, 1C all patterns, 0A current song, 0B all songs, 0F all data;
  write requests 11 (pattern, with destination bank bit + number) and 1A (song). Dumps: 40 current
  pattern, 4C all patterns, 58 current song, 57 all songs, 50 all data. Replies: 21 write completed,
  22 write error, 23 load completed, 24 load error, 26 format error.
- Data is 8-bit → 7-bit packed in 7-byte groups (high bits collected into a leading byte).
- Pattern dump layout (TABLE2/3): tempo (2 bytes), scale/beat + length byte, swing byte, then two
  208-byte parts: 14 synth bytes, 64 note numbers (MSB = trigger off), 64 gate times, 64 motion
  values, motion type, motion destination. All patterns = 256 × 420 bytes = 107,520 bytes before
  packing. Global data = 128 bytes (metronome, input gain, bend range, clock, 64 Pattern Set slots).
- Dump from the panel: MIDI mode → Dump → Ptn / SnG / ALL → Play. Reception requires Protect off.

---

## 5. Sound character notes

Sources: SOS review; Vintage Synth Explorer; Synthtopia; Audiofanzine EA-1 and EA-1 mkII user reviews;
getlofi.com; rekkerd.org (SampleScience "EA-1 Synthesis" library); Korg marketing via VSE;
MPC-Forums thread summarised in search results (https://www.mpc-forums.com/viewtopic.php?p=143598,
page itself not retrievable); theprodigy.info gear page; YouTube titles listed below.

### 5.1 What reviewers say

- Oscillators: SOS — raw waveforms "vaguely samey"; the character comes from OSC Mod, filter and
  effects. Sweetwater (1999 marketing) — "startlingly accurate analog sawtooth, square, and triangle
  waveforms". Audiofanzine users: "the two osc mods are poor" (one wishes for FM), "a little cold
  sometimes", "not really fat, not always clear", "remains far from the analog"; the mkII reviewer
  instead calls it "warm, round / fat … certainly not a Virus".
- Decimator: SOS — "a down-sampled, grungy edginess"; this is the one deliberately aliasing element.
- Filter: SOS — "Japanese" rather than American, Korg/Roland-ish, self-oscillates unevenly while
  sweeping; "the typical 'meow-ing'" (Korg). Audiofanzine: a single low-pass, "correct", "can go down
  low"; one user criticises the resonance. Nobody describes it as creamy; the consensus is a clean,
  slightly thin VA low-pass that gets aggressive with resonance + distortion.
- Acid: Korg's own factory list includes "Acid Tek 1/2" and "Classic Aceeed"; SOS praises "wonderfully
  deep and powerful bass lines". One Audiofanzine owner insists it sounds "nothing to do with a 303";
  a YouTube demo below is titled "Acid Demo". Expect 303-adjacent sequences but a brighter, more
  digital-edged resonance than a 303, with no accent and no true slide circuit.
- Distortion: universally described as blunt — SOS "could hardly be described as subtle",
  Audiofanzine "frighteningly" bad / "not adjustable". The manual itself pushes resonance + distortion
  as the signature move.
- Effects: SOS — "fun but erratic"; chorus/flanger at audio-rate LFO "beautifully crazed and
  over-the-top"; tempo delay useful, "syncopated echoes" with two independent delays. getlofi: delay
  "virtually identical" to the MS2000's. SampleScience markets the sampled EA-1 as "gritty synths" and
  "lofi FXs" suited to minimal techno and IDM.
- Digital edge / aliasing: no source measures it. Given Fs = 39.0625 kHz, a 20 kHz cutoff legend, and
  the decimator, some aliasing on bright saws at high pitches is plausible but **UNCONFIRMED**; the
  "cold"/"thin" comments are the closest evidence.
- Overall verdicts: SOS — "a very capable, great-sounding synth/sequencer combination", "close to the
  cutting edge" rather than vintage; cons "limited synth parameters, no LFO, slightly fiddly controls,
  mains-only". Synthtopia (2003) — oscillators and filters "less punchy and full than traditional
  analog synths" but "interesting in its own right"; "not a replacement for a full analog synth".
  Audiofanzine: "cool for the sequence", "trancey", "electronic music in general", used live. MPC-Forums
  (2004): one poster "worst Electribe ever", another "pretty good". AudioPilz's 2020 "Bad Gear" episode
  title asks whether it is the "Worst Of The Tribe".
- Lo-fi / drone use: several YouTube uploads use it as a noise/drone machine via audio-rate
  chorus LFO, self-oscillating filter, decimator and delay feedback.

### 5.2 Genres and notable users

- Korg's factory styles: Techno/Trance/Acid (bank A), House, Garage/2-Step, Hip-Hop/R&B, Big Beat
  (bank B), Drum'n'Bass, Trip-Hop, "Other" (reggae, bossa), "Rhythmic", sound effects (bank C).
- Vintage Synth Explorer lists **Orbital, Crystal Distortion, Cirrus** as users. theprodigy.info
  lists the EA-1 among Liam Howlett's gear (no track attribution given). Korg marketing: "the
  phattest, most aggressive analog synth sounds possible". VSE: "known for the aggressive, acidic power
  of its basses and leads" (a seller's phrase relayed in search results).

### 5.3 YouTube audio references (titles and URLs located via search; descriptions could not be
fetched, and whether a given video uses the original or the mkII was only verifiable from titles)

1. "Bad Gear - Korg Electribe EA-1 - Worst Of The Tribe???" — AudioPilz (May 2020), ~5 min of
   knob-twisting plus commentary: https://www.youtube.com/watch?v=aiisa8flZ2s
2. "KORG EA-1 Electribe - Factory Presets Demo" — walks through factory patterns:
   https://www.youtube.com/watch?v=VO98LSV_N_w
3. "KORG Electribe EA-1 Acid Demo - raw, no talky" — unprocessed acid-style sequences:
   https://www.youtube.com/watch?v=MZlEVmtt804
4. "Korg Electribe EA-1 Factory Demo" — the built-in demo songs (description mentions song 6
   "Elect-Rave"): https://www.youtube.com/watch?v=oPq7WtFw6Rk
5. "Korg Electribe EA-1 The Ultimate Drone/Noise Machine?" — live single-voice drone/noise, no
   external processing: https://www.youtube.com/watch?v=6729Y-a9bNU
6. "Korg Electribe EA-1 as an effect" — Audio In / filter-bank use in a live rig:
   https://www.youtube.com/watch?v=_qB_srDIo4s
7. "Electribe EA-1 & ER-1 Demo" — the 1999 pair synced together:
   https://www.youtube.com/watch?v=fxR-WOWSu3w

(Excluded as explicitly mkII: "Jamuary 2019, Day 4: Acidic Beats (Electribe EA-1 mkII …)".)

---

## 6. Factory sound (organisation only, no pattern data)

Sources: Owner's Manual p. 50 "Pattern Name List" / "Song Name List"; EasyStart; SOS.

- 256 slots; banks A, B and C (192 patterns) are preset material; **bank D holds the patterns used
  by the demo songs plus initial (empty) data** — EasyStart treats d.01–d.26 as content and suggests
  d.28 onward (and d.63/d.64) as empty scratch slots. SOS counts "192 preset patterns".
- Each list entry carries a Style tag and a tempo. Patterns tagged with an 'A' use Audio In for OSC1.
- Bank A (A01–A64): entirely **Techno** in the broad 1999 sense — Goa/psy-trance, Detroit, minimal,
  acid ("Acid Tek", "Classic Aceeed"), electro, "Komputer Musik", bleepy sequences and arpeggio
  lines; tempos mostly 120–145 (range 100–160).
- Bank B (B01–B64): **House** (B01–B21, incl. ambient and house-chord stabs, 120–140),
  **Garage / Speed Garage / 2-Step / organ bass** (B22–B34, 130–134), **Hip-Hop / R&B** (B35–B49,
  80–110), **Big Beat / Rock** (B50–B62, 96–136), and two **Drum'n'Bass** "Jump Up" patterns
  (B63–B64, 166–170).
- Bank C (C01–C64): **Drum'n'Bass** (C01–C16, 153–185, incl. tech-step and jump-up), **Trip Hop**
  (C17–C25, 80–90), **Other** (C26–C37: neo-bossa, lounge, reggae/ragga, 75–163), **Rhythmic**
  (C38–C56: tribal, percussive, odd-ball sequences, 88–180), **S.E.** sound effects (C57–C64, 80–140,
  two of them Audio In).
- Overall tempo span in the factory list: 75–185 BPM.
- Pattern Set factory assignment: the first pattern of each style sits on step keys 1–16.
- Songs: S01–S07 are demo songs (two "Electribe Remix" parts, "STATION X Troyster Mix", "Bass Burn",
  "Macrocaustic", "Elect-Rave", "Dugan's Midnight X-press"); S08–S16 blank.
- The manual's "Example sounds" page (p. 46) gives knob diagrams for twelve starting points: Saw Bass,
  Sub Bass, Octave Bass, Detuned Saw, Distortion Lead/Bass, Pulse Width Mod, Sync Lead, Ring Mod,
  Decimator, 5th Pad, Deep Flange, Beam — a good checklist of what the engine is expected to do
  (the knob positions are graphics and were not transcribed).
- Factory reset: hold Transpose + Write at power-on, display `PLd`, press Play; ~15 s.

---

## 7. Open questions (not settled by any located source)

1. Filter slope (12 vs 24 dB/oct) and topology; resonance-vs-cutoff gain compensation.
2. Decay envelope curve (linear vs exponential) and whether EG Int scales in Hz or octaves.
3. Note priority within a part (last vs low/high) for overlapping MIDI notes.
4. Portamento law (constant time vs constant rate) and whether it is legato-only.
5. OSC2 Pitch Ofs knob law across –12…+48 semitones in 128 steps; location of the detent.
6. Whether Chorus/Flanger and Tempo Delay run simultaneously (strongly implied; not stated).
7. Delay-line bandwidth / any deliberate lo-fi colouring; chorus delay-line length and LFO shape.
8. Exact waveform differences between OSC1 and OSC2 ("slightly heavier fundamental" on OSC1).
9. Number of TMS57070 DSPs (one per block diagram OCR, two per getlofi teardown).
10. Whether the Gearspace threads (403 to automated fetch) contain measured aliasing/filter data.
