# Android control and routing audit

Audited against the `agent/android-audio-ui-rebuild` implementation and the shared schema 12 `Session` model.

## Routing result

The Android frontend mutates the same `Session` used by handheld and desktop builds, then submits the newest complete state to `AudioGraph`. A temporarily full realtime mailbox does not lose the final gesture value: the UI keeps it pending and retries on the next frame.

The approved renderer exposes all currently effective controls:

- Place: landscape and five performance macros;
- Actor / Sound: source, engine, frequency, timbre, color, motion, texture, level, pan, Plaits model/output and Scala tuning;
- Actor / Events: manual trigger, event density/rate feedback and Euclidean settings;
- Actor / Modulation: four lanes, enable, shape, destination, rate, bipolar depth/offset and cross-mod source/amount;
- FX: four serial actor slots with type, enable/bypass and all applicable parameters;
- Master: level, tempo and four independent master FX slots;
- Memory: eight slots, load/save, landscape reset, locale and fade times;
- Header: output fade and permanent `KILL`, routed directly to `AudioGraph::panic()`.

Special pickers use explicit sentinels and resolve correctly: Scala scale, Plaits model/output, modulation waveform and modulation destination.

## Deliberately not exposed

`PerformanceSettings::morph` and `morph_target` remain dormant in the shared model: neither the Android nor handheld/desktop DSP path currently consumes them. Exposing them only on Android would create a control that appears to work but cannot change sound, so they stay hidden until morphing is implemented in the core.

## Persistence

Autosave and all eight memory slots persist schema 12, including independent Actor/Master FX enabled states. Schema 11 sessions migrate with existing configured effects enabled by default.
