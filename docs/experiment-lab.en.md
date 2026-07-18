# Cursed Drone Experimental 0.11.1 — Guided Workflow

This build exists only on `experiment/v0.11-guided-workflow`. It installs beside the public version as **Cursed Drone Lab** and keeps separate saves.

## One coherent path

1. **PLACE** — choose a landscape, shape its state and balance four actors;
2. **ACTOR** — edit the selected sound source;
3. **FX** — four serial effects for that actor;
4. **MASTER** — final level, process tempo and four master effects;
5. **MEMORY** — eight user states, landscape restore and settings.

A landscape is a place. Four actors live in it. Each actor passes through its own FX chain. Their mix then passes through Master FX.

## Controls

| Button | Stable meaning |
| --- | --- |
| D-pad | select a field or change its value |
| A | open, apply or execute |
| B | back or cancel; hold for emergency Kill |
| X | next section on the current page |
| Y | contextual help |
| L / R | previous / next page |
| Select | fade the final output |
| Start | quick menu |
| Start + Select | save the last state and exit |

Holding a direction while editing a number accelerates to 2× and then 5×. It also works in musical-source, event and modulation settings.

## Place

X cycles Landscape, Macros and Actors. Landscape opens with A. Macros use Up/Down to select and Left/Right to change. Actors use Left/Right to select, Up/Down for level and A for mute.

## Actor

X switches Basic and Advanced. Basic controls active state, Landscape/Musical source, engine and sound character. Switching a Musical actor back to Landscape restores only that actor from the current place recipe.

Advanced contains 16 musical models, MAIN/AUX/MIX/STEREO output, Scala tuning, a note-name root, Euclidean events and four modulation rows with bounded cross-modulation.

## FX and Master FX

Every actor retains four effects. Master now has four additional effects after the actor mix. A long Delay or Diffuser can preserve a tail while changing landscapes.

## Memory and recovery

Autosave always resumes the last state. Eight explicit memory files live beside it:

```text
conf/memory-1.cdrone ... conf/memory-8.cdrone
```

Restore Landscape returns the four actors to the place recipe while preserving the Master FX chain.

## Removed from the normal path

The separate Morph and Stage modes are hidden. Their extra modes and vocabulary cost more than they returned. The advanced sound engine remains, but it now lives inside Place → Actor → FX → Master → Memory.
