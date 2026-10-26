# 8cee Boom-8 — Multiband Dynamics

A 4-band compressor voiced for 90s NY boom bap: knocking kicks, cracking snares,
dusty jazz/soul samples, and that sampler grit.

## Signal flow

Input → **Drive** → 4-band split (Linkwitz-Riley 24 dB/oct, phase-compensated) →
per-band compressor → sum → **Grit** → **Mix** (parallel, phase-aligned with the dry signal) → **Output**

| Section | What it does |
|---|---|
| Crossover | Three splits: Low / Low-Mid / Hi-Mid / High. With all bands at 1:1 the plug-in is perfectly flat. |
| Threshold, Ratio, Attack, Release, Gain | Per band. Stereo-linked peak detection, 6 dB soft knee. |
| GR meter | Gain reduction per band, 0–24 dB. |
| Solo / Bypass | Solo to hear a band alone; Bypass leaves the band uncompressed. |
| Drive | Warm, slightly asymmetric saturation before the compressors. |
| Grit | 12-bit / 26 kHz sample-and-hold crunch, blended in. |
| Mix | Parallel ("New York") compression. Dry and wet stay in phase. |
| Output | Final level, ±18 dB. |

## Factory presets

Presets are roughly level-matched so you can A/B fairly.

| Preset | Use it on |
|---|---|
| Init | Flat starting point |
| Dusty Break Bus | Drum bus glue with a little dirt, 75 % parallel |
| Knock Kick | Slow low-band attack lets the kick punch through, then holds it |
| Snare Crack | Pushes the 1–6 kHz crack, keeps the body tight |
| Jazz Sample Glue | Gentle 2:1 on all bands for chopped jazz/soul loops |
| Muted Horns | Tames honky upper mids and fizz on horn samples |
| Filtered Bassline | Heavy low-end control with the top rolled away |
| Hat Tamer | Only touches the highs — de-harsh bright hats |
| SP Crunch | Strong grit + drive for that sampler crunch |
| Basement Tape | Darker, warmer, rounded top end |
| NY Smash | Crushed bands blended in at 40 % — big parallel punch |
| Vinyl Loop Lift | Lifts thump and air on a vinyl loop, scoops the mud |
| Head Nod Vocals | Rap vocals: controls lows, brings presence, de-esses the top |
| Boom Bap Master | Light 1.5–2:1 across the mix |

Presets also show up in your DAW's own program/preset list.

## Download

Go to the repo's front page and click **Releases → Latest build** (right-hand side),
then download the zip for your system:

- **8cee-Boom-8-Windows.zip**
- **8cee-Boom-8-macOS.zip**

Unzip it and copy `8cee Boom-8.vst3` into your VST3 folder:
- Windows: `C:\Program Files\Common Files\VST3`
- macOS: `/Library/Audio/Plug-Ins/VST3` (and the `.component` into `/Library/Audio/Plug-Ins/Components` for Logic)

Then rescan plug-ins in your DAW.

Every push to `main` rebuilds the plug-ins and updates that release automatically
(via `.github/workflows/build.yml`, about 5–15 minutes).

The macOS build is unsigned: if macOS blocks it when your DAW scans it, click **Allow Anyway**
under System Settings → Privacy & Security, then rescan.
