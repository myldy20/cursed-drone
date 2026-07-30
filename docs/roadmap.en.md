# Cursed Drone roadmap — completed at 1.0.0

## Product status

Cursed Drone 1.0.0 is the feature-complete standalone release. It is a multi-platform procedural soundscape instrument with one shared C++ core, session format, synthesis catalogue and effect catalogue. TrimUI/desktop use a controller-first interface; Android and WebAssembly share the adaptive touch interface.

The project now moves to **maintenance mode**. New synthesis layers, memory workflows, scene automation and larger performance-system features are no longer planned for this repository. Future modular instrument development will continue in BRKSTN, where Cursed Drone can inform or become one sound module among others.

## Original roadmap — completed

- guided workflow: Place → Actor → FX → Master → Memory;
- Musical source based on pinned MIT-licensed Plaits code;
- 16 curated macro models with MAIN/AUX/MIX/STEREO routing;
- Scala tuning, Euclidean events and four modulation rows per actor;
- Reverse Grains, compound processors, four Actor FX and four Master FX;
- autosave, eight memory slots, schema migration and `.bak` recovery;
- verified Knulli/PortMaster and NextUI packages for TrimUI Brick;
- release-optimised Android ARM64 build;
- public WebAssembly build with IndexedDB, mouse, touch, fullscreen and localisation;
- one version source, root CMake graph, cross-platform CI and automatic release publishing;
- realtime DSP optimisation, constant-time Kill and session sanitisation;
- browser interaction regression tests across mouse, touch, DPR 1/2 and Retina letterboxing;
- practical field validation on TrimUI Brick, Pixel 8 Pro and macOS browser;
- Musical model and routing documentation;
- structured reliability reports with platform, reproduction steps and logs.

## 1.0 field baseline

Maintainer testing observed approximately 50% load in normal TrimUI Brick use and less than 80% at the heaviest tested point. Pixel 8 Pro and macOS browser testing showed substantially lower load with no noticeable release-blocking audio or interaction failures.

These numbers are field observations, not guarantees. Exact results depend on firmware, browser, clock settings, audio devices and patch complexity.

## Maintenance policy

Changes after 1.0 are limited to:

1. reproducible crashes, data loss and severe audio failures;
2. compatibility fixes for supported platforms;
3. security, licensing and dependency maintenance;
4. build and release infrastructure repairs;
5. documentation corrections.

Feature requests, speculative ports and large DSP expansions are not part of the maintenance plan.

## Deliberately not implemented

The following earlier ideas are closed rather than postponed:

- named or duplicated memory slots and general undo;
- true landscape morphing;
- a freely routed modulation matrix;
- Stage-style scene automation;
- additional hardware ports without a maintained test path.

Those ideas may reappear as reusable BRKSTN modules or infrastructure, but Cursed Drone itself stays small, recognisable and stable.
