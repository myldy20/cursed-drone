# Cursed Drone 0.12.2 — Guided Workflow

Cursed Drone is a live soundscape instrument: choose a **place**, shape four **actors**, process them with actor FX, then finish the whole space with Master FX.

## One path through the instrument

1. **PLACE** — landscape, five performance macros and four actor levels/mutes.
2. **ACTOR** — source, engine and sound character for the selected actor.
3. **FX** — four serial effects for that actor; effect type and parameters live in one editor.
4. **MASTER** — final level, tempo and four effects after the actor mix.
5. **MEMORY** — eight user states, landscape restore, language and fade settings.

## Controls

| Button | Meaning |
| --- | --- |
| D-pad | navigate; edit the selected value; hold to accelerate |
| A | open, confirm, perform the selected action, or mute an actor |
| B | back/cancel; hold for emergency Kill |
| X | next focus section on the current page |
| Y | contextual help |
| L / R | previous / next page |
| Select | fade the final output |
| Start | quick menu |
| Start + Select | save the current state and exit |

## Place

X cycles **Landscape → Macros → Actors**. In Actors, Left/Right selects an actor, Up/Down changes its level, and A mutes/unmutes it. Muting is authoritative in the audio graph; an actor recipe or old actor-FX tail cannot turn it back on.

## Actor

The actor selector is available on this page. Basic mode contains active state, **Trigger Now**, Landscape/Musical source, engine, frequency, character and **Event Rate**. Advanced mode contains 16 Musical models, MAIN/AUX/MIX/STEREO routing, built-in or user Scala tuning, root note, Euclidean events and four modulation rows with bipolar depth and bounded rate cross-modulation.

For event-based engines, select **Trigger Now** and press A to hear the selected actor immediately. A short `!` flash on actor cards confirms both manual and generated events. Event Rate controls that actor's average event frequency; the value line shows both events per second/minute and **MAX**, the upper-bound wait before the actor scheduler forces the next event. Natural events may still happen earlier. The global **Events** macro raises or lowers activity across the whole place. Hold B for about 1.1 seconds to run **Kill Silence**, which resets voices and effect memory; it is an emergency silence command, not the preview trigger.

## FX and Master

Every actor has four serial FX slots. The Master page has four more slots after all actors are mixed. A long Delay, Diffuser or Reverse Grains on Master can preserve atmosphere while the underlying place changes.

## Memory

Autosave resumes the last state. Eight explicit memories are stored as `memory-1.cdrone` … `memory-8.cdrone`. **Restore Landscape** rebuilds the four actors from the current place recipe while preserving Master FX.
