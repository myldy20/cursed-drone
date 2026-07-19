# Architecture — Cursed Drone 0.12.1

```text
Place macros
   ↓
Actor 1 source → modulation → FX1 → FX2 → FX3 → FX4 → level/pan ┐
Actor 2 source → modulation → FX1 → FX2 → FX3 → FX4 → level/pan ├→ mix
Actor 3 source → modulation → FX1 → FX2 → FX3 → FX4 → level/pan ┤
Actor 4 source → modulation → FX1 → FX2 → FX3 → FX4 → level/pan ┘
                              mix → Master FX1..4 → DC blocker → limiter → fade
```

The audio thread receives complete `Session` snapshots through an SPSC queue. Schema 10 stores four actors, Scala tuning, Euclidean settings, four modulation rows, actor FX, Master FX and performance macros.

Actor `enabled` is authoritative. Landscape/morph recipe construction may change topology and parameters but cannot re-enable a muted actor. Disabled actors still feed silence through their processors so buffers decay internally; their actor-FX tails are suppressed. Master tails remain global.

Autosave and eight memory slots use readable `.cdrone` files. Built-in Scala files ship under assets; user scales are read from the runtime data directory.
