# Diagnostics and bug reports

[Platform index](../platform.md) · [Performance and profiling](performance.md) · [Audio](../audio.md) · [Packaging](packaging.md)

What a player can send us, and what the game writes so that a report can be answered without a second round of questions. Everything here
is for a player run (no test, capture or headless switch); a test or a gate sees none of it unless it sets `P4PC_DIAG=1`. `P4PC_DIAG=0` turns it off.

## Where the logs are

| | Folder | Files |
|---|---|---|
| Windows | `%APPDATA%\p4pc\Persona4\` | `p4pc.log`, `p4pc.prev.log`, `p4pc.unclean.log`, `p4pc.ini` |
| Linux | `~/.local/share/p4pc/Persona4/` | the same, and `p4pc-crash.log` |
| macOS | `~/Library/Application Support/p4pc/Persona4/` | the same, and `p4pc-crash.log` |
| Android | the app's private files folder, reached by **Export logs** (below) | `boot.log`, `boot.prev.log`, `boot.unclean.log`, `p4pc-crash.log` |

- `p4pc.log` (Android: `boot.log`) is the current or last run. It is **not** truncated by the next start any more: the run before it is kept as
  `p4pc.prev.log`, and a run that did not end with the `p4diag: clean exit` line (a crash, a kill, a power cut) is also copied to
  `p4pc.unclean.log`, which the next clean run does not replace. A crash followed by a restart therefore does not lose the crash's log.
- A native crash report is in `p4pc-crash.log` on Linux, macOS and Android; on Windows the report is in `p4pc.log` (the last lines before the end).
  Every report starts with the `p4pc: context:` line: the build, the area and the time of the run it is from.
- The logs never include a path of yours in the lines the game adds for diagnostics (the profile folder and the user name are replaced). Lines
  from before (such as the one naming the settings file) can still hold your user name: look before you post a log in public.

**Windows, Linux, macOS:** open the folder (Windows: press Win+R, paste `%APPDATA%\p4pc\Persona4\`), and attach `p4pc.log`, and
`p4pc.prev.log` or `p4pc.unclean.log` or `p4pc-crash.log` if the problem was in an earlier run. `p4pc --help` prints the folder.

**Android:** open the app (the first screen) and tap **Export logs (for a bug report)**. The system's file dialog asks where to save one zip
(for example Downloads); attach that zip. After a crash the app offers it directly. The zip holds the logs, the settings without any path, and
`device.txt` (app version, device, SoC, Android version, screen, memory, battery saver and thermal state at that moment). It never holds saves,
the disc image or game files. Without the export, the only way to read the logs is `adb logcat -d -s p4pc` (the game also prints to logcat).

## The log header

The first lines of every log (`p4diag:` prefix), written before the game starts:

- `build:` the package's `VERSION.txt` (name, commit, build date), found beside the program or up to four folders above it; on Android
  the app's version name and code; otherwise "unknown" and the next line, `program:`, names the file, its size and date.
- `os:`, `cpu:` (model, logical cores, memory), the highest clock of every core when the system allows reading it (a phone's big and little
  cores), and on Android `device:` (maker, model, SoC, platform).
- `settings:` and `video policy:` the effective video, audio and input settings (the environment wins over the file; `(default)` is a
  setting that is not set), `mods:` the enabled mods, load order and packs, `environment:` any `P4PC_*` switches that are set.
- `disc:` the size of the disc image and the executable read from it, and the result of the disc check (`SLUS_217.82` in the image's root:
  Persona 4 USA). Never the path of either.
- `display:` (once, at the first frame) the video driver, the window in pixels, the display and its refresh rate, the swap interval.
- The graphics card is the `p4gl:` line (GL version and driver, renderer, vendor) and the audio output the `p4snd: output` line (below).
  The `p4gl: optional features:` line after it says which optional graphics-driver features exist (persistent buffers, framebuffer fetch, parallel shader
  compile, timer queries, multi-draw, base-vertex draws, program binaries, framebuffer invalidation, ASTC and ETC2); the game runs without any of them.

## Performance lines

One line per ten seconds of play, and one when the area changes (at least 30 frames in it), tagged with the area:

```
p4diag: perf t=212s area="f12:r3 ev 176.1.0" fps=29.8 n=298 avg=33.5ms p50=34 p95=34 p99=51 max=84ms slow=2 hitch=0 cad=0/296/2/0 busy=18.2ms(54%) core=7@2400MHz(80%)
```

- `area` is the field and room (`f12:r3`), the running event's three ids when a cutscene or event task is running (`ev 176.1.0`) and
  `battle` during a battle. `no-scene` is the title or a load.
- `n` flips took `avg`; `p50`, `p95`, `p99` are the flip time that share of flips did not exceed (to the millisecond); `max`.
  A 30 Hz game flips every 33 ms: `slow` counts flips of 42 ms or more (a missed tick), `hitch` 100 ms or more; `cad` counts flips of
  1, 2, 3 and 4 or more 60 Hz ticks.
- `busy` is the game thread's CPU time per flip and its share of the flip. **A share near 100% means the CPU is the limit** (the game
  is running as fast as that core goes); a low share means the thread waited (the display, the GPU, a file).
  `core` (Linux and Android) is the core the game thread was on and how often, with that core's highest clock: a phone running the
  game on a little core shows it here.
- A flip of 200 ms or more also gets its own `hitch` line (at most one a second, a hundred a run) with the CPU time in it.
- Flips of five seconds or more (the app in the background, the machine asleep) are not frames; they are counted in
  `background-gaps` in the summary.

At exit, `summary run=…` has the same figures for the whole run and `summary area "…"` lines give the eight worst areas (by p95, at
least 120 flips). The log ends with `p4diag: clean exit`.

The cost is one clock read and a few additions per flip, an area sample every 15 flips and one line every ten seconds; at most 3000
perf lines a run. Nothing is written per frame.

## Audio

`p4snd: output "<device>" via <driver>: device 48000 Hz, 2 ch, SDL_AUDIO_F32LE, buffer 1024 frames (21.3 ms)` names the output when it opens.
The mixer renders into the device's callback; its own timing is counted there (`src/platform/sound/p4snd_mixer.c`, `P4SndDiag`) and the game
thread prints it:

- `audio underrun? …`: a device callback came more than 1.75 buffer periods after the last one, so the device probably ran dry; the
  line gives the gap, the buffer, the time and the area. (SDL does not report underruns; this is how they are seen.)
- `audio slow render …`: a callback took 80% or more of the time of the audio it made: no margin left.
- `audio lock wait …`: the callback waited 1 ms or more for the mixer lock, which the game thread held.
- One `audio t=…` line with each perf line: callbacks, late callbacks, slow renders, lock waits, **clipped samples** (output beyond ±1
  before it is cut, which sounds like crackle) and the loudest sample, render time (mean, max) and the largest gap since the last line.
  At most 60 event lines a run; the counters keep counting.

## Reporting a problem

The GitHub issue form asks for the package or app version (the `build:` line), the platform, what happened, where and when,
the settings that matter (Resolution, in particular), and the files above. Do not attach the disc image, the game executable, extracted
game files, or save/memory-card files.

## Tests

`p4diag` (frame statistics, the area tag over a fake game memory, the redaction, a whole run's log: header, perf, summary, the clean-exit
line), `p4iso` (the log rotation: previous and unclean logs), `mixer_diag` (a slow callback is counted and reported with the device's
buffer, on SDL's dummy driver), `crash_log` (the context line in the native crash report) and `android_pad_layout` (the Export-logs zip:
its entries, the settings without paths, a long log cut to its end, on a host JDK).
