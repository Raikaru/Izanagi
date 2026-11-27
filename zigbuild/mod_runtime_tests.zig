//! tools/mods runtime tests and the tools/luau golden-station script witness, for each applicable profile.
//! The scene and calendar witnesses exist only for the non-Apple guest64 host profile. Their native
//! sources use the isolated tools/mods flags, not the game directory's inherited compile/link options.
//! 410-luau-closure includes the scene-status and calendar executables, but not scene-position.
//!
//! Each test uses tools/zig_runtime_gates.py with CTest's timeout, working directory, environment and
//! labels. 64w tests use Wine and require wine64_ready; golden-station's timeout is not Wine-scaled.
//! These synthetic fixtures are not hardware or in-game qualification.
const std = @import("std");
const core = @import("core.zig");
const gates = @import("runtime_gates.zig");
const Step = std.Build.Step;

pub const Options = struct {
    /// ${P4_PYTHON}
    python: []const u8,
    /// The cross-process lock directory shared with the other test modules (zig_runtime_gates.py --lock-dir).
    lock_dir: []const u8,
    /// P4PC_MOD_WITNESS_TESTS: mod_luau_test<sfx> carries 014-mod-recovery.cmake's additions. Must be the value the
    /// 014 registrations (native_tests_a) pass to core.modLuauTests; the targets are built once.
    mod_witness_tests: bool,
    /// ${P4_MOD_WINE} (wine64 or wine) as words; REQUIRED when the 64w Luau profile exists.
    mod_wine: []const []const u8,
    /// The wine64_ready fixture step (runtime_gates.fixture(b, prefix, "wine64_ready")): FIXTURES_REQUIRED of the 64w tests.
    wine_ready: *Step,
    /// P4_WINE_TIMEOUT_SCALE (cmake/wine-tests.cmake).
    wine_timeout_scale: u32 = 4,
    /// Step names: `<prefix>:<test>` and `<prefix>-label:<label>`.
    step_prefix: []const u8 = "mod-test",
    /// The CTest selection of the `all` step this module adds its selected tests to.
    all: gates.Selection = .{},
};

pub const Registered = struct {
    /// Every test name registered (for the `if(TEST ...)` checks of other modules and the parity listing).
    test_names: []const []const u8,
    /// `<prefix>-label:<label>` steps by label (host, performance, port64, port64win).
    label_steps: []const LabelStep,
};

pub const LabelStep = struct { label: []const u8, step: *Step };

const Word = union(enum) {
    lit: []const u8,
    python,
    /// a source-tree file
    src: []const u8,
    /// the build-tree product of a target (graph.artifacts)
    target: []const u8,
    /// a path in the tools/mods binary directory (${CMAKE_CURRENT_BINARY_DIR})
    bin: []const u8,
    /// the runner words: the target's CROSSCOMPILING_EMULATOR, or ${P4_MOD_WINE} for 64w
    runner,
};

const Test = struct {
    name: []const u8,
    argv: []const Word,
    labels: []const []const u8,
    timeout: u32,
    skip: ?u8 = null,
    env: []const []const u8,
    /// the pointer model: its runner words, and for .m64w the Wine fixture, environment and timeout scale
    model: core.Model = .i386,
    /// tools/luau's golden-station test has its own working directory and unscaled Wine timeout.
    luau_dir: bool = false,
};

const env_vm = [_][]const u8{ "P4PC_MODS=0", "P4PC_PACKS=0", "P4PC_INPUT=script", "P4PC_VIRTUAL_CLOCK=1", "SDL_AUDIODRIVER=dummy" };
const env_script = [_][]const u8{ "P4PC_MODS=0", "P4PC_PACKS=0", "P4PC_INPUT=script" };
const env_frame = [_][]const u8{ "P4PC_MODS=0", "P4PC_PACKS=0", "P4PC_INPUT=script", "P4PC_CONFIG=none", "SDL_AUDIODRIVER=dummy" };

const api_check_scripts = [_][]const u8{
    "include/p4pc/p4.d.luau",                     "tests/mod_fixtures/native_ui/main.luau",
    "examples/mods/p4pc.example/main.luau",       "mods/p4pc.challenge/main.luau",
    "tests/mod_fixtures/game_services/main.luau", "tests/mod_fixtures/calendar_pass/main.luau",
    "mods/p4pc.practice/main.luau",               "mods/p4pc.hardmode/main.luau",
    "mods/p4pc.golden/main.luau",                 "mods/p4pc.randomizer/main.luau",
    "mods/p4pc.qol/main.luau",                    "tests/mod_fixtures/ui_overlay/main.luau",
    "tests/mod_fixtures/ui_native_row/main.luau", "tests/mod_fixtures/formula_stages/main.luau",
    "tests/mod_fixtures/data_rebalance/data.luau", "tests/mod_fixtures/data_rebalance/main.luau",
};

/// Registers the tools/mods tests of every Luau-active model of the profile; selected ones are added to `all`.
/// Call before core.finish(graph); the fixture targets are built through core.modLuauTests.
pub fn register(b: *std.Build, graph: *const core.Graph, all: *Step, options: Options) Registered {
    const a = b.allocator;
    var tests: std.ArrayList(Test) = .empty;
    // CMAKE_CROSSCOMPILING: the llvm-mingw Windows tree (toolchain-win32.cmake) and the Android NDK trees
    const crosscompiling = graph.win32 or graph.android;
    for ([_]core.Model{ .i386, .m64, .m64w }) |model| {
        if (!core.modLuauProfile(graph, model)) continue;
        if (core.modLuauTests(graph, model, options.mod_witness_tests) == null)
            std.debug.panic("mod_runtime_tests: the {s} Luau mod profile exists but core built no mod_luau_test", .{model.suffix()});
        const sfx = model.suffix();
        const w64 = model == .m64w;
        const luau_exe = b.fmt("mod_luau_test{s}", .{sfx});
        const save_exe = b.fmt("mod_luau_save_test{s}", .{sfx});
        const n = struct {
            fn f(bb: *std.Build, base: []const u8, s: []const u8) []const u8 {
                return bb.fmt("{s}{s}", .{ base, s });
            }
        }.f;
        // LABELS: "host" (+ "performance"); 64 appends port64; 64w replaces them with "host;port64win"
        const host: []const []const u8 = switch (model) {
            .i386 => &.{"host"},
            .m64 => &.{ "host", "port64" },
            .m64w => &.{ "host", "port64win" },
        };
        const perf: []const []const u8 = switch (model) {
            .i386 => &.{ "host", "performance" },
            .m64 => &.{ "host", "performance", "port64" },
            .m64w => &.{ "host", "port64win" },
        };
        const list = [_]Test{
            .{ .name = n(b, "mod_luau_save", sfx), .argv = dupe(a, &.{ .runner, .{ .target = save_exe }, .{ .bin = b.fmt("save-fixture-data{s}", .{sfx}) } }), .labels = host, .timeout = 60, .env = &env_vm },
            .{ .name = n(b, "mod_luau_save_recovery", sfx), .argv = dupe(a, &.{ .python, .{ .src = "tests/mod_luau_save_failure_test.py" }, .runner, .{ .target = save_exe }, .{ .lit = "--driver-end" }, .{ .bin = b.fmt("save-failure-data{s}", .{sfx}) } }), .labels = host, .timeout = 60, .env = &env_vm },
            .{ .name = n(b, "mod_luau", sfx), .argv = dupe(a, &.{ .runner, .{ .target = luau_exe }, .{ .bin = b.fmt("fixture-data{s}", .{sfx}) } }), .labels = host, .timeout = 60, .env = &env_vm },
            .{ .name = n(b, "mod_luau_stage_failure", sfx), .argv = dupe(a, &.{ .python, .{ .src = "tests/mod_luau_stage_failure_test.py" }, .runner, .{ .target = luau_exe }, .{ .lit = "--driver-end" }, .{ .bin = b.fmt("stage-failure-data{s}", .{sfx}) } }), .labels = host, .timeout = 200, .env = &env_vm },
            .{ .name = n(b, "mod_luau_quiescence", sfx), .argv = dupe(a, &.{ .python, .{ .src = "tests/mod_callback_quiescence_test.py" }, .{ .lit = "event" }, .runner, .{ .target = luau_exe } }), .labels = host, .timeout = 30, .skip = 77, .env = &env_script },
            .{ .name = n(b, "mod_frame_cpu", sfx), .argv = dupe(a, &.{ .python, .{ .src = "tests/mod_frame_cpu_test.py" }, .runner, .{ .target = luau_exe }, .{ .lit = "--driver-end" } }), .labels = host, .timeout = 200, .env = &env_frame },
        };
        for (list) |t| tests.append(a, withModel(t, model)) catch @panic("OOM");
        if (model != .i386 and graph.port64_arch != .aarch64 and !graph.android) {
            var candidate: std.ArrayList(Word) = .empty;
            candidate.appendSlice(a, &.{ .python, .{ .src = "luau-zig/tests/mod_contract_lane.py" },
                .{ .lit = "--zig" }, .{ .lit = b.graph.zig_exe },
                .{ .lit = "--python" }, .{ .lit = options.python } }) catch @panic("OOM");
            if (w64) {
                if (options.mod_wine.len != 1) std.debug.panic("mod_runtime_tests: Zig contract needs one Wine executable", .{});
                candidate.appendSlice(a, &.{ .{ .lit = "--wine" }, .{ .lit = options.mod_wine[0] } }) catch @panic("OOM");
            }
            tests.append(a, withModel(.{
                .name = n(b, "mod_luau_zig_contract", sfx),
                .argv = candidate.items,
                .labels = host,
                .timeout = 1800,
                .env = &env_vm,
            }, model)) catch @panic("OOM");
        }
        // These three targets are defined only for the non-Apple 64-bit host profile. Position
        // intentionally has only "host"; status and calendar additionally carry "port64".
        if (model == .m64 and !graph.apple) {
            const v = core.vars(graph, model);
            var defs: std.ArrayList([]const u8) = .empty;
            defs.appendSlice(a, v.port_defines) catch @panic("OOM");
            defs.appendSlice(a, &.{ "P4_GUEST64=1", "P4_GUEST_CHECK=1" }) catch @panic("OOM");
            const Witness = struct { base: []const u8, sources: []const []const u8 };
            for ([_]Witness{
                .{ .base = "mod_scene_position", .sources = &.{ "tests/mod_scene_position_test.c", "src/platform/mods/game_services.c" } },
                .{ .base = "mod_scene_status", .sources = &.{"tests/mod_scene_status_test.c"} },
                .{ .base = "mod_calendar_hold", .sources = &.{"tests/mod_calendar_hold_test.c"} },
            }) |w| {
                const target = b.fmt("{s}_test64", .{w.base});
                const art = core.addExecutable(graph, model, .{
                    .name = target,
                    .dir = "tools/mods/",
                    .sources = w.sources,
                    .defines = defs.items,
                    .includes = dupeStrings(a, &.{ b.pathJoin(&.{ graph.root, "include" }), v.gen_dir }),
                    .compile_options = &.{ "-ffunction-sections", "-fdata-sections" },
                    .link_options = &.{"-Wl,--gc-sections"},
                    .host_adapter = true,
                }) orelse std.debug.panic("mod_runtime_tests: no guest64 executable for {s}", .{target});
                if (!std.mem.eql(u8, w.base, "mod_scene_position")) {
                    if (b.top_level_steps.get("p4_x64")) |cohort| cohort.step.dependOn(art.step);
                }
                tests.append(a, withModel(.{
                    .name = n(b, w.base, sfx),
                    .argv = dupe(a, &.{ .runner, .{ .target = target } }),
                    .labels = if (std.mem.eql(u8, w.base, "mod_scene_position")) &.{"host"} else host,
                    .timeout = 20,
                    .env = &.{},
                }, model)) catch @panic("OOM");
            }
        }
        if (!crosscompiling or w64) {
            var api: std.ArrayList(Word) = .empty;
            api.appendSlice(a, &.{ .python, .{ .src = "tests/mod_api_check_test.py" }, .runner, .{ .target = b.fmt("p4_luau_check{s}", .{sfx}) }, .{ .lit = "--driver-end" } }) catch @panic("OOM");
            for (api_check_scripts) |s| api.append(a, .{ .src = s }) catch @panic("OOM");
            tests.append(a, withModel(.{ .name = n(b, "mod_api_check", sfx), .argv = api.items, .labels = host, .timeout = 60, .env = &env_script }, model)) catch @panic("OOM");
            tests.append(a, withModel(.{ .name = n(b, "mod_hook_cost", sfx), .argv = dupe(a, &.{ .runner, .{ .target = luau_exe }, .{ .lit = "--hook-cost" }, .{ .bin = b.fmt("hook-cost-data{s}", .{sfx}) } }), .labels = perf, .timeout = 120, .env = &env_vm }, model)) catch @panic("OOM");
        }
    }
    // tools/luau is independent of P4PC_MOD_SUPPORT. With no Wine its 64w
    // profile keeps the executable but returns before registering runtime tests.
    for ([_]core.Model{ .i386, .m64, .m64w }) |model| {
        if (!core.luauProfile(graph, model) or (model == .m64w and options.mod_wine.len == 0)) continue;
        const labels: []const []const u8 = switch (model) {
            .i386 => &.{"host"},
            .m64 => &.{ "host", "port64" },
            .m64w => &.{ "host", "port64win" },
        };
        tests.append(a, .{
            .name = b.fmt("mod_golden_station{s}", .{model.suffix()}),
            .argv = dupe(a, &.{ .runner, .{ .target = b.fmt("luau_script_runner{s}", .{model.suffix()}) },
                .{ .src = "tests/mod_golden_station_fixture.luau" },
                .{ .src = "mods/p4pc.golden/main.luau" },
                .{ .src = "tests/mod_golden_station_test.luau" } }),
            .labels = labels,
            .timeout = 30,
            .env = &.{},
            .luau_dir = true,
            .model = model,
        }) catch @panic("OOM");
    }
    return emit(b, graph, all, options, tests.items);
}

fn dupe(a: std.mem.Allocator, words: []const Word) []const Word {
    return a.dupe(Word, words) catch @panic("OOM");
}

fn dupeStrings(a: std.mem.Allocator, strings: []const []const u8) []const []const u8 {
    return a.dupe([]const u8, strings) catch @panic("OOM");
}

/// The test's model; for 64w that means FIXTURES_REQUIRED wine64_ready, ENVIRONMENT += WINEDEBUG=-all
/// WINEPREFIX=<build>/tests/wine64prefix, and p4_wine_scale_timeouts.
fn withModel(t: Test, model: core.Model) Test {
    var x = t;
    x.model = model;
    return x;
}

fn emit(b: *std.Build, graph: *const core.Graph, all: *Step, o: Options, tests: []const Test) Registered {
    const a = b.allocator;
    const runner = b.path("tools/zig_runtime_gates.py");
    const mods_dir = b.pathJoin(&.{ graph.build_dir, "tools", "mods" });
    const wf = b.addWriteFiles();
    const dirs = wf.add("mod_runtime_tests.dirs", "tools/mods\ntools/luau\ntests\n");
    const prepare = b.addSystemCommand(&.{o.python});
    prepare.setName("mod runtime tests: configure directories");
    prepare.addFileArg2(runner, .{ .make_absolute = true });
    prepare.addArgs(&.{ "prepare", b.fmt("--binary-dir={s}", .{graph.build_dir}) });
    prepare.addFileArg2(dirs, .{ .prefix = "--dirs=", .make_absolute = true });
    prepare.has_side_effects = true;
    prepare.expectExitCode(0);

    var names: std.ArrayList([]const u8) = .empty;
    var labels: std.StringArrayHashMapUnmanaged(*Step) = .empty;
    for (tests) |t| {
        names.append(a, t.name) catch @panic("OOM");
        const top = b.step(b.fmt("{s}:{s}", .{ o.step_prefix, t.name }), b.fmt("Run tools/mods test {s}", .{t.name}));
        for (t.labels) |l| {
            const gop = labels.getOrPut(a, l) catch @panic("OOM");
            if (!gop.found_existing) gop.value_ptr.* = b.step(b.fmt("{s}-label:{s}", .{ o.step_prefix, l }), b.fmt("Run the tools/mods tests labelled {s}", .{l}));
            gop.value_ptr.*.dependOn(top);
        }
        if (selected(o.all, t)) all.dependOn(top);
        if (missing(b, graph, o, t)) |why| {
            top.dependOn(&b.addFail(b.fmt("{s}: {s}", .{ t.name, why })).step);
            continue;
        }
        const run = b.addSystemCommand(&.{o.python});
        run.setName(b.fmt("mod test {s}", .{t.name}));
        run.addFileArg2(runner, .{ .make_absolute = true });
        run.addArgs(&.{ "run", b.fmt("--name={s}", .{t.name}) });
        run.addArg(b.fmt("--timeout={d}", .{if (t.model == .m64w and !t.luau_dir) t.timeout * o.wine_timeout_scale else t.timeout}));
        if (t.skip) |code| run.addArg(b.fmt("--skip-code={d}", .{code}));
        run.addArg(b.fmt("--cwd={s}", .{if (t.luau_dir) b.pathJoin(&.{ graph.build_dir, "tools", "luau" }) else mods_dir}));
        run.addArg(b.fmt("--lock-dir={s}", .{o.lock_dir}));
        for (t.labels) |l| run.addArg(b.fmt("--label={s}", .{l}));
        for (t.env) |e| run.addArg(b.fmt("--env={s}", .{e}));
        if (t.model == .m64w) {
            run.addArg("--env=WINEDEBUG=-all");
            run.addArg(b.fmt("--env=WINEPREFIX={s}", .{b.pathJoin(&.{ graph.build_dir, "tests", "wine64prefix" })}));
            run.step.dependOn(o.wine_ready);
        }
        run.addArg(b.fmt("--result={s}/zig-gates/{s}.json", .{ graph.build_dir, t.name }));
        run.addArg("--");
        const model = t.model;
        for (t.argv) |w| switch (w) {
            .lit => |s| run.addArg(s),
            .python => run.addArg(o.python),
            .src => |rel| run.addFileArg2(b.path(rel), .{ .make_absolute = true }),
            .target => |name| {
                const target = graph.artifacts.get(name).?;
                run.addArg(target.path);
                run.addFileInput(target.lazy);
                run.step.dependOn(target.step);
            },
            .bin => |rel| run.addArg(b.pathJoin(&.{ mods_dir, rel })),
            .runner => for (if (model == .m64w) o.mod_wine else core.emulator(graph, model)) |word| run.addArg(word),
        };
        run.step.dependOn(&prepare.step);
        run.has_side_effects = true;
        run.expectExitCode(0);
        top.dependOn(&run.step);
    }
    var label_list: std.ArrayList(LabelStep) = .empty;
    var it = labels.iterator();
    while (it.next()) |e| label_list.append(a, .{ .label = e.key_ptr.*, .step = e.value_ptr.* }) catch @panic("OOM");
    return .{ .test_names = names.items, .label_steps = label_list.items };
}

fn missing(b: *std.Build, graph: *const core.Graph, o: Options, t: Test) ?[]const u8 {
    if (t.model == .m64w and o.mod_wine.len == 0) return "${P4_MOD_WINE} (wine64/wine) is REQUIRED for the Windows x64 Luau mod profile (-Dwine64)";
    for (t.argv) |w| if (w == .target and graph.artifacts.get(w.target) == null)
        return b.fmt("the build supplied no artifact '{s}'", .{w.target});
    return null;
}

fn selected(sel: gates.Selection, t: Test) bool {
    for (sel.exclude_names) |x| if (std.mem.eql(u8, x, t.name)) return false;
    for (sel.exclude_labels) |l| if (hasLabel(t, l)) return false;
    if (sel.include_labels) |inc| for (inc) |l| if (!hasLabel(t, l)) return false;
    return true;
}

fn hasLabel(t: Test, label: []const u8) bool {
    for (t.labels) |l| if (std.mem.eql(u8, l, label)) return true;
    return false;
}
