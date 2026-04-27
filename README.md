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

Alpha. Most stages are voiced and audibly shape the signal: Drive, Detune, Chorus, Vibrato, Wow/Flutter, and Spring Reverb. Bitcrush, Slap Delay, Plate Reverb, and the Flanger mode of the Chorus stage are still pass-through stubs awaiting voicing. Presets load correctly, the UI is functional, and the plugin runs in Logic Pro, Ableton, and Standalone.

Voicing is rough first-draft across the board. Expect tuning passes per stage and per preset before anything ships as v1.

Roadmap (rough, not committed):

- Voice the remaining stubbed stages: Bitcrush, Slap Delay, Plate Reverb, Flanger mode
- Tune voicing across the five presets once stage Hz mappings stabilize
- Tremolo and Delay stages in an expanding drawer
- Character toggle on Drive (tube vs digital clip) for PALTH accuracy
- More presets

## Install

For musicians who just want to use the plugin. Download a release, drag two files into your plugin folders, restart your DAW.

1. Go to the [Releases page](https://github.com/orphicaxiom/big-dawg/releases) and download the latest zip.
2. Unzip it.
3. Drag `BigDawg.component` to `~/Library/Audio/Plug-Ins/Components/`.
4. Drag `BigDawg.vst3` to `~/Library/Audio/Plug-Ins/VST3/`.
5. Restart your DAW and rescan plugins.

`~/Library` is hidden in Finder by default. In Finder, hit `Cmd+Shift+G` and paste the path.

### Troubleshooting

The binary is unsigned. macOS Gatekeeper will quarantine it on download and the plugin won't load until you remove the quarantine attribute. Open Terminal and run:

```bash
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/BigDawg.component
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/BigDawg.vst3
```

Restart your DAW after running these.

If the plugin still doesn't appear, try a hard rescan in your DAW (in Logic: delete the plugin cache at `~/Library/Caches/AudioUnitCache/` and restart).

## Build from source

For developers. Requires CMake, Xcode command line tools, and JUCE (expected at a sibling path; see `CMakeLists.txt`).

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
