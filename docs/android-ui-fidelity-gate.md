# Android visual fidelity gate

The Android frontend is not accepted merely because all controls exist. The Pixel 8 Pro implementation must reproduce the approved wide-screen Cursed Drone design rather than restyling the handheld interface.

## Reference surface

- Pixel 8 Pro landscape reference: **2992 × 1344 px**.
- Full usable width; no virtual controller, letterboxing or unexplained side bars.
- Five pages remain directly reachable: **Place, Actor, FX, Master, Memory**.
- The shared 0.13 core remains authoritative for Session, DSP, effects, catalogues and persistence.

## Visual system

The approved direction is a polished dark instrument dashboard, not a tracker grid and not a generic SDL debug panel.

- near-black layered background;
- warm cream primary typography;
- restrained purple identity/accent layer;
- individual green, amber, red and blue actor accents;
- soft depth separation and controlled glow rather than one-pixel outlines everywhere;
- modern proportional/semibold typography; the 5×7 bitmap font is not acceptable for the final Android frontend;
- rounded or shaped cards with intentional internal padding;
- thin tracks, luminous fills and clear handles for sliders;
- meters, event flashes and DSP status are integrated visual instruments, not raw debug numbers;
- labels use visual hierarchy: page/actor identity, value, secondary explanation.

## Place composition

- branded title/status region with DSP load, output meter, Fade and permanent Kill;
- direct page navigation remains visible;
- one full-width landscape selector;
- five large performance macros grouped on the left;
- four actor cards in a 2×2 group on the right;
- each actor card shows number/name, level, live signal, event activity and an integrated Mute/Unmute action;
- no control may visually resemble a disabled debug rectangle when it is active.

## Other pages

- Actor uses a compact actor selector and clear Sound / Events / Modulation hierarchy;
- FX communicates an ordered serial chain, with effect identity visually stronger than parameter chrome;
- Master gives output, tempo, Fade and global effects a coherent mix-bus composition;
- Memory separates slots/actions from application settings and identity.

## Interaction contract

- tap applies choices directly through visible pickers;
- tap or horizontal drag sets sliders continuously;
- drawing and hit testing share the same geometry;
- no synthetic D-pad or hidden focus model;
- inactive controls do not retain touch targets;
- all touch targets remain practical on a real Pixel 8 Pro.

## Required review sequence

1. Build the actual Android renderer as a desktop snapshot target.
2. Publish **all five 2992×1344 BMP renders** from CI.
3. Review typography, spacing, geometry, hierarchy and control styling against the approved mockups.
4. Only after the static renders pass may CI publish an APK as a design candidate.
5. Hardware validation then checks touch routing, audio continuity, background/resume and thermal stability.

A green compiler/build result is not visual approval.
