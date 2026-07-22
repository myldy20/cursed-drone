# Android approved UI — Cursed Drone 0.13

This document freezes the Android visual and interaction contract approved after the rejected prototypes in PR #13 and PR #15.

## Shared architecture

Android uses the same `cursed_drone_core`, `Session`, catalogues, audio graph, session files, event triggering, Scala tuning and effects as desktop and handheld builds. Only layout, drawing, touch routing and Android lifecycle integration are platform-specific.

## Screens

- **Place:** landscape selector, five performance macros and four actor cards with level, meters, event flashes and integrated mute.
- **Actor:** actor selector, landscape/musical source, engine, manual event trigger, Event Rate and separate Sound, Events and Modulation sections.
- **FX:** four serial actor effects. A slot must be selected before its existing core parameters are edited.
- **Master:** master level, tempo, output fade and four post-mix effects.
- **Memory:** eight slots, load, save, restore landscape, autosave status, language, fade timings and application information.

## Touch contract

- Every visible button maps to one explicit action.
- Page tabs remain reachable with one tap.
- Sliders support direct positioning and continuous horizontal drag.
- Drawing and hit testing use the same rectangles.
- Actor mute buttons never overlap actor selection or level controls.
- Pickers are visible modal grids; tapping an item applies it directly.
- No touch action is translated into keyboard or D-pad focus navigation.

The generated concepts are visual references, but implementation exposes only fields that really exist in the 0.13 core. Decorative parameters that are not represented by `Session` are intentionally omitted.