# Installing Cursed Drone on a handheld

[Русская версия](install.ru.md)

## Read this first

Cursed Drone is currently a **public test**, not an official PortMaster catalogue release.

The only device tested on real hardware is:

- **TrimUI Brick**;
- **Knulli scarab 2026/05/11**;
- AArch64 Linux;
- Mali/OpenGL ES 2 renderer;
- ALSA audio at 48 kHz stereo, 512-sample buffer;
- built-in `TRIMUI Brick Controller`.

The package is built for **AArch64**. Other PortMaster-compatible handhelds may work, but they are unverified. Do not describe an untested device as supported until somebody has supplied a useful log or issue report.

## 1. Download the package

Open the latest successful [GitHub Actions build](https://github.com/myldy20/cursed-drone/actions/workflows/build.yml) and download the artifact named:

```text
cursed-drone-portmaster-aarch64
```

GitHub places build artifacts inside an outer ZIP. Extract that file first. Inside it you should find:

```text
curseddrone-aarch64-test.zip
```

That inner ZIP is the actual PortMaster package.

## 2. Find the Ports directory

The package must be extracted into the same directory that already contains other PortMaster launch scripts.

| Firmware / setup | Typical Ports directory | Status |
| --- | --- | --- |
| TrimUI Brick + Knulli | `/userdata/roms/ports/` | **Verified** |
| Other Knulli devices | `/userdata/roms/ports/` | Unverified |
| ArkOS, ROMs on first card | `/roms/ports/` | Unverified |
| ArkOS, ROMs on second card | `/roms2/ports/` | Unverified |
| ROCKNIX / JELOS-style storage | the `ports` folder inside the active ROMs library | Unverified |
| AmberELEC | the `ports` folder inside the active ROMs library | Unverified |

Firmware layouts vary. The reliable rule is simple: locate the folder containing launchers for ports you already use, then extract Cursed Drone into that folder.

## 3A. Install using an SD card or file manager

Extract `curseddrone-aarch64-test.zip` directly into the Ports directory.

Correct result:

```text
<ports>/Cursed Drone.sh
<ports>/curseddrone/cursed-drone-sdl.aarch64
<ports>/curseddrone/cursed-drone-probe.aarch64
<ports>/curseddrone/assets/cursed-drone-splash.bmp
<ports>/curseddrone/README.md
```

A common mistake is creating one folder too many:

```text
<ports>/curseddrone-aarch64-test/Cursed Drone.sh   # wrong
```

`Cursed Drone.sh` must sit directly inside the Ports directory.

After copying, refresh the game list or reboot the handheld.

## 3B. Install on TrimUI Brick using SCP

The commands below assume:

- the handheld address is `10.53.219.134`;
- the package is in the Mac `Downloads` folder;
- SSH user is `root`.

Run this in a **local Terminal window on the Mac**, not inside the existing SSH session:

```bash
scp ~/Downloads/curseddrone-aarch64-test.zip \
  root@10.53.219.134:/userdata/system/
```

Connect to the Brick:

```bash
ssh root@10.53.219.134
```

Install or update:

```bash
unzip -o /userdata/system/curseddrone-aarch64-test.zip \
  -d /userdata/roms/ports/

chmod +x "/userdata/roms/ports/Cursed Drone.sh"
chmod +x /userdata/roms/ports/curseddrone/*.aarch64
sync
reboot
```

Existing settings are not deleted by this update procedure.

## 4. First launch

Open:

```text
Ports → Cursed Drone
```

On the first launch, a device probe appears before the main interface. It checks:

- video driver and renderer;
- audio driver, frequency, channels and buffer size;
- available game controllers;
- physical button mapping.

Press the controls you want to test, then press **Start**. The probe writes its report and opens Cursed Drone.

The branded splash screen is shown after the probe. It can be skipped by pressing a key, and can be disabled for debugging with:

```bash
export CURSED_DRONE_SKIP_SPLASH=1
```

## 5. Essential controls

| Action | Button |
| --- | --- |
| navigate | D-pad |
| decrease / increase | L / R |
| open a picker | Start |
| confirm / cancel picker | B / A |
| next page | X |
| mute selected track | B outside a picker |
| hard Kill | A outside a picker |
| select next source track on FX page | Y |
| output fade | Select |
| save and exit | hold Start, then press Select |

The exit chord is order-sensitive by design: hold **Start**, then press **Select**.

## 6. Saves and logs

All PortMaster runtime data lives under:

```text
<ports>/curseddrone/conf/
```

Files:

```text
conf/autosave.cdrone       current session after v0.8
conf/device-probe.log      hardware probe report
conf/cursed-drone.log      application stdout/stderr
conf/probe-v1.complete     marker that skips the probe
```

Older builds stored the autosave in:

```text
conf/myldy20/cursed-drone/autosave.cdrone
```

Version 0.8 reads that old location once if needed and migrates future saves to `conf/autosave.cdrone`.

### Run the hardware probe again

```bash
rm -f /userdata/roms/ports/curseddrone/conf/probe-v1.complete
```

### Reset only the saved synth state

```bash
rm -f /userdata/roms/ports/curseddrone/conf/autosave.cdrone
rm -f /userdata/roms/ports/curseddrone/conf/myldy20/cursed-drone/autosave.cdrone
```

## 7. Send useful logs with a bug report

On TrimUI Brick, create one archive:

```bash
tar -czf /userdata/system/cursed-drone-logs.tar.gz \
  -C /userdata/roms/ports/curseddrone/conf .
```

Copy it to your computer:

```bash
scp root@10.53.219.134:/userdata/system/cursed-drone-logs.tar.gz \
  ~/Downloads/
```

Attach the archive to a GitHub issue and include:

- handheld model;
- firmware name and version;
- what you pressed or changed before the problem;
- whether audio continued;
- approximate DSP load shown by the application.

## 8. Troubleshooting

### The port does not appear

Check that the launcher is exactly here:

```text
<ports>/Cursed Drone.sh
```

Then refresh the game list or reboot.

### Black screen or immediate return to the menu

Read:

```text
<ports>/curseddrone/conf/cursed-drone.log
<ports>/curseddrone/conf/device-probe.log
```

Also verify that both `.aarch64` binaries exist and are executable.

### The probe repeats every time

The firmware may not be able to create:

```text
conf/probe-v1.complete
```

Check free space and write permissions on the ROMs partition.

### Controls do not match the labels

Delete `probe-v1.complete`, run the probe again and attach `device-probe.log` to an issue. Different firmware builds can expose different SDL controller mappings.

### Audio crackles

Record the shown DSP percentage, scene name and active engines. Do not immediately overclock the handheld: first provide a log and a reproducible patch, because the project is intended to remain usable on modest AArch64 devices.

## 9. Uninstall

Remove:

```text
<ports>/Cursed Drone.sh
<ports>/curseddrone/
```

Deleting `curseddrone/` also deletes the autosave and logs, so copy `conf/autosave.cdrone` first if you want to keep the current patch.
