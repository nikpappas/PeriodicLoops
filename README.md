# PeriodicLoop

**A generative MIDI FX plugin built around one idea: invisible orbits.**

Each voice is a dot travelling around a circle at its own rate. When it completes a lap, it fires a MIDI note. Stack multiple voices with different periods and you get naturally evolving polyrhythmic patterns — no step sequencers, no grids, no manual programming.

---

<!-- SNAPSHOT: main UI overview -->
![PeriodicLoop UI](docs/images/overview.png)

---

## What it does

PeriodicLoop sends MIDI notes to your DAW instrument. It produces no audio itself. You point it at a synth, sampler, or drum machine and let the orbits drive it.

- **Polyrhythmic by default** — every voice has an independent period, so complex phasing patterns emerge from simple settings
- **Groups** — organise voices by colour; mute or solo entire groups at once
- **Four modes** — Pro, Melody, Drums, Silica — each with a purpose-built interface
- **CC modulation** — sine, triangle, or sawtooth LFOs sent as MIDI CC, also period-based
- **Host-sync or free-running** — lock to your DAW's transport or run independently in milliseconds
- **Fully state-saved** — every parameter survives DAW project save/reload

---

## Modes

### Pro
Full manual control. Per-voice pitch, period, offset, duration, and channel. No constraints. For when you know exactly what you want.

<!-- SNAPSHOT: Pro mode note list -->
![Pro mode](docs/images/mode_pro.png)

---

### Melody
Scale-constrained melodic voices. Pick a root and scale; snippet functions (trill, mordent, turn, tremolo…) generate multi-note gestures per group. Great for evolving melodic material.

<!-- SNAPSHOT: Melody mode with scale picker -->
![Melody mode](docs/images/mode_melody.png)

---

### Drums
Groups map to drum families (kicks, snares, hats…). Pattern functions — Single, Flam, Drag, Paradiddle, Roll — compute offsets automatically. Channel defaults to 10.

<!-- SNAPSHOT: Drums mode with pattern selector -->
![Drums mode](docs/images/mode_drums.png)

---

### Silica
Fully algorithmic. Set a period and a note count; the plugin distributes N voices evenly across the period and arpeggios up, down, or bounces through a scale. No manual editing — just parameters.

<!-- SNAPSHOT: Silica mode with algorithmic preview -->
![Silica mode](docs/images/mode_silica.png)

---

## Visualisation

The orbital display shows every active voice as a coloured dot moving clockwise around its ring. Voices in the same group share a colour. When a dot completes a lap, a brief flash confirms the trigger. Everything is read-only — the display reflects playback state, not a control surface.

<!-- SNAPSHOT: orbital display close-up -->
![Orbital display](docs/images/orbital.png)

---

## CC Modulation

Add CC lanes alongside your notes. Each CC runs its own oscillator — sine, tri, or saw — at a period you set in beats or seconds. Solo individual CCs, mute them, or dial in depth and offset with the knob panel. The waveform display shows all active CCs scrolling in real time.

<!-- SNAPSHOT: CC panel with waveform display -->
![CC display](docs/images/cc_display.png)

---

## Building

Requires **JUCE 8.x** and **CMake 3.22+**. JUCE must be installed and findable via `find_package`.

```bash
# Configure
cmake -S . -B build

# Release build
cmake --build build --config Release

# Debug build
cmake --build build --config Debug
```

Outputs: **VST3**, **AU**, **Standalone**

---

## Format & platform

| | |
|---|---|
| Type | MIDI FX (no audio busses) |
| Formats | VST3, AU, Standalone |
| Targets | macOS, Windows |
| JUCE | 8.x |
| Standard | C++20 |

---

## Project layout

```
src/
  processor/   PLoops (AudioProcessor), Engine, PluginState
  ui/          PLoopsUi editor + all dumb components
  music/       PeriodicNote, PeriodicCC, scales, GM drum map
  utils/       Platform helpers, time conversion
  logging/     pl_debug / pl_info / pl_error
docs/
  requirements.md   Intended behaviour — source of truth
  architecture.md   Design decisions and constraints
```

---

## Licence

<!-- Add licence here -->
