# p4mod: make, check, test and pack a mod

`p4mod` ships beside the game: `p4mod.exe` in the Windows zip, and inside the Linux
AppImage as `p4pc-linux64-<version>.AppImage p4mod ...`. It needs no Python, no
compiler and no reverse engineering. Every command prints `--help`, and exits 0 on
success, 1 when the mod has errors or a test failed, and 2 on a usage or file
problem.

```
p4mod setup --disc "D:\Games\Persona 4.iso"   # once
p4mod new yourname.first-mod                   # creates a folder
p4mod check yourname.first-mod                 # validate the new mod
p4mod test yourname.first-mod                  # run a quiet boot with only this mod
p4mod test yourname.first-mod --scenario battle --save SAVE --encounter ID  # one real battle
p4mod pack yourname.first-mod                  # make a zip after checking
```

## setup

`p4mod setup --disc <image>` checks that the image is the Persona 4 (USA) disc
the port is built for: its `SLUS_217.82` must be the retail executable. It then
records the path in the game's own settings file (`p4pc.ini`, `iso` under
`[game]`), so the game and `p4mod test` both find it. Every other line is kept,
and the file is written as the game writes it: the new text into a temporary
file beside it, then renamed over it, so a failure leaves the old file whole.
Nothing else is copied or changed. A disc path the game could not read back exactly from
`p4pc.ini` (a line break or other control character, `;` or `#` after a space, or a space
at either end) is refused before anything is written. The settings file is `P4PC_CONFIG` if that is set, otherwise
`%APPDATA%\p4pc\Persona4\p4pc.ini` on Windows and
`~/.local/share/p4pc/Persona4/p4pc.ini` on Linux.

## new

`p4mod new <id>` creates `<id>/` in the current folder (`--dir` picks another)
with a `mod.ini` at this build's API version and a `README.md`. The default
template, `--luau`, adds a strict-typed `main.luau` that shows a status line
and logs a line when a battle starts, and its `requires=` names the two
capabilities it uses.
`--files` makes a replacement-files mod instead, with an empty `files/DATA/`. Ids
are 1-63 lowercase letters, digits, `.`, `_` or `-`, not starting or ending with
`.` (the game's own rule), for example `yourname.better-agi`. If creating the
folder fails part-way, the new folder is removed; nothing that existed is touched.

### Try your first Luau mod

Run the commands above from a terminal in the folder where you want to keep
your mod. Open `yourname.first-mod/main.luau` in a text editor. The last line,
`p4.log("yourname.first-mod ready")`, runs when the script loads. Change `ready`
to another message, save the file, then run `p4mod check yourname.first-mod` and
`p4mod test yourname.first-mod` again. The boot test prints that message if the
script loads; its PASS means the title reached its input-ready state, not that
every callback ran.

The `battle_start` callback in the same file sets a battle HUD status and logs
`yourname.first-mod saw a battle start`. A boot or new-game PASS does not test
that callback; `p4mod test yourname.first-mod --scenario battle ...` ([battle](#battle))
does, and prints that line if the callback ran.
See [Luau mods](lua.md) for the API and [File mods](files.md) for installation.
This starter is an event example; it does not change Agi or any other game data.

## check

`p4mod check [folder]` validates a mod the way the game loads it, and prints one
line per finding: `file:line:column: level: what. why. fix.`

- **mod.ini** follows the game's rules ([File mods](files.md)): exactly one
  `[mod]` with `id`, `name`, `version`, `api`, `priority` and `order`, no unknown
  or repeated keys, section names and keys the game accepts, an `api` no newer
  than this build, `requires=` keys this build provides at the revision named
  ([Capabilities](capabilities.md)), and only sections this build reads.
- **Package files** are found through the game's own package index: names match
  ASCII case-insensitively (`Data/Edits.xml` is `data/edits.xml`), and a link,
  reparse point, special file, executable, name with `\ : ;` or control
  characters, or two names that differ only in case makes the game refuse the
  whole folder, so check does too, naming the entry.
- **main.luau** is type-checked in strict mode against `p4.d.luau`, the same
  declarations the game's API is built from. Point your editor's Luau language
  server at the `p4.d.luau` beside `p4mod` for the same types while you write.
  `p4.data.edit` there is an error: the game raises one when it is called outside
  `data.luau`.
- **data.luau** is type-checked in strict mode against only the `p4` it gets
  while the game builds its tables ([Data edits](systems/data-and-edits.md)):
  `p4.data` (its readers and `edit`), `p4.log`, `p4.has`, `p4.capabilities`,
  `p4.settings.get` (a toggle's boolean or a choice's string) and `p4.id`.
  Anything else, such as `p4.events.on`, is a type error with a note to move that
  code to `main.luau`. It must be at most 1 MiB. The rows and values it edits are
  not run by check.
- **data/edits.xml** (at most 256 KiB) goes through the game's own decoder and
  field rules: well-formed XML, the `<p4-edits schema="1">` root, rows and
  children, attributes, slots, whole numbers and their ranges, element, affinity
  and ailment names, and learn levels up to 99. A refusal names the line, as the
  game would; numbers past 4294967295 are refused, never wrapped. Rules that need
  your disc's tables (that a named row exists, a skill is not passive, an item's
  kind, a Persona's base level and learnset) are counted in a note and left to
  `p4mod test`. So are the game's cross-field rules (every skill's hits.min <= hits.max,
  each enemy's drop chances at most 100% in all), which it checks on the final rows
  after the mod's `data.luau` has run too: check names them in a note when the file
  writes those fields.
- **[table-source]** sections go through the game's own `when` gate rules first (a
  declared toggle `[setting]` with `apply = restart`, at most one `when`), then its
  metadata rules (`owner:key`, `file`, `format=p4-tables-xml`, `schema=1`,
  `revision`, unique keys) and their file (at most 8 MiB, found through the index)
  through the game's own table decoder: XML, no DTDs or NUL bytes, one
  `<p4-tables schema="1">` root, flat records, each record's required and known
  attributes and their values. A section whose toggle is off by default is still
  checked, but its problems are warnings ("with <key> on"), since the game loads it
  only once the player turns the toggle on. What the game checks when it registers
  rows (the IDs and keys they claim, family state) is left to `p4mod test`, with a note.
- **Capabilities used.** Each `p4.` function, event name or manifest section the
  mod uses, each package file a capability covers (`data/edits.xml`: `data.xml@1`),
  item renames (`data.item_names@1`), shop fields (`data.shop@1`) and dungeon rows
  (`data.dungeons@1`) in either `data.luau` or `data/edits.xml`, and each
  table-source feature the decoded records use (`status-ailment` rows, skill
  `derive`, `effect-source` and `delivery`, item `derive`: [Tables](tables.md)) maps
  to the capability key that provides it. A key the mod uses but `requires=` does
  not list is a warning: list it, so an older build refuses the mod with a clear
  message instead of failing later. Build-specific raw access
  (`p4.hook`, `p4.call`, `p4.fn`, `p4.describe`, `p4.mem`; `advanced@1`) is a note:
  those addresses can change with port updates, so prefer a semantic capability.

## test

`p4mod test [folder]` runs the check, then starts the game with only this mod
enabled: hidden window, no sound, scripted input, and a fresh, uniquely created
save and settings folder under the system temporary folder for every invocation,
private to your user account (mode 0700, or an owner-only ACL on Windows).
Existing folders are never cleared, and your own saves and settings are never touched. It prints
the mod's log lines and `PASS` or `FAIL` with the reason: the game refused the
mod, refused its data edits (the game's line naming the row, field or value is
shown), a callback failed (the error line is shown), or the game did not reach the
scenario. The mod folder is copied for the run only after every entry in it is
checked to be an ordinary file or folder; a link or special file stops the test
before anything is copied. The game started is the one beside `p4mod` as the
system reports p4mod's own location (never the current folder), or `--game`.

| `--scenario` | Runs until |
|---|---|
| `boot` (default) | the title accepts input, after `main.luau` ran |
| `newgame` | New Game is confirmed and the Dojima living room (field 7, room 2) is reached, so frame and field callbacks run |
| `battle` | one real battle from a save of yours ends: won, lost, escaped or concluded by the story |

`--frames N` moves the stopping point (1 to 1000000: in presented frames, 900 for
`boot` and 30500 for `newgame`; in completed battle updates, 100000, for `battle`).
Boot fails if the title has not accepted input by that point; New Game fails if the
room has not been reached; a battle fails if it has not ended. Reaching the frame alone is
not a pass. The verdict comes from the result file the game itself writes at that
point (`P4PC_GATE_RESULT`, `gate-result.txt` in the work folder): the title state
it saw, the New Game confirmation and the field and room. Log lines, which a
mod's `p4.log` can imitate, can only turn a run into a FAIL, never a PASS; the
mod lines echoed are shown with control characters replaced by `?`. A mod that
writes the game's memory directly (`advanced@1`) can still change what the game
sees. `--game` names the game executable
when it is not beside `p4mod`.
Every run's whole log and other evidence stay in its work folder, whose path is printed by the command.
On Windows the game opens files through the ANSI code page, so p4mod refuses, with exit
status 2, any command line with a character beyond ASCII (before Windows could best-fit it
onto a different ASCII path) and any mod folder, resolved mod folder (through junctions),
game, disc image, settings file or temporary folder whose path is not plain ASCII.

### battle

```
p4mod test yourname.first-mod --scenario battle --save "%APPDATA%\p4pc\Persona4\save\mc0\BASLUS-21782Save0" --encounter <ID>
```

The battle runs in the game's own battle simulator
([Native battle simulator](../platform/battle_sim.md)): the real party, enemy, AI,
damage and random-number code, with no window. It needs:

- **`--save`**, one of your saves: a slot folder `BASLUS-21782Save<N>` as the game
  writes it (players' saves are in `save\mc0\` beside `p4pc.ini`) or the data file
  of the same name inside it, or one brought in with `p4pc --import-save`. It is read
  once and never opened for writing; `p4mod` checks that it is a complete save image
  (version, records and checksums, as the game checks it) before anything starts, and
  the battle's copy lives in the work folder. A link or special file is refused. The
  battle uses that save's party, Personas, items and HP/SP as saved; nothing is
  healed or prepared. Mod save data (`.p4mods` beside the data file) is not
  carried: a save provider is loaded with nil, as for a save made without the
  mod, and the test says so.
- **`--encounter`**, the battle: a native encounter ID (decimal or `0x` hex), or
  its key in the game's catalog ([Data](systems/data-and-edits.md#keys)). An encounter
  is a *formation*, the `id` of `p4.data.formation(id)` and `p4.encounter.info(id)`.
  Formations have keys (`p4:formation:#17`, or the key a mod gives a formation it
  adds) but **no names**, so `--encounter` takes no name; to find one, look it up
  from a mod, for example through `p4.data.floor(...).encounters` or by logging
  `p4.data.formation(id).enemies`. An ID the loaded table does not have ends as
  `engine_error` (`invalid_encounter`).
- **who chooses the party's commands**: `--policy typical` (default), `greedy`,
  `cautious` or `attentive`, the simulator's native players, or `--plan FILE`, a
  battle plan of `battle:`, `battlerule:` and `battleturn:` lines (printable
  ASCII, at most 16384 bytes once resolved; the syntax is in
  `include/p4pc/walk_script.h`). Wherever a plan takes a native skill, item or
  enemy ID, it may hold a name or key in double quotes instead:

  | Where | Kind |
  |---|---|
  | `skill:"..."` | skill |
  | `item:"..."` | item (a battle item, ID 0x301 to 0x3FF) |
  | `enemy:"..."`, `charge:"..."`, `guard="..."`, `target1="..."` to `target8="..."` | enemy |

  ```
  battle:first=circle,down,cross,skill:"Agilao",cross;target1="Cowardly Maya"
  battleturn:enemy:"p4:enemy:cowardly-maya"=circle,down,cross,skill:"Agi",cross
  battlerule:dead@1=down,down,cross,item:"Revival Bead",cross
  ```

  Party members (`ally:`, `dead:`, `@1,2`, `first1=`) stay native character IDs
  1 to 8: the catalog does not name them. Unquoted numbers stay native IDs.
- `--seed N` (default 0) picks the battle's random-number seeds, derived as
  `tools/battle_sim.py` derives run `N` of master seed 0.

**Names are the game's own.** The catalog is read from your disc while the game
starts, together with the rows mods rename or add, so `p4mod` does not
guess IDs: when the encounter is a key or the plan holds quoted names, the test
first boots the game once (into `resolve/` in the work folder, to its frame-900
gate as the game's own result file reports) with your mod and a private resolver
package that asks `p4.data` for each one and writes the answers to its own
`p4.artifacts` file (`resolve/mod-artifacts/p4mod.resolver/answers.txt`), which the
game lets only that package write. Log lines are never read as answers; a missing,
incomplete or repeated answer fails the test.
Names follow the catalog's rules: case, spaces and punctuation don't matter, a key
always works, a name the disc shares between rows is refused with the candidates'
keys, and an unknown one with the nearest names. Each problem is printed as
`<plan file>:line:column: error: ...` (or `--encounter: error: ...`), and the
battle does not start. Each resolved name is printed with its ID and key, and the
resolved plan the battle ran is kept as `plan.txt` in the work folder.

Only this mod is enabled in the battle, as the run's declared mod set: `p4mod`
names its files by a SHA-256 identity, the game must report exactly this mod
and version active in the battle's result, and the run is mod-witness output,
never a retail reference. Inherited `P4PC_*` and `SDL_*` settings are dropped,
and an inherited `P4PC_MOD_WITNESS` is refused. It prints the mod's log lines,
the simulator's lines and the result:

```
p4mod test: battle win (native_battle_exit_victory) after <N> native updates; encounter <ID>, typical-player-v1, seed 0
  active mod: yourname.first-mod 0.1
p4mod test: PASS (evidence in ...)
```

The verdict comes from the simulator's own `result.json`; as for the other
scenarios, log lines can only add a failure. PASS means the battle reached its
native end (`win`, `defeat`, `escape` or `scripted_conclusion`) with the mod
enabled, active, its data edits accepted and without a mod error. A defeat
passes: your mod may make a fight harder; the outcome line says which. It fails
when the save is not a save image, a name did not resolve, the game refused or
dropped the mod, a callback failed, the battle did not end within `--frames`
updates (`update_limit`/`action_limit`), the simulator could not run it
(`engine_error`/`policy_error`, with its cause: a plan it cannot parse stops
before the battle, one it cannot follow ends as
`policy_error`/`walk_battle_plan_failed`), or the result names another
encounter, mod set or policy. The work folder keeps `entry.bin` (what the
simulator ran), `result.json`, `trace.jsonl`, the staged mod and the logs.

## pack

`p4mod pack [folder]` runs the check and, when it has no errors, writes
`<id>-<version>.zip` beside the folder (`--out` names another file); `p4mod pack .`
writes beside the current folder, never inside it. A version that is not a plain
file name (letters, digits, `.`, `_`, `-` or `+`, not starting with `.`) cannot
name the zip: pass `--out`. The zip's single top folder is the mod id and holds
every file the game's package index accepts except hidden files and zips, so
players unzip it straight into their `mods` folder.
The output must not already exist; choose another name rather than overwriting it.
A failed write removes the partial output and nothing else.
Archives use ZIP32, not ZIP64: packages that exceed its entry count, name,
file-size or archive-size limits are refused without creating an output.
Package entries must be ordinary files or directories: symlinks, reparse points
and special files are refused, as in the game's [package rules](files.md#package).
On Wine 10, Unix host symlinks can be exposed to Windows applications as their
targets rather than as reparse points. The Windows build cannot reliably detect
such links there: do not use it under Wine to check or pack an untrusted mod
tree. Use the native Linux build or Windows instead.
Keep the source tree unchanged during the scan and pack; this is not an audit of
an adversarially changing filesystem.
