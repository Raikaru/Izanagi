//! Native test executables and registrations of the runtime-gate domain under `zig build`, without CMake, CTest or Ninja:
//!
//! * the add_executable targets of tests/cmake/090-movie.cmake .. 191-nav-probe.cmake, 160-tools-ovdiff.cmake,
//!   210-port64-runtime.cmake (port64_task_test, the build64.py prototypes, crash_log_test64, port64_coro_win_test) and
//!   220-port64-host.cmake (ovdiff64, printf_abi_calls{64,64w}, p4_ovr_test64, and the p4_host64_exe twins `<exe>64` /
//!   `<exe>64w` that 290-port64-host-cases.cmake's p4_host64_tests makes for those executables); runtime_gates.zig
//!   registers the gates that run them;
//! * the ovdiff/mipsdiff test pairs of 250-tools.cmake and lockstep_draw_test/lockstep_draw_buffer of
//!   270-platform-lockstep.cmake, registered here (one tools/zig_runtime_gates.py run per test);
//! * the p4_x64 / p4_x64w aggregates and the port64_task64 target of 240-port64-targets.cmake / 210.
//!
//! Targets are made through the native-test API of zigbuild/core.zig (addExecutable, hostTwin, addOverrideTest64,
//! addObjectLibrary): the tests directory's flags, the model flags and the link come from the graph. A target the profile
//! cannot model is never replaced by a pass: core makes it fail with its reason, and a test of this file whose executable
//! is missing fails as NOT RUN with the reason.
//!
//! Integration contract (build.zig owns the call, after core.build and before core.finish / reading graph.artifacts):
//!   const c = native_tests_c.register(b, graph, nonserial, .{ ... Options ... });
//!   runtime_gates.register(..., .artifacts = graph.artifacts ++ c.artifacts,
//!       .external_executables = c.external_executables, .external_host_cases = c.external_host_cases,
//!       .external_test_names = c.external_test_names, ...);
//! The test executables land in graph.artifacts (core registers them); `Result.artifacts` holds only the custom-command
//! outputs runtime_gates.zig keys by file name (port64_task_test64, port64_task_retail64, port64_coro_test64,
//! crash_log_test64). Other modules add their own 64-bit twins to `Result.x64` / `Result.x64w` (P4_HOST64_EXES).
const std = @import("std");
const builtin = @import("builtin");
const core = @import("core.zig");
const rg = @import("runtime_gates.zig");

const Build = std.Build;
const Step = Build.Step;
const Run = Step.Run;
const LazyPath = Build.LazyPath;
const Model = core.Model;

pub const Options = struct {
    /// Directory of the cross-process lock files (shared with runtime_gates.zig).
    lock_dir: []const u8,
    /// The CTest --timeout the suite passes, for tests without a TIMEOUT.
    default_timeout_s: u32,
    /// ${P4_WINE64} (find_program wine64/wine); null = not found (210: port64_coro_win_test is not defined).
    wine64: ?[]const u8 = null,
    /// Retail inputs given to the tests' environment (P4PC_ISO, P4PC_ELF, P4PC_REF_DIR); null: removed.
    retail: ?rg.Retail = null,
    step_prefix: []const u8 = "native-c",
};

pub const Result = struct {
    /// Custom-command outputs keyed by file name (not in graph.artifacts).
    artifacts: []const rg.Artifact,
    /// Executables of these fragments runtime_gates.zig does not list itself (lockstep_draw_test).
    external_executables: []const rg.ExternalExe,
    /// P4_CMD_TESTS entries registered here that p4_host64_tests twins (lockstep_draw_buffer).
    external_host_cases: []const rg.HostCase,
    /// Every test registered here (`if(TEST ...)` of the host twins).
    external_test_names: []const []const u8,
    /// 240's p4_x64 / p4_x64w aggregates.
    x64: *Step,
    x64w: *Step,
};

// ---------------------------------------------------------------------------------------------------------------------

const M = struct {
    b: *Build,
    g: *const core.Graph,
    a: std.mem.Allocator,
    o: Options,
    root: []const u8,
    v: core.Vars, // the i386 (tree-native) model's variables

    fn fmt(m: *const M, comptime f: []const u8, args: anytype) []const u8 {
        return m.b.fmt(f, args);
    }

    /// ${CMAKE_SOURCE_DIR}/<rel>
    fn s(m: *const M, rel: []const u8) []const u8 {
        return m.b.pathJoin(&.{ m.root, rel });
    }

    /// ${CMAKE_CURRENT_SOURCE_DIR}/<name> of tests/CMakeLists.txt
    fn t(m: *const M, name: []const u8) []const u8 {
        return m.b.pathJoin(&.{ m.root, "tests", name });
    }

    fn cat(m: *const M, parts: []const []const []const u8) []const []const u8 {
        var out: std.ArrayList([]const u8) = .empty;
        for (parts) |p| out.appendSlice(m.a, p) catch @panic("OOM");
        return out.items;
    }

    fn strs(m: *const M, items: []const []const u8) []const []const u8 {
        return m.a.dupe([]const u8, items) catch @panic("OOM");
    }

    fn lp(m: *const M, abs: []const u8) LazyPath {
        return m.b.graph.cwdRelativePath(abs);
    }

    /// ${P4_GUEST_TEST_SRC} (000-registration.cmake)
    fn guestTestSrc(m: *const M) []const []const u8 {
        return m.strs(&.{ m.t("guest_test.c"), m.s("src/platform/gport.c"), m.s("src/platform/guest.c"), m.v.os_src });
    }

    /// ${P4_EE_STDIO_GUEST_SOURCES} (010-platform.cmake): ${P4_GUEST_TEST_SRC} without guest_test.c
    fn eeStdioGuestSources(m: *const M) []const []const u8 {
        return m.strs(&.{ m.s("src/platform/gport.c"), m.s("src/platform/guest.c"), m.v.os_src });
    }

    /// ${P4_EE_CRT_SOURCES} (010-platform.cmake, with ${P4_EE_DECIMAL_SOURCES})
    fn eeCrtSources(m: *const M) []const []const u8 {
        return m.strs(&.{
            m.s("src/platform/ee_bytes.c"),    m.s("src/platform/ps2_libc.c"),  m.s("src/platform/ee_printf.c"),
            m.s("src/platform/ee_scanf.c"),    m.s("src/platform/ee_strtod.c"), m.s("src/platform/ee_libm.c"),
            m.s("src/platform/ee_float.c"),    m.s("overrides/00440bb8.c"),
            // ${P4_EE_DECIMAL_SOURCES}
            m.s("src/platform/ee_decimal.c"),  m.s("src/platform/ee_dtoa.c"),   m.s("src/platform/ps2_fpbit.c"),
            m.s("src/platform/kernel_heap.c"), m.s("src/platform/ee_heap.c"),   m.t("ee_heap_kernel.c"),
        });
    }

    /// The compiler environment every compile of the graph runs with (core's useCompiler).
    fn useCompiler(m: *const M, run: *Run) void {
        run.step.dependOn(m.g.cc_ready);
        run.setEnvironmentVariable("P4PC_ZIG_CC", "1");
        run.setEnvironmentVariable("P4PC_ZIG_CC_WRAPPER", m.g.cc);
        run.setEnvironmentVariable("P4PC_BUILD_TOOLCHAIN_ID", m.g.toolchain_id);
    }

    fn inputs(m: *const M, run: *Run, rels: []const []const u8) void {
        for (rels) |rel| run.addFileInput(m.b.path(rel));
    }

    /// SOURCES `<srcs> "${P4_TABLES}" $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee>`
    /// in that link order (every image-linked test lists its own sources first, then the tables, then the objects).
    fn imageOrder(m: *const M, srcs: []const []const u8) []const core.LinkInput {
        var out: std.ArrayList(core.LinkInput) = .empty;
        for (srcs) |src| out.append(m.a, .{ .source = src }) catch @panic("OOM");
        for (image_objects) |ref| out.append(m.a, .{ .objects = ref }) catch @panic("OOM");
        return out.items;
    }
};

/// $<TARGET_OBJECTS:p4decomp> $<TARGET_OBJECTS:p4port> $<TARGET_OBJECTS:p4portee> with "${P4_TABLES}" (the game image).
const image_objects = [_]core.ObjectsRef{ .tables, .p4decomp, .p4port, .p4portee };
const game_libs = [_]core.Lib{ .m, .sdl3, .ffmpeg };

/// A domain executable: its i386 (tree-native) add_executable, and whether its LINK_OPTIONS wrap symbols.
const Domain = struct {
    spec: core.ExecutableSpec,
    wraps: bool = false,
    /// p4_host64_tests makes no twin (port64_* tests only, or an explicit 64-bit target exists).
    no_twin: bool = false,
};

// ---------------------------------------------------------------------------------------------------------------------

pub fn register(b: *Build, graph: *const core.Graph, nonserial: *Step, options: Options) Result {
    const m: M = .{ .b = b, .g = graph, .a = b.allocator, .o = options, .root = graph.root, .v = core.vars(graph, .i386) };
    var tests: std.ArrayList(Test) = .empty;
    var extra: std.ArrayList(rg.Artifact) = .empty;
    var host64: std.ArrayList(*Step) = .empty;
    var host64w: std.ArrayList(*Step) = .empty;

    const link64 = core.available(graph, .m64) == null; // P4_PORT64_LINK AND P4_PORT64_OBJS
    const link64w = core.available(graph, .m64w) == null; // TARGET p4pc64w

    // ---- 090 .. 191 and 160: the domain executables, then 210's port64_task_test / port64_coro_win_test
    const domains = domainExecutables(&m);
    for (domains) |d| _ = core.addExecutable(graph, .i386, d.spec);

    // ---- 270-platform-lockstep.cmake
    const lockstep_spec: core.ExecutableSpec = .{
        .name = "lockstep_draw_test",
        .sources = m.strs(&.{ m.t("lockstep_draw_test.c"), m.s("src/platform/lockstep_draw.c") }),
    };
    const lockstep = core.addExecutable(graph, .i386, lockstep_spec);
    tests.append(m.a, .{
        .name = "lockstep_draw_buffer",
        .argv = m.a.dupe(Word, &.{.{ .target = "lockstep_draw_test" }}) catch @panic("OOM"),
        .missing = if (lockstep == null) m.fmt("lockstep_draw_test: {s}", .{core.available(graph, .i386).?}) else null,
    }) catch @panic("OOM");

    // ---- 250-tools.cmake: if(TARGET ovdiff) (160: NOT P4_WIN32)
    if (!graph.win32) {
        const why: ?[]const u8 = if (core.available(graph, .i386)) |r| m.fmt("ovdiff: {s}", .{r}) else null;
        ovdiffTests(&m, &tests, "ovdiff", "", why, false);
    }

    // ---- 210-port64-runtime.cmake: the x86_64 task prototype (custom commands) and its target
    var port64_task64: ?*Step = null;
    if (!graph.win32 and graph.platform == .linux and builtin.cpu.arch == .x86_64 and graph.port64_arch == .x86_64) {
        port64_task64 = taskPrototypes(&m, &extra);
    }

    // ---- 220-port64-host.cmake / 290-port64-host-cases.cmake (if(P4_PORT64_LINK AND P4_PORT64_OBJS))
    if (link64) {
        // printf_abi_calls{64,64w}: the rewritten printf call witness (`TARGET p4decomp64w` for 64w, linked only by twins)
        printfAbiCalls(&m, .m64);
        if (link64w) printfAbiCalls(&m, .m64w);

        // ovdiff64 (NOT P4_APPLE) and its 250 tests
        if (!graph.apple) {
            const ovdiff64 = core.addExecutable(graph, .m64, ovdiffSpec(&m, "ovdiff64"));
            if (ovdiff64) |art| host64.append(m.a, art.step) catch @panic("OOM");
            ovdiffTests(&m, &tests, "ovdiff64", "64", if (ovdiff64 == null) m.fmt("ovdiff64: {s}", .{core.available(graph, .m64).?}) else null, true);
        }

        // p4_ovr_test64: the override tests' twins
        for (overrideTests(&m)) |spec| {
            if (core.addOverrideTest64(graph, .m64, spec)) |art| host64.append(m.a, art.step) catch @panic("OOM");
            if (link64w) if (core.addOverrideTest64(graph, .m64w, spec)) |art| host64w.append(m.a, art.step) catch @panic("OOM");
        }

        // p4_host64_exe twins of the executables these fragments define (the tests are twinned by runtime_gates.zig)
        var twinned: std.ArrayList(Domain) = .empty;
        twinned.appendSlice(m.a, domains) catch @panic("OOM");
        twinned.append(m.a, .{ .spec = lockstep_spec }) catch @panic("OOM");
        for (twinned.items) |d| {
            if (d.no_twin) continue;
            if (graph.apple and d.wraps) continue; // ld64 has no --wrap
            if (core.hostTwin(graph, d.spec, .m64)) |art| host64.append(m.a, art.step) catch @panic("OOM");
            if (link64w and !std.mem.eql(u8, d.spec.name, "ovdiff")) {
                if (core.hostTwin(graph, d.spec, .m64w)) |art| host64w.append(m.a, art.step) catch @panic("OOM");
            }
        }
    }

    // ---- 240-port64-targets.cmake: the 64-bit builds as one unit
    const x64 = b.step("p4_x64", "The 64-bit builds and the executables of the tests labelled port64 (CMake p4_x64)");
    const x64w = b.step("p4_x64w", "The Windows x64 mirror, executables and their test executables (CMake p4_x64w)");
    for ([_][]const u8{ "p4pc64", "p4pc_viewer64" }) |n| if (graph.artifacts.get(n)) |art| x64.dependOn(art.step);
    if (core.decompStep(graph, .m64)) |s| x64.dependOn(s);
    if (port64_task64) |s| x64.dependOn(s);
    if (graph.artifacts.get("port64_task_test")) |art| x64.dependOn(art.step);
    if (core.decompStep(graph, .m64w)) |s| x64w.dependOn(s);
    for ([_][]const u8{ "p4pc64w", "p4pc_viewer64w", "port64_coro_win_test" }) |n| if (graph.artifacts.get(n)) |art| x64w.dependOn(art.step);
    if (link64w) if (core.vars(graph, .m64w).runtime_ready) |s| x64w.dependOn(s); // p4pc64w_dlls
    if (graph.port64) {
        // p4_x64_abi: ${P4_G64}/abi.json
        if (graph.g64_view) |g64| {
            g64.addStepDependencies(x64);
            g64.addStepDependencies(x64w);
        }
    }
    for (host64.items) |s| x64.dependOn(s);
    for (host64w.items) |s| x64w.dependOn(s);

    // ---- the tests of this file
    emitTests(&m, nonserial, tests.items);

    var names: std.ArrayList([]const u8) = .empty;
    for (tests.items) |te| names.append(m.a, te.name) catch @panic("OOM");
    return .{
        .artifacts = extra.items,
        .external_executables = m.a.dupe(rg.ExternalExe, &.{.{ .name = "lockstep_draw_test" }}) catch @panic("OOM"),
        // 270's test as 290 sees it: `lockstep_draw_test` (a bare target name), no properties yet.
        .external_host_cases = m.a.dupe(rg.HostCase, &.{.{
            .name = "lockstep_draw_buffer",
            .argv = m.a.dupe(rg.Arg, &.{.{ .bare = "lockstep_draw_test" }}) catch @panic("OOM"),
        }}) catch @panic("OOM"),
        .external_test_names = names.items,
        .x64 = x64,
        .x64w = x64w,
    };
}

// ---------------------------------------------------------------------------------------------------------------------
// Executables

/// 160-tools-ovdiff.cmake (ovdiff) and 220-port64-host.cmake (ovdiff64): the same sources, options and link.
fn ovdiffSpec(m: *const M, name: []const u8) core.ExecutableSpec {
    return .{
        .name = name,
        .link_order = m.imageOrder(&.{ m.s("tools/mipsdiff/ovdiff.c"), m.s("tools/mipsdiff/r5900.c") }),
        .includes = m.strs(&.{ m.s("include"), m.s("tools/mipsdiff") }),
        .compile_options = &.{ "-fno-builtin", "-Wno-unused" },
        .defines = m.v.port_defines,
        .libs = &game_libs,
        // 160: p4_ps2_exe(ovdiff); 220's ovdiff64 does not call it
        .ps2_exe = std.mem.eql(u8, name, "ovdiff"),
        .link_options = &.{ "-Wl,--wrap=func_0043f810", "-Wl,--wrap=func_0043f9c8" },
    };
}

/// The executables of 090 .. 191, 160 and 210, as add_executable defines them (i386 / tree-native model).
fn domainExecutables(m: *const M) []const Domain {
    const g = m.g;
    const v = m.v;
    const guest = m.guestTestSrc();
    const port_defs = v.port_defines;
    var out: std.ArrayList(Domain) = .empty;
    const add = struct {
        fn f(list: *std.ArrayList(Domain), a: std.mem.Allocator, d: Domain) void {
            list.append(a, d) catch @panic("OOM");
        }
    }.f;
    // A test linked against the game image: "${P4_TABLES}" and the p4decomp/p4port/p4portee objects, -fno-builtin,
    // ${P4_PORT_DEFINES}, m ${P4_SDL3_LIB} ${P4_FFMPEG_LIBS}, p4_ps2_exe.
    const imageExe = struct {
        fn f(mm: *const M, name: []const u8, srcs: []const []const u8, incs: []const []const u8, link: []const []const u8) core.ExecutableSpec {
            return .{
                .name = name,
                .link_order = mm.imageOrder(srcs),
                .includes = incs,
                .compile_options = &.{"-fno-builtin"},
                .defines = mm.v.port_defines,
                .libs = &game_libs,
                .link_options = link,
                .ps2_exe = true,
            };
        }
    }.f;
    const inc = m.s("include");
    const sound = m.s("src/platform/sound");
    const files = m.s("src/platform/files");

    // 090-movie.cmake
    add(&out, m.a, .{ .spec = .{
        .name = "sfmpvp_test",
        .sources = m.cat(&.{
            &.{ m.t("sfmpvp_test.c"), m.t("guest_test.c"), m.s("src/platform/gport.c"), m.s("src/platform/guest.c"), m.s("src/movie/sfmpvp.c"), m.s("src/movie/movie_upload.c") },
            v.png_sources,
            &.{ m.s("src/platform/files/vfs.c"), m.s("src/platform/files/iso9660.c"), v.os_src },
        }),
        .objects = m.a.dupe(core.ObjectsRef, &.{.p4movie}) catch @panic("OOM"),
        .includes = m.strs(&.{ m.s("tests"), m.s("src/movie"), m.s("src/viewer"), m.s("src/render"), files }),
        .defines = port_defs,
        .libs = m.a.dupe(core.Lib, &.{ .ffmpeg, .m }) catch @panic("OOM"),
        .ps2_exe = true,
        .link_options = if (g.win32) m.strs(&.{"-Wl,--large-address-aware"}) else m.strs(&.{}),
    } });
    add(&out, m.a, .{ .spec = .{
        .name = "mwply_test",
        .sources = m.cat(&.{
            &.{ m.t("mwply_test.c"), m.t("guest_test.c"), m.s("src/platform/gport.c"), m.s("src/platform/guest.c"), m.s("src/movie/mwply.c") },
            v.png_sources,
            &.{ m.s("src/platform/files/vfs.c"), m.s("src/platform/files/iso9660.c"), v.os_src },
        }),
        .objects = m.a.dupe(core.ObjectsRef, &.{.p4movie}) catch @panic("OOM"),
        .includes = m.strs(&.{ m.s("tests"), m.s("src/movie"), m.s("src/viewer"), files }),
        .defines = port_defs,
        .libs = m.a.dupe(core.Lib, &.{ .ffmpeg, .m }) catch @panic("OOM"),
    } });
    add(&out, m.a, .{ .spec = imageExe(m, "movie_smoke", m.strs(&.{m.t("movie_smoke.c")}), m.strs(&.{ inc, m.s("src/movie"), m.s("src/render"), m.s("src/viewer"), files }), &.{}) });
    add(&out, m.a, .{ .spec = imageExe(m, "movie_task_smoke", m.strs(&.{m.t("movie_task_smoke.c")}), m.strs(&.{ inc, m.s("src/render"), m.s("src/viewer"), files }), &.{}) });

    // 111-import-max.cmake
    add(&out, m.a, .{ .spec = .{
        .name = "import_max_test",
        .sources = m.strs(&.{ m.t("import_max_test.c"), m.s("src/platform/memcard/import_max.c"), v.os_src }),
        .includes = m.strs(&.{inc}),
    } });
    // 112-import-save.cmake
    add(&out, m.a, .{ .spec = .{
        .name = "save_import_test",
        .sources = m.strs(&.{
            m.t("save_import_test.c"),
            m.s("src/platform/memcard/save_import.c"),
            m.s("src/platform/memcard/save_image.c"),
            m.s("src/platform/memcard/save_transaction.c"),
            m.s("src/platform/memcard/mod_save_custody.c"),
            m.s("src/platform/memcard/mod_save_hash.c"),
            m.s("src/platform/memcard/mod_save_format.c"),
            m.s("src/platform/memcard/import_parse.c"),
            m.s("src/platform/memcard/import_card.c"),
            m.s("src/platform/memcard/import_max.c"),
            m.s("src/platform/memcard/import_cbs.c"),
            m.s("third_party/miniz/miniz_tinfl.c"),
            v.os_src,
        }),
        .libs = m.a.dupe(core.Lib, &.{.m}) catch @panic("OOM"),
    } });
    add(&out, m.a, .{ .spec = .{
        .name = "import_cbs_test",
        .sources = m.strs(&.{ m.t("import_cbs_test.c"), m.s("src/platform/memcard/import_cbs.c"), m.s("third_party/miniz/miniz_tinfl.c") }),
    } });

    // 140-movie-audio.cmake
    add(&out, m.a, .{ .spec = .{
        .name = "ffmpeg_license_test",
        .sources = m.strs(&.{m.t("ffmpeg_license_test.c")}),
        .includes = m.strs(&.{v.ffmpeg_inc}),
        .libs = m.a.dupe(core.Lib, &.{ .ffmpeg, .m }) catch @panic("OOM"),
    } });
    add(&out, m.a, .{ .spec = .{
        .name = "movie_audio_test",
        .sources = m.strs(&.{ m.t("movie_audio_test.c"), m.s("src/platform/sound/movie_audio.c"), m.s("src/platform/sound/p4snd_mixer.c"), m.s("src/platform/sound/p4snd_reverb.c"), v.os_src }),
        .includes = m.strs(&.{ inc, sound, m.s("src/movie") }),
        .libs = m.a.dupe(core.Lib, &.{ .m, .sdl3 }) catch @panic("OOM"),
    } });

    // 150-cri-audio.cmake
    add(&out, m.a, .{ .wraps = true, .spec = imageExe(m, "cri_cpu_adapter_test", m.strs(&.{ m.t("cri_cpu_adapter_test.c"), m.s("tools/mipsdiff/r5900.c") }), m.strs(&.{ inc, m.s("tools/mipsdiff") }), &.{
        "-Wl,--gc-sections",
        "-Wl,--wrap=func_004e2108",
        "-Wl,--wrap=cvFsSeek",
        "-Wl,--wrap=func_004e22f8",
        "-Wl,--wrap=cvFsGetStat",
        "-Wl,--wrap=func_004e2750",
        "-Wl,--wrap=cvFsGetNumTr",
        "-Wl,--wrap=func_004e4190",
        "-Wl,--wrap=cvFsCallUsrErrFn",
        "-Wl,--wrap=func_004f0578",
    }) });
    add(&out, m.a, .{ .wraps = true, .spec = imageExe(m, "cri_sfd_callback_test", m.strs(&.{ m.t("cri_sfd_callback_test.c"), m.s("tools/mipsdiff/r5900.c") }), m.strs(&.{ inc, m.s("tools/mipsdiff") }), &.{ "-Wl,--gc-sections", "-Wl,--wrap=func_004d6910" }) });
    add(&out, m.a, .{ .spec = imageExe(m, "cri_rofs_metadata_test", m.strs(&.{m.t("cri_rofs_metadata_test.c")}), m.strs(&.{inc}), &.{"-Wl,--gc-sections"}) });
    add(&out, m.a, .{ .spec = imageExe(m, "axrna_test", m.strs(&.{m.t("axrna_test.c")}), m.strs(&.{ inc, sound }), &.{}) });
    add(&out, m.a, .{ .spec = imageExe(m, "adxt_playend_test", m.strs(&.{m.t("adxt_playend_test.c")}), m.strs(&.{ inc, sound }), &.{}) });
    add(&out, m.a, .{ .spec = imageExe(m, "movie_start_test", m.strs(&.{m.t("movie_start_test.c")}), m.strs(&.{ inc, m.s("tests"), m.s("src/movie"), files }), &.{}) });
    add(&out, m.a, .{ .spec = imageExe(m, "cri_adxi_rpc_test", m.strs(&.{m.t("cri_adxi_rpc_test.c")}), m.strs(&.{ inc, m.s("src/cri"), sound }), &.{}) });
    add(&out, m.a, .{ .spec = .{
        .name = "mixer_device_test",
        .sources = m.strs(&.{ m.t("mixer_device_test.c"), m.s("src/platform/sound/p4snd_reverb.c"), v.os_src }),
        .includes = m.strs(&.{ inc, sound }),
        .libs = m.a.dupe(core.Lib, &.{ .m, .sdl3 }) catch @panic("OOM"),
    } });
    add(&out, m.a, .{ .spec = .{
        .name = "sebank_test",
        .sources = m.strs(&.{ m.t("sebank_test.c"), m.s("src/platform/sound/hd_bank.c"), m.s("src/platform/sound/vag.c"), m.s("src/platform/files/vfs.c"), m.s("src/platform/files/iso9660.c"), v.os_src }),
        .includes = m.strs(&.{ inc, sound, files }),
        .defines = port_defs,
        .libs = m.a.dupe(core.Lib, &.{.m}) catch @panic("OOM"),
    } });
    add(&out, m.a, .{ .spec = .{
        .name = "cri_acs_test",
        .sources = m.cat(&.{ &.{m.t("cri_acs_test.c")}, guest, &.{ m.s("src/platform/ee_dfloat.c"), m.s("src/platform/ee_dlibm.c"), m.s("src/platform/ps2_fpbit.c") } }),
        .libs = m.a.dupe(core.Lib, &.{.m}) catch @panic("OOM"),
        .ps2_exe = true,
    } });
    add(&out, m.a, .{ .spec = imageExe(m, "adx_wav_test", m.strs(&.{m.t("adx_wav_test.c")}), m.strs(&.{ inc, sound, files }), &.{}) });
    add(&out, m.a, .{ .spec = imageExe(m, "ahx_wav_test", m.strs(&.{m.t("ahx_wav_test.c")}), m.strs(&.{ inc, sound, files, v.ffmpeg_inc }), &.{}) });
    add(&out, m.a, .{ .spec = imageExe(m, "aixp_wav_test", m.strs(&.{m.t("aixp_wav_test.c")}), m.strs(&.{ inc, sound, files }), &.{}) });

    // 160-tools-ovdiff.cmake (NOT P4_WIN32); its 64-bit twin is 220's own ovdiff64 (p4_host64_exe returns on it)
    if (!g.win32) add(&out, m.a, .{ .wraps = true, .no_twin = true, .spec = ovdiffSpec(m, "ovdiff") });

    // 170-cri-state.cmake
    add(&out, m.a, .{ .spec = .{ .name = "cri_acssnd_test", .sources = m.cat(&.{ &.{m.t("cri_acssnd_test.c")}, guest }), .ps2_exe = true } });
    add(&out, m.a, .{ .spec = .{ .name = "cri_acs_step_test", .sources = m.cat(&.{ &.{m.t("cri_acs_step_test.c")}, guest }), .ps2_exe = true } });
    for ([_][]const u8{ "cri_adxm_test", "cri_init_test", "cri_stm_test" }) |name| {
        add(&out, m.a, .{ .spec = .{ .name = name, .sources = m.cat(&.{ &.{m.t(m.fmt("{s}.c", .{name}))}, guest }), .includes = m.strs(&.{inc}), .ps2_exe = true } });
    }
    add(&out, m.a, .{ .spec = .{
        .name = "cri_cvfs_test",
        .sources = m.cat(&.{ &.{m.t("cri_cvfs_test.c")}, m.eeCrtSources(), m.eeStdioGuestSources() }),
        .includes = m.strs(&.{ inc, m.s("src/platform"), m.s("third_party") }),
        .ps2_exe = true,
    } });
    add(&out, m.a, .{ .spec = .{ .name = "cri_adxt_start_test", .sources = m.cat(&.{ &.{m.t("cri_adxt_start_test.c")}, guest }), .includes = m.strs(&.{inc}), .ps2_exe = true } });
    add(&out, m.a, .{ .spec = imageExe(m, "rofs_open_test", m.strs(&.{m.t("rofs_open_test.c")}), m.strs(&.{ inc, sound, files }), &.{}) });
    add(&out, m.a, .{ .spec = imageExe(m, "cri_file_size_test", m.strs(&.{m.t("cri_file_size_test.c")}), m.strs(&.{ inc, files }), &.{}) });

    // 190-platform-walk.cmake
    for ([_][]const u8{ "walk_choice_test", "dungeon_digest_test", "walk_battle_rules_test" }) |name| {
        add(&out, m.a, .{ .spec = .{
            .name = name,
            .sources = m.strs(&.{ m.t(m.fmt("{s}.c", .{name})), m.s("src/platform/pad/walk_script.c") }),
            .includes = if (std.mem.eql(u8, name, "walk_battle_rules_test")) m.strs(&.{inc}) else m.strs(&.{}),
            .libs = m.a.dupe(core.Lib, &.{.m}) catch @panic("OOM"),
        } });
    }

    // 191-nav-probe.cmake
    add(&out, m.a, .{ .spec = .{
        .name = "nav_probe_test",
        .sources = m.cat(&.{
            &.{ m.t("nav_probe_test.c"), m.s("src/platform/nav_probe.c"), m.s("src/platform/pad/pad_sdl.c"), m.s("src/platform/pad/walk_script.c"), m.s("src/platform/pad/ps2_pad.c"), m.s("src/platform/config/p4cfg.c") },
            guest,
            &.{m.t("battle_sim_inactive.c")}, // target_sources(nav_probe_test PRIVATE battle_sim_inactive.c)
        }),
        .defines = port_defs,
        .libs = m.a.dupe(core.Lib, &.{ .m, .sdl3 }) catch @panic("OOM"),
        .ps2_exe = true,
    } });

    // 210-port64-runtime.cmake: the i386 task prototype (Linux x86_64 host, P4_PORT64_ARCH x86_64)
    if (!g.win32 and g.platform == .linux and builtin.cpu.arch == .x86_64 and g.port64_arch == .x86_64) {
        const unit = m.b.pathJoin(&.{ v.gen_dir, "decomp/src/Kernel/sdkTask.c" });
        var libs: std.ArrayList(core.Lib) = .empty;
        var link: []const []const u8 = &.{};
        if (g.mod_support) {
            link = &.{"-Wl,--gc-sections"};
            libs.appendSlice(m.a, &.{ .{ .target = "p4_mod_game" }, .{ .target = "p4_mod_core" } }) catch @panic("OOM");
        }
        add(&out, m.a, .{ .no_twin = true, .spec = .{
            .name = "port64_task_test",
            .sources = m.strs(&.{ m.t("port64_task_test.c"), unit, v.os_src, m.s("src/platform/ee_bytes.c") }),
            .source_props = m.a.dupe(core.SourceProps, &.{.{
                .path = unit,
                .compile_options = &.{ "-malign-double", "-mno-implicit-float", "-DP4_DECOMP_UNIT=1", "-std=gnu89", "-fno-builtin", "-DINCLUDE_ASM(FOLDER,NAME)=", "-DINCLUDE_RODATA(FOLDER,NAME)=", "-Wno-int-conversion", "-Wno-incompatible-function-pointer-types", "-Wno-return-mismatch", "-Wno-incompatible-pointer-types" },
            }}) catch @panic("OOM"),
            .includes = m.strs(&.{ m.b.pathJoin(&.{ v.gen_dir, "decomp" }), m.b.pathJoin(&.{ v.gen_dir, "decomp/include" }), m.b.pathJoin(&.{ v.gen_dir, "decomp/src" }) }),
            .defines = port_defs,
            .link_options = link,
            .libs = libs.items,
            .after = m.a.dupe(*Step, &.{v.mirror}) catch @panic("OOM"), // add_dependencies(port64_task_test p4_mirror)
        } });
    }
    // 210: Windows x64 coroutines (if(TARGET p4decomp64w) and find_program(P4_WINE64)): a tests-directory target of
    // the Linux tree whose own options retarget it to x86_64-w64-mingw32.
    if (g.port64w and m.o.wine64 != null) {
        const pe = v.pe_repro_link_flags;
        add(&out, m.a, .{ .no_twin = true, .spec = .{
            .name = "port64_coro_win_test",
            .suffix = ".exe",
            .sources = m.strs(&.{ m.t("port64_coro_win_test.c"), m.s("src/platform/os/os_win32.c") }),
            .compile_options = &.{ "--target=x86_64-w64-mingw32", "-m64", "-fno-builtin" },
            .link_options = m.cat(&.{ &.{ "--target=x86_64-w64-mingw32", "-m64" }, pe }),
            .libs = m.a.dupe(core.Lib, &.{ .{ .raw = "-lkernel32" }, .{ .raw = "-luser32" } }) catch @panic("OOM"),
        } });
    }
    return out.items;
}

/// 220: p4_ovr_test64(<exe> <test source> OVERRIDES ... [FLAGS ...] [SOURCES ...]) calls, in order.
fn overrideTests(m: *const M) []const core.OverrideTestSpec {
    return m.a.dupe(core.OverrideTestSpec, &.{
        .{ .exe = "qsort_test", .test_source = "tests/qsort_test.c", .overrides = &.{"00440bb8"}, .sources = m.strs(&.{m.s("src/platform/ee_bytes.c")}) },
        .{ .exe = "spline_test", .test_source = "tests/spline_test.c", .overrides = &.{"003b9a40"} },
        .{
            .exe = "lockstep_restore_witness_test",
            .test_source = "tests/lockstep_restore_witness_test.c",
            .overrides = &.{"0010e710"},
            .flags = m.strs(&.{ "-include", m.t("lockstep_restore_witness_prelude.h") }),
            .sources = m.strs(&.{ m.t("lockstep_align_guest_header_test.c"), m.s("src/platform/ee_bytes.c") }),
        },
        .{ .exe = "bmp_test", .test_source = "tests/bmp_test.c", .overrides = &.{"003d8c00"} },
        .{ .exe = "promoted_leaf_test", .test_source = "tests/promoted_leaf_test.c", .overrides = &.{
            "001f5680", "00396e80", "003cc250", "003d5710", "003b5fb0", "003b6390", "003b6420", "003b64c0",
            "003b4230", "003b65d0", "003b6540", "003d0790", "003cbf30", "003ce840", "0039a910", "003a2770",
            "003a5180", "003b4ab0", "003b4c10", "003b4ff0", "003b5650", "003b5ac0", "003b5bf0", "003b6020",
            "003b61e0",
        } },
        .{
            .exe = "palquant_test",
            .test_source = "tests/palquant_test.c",
            .overrides = &.{ "003f07d0", "003f0610", "003eff30", "003f0110", "003f0510", "003f0c10", "003f0de0", "003f02d0", "003f03f0", "003f1030", "003f1240", "003f13b0", "003f1520", "003f1600", "003f1700", "003f17e0", "003f1ce0" },
            // ${CMAKE_SOURCE_DIR}/decomp/include: the source tree's decomp checkout, as written (not ${P4_DECOMP})
            .flags = m.strs(&.{ "-I", m.s("decomp/include"), "-I", m.s("tests"), "-include", m.t("palquant_prelude.h") }),
        },
        .{
            .exe = "rtanim_test",
            .test_source = "tests/rtanim_test.c",
            .overrides = &.{ "00415ad0", "004165a0", "00415bc0", "00417310", "00416e10", "003d5840", "003d59d0", "003d5bc0", "003dcc70" },
            .sources = m.strs(&.{ m.s("src/platform/rw_rtanim_record.c"), m.s("src/platform/ee_bytes.c"), m.s("src/platform/ee_libm.c"), m.s("src/platform/ee_float.c") }),
        },
        .{
            .exe = "matrix_override_test",
            .test_source = "tests/matrix_override_test.c",
            .overrides = &.{ "003df9f0", "003e0e20", "003e0680" },
            .flags = m.strs(&.{ "-include", m.t("matrix_override_prelude.h") }),
        },
        .{
            .exe = "rxheap_test",
            .test_source = "tests/rxheap_test.c",
            .overrides = &.{ "00415070", "00414ef0", "00414930", "00414820", "00414e10", "00414a30", "00414b50", "00414c10" },
            .sources = m.strs(&.{m.s("src/platform/rx_heap_realloc.c")}),
        },
    }) catch @panic("OOM");
}

/// 220: printf_abi_calls<sfx>, the OBJECT library of tests/data/ee_printf_abi_calls.c rewritten by
/// `printf_rewrite.py --full` for the model, which p4_host64_exe links in place of a clone of 010's printf_abi_calls.
fn printfAbiCalls(m: *const M, model: Model) void {
    const b = m.b;
    const mv = core.vars(m.g, model);
    const sfx = model.suffix();
    const source = m.t("data/ee_printf_abi_calls.c");
    const run = b.addSystemCommand(&.{ mv.python, m.s("tools/port64/printf_rewrite.py"), "--full", "--clang", mv.cc, "--root", m.t("data"), "--out" });
    run.setName(m.fmt("printf_rewrite ee_printf_abi_calls{s}.i", .{sfx}));
    const cached = run.addOutputFileArg(m.fmt("ee_printf_abi_calls{s}.i", .{sfx}));
    run.addArg(source);
    run.addArg("--");
    run.addArgs(mv.model_flags);
    m.inputs(run, &.{ "tests/data/ee_printf_abi_calls.c", "tools/port64/printf_rewrite.py", "tools/port64/gptr_rewrite.py" });
    m.useCompiler(run);
    // ${CMAKE_CURRENT_BINARY_DIR}/ee_printf_abi_calls<sfx>.i: placed at its fixed path (the compile names it)
    const fixed = b.pathJoin(&.{ mv.build_dir, "tests", m.fmt("ee_printf_abi_calls{s}.i", .{sfx}) });
    const stage = b.addSystemCommand(&.{ mv.python, m.s("tools/zig_build_step.py"), "stage", "--" });
    stage.setName(m.fmt("stage ee_printf_abi_calls{s}.i", .{sfx}));
    stage.addFileArg2(cached, .{ .prefix = m.fmt("{s}=", .{fixed}) });
    stage.has_side_effects = true;
    const lib = core.addObjectLibrary(m.g, model, .{
        .name = "printf_abi_calls",
        .generated = m.a.dupe(core.GeneratedFiles, &.{.{
            .paths = m.strs(&.{fixed}),
            .ready = &stage.step,
            .cached = m.a.dupe(LazyPath, &.{cached}) catch @panic("OOM"),
        }}) catch @panic("OOM"),
    }) orelse return;
    core.setTwinLibrary(m.g, "printf_abi_calls", model, lib);
}

/// 210: the x86_64 task prototype, its retail-interpreter reference, the coroutine test (tools/port64/build64.py) and
/// crash_log_test64, behind the P4_X64_PROBE configure check; returns port64_task64 (ALL).
fn taskPrototypes(m: *const M, extra: *std.ArrayList(rg.Artifact)) *Step {
    const b = m.b;
    const v = m.v;
    const g = m.g;
    // P4_X64_PROBE: CMake skips the targets when the compiler cannot target x86_64; here the build fails instead.
    const probe = b.addSystemCommand(&.{ g.cc, "--target=x86_64-linux-gnu", "-fsyntax-only", "-x", "c" });
    probe.setName("P4_X64_PROBE (clang --target=x86_64-linux-gnu include/p4pc/guest.h)");
    probe.addFileArg(b.path("include/p4pc/guest.h"));
    probe.has_side_effects = true;
    m.useCompiler(probe);
    probe.expectExitCode(0);

    const task64 = b.step("port64_task64", "The x86_64 task prototype, retail reference, coroutine and crash-log tests (CMake port64_task64)");
    g.steps.artifacts.dependOn(task64); // add_custom_target(port64_task64 ALL ...)

    const build64 = struct {
        fn f(mm: *const M, name: []const u8, out_dir: []const u8, args: []const []const u8, after_probe: *Step) struct { run: *Run, exe: LazyPath } {
            const bb = mm.b;
            const run = bb.addSystemCommand(&.{ mm.v.python, mm.s("tools/port64/build64.py"), "--gen", mm.v.gen_dir, "--root", mm.root, "--out" });
            run.setName(mm.fmt("build64 {s}", .{name}));
            _ = run.addOutputDirectoryArg(out_dir);
            run.addArgs(args);
            run.addArg("--exe");
            const exe = run.addOutputFileArg(name);
            run.addArgs(&.{ "--python", mm.v.python, "--clang", mm.g.cc });
            mm.inputs(run, &.{"tools/port64/build64.py"});
            mm.useCompiler(run);
            run.step.dependOn(after_probe);
            return .{ .run = run, .exe = exe };
        }
    }.f;

    // P4_TASK_BRIDGE64_ARGS
    var bridge: std.ArrayList([]const u8) = .empty;
    var bridge_libs: [2]?core.LibraryRef = .{ null, null };
    if (g.mod_support) {
        if (core.library(g, "p4_mod_game64")) |game| {
            const mcore = core.library(g, "p4_mod_core64") orelse std.debug.panic("native_tests_c: p4_mod_game64 exists without p4_mod_core64", .{});
            bridge.appendSlice(m.a, &.{ "--lib=-Wl,--gc-sections", m.fmt("--lib={s}", .{game.path}), m.fmt("--lib={s}", .{mcore.path}) }) catch @panic("OOM");
            bridge_libs = .{ game, mcore };
        } else {
            bridge.appendSlice(m.a, &.{ "--src", v.mod_bridges, "--src", "src/platform/mods/hooks.c", "--src", "src/platform/mods/numbers.c", "--src", "src/platform/mods/failure.c", "--cflag=-ffunction-sections", "--cflag=-fdata-sections", "--lib=-Wl,--gc-sections" }) catch @panic("OOM");
        }
    }

    // P4_TASK64: DEPENDS p4_mirror (the rewritten unit and mirror headers): ordered after the mirror and re-run every build,
    // since the mirror tree it reads is not a declared file set.
    const task = build64(m, "port64_task_test64", "port64", m.cat(&.{
        &.{ "--unit", "src/Kernel/sdkTask.c", "--src", "tests/port64_task_test.c", "--src", "src/platform/ee_bytes.c" },
        bridge.items,
        &.{ "--src", "src/platform/guest.c", "--src", "src/platform/os/os_posix.c" },
    }), &probe.step);
    task.run.step.dependOn(v.mirror);
    task.run.has_side_effects = true;
    for (bridge_libs) |l| {
        const lib = l orelse continue;
        task.run.step.dependOn(lib.ready);
        task.run.addFileInput(lib.lazy);
    }
    m.inputs(task.run, &.{ "tools/port64/gptr_rewrite.py", "tests/port64_task_test.c", "src/platform/guest.c", "src/platform/ee_bytes.c", "include/p4pc/guest.h", "include/p4pc/compat.h" });

    const taskr = build64(m, "port64_task_retail64", "port64r", &.{
        "--src", "tests/port64_task_test.c", "--src", "tools/mipsdiff/r5900.c", "--src", "src/platform/guest.c", "--src", "src/platform/os/os_posix.c",
        "--cflag=-DP4_RUN_RETAIL=1", m.fmt("--cflag=-I{s}", .{m.s("tools/mipsdiff")}), "--lib=-lm",
    }, &probe.step);
    taskr.run.step.dependOn(v.mirror);
    taskr.run.has_side_effects = true;
    m.inputs(taskr.run, &.{ "tests/port64_task_test.c", "src/platform/guest.c", "tools/mipsdiff/r5900.c", "include/p4pc/guest.h" });

    const coro = build64(m, "port64_coro_test64", "port64c", &.{ "--src", "tests/port64_coro_test.c", "--src", "src/platform/guest.c", "--src", "src/platform/os/os_posix.c" }, &probe.step);
    m.inputs(coro.run, &.{ "tests/port64_coro_test.c", "src/platform/guest.c", "src/platform/os/os_posix.c", "include/p4pc/guest.h", "include/p4pc/os.h" });

    // P4_CRASH64: a direct compile and link with ${CMAKE_C_COMPILER}
    const crash = b.addSystemCommand(&.{ g.cc, "--target=x86_64-linux-gnu", "-m64", "-O2", "-g", "-DP4_GUEST_CHECK", "-ffunction-sections", "-fdata-sections", m.fmt("-I{s}", .{m.s("include")}), "-include", "p4pc/compat.h" });
    crash.setName("crash_log_test64");
    crash.addFileArg(b.path("tests/crash_log_test.c"));
    crash.addFileArg(b.path("src/platform/guest.c"));
    crash.addFileArg(b.path("src/platform/os/os_posix.c"));
    crash.addArgs(&.{ "-Wl,--gc-sections", "-ldl", "-o" });
    const crash_exe = crash.addOutputFileArg("crash_log_test64");
    m.inputs(crash, &.{ "include/p4pc/os.h", "include/p4pc/guest.h", "include/p4pc/compat.h" });
    m.useCompiler(crash);
    crash.step.dependOn(&probe.step);

    for ([_]struct { []const u8, LazyPath, *Step }{
        .{ "port64_task_test64", task.exe, &task.run.step },
        .{ "port64_task_retail64", taskr.exe, &taskr.run.step },
        .{ "port64_coro_test64", coro.exe, &coro.run.step },
        .{ "crash_log_test64", crash_exe, &crash.step },
    }) |e| {
        extra.append(m.a, .{ .name = e[0], .path = e[1] }) catch @panic("OOM");
        task64.dependOn(e[2]);
    }
    return task64;
}

// ---------------------------------------------------------------------------------------------------------------------
// Tests

const Word = union(enum) {
    lit: []const u8,
    /// ${P4_PYTHON}
    python,
    /// ${CMAKE_SOURCE_DIR}/<rel>
    src: []const u8,
    /// $<TARGET_FILE:name> (graph.artifacts)
    target: []const u8,
};

const Test = struct {
    name: []const u8,
    argv: []const Word,
    /// ENVIRONMENT before tests/CMakeLists.txt's tail (P4PC_MODS=0, P4PC_PACKS=0, P4PC_BUNDLED_MOD_DIR=)
    env: []const []const u8 = &.{},
    labels: []const []const u8 = &.{},
    skip: ?u32 = null,
    timeout: ?u32 = null,
    /// The configure condition held but the executable cannot be built in this profile: the test fails with this.
    missing: ?[]const u8 = null,
};

/// 250-tools.cmake: the ovdiff (`sfx` "") or ovdiff64 (`sfx` "64", LABELS port64) test set.
fn ovdiffTests(m: *const M, tests: *std.ArrayList(Test), exe: []const u8, sfx: []const u8, missing: ?[]const u8, port64: bool) void {
    const labels: []const []const u8 = if (port64) &.{"port64"} else &.{};
    const mipsdiff = m.fmt("P4_MIPSDIFF_DIR={s}", .{m.s("tools/mipsdiff")});
    const a = m.a;
    const list = [_]Test{
        // ovdiff_ee_abi: no SKIP_RETURN_CODE for i386; 77 for the 64-bit one
        .{ .name = m.fmt("ovdiff_ee_abi{s}", .{sfx}), .argv = a.dupe(Word, &.{ .{ .target = exe }, .{ .lit = "--selftest-abi" } }) catch @panic("OOM"), .env = m.strs(&.{mipsdiff}), .skip = if (port64) 77 else null, .labels = labels },
        .{ .name = m.fmt("ovdiff_ee_lanes{s}", .{sfx}), .argv = a.dupe(Word, &.{ .{ .target = exe }, .{ .lit = "--selftest-lanes" } }) catch @panic("OOM"), .skip = 77, .labels = labels },
        .{ .name = m.fmt("ovdiff_synthetic_words{s}", .{sfx}), .argv = a.dupe(Word, &.{ .{ .target = exe }, .{ .lit = "--selftest-synthetic-words" } }) catch @panic("OOM"), .labels = labels },
        .{ .name = m.fmt("mipsdiff_replay_oracle{s}", .{sfx}), .argv = a.dupe(Word, &.{ .python, .{ .src = "tests/mipsdiff_replay_test.py" }, .{ .target = exe } }) catch @panic("OOM"), .skip = 77, .labels = labels },
        .{ .name = m.fmt("mipsdiff_windows{s}", .{sfx}), .argv = a.dupe(Word, &.{ .python, .{ .src = "tests/mipsdiff_window_test.py" }, .{ .target = exe } }) catch @panic("OOM"), .skip = 77, .labels = labels },
    };
    for (list) |te| {
        var x = te;
        x.missing = missing;
        tests.append(a, x) catch @panic("OOM");
    }
}

/// One tools/zig_runtime_gates.py run per test, with CTest's semantics (TIMEOUT, SKIP_RETURN_CODE as NOT RUN, the tests
/// directory as WORKING_DIRECTORY, ENVIRONMENT in order, CROSSCOMPILING_EMULATOR before a target command).
fn emitTests(m: *const M, nonserial: *Step, tests: []const Test) void {
    const b = m.b;
    const runner = b.path("tools/zig_runtime_gates.py");
    const binary_dir = m.v.build_dir;
    const wf = b.addWriteFiles();
    const dirs = wf.add("native_tests_c.dirs", "tests\n");
    const prepare = b.addSystemCommand(&.{m.v.python});
    prepare.setName("native tests C: configure directories");
    prepare.addFileArg2(runner, .{ .make_absolute = true });
    prepare.addArgs(&.{ "prepare", m.fmt("--binary-dir={s}", .{binary_dir}) });
    prepare.addFileArg2(dirs, .{ .prefix = "--dirs=", .make_absolute = true });
    prepare.has_side_effects = true;

    var label_steps: std.StringArrayHashMapUnmanaged(*Step) = .empty;
    for (tests) |te| {
        const top = b.step(m.fmt("{s}:{s}", .{ m.o.step_prefix, te.name }), m.fmt("Run native test {s}", .{te.name}));
        nonserial.dependOn(top);
        for (te.labels) |l| {
            const gop = label_steps.getOrPut(m.a, l) catch @panic("OOM");
            if (!gop.found_existing) gop.value_ptr.* = b.step(m.fmt("{s}-label:{s}", .{ m.o.step_prefix, l }), m.fmt("Run the native tests of zigbuild/native_tests_c.zig labelled {s}", .{l}));
            gop.value_ptr.*.dependOn(top);
        }
        if (missingInput(m, te)) |why| {
            top.dependOn(&b.addFail(m.fmt("{s}: {s}", .{ te.name, why })).step);
            continue;
        }
        const run = b.addSystemCommand(&.{m.v.python});
        run.setName(m.fmt("native test {s}", .{te.name}));
        run.addFileArg2(runner, .{ .make_absolute = true });
        run.addArgs(&.{ "run", m.fmt("--name={s}", .{te.name}) });
        run.addArg(m.fmt("--timeout={d}", .{te.timeout orelse m.o.default_timeout_s}));
        if (te.skip) |code| run.addArg(m.fmt("--skip-code={d}", .{code}));
        run.addArg(m.fmt("--cwd={s}", .{b.pathJoin(&.{ binary_dir, "tests" })}));
        run.addArg(m.fmt("--lock-dir={s}", .{m.o.lock_dir}));
        for (te.labels) |l| run.addArg(m.fmt("--label={s}", .{l}));
        if (m.o.retail) |ret| {
            run.addArg(m.fmt("--retail-iso={s}", .{ret.iso}));
            run.addArg(m.fmt("--retail-elf={s}", .{ret.elf}));
            if (ret.ref_dir) |d| run.addArg(m.fmt("--ref-dir={s}", .{d}));
        }
        for (te.env) |e| run.addArg(m.fmt("--env={s}", .{e}));
        for ([_][]const u8{ "P4PC_MODS=0", "P4PC_PACKS=0", "P4PC_BUNDLED_MOD_DIR=" }) |e| run.addArg(m.fmt("--env={s}", .{e}));
        run.addArg(m.fmt("--result={s}/zig-gates/{s}.json", .{ binary_dir, te.name }));
        run.addArg("--");
        if (te.argv[0] == .target) for (core.emulator(m.g, .i386)) |w| run.addArg(w);
        for (te.argv) |w| switch (w) {
            .lit => |s| run.addArg(s),
            .python => run.addArg(m.v.python),
            .src => |rel| run.addFileArg2(b.path(rel), .{ .make_absolute = true }),
            // $<TARGET_FILE:n>: the staged product at its absolute build-tree path (beside the DLLs/runtime files the
            // graph places there), not the cache path, which a relative cwd would misresolve; re-run when it changes.
            .target => |n| {
                const art = m.g.artifacts.get(n).?;
                run.addArg(art.path);
                run.step.dependOn(art.step);
                run.addFileInput(art.lazy);
            },
        };
        run.step.dependOn(&prepare.step);
        run.has_side_effects = true;
        run.expectExitCode(0);
        top.dependOn(&run.step);
    }
}

fn missingInput(m: *const M, te: Test) ?[]const u8 {
    if (te.missing) |why| return why;
    for (te.argv) |w| if (w == .target and m.g.artifacts.get(w.target) == null)
        return m.fmt("the build supplied no artifact '{s}'", .{w.target});
    return null;
}
