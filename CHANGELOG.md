# Changelog

Release-specific details live in `docs/releases/`. This file is the compact project history and version index.

## 1.0.0

- declare the standalone instrument feature-complete and move the repository to maintenance mode;
- record the practical field baseline: roughly 50% ordinary TrimUI Brick load and below 80% maximum observed load, with substantially lower observed load on Pixel 8 Pro and macOS browser testing;
- document all 16 Musical models, the four macro controls and exact MAIN/AUX/MIX/STEREO routing;
- add bilingual support guidance and structured reliability issue reports requiring platform, reproduction steps and logs;
- close the feature roadmap and direct future modular instrument development toward BRKSTN;
- retain the full 0.14.1 persistence, realtime and browser-regression hardening baseline.

See [`docs/releases/v1.0.0.md`](docs/releases/v1.0.0.md).

## 0.14.1

- sanitize all session values and recover corrupt primary saves from `.bak`;
- flush Android state on background/termination and persist Web saves promptly to IndexedDB;
- move Scala pitch quantization to a bounded control-rate cadence;
- add real browser mouse/touch/Retina interaction smoke tests;
- require semantic version bumps and release notes for shipped runtime changes.

See [`docs/releases/v0.14.1.md`](docs/releases/v0.14.1.md).

## 0.14.0

- added the public GitHub Pages WebAssembly/Web Audio version;
- reused the approved Android interface instead of creating a drifting browser frontend;
- added IndexedDB persistence, browser localisation, fullscreen handling and adaptive web audio/render settings;
- restored the complete Musical model, output-routing and tuning controls;
- fixed desktop mouse input, Retina sizing and double logical-coordinate conversion;
- refreshed third-party attribution and the post-0.14 roadmap.

See [`docs/releases/v0.14.0.md`](docs/releases/v0.14.0.md).

## 0.13.1

- completed the release-optimised Android ARM64 frontend;
- unified CMake, versioning, session format and CI across macOS, Android, PortMaster and NextUI;
- delivered the major realtime DSP optimisation pass and safer persistence/Kill behaviour.

See [`docs/releases/v0.13.1.md`](docs/releases/v0.13.1.md).

## 0.12.3

- fixed Actor-field reachability and desktop confirmations;
- made Kill constant-time in the realtime path;
- added atomic session saving, backups and additional regression tests.

See [`docs/releases/v0.12.3.md`](docs/releases/v0.12.3.md).

Older release notes remain available under [`docs/releases/`](docs/releases/).
