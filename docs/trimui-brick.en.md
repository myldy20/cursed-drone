# TrimUI Brick and porting

The reference target is the user's TrimUI Brick running Knulli Scarab. The observed unit reports aarch64, four CPU cores up to roughly 1416 MHz, about 1 GB RAM, and a `1024×768` screen. These are measurements of one device/firmware combination, not a promise for every revision.

The UI renders at logical `512×384`, giving the Brick an exact 2× scale. The audio target is stereo float 48 kHz at 256 or 512 callback frames, selected only after xrun testing.

SDL2 is the common video/controller/audio layer and has strong PortMaster precedent. The audio core does not depend on SDL, so a direct ALSA/KMS adapter remains possible if a firmware needs one.

## Required device probe

Record kernel, CPU/RAM, available governors/frequencies, input devices, audio devices, SDL driver/fullscreen behavior, actual audio format and callback size, every hardware button/chord, writable storage, atomic rename, suspend/resume and ten-minute callback overrun count. Stock, CrossMix, Knulli and Anbernic firmware must have explicit mapping profiles; identical SDL codes must never be assumed.

The preliminary launcher is `platform/trimui-brick/launch.sh`. It intentionally changes no governor or system state until a safe firmware-specific API and reliable restoration path are proven.

Expected package shape:

```text
CursedDrone/
├── bin/aarch64/cursed-drone-sdl
├── data/
├── licenses/
├── platform/trimui-brick/launch.sh
└── README.txt
```

Writable data lives outside the read-only program area and is provided as `CURSED_DRONE_DATA_DIR`. Autosave will use a temporary file followed by atomic rename.

## Performance gate

Benchmark engine-only, 16 light effects, mixed medium/light patches and heavy algorithms at each available CPU frequency. No audio callback may allocate, access files, lock, format strings or clear large memory. Telemetry crosses a lock-free channel.

The early success gate is ten uninterrupted minutes of four engines on the Brick at 1008 MHz. UI polish follows that result.

The first public package is PortMaster aarch64 with a self-contained binary/dependency declaration, state-restoring launcher, mapping profiles, writable save path, bilingual notes, checksums and complete license bundle. armhf follows measurement. libretro is deferred because standalone code controls latency, autosave, system buttons and long-running audio more reliably.
