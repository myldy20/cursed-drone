# Installing Cursed Drone on NextUI

> Status: the package is built and validated in AArch64 CI, but has not yet been tested on a physical TrimUI Brick running NextUI.

Do not install the Knulli/PortMaster package on NextUI. NextUI uses its own Tool Pak layout.

## Installation

1. Download the `cursed-drone-nextui-tg5040` artifact from a successful GitHub Actions run.
2. GitHub may wrap it in an outer ZIP. Inside, locate `cursed-drone-nextui-tg5040-test.zip`.
3. Extract the inner archive to the **root of the NextUI SD card**.
4. Confirm the final path:

```text
Tools/tg5040/Cursed Drone.pak/launch.sh
```

5. Safely eject the card, insert it into the handheld, and open **Tools → Cursed Drone**.

Do not place the Pak under the hidden `.system` folder. NextUI replaces that folder during updates.

## Data and logs

Autosave:

```text
.userdata/tg5040/cursed-drone/autosave.cdrone
```

Launch log:

```text
.userdata/tg5040/logs/Cursed Drone.txt
```

If the application immediately returns to the menu, provide that log.

## Limitations

Cursed Drone runs as a standalone SDL application. NextUI save states, auto-resume, and its standard in-game menu do not apply.

To save and exit Cursed Drone, hold **Start**, then press **Select**.
