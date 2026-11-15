# Learn by building

[Modding home](../README.md)

Start with [Your first mod in 15 minutes](../start/first-mod.md): a setting and
battle-start HUD line using the current API. Its complete source lives in
[`examples/tutorials/00-first-mod`](../../../examples/tutorials/00-first-mod/).

More goal-based tutorials will be published with the capabilities they use,
not before. Named skill power/cost, enemy HP and shop stock are available now
through [game data](../systems/data-and-edits.md) (`data.luau`/`data/edits.xml`,
capabilities `data.edit@1`/`data.xml@1`) rather than a tutorial, and ailment
reactions and in-run changes go through the stage APIs and `data.session@1`.
Battle start is not an ailment event and the HUD line is not a
message-window API.
