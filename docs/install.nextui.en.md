# Install Cursed Drone 0.12.2 on NextUI

> Verified on a real TrimUI Brick with NextUI: launch, video, audio and controls work.

1. Download the `cursed-drone-nextui-tg5040` artifact.
2. Open GitHub's outer ZIP and locate `cursed-drone-nextui-tg5040-test.zip`.
3. Extract the inner ZIP to the **root of the NextUI SD card**.
4. Confirm `Tools/tg5040/Cursed Drone.pak/launch.sh` exists.
5. Open **Tools → Cursed Drone**.

Do not install the PortMaster ZIP on NextUI and do not place third-party Paks in the hidden `.system` folder.

## Update without losing work

Replace `Tools/tg5040/Cursed Drone.pak/`. Runtime data remains under `.userdata/tg5040/cursed-drone/`.

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

## Data and log

```text
.userdata/tg5040/cursed-drone/autosave.cdrone
.userdata/tg5040/cursed-drone/memory-1.cdrone ... memory-8.cdrone
.userdata/tg5040/cursed-drone/scales/*.scl
.userdata/tg5040/logs/Cursed Drone.txt
```

NextUI save states and auto-resume do not apply because Cursed Drone is a standalone SDL app.

The Tool Pak includes `NOTICE.md` and `ADDITIONAL_TERMS.md` under `licenses/`.
