# Third-party notices / Сторонний код

Cursed Drone project code is GPL-3.0-or-later. The components below retain their upstream licences.

## DaisySP subset

- Upstream: `electro-smith/DaisySP`
- Pinned commit: `599511b740f8f3a9b8db72a0642aa45b8a23c3a3`
- Local files: audited filters, noise, physical-model and oscillator sources under `third_party/daisysp/`
- License: MIT, full text stored locally
- Use: procedural engines, resonators, filters and the Drip water model

## Original musical-engine source

- Upstream: `pichenettes/eurorack`
- Pinned commit: `08460a69a7e1f7a81c5a2abcc7189c9a6b7208d4`
- Local form: git submodule at `third_party/eurorack`
- Compiled files: selected `plaits/dsp`, `plaits/resources.cc` and required `stmlib` DSP/utilities; tests excluded
- License: MIT for the compiled source set; bundled text in `third_party/PLAITS_LICENSE.txt`
- Product use: sixteen curated models exposed under the neutral UI label **Musical / Музыкальный**

The project is not affiliated with or endorsed by Mutable Instruments and does not market the product using the original module name.

## font512

- Upstream: `alexfru/512_8`
- Pinned commit: `6bd43ef930697ac54567e9fcf59e6ffc24c6813e`
- License: Unlicense / public domain
- Use: 512-glyph bitmap UI font with Latin and Russian glyphs

References mentioned in research documents are not dependencies unless listed above. GPL compatibility does not remove attribution obligations.
