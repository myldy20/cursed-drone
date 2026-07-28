# Third-party notices / Сторонний код

Cursed Drone first-party code is GPL-3.0-or-later and, for files that point to it, is also subject to `ADDITIONAL_TERMS.md`. `NOTICE.md` summarizes the required attribution. The components below are third-party material and retain their upstream licences; Cursed Drone's additional terms do not apply to them.

## DaisySP subset

- Upstream: `electro-smith/DaisySP`
- Pinned commit: `599511b740f8f3a9b8db72a0642aa45b8a23c3a3`
- Local files: audited filters, noise, physical-model and oscillator sources under `third_party/daisysp/`
- License: MIT, full text stored locally
- Use: procedural engines, resonators, filters and the Drip water model

## Mutable Instruments Plaits DSP source

- Upstream: `pichenettes/eurorack`
- Pinned commit: `08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4`
- Local form: git submodule at `third_party/eurorack`
- Compiled files: selected `plaits/dsp`, `plaits/resources.cc` and required `stmlib` DSP/utilities; tests excluded
- License: MIT for the compiled STM32 source set; bundled text in `third_party/PLAITS_LICENSE.txt`
- Product use: sixteen curated macro-oscillator models exposed as the **Musical / Музыкальный** source

The MIT licence permits use, modification and redistribution provided its copyright and permission notice are retained. Internal source names and some technical labels may retain the upstream word **Plaits** for traceability and attribution. The product itself is named **Cursed Drone**; it is not sold or presented as a Mutable Instruments or Plaits product.

**Mutable Instruments** is a registered trademark. Cursed Drone is an independent project and is not affiliated with, endorsed by or supported by Mutable Instruments or Emilie Gillet. The upstream recommendation not to reuse original module names for derivative products is respected by the Cursed Drone product name and Musical source branding.

## font512

- Upstream: `alexfru/512_8`
- Pinned commit: `6bd43ef930697ac54567e9fcf59e6ffc24c6813e`
- License: Unlicense / public domain
- Use: 512-glyph bitmap UI font with Latin and Russian glyphs

References mentioned in research documents are not dependencies unless listed above. GPL compatibility does not remove attribution obligations.
