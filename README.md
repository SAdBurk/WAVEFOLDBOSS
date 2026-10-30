# Wave Folding Boss

A pink-leopard, scene-core wavefolder plugin. Wavefolding only: no filters, EQ or other processing.
The classic distorted-drum fold sound lives here.

**Formats:** VST3 (Windows + macOS), AU (macOS), Standalone

## Controls

| Knob | What it does |
|---|---|
| **Fold** | Drive into the folder, 0-40 dB. More drive = more folds = more harmonics. |
| **Stages** | 1-4 cascaded fold stages. Each extra stage re-amplifies and folds again for harsher, denser fold patterns. |
| **Shape** | 0% = hard triangle fold, 100% = smooth sine fold. |
| **Symmetry** | Shifts the signal before folding. Creates even harmonics (a DC blocker removes the resulting offset). |
| **Mix** | Dry/wet, latency-compensated. |
| **Output** | Output level, -24 to +6 dB. |

Double-click any knob to reset it. The screen shows the live transfer curve and a folded sine.

Internals: 4x oversampling (linear-phase half-band FIR) to keep aliasing down, parameter smoothing,
reported latency for host delay compensation.

## Getting the Windows and macOS builds

Plugins can only be compiled on their own OS, so this repo builds both automatically:

1. Push this folder to a GitHub repository.
2. Open the **Actions** tab, run **Build Wave Folding Boss** (it also runs on every push).
3. Download `WaveFoldingBoss-Windows` and `WaveFoldingBoss-macOS` from the finished run.

### Installing

- **Windows:** copy `Wave Folding Boss.vst3` to `C:\Program Files\Common Files\VST3\`.
- **macOS:** copy `Wave Folding Boss.vst3` to `/Library/Audio/Plug-Ins/VST3/` (and `.component` to `/Library/Audio/Plug-Ins/Components/`).
  The builds are not notarized, so clear the quarantine flag once:
  `xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/Wave Folding Boss.vst3"`

## Building locally

Needs CMake 3.22+ and a C++17 compiler (Visual Studio 2022 or Xcode).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Output lands in `build/WaveFoldingBoss_artefacts/Release/`.
Add `-DWFB_COPY_AFTER_BUILD=ON` to copy plugins into your system plugin folders automatically.

## About "VST" (VST2)

Steinberg ended VST2 and no longer licenses its SDK, so this project targets VST3, which every current DAW supports.
