//! Native executables and tests from tests/cmake/040-platform-kernel.cmake through
//! 070-overrides-animation.cmake. CTest remains the qualifying authority.
const std = @import("std");
const core = @import("core.zig");

const palquant = [_][]const u8{
    "003f07d0", "003f0610", "003eff30", "003f0110", "003f0510", "003f0c10", "003f0de0",
    "003f02d0", "003f03f0", "003f1030", "003f1240", "003f13b0", "003f1520", "003f1600", "003f1700", "003f17e0", "003f1ce0",
};
const promoted = [_][]const u8{
    "001f5680", "00396e80", "003cc250", "003d5710", "003b5fb0", "003b6390", "003b6420", "003b64c0",
    "003b4230", "003b65d0", "003b6540", "003d0790", "003cbf30", "003ce840", "0039a910", "003a2770", "003a5180",
    "003b4ab0", "003b4c10", "003b4ff0", "003b5650", "003b5ac0", "003b5bf0", "003b6020", "003b61e0",
};
const rxheap = [_][]const u8{ "00415070", "00414ef0", "00414930", "00414820", "00414e10", "00414a30", "00414b50", "00414c10" };
const rtanim = [_][]const u8{ "00415ad0", "004165a0", "00415bc0", "00417310", "00416e10", "003d5840", "003d59d0", "003d5bc0", "003dcc70" };

const Includes = enum { include, include_tests, tests_mirror, mirror, movie_files, movie_render, include_mipsdiff };
const Libraries = enum { math, none, movie, colour, game };
const Objects = enum { none, movie, game };
const Forced = enum { none, palquant, type_h, rxheap };
const Target = struct {
    name: []const u8,
    sources: []const []const u8,
    overrides: []const []const u8 = &.{},
    os: bool = false,
    guest_fixture: bool = false,
    includes: Includes = .include,
    libs: Libraries = .math,
    objects: Objects = .none,
    forced: Forced = .none,
    ps2: bool = false,
    port_defines: bool = false,
    no_builtin: bool = false,
    generic_colour: bool = false,
    mirror_dependency: bool = false,
};

const targets = [_]Target{
    .{ .name = "kernel_threads_test", .sources = &.{ "tests/kernel_threads_test.c", "tests/state_dump_test_stubs.c", "src/platform/kernel_thread.c", "src/platform/state_dump.c", "src/platform/ps2_kernel.c", "src/platform/guest.c" }, .os = true, .includes = .include_tests },
    .{ .name = "capture_state_test", .sources = &.{"tests/capture_state_test.c"}, .guest_fixture = true, .includes = .include_tests, .ps2 = true },
    .{ .name = "walk_tasks_test", .sources = &.{ "tests/walk_tasks_test.c", "src/platform/pad/walk_script.c" } },
    .{ .name = "ee_float_test", .sources = &.{"tests/ee_float_test.c"} },
    .{ .name = "fn_adapt_test", .sources = &.{ "tests/fn_adapt_test.c", "tests/guest_test.c", "src/platform/gport.c", "src/platform/guest.c" }, .os = true, .includes = .include_tests, .ps2 = true },
    .{ .name = "palquant_test", .sources = &.{"tests/palquant_test.c"}, .overrides = &palquant, .os = true, .includes = .tests_mirror, .forced = .palquant, .ps2 = true, .mirror_dependency = true },
    .{ .name = "promoted_leaf_test", .sources = &.{"tests/promoted_leaf_test.c"}, .overrides = &promoted, .os = true, .includes = .mirror, .libs = .none, .forced = .type_h, .ps2 = true, .mirror_dependency = true },
    .{ .name = "rxheap_test", .sources = &.{ "tests/rxheap_test.c", "src/platform/rx_heap_realloc.c" }, .overrides = &rxheap, .os = true, .includes = .mirror, .forced = .rxheap, .ps2 = true, .mirror_dependency = true },
    .{ .name = "sfd_decode_test", .sources = &.{ "tests/sfd_decode_test.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c" }, .os = true, .includes = .movie_files, .libs = .movie, .objects = .movie, .port_defines = true },
    .{ .name = "movie_colour_test", .sources = &.{ "tests/movie_colour_test.c", "src/movie/mpeg2_dec.c", "src/render/p4gl_gl.c", "src/render/p4gl_rt.c", "src/render/p4gl_profile.c" }, .includes = .movie_render, .libs = .colour },
    .{ .name = "movie_colour_generic_test", .sources = &.{ "tests/movie_colour_test.c", "src/movie/mpeg2_dec.c", "src/render/p4gl_gl.c", "src/render/p4gl_rt.c", "src/render/p4gl_profile.c" }, .includes = .movie_render, .libs = .colour, .generic_colour = true },
    .{ .name = "rtanim_test", .sources = &.{ "tests/rtanim_test.c", "src/platform/rw_rtanim_record.c", "src/platform/ee_bytes.c", "src/platform/ee_libm.c", "src/platform/ee_float.c" }, .overrides = &rtanim },
    .{ .name = "battle_camera_curve_test", .sources = &.{"tests/battle_camera_curve_test.c"}, .libs = .game, .objects = .game, .ps2 = true, .port_defines = true, .no_builtin = true },
    .{ .name = "anim_accumulator_order_test", .sources = &.{ "tests/anim_accumulator_order_test.c", "tools/mipsdiff/r5900.c" }, .includes = .include_mipsdiff, .libs = .game, .objects = .game, .ps2 = true, .port_defines = true, .no_builtin = true },
    .{ .name = "bmp_test", .sources = &.{ "tests/bmp_test.c", "overrides/003d8c00.c" }, .guest_fixture = true },
    .{ .name = "spline_test", .sources = &.{ "tests/spline_test.c", "overrides/003b9a40.c" }, .guest_fixture = true },
};

const Test = struct {
    name: []const u8,
    target: []const u8,
    args: []const []const u8 = &.{},
    env: []const []const u8 = &.{},
    skip: bool = false,
    timeout: ?u32 = null,
    gl: bool = false,
    sfdview_output: bool = false,
};
const tests = [_]Test{
    .{ .name = "kernel_threads", .target = "kernel_threads_test", .env = &.{ "P4PC_VIRTUAL_CLOCK=1", "P4PC_TURBO=0" }, .timeout = 60 },
    .{ .name = "kernel_threads_paced", .target = "kernel_threads_test", .env = &.{ "P4PC_VIRTUAL_CLOCK=0", "P4PC_TURBO=0" }, .timeout = 60 },
    .{ .name = "kernel_threads_carry", .target = "kernel_threads_test", .env = &.{ "P4PC_VIRTUAL_CLOCK=0", "P4PC_TURBO=0", "P4PC_PACING=carry" }, .timeout = 60 },
    .{ .name = "capture_state", .target = "capture_state_test" },
    .{ .name = "walk_tasks", .target = "walk_tasks_test" },
    .{ .name = "ee_float", .target = "ee_float_test" },
    .{ .name = "fn_adapt", .target = "fn_adapt_test", .skip = true },
    .{ .name = "palquant", .target = "palquant_test" },
    .{ .name = "promoted_leaf", .target = "promoted_leaf_test" },
    .{ .name = "rxheap", .target = "rxheap_test", .skip = true },
    .{ .name = "sfd_decode", .target = "sfd_decode_test", .skip = true },
    .{ .name = "sfdview_disc", .target = "p4pc_sfdview", .args = &.{ "DATA/SFD/P4CT019.SFD", "3", "1" }, .sfdview_output = true, .skip = true },
    .{ .name = "movie_colour_cpu", .target = "movie_colour_test", .args = &.{"--cpu"}, .skip = true, .timeout = 120 },
    .{ .name = "movie_colour_gles", .target = "movie_colour_test", .skip = true, .timeout = 120, .gl = true },
    .{ .name = "movie_colour_gl", .target = "movie_colour_test", .args = &.{"--gl"}, .skip = true, .timeout = 120, .gl = true },
    .{ .name = "movie_colour_generic_cpu", .target = "movie_colour_generic_test", .args = &.{"--cpu"}, .skip = true, .timeout = 120 },
    .{ .name = "movie_colour_generic_gles", .target = "movie_colour_generic_test", .skip = true, .timeout = 120, .gl = true },
    .{ .name = "movie_colour_generic_gl", .target = "movie_colour_generic_test", .args = &.{"--gl"}, .skip = true, .timeout = 120, .gl = true },
    .{ .name = "rtanim", .target = "rtanim_test" },
    .{ .name = "battle_camera_curve", .target = "battle_camera_curve_test", .skip = true, .timeout = 60 },
    .{ .name = "anim_accumulator_order", .target = "anim_accumulator_order_test", .skip = true, .timeout = 120 },
    .{ .name = "bmp", .target = "bmp_test" },
    .{ .name = "spline", .target = "spline_test" },
};

const gates = @import("runtime_gates.zig");

/// All custody and selection inputs come from the same configuration as the
/// runtime gate registry. Label aggregates can be shared with that registry.
pub const Options = struct {
    default_timeout_s: u32,
    gl: gates.GlPolicy,
    retail: ?gates.Retail,
    lock_dir: []const u8,
    selection: gates.Selection = .{},
    wine64: ?[]const u8,
    wine_timeout_scale: u32,
    wine_ready: ?*std.Build.Step,
    /// C's P4_HOST64_EXES and P4_HOST64W_EXES build aggregates.
    x64: *std.Build.Step,
    x64w: *std.Build.Step,
    gl_label: ?*std.Build.Step = null,
    port64_label: ?*std.Build.Step = null,
    port64win_label: ?*std.Build.Step = null,
};

fn contains(items: []const []const u8, value: []const u8) bool {
    for (items) |item| if (std.mem.eql(u8, item, value)) return true;
    return false;
}

fn selected(selection: gates.Selection, name: []const u8, labels: []const []const u8) bool {
    if (contains(selection.exclude_names, name)) return false;
    for (selection.exclude_labels) |label| if (contains(labels, label)) return false;
    if (selection.include_labels) |wanted| {
        for (wanted) |label| if (!contains(labels, label)) return false;
    }
    return true;
}

fn addCase(b: *std.Build, graph: *const core.Graph, nonserial: *std.Build.Step, o: Options, t: Test, model: core.Model, twin: bool, art: ?core.Artifact) void {
    const suffix: []const u8 = if (!twin) "" else if (model == .m64w) "64w" else "64";
    const name = b.fmt("{s}{s}", .{ t.name, suffix });
    const labels: []const []const u8 = if (t.gl)
        (if (twin) (if (model == .m64w) &.{ "port64win", "gl" } else &.{ "port64", "gl" }) else &.{"gl"})
    else if (twin)
        (if (model == .m64w) &.{"port64win"} else &.{"port64"})
    else
        &.{};
    const top = b.step(b.fmt("native-b:{s}", .{name}), b.fmt("Run native test {s}", .{name}));
    if (selected(o.selection, name, labels)) nonserial.dependOn(top);
    if (t.gl) if (o.gl_label) |label| label.dependOn(top);
    if (twin) {
        const label = if (model == .m64w) o.port64win_label else o.port64_label;
        if (label) |step| step.dependOn(top);
    }
    const exe = art orelse {
        top.dependOn(&b.addFail(b.fmt("native test {s}: executable {s}{s} is unavailable", .{ name, t.target, suffix })).step);
        return;
    };
    if (model == .m64w and o.wine_ready == null) {
        top.dependOn(&b.addFail(b.fmt("native test {s}: wine64_ready fixture is unavailable", .{name})).step);
        return;
    }
    const wd = if (twin) b.fmt("tests/host{s}/{s}", .{ suffix, t.name }) else "tests";
    const save = b.fmt("save/{s}", .{name});
    const wf = b.addWriteFiles();
    const dirs = wf.add(b.fmt("native-b-{s}-dirs.txt", .{name}), b.fmt("{s}\n{s}\n", .{ wd, save }));
    const prepare = b.addSystemCommand(&.{ graph.python, b.pathJoin(&.{ graph.root, "tools/zig_runtime_gates.py" }), "prepare", b.fmt("--binary-dir={s}", .{graph.build_dir}) });
    prepare.addFileArg2(dirs, .{ .prefix = "--dirs=", .make_absolute = true });
    const run = b.addSystemCommand(&.{ graph.python, b.pathJoin(&.{ graph.root, "tools/zig_runtime_gates.py" }), "run", b.fmt("--name={s}", .{name}) });
    var timeout = t.timeout orelse o.default_timeout_s;
    if (t.timeout != null and graph.port64_arch == .aarch64) timeout *= 4;
    const policy_timeout = timeout;
    if (model == .m64w and t.timeout != null) timeout *= o.wine_timeout_scale;
    run.addArg(b.fmt("--timeout={d}", .{timeout}));
    if (t.skip) run.addArg("--skip-code=77");
    run.addArgs(&.{ b.fmt("--cwd={s}", .{b.pathJoin(&.{ graph.build_dir, wd })}), b.fmt("--lock-dir={s}", .{o.lock_dir}) });
    for (labels) |label| run.addArg(b.fmt("--label={s}", .{label}));
    if (t.gl and o.gl == .software) run.addArg("--software-gl");
    if (o.retail) |ret| {
        run.addArgs(&.{ b.fmt("--retail-iso={s}", .{ret.iso}), b.fmt("--retail-elf={s}", .{ret.elf}) });
        if (ret.ref_dir) |dir| run.addArg(b.fmt("--ref-dir={s}", .{dir}));
    }
    for (t.env) |env| run.addArg(b.fmt("--env={s}", .{env}));
    run.addArgs(&.{
        b.fmt("--env=P4PC_SAVE_DIR={s}", .{b.pathJoin(&.{ graph.build_dir, save })}),
        "--env=P4PC_CONFIG=none", "--env=P4PC_INPUT=script",
        "--env=P4PC_MODS=0", "--env=P4PC_PACKS=0", "--env=P4PC_BUNDLED_MOD_DIR=",
    });
    if (t.timeout != null and graph.port64_arch == .aarch64)
        run.addArg(b.fmt("--env=P4PC_GATE_TIMEOUT={d}", .{policy_timeout}));
    if (model == .m64w) {
        run.addArg("--env=WINEDEBUG=-all");
        run.addArg(b.fmt("--env=WINEPREFIX={s}", .{b.pathJoin(&.{ graph.build_dir, "tests/wine64prefix" })}));
    }
    run.addArg(b.fmt("--result={s}", .{b.pathJoin(&.{ graph.build_dir, "zig-gates", b.fmt("{s}.json", .{name}) })}));
    run.addArg("--");
    if (model == .m64w) {
        run.addArg(o.wine64 orelse {
            top.dependOn(&b.addFail(b.fmt("native test {s}: Wine is unavailable", .{name})).step);
            return;
        });
    } else {
        run.addArgs(core.emulator(graph, model));
    }
    // Use the staged executable: Windows and macOS runtime libraries live next
    // to it, not beside the cached executable.
    run.addArg(exe.path);
    for (t.args, 0..) |arg, i| {
        run.addArg(arg);
        if (t.sfdview_output and i == 0)
            run.addArg(b.pathJoin(&.{ graph.build_dir, if (twin) b.fmt("{s}_sfdview", .{suffix}) else "sfdview" }));
    }
    run.step.dependOn(&prepare.step);
    run.step.dependOn(exe.step);
    if (model == .m64w) run.step.dependOn(o.wine_ready.?);
    run.has_side_effects = true;
    top.dependOn(&run.step);
}

fn targetSpec(b: *std.Build, graph: *const core.Graph, t: Target) core.ExecutableSpec {
    const v = core.vars(graph, .i386);
    const a = b.allocator;
    var sources: std.ArrayList([]const u8) = .empty;
    // Each retail override is its own translation unit.
    sources.append(a, t.sources[0]) catch @panic("OOM");
    for (t.overrides) |addr| sources.append(a, b.fmt("overrides/{s}.c", .{addr})) catch @panic("OOM");
    sources.appendSlice(a, t.sources[1..]) catch @panic("OOM");
    if (t.guest_fixture) sources.appendSlice(a, &.{ "tests/guest_test.c", "src/platform/gport.c", "src/platform/guest.c", v.os_src }) catch @panic("OOM")
    else if (t.os) sources.append(a, v.os_src) catch @panic("OOM");
    const includes: []const []const u8 = switch (t.includes) {
        .include => a.dupe([]const u8, &.{b.pathJoin(&.{ v.root, "include" })}) catch @panic("OOM"),
        .include_tests => a.dupe([]const u8, &.{ b.pathJoin(&.{ v.root, "include" }), b.pathJoin(&.{ v.root, "tests" }) }) catch @panic("OOM"),
        .tests_mirror => a.dupe([]const u8, &.{ b.pathJoin(&.{ v.root, "tests" }), b.pathJoin(&.{ v.gen_dir, "decomp/include" }) }) catch @panic("OOM"),
        .mirror => a.dupe([]const u8, &.{b.pathJoin(&.{ v.gen_dir, "decomp/include" })}) catch @panic("OOM"),
        .movie_files => a.dupe([]const u8, &.{ b.pathJoin(&.{ v.root, "src/movie" }), b.pathJoin(&.{ v.root, "src/platform/files" }) }) catch @panic("OOM"),
        .movie_render => a.dupe([]const u8, &.{ b.pathJoin(&.{ v.root, "src/movie" }), b.pathJoin(&.{ v.root, "src/render" }), v.ffmpeg_inc }) catch @panic("OOM"),
        .include_mipsdiff => a.dupe([]const u8, &.{ b.pathJoin(&.{ v.root, "include" }), b.pathJoin(&.{ v.root, "tools/mipsdiff" }) }) catch @panic("OOM"),
    };
    const libs: []const core.Lib = switch (t.libs) {
        .none => &.{},
        .math => &.{.m},
        .movie => &.{ .avcodec, .swscale, .avutil, .m },
        .colour => &.{ .sdl3, .avcodec, .swscale, .avutil, .m },
        .game => &.{ .m, .sdl3, .ffmpeg },
    };
    const objects: []const core.ObjectsRef = switch (t.objects) {
        .none => &.{},
        .movie => &.{.p4movie},
        .game => &.{ .tables, .p4decomp, .p4port, .p4portee },
    };
    var defines: std.ArrayList([]const u8) = .empty;
    if (t.port_defines) defines.appendSlice(a, v.port_defines) catch @panic("OOM");
    if (t.generic_colour) defines.append(a, "P4GL_TEST_GENERIC_C=1") catch @panic("OOM");
    var options: std.ArrayList([]const u8) = .empty;
    if (t.no_builtin) options.append(a, "-fno-builtin") catch @panic("OOM");
    switch (t.forced) {
        .none => {},
        .palquant => options.appendSlice(a, &.{ "-include", b.pathJoin(&.{ graph.root, "tests/palquant_prelude.h" }) }) catch @panic("OOM"),
        .type_h => options.appendSlice(a, &.{ "-include", "type.h" }) catch @panic("OOM"),
        .rxheap => options.appendSlice(a, &.{ "-include", "stddef.h", "-include", "type.h" }) catch @panic("OOM"),
    }
    return .{
        .name = t.name,
        .sources = sources.items,
        .objects = objects,
        .defines = defines.items,
        .includes = includes,
        .compile_options = options.items,
        .libs = libs,
        .ps2_exe = t.ps2,
        .after = if (t.mirror_dependency) a.dupe(*std.Build.Step, &.{v.mirror}) catch @panic("OOM") else &.{},
    };
}

// C owns these p4_ovr_test64 targets, including their unit64 rewrite flags.
// Never fall back to direct compilation if C did not provide one.
fn sharedOverride(name: []const u8) bool {
    return contains(&.{ "bmp_test", "spline_test", "palquant_test", "promoted_leaf_test", "rxheap_test", "rtanim_test" }, name);
}

fn hostProduct(b: *std.Build, graph: *const core.Graph, spec: core.ExecutableSpec, model: core.Model, shared_override: bool) ?core.Artifact {
    const name = b.fmt("{s}{s}", .{ spec.name, model.suffix() });
    if (graph.artifacts.get(name)) |artifact| return artifact;
    if (shared_override) return null;
    return core.hostTwin(graph, spec, model);
}

fn attachProduct(b: *std.Build, aggregate: *std.Build.Step, artifact: ?core.Artifact, name: []const u8, model: core.Model) void {
    if (artifact) |art| {
        aggregate.dependOn(art.step);
    } else {
        aggregate.dependOn(&b.addFail(b.fmt("native test target {s}{s} is unavailable", .{ name, model.suffix() })).step);
    }
}

const Versions = struct { base: ?core.Artifact = null, host64: ?core.Artifact = null, host64w: ?core.Artifact = null };

/// The caller supplies the runtime policy and fixture objects and calls
/// core.finish(graph) after every module has registered its targets.
pub fn register(b: *std.Build, graph: *const core.Graph, nonserial: *std.Build.Step, o: Options) void {
    // Android returns from the root project before 400-tests.cmake.
    if (graph.android) return;
    var products: std.StringHashMapUnmanaged(Versions) = .empty;
    const host64 = graph.port64 and graph.link64;
    const host64w = host64 and graph.link64w;
    for (targets) |target| {
        const spec = targetSpec(b, graph, target);
        var versions: Versions = .{ .base = core.addExecutable(graph, .i386, spec) };
        if (host64) {
            versions.host64 = hostProduct(b, graph, spec, .m64, sharedOverride(target.name));
            attachProduct(b, o.x64, versions.host64, target.name, .m64);
            if (host64w) {
                versions.host64w = hostProduct(b, graph, spec, .m64w, sharedOverride(target.name));
                attachProduct(b, o.x64w, versions.host64w, target.name, .m64w);
            }
        }
        products.put(b.allocator, target.name, versions) catch @panic("OOM");
    }
    var sfdview: Versions = .{ .base = graph.artifacts.get("p4pc_sfdview") };
    if (host64) {
        const spec = core.sfdviewSpec(graph);
        sfdview.host64 = hostProduct(b, graph, spec, .m64, false);
        attachProduct(b, o.x64, sfdview.host64, spec.name, .m64);
        if (host64w) {
            sfdview.host64w = hostProduct(b, graph, spec, .m64w, false);
            attachProduct(b, o.x64w, sfdview.host64w, spec.name, .m64w);
        }
    }
    products.put(b.allocator, "p4pc_sfdview", sfdview) catch @panic("OOM");
    for (tests) |test_case| {
        const versions = products.get(test_case.target).?;
        addCase(b, graph, nonserial, o, test_case, .i386, false, versions.base);
        if (host64) addCase(b, graph, nonserial, o, test_case, .m64, true, versions.host64);
        // The executable is built without Wine, but CMake registers no test.
        if (host64w and o.wine64 != null) addCase(b, graph, nonserial, o, test_case, .m64w, true, versions.host64w);
    }
}
