# Install Cursed Drone 0.12.2 on Knulli / PortMaster

> Verified on a real TrimUI Brick running Knulli. Other AArch64 PortMaster handhelds remain unverified.

1. Download the `cursed-drone-portmaster-aarch64` artifact from a successful build.
2. Open GitHub's outer ZIP and locate `curseddrone-aarch64-test.zip`.
3. Extract the inner ZIP into `/userdata/roms/ports/`.
4. Confirm `/userdata/roms/ports/Cursed Drone.sh` and `/userdata/roms/ports/curseddrone/cursed-drone-sdl.aarch64` exist.
5. Reboot or refresh Ports, then open **Ports → Cursed Drone**. Press Start after the first hardware probe.

## Update without losing work

Extract a newer package over the same folders. Do not delete `curseddrone/conf/`; it contains autosave, eight memories and user scales.

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

## Data and logs

```text
curseddrone/conf/autosave.cdrone
curseddrone/conf/memory-1.cdrone ... memory-8.cdrone
curseddrone/conf/scales/*.scl
curseddrone/conf/device-probe.log
curseddrone/conf/cursed-drone.log
```

Built-in scales are packaged under `curseddrone/assets/scales/`. Copy additional Scala `.scl` files into `conf/scales/`.

The Knulli and NextUI archives are not interchangeable.

The package also includes `NOTICE.md` and `ADDITIONAL_TERMS.md` under `curseddrone/licenses/`.
