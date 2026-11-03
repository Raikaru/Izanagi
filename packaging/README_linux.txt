Persona 4 PC - test build @VERSION@  (Linux @ARCH@)
==========================================================

An unofficial, unfinished PC port of Shin Megami Tensei: Persona 4 (USA, SLUS-21782).
It contains NO game data. You need your own copy of the game: a disc image (.iso) of
your own Persona 4 (USA) PlayStation 2 disc. Nothing is downloaded and nothing is
sent anywhere.

Build: @BUILDLINE@
@UNLANDED@

Starting
--------
1. Make the AppImage executable: chmod +x p4pc-linux64-*.AppImage
2. Run it. Without FUSE, add --appimage-extract-and-run.
3. The first time, a file dialog asks for the disc image. Pick the .iso of your own
   Persona 4 (USA). The game checks that it is the right disc, remembers the path
   (p4pc.ini, see below) and reads its program file (SLUS_217.82) from the disc.
   If you move the image, delete the "iso = ..." line in p4pc.ini and start again.
   Without a file dialog on your system (no portal or zenity), put the path of the
   image in p4pc.ini by hand: a line  iso = /path/to/image.iso  in the section [game].
4. The game starts in a normal window, paced to the PS2's 60 Hz clock (normally
   30 presented frames per second). Audio is on.

Needs: a 64-bit Linux with a glibc at least as new as Debian 13's (2.41), X11 or
Wayland, and OpenGL ES 3.0 or OpenGL 3.3 core (Mesa or a vendor driver). VERSION.txt
lists the system libraries the program loads.

Controls (keyboard; change them in p4pc.ini, section [input])
-------------------------------------------------------------
  Move / D-pad     arrow keys or W A S D
  Cross (confirm)  Z          Circle (back)   X
  Triangle         C          Square          V
  Start            Enter      Select          Backspace
  L1 / R1          Q / E      L2 / R2         1 / 3
  L3 / R3          Left Shift / Right Shift
  Fullscreen       F11 or Alt+Enter

Gamepad: any controller SDL knows (Xbox, PlayStation, Switch Pro ...) works when
plugged in before or during the game. South button = Cross, East = Circle,
West = Square, North = Triangle, Start / Back = Start / Select, shoulders = L1 / R1,
triggers = L2 / R2, sticks and D-pad as expected.

Where things are
----------------
  ~/.local/share/p4pc/Persona4/   (or $XDG_DATA_HOME/p4pc/Persona4/)
      p4pc.ini        settings (window size, fullscreen, volume, key bindings,
                      the path of your disc image). Plain text; delete a line to
                      get its default.
      save/           your saves: the game's own memory-card folder. Copy it to
                      back up your progress.
      p4pc.log        what the game printed on its last run. Attach it when you
                      report a problem.
      p4pc-crash.log  retained native crash details (attach with p4pc.log).
      SLUS_217.82     the game's program file, read from your disc.
The folder is not inside the unpacked one, so the package can be replaced by a newer
build without touching your saves.

The AppImage contains an inert example in examples/mods/p4pc.example. To try
it, extract the AppImage, copy that folder into the mods folder beside
p4pc.ini, enable p4pc.example in MODS, and restart. It logs time-slot setter calls;
its counter is restored from the mod save sidecar when you load.

This is a test build
--------------------
Parts of the game are not finished and some things will look or sound wrong. Please
note what you were doing, the game's day and place, and send p4pc.log and your save
folder with a report. Never attach your disc image, its program file (SLUS_217.82)
or memory-card files to a public issue. The Linux package has had less play testing
than the Windows x64 zip.

Licences
--------
See THIRD_PARTY_NOTICES.txt and licenses inside the AppImage. Extract with
--appimage-extract; LGPL FFmpeg libraries in squashfs-root/usr/lib can be replaced
with ABI-compatible builds, then run squashfs-root/AppRun. Complete SDL/FFmpeg
source and the build recipe are in usr/share/p4pc/source. The package is made only
after an LGPL-only FFmpeg licence check passes (no GPL or nonfree components).
