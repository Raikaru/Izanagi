Persona 4 PC - test build @VERSION@  (@ARCH@)
==========================================================

An unofficial, unfinished PC port of Shin Megami Tensei: Persona 4 (USA, SLUS-21782).
It contains NO game data. You need your own copy of the game: a disc image (.iso) of
your own Persona 4 (USA) PlayStation 2 disc. Normal game launches do not download
game data or upload anything.

@BUILDTYPE@
Build: @BUILDLINE@
@UNLANDED@

Starting
--------
1. Unzip the whole folder anywhere (keep the .dll files next to @EXE@).
2. Double-click @EXE@.
3. The first time, a file dialog asks for the disc image. Pick the .iso of your own
   Persona 4 (USA). The game checks that it is the right disc, remembers the path
   (p4pc.ini, see below) and reads its program file (SLUS_217.82) from the disc.
   If you move the image, delete the "iso = ..." line in p4pc.ini and start again.
4. The game starts in a normal window, paced to the PS2's 60 Hz clock (normally
   30 presented frames per second). Audio is on.

Controls (defaults; rebind in Config -> Controls)
-----------------------------------------------
  Move / D-pad     arrow keys or W A S D
  Cross (confirm)  Z          Circle (back)   X
  Triangle         C          Square          V
  Start            Enter      Select          Backspace
  L1 / R1          Q / E      L2 / R2         1 / 3
  L3 / R3          Left Shift / Right Shift
  Fullscreen       F11 or Alt+Enter
@LOCKSTEP_KEY@

Gamepad: any controller SDL knows (Xbox, PlayStation, Switch Pro ...) works when
plugged in before or during the game. South button = Cross, East = Circle,
West = Square, North = Triangle, Start / Back = Start / Select, shoulders = L1 / R1,
triggers = L2 / R2, sticks and D-pad as expected.
Stick sensitivity now matches PCSX2's default: full tilt keeps running despite
small rim jitter or off-cardinal headings (including XInput/8BitDo controllers).
The optional [input] deadzone setting defaults to 0; the game has its own dead
zone. Existing settings are kept; remove deadzone from p4pc.ini to use the default.

PC settings and PS2 save import
------------------------------
Open Config with Esc or F1, or hold gamepad Select + Start for 0.6 seconds.
It uses the game's own Config task (also reached from Camp -> System -> Config).
PC rows include render Resolution, Aspect ratio, Movie fit and Controls.
Resolution can use native fitted window pixels, a fixed size such as 1920x1080,
or an integer scale. Resolution changes live; Aspect ratio changes need a restart.
16:9 widens the view rather than stretching the interface. Movie fit affects only
movies: bars preserves the full picture, zoom fills/crops, stretch fills freely.
Key/button rebinding and these choices are saved in p4pc.ini across launches.

Config -> Import save accepts PS2/PS3 save containers (including CBS, MAX and PSV),
PCSX2 memory-card images and folder-card saves. Unzip downloads first. Import
never replaces the running party: return to LOAD GAME to use the imported save.
Normal imports use an empty slot, keep the source unchanged and never overwrite
an existing save. Back up your save folder before moving or replacing saves.

Where things are
----------------
  @APPDATA@
      p4pc.ini        settings (window size, fullscreen, volume, key bindings,
                      the path of your disc image). Plain text; delete a line to
                      get its default.
      save\           your saves: the game's own memory-card folder. Copy it to
                      back up your progress.
      p4pc.log        what the game printed on its last run. Attach it when you
                      report a problem.
      SLUS_217.82     the game's program file, read from your disc.
The folder is not inside the unzipped folder, so the zip can be replaced by a newer
build without touching your saves.

Optional mods made for this port ship in the mods folder beside the game program
(the mods folder in this archive). None of them is active: choose one in Config >
MODS, set its options on its MODS page, and restart. A copy with the same id in
your own mods folder beside p4pc.ini replaces the shipped one.

Hard mode preview: with BOSS TACTICS on, Avenger Knight, Shadow Yukiko and
Shadow Kanji use their own actions and changing phases to signal dangerous
moments. Golden-row boss tuning remains provisional and still needs final
in-game validation.

Fusion Arts offers WEAPON FUSION after Yukiko's Castle is cleared. Enable the
mod and its WEAPON FUSION option, then restart. In the Velvet Room, use Square
on the root menu to open the forge. Choose a fighter, an unequipped weapon and
a carried Persona; the confirmation shows the result and fee. Forging consumes
the weapon and Persona and cannot be undone. The equipped or last Persona cannot
be consumed. You may equip a successful result immediately; your old equipped
weapon returns to your items (EQUIP NOW is unavailable while you hold 99 of it).
Physical Split, if enabled, also types forged weapons.

The optional API example in examples\mods\p4pc.example is not installed or
enabled by the archive. To try it, copy that folder to the mods folder beside
p4pc.ini, enable p4pc.example in MODS, and restart. It logs time-slot setter calls;
its counter is restored from the mod save sidecar when you load. Its COUNT TIME
SLOTS row under MODS turns the counter on and off, and it logs the text file it
ships in files\DATA.

To make a mod, use p4mod.exe from a command prompt in this folder: "p4mod --help"
lists its commands. "p4mod setup --disc <your disc image>" once, then "p4mod new",
"p4mod check", "p4mod test" and "p4mod pack". p4.d.luau beside it gives a Luau
editor the API's types.

This is a test build
--------------------
@BUGREPORT@
Parts of the game are not finished and some things will look or sound wrong. Please
note what you were doing, the game's day and place, and send p4pc.log and your save
folder with a report. Never attach your disc image, its program file (SLUS_217.82)
or memory-card files to a public issue.

Fresh Windows package checks cover New Game through the fog-dream battle to
04/15, saving and loading, voiced 04/17 event E176 through to 04/18, and the
04/17 castle story through the 2F landing. They also exercise native PC settings,
1920x1080 native rendering, 16:9 all-out cut-ins, saved key bindings/movie fit,
CBS save import and LOAD GAME into Heaven's 10th Paradise, plus the Marukyu
post-Rise event through Teddie's first command menu (not Teddie victory).

The wider checkpoint suite covers Yukiko's rescue and the post-rescue classroom,
Bathhouse 7F visit/escape, later collection-save story checkpoints through
Heaven, December branches, Christmas and the March 20 farewell/Margaret event.
These are independent loaded-save/checkpoint tests, not a continuous New Game
playthrough, all dungeon floors, every boss victory or a certified ending.
Level-99 retail-save system checks cover dungeon navigation, menus, shops,
fusion and the compendium. This is not a full-game playthrough.

Scene fixes in this release series: voiced events now complete; the fog-dream
battle draws its stage floor and correct fog wash; sprite and letterbox edge
pixels follow the PS2's fill rule; rain particle sizing is corrected; disc
completion ordering now preserves the story's weather/fog selection; haze
interpolation and material sampling are corrected. The automated boss-battle
input sequence now uses a sustainable attack plan; this is a test-driver fix,
not a change to the player's combat timing, stats or balance.

Also fixed: the Dojima living room's tatami could turn bright white after a long
continuous play session; shadow render targets are now released correctly.
Dark cross-shaped bands in the dream battle's fog background are fixed. Yu is
visible there, but intentionally heavily washed out by the game's original fog;
this is not a reduced-fog or enhanced-visibility mode.

Known visual differences: the fog-dream stage floor and fog wash have been fixed,
but the TV outline animation phase and COMMAND ring angle can differ from retail.
TV-entrance haze still has clipping differences; near-fog noise accumulation and
some animated lamps, fog and character poses also differ from retail captures.

Licences
--------
See THIRD_PARTY_NOTICES.txt and the licenses folder. The movie decoder is FFmpeg
(LGPL 2.1 or later, as DLLs you can replace), the window and input library is SDL3
(zlib).
Both Windows package architectures run a mandatory LGPL-only licence check against
their shipped FFmpeg DLLs before the zip is produced.
