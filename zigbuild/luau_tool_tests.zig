//! The test registrations of tools/luau/CMakeLists.txt (p4_luau_profile, one per p4_host_cxx_profiles model ""/64/64w
//! when P4PC_LUAU): luau_sandbox, mod_randomizer_engine, mod_hardmode_lifecycle and, when not cross-compiling or for 64w,
//! luau_analysis, p4mod and p4mod_zip32, each with the model suffix; plus cmake/sources/410-luau-closure.cmake's build
//! dependencies of the p4_x64 / p4_x64w aggregates on the Luau tools, p4mod and its fixtures, and the mod VM fixtures.
//!
//! The executables are core's p4_luau_check<sfx>, luau_sandbox_test<sfx> and luau_script_runner<sfx>, and p4mod.zig's
//! p4mod<sfx>, p4mod_game_fixture<sfx> and p4mod_zip32_test<sfx> (registered here, before core.finish); each test command
//! takes the staged build-tree product (Artifact.path, ready after Artifact.step). Each test is one
//! tools/zig_runtime_gates.py run with CTest's semantics: TIMEOUT (times the Wine scale where the source calls
//! p4_wine_scale_timeouts), the tools/luau binary directory as WORKING_DIRECTORY, ENVIRONMENT in order, LABELS. tools/luau
//! is not tests/, so tests/CMakeLists.txt's environment and the P4_CMD_TESTS host twins do not apply.
//!
//! 64w: when Wine (${P4_LUAU_WINE}) is absent, CMake registers no 64w Luau test (a STATUS message; the binaries stay
//! built); Registered.skipped_64w carries that reason. With Wine, the 64w tests require the wine64_ready fixture.
const std = @import("std");
const core = @import("core.zig");
const p4mod = @import("p4mod.zig");
const gates = @import("runtime_gates.zig");
const Step = std.Build.Step;

pub const Options = struct {
    /// ${Python3_EXECUTABLE}
    python: []const u8,
    /// The cross-process lock directory shared with the other test modules (zig_runtime_gates.py --lock-dir).
    lock_dir: []const u8,
    /// ${P4_LUAU_WINE}: find_program(NAMES wine64 wine) as words; empty when not found.
    luau_wine: []const []const u8,
    /// The wine64_ready fixture step (runtime_gates.fixture(b, prefix, "wine64_ready")): FIXTURES_REQUIRED of the 64w tests.
    wine_ready: *Step,
    /// P4_WINE_TIMEOUT_SCALE (cmake/wine-tests.cmake).
    wine_timeout_scale: u32 = 4,
    /// 240's p4_x64 / p4_x64w aggregates (native_tests_c.register's x64 / x64w) for 410-luau-closure.cmake; null when the
    /// profile has no such target. The mod VM fixtures they also take must already be registered
    /// (mod_runtime_tests.register first).
    x64: ?*Step = null,
    x64w: ?*Step = null,
    /// Step names: `<prefix>:<test>` and `<prefix>-label:<label>`.
    step_prefix: []const u8 = "luau-test",
    /// The CTest selection of the `all` step this module adds its selected tests to.
    all: gates.Selection = .{},
};

pub const Registered = struct {
    /// Every test name registered (for the `if(TEST ...)` checks of other modules and the parity listing).
    test_names: []const []const u8,
    /// `<prefix>-label:<label>` steps by label (host, port64, port64win).
    label_steps: []const LabelStep,
    /// Why CMake registers no 64w Luau test in this profile (the 64w profile exists but Wine was not found); null otherwise.
    skipped_64w: ?[]const u8,
};

pub const LabelStep = struct { label: []const u8, step: *Step };

const Word = union(enum) {
    lit: []const u8,
    python,
    /// a source-tree file
    src: []const u8,
    /// the staged build-tree product of a target (graph.artifacts)
    target: []const u8,
    /// the runner words: CROSSCOMPILING_EMULATOR, or ${P4_LUAU_WINE} for 64w
    runner,
};

const Test = struct {
    name: []const u8,
    argv: []const Word,
    labels: []const []const u8,
    timeout: u32,
    env: []const []const u8,
    model: core.Model,
    /// p4_wine_scale_timeouts applies (luau_sandbox64w, luau_analysis64w, p4mod64w, p4mod_zip3264w; not the two
    /// script-runner witnesses)
    wine_scaled: bool = false,
};

const env_script = [_][]const u8{ "P4PC_INPUT=script", "P4PC_MODS=0", "P4PC_PACKS=0" };
const env_runner = [_][]const u8{ "P4PC_INPUT=script", "P4PC_MODS=0", "P4PC_PACKS=0", "P4PC_ISO=" };

/// Registers the tools/luau tests of every Luau profile model; selected ones are added to `all`. Call after
/// mod_runtime_tests.register (410's mod fixture dependencies) and before core.finish(graph).
pub fn register(b: *std.Build, graph: *const core.Graph, all: *Step, options: Options) Registered {
    const a = b.allocator;
    var tests: std.ArrayList(Test) = .empty;
    var skipped_64w: ?[]const u8 = null;
    // CMAKE_CROSSCOMPILING: the llvm-mingw Windows tree (toolchain-win32.cmake) and the Android NDK trees
    const crosscompiling = graph.win32 or graph.android;
    for ([_]core.Model{ .i386, .m64, .m64w }) |model| {
        if (!core.luauProfile(graph, model)) continue;
        const sfx = model.suffix();
        const check = b.fmt("p4_luau_check{s}", .{sfx});
        const sandbox = b.fmt("luau_sandbox_test{s}", .{sfx});
        const script_runner = b.fmt("luau_script_runner{s}", .{sfx});
        const tools = p4mod.register(b, graph, model);
        // the modder's CLI, its archives and fixtures exist in every Luau profile (whether or not their tests are registered)
        // 410-luau-closure.cmake: the cohort builds the Luau tools, p4mod and its fixtures, and the mod VM fixtures of its model
        const cohort: ?*Step = switch (model) {
            .i386 => null,
            .m64 => options.x64,
            .m64w => options.x64w,
        };
        if (cohort) |agg| {
            for ([_][]const u8{ check, sandbox, script_runner }) |t| agg.dependOn(artifact(graph, t).step);
            for ([_]core.Artifact{ tools.p4mod, tools.game_fixture, tools.zip32_test }) |t| agg.dependOn(t.step);
            if (core.modLuauProfile(graph, model)) for ([_][]const u8{ "mod_luau_test", "mod_luau_save_test" }) |base| {
                const name = b.fmt("{s}{s}", .{ base, sfx });
                const art = graph.artifacts.get(name) orelse
                    std.debug.panic("luau_tool_tests: {s} is not registered; call mod_runtime_tests.register first", .{name});
                agg.dependOn(art.step);
            };
        }
        if (model == .m64w and options.luau_wine.len == 0) {
            // find_program(P4_LUAU_WINE) failed: CMake returns before add_test (STATUS "Luau64w runtime tests skipped")
            skipped_64w = "Luau64w runtime tests not registered: Wine unavailable (tools/luau/CMakeLists.txt); pass -Dwine64 to register them";
            continue;
        }
        const host: []const []const u8 = switch (model) {
            .i386 => &.{"host"},
            .m64 => &.{ "host", "port64" },
            .m64w => &.{ "host", "port64win" },
        };
        // luau_sandbox: COMMAND luau_sandbox_test<sfx> (a target name: CTest runs it under its CROSSCOMPILING_EMULATOR),
        // or "${P4_LUAU_WINE}" $<TARGET_FILE:...> for 64w
        tests.append(a, .{ .name = b.fmt("luau_sandbox{s}", .{sfx}), .argv = dupe(a, &.{ .runner, .{ .target = sandbox } }), .labels = host, .timeout = 20, .env = &env_script, .model = model, .wine_scaled = true }) catch @panic("OOM");
        tests.append(a, .{ .name = b.fmt("mod_randomizer_engine{s}", .{sfx}), .argv = dupe(a, &.{ .python, .{ .src = "tests/mod_randomizer_engine_test.py" }, .{ .lit = "--" }, .runner, .{ .target = script_runner } }), .labels = host, .timeout = 120, .env = &env_runner, .model = model }) catch @panic("OOM");
        tests.append(a, .{ .name = b.fmt("mod_hardmode_lifecycle{s}", .{sfx}), .argv = dupe(a, &.{ .python, .{ .src = "tests/mod_hardmode_lifecycle_test.py" }, .{ .lit = "--" }, .runner, .{ .target = script_runner } }), .labels = host, .timeout = 120, .env = &env_script, .model = model }) catch @panic("OOM");
        if (!crosscompiling or model == .m64w) {
            tests.append(a, .{ .name = b.fmt("luau_analysis{s}", .{sfx}), .argv = dupe(a, &.{ .python, .{ .src = "tests/luau_check_test.py" }, .runner, .{ .target = check } }), .labels = host, .timeout = 30, .env = &env_script, .model = model, .wine_scaled = true }) catch @panic("OOM");
            // p4mod: python tests/p4mod_test.py ${emulator} $<TARGET_FILE:p4mod> --game-fixture $<TARGET_FILE:p4mod_game_fixture>
            tests.append(a, .{ .name = b.fmt("p4mod{s}", .{sfx}), .argv = dupe(a, &.{ .python, .{ .src = "tests/p4mod_test.py" }, .runner, .{ .target = b.fmt("p4mod{s}", .{sfx}) }, .{ .lit = "--game-fixture" }, .{ .target = b.fmt("p4mod_game_fixture{s}", .{sfx}) } }), .labels = host, .timeout = 30, .env = &env_script, .model = model, .wine_scaled = true }) catch @panic("OOM");
            // p4mod_zip32: ${emulator} $<TARGET_FILE:p4mod_zip32_test>
            tests.append(a, .{ .name = b.fmt("p4mod_zip32{s}", .{sfx}), .argv = dupe(a, &.{ .runner, .{ .target = b.fmt("p4mod_zip32_test{s}", .{sfx}) } }), .labels = host, .timeout = 30, .env = &env_script, .model = model, .wine_scaled = true }) catch @panic("OOM");
        }
    }
    var reg = emit(b, graph, all, options, tests.items);
    reg.skipped_64w = skipped_64w;
    return reg;
}

fn artifact(graph: *const core.Graph, name: []const u8) core.Artifact {
    return graph.artifacts.get(name) orelse std.debug.panic("luau_tool_tests: core built no {s} for a Luau profile model", .{name});
}

fn dupe(a: std.mem.Allocator, words: []const Word) []const Word {
    return a.dupe(Word, words) catch @panic("OOM");
}

fn emit(b: *std.Build, graph: *const core.Graph, all: *Step, o: Options, tests: []const Test) Registered {
    const a = b.allocator;
    const runner = b.path("tools/zig_runtime_gates.py");
    const luau_dir = b.pathJoin(&.{ graph.build_dir, "tools", "luau" });
    const wf = b.addWriteFiles();
    const dirs = wf.add("luau_tool_tests.dirs", "tools/luau\ntests\n");
    const prepare = b.addSystemCommand(&.{o.python});
    prepare.setName("luau tool tests: configure directories");
    prepare.addFileArg2(runner, .{ .make_absolute = true });
    prepare.addArgs(&.{ "prepare", b.fmt("--binary-dir={s}", .{graph.build_dir}) });
    prepare.addFileArg2(dirs, .{ .prefix = "--dirs=", .make_absolute = true });
    prepare.has_side_effects = true;
    prepare.expectExitCode(0);

    var names: std.ArrayList([]const u8) = .empty;
    var labels: std.StringArrayHashMapUnmanaged(*Step) = .empty;
    for (tests) |t| {
        names.append(a, t.name) catch @panic("OOM");
        const top = b.step(b.fmt("{s}:{s}", .{ o.step_prefix, t.name }), b.fmt("Run tools/luau test {s}", .{t.name}));
        for (t.labels) |l| {
            const gop = labels.getOrPut(a, l) catch @panic("OOM");
            if (!gop.found_existing) gop.value_ptr.* = b.step(b.fmt("{s}-label:{s}", .{ o.step_prefix, l }), b.fmt("Run the tools/luau tests labelled {s}", .{l}));
            gop.value_ptr.*.dependOn(top);
        }
        if (selected(o.all, t)) all.dependOn(top);
        if (missing(b, graph, t)) |why| {
            top.dependOn(&b.addFail(b.fmt("{s}: {s}", .{ t.name, why })).step);
            continue;
        }
        const w64 = t.model == .m64w;
        const run = b.addSystemCommand(&.{o.python});
        run.setName(b.fmt("luau test {s}", .{t.name}));
        run.addFileArg2(runner, .{ .make_absolute = true });
        run.addArgs(&.{ "run", b.fmt("--name={s}", .{t.name}) });
        run.addArg(b.fmt("--timeout={d}", .{if (w64 and t.wine_scaled) t.timeout * o.wine_timeout_scale else t.timeout}));
        run.addArg(b.fmt("--cwd={s}", .{luau_dir}));
        run.addArg(b.fmt("--lock-dir={s}", .{o.lock_dir}));
        for (t.labels) |l| run.addArg(b.fmt("--label={s}", .{l}));
        for (t.env) |e| run.addArg(b.fmt("--env={s}", .{e}));
        if (w64) {
            run.addArg("--env=WINEDEBUG=-all");
            run.addArg(b.fmt("--env=WINEPREFIX={s}", .{b.pathJoin(&.{ graph.build_dir, "tests", "wine64prefix" })}));
            run.step.dependOn(o.wine_ready);
        }
        run.addArg(b.fmt("--result={s}/zig-gates/{s}.json", .{ graph.build_dir, t.name }));
        run.addArg("--");
        for (t.argv) |w| switch (w) {
            .lit => |s| run.addArg(s),
            .python => run.addArg(o.python),
            .src => |rel| run.addFileArg2(b.path(rel), .{ .make_absolute = true }),
            .target => |name| {
                const art = graph.artifacts.get(name).?;
                run.addArg(art.path);
                run.addFileInput(art.lazy);
                run.step.dependOn(art.step);
            },
            .runner => for (if (w64) o.luau_wine else core.emulator(graph, t.model)) |word| run.addArg(word),
        };
        run.step.dependOn(&prepare.step);
        run.has_side_effects = true;
        run.expectExitCode(0);
        top.dependOn(&run.step);
    }
    var label_list: std.ArrayList(LabelStep) = .empty;
    var it = labels.iterator();
    while (it.next()) |e| label_list.append(a, .{ .label = e.key_ptr.*, .step = e.value_ptr.* }) catch @panic("OOM");
    return .{ .test_names = names.items, .label_steps = label_list.items, .skipped_64w = null };
}

fn missing(b: *std.Build, graph: *const core.Graph, t: Test) ?[]const u8 {
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
