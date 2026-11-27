# Native battle simulator

The battle simulator runs the game's native battle code for testing; it is not a
mod or a replacement for playing the game. Its results are not, by themselves,
proof of PlayStation 2 behavior or of compatibility across every encounter.

## Test a mod

Use [`p4mod test --scenario battle`](../modding/p4mod.md#battle) to run a mod
against a chosen formation and a copy of your own save. The guide explains the
save and disc requirements, party policies, battle plans, and result files. The
source save is not opened for writing. Do not publish saves or game data with a
bug report.

## Coordinator and worker

`tools/battle_sim.py` coordinates scenario batches and invokes a native player
worker. It compiles the scenario and save into a bounded binary entry rather than
accepting arbitrary guest-memory writes. The worker writes a JSON result and a
JSONL trace to separate output files. Outcomes distinguish a victory, defeat,
escape, scripted conclusion, limits, and errors; reaching a limit is not a win.
Run `python3 tools/battle_sim.py --help` for the coordinator's options.

The native simulator and party policies are implemented in Zig and retain the
game's C ABI. Zig safety failures print a diagnostic and abort; they are not
recorded as successful outcomes. The Python coordinator schedules workers and
records evidence; it does not resolve combat. The separate Linux fork-search
tool `tools/battle_tas.py` remains Python.

## Expert candidate policy

Policy 9 (`{"kind":"expert","version":1}`) is a study candidate, not a human
benchmark. It scores only a copied observation of the party, visible enemy status
and buffs, the native legal menus and targets, owned Personas and remembered hit
outcomes; it keeps no enemy HP, nonpublic status bits, hidden affinities or RNG.
Its priorities are emergency recovery, Guard on a fresh visible charge, removal of
a visible party debuff, an observed weakness, then the strategic (policy 8)
scoring. A scenario's optional `is_boss` flag pins encounter context a player knows
from presentation. `options.trace_decisions` records the observation and selected
candidate for policies 8 and 9, and a scripted `opening` hands the named policy
control after fixed `battleturn:` commands, so a witness can assert a decision
state. The tests under `tests/battle_expert_*` check the scorer, its Zig/C-reference
parity, the observation boundary and such witnesses.

CMake remains the authoritative release build; `zig build` is experimental.
Compare paired batches of the same existing scenarios and source baseline with
`python tools/battle_sim_compare.py /private/c-batch /private/zig-batch`.
ENTRY, result and trace bytes must match for every seed; errors, limits, retries,
missing cases and JSON whitespace differences fail. Executable identities are
retained, not substituted for outcome evidence. Historical receipts do not by
themselves qualify the Zig port.

The worker uses the native party, enemy, AI, damage and random-number code.
Renderless mode removes the graphics device but still runs CPU-side game draw,
effect, motion and script work. Its supported virtual presentation is 4:3 at
640×448; it does not produce an image or support pixel readback. A rendered
comparison and a real-game comparison exercise different behavior and must not
be inferred from a renderless result.

The worker's entry, result and trace formats are implementation interfaces;
normal mod testing should use `p4mod` instead of constructing binary entries by
hand. The coordinator's inputs and output identities are retained with its
reports. Tests that need a game disc, save or external reference require the
user to supply those files; they are not distributed with Izanagi.
