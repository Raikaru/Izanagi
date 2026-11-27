//! `zig build` for the PC port, with the pinned Zig nightly of cmake/zig/pin.cmake: the game graph of zigbuild/core.zig
//! (one profile per build tree: -Dplatform, -Dport64-arch / -Dandroid-abi, -Dbuild-dir; `zig build -h` lists the options),
//! the tooling tests of zigbuild/tool_tests.zig and the runtime gates of zigbuild/runtime_gates.zig.
//!
//! Steps:
//! * the default (install) step builds the profile's artifacts; `artifacts`, `compile-only`, `census`, `census-generate`,
//!   `crt`, `tables`, `tools` and `mirror` are the CMake aggregates of the same names; `build-gates` is compile-only +
//!   census + CRT + tables; each executable has its own step (p4pc, p4pc_viewer, p4pc64, p4pc64w, ...).
//! * `zig-tool-test-<name>` / `zig-tool-label-*` and `gate:<name>` / `gate-label:<label>` select single tests;
//!   `zig-graph-sources`, `zig-graph-adapters`, `zig-graph-profiles` and `zig-cc-ast` are the graph's own tests.
//! * `tests-nonserial` is every non-serial tooling test and runtime gate (CTest's `all` minus RUN_SERIAL); `test` runs
//!   remote_check_host after all of them (its RUN_SERIAL), so it is the whole suite.
//! * `all` is artifacts + tools + build gates + `test`, and it fails while any CTest registration has no Zig counterpart
//!   (see `unported` below): it can never pass on a subset of the suite. The runtime gates are non-qualifying by
//!   construction (zigbuild/runtime_gates.zig GlPolicy); qualifying GPU gates stay with CTest and the local gate runner.
const std = @import("std");
const builtin = @import("builtin");
const core = @import("zigbuild/core.zig");
const tool_tests = @import("zigbuild/tool_tests.zig");
const runtime_gates = @import("zigbuild/runtime_gates.zig");
const native_a = @import("zigbuild/native_tests_a.zig");
const native_b = @import("zigbuild/native_tests_b.zig");
const native_c = @import("zigbuild/native_tests_c.zig");
const mod_tests = @import("zigbuild/mod_runtime_tests.zig");
const luau_tests = @import("zigbuild/luau_tool_tests.zig");

/// Native registrations cannot qualify `all` until CTest parity and the
/// required hardware gates have been verified.
const unported = [_][]const u8{
    "tests/cmake/000-070, 240, 250 and 270: native registration and cross-profile parity remain unverified",
    "tools/mods: registration and cross-profile parity remain unverified",
    "tools/luau: registration and cross-profile parity remain unverified",
    "hardware GPU custody and qualifying end-to-end gates are not implemented in zig build",
    "API base (2a4420df) registrations not yet mirrored: the new tests/cmake fragments (012-capabilities, 013-game-data/-inventory/-party, 015-battle-*, 017-mod-calendar, 019-game-data-edit, 185-*-witness) and the changed ones (000, 010, 013-*, 016, 100, 160, 180, 192, 210, 220, 240, 250, 253, 258, 290)",
    "tools/mods: lua_semantic_subscription<sfx> and mod_hook_cost's emulated-performance environment are not mirrored",
    "Golden (404a7116): mod_golden_station, mod_scene_position, mod_scene_status and mod_calendar_hold registrations require cross-profile verification",
};

pub fn build(b: *std.Build) void {
    const graph = core.build(b);
    const root = graph.root;
    const tests_dir = b.pathJoin(&.{ graph.build_dir, "tests" });
    const lock_dir = b.pathJoin(&.{ graph.build_dir, "locks" });
    const lock_file = b.pathJoin(&.{ lock_dir, "host-tooling.lock" });
    const timeout = b.option(u32, "test-timeout", "Default test timeout in seconds (CTest --timeout)") orelse 1500;

    // The suite: every non-serial test into `nonserial`; the serial phase (remote_check_host) after it into `suite`.
    const nonserial = b.step("tests-nonserial", "Every non-serial tooling test and runtime gate");
    const suite = b.step("test", "The whole test suite: the non-serial tests, then the RUN_SERIAL remote_check_host");

    // Keep the experimental Zig VM separate from the production C++ Luau
    // profiles. Its own build links the unchanged mod witnesses to the Zig
    // bridge and builds the two upstream conformance adapters.
    const zig_luau = b.addSystemCommand(&.{ b.graph.zig_exe, "build", "-Doptimize=ReleaseSafe", b.fmt("-Dpython={s}", .{graph.python}), "--prefix", b.pathJoin(&.{ graph.build_dir, "luau-zig" }), "upstream-script-candidate", "upstream-api-candidate", "mod-luau-tests", "sandbox-host", "script-runner" });
    zig_luau.setCwd(b.path("luau-zig"));
    zig_luau.has_side_effects = true;
    b.step("luau-zig-candidates", "Build the experimental Zig Luau conformance and mod witnesses").dependOn(&zig_luau.step);
    // Full upstream is externally staged for evidence, never part of the
    // hermetic CTest-equivalent suite or its land gates.
    const zig_luau_evidence = b.addSystemCommand(&.{ graph.python, b.pathJoin(&.{ root, "luau-zig/tests/upstream_lane.py" }),
        "--zig", b.graph.zig_exe, "--python", graph.python, "--scratch-root", graph.build_dir });
    zig_luau_evidence.has_side_effects = true;
    b.step("luau-zig-upstream-evidence", "Compare Zig Luau against a separately staged pinned upstream checkout").dependOn(&zig_luau_evidence.step);

    // The graph's own semantic tests, under the host-tooling lock and timeout of the tooling fixtures.
    const graph_sources = graphTest(b, graph, "zig-graph-sources", "sources", lock_file, tests_dir, 180);
    const graph_adapters = graphTest(b, graph, "zig-graph-adapters", "adapters", lock_file, tests_dir, 180);
    const graph_profiles = graphTest(b, graph, "zig-graph-profiles", "profiles", lock_file, tests_dir, 180);
    nonserial.dependOn(graph_profiles);
    graph.steps.check.dependOn(graph_profiles);
    const ast = toolRun(b, graph, "zig_cc_ast", lock_file, tests_dir, 300);
    ast.addArgs(&.{ graph.python, b.pathJoin(&.{ root, "tests/zig_cc_ast_test.py" }) });
    ast.setEnvironmentVariable("P4PC_ZIG_CC_WRAPPER", graph.cc);
    ast.step.dependOn(graph.cc_ready);
    ast.addFileInput(b.path("tests/zig_cc_ast_test.py"));
    const ast_step = b.step("zig-cc-ast", "The pinned Zig compiler path's preprocessed AST, ABI and rewrite test (tests/zig_cc_ast_test.py)");
    ast_step.dependOn(&ast.step);
    nonserial.dependOn(&ast.step);

    const native_linux_with_cxx = builtin.os.tag == .linux and graph.platform == .linux and graph.luau;
    tool_tests.register(b, nonserial, .{
        .python = graph.python,
        .clang = graph.cc,
        .nm = graph.llvm_nm,
        .objdump = graph.llvm_objdump,
        .working_dir = tests_dir,
        .lock_file = lock_file,
        .private_tools = exists(b, root, "tests/cmake/250-tools-private.cmake"),
        .private_orchestrator = exists(b, root, "tests/cmake/252-orchestrator-private.cmake"),
        .private_platform = exists(b, root, "tests/cmake/260-platform-checks-private.cmake"),
        .native_linux_with_cxx = native_linux_with_cxx,
        .cmake_sources_replacement = graph_sources,
        .host_adapter_graph_replacement = graph_adapters,
        .remote_check_host_serial = suite,
        .default_timeout_seconds = timeout,
    });

    // Runtime gates: software-rendered and non-qualifying at most; refused by default.
    const gl = b.option(runtime_gates.GlPolicy, "gate-gl", "GL-capable runtime gates: refuse (default) or software (Mesa, non-qualifying)") orelse .refuse;
    const iso = b.option([]const u8, "gate-iso", "Retail disc image for the runtime gates (P4PC_ISO)");
    const elf = b.option([]const u8, "gate-elf", "Retail SLUS_217.82 for the runtime gates (P4PC_ELF)");
    const ref_dir = b.option([]const u8, "gate-ref-dir", "Reference directory of the runtime gates (P4PC_REF_DIR)");
    const wine64 = b.option([]const u8, "wine64", "wine64/wine executable for the Windows x64 gates (P4_WINE64)");
    const mod_witness = b.option(bool, "mod-witness-tests", "Register the isolated MOD_WITNESS recovery tests (P4PC_MOD_WITNESS_TESTS)") orelse false;
    if (core.modLuauProfile(graph, .m64w) and wine64 == null)
        std.process.fatal("tools/mods: Wine is required for the Windows x64 Luau mod profile; pass -Dwine64=<path>", .{});
    const mod_wine: []const []const u8 = if (wine64) |w| b.allocator.dupe([]const u8, &.{w}) catch @panic("OOM") else &.{};
    const retail: ?runtime_gates.Retail = if (iso != null and elf != null)
        .{ .iso = iso.?, .elf = elf.?, .ref_dir = ref_dir, .configured_elf = graph.retail }
    else
        null;
    const wine_ready = runtime_gates.fixture(b, "gate", "wine64_ready");
    // C owns shared override twins and must register before A or the artifact scan.
    const c = native_c.register(b, graph, nonserial, .{
        .lock_dir = lock_dir, .default_timeout_s = timeout, .wine64 = wine64, .retail = retail,
    });
    const a = native_a.register(b, graph, nonserial, .{
        .default_timeout_s = timeout, .decomp = core.vars(graph, .i386).decomp,
        .x64 = c.x64, .x64w = c.x64w,
        .software_gl = gl == .software, .retail_iso = iso, .retail_elf = elf, .retail_ref_dir = ref_dir,
        .mod_witness_tests = mod_witness, .mod_wine = mod_wine, .wine_ready = wine_ready,
    });
    const native_b_gl = b.step("native-b-label-gl", "Native 040-070 GL tests");
    const native_b_64 = b.step("native-b-label-port64", "Native 040-070 64-bit tests");
    const native_b_64w = b.step("native-b-label-port64win", "Native 040-070 Windows 64-bit tests");
    native_b.register(b, graph, nonserial, .{
        .default_timeout_s = timeout, .gl = gl, .retail = retail, .lock_dir = lock_dir,
        .wine64 = wine64, .wine_timeout_scale = 4, .wine_ready = wine_ready,
        .gl_label = native_b_gl, .port64_label = native_b_64, .port64win_label = native_b_64w,
        .x64 = c.x64, .x64w = c.x64w,
    });
    const mods = mod_tests.register(b, graph, nonserial, .{
        .python = graph.python, .lock_dir = lock_dir, .mod_witness_tests = mod_witness,
        .mod_wine = mod_wine, .wine_ready = wine_ready,
    });
    const luau = luau_tests.register(b, graph, nonserial, .{
        .python = graph.python, .lock_dir = lock_dir, .luau_wine = mod_wine,
        .wine_ready = wine_ready, .x64 = c.x64, .x64w = c.x64w,
    });
    var external_names: std.ArrayList([]const u8) = .empty;
    external_names.appendSlice(b.allocator, a.external_test_names) catch @panic("OOM");
    external_names.appendSlice(b.allocator, c.external_test_names) catch @panic("OOM");
    external_names.appendSlice(b.allocator, mods.test_names) catch @panic("OOM");
    external_names.appendSlice(b.allocator, luau.test_names) catch @panic("OOM");
    var external_cases: std.ArrayList(runtime_gates.HostCase) = .empty;
    external_cases.appendSlice(b.allocator, a.external_host_cases) catch @panic("OOM");
    external_cases.appendSlice(b.allocator, c.external_host_cases) catch @panic("OOM");
    var external_exes: std.ArrayList(runtime_gates.ExternalExe) = .empty;
    external_exes.appendSlice(b.allocator, a.external_executables) catch @panic("OOM");
    external_exes.appendSlice(b.allocator, c.external_executables) catch @panic("OOM");
    var artifacts: std.ArrayList(runtime_gates.Artifact) = .empty;
    var target_names: std.ArrayList([]const u8) = .empty;
    var it = graph.artifacts.iterator();
    while (it.next()) |entry| {
        artifacts.append(b.allocator, .{ .name = entry.key_ptr.*, .path = entry.value_ptr.lazy, .runtime_path = entry.value_ptr.path, .ready = entry.value_ptr.step }) catch @panic("OOM");
        target_names.append(b.allocator, entry.key_ptr.*) catch @panic("OOM");
    }
    artifacts.appendSlice(b.allocator, c.artifacts) catch @panic("OOM");
    if (graph.port64w) target_names.append(b.allocator, "p4decomp64w") catch @panic("OOM");
    runtime_gates.register(b, nonserial, .{
        .binary_dir = graph.build_dir,
        .python = graph.python,
        .profile = .{
            .win32 = graph.win32,
            .apple = graph.apple,
            .linux_x86_64_host = graph.platform == .linux and builtin.cpu.arch == .x86_64,
            .port64_arch = if (graph.port64_arch == .aarch64) .aarch64 else .x86_64,
            .port64_link = graph.link64,
            .port64_objs = graph.port64,
            .x64_probe = graph.port64,
            .release = b.option(bool, "p4-release", "Release build: require LGPL-only FFmpeg (P4_RELEASE)") orelse false,
            .mod_witness_tests = mod_witness,
            .targets = target_names.items,
        },
        .artifacts = artifacts.items,
        .external_test_names = external_names.items,
        .external_host_cases = external_cases.items,
        .external_executables = external_exes.items,
        .retail = retail,
        .port64 = if (graph.port64) .{
            .g64 = graph.g64_view.?,
            .census_baseline = relativeTo(root, graph.census_baseline),
            .c_compiler = graph.cc,
        } else .{},
        .wine64 = wine64,
        .mod_wine = mod_wine,
        .gl = gl,
        .lock_dir = lock_dir,
        .default_timeout_s = timeout,
    });
    if (b.top_level_steps.get("gate-label:gl")) |label| label.step.dependOn(native_b_gl);
    if (b.top_level_steps.get("gate-label:port64")) |label| label.step.dependOn(native_b_64);
    if (b.top_level_steps.get("gate-label:port64win")) |label| label.step.dependOn(native_b_64w);
    for (mods.label_steps) |label| {
        if (b.top_level_steps.get(b.fmt("gate-label:{s}", .{label.label}))) |step|
            step.step.dependOn(label.step);
    }
    for (luau.label_steps) |label| {
        if (b.top_level_steps.get(b.fmt("gate-label:{s}", .{label.label}))) |step|
            step.step.dependOn(label.step);
    }

    const all = b.step("all", "Every artifact, tool, build gate and test of the profile (fails while the suite is incomplete)");
    all.dependOn(graph.steps.artifacts);
    all.dependOn(graph.steps.tools);
    all.dependOn(graph.steps.check);
    all.dependOn(suite);
    var message: std.ArrayList(u8) = .empty;
    message.appendSlice(b.allocator, "`all` is incomplete or unqualified:") catch @panic("OOM");
    for (unported) |u| message.print(b.allocator, "\n  {s}", .{u}) catch @panic("OOM");
    all.dependOn(&b.addFail(message.items).step);
    core.finish(graph);
}

fn exists(b: *std.Build, root: []const u8, rel: []const u8) bool {
    std.Io.Dir.cwd().access(b.graph.io, b.pathJoin(&.{ root, rel }), .{}) catch return false;
    return true;
}

fn relativeTo(root: []const u8, path: []const u8) []const u8 {
    if (std.mem.startsWith(u8, path, root) and path.len > root.len and path[root.len] == '/') return path[root.len + 1 ..];
    return path;
}

/// A tooling fixture run by tools/zig_tool_tests.py (the host-tooling lock, the timeout, the tests' working directory and
/// the P4PC_MODS=0/P4PC_PACKS=0/P4PC_BUNDLED_MOD_DIR= environment); the caller appends `-- <command>`.
fn toolRun(b: *std.Build, graph: *const core.Graph, name: []const u8, lock_file: []const u8, cwd: []const u8, timeout_s: u32) *std.Build.Step.Run {
    const run = b.addSystemCommand(&.{ graph.python, b.pathJoin(&.{ graph.root, "tools/zig_tool_tests.py" }), "--name", name, "--cwd", cwd, "--lock", lock_file, "--timeout", b.fmt("{d}", .{timeout_s}), "--" });
    run.has_side_effects = true;
    return run;
}

/// tools/zig_build_graph_test.py <mode> as its own Run, reachable as the public step `step_name`; the returned Run step is
/// the one the suite registrations depend on.
fn graphTest(b: *std.Build, graph: *const core.Graph, step_name: []const u8, mode: []const u8, lock_file: []const u8, cwd: []const u8, timeout_s: u32) *std.Build.Step {
    const run = toolRun(b, graph, step_name, lock_file, cwd, timeout_s);
    run.addArgs(&.{ graph.python, b.pathJoin(&.{ graph.root, "tools/zig_build_graph_test.py" }), mode, "--build", graph.build_dir });
    if (std.mem.eql(u8, mode, "profiles")) {
        if (b.option([]const u8, "graph-cmake-cache", "Actual qualified CMake cache for the optimization census")) |cache|
            run.addArgs(&.{ "--cmake-cache", cache });
    }
    const public = b.step(step_name, b.fmt("The graph's semantic check against the CMake definitions (tools/zig_build_graph_test.py {s})", .{mode}));
    public.dependOn(&run.step);
    return &run.step;
}
