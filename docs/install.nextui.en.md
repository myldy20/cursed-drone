# Install Cursed Drone 0.14.1 on NextUI

> Verified on a real TrimUI Brick with NextUI: launch, video, audio and controls work.

1. Download `cursed-drone-v0.14.1-nextui-tg5040.zip` from the GitHub release.
2. Extract the ZIP to the **root of the NextUI SD card**.
3. Confirm `Tools/tg5040/Cursed Drone.pak/launch.sh` exists.
4. Open **Tools → Cursed Drone**.

CI artifacts use development-oriented names and may contain an additional outer archive. Normal users should install the versioned ZIP from GitHub Releases.

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

The Tool Pak includes the project licence, third-party notices, the Musical-engine MIT licence, `NOTICE.md` and `ADDITIONAL_TERMS.md` under `licenses/`.
