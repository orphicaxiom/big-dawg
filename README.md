# BIG DAWG

A free all-in-one lo-fi guitar tone plugin. AU, VST3, Standalone. macOS.

Started as a Mac DeMarco emulator. Became something bigger.

## What it does

BIG DAWG is a signal chain of effects tuned for bedroom-pop, lo-fi, slowcore, and shoegaze guitar tones. It ships with presets modeled after specific artists and recordings.

Signal chain:

```
INPUT -> DRIVE -> DETUNE -> BITCRUSH -> CHORUS/FLANGER -> VIBRATO -> WOW/FLUTTER -> REVERB (SPRING/SLAP/PLATE) -> OUT
```

Nonlinear stages (Drive, Detune, Bitcrush, Chorus/Flanger) run at 2x oversampling to reduce aliasing. Everything else runs at base rate.

## Presets

- **VICEROY** - Mac DeMarco, Salad Days era. Teisco through a Vibro Champ, Alesis rack chorus, Fostex tape wow.
- **ELVIS (THE SAD ONE)** - Mat Cothran, Coma Cinema and Elvis Depressedly. Handheld tape recorder character, lightly overdriven, warbly.
- **PALTH** - Daniel Johann Lines, salvia palth, melanchole. The core move is cranking drive until the signal clips. Wide slow chorus, plate reverb soaked.
- **INHALANT** - Duster, Stratosphere adjacent. Not a true Duster recreation (the real recipe needs tape delay and tremolo, not yet implemented) but a slowcore atmosphere approximation.
- **BASELINE** - Everything off. Reset reference.

## Status

Alpha. Scaffold complete. DSP is stubbed (pass-through) across all stages. Preset values load correctly; actual DSP voicing is pending. The plugin loads in Logic Pro, Ableton, and Standalone, and the UI is functional.

Roadmap (rough, not committed):

- Voice the DSP stages starting with Drive, Detune, Chorus
- Tremolo and Delay stages in an expanding drawer
- Character toggle on Drive (tube vs digital clip) for PALTH accuracy
- More presets

## Build

Requires CMake, Xcode command line tools, and JUCE (expected at a sibling path; see `CMakeLists.txt`).

```bash
cd demarco-vst
cmake -B build -G "Unix Makefiles"
cmake --build build --config Debug
```

Outputs installed to:

- `~/Library/Audio/Plug-Ins/Components/BigDawg.component` (AU)
- `~/Library/Audio/Plug-Ins/VST3/BigDawg.vst3` (VST3)
- `build/DemarcoTone_artefacts/Standalone/BigDawg.app` (Standalone)

Known quirks:

- Project paths with apostrophes break JUCE's VST3 manifest helper. Use a symlink without special characters.
- Product name is `BigDawg` (no space) because JUCE's VST3 post-link chokes on spaces. The UI wordmark paints `BIG DAWG.` as literal text.

## Design

Swiss-brutalist UI. Archivo Black wordmark, IBM Plex Mono for all labels and values. Palette is pale sage background, near-black rules and text, Olivetti red as single accent, cyan reserved for modulation indicators.

Reference: Vignelli, Muller-Brockmann, Olivetti.

## Credits

Built by James Johnson (Local Praxis, Port Aransas TX) with Claude Code.

Preset research drew on interviews, gear rundowns, and direct conversation with Daniel Johann Lines (salvia palth) on how melanchole was actually recorded.

## License

TBD. Probably MIT or similar permissive. Until then, consider it source-available for personal use.
