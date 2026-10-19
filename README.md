# fliptribe

A sound-and-workflow recreation of the **Korg Electribe·A EA-1** (the original 1999 silver one) for the
Retroid Pocket Flip 2 running ROCKNIX. Native C++17 on SDL2, drawn as a skeuomorphic panel for the 1920x1080
screen, playable with the touchscreen, the built-in pad, or both at once.

It is not an emulator of the EA-1's firmware (there is no ROM dump to run); it is a new synth engine built from
the owner's manual, the MIDI implementation, the service-manual block diagram and the 1999 reviews, collected in
[docs/ea1-reference.md](docs/ea1-reference.md). The goal is "sounds like one and works like one".

## What is modelled

- Two monophonic parts, each: OSC1 + OSC2 (saw / square / triangle; OSC1 with the heavier fundamental the
  manual describes), OSC Mod **Off / Ring / Sync / Deci** (decimator, as on the original; the mkII's Cross is
  not here), OSC Balance with the mod output on the OSC2 side, OSC2 Pitch Offset -1 .. +4 octaves, always-on
  Portamento, one resonant low-pass filter that self-oscillates, **one decay envelope on cutoff only**
  (bipolar EG Int, 0 ms .. 10 s), an amp that is a pure gate, the blunt on/off Distortion with Level as its
  drive, and per part a Chorus/Flanger (LFO 0.2 Hz .. 5 kHz) into a Tempo Delay (16 step-synced times, one
  Depth knob for wet and feedback).
- Gate times of 0.25 .. 64 steps; a gate that reaches into the next step plays **legato without retriggering
  the envelope**, which with portamento is how the EA-1 does slides.
- 256 patterns (A01 .. d64) of 1 .. 4 bars, 16th / 32nd / triplet scales (12 steps per bar in triplet), tempo
  20 .. 300, swing 50 .. 75 % on even steps, motion sequences (Smooth or Trig Hold), live Transpose, tap tempo,
  the five-row parameter matrix (Pattern / Tempo / Step Rec / Pitch / Gate Time) with cursor keys and dial,
  WRITE with a destination, and the Shift + step-key functions (length, beat, swing, copy part, clear motion,
  clear part).
- The engine runs at the hardware's **39.0625 kHz** and is resampled by SDL, so the top end has the same
  bandwidth as the real thing.

Not yet: Song mode, Pattern Set, Audio In as OSC1, MIDI (tempo and transport sync between units goes over
Wi-Fi instead, see below), and the hardware's one-motion-lane-per-part limit (fliptribe records one lane per knob).

Twelve original demo patterns ship in `patterns/` (A01 .. A12). They are new material written in the
EA-1 idiom, not transcriptions of Korg's factory set.

## Controls

Touch: drag a knob up/down (hold SHIFT for fine), tap keys. SHIFT latches for one action if you tap it.

| Pad | Function |
|---|---|
| Start | Play / stop (Shift + Start = reset & play) |
| Select | Rec on/off (Shift + Select = quit) |
| D-pad | Move the focus ring between controls; with Shift: up/down = cursor rows, left/right = dial |
| Left stick | Turn the focused knob (Shift = fine) |
| Right stick | Cutoff (vertical) and Resonance (horizontal) of the edit part, always |
| A | Press the focused control (toggle a step, press a key) |
| B | Shift (hold) |
| X | Keyboard function on/off |
| Y | Part select |
| L1 / R1 | Select ◀ / ▶ (bar shown, or octave in keyboard mode; with Shift: target step) |
| L2 | Mute the edit part |
| R2 | Tap tempo |
| L3 | Transpose (then dial or step keys, key 4 = C) |
| R3 | Write |
| Guide | Quit |

Desktop testing (`fliptribe --window`): 1-8 and q-i are the step keys, space play, enter rec, backspace
stop, k keyboard, p part, [ ] select, PageUp/PageDown cursor, , . dial, arrows focus, +/- turn, a press,
f effect type, s Wi-Fi sync mode, Tab tap, Ctrl+W write, Esc quit.

## Wi-Fi sync

fliptribe locks tempo, transport and phase with other units on the same network: more Flip 2s, and the sibling
grooveboxes [HI5](https://github.com/mercurious/hi5), [Jaffedrone](https://github.com/mercurious/jaffedrone)
and Rap Machine, which all speak the same small protocol (UDP broadcast on port **5150**, ten packets a second
carrying tempo, playing state and position in 16th notes). A mixed group elects one leader and everyone else
follows it: Start/Stop, the tempo and the bar position all come from the leader, and followers stay in phase
by trimming their playback speed by up to 3 % (jumping only when more than half a 16th out). 32nd and triplet
scales and swing are mapped onto the shared 16th-note grid, so a triplet pattern here lines up bar for bar with
a straight one on the leader.

The **WI-FI SYNC** key in the sequence-control section steps through the modes; the key shows the mode and
under it the link state (leading or following, peers heard, how far behind the leader in ms). The mode is kept
in the settings, and `--sync OFF|AUTO|LEAD|FOLLOW` sets it from the command line.

| Mode | |
|---|---|
| OFF | neither sends nor listens (default) |
| AUTO | leads if no unit is set to LEAD and its random id is the lowest, otherwise follows |
| LEAD | leads (if two are set to LEAD, the lowest id wins) |
| FOLLOW | never leads |

The Flip 2's Wi-Fi has to be on; the packets stay on the local network. `fliptribe_synctest LEAD|FOLLOW|AUTO
BPM SECONDS [16TH|32ND|TRIPLET [SWING%]]` (CMake target, not built by default) runs the engine headless with the
sync module, to check interop against HI5's or Jaffedrone's synctest on one machine.

## Build and install

```sh
build/build.sh                 # podman, Ubuntu 24.04 container -> dist/fliptribe (glibc 2.39 < ROCKNIX 2.41)
flip2/install.sh flip2-12g-usb # rsync to /storage/apps/fliptribe and write the Ports entry
```

Then Ports > fliptribe in EmulationStation. Written patterns and settings live in
`/storage/apps/fliptribe/data` and survive reinstalls. On the device the launcher is
`/storage/apps/fliptribe/launch.sh`; `launch.sh --play --slot A03` starts a pattern straight away.

Offline tools (no window, no audio device):

```sh
fliptribe --render out.wav --seconds 8 --slot A01   # render a pattern at 39062.5 Hz
fliptribe --screenshot panel.bmp --slot A02         # draw one frame offscreen
python3 tools/make_patterns.py                      # regenerate the shipped demo patterns
```

## Layout of the source

- `src/params.h` the EA-1 parameter table (the per-part list is the hardware's 14-byte synth block) and pattern data.
- `src/dsp.h` oscillators (polyBLEP, hard sync), ladder low-pass, decay EG, gate, glide, decimator, chorus/flanger, tempo delay.
- `src/engine.*` voices, sequencer, motion sequencing, the lock-free command queue and the UI snapshot.
- `src/ui.*` the panel: layout, hit testing, touch / pad / keyboard handling, drawing.
- `src/gfx.*` SDL_Renderer helpers, anti-aliased sprite rasteriser, 7-segment display, text cache.
- `src/sync.*` Wi-Fi sync (the HI5S protocol shared with HI5, Jaffedrone and Rap Machine).
- `src/store.*` JSON pattern files and settings. `docs/ea1-reference.md` the hardware reference.

Fonts: Liberation Sans (SIL Open Font License, see `assets/fonts/LICENSE`). Code: MIT.
