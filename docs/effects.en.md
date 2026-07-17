# Effects: basic processors and compound recipes

Cursed Drone has four serial FX slots on every sound actor. Version 0.9 separates the chooser into two groups:

- **Basic** — one recognizable signal-processing operation with direct controls;
- **Compound** — a curated drone/ambient recipe where three controls move several internal processes together.

The hardware performance target remains TrimUI Brick. Compound effects therefore reuse the existing delay memory and use lightweight filters, all-pass stages, oscillators and envelopes instead of loading convolution impulses or allocating a full reverb for every FX slot.

## Basic effects

| Effect | Controls | What it does |
| --- | --- | --- |
| Empty | — | Passes the signal unchanged. |
| Drive | Drive | Strong soft saturation. The upper half is intentionally much more destructive than in v0.8. |
| Lowpass | Mix, Cutoff | Darkens the source, from near-sub filtering to an almost open signal. |
| Highpass | Mix, Cutoff | Removes body and can reduce the source to brittle motion and air. |
| Tremolo | Depth, Rate | Slow breathing through audio-rate amplitude modulation. |
| Delay | Mix, Time, Feedback | Cross-feedback stereo delay with a range from short slap to long regenerating trails. |
| Crusher | Crush, Rate | Bit-depth reduction and sample holding. Extreme settings are deliberately severe. |
| Folder | Fold, Depth | Wavefolding for metallic and unstable harmonics. |
| Ring Mod | Mix, Rate | Very slow beating through audio-rate ring modulation. |
| Comb | Mix, Size, Resonance | Tuned resonant delay for pipe, string and metallic-body effects. |
| Chorus | Mix, Rate, Depth | Dual modulated short delays with different stereo phases. |
| Flanger | Mix, Rate, Feedback | Very short swept delay with regenerative stereo feedback. |
| Phaser | Mix, Rate, Feedback | Four all-pass stages with a swept frequency and resonant feedback. |
| Diffuser | Mix, Size, Decay | Four incommensurate delay taps and cross-channel feedback, turning events into a dense but economical cloud. |
| AHDR | Depth, Rate, Shape | A cycling Attack–Hold–Decay–Release contour for rhythmic breathing without a note sequencer. |

## Compound effects

Compound controls are **Intensity**, **Character** and **Motion**. They are macros rather than aliases for one hidden parameter.

### Tape Void

Slow stereo wow/chorus → saturation → dark filtering → regenerative delay. Low settings add worn width; high settings smear the source into a moving tape cloud.

### Black Hole

Resonant phaser → low-frequency ring modulation → comb resonance → dark low-pass. Designed for collapsing, inward-moving tones rather than a conventional spacious reverb.

### Ritual Gate

Cycling AHDR contour drives amplitude, cutoff and saturation, then excites a feedback delay. It can create breathing pulses, long ritual swells and broken repeated gates.

### Rust Cloud

Crusher → wavefolder → flanger → comb. Produces corroded digital-metallic motion and is intentionally capable of replacing the source almost completely.

### Deep Sea

Low-pass pressure → slow chorus → shallow tremolo → diffusion. It is the darkest compound recipe and avoids bright decorative shimmer.

## Modulation architecture

Each slot still has four existing modulators, which can target pitch, timbre, colour, motion, texture, level, pan or the amount of any FX slot. Compound effects add **internal macro modulation**: one control can move several internal rates, cutoffs and feedback paths.

A fully editable cross-modulation matrix—where a modulator changes another modulator's rate or depth—is intentionally not exposed in v0.9. It needs a dedicated interface and explicit limits to stay understandable and stable on handheld hardware.

## DSP and source credits

The project uses a pinned, audited subset of **DaisySP** under its MIT licence for several synthesis engines. Cursed Drone's new v0.9 effects are project-specific implementations using standard delay, all-pass, filtering and envelope techniques; no external effect source file is copied into the project.

The diffusion and feedback design follows the general family of structures documented in classic artificial-reverberation literature, including Jon Dattorro's effect-design work. Cursed Drone does not include a full Dattorro plate or DaisySP ReverbSc in every slot: that would be a poor CPU and memory trade-off for the target handheld.

See [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) for the exact bundled components and licences.
