Persona 4 PC - test build @SHA@  (macOS @ARCH@)
==========================================================

An unofficial, unfinished PC port of Shin Megami Tensei: Persona 4 (USA, SLUS-21782).
It contains NO game data. You need your own copy of the game: a disc image (.iso) of
your own Persona 4 (USA) PlayStation 2 disc. Nothing is downloaded and nothing is
sent anywhere.

Build: @BUILDLINE@
@UNLANDED@

This package is for @MACHINE@ (@ARCH@). Use the other macOS package on the other kind
of Mac. It needs macOS @MACOS_MIN@ or newer.

Starting
--------
1. Unzip the package and move "Persona 4 PC.app" wherever you like (Applications is
   fine). Keep README.txt for later.
2. The app is not notarized by Apple: it is signed only with an ad-hoc signature,
   because the project has no Apple Developer ID. The first time, macOS refuses to
   open it ("cannot be opened because the developer cannot be verified", or "is
   damaged"). Either:
     - right-click (or Control-click) the app, choose Open, then Open again in the
       dialog; on macOS 15 and newer, open it once, then in System Settings >
       Privacy & Security click "Open Anyway" next to the message about the app; or
     - in Terminal, remove the quarantine flag the download gave it, then open it
       normally:
         xattr -dr com.apple.quarantine "/path/to/Persona 4 PC.app"
   You only do this once per copy of the app.
3. The first time, a file dialog asks for the disc image. Pick the .iso of your own
   Persona 4 (USA). The game checks that it is the right disc, remembers the path
   (p4pc.ini, see below) and reads its program file (SLUS_217.82) from the disc.
   If you move the image, delete the "iso = ..." line in p4pc.ini and start again.
4. The game starts in a normal window, paced to the PS2's 60 Hz clock (normally
   30 presented frames per second). Audio is on.

Graphics: OpenGL ES 3.0 through ANGLE on Metal (the libraries are inside the app).
If that cannot start, the game falls back to Apple's own OpenGL. To draw with
Apple's OpenGL from the start (for example if the picture is wrong on Metal), start
the app from Terminal with P4PC_GL_API=gl in the environment:
    P4PC_GL_API=gl "/path/to/Persona 4 PC.app/Contents/MacOS/p4pc"

Controls (keyboard; change them in p4pc.ini, section [input])
-------------------------------------------------------------
  Move / D-pad     arrow keys or W A S D
  Cross (confirm)  Z          Circle (back)   X
  Triangle         C          Square          V
  Start            Enter      Select          Backspace
  L1 / R1          Q / E      L2 / R2         1 / 3
  L3 / R3          Left Shift / Right Shift
  Fullscreen       F11 or Alt+Enter (Option+Enter; on a laptop keyboard F11 may need fn)

Gamepad: any controller SDL knows (Xbox, PlayStation, Switch Pro ...) works when
connected before or during the game. South button = Cross, East = Circle,
West = Square, North = Triangle, Start / Back = Start / Select, shoulders = L1 / R1,
triggers = L2 / R2, sticks and D-pad as expected.

Where things are
----------------
  ~/Library/Application Support/p4pc/Persona4/
      p4pc.ini        settings (window size, fullscreen, volume, key bindings,
                      the path of your disc image). Plain text; delete a line to
                      get its default.
      save/           your saves: the game's own memory-card folder. Copy it to
                      back up your progress.
      p4pc.log        what the game printed on its last run. Attach it when you
                      report a problem.
      p4pc-crash.log  retained native crash details (attach with p4pc.log).
      SLUS_217.82     the game's program file, read from your disc.
(In Finder: Go > Go to Folder..., then paste the path above.) The folder is not
inside the app, so the app can be replaced by a newer build without touching your
saves.

Optional mods made for this port ship in the mods folder beside the game program
(Contents/Resources/mods inside the app). None of them is active: choose one in
Config > MODS, set its options on its MODS page, and restart. A copy with the same
id in your own mods folder beside p4pc.ini replaces the shipped one.

Fusion Arts offers WEAPON FUSION after Yukiko's Castle is cleared. Enable the
mod and its WEAPON FUSION option, then restart. In the Velvet Room, use Square
on the root menu to open the forge. Choose a fighter, an unequipped weapon and
a carried Persona; the confirmation shows the result and fee. Forging consumes
the weapon and Persona and cannot be undone. The equipped or last Persona cannot
be consumed. You may equip a successful result immediately; your old equipped
weapon returns to your items (EQUIP NOW is unavailable while you hold 99 of it).
Physical Split, if enabled, also types forged weapons.

The package contains an inert example in examples/mods/p4pc.example. To try it, copy
that folder into the mods folder beside p4pc.ini, enable p4pc.example in MODS, and
restart. It logs time-slot setter calls; its counter is restored from the mod save
sidecar when you load. Its COUNT TIME SLOTS row under MODS turns the counter on
and off, and it logs the text file it ships in files/DATA.

This is a test build
--------------------
Parts of the game are not finished and some things will look or sound wrong. Please
note what you were doing, the game's day and place, and send p4pc.log and your save
folder with a report. The macOS packages are the newest of the ports and are built
and tested only on GitHub's macOS build machines: they have had no play testing on
real Macs yet.

Licences
--------
See THIRD_PARTY_NOTICES.txt and the licenses folder. The LGPL FFmpeg libraries are
separate files in "Persona 4 PC.app/Contents/Frameworks" and can be replaced with
ABI-compatible builds (sign the app again afterwards: codesign --force --deep -s -
"Persona 4 PC.app"). The package is made only after an LGPL-only FFmpeg licence check
passes (no GPL or nonfree components).
