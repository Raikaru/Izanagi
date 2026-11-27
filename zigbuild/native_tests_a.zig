//! Native and standalone script registrations from 000 through 030.
//! CTest remains authoritative until cross-profile parity has been established.
const std = @import("std");
const core = @import("core.zig");
const native = @import("native_tests_a/native.zig");
const gates = @import("runtime_gates.zig");
pub const Registry = native.Registry;

pub const Options = struct {
    /// CTest --timeout for registrations without a TIMEOUT property.
    default_timeout_s: u32,
    /// Absolute configured P4_DECOMP, including when it is not the default.
    decomp: []const u8,
    /// CMake-equivalent x64 aggregate steps, shared with the other test modules.
    x64: *std.Build.Step,
    x64w: *std.Build.Step,
    /// GL gates are refused by default; true admits only the existing
    /// software-only non-qualifying runner policy.
    software_gl: bool = false,
    /// Explicit retail inputs; unset inputs are not inherited from the host.
    retail_iso: ?[]const u8 = null,
    retail_elf: ?[]const u8 = null,
    retail_ref_dir: ?[]const u8 = null,
    mod_witness_tests: bool = false,
    mod_wine: []const []const u8 = &.{},
    /// The real wine64_ready setup step (port64_coro_win); required for 64w recovery.
    wine_ready: ?*std.Build.Step = null,
    gate_prefix: []const u8 = "gate",
    all: gates.Selection = .{},
};

const Test = struct {
    name: []const u8,
    script: []const u8,
    args: []const []const u8 = &.{},
    timeout: ?u32 = null,
    host: bool = false,
    needs_decomp: bool = false,
};

const tests = [_]Test{
    .{ .name = "event_scene_state", .script = "tests/event_scene_state_test.py" },
    .{ .name = "input_guard", .script = "tests/input_guard_test.py" },
    .{ .name = "gen_port_contracts", .script = "tests/gen_port_test.py", .needs_decomp = true },
    .{ .name = "live_argument_discovery", .script = "tests/audit_live_args_test.py" },
    .{ .name = "build_id", .script = "tests/build_id_test.py" },
    .{ .name = "table_consumers", .script = "tests/table_consumers_test.py" },
    .{ .name = "mod_codegen", .script = "tests/mod_codegen_test.py" },
    .{ .name = "ui_art_check", .script = "tools/ui_art/ui_art.py", .args = &.{"check"}, .timeout = 120, .host = true },
    .{ .name = "spu_pitch_coefficients", .script = "tools/spu_pitch_table.py" },
};

/// Supplies the configured decompiler path and CTest's default timeout.
/// Returned metadata must be passed to runtime_gates for host twins.
pub fn register(b: *std.Build, graph: *core.Graph, nonserial: *std.Build.Step, options: Options) Registry {
    // Android64 returns from the root source list at 350-port64-player,
    // before the tests subdirectory is included.
    if (graph.android64) return .{ .external_test_names = &.{}, .external_host_cases = &.{}, .external_executables = &.{} };
    var registry = native.register(b, graph, nonserial, options.default_timeout_s, options.software_gl, .{
        .iso = options.retail_iso, .elf = options.retail_elf, .ref_dir = options.retail_ref_dir,
    }, .{ .enabled = options.mod_witness_tests, .wine = options.mod_wine, .wine_ready = options.wine_ready, .gate_prefix = options.gate_prefix }, options.all, options.x64, options.x64w);
    var names: std.ArrayList([]const u8) = .empty;
    names.appendSlice(b.allocator, registry.external_test_names) catch @panic("OOM");
    const port64 = b.step("native-a-label-port64", "Standalone 000-030 tests labeled port64");
    const host = b.step("native-a-label-host", "Standalone 000-030 tests labeled host");
    const workdir = b.pathJoin(&.{ graph.build_dir, "tests" });
    const lock_dir = b.pathJoin(&.{ graph.build_dir, "locks" });
    const prepare = b.pathJoin(&.{ graph.root, "zigbuild/native_tests_a/prepare.py" });
    const runner = b.pathJoin(&.{ graph.root, "tools/zig_runtime_gates.py" });
    for (tests) |test_case| {
        names.append(b.allocator, test_case.name) catch @panic("OOM");
        const save = b.pathJoin(&.{ graph.build_dir, "save", test_case.name });
        const timeout = if (test_case.timeout) |seconds|
            if (graph.port64_arch == .aarch64) seconds * 4 else seconds
        else
            options.default_timeout_s;
        const result = b.pathJoin(&.{ graph.build_dir, "gates", "native-a", b.fmt("{s}.json", .{test_case.name}) });
        const run = b.addSystemCommand(&.{ graph.python, prepare, save, workdir, graph.python, runner, "run",
            "--name", test_case.name, "--cwd", workdir, "--lock-dir", lock_dir, "--lock", "host-tooling",
            "--timeout", b.fmt("{d}", .{timeout}), "--result", result,
            "--env", b.fmt("P4PC_SAVE_DIR={s}", .{save}), "--env", "P4PC_CONFIG=none", "--env", "P4PC_INPUT=script",
            "--env", "P4PC_MODS=0", "--env", "P4PC_PACKS=0", "--env", "P4PC_BUNDLED_MOD_DIR=" });
        native.addRetail(b, run, .{ .iso = options.retail_iso, .elf = options.retail_elf, .ref_dir = options.retail_ref_dir });
        if (test_case.needs_decomp) run.addArgs(&.{ "--env", b.fmt("P4PC_TEST_DECOMP={s}", .{options.decomp}) });
        if (test_case.timeout != null and graph.port64_arch == .aarch64)
            run.addArgs(&.{ "--env", b.fmt("P4PC_GATE_TIMEOUT={d}", .{timeout}) });
        run.addArgs(&.{ "--", graph.python });
        run.addFileArg2(b.path(test_case.script), .{ .make_absolute = true });
        run.addArgs(test_case.args);
        if (std.mem.eql(u8, test_case.name, "spu_pitch_coefficients")) {
            run.addArgs(&.{ "--out", b.pathJoin(&.{ graph.root, "src/platform/sound/spu_pitch_table.h" }), "--check" });
        }
        run.addFileInput(b.path("zigbuild/native_tests_a/prepare.py"));
        run.addFileInput(b.path("tools/zig_runtime_gates.py"));
        run.has_side_effects = true;
        const named = b.step(b.fmt("native-a-test-{s}", .{test_case.name}), b.fmt("Run {s}", .{test_case.name}));
        named.dependOn(&run.step);
        if (native.selected(options.all, test_case.name, if (test_case.host) &.{ "port64", "host" } else &.{"port64"})) nonserial.dependOn(named);
        // 250-tools.cmake adds port64 to every plain Python command.
        port64.dependOn(named);
        if (test_case.host) host.dependOn(named);
    }
    registry.external_test_names = names.items;
    return registry;
}
