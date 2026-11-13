# Your first mod in 15 minutes

**What you'll make:** a MODS toggle that controls a short HUD message at the
start of each battle. It is a HUD line, not a native battle message window.

**What you need:** the installed game with your own disc, a text editor, a save
from which you can enter battle, and a port supporting **API 1.6** with the
capability keys this mod declares (`events.battle_start`, `events.battle_end`,
`ui.status`). Complete [setup](install.md) first.

**Time:** about 15 minutes, excluding installation and travel to a battle.

*What you'll see: `A BATTLE HAS STARTED` on the HUD line while the battle runs.*

## 1. Create the package

In the `mods` folder beside `p4pc.ini`, make a folder named `tutorial.first`.
Inside it create `mod.ini`:

```ini
[mod]
id = tutorial.first
name = First battle notice
version = 1.0.0
api = 1.6
priority = 0
order = 0
requires = events.battle_start@1, events.battle_end@1, ui.status@1

[setting notice]
kind = toggle
default = on
label = BATTLE NOTICE
help = Show a message at the start of battle.
apply = live
```

Each of the six `[mod]` keys before `requires` is required, each exactly once,
in one `[mod]` section. The id, not the folder name or display name, identifies
the mod. `api = 1.6` is the release minor the manifest understands; `requires`
names the features this script uses. The **HUD** placement of `p4.ui.status`
is covered by the `ui.status` key; settings themselves need the declared
`[setting]` section. See [capabilities](../capabilities.md) for the key list
and diagnostics.

**If the package does not appear in MODS:** check the location, spelling and
extension (`mod.ini`, not `mod.ini.txt`), and confirm that the manifest has all
six keys. An explicit mod directory overrides the default. See
[package rules](../files.md#package).

## 2. Add the behavior

In the same folder create `main.luau`:

```luau
--!strict
-- Show a HUD line for each battle that begins while the toggle is on.
p4.events.on("battle_start", function(_event)
    if p4.settings.get("notice") then
        if not p4.ui.status("A BATTLE HAS STARTED", "hud") then
            p4.log("No room for the battle notice")
        end
    end
end)

p4.events.on("battle_end", function(_event)
    p4.ui.status(nil)
end)
```

`battle_start` reads the current toggle value. If on, it requests the line;
`battle_end` clears it. This live setting is checked at **the next battle
start**: switching it off during a battle does not remove the already-set line.
There are four status rows shared by mods. If earlier mods fill them, the call
returns false and the log explains why this example did not show a line.

The complete copyable package is also in
[`examples/tutorials/00-first-mod`](../../../examples/tutorials/00-first-mod/).
You can optionally [type-check it](install.md#optional-type-checker).

**If a script error appears:** make sure both files are directly inside
`tutorial.first`, saved as plain text, and that the quotes and parentheses
match the example. Read the named error in the game log; do not assume a
successful static check means the script ran in game.

## 3. Enable and try it

1. Launch the game, open **Config** (F1 or Esc on desktop), choose **MODS**,
   then open **MOD tutorial.first**. Set **ENABLED AFTER RESTART** to on and
   restart the game: package selection changes take effect on restart.
2. In that package's MODS page, find **BATTLE NOTICE** and leave it on. Load
   your save and enter a battle. The line should appear, then clear when the
   battle ends.
3. Go back to the same MODS page and turn **BATTLE NOTICE** off. Enter another
   battle: the line should not appear. Turning the toggle back on affects the
   next battle. The setting is remembered separately from your save.

**If you see no line:** verify the mod is enabled after restart, the toggle is
on, and that you entered a *new* battle after changing it. Check the log for
`No room for the battle notice`; another mod may occupy the shared status rows.
If the mod is refused, check its reported `api`/`requires` refusal and use an
API 1.6 port providing those keys. See [troubleshooting](../troubleshoot/README.md).

## Next

Read [events, settings and save behavior](../systems/README.md) or look up the
[API versions](../reference/README.md). Named skill edits are available
through [game data](../systems/data-and-edits.md); ailment reactions and
message-window tutorials do not exist yet. The [tutorial index](../learn/README.md)
will link to them when their capabilities are available.
