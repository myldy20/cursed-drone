# Actor mute root cause

In Guided Workflow 0.11.1 the UI correctly changed `Session::slots[n].enabled`, but `AudioGraph::rebuild_effective_slots()` replaced it with `source.enabled || destination.enabled`. Landscape recipes normally enable all four destination actors, so every muted actor was immediately enabled again in the effective audio graph.

The public release must:

1. treat `source.enabled` as authoritative;
2. suppress per-actor effect tails while the actor is muted;
3. include a regression test that submits a muted session to a running audio graph and verifies silence.
