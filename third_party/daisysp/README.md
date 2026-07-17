# DaisySP subset

This directory vendors a reviewed, build-local subset of
[electro-smith/DaisySP](https://github.com/electro-smith/DaisySP) at commit
`599511b740f8f3a9b8db72a0642aa45b8a23c3a3`.

Included modules:

- `Utility/dsp.h`;
- `Filters/svf.{h,cpp}`;
- `Noise/dust.h`, `clockednoise.{h,cpp}`, `grainlet.{h,cpp}`, `particle.{h,cpp}`;
- `PhysicalModeling/drip.{h,cpp}`, `modalvoice.{h,cpp}`, `resonator.{h,cpp}`;
- `Synthesis/oscillator.{h,cpp}`.

The files are used by the four 0.3 expert-mode engines. Starting in 0.4, the
stable state-variable filter is also a building block for the procedural scene
actors. In 0.6 the Perry Cook/Soundpipe `Drip` model powers the wet-cave drop
actor. The upstream MIT license is reproduced in `LICENSE`.

`drip.cpp` contains one documented local correction: the second and third
resonators now store their local filter inputs. The pinned upstream port assigns
two never-updated member fields instead, which silences those resonances. Three
small parameter setters expose frequency, damping and source density without
changing the model.

To update the subset, check out a reviewed DaisySP commit, copy the same file
set, update the pinned SHA above and in `THIRD_PARTY_NOTICES.md`, then run the
core tests and the device CPU benchmark before committing.
