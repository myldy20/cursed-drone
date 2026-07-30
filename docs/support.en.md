# Support and reliability reports

Cursed Drone is feature-complete and maintained for reproducible defects, data loss, severe audio failures and supported-platform compatibility. New feature requests are outside the maintenance plan.

## Before opening a report

1. Reproduce the problem with the latest published release.
2. Restart the application and try the same sequence again.
3. Record the exact version, device, operating system or firmware and browser where relevant.
4. Write the smallest reliable sequence that causes the problem.
5. Attach the available log without deleting the lines immediately before the failure.

Use the repository **Bug report / reliability issue** form. Reports without a version, platform and reproduction sequence may not be actionable.

## Log locations

### Knulli / PortMaster

The application directory is normally `roms/ports/curseddrone/`. Useful files are under `curseddrone/conf/`:

- `cursed-drone.log` — output from the latest application run;
- `device-probe.log` — first-launch hardware and SDL probe;
- `autosave.cdrone` and `.bak` — only attach these when the problem concerns loading or recovery and the session contains nothing private.

### NextUI

The default log is:

`/.userdata/tg5040/logs/Cursed Drone.txt`

Runtime data is normally under:

`/.userdata/tg5040/cursed-drone/`

### Android

Include the phone model, Android version and whether Bluetooth, USB audio or the built-in output was used. When Android platform tools are available, capture a log immediately after reproducing the problem:

```bash
adb logcat -d > cursed-drone-android-log.txt
```

The application stores sessions in private app storage. Uninstalling the application can remove them.

### Web

Include the full browser name and version, operating system, page URL, audio output device and whether the page was fullscreen. Open the browser developer console, reproduce the issue and copy the relevant console output. Also state whether reloading the page fixed it.

### macOS native build

Launch the binary from Terminal and save its output:

```bash
./cursed-drone 2>&1 | tee cursed-drone-macos-log.txt
```

## Live-use reliability

The maintainer's field baseline is approximately 50% ordinary load and below 80% maximum observed load on TrimUI Brick, with substantially lower observed load on Pixel 8 Pro and macOS browser testing. This is not a guarantee for every patch or routing setup.

For a performance:

- rehearse the exact build, scene memories, audio output and power arrangement;
- avoid updating firmware, browser or application immediately before the show;
- keep a copy of the working release and memory files;
- keep a fallback audio source available.
