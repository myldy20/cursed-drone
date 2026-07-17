# Cursed Drone Experimental 0.10 — Performance Lab

This build exists only on `experiment/plaits-performance-lab`. It does not replace public v0.9 and must not be merged into `main` before a real TrimUI Brick test.

## Simplified controls

| Button | Normal operation |
| --- | --- |
| D-pad | navigation only |
| L/R | decrease / increase the selected value |
| A | open, perform or confirm |
| B | back or cancel; hold for emergency Kill |
| X | next main page |
| Y | next actor |
| Select | output fade |
| Start | open Performance Lab |
| Start + Select | save and exit |

Physical A/B labels on TrimUI Brick follow this table regardless of SDL's internal button numbering.

## Performance Lab

Press `Start`. D-pad Left/Right changes the section and Up/Down changes the field. L/R edits. A performs an action. Y selects the actor. B closes the Lab.

### ACTOR

- `ENGINE` activates the original macro-oscillator actor;
- `MODEL` selects one of 16 musical engines;
- `OUTPUT` selects `MAIN`, `AUX`, `MIX` or true `STEREO`;
- `SCALA` selects a built-in or user `.scl` tuning;
- `ROOT` sets the scale's MIDI root note.

Put user Scala files in `curseddrone/conf/scales/`. They are discovered on the next launch.

### MOD

Four modulation rows per actor. X selects the row.

- enabled;
- sine, triangle, sample-and-hold or random-walk source;
- destination;
- rate;
- bipolar depth from −100% to +100%;
- a previous modulator as a rate-modulation source;
- bipolar cross-mod amount.

A modulator can only change an earlier row's output. This prevents accidental instantaneous digital feedback loops.

### EVENT

Euclidean event generator:

- 1–32 steps;
- pulse count;
- rotation;
- probability;
- install `Reverse Grains` into FX4.

It excites musical and physical actors without turning the primary UI into a sequencer.

### MORPH

- target landscape;
- continuous 0–100% morph;
- `COMMIT` applies the target and returns morph to zero.

Parameters and levels interpolate continuously. Engine and FX topology changes near the midpoint, avoiding the DSP cost of four duplicate actors.

### STAGE

A opens the minimal performance view:

- five performance macros;
- morph;
- four actor levels and mute states;
- output fade.

B returns to the normal interface. Detailed controls stay in Lab and do not pollute live operation.

## Reverse Grains

Two overlapping reverse grains read a shared circular buffer at different offsets. Controls:

- Mix;
- Grain duration and spread;
- Feedback.

The effect reuses the memory owned by its FX slot instead of allocating another global multi-megabyte buffer.

## Macro-oscillator source

The branch pins upstream `pichenettes/eurorack` and uses original Plaits and stmlib code under the MIT licence. Cursed Drone uses neutral product labels and does not present this derivative work as an official Mutable Instruments product.
