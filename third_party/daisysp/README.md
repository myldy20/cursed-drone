# DaisySP subset

This directory vendors an unmodified, build-local subset of
[electro-smith/DaisySP](https://github.com/electro-smith/DaisySP) at commit
`599511b740f8f3a9b8db72a0642aa45b8a23c3a3`.

Included modules:

- `Utility/dsp.h`;
- `Filters/svf.{h,cpp}`;
- `Noise/dust.h`, `clockednoise.{h,cpp}`, `grainlet.{h,cpp}`, `particle.{h,cpp}`;
- `PhysicalModeling/modalvoice.{h,cpp}`, `resonator.{h,cpp}`;
- `Synthesis/oscillator.{h,cpp}`.

The files are used by the four provisional product engines in Cursed Drone
0.3.0. The upstream MIT license is reproduced in `LICENSE`. No upstream
source file in this directory has been modified.

To update the subset, check out a reviewed DaisySP commit, copy the same file
set, update the pinned SHA above and in `THIRD_PARTY_NOTICES.md`, then run the
core tests and the device CPU benchmark before committing.
