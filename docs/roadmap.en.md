# Roadmap after 0.14.0

## Current stage

Cursed Drone has moved beyond isolated platform ports into a **multi-platform public beta and product-hardening stage**. Every target shares one C++ core, session format, synthesis catalogue and effect catalogue. The controller-first TrimUI/desktop interface remains platform-appropriate, while Android and WebAssembly share the adaptive touch interface.

## Completed from the original roadmap

- guided workflow: Place → Actor → FX → Master → Memory;
- Musical source based on pinned MIT-licensed Plaits code, with 16 macro models and MAIN/AUX/MIX/STEREO routing;
- Scala tuning, Euclidean events and four modulation rows per actor;
- Reverse Grains, compound processors, four Actor FX and four Master FX;
- autosave, eight memory slots and session-schema migration;
- verified Knulli/PortMaster and NextUI packages;
- release-optimised Android ARM64 build;
- public WebAssembly build with IndexedDB, mouse, touch, fullscreen and localisation;
- one version source, CMake graph, CI matrix and automatic release publishing;
- realtime DSP P1 optimisations, constant-time Kill and atomic persistence.

## 0.14.x — harden the platforms first

1. Add browser interaction smoke tests to CI: multiple aspect ratios, DPR 1/2, mouse clicks and drags, plus touch emulation. This is the direct regression barrier for the Web/Retina coordinate fix.
2. Capture reproducible DSP and audio-buffer profiles on TrimUI Brick, Pixel 8 Pro and a representative desktop browser; publish practical budgets and known heavy scenes.
3. Add concise descriptions for Musical models, advanced parameters and output routing without crowding the performance surface.
4. Route support through tickets containing logs, firmware/browser version and exact reproduction steps.

## 0.15.0 — memory workflow and feedback

1. Named memory slots.
2. Copy/duplicate state and one-step undo without turning the instrument into a file manager.
3. Clearer visual feedback for each actor's contribution, event activity and Master FX tails.
4. Small live-performance UX improvements: clearer current source, selected FX and save state.

## After 0.15 — creative expansion

1. Implement real landscape morphing; the stored morph fields remain intentionally hidden until the shared DSP path supports it.
2. Decide whether the current four modulation rows are sufficient or should become a more freely routed modulation matrix.
3. Explore Stage-style scene automation and longer event development only if the controls remain practical on TrimUI Brick.
4. Add hardware ports only when a test-device owner, reproducible logs and a sustainable support budget exist.

## Recommended next move

Do not add another large DSP layer immediately after the browser release. First ship a 0.14.x hardening pass with automated browser interaction testing and hardware profiling. Then build 0.15.0 around memory labels, undo and visual feedback. Treat morphing as the next large creative release after those foundations are stable.
