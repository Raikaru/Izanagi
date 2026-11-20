# Testing and harnesses

[Platform index](../platform.md) · [Contributing](../../CONTRIBUTING.md)

## Headless execution

Build and run tests in the `p4pc-ci` container as described in
[BUILDING.md](../../BUILDING.md). `sh tools/ci_tests.sh build` runs the suite;
`ctest --test-dir build --output-on-failure` runs the configured tests directly.
Tests requiring game data or external reference captures may skip when those
inputs are absent. A missing input is not evidence that the corresponding game
behavior passed.

For unattended game runs use script input (`P4PC_INPUT=script`), isolated writable
saves and a bounded deadline. Use `P4PC_CONFIG=none` to avoid changing personal
settings. `P4PC_HIDDEN_WINDOW=1` requests a hidden Windows GL context;
`P4PC_OFFSCREEN=1` omits swaps and may change edge pixels. Do not submit game
data, captures, memory dumps or saves to the repository.

## Viewer and renderer smoke

The viewer loads and renders game models from a user-supplied disc image. The
`render_smoke` test requires a graphics context, `P4PC_ISO` and `P4PC_ELF`.
Renderer configuration and diagnostics are described in
[renderer documentation](../renderer_port.md).

## Captures and diagnostics

`P4PC_CAPTURE_FRAMES` selects presented frames, and `P4PC_CAPTURE_PNG` names
the output pattern. `P4PC_DUMP_TASKS_FRAMES` requests task-tree dumps.
`P4PC_DUMP_RAM` and `P4PC_DUMP_RAM_AT` capture guest memory for diagnosis;
`P4PC_DUMP_RAM_MAX_MB` bounds disk use. These outputs can contain game data and
should remain private. For profiling controls see [performance](performance.md).

## Title and battle drivers

`P4PC_AUTOPRESS_NEWGAME` and `P4PC_AUTOPRESS_LOAD` drive the title menu using
scripted input. `P4PC_TITLE_WATCH_OPENING=1` allows the opening movie to finish.
These controls are intended for bounded automated runs, not normal play.

For mod testing, [`p4mod test`](../modding/p4mod.md) provides boot, new-game and
battle scenarios. The [native battle simulator](battle_sim.md) describes the
worker and its limits; a simulator result alone does not establish PS2 parity.
