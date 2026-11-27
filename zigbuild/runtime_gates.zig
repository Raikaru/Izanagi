//! Runtime gates under `zig build`, without CMake, CTest or Ninja.
//!
//! Scope: the gameplay, scene, movie, CRI and mod-witness gates of
//! tests/cmake/090-movie.cmake .. 192-randomizer-mod.cmake and the port64
//! runtime/host cases of 210-port64-runtime.cmake, 220-port64-host.cmake and
//! 290-port64-host-cases.cmake.
//!
//! The registry replays, in CMake order, the semantics that give those tests
//! their final shape: the add_test wrapper of 000-registration.cmake (64-bit
//! game clones, plain tests, the host-tooling lock), the fragments' own
//! add_test/set_tests_properties calls (static loops unrolled, configure
//! conditions kept as `Profile` conditions), the policy fragments 200, 230,
//! 240, 250 (plain label) and 290 (host twins, wine fixture and Wine budget),
//! and the tail of tests/CMakeLists.txt. Each test becomes one Run step of
//! tools/zig_runtime_gates.py, which enforces TIMEOUT, SKIP_RETURN_CODE,
//! WORKING_DIRECTORY, ENVIRONMENT, RESOURCE_LOCK, RUN_SERIAL and the
//! software-only GL policy. A skip return code is reported NOT RUN and fails
//! the step: absence of an input is never a pass.
//!
//! The CTest gates stay the authority. Every result this module produces is
//! non-qualifying; there is no hardware-GPU path here (see `GlPolicy`).
//!
//! Integration contract (build.zig owns the call):
//!   runtime_gates.register(b, all_step, .{ ... Options ... });
//! creates `<prefix>:<test>` and `<prefix>-label:<label>` top-level steps,
//! `<prefix>-manifest` (installs gates/runtime_gates.json, CTest json-v1 shape)
//! and makes `all` depend on the gates selected by `Options.all`.

const std = @import("std");

pub const Port64Arch = enum { none, x86_64, aarch64 };

/// Configure facts the CMake fragments branch on. Every field is explicit:
/// the module never guesses a profile from the host.
pub const Profile = struct {
    /// P4_WIN32: the 32-bit Windows build.
    win32: bool,
    /// P4_APPLE.
    apple: bool,
    /// CMAKE_SYSTEM_NAME is Linux and CMAKE_HOST_SYSTEM_PROCESSOR matches x86_64|AMD64 (210-port64-runtime.cmake).
    linux_x86_64_host: bool,
    /// P4_PORT64_ARCH.
    port64_arch: Port64Arch,
    /// P4_PORT64_LINK.
    port64_link: bool,
    /// P4_PORT64_OBJS is non-empty (the rewritten 64-bit mirror is built).
    port64_objs: bool,
    /// P4_X64_PROBE == 0: `clang --target=x86_64-linux-gnu -fsyntax-only include/p4pc/guest.h` succeeded (210).
    x64_probe: bool,
    /// P4_RELEASE.
    release: bool,
    /// P4PC_MOD_WITNESS_TESTS.
    mod_witness_tests: bool,
    /// Build-level CMake targets that exist (`if(TARGET ...)`): any of p4pc, p4pc_viewer, p4pc64,
    /// p4pc_viewer64, p4pc64w, p4decomp64w, luau_script_runner64.
    targets: []const []const u8,
};

pub const ArtifactKind = enum { executable, file, directory };

/// A built input of a gate, keyed by its CMake target name ($<TARGET_FILE:name> or a bare
/// command name) or, for custom-command outputs, by the output's file name
/// (port64_task_test64, port64_task_retail64, port64_coro_test64, crash_log_test64).
/// Host twins need `<exe>64` / `<exe>64w`. A gate whose artifact is missing fails.
pub const Artifact = struct {
    name: []const u8,
    path: std.Build.LazyPath,
    /// Absolute staged path and producer when the executable needs adjacent runtime files.
    runtime_path: ?[]const u8 = null,
    ready: ?*std.Build.Step = null,
    kind: ArtifactKind = .executable,
    /// The target's CROSSCOMPILING_EMULATOR (empty when unset).
    emulator: []const []const u8 = &.{},
};

/// Retail inputs. Unset: P4PC_ISO, P4PC_ELF and P4PC_REF_DIR are removed from every gate's
/// environment, so retail gates exit with their skip code, which fails as NOT RUN.
pub const Retail = struct {
    /// P4PC_ISO.
    iso: []const u8,
    /// P4PC_ELF.
    elf: []const u8,
    /// P4PC_REF_DIR; null keeps the scripts' default (<dir of iso>/refs).
    ref_dir: ?[]const u8 = null,
    /// The P4_RETAIL configure value (port64_task, port64_task_retail); null leaves them
    /// unregistered exactly as an unset P4_RETAIL does.
    configured_elf: ?[]const u8 = null,
};

pub const Port64 = struct {
    /// ${P4_G64}: the generated 64-bit mirror directory (units/, abi.json, active_units.txt).
    g64: ?std.Build.LazyPath = null,
    /// ${P4_CENSUS_BASELINE}, relative to the source root; null = unset.
    census_baseline: ?[]const u8 = null,
    /// ${CMAKE_C_COMPILER} given to `unit64.py report --clang`.
    c_compiler: ?[]const u8 = null,
};

/// How a gate that can open a GL context (label `gl`, label `mod-witness`, or a command that runs
/// p4pc/p4pc_viewer in any ABI) is admitted. There is deliberately no hardware mode: qualifying
/// GPU gates run only through the existing CTest manifest and local gate runner custody path,
/// which stay the authority. Gates that name a hardware renderer (P4PC_GATE_RENDERER not
/// llvmpipe) are always refused here.
pub const GlPolicy = enum {
    /// Fail every GL-capable gate.
    refuse,
    /// Linux only, enforced by the runner: Mesa software rendering forced, refused when any GPU device
    /// node is accessible, and failed when the log names a non-software renderer. Results are
    /// recorded as non-qualifying.
    software,
};

/// The `all` selection, as exact CTest -L / -LE / -E values.
pub const Selection = struct {
    include_labels: ?[]const []const u8 = null,
    exclude_labels: []const []const u8 = &.{},
    exclude_names: []const []const u8 = &.{},
};

/// An executable target defined outside this module's fragments that a host twin may rebuild.
pub const ExternalExe = struct {
    name: []const u8,
    /// Its LINK_OPTIONS contain --wrap= (not twinned on macOS).
    wraps: bool = false,
};

/// A P4_CMD_TESTS entry registered outside this module's fragments, as its properties stand
/// when 290-port64-host-cases.cmake runs (after 200/230/240/250). Supplied so that
/// p4_host64_tests can twin it with the same rules as the domain tests.
pub const HostCase = struct {
    name: []const u8,
    argv: []const Arg,
    env: []const Env = &.{},
    labels: []const []const u8 = &.{},
    timeout: ?u32 = null,
    skip: ?u32 = null,
    /// WORKING_DIRECTORY relative to binary_dir ("" = binary_dir); null = unset.
    cwd: ?[]const u8 = null,
    run_serial: bool = false,
    locks: []const []const u8 = &.{},
};

pub const Options = struct {
    /// Absolute directory used where CMake uses ${CMAKE_BINARY_DIR} (outputs, save/, tests/).
    binary_dir: []const u8,
    /// ${P4_PYTHON}.
    python: []const u8,
    profile: Profile,
    artifacts: []const Artifact,
    retail: ?Retail = null,
    port64: Port64 = .{},
    /// ${P4_WINE64} (find_program wine64/wine); null = not found.
    wine64: ?[]const u8 = null,
    /// ${P4_MOD_WINE} as a CMake list.
    mod_wine: []const []const u8 = &.{},
    gl: GlPolicy,
    /// Directory of the cross-process lock files (RESOURCE_LOCK names, RUN_SERIAL). Share it with
    /// every module that honours the host-tooling lock.
    lock_dir: []const u8,
    /// The CTest --timeout the suite passes, for tests without a TIMEOUT.
    default_timeout_s: u32,
    /// P4_WINE_TIMEOUT_SCALE.
    wine_timeout_scale: u32 = 4,
    step_prefix: []const u8 = "gate",
    all: Selection = .{},
    /// Test names registered by other modules (`if(TEST ...)` checks of the host twins).
    external_test_names: []const []const u8 = &.{},
    external_host_cases: []const HostCase = &.{},
    external_executables: []const ExternalExe = &.{},
};

pub const Src = struct { prefix: []const u8 = "", path: []const u8, dir: bool = false };
pub const Gen = struct { prefix: []const u8 = "", sub: []const u8, dir: bool = false };

/// One command word, symbolic until the Run step is made.
pub const Arg = union(enum) {
    lit: []const u8,
    /// ${P4_PYTHON}
    python,
    /// ${CMAKE_COMMAND}; only as `cmake -E env NAME=VALUE... <command>`
    cmake,
    /// ${P4_WINE64}
    wine64,
    /// ${CMAKE_CTEST_COMMAND}: answered from this registry's manifest
    ctest,
    /// [prefix]${CMAKE_SOURCE_DIR}/path
    src: Src,
    /// ${CMAKE_BINARY_DIR}/path ("" = the directory itself)
    bin: []const u8,
    /// $<TARGET_FILE:name>
    target: []const u8,
    /// a target named as the command (CMake substitutes its file)
    bare: []const u8,
    /// a configure-known build output that is not a target (an Artifact key)
    file: []const u8,
    /// [prefix]${P4_G64}/sub
    gen: Gen,
};

pub const EnvValue = union(enum) {
    lit: []const u8,
    src: []const u8,
    src_dir: []const u8,
    bin: []const u8,
};

pub const Env = struct { key: []const u8, value: EnvValue };

fn lit(s: []const u8) Arg {
    return .{ .lit = s };
}
fn src(p: []const u8) Arg {
    return .{ .src = .{ .path = p } };
}
fn srcp(prefix: []const u8, p: []const u8) Arg {
    return .{ .src = .{ .prefix = prefix, .path = p } };
}
fn bin(p: []const u8) Arg {
    return .{ .bin = p };
}
fn tf(t: []const u8) Arg {
    return .{ .target = t };
}
fn bare(t: []const u8) Arg {
    return .{ .bare = t };
}
fn file(k: []const u8) Arg {
    return .{ .file = k };
}
const py: Arg = .python;
const cmake: Arg = .cmake;

fn env(k: []const u8, v: []const u8) Env {
    return .{ .key = k, .value = .{ .lit = v } };
}
fn esrc(k: []const u8, p: []const u8) Env {
    return .{ .key = k, .value = .{ .src = p } };
}
fn esrcdir(k: []const u8, p: []const u8) Env {
    return .{ .key = k, .value = .{ .src_dir = p } };
}
fn ebin(k: []const u8, p: []const u8) Env {
    return .{ .key = k, .value = .{ .bin = p } };
}

/// Executables add_executable defines in this module's fragments (TYPE EXECUTABLE), and whether
/// their LINK_OPTIONS wrap symbols (--wrap=; 150-cri-audio, 160-tools-ovdiff, 220-port64-host).
const domain_executables = [_]ExternalExe{
    .{ .name = "adx_wav_test" },
    .{ .name = "adxt_playend_test" },
    .{ .name = "ahx_wav_test" },
    .{ .name = "aixp_wav_test" },
    .{ .name = "axrna_test" },
    .{ .name = "cri_acs_step_test" },
    .{ .name = "cri_acs_test" },
    .{ .name = "cri_acssnd_test" },
    .{ .name = "cri_adxi_rpc_test" },
    .{ .name = "cri_adxm_test" },
    .{ .name = "cri_adxt_start_test" },
    .{ .name = "cri_cpu_adapter_test", .wraps = true },
    .{ .name = "cri_cvfs_test" },
    .{ .name = "cri_file_size_test" },
    .{ .name = "cri_init_test" },
    .{ .name = "cri_rofs_metadata_test" },
    .{ .name = "cri_sfd_callback_test", .wraps = true },
    .{ .name = "cri_stm_test" },
    .{ .name = "dungeon_digest_test" },
    .{ .name = "ffmpeg_license_test" },
    .{ .name = "import_cbs_test" },
    .{ .name = "import_max_test" },
    .{ .name = "mixer_device_test" },
    .{ .name = "movie_audio_test" },
    .{ .name = "movie_smoke" },
    .{ .name = "movie_start_test" },
    .{ .name = "movie_task_smoke" },
    .{ .name = "mwply_test" },
    .{ .name = "nav_probe_test" },
    .{ .name = "ovdiff", .wraps = true },
    .{ .name = "ovdiff64", .wraps = true },
    .{ .name = "port64_coro_win_test" },
    .{ .name = "port64_task_test" },
    .{ .name = "rofs_open_test" },
    .{ .name = "save_import_test" },
    .{ .name = "sebank_test" },
    .{ .name = "sfmpvp_test" },
    .{ .name = "walk_battle_rules_test" },
    .{ .name = "walk_choice_test" },
};

const Test = struct {
    name: []const u8,
    argv: []const Arg,
    env: std.ArrayList(Env) = .empty,
    labels: std.ArrayList([]const u8) = .empty,
    timeout: ?u32 = null,
    skip: ?u32 = null,
    /// WORKING_DIRECTORY relative to binary_dir ("" = binary_dir); null = CTest's default (binary_dir/tests).
    cwd: ?[]const u8 = null,
    locks: std.ArrayList([]const u8) = .empty,
    run_serial: bool = false,
    fixtures_setup: std.ArrayList([]const u8) = .empty,
    fixtures_required: std.ArrayList([]const u8) = .empty,
    /// Registered through the add_test wrapper (P4_CMD_TESTS).
    in_cmd_tests: bool = false,
};

const Props = struct {
    skip: ?u32 = null,
    timeout: ?u32 = null,
    labels: ?[]const []const u8 = null,
    env: ?[]const Env = null,
    cwd: ?[]const u8 = null,
};

const Registry = struct {
    b: *std.Build,
    gpa: std.mem.Allocator,
    o: *const Options,
    p: Profile,
    tests: std.ArrayList(*Test) = .empty,
    /// P4_GATE_CLONES (000-registration.cmake).
    clones: std.ArrayList([]const u8) = .empty,
    /// P4_PLAIN_TESTS (000-registration.cmake).
    plain: std.ArrayList([]const u8) = .empty,
    /// Directories CMake makes at configure time (file(MAKE_DIRECTORY)), relative to binary_dir.
    dirs: std.ArrayList([]const u8) = .empty,
    /// P4_ALL_TESTS as 230-platform-policy.cmake reads it.
    all_at_230: []const []const u8 = &.{},

    fn find(r: *Registry, name: []const u8) ?*Test {
        for (r.tests.items) |t| if (std.mem.eql(u8, t.name, name)) return t;
        return null;
    }

    fn get(r: *Registry, name: []const u8) *Test {
        return r.find(name) orelse std.debug.panic("runtime_gates: no test {s} to set properties on", .{name});
    }

    /// `if(TEST name)`, including tests other modules register.
    fn has(r: *Registry, name: []const u8) bool {
        return r.find(name) != null or contains(r.o.external_test_names, name);
    }

    /// `if(TARGET name)` for the build-level targets.
    fn hasTarget(r: *Registry, name: []const u8) bool {
        return contains(r.p.targets, name);
    }

    fn fmt(r: *Registry, comptime f: []const u8, args: anytype) []const u8 {
        return r.b.fmt(f, args);
    }

    fn mkdirBin(r: *Registry, rel: []const u8) void {
        if (!contains(r.dirs.items, rel)) r.dirs.append(r.gpa, rel) catch @panic("OOM");
    }

    /// _add_test: CMake refuses a second test of one name.
    fn rawAdd(r: *Registry, name: []const u8, argv: []const Arg) *Test {
        if (r.has(name)) std.debug.panic("runtime_gates: add_test given test NAME {s} which already exists", .{name});
        const t = r.gpa.create(Test) catch @panic("OOM");
        t.* = .{ .name = name, .argv = r.gpa.dupe(Arg, argv) catch @panic("OOM") };
        r.tests.append(r.gpa, t) catch @panic("OOM");
        return t;
    }

    /// The add_test wrapper of 000-registration.cmake.
    fn add(r: *Registry, name: []const u8, argv: []const Arg) void {
        const t = r.rawAdd(name, argv);
        t.in_cmd_tests = true;
        // `cmake -E env NAME=VALUE ... <program>`: the program is the first word after the assignments.
        var first = argv[0];
        if (argv[0] == .cmake) {
            for (argv[1..]) |a| {
                if (!isEnvWord(a)) {
                    first = a;
                    break;
                }
            }
        }
        if (first != .bare and !hasTargetGenex(argv)) {
            r.plain.append(r.gpa, name) catch @panic("OOM");
            if (first == .python) r.setList(&t.locks, &.{"host-tooling"});
        }
        if (r.hasTarget("p4pc64")) {
            var found = false;
            const cmd64 = r.gpa.alloc(Arg, argv.len) catch @panic("OOM");
            for (argv, 0..) |a, i| {
                if (a == .target and std.mem.eql(u8, a.target, "p4pc")) {
                    cmd64[i] = tf("p4pc64");
                    found = true;
                } else cmd64[i] = a;
            }
            // scene_battle_floor_a64 has its own limit (210-port64-runtime.cmake).
            if (std.mem.eql(u8, name, "scene_battle_floor_a")) found = false;
            const name64 = r.fmt("{s}64", .{name});
            if (found and !r.has(name64)) {
                _ = r.rawAdd(name64, cmd64);
                r.clones.append(r.gpa, name) catch @panic("OOM");
            }
        }
    }

    fn setList(r: *Registry, list: *std.ArrayList([]const u8), items: []const []const u8) void {
        list.clearRetainingCapacity();
        list.appendSlice(r.gpa, items) catch @panic("OOM");
    }

    fn setEnv(r: *Registry, t: *Test, items: []const Env) void {
        t.env.clearRetainingCapacity();
        t.env.appendSlice(r.gpa, items) catch @panic("OOM");
    }

    /// set_tests_properties(<names> PROPERTIES ...): each given property is replaced.
    fn props(r: *Registry, names: []const []const u8, p: Props) void {
        for (names) |n| {
            const t = r.get(n);
            if (p.skip) |v| t.skip = v;
            if (p.timeout) |v| t.timeout = v;
            if (p.labels) |v| r.setList(&t.labels, v);
            if (p.env) |v| r.setEnv(t, v);
            if (p.cwd) |v| t.cwd = v;
        }
    }

    fn appendLabels(r: *Registry, name: []const u8, labels: []const []const u8) void {
        r.get(name).labels.appendSlice(r.gpa, labels) catch @panic("OOM");
    }

    fn appendEnv(r: *Registry, name: []const u8, items: []const Env) void {
        r.get(name).env.appendSlice(r.gpa, items) catch @panic("OOM");
    }

    fn emulator(r: *Registry, target: []const u8) []const []const u8 {
        for (r.o.artifacts) |a| if (std.mem.eql(u8, a.name, target)) return a.emulator;
        return &.{};
    }

    fn isExecutable(r: *Registry, name: []const u8) bool {
        for (domain_executables) |e| if (std.mem.eql(u8, e.name, name)) return true;
        for (r.o.external_executables) |e| if (std.mem.eql(u8, e.name, name)) return true;
        return false;
    }

    fn wraps(r: *Registry, name: []const u8) bool {
        for (domain_executables) |e| if (std.mem.eql(u8, e.name, name)) return e.wraps;
        for (r.o.external_executables) |e| if (std.mem.eql(u8, e.name, name)) return e.wraps;
        return false;
    }

    fn wineprefix(r: *Registry) []const u8 {
        return r.fmt("WINEPREFIX={s}/tests/wine64prefix", .{r.o.binary_dir});
    }
};

fn contains(list: []const []const u8, s: []const u8) bool {
    for (list) |x| if (std.mem.eql(u8, x, s)) return true;
    return false;
}

fn isEnvWord(a: Arg) bool {
    if (a != .lit) return false;
    const s = a.lit;
    if (std.mem.eql(u8, s, "-E") or std.mem.eql(u8, s, "env")) return true;
    const eq = std.mem.findScalar(u8, s, '=') orelse return false;
    return isIdentifier(s[0..eq]);
}

fn isIdentifier(s: []const u8) bool {
    if (s.len == 0) return false;
    for (s, 0..) |c, i| {
        const ok = c == '_' or std.ascii.isAlphabetic(c) or (i > 0 and std.ascii.isDigit(c));
        if (!ok) return false;
    }
    return true;
}

fn hasTargetGenex(argv: []const Arg) bool {
    for (argv) |a| switch (a) {
        .target => return true,
        .lit => |s| if (std.mem.find(u8, s, "$<TARGET") != null) return true,
        else => {},
    };
    return false;
}

fn hasLabel(t: *const Test, label: []const u8) bool {
    return contains(t.labels.items, label);
}

/// `name MATCHES "64(w)?$"`.
fn endsWith64(name: []const u8) bool {
    return std.mem.endsWith(u8, name, "64") or std.mem.endsWith(u8, name, "64w");
}

/// The ENVIRONMENT list as CMake joins it, for the regex checks of the policies.
fn envEntryText(r: *Registry, e: Env) []const u8 {
    return switch (e.value) {
        .lit => |v| r.fmt("{s}={s}", .{ e.key, v }),
        .src, .src_dir => |v| r.fmt("{s}=${{CMAKE_SOURCE_DIR}}/{s}", .{ e.key, v }),
        .bin => |v| r.fmt("{s}={s}/{s}", .{ e.key, r.o.binary_dir, v }),
    };
}

/// 200-platform-policy.cmake: a save root per test; the virtual clock for `scenes`.
fn policy200(r: *Registry) void {
    for (r.tests.items) |t| {
        const rel = r.fmt("save/{s}", .{t.name});
        r.mkdirBin(rel);
        t.env.append(r.gpa, ebin("P4PC_SAVE_DIR", rel)) catch @panic("OOM");
        if (hasLabel(t, "scenes")) t.env.append(r.gpa, env("P4PC_VIRTUAL_CLOCK", "1")) catch @panic("OOM");
    }
}

/// 230-platform-policy.cmake: no player settings, scripted input unless the virtual
/// controller is explicit, and the aarch64 qemu-user time scale.
fn policy230(r: *Registry) void {
    var names: std.ArrayList([]const u8) = .empty;
    for (r.tests.items) |t| {
        names.append(r.gpa, t.name) catch @panic("OOM");
        t.env.append(r.gpa, env("P4PC_CONFIG", "none")) catch @panic("OOM");
        var virtual = false;
        for (t.env.items) |e| {
            if (std.mem.eql(u8, envEntryText(r, e), "P4PC_INPUT=virtual")) virtual = true;
        }
        if (!virtual) t.env.append(r.gpa, env("P4PC_INPUT", "script")) catch @panic("OOM");
    }
    if (r.p.port64_arch == .aarch64) {
        for (r.tests.items) |t| {
            if (t.timeout) |v| {
                if (v == 0) continue;
                t.timeout = v * 4;
                t.env.append(r.gpa, env("P4PC_GATE_TIMEOUT", r.fmt("{d}", .{v * 4}))) catch @panic("OOM");
            }
        }
    }
    r.all_at_230 = names.items;
}

/// 240-port64-targets.cmake: port64 labels, the clones' copied properties, scenes64.
fn policy240(r: *Registry) void {
    for ([_][]const u8{ "boot64", "port64_coro", "port64_task", "port64_task_retail", "port64_coro_win", "port64_mirror", "port64_census", "seam_abi_mirror" }) |n| {
        if (r.find(n)) |t| t.labels.append(r.gpa, "port64") catch @panic("OOM");
    }
    for (r.clones.items) |n| {
        const t = r.get(n);
        const t64 = r.get(r.fmt("{s}64", .{n}));
        if (t.timeout) |v| {
            if (v != 0) t64.timeout = v;
        }
        if (t.skip) |v| {
            if (v != 0) t64.skip = v;
        }
        if (t.cwd) |v| t64.cwd = v;
        if (t.env.items.len != 0) r.setEnv(t64, t.env.items);
        const slow = contains(&.{ "story_to_0415", "street_0417", "castle_gate_0417", "castle_2f_0417", "home_0417", "save_play", "fields_load", "newgame_to_field", "systems_club_floor_menu", "scene_tv_world_enter", "scene_shadow_haze" }, n);
        if (slow) {
            r.setList(&t64.labels, &.{ "gl", "port64", "slow" });
        } else {
            r.setList(&t64.labels, &.{ "gl", "port64" });
        }
    }
    for (r.all_at_230) |n| {
        const t = r.get(n);
        if (!hasLabel(t, "scenes")) continue;
        const t64 = r.find(r.fmt("{s}64", .{n})) orelse continue;
        t64.labels.append(r.gpa, "scenes64") catch @panic("OOM");
        var clocked = false;
        for (t64.env.items) |e| {
            if (std.mem.find(u8, envEntryText(r, e), "P4PC_VIRTUAL_CLOCK=") != null) clocked = true;
        }
        if (!clocked) t64.env.append(r.gpa, env("P4PC_VIRTUAL_CLOCK", "1")) catch @panic("OOM");
    }
}

/// 250-tools.cmake: every plain test so far is labelled port64.
fn policy250(r: *Registry) void {
    for (r.plain.items) |n| r.get(n).labels.append(r.gpa, "port64") catch @panic("OOM");
}

/// tests/CMakeLists.txt tail: mods and packs off except the admitted bundled witnesses;
/// no test reads the bundled mod directory beside the program.
fn policyTail(r: *Registry) void {
    for (r.tests.items) |t| {
        if (!isAdmittedBundledWitness(t.name)) {
            t.env.append(r.gpa, env("P4PC_MODS", "0")) catch @panic("OOM");
            t.env.append(r.gpa, env("P4PC_PACKS", "0")) catch @panic("OOM");
        }
        t.env.append(r.gpa, env("P4PC_BUNDLED_MOD_DIR", "")) catch @panic("OOM");
    }
}

/// `^mod_(example|qol|physsplit)_game(64w|64)?$`
fn isAdmittedBundledWitness(name: []const u8) bool {
    for ([_][]const u8{ "mod_example_game", "mod_qol_game", "mod_physsplit_game" }) |stem| {
        if (!std.mem.startsWith(u8, name, stem)) continue;
        const rest = name[stem.len..];
        if (rest.len == 0 or std.mem.eql(u8, rest, "64") or std.mem.eql(u8, rest, "64w")) return true;
    }
    return false;
}

/// A P4_CMD_TESTS entry as p4_host64_tests sees it.
const CaseView = struct {
    name: []const u8,
    argv: []const Arg,
    env: []const Env,
    labels: []const []const u8,
    timeout: ?u32,
    skip: ?u32,
    cwd: ?[]const u8,
    run_serial: bool,
    locks: []const []const u8,
};

/// p4_host64_tests(<suffix>) of 220-port64-host.cmake over one case.
fn hostTwin(r: *Registry, c: CaseView, suffix: []const u8) void {
    const w64 = std.mem.eql(u8, suffix, "64w");
    if (std.mem.eql(u8, c.name, "fpu_state") and !w64 and r.p.port64_arch == .aarch64) return;
    if (endsWith64(c.name) or std.mem.startsWith(u8, c.name, "port64_")) return;
    var exe: ?[]const u8 = null;
    for (c.argv) |a| switch (a) {
        .bare => |n| {
            exe = n;
            break;
        },
        .target => |n| {
            exe = n;
            break;
        },
        else => {},
    };
    const e = exe orelse return;
    if (endsWith64(e) or std.mem.eql(u8, e, "p4pc") or std.mem.eql(u8, e, "p4pc_viewer")) return;
    if (w64 and std.mem.eql(u8, e, "ovdiff")) return;
    if (r.p.apple and r.wraps(e)) return;
    if (!r.isExecutable(e)) return;
    const twin = r.fmt("{s}{s}", .{ c.name, suffix });
    if (r.has(twin)) return;
    if (w64 and r.o.wine64 == null) return;
    var cmd: std.ArrayList(Arg) = .empty;
    if (w64) cmd.appendSlice(r.gpa, &.{ cmake, lit("-E"), lit("env"), lit("WINEDEBUG=-all"), lit(r.wineprefix()) }) catch @panic("OOM");
    for (c.argv) |a| {
        const same_exe = switch (a) {
            .bare, .target => |n| std.mem.eql(u8, n, e),
            else => false,
        };
        if (same_exe) {
            const exe_twin = r.fmt("{s}{s}", .{ e, suffix });
            if (w64) {
                cmd.appendSlice(r.gpa, &.{ .wine64, tf(exe_twin) }) catch @panic("OOM");
            } else if (a == .bare) {
                cmd.append(r.gpa, bare(exe_twin)) catch @panic("OOM");
            } else cmd.append(r.gpa, tf(exe_twin)) catch @panic("OOM");
        } else if (a == .target and r.isExecutable(a.target)) {
            const other = r.fmt("{s}{s}", .{ a.target, suffix });
            if (w64) cmd.append(r.gpa, .wine64) catch @panic("OOM");
            cmd.append(r.gpa, tf(other)) catch @panic("OOM");
        } else if (a == .bin and std.mem.startsWith(u8, a.bin, r.fmt("{s}_", .{suffix}))) {
            cmd.append(r.gpa, a) catch @panic("OOM");
        } else if (a == .bin and a.bin.len != 0 and std.mem.findScalar(u8, a.bin, '/') == null) {
            cmd.append(r.gpa, bin(r.fmt("{s}_{s}", .{ suffix, a.bin }))) catch @panic("OOM");
        } else cmd.append(r.gpa, a) catch @panic("OOM");
    }
    r.add(twin, cmd.items);
    const t = r.get(twin);
    if (w64) t.fixtures_required.append(r.gpa, "wine64_ready") catch @panic("OOM");
    var labels: std.ArrayList([]const u8) = .empty;
    const arch_label: []const u8 = if (w64) "port64win" else "port64";
    labels.append(r.gpa, arch_label) catch @panic("OOM");
    if (contains(c.labels, "gl")) labels.append(r.gpa, "gl") catch @panic("OOM");
    if (contains(c.labels, "slow") or contains(&.{ "ovdiff_dblcores", "movie_task_opening", "movie_task_p4ct001" }, c.name))
        labels.append(r.gpa, "slow") catch @panic("OOM");
    const work = r.fmt("tests/host{s}/{s}", .{ suffix, c.name });
    r.mkdirBin(work);
    r.setList(&t.labels, labels.items);
    t.cwd = work;
    if (c.skip) |v| {
        if (v != 0) t.skip = v;
    }
    if (c.timeout) |v| {
        if (v != 0) t.timeout = v;
    }
    if (c.cwd) |v| t.cwd = v;
    if (c.run_serial) t.run_serial = true;
    if (c.locks.len != 0) r.setList(&t.locks, c.locks);
    if (w64) {
        if (t.timeout) |v| {
            if (v != 0) t.timeout = v * r.o.wine_timeout_scale;
        }
    }
    var env64: std.ArrayList(Env) = .empty;
    for (c.env) |item| {
        if (!std.mem.eql(u8, item.key, "P4PC_SAVE_DIR")) env64.append(r.gpa, item) catch @panic("OOM");
    }
    const save = r.fmt("save/{s}{s}", .{ c.name, suffix });
    r.mkdirBin(save);
    env64.append(r.gpa, ebin("P4PC_SAVE_DIR", save)) catch @panic("OOM");
    r.setEnv(t, env64.items);
}

fn hostTwins(r: *Registry, suffix: []const u8) void {
    for (r.o.external_host_cases) |c| hostTwin(r, .{
        .name = c.name,
        .argv = c.argv,
        .env = c.env,
        .labels = c.labels,
        .timeout = c.timeout,
        .skip = c.skip,
        .cwd = c.cwd,
        .run_serial = c.run_serial,
        .locks = c.locks,
    }, suffix);
    // Snapshot: twins registered here are themselves P4_CMD_TESTS entries but end in 64/64w.
    const count = r.tests.items.len;
    var i: usize = 0;
    while (i < count) : (i += 1) {
        const t = r.tests.items[i];
        if (!t.in_cmd_tests) continue;
        hostTwin(r, .{
            .name = t.name,
            .argv = t.argv,
            .env = r.gpa.dupe(Env, t.env.items) catch @panic("OOM"),
            .labels = r.gpa.dupe([]const u8, t.labels.items) catch @panic("OOM"),
            .timeout = t.timeout,
            .skip = t.skip,
            .cwd = t.cwd,
            .run_serial = t.run_serial,
            .locks = r.gpa.dupe([]const u8, t.locks.items) catch @panic("OOM"),
        }, suffix);
    }
}

const witness_env = [_]Env{ env("P4PC_MODS", "1"), env("P4PC_PACKS", "0"), env("P4PC_INPUT", "script"), env("P4PC_MOD_WITNESS", "1") };

const Pair = struct { target: []const u8, suffix: []const u8 };
const all_pairs = [_]Pair{ .{ .target = "p4pc", .suffix = "" }, .{ .target = "p4pc64", .suffix = "64" }, .{ .target = "p4pc64w", .suffix = "64w" } };
const wide_pairs = [_]Pair{ .{ .target = "p4pc64", .suffix = "64" }, .{ .target = "p4pc64w", .suffix = "64w" } };

/// The driver words of a witness run: the target's CROSSCOMPILING_EMULATOR, or ${P4_MOD_WINE} for 64w.
fn witnessDriver(r: *Registry, pair: Pair) []const []const u8 {
    if (std.mem.eql(u8, pair.suffix, "64w")) return r.o.mod_wine;
    return r.emulator(pair.target);
}

fn wineEnv(r: *Registry) []const Env {
    return r.gpa.dupe(Env, &.{ env("WINEDEBUG", "-all"), ebin("WINEPREFIX", "tests/wine64prefix") }) catch @panic("OOM");
}

/// The p4pc|p4pc64|p4pc64w witness loop of 185..189 (`raw`: 189-golden-witness uses _add_test
/// for every pair instead of the wrapper behind `if(NOT TEST ...)`).
fn modWitness(r: *Registry, stem: []const u8, out: []const u8, timeout: u32, pairs: []const Pair, raw: bool) void {
    for (pairs) |pair| {
        if (!r.hasTarget(pair.target)) continue;
        const name = r.fmt("{s}{s}", .{ stem, pair.suffix });
        var cmd: std.ArrayList(Arg) = .empty;
        cmd.appendSlice(r.gpa, &.{ py, src(r.fmt("tests/{s}_test.py", .{stem})), lit("--out"), bin(r.fmt("{s}{s}", .{ out, pair.suffix })), lit("--driver") }) catch @panic("OOM");
        for (witnessDriver(r, pair)) |w| cmd.append(r.gpa, lit(w)) catch @panic("OOM");
        cmd.append(r.gpa, tf(pair.target)) catch @panic("OOM");
        if (raw) {
            _ = r.rawAdd(name, cmd.items);
        } else if (!r.has(name)) r.add(name, cmd.items);
        r.props(&.{name}, .{ .labels = &.{"mod-witness"}, .timeout = timeout, .skip = 77, .env = &witness_env });
        if (std.mem.eql(u8, pair.suffix, "64w")) r.appendEnv(name, wineEnv(r));
    }
}

fn frag185to189(r: *Registry) void {
    if (!r.p.mod_witness_tests) {
        frag188Package(r);
        return;
    }
    modWitness(r, "mod_enemy_game", "mod-enemy-witness", 5400, &all_pairs, false);
    modWitness(r, "mod_persona_game", "mod-persona-witness", 5400, &all_pairs, false);
    modWitness(r, "mod_element_game", "mod-element-witness", 5400, &all_pairs, false);
    frag188Package(r);
    modWitness(r, "mod_physsplit_game", "mod-physsplit-witness", 10800, &all_pairs, false);
    modWitness(r, "mod_qol_game", "mod-qol-witness", 5400, &all_pairs, false);
    modWitness(r, "mod_golden_game", "mod-golden-witness", 5400, &wide_pairs, true);
    modWitness(r, "mod_hardmode_game", "mod-hardmode-witness", 7200, &all_pairs, false);
    modWitness(r, "mod_item_game", "mod-item-witness", 5400, &all_pairs, false);
    modWitness(r, "mod_skill_game", "mod-skill-witness", 5400, &all_pairs, false);
    modWitness(r, "mod_overlay_game", "mod-overlay-witness", 2400, &all_pairs, false);
    // 189-ui-overlay-witness.cmake: a mod card in the real Velvet fusion list; 000-registration.cmake clones it.
    if (r.hasTarget("p4pc")) {
        r.add("native_rows_velvet", &.{ lit("sh"), src("tests/native_rows_velvet_gate.sh"), tf("p4pc") });
        r.props(&.{"native_rows_velvet"}, .{ .skip = 77, .labels = &.{"mod-witness"}, .timeout = 1500, .env = &witness_env });
    }
}

/// 188-physsplit-mod.cmake: the committed package is what tools/physsplit.py generates.
fn frag188Package(r: *Registry) void {
    r.add("physsplit_package", &.{ py, src("tools/physsplit.py"), lit("check") });
    r.props(&.{"physsplit_package"}, .{ .labels = &.{"host"}, .timeout = 120 });
}

/// 192-randomizer-mod.cmake.
fn frag192(r: *Registry) void {
    if (!(r.p.mod_witness_tests and r.hasTarget("luau_script_runner64"))) return;
    const emu = r.emulator("luau_script_runner64");
    if (!r.has("mod_randomizer_corpus")) {
        var cmd: std.ArrayList(Arg) = .empty;
        cmd.appendSlice(r.gpa, &.{ py, src("tests/mod_randomizer_engine_test.py"), lit("--corpus-only"), lit("--seeds"), lit("1000"), lit("--") }) catch @panic("OOM");
        for (emu) |w| cmd.append(r.gpa, lit(w)) catch @panic("OOM");
        cmd.append(r.gpa, tf("luau_script_runner64")) catch @panic("OOM");
        r.add("mod_randomizer_corpus", cmd.items);
        r.props(&.{"mod_randomizer_corpus"}, .{ .labels = &.{"mod-witness"}, .timeout = 3600, .skip = 77, .env = &witness_env });
    }
    for (all_pairs) |pair| {
        if (!r.hasTarget(pair.target)) continue;
        const driver = witnessDriver(r, pair);
        for ([_][]const u8{ "game", "dungeon" }) |kind| {
            const name = r.fmt("mod_randomizer_{s}{s}", .{ kind, pair.suffix });
            if (r.has(name)) continue;
            const out: []const u8 = if (std.mem.eql(u8, kind, "game")) "mod-randomizer-witness" else "mod-randomizer-dungeon";
            var cmd: std.ArrayList(Arg) = .empty;
            cmd.appendSlice(r.gpa, &.{ py, src(r.fmt("tests/mod_randomizer_{s}_test.py", .{kind})), lit("--out"), bin(r.fmt("{s}{s}", .{ out, pair.suffix })), lit("--runner") }) catch @panic("OOM");
            for (emu) |w| cmd.append(r.gpa, lit(w)) catch @panic("OOM");
            cmd.appendSlice(r.gpa, &.{ tf("luau_script_runner64"), lit("--driver") }) catch @panic("OOM");
            for (driver) |w| cmd.append(r.gpa, lit(w)) catch @panic("OOM");
            cmd.append(r.gpa, tf(pair.target)) catch @panic("OOM");
            r.add(name, cmd.items);
        }
        for ([_][]const u8{ "game", "dungeon" }) |kind| {
            const name = r.fmt("mod_randomizer_{s}{s}", .{ kind, pair.suffix });
            r.props(&.{name}, .{ .labels = &.{"mod-witness"}, .timeout = 7200, .skip = 77, .env = &witness_env });
            if (std.mem.eql(u8, pair.suffix, "64w")) r.appendEnv(name, wineEnv(r));
        }
    }
}

/// 210-port64-runtime.cmake.
fn frag210(r: *Registry) void {
    if (!r.p.win32 and r.p.linux_x86_64_host and r.p.port64_arch == .x86_64 and r.p.x64_probe) {
        r.add("crash_log64", &.{ py, src("tests/crash_log_test.py"), file("crash_log_test64") });
        r.props(&.{"crash_log64"}, .{ .labels = &.{"port64"}, .timeout = 90 });
        r.add("port64_coro", &.{file("port64_coro_test64")});
        r.props(&.{"port64_coro"}, .{ .skip = 77 });
        const configured_elf: ?[]const u8 = if (r.o.retail) |retail| retail.configured_elf else null;
        if (configured_elf) |elf| {
            r.add("port64_task", &.{ lit("sh"), src("tests/port64_task.sh"), tf("port64_task_test"), file("port64_task_test64"), lit(elf) });
            r.add("port64_task_retail", &.{ lit("sh"), src("tests/port64_task.sh"), file("port64_task_retail64"), file("port64_task_test64"), lit(elf) });
            r.props(&.{ "port64_task", "port64_task_retail" }, .{ .skip = 77 });
        }
    }
    if (r.hasTarget("p4decomp64w")) {
        if (r.o.wine64 != null) {
            r.add("port64_coro_win", &.{ cmake, lit("-E"), lit("env"), lit("WINEDEBUG=-all"), lit(r.wineprefix()), .wine64, tf("port64_coro_win_test") });
            r.props(&.{"port64_coro_win"}, .{ .timeout = 120 });
        }
    }
    if (r.hasTarget("p4pc64")) {
        r.add("boot64", &.{ lit("sh"), src("tests/boot64_gate.sh"), tf("p4pc64") });
        r.props(&.{"boot64"}, .{ .skip = 77, .timeout = 300 });
        r.add("scene_battle_floor_a64", &.{ lit("sh"), src("tests/battle_mad.sh"), tf("p4pc64"), lit("512"), lit("p4-battle-floor-a"), lit("1400"), lit("14.4"), lit("cross@360:6,cross@420:6,cross@480:6,cross@540:6,cross@600:6,cross@660:6,cross@720:6,cross@780:6,cross@840:6,cross@900:6,cross@960:6,cross@1020:6,cross@1080:6,cross@1140:6,cross@1200:6"), lit("P4PC_VIRTUAL_CLOCK=1") });
        r.props(&.{"scene_battle_floor_a64"}, .{ .skip = 77, .labels = &.{ "gl", "port64" }, .timeout = 300 });
    }
    if (r.p.port64_objs) {
        r.add("port64_mirror", &.{ py, src("tools/port64/unit64.py"), lit("report"), lit("--dir"), .{ .gen = .{ .sub = "units", .dir = true } }, lit("--baseline"), src("config/port64_unsupported.txt") });
        if (r.o.port64.census_baseline) |baseline| {
            r.add("port64_census", &.{ py, src("tools/port64/unit64.py"), lit("report"), lit("--dir"), .{ .gen = .{ .sub = "units", .dir = true } }, lit("--census-baseline"), src(baseline), lit("--clang"), lit(r.o.port64.c_compiler orelse std.debug.panic("runtime_gates: port64_census needs Options.port64.c_compiler", .{})), lit("--active-units"), .{ .gen = .{ .sub = "active_units.txt" } } });
        }
        r.add("seam_abi_mirror", &.{ py, src("tools/port64/seam_abi.py"), lit("--check"), lit("--mirror"), .{ .gen = .{ .sub = "abi.json" } } });
    }
}

/// 290-port64-host-cases.cmake.
fn frag290(r: *Registry) void {
    if (!(r.p.port64_link and r.p.port64_objs)) return;
    hostTwins(r, "64");
    if (r.hasTarget("p4pc64w")) {
        if (r.find("port64_coro_win")) |t| {
            t.labels.append(r.gpa, "port64win") catch @panic("OOM");
            r.setList(&t.fixtures_setup, &.{"wine64_ready"});
        }
        hostTwins(r, "64w");
        r.add("wine_timeouts64w", &.{ py, src("tests/wine_timeout_test.py"), bin(""), lit(r.fmt("{d}", .{r.o.wine_timeout_scale})), .ctest });
        r.props(&.{"wine_timeouts64w"}, .{ .labels = &.{ "host", "port64win" }, .timeout = 60 });
    }
}

/// Registers the runtime gates (see the file comment for the contract).
pub fn register(b: *std.Build, all: *std.Build.Step, options: Options) void {
    const o = b.allocator.create(Options) catch @panic("OOM");
    o.* = options;
    var r: Registry = .{ .b = b, .gpa = b.allocator, .o = o, .p = options.profile };
    // CTest runs a test without WORKING_DIRECTORY in the tests' build directory, which CMake creates.
    r.mkdirBin("tests");
    registerFragments(&r);
    emit(&r, all);
}

fn registerFragments(r: *Registry) void {
    frag090(r);
    frag100(r);
    frag110(r);
    frag111(r);
    frag112(r);
    frag120(r);
    frag130(r);
    frag140(r);
    frag150(r);
    frag160(r);
    frag170(r);
    frag180(r);
    frag185to189(r);
    frag190(r);
    frag191(r);
    frag192(r);
    policy200(r);
    frag210(r);
    if (r.p.port64_link and r.p.port64_objs) frag220(r);
    policy230(r);
    policy240(r);
    policy250(r);
    frag290(r);
    policyTail(r);
}

fn artifact(o: *const Options, name: []const u8) ?Artifact {
    for (o.artifacts) |a| if (std.mem.eql(u8, a.name, name)) return a;
    return null;
}

/// Why a gate cannot be made (an input the build did not supply), or null.
fn missingInput(r: *Registry, t: *const Test) ?[]const u8 {
    for (t.argv) |a| switch (a) {
        .target, .bare, .file => |n| if (artifact(r.o, n) == null)
            return r.fmt("runtime gate {s}: the build supplied no artifact '{s}'", .{ t.name, n }),
        .wine64 => if (r.o.wine64 == null)
            return r.fmt("runtime gate {s}: P4_WINE64 (Options.wine64) is unset", .{t.name}),
        .gen => if (r.o.port64.g64 == null)
            return r.fmt("runtime gate {s}: P4_G64 (Options.port64.g64) is unset", .{t.name}),
        else => {},
    };
    if (hardwareRenderer(t)) |renderer|
        return r.fmt("runtime gate {s} is judged on hardware renderer '{s}': only the CTest manifest and the local gate runner custody path may run it", .{ t.name, renderer });
    if (glCapable(t) and r.o.gl == .refuse)
        return r.fmt("runtime gate {s} can open a GL context and Options.gl is .refuse", .{t.name});
    return null;
}

/// The gate opens a GL context: label gl or mod-witness, or it runs the game or viewer of any ABI.
fn glCapable(t: *const Test) bool {
    if (hasLabel(t, "gl") or hasLabel(t, "mod-witness")) return true;
    for (t.argv) |a| switch (a) {
        .target, .bare => |n| for ([_][]const u8{ "p4pc", "p4pc64", "p4pc64w", "p4pc_viewer", "p4pc_viewer64", "p4pc_viewer64w" }) |game| {
            if (std.mem.eql(u8, n, game)) return true;
        },
        else => {},
    };
    return false;
}

/// A P4PC_GATE_RENDERER (argument or environment) that is not the llvmpipe software renderer.
fn hardwareRenderer(t: *const Test) ?[]const u8 {
    const key = "P4PC_GATE_RENDERER=";
    for (t.argv) |a| {
        if (a == .lit and std.mem.startsWith(u8, a.lit, key) and std.mem.find(u8, a.lit, "llvmpipe") == null) return a.lit[key.len..];
    }
    for (t.env.items) |e| {
        if (!std.mem.eql(u8, e.key, "P4PC_GATE_RENDERER")) continue;
        switch (e.value) {
            .lit => |v| if (std.mem.find(u8, v, "llvmpipe") == null) return v,
            else => return "(path)",
        }
    }
    return null;
}

fn binPath(r: *Registry, rel: []const u8) []const u8 {
    if (rel.len == 0) return r.o.binary_dir;
    return r.fmt("{s}/{s}", .{ r.o.binary_dir, rel });
}

fn addPath(run: *std.Build.Step.Run, lp: std.Build.LazyPath, prefix: []const u8, dir: bool) void {
    if (dir) {
        run.addDirectoryArg2(lp, .{ .prefix = prefix, .make_absolute = true });
    } else run.addFileArg2(lp, .{ .prefix = prefix, .make_absolute = true });
}

fn addCommandArg(r: *Registry, run: *std.Build.Step.Run, a: Arg) void {
    const b = r.b;
    switch (a) {
        .lit => |s| run.addArg(s),
        .python => run.addArg(r.o.python),
        .wine64 => run.addArg(r.o.wine64.?),
        .ctest => run.addArg(ctest_placeholder),
        .cmake => std.debug.panic("runtime_gates: ${{CMAKE_COMMAND}} is supported only as `cmake -E env` prefix", .{}),
        .src => |s| addPath(run, b.path(s.path), s.prefix, s.dir),
        .bin => |rel| run.addArg(binPath(r, rel)),
        .target, .bare, .file => |n| {
            const art = artifact(r.o, n).?;
            if (art.runtime_path) |path| {
                run.addArg(path);
                std.debug.assert(art.kind != .directory);
                run.addFileInput(art.path);
                if (art.ready) |ready| run.step.dependOn(ready);
            } else addPath(run, art.path, "", art.kind == .directory);
        },
        .gen => |g| addPath(run, r.o.port64.g64.?.path(b, g.sub), g.prefix, g.dir),
    }
}

/// The word wine_timeout_test.py runs as CTest; the runner substitutes a listing of this registry.
const ctest_placeholder = "@ZIG_RUNTIME_GATES_CTEST@";

fn selected(o: *const Options, t: *const Test) bool {
    if (contains(o.all.exclude_names, t.name)) return false;
    for (o.all.exclude_labels) |l| if (hasLabel(t, l)) return false;
    if (o.all.include_labels) |inc| {
        for (inc) |l| if (!hasLabel(t, l)) return false;
    }
    return true;
}

const LabelStep = struct { label: []const u8, step: *std.Build.Step };
const Made = struct { t: *Test, step: *std.Build.Step, run: ?*std.Build.Step.Run };

fn emit(r: *Registry, all: *std.Build.Step) void {
    const b = r.b;
    const o = r.o;
    const runner = b.path("tools/zig_runtime_gates.py");
    const wf = b.addWriteFiles();
    const manifest = wf.add("runtime_gates.json", manifestJson(r));
    var dir_text: std.ArrayList(u8) = .empty;
    for (r.dirs.items) |d| {
        dir_text.appendSlice(r.gpa, d) catch @panic("OOM");
        dir_text.append(r.gpa, '\n') catch @panic("OOM");
    }
    const dirs = wf.add("runtime_gates.dirs", dir_text.items);
    const install = b.addInstallFile(manifest, "gates/runtime_gates.json");
    const manifest_step = b.step(r.fmt("{s}-manifest", .{o.step_prefix}), "Install the runtime gate registry (gates/runtime_gates.json, CTest json-v1 shape)");
    manifest_step.dependOn(&install.step);

    // CMake makes these directories at configure time (save roots, host work directories).
    const prepare = b.addSystemCommand(&.{o.python});
    prepare.setName("runtime gates: configure directories");
    prepare.addFileArg2(runner, .{ .make_absolute = true });
    prepare.addArgs(&.{ "prepare", r.fmt("--binary-dir={s}", .{o.binary_dir}) });
    prepare.addFileArg2(dirs, .{ .prefix = "--dirs=", .make_absolute = true });
    prepare.has_side_effects = true;
    prepare.expectExitCode(0);

    var made: std.ArrayList(Made) = .empty;
    var labels: std.ArrayList(LabelStep) = .empty;
    for (r.tests.items) |t| {
        const top = b.step(r.fmt("{s}:{s}", .{ o.step_prefix, t.name }), r.fmt("Run runtime gate {s}", .{t.name}));
        var run_opt: ?*std.Build.Step.Run = null;
        if (missingInput(r, t)) |why| {
            top.dependOn(&b.addFail(why).step);
        } else {
            const run = makeRun(r, t, runner, manifest);
            run.step.dependOn(&prepare.step);
            top.dependOn(&run.step);
            run_opt = run;
        }
        made.append(r.gpa, .{ .t = t, .step = top, .run = run_opt }) catch @panic("OOM");
        for (t.labels.items) |l| {
            const ls = for (labels.items) |x| {
                if (std.mem.eql(u8, x.label, l)) break x.step;
            } else blk: {
                const s = b.step(r.fmt("{s}-label:{s}", .{ o.step_prefix, l }), r.fmt("Run the runtime gates labelled {s}", .{l}));
                labels.append(r.gpa, .{ .label = l, .step = s }) catch @panic("OOM");
                break :blk s;
            };
            ls.dependOn(top);
        }
        if (selected(o, t)) all.dependOn(top);
    }
    // FIXTURES_REQUIRED: the setup tests run first, and their failure stops the dependants.
    for (made.items) |m| {
        const run = m.run orelse continue;
        for (m.t.fixtures_required.items) |required| {
            for (made.items) |setup| {
                if (!contains(setup.t.fixtures_setup.items, required)) continue;
                run.step.dependOn(setup.step);
            }
        }
    }
    // Fixtures other modules require (fixture()): wired to their setup gates now, or failed with the reason.
    var it = fixture_requests.iterator();
    while (it.next()) |e| wireFixture(r, e.key_ptr.*, e.value_ptr.*, made.items);
    emitted = .{ .r = r, .made = made.items };
}

/// A CTest FIXTURES_REQUIRED of a test registered by another module (e.g. wine64_ready for mod_loader_recovery64w and the
/// native host twins): a step to make that test's Run depend on. It runs the FIXTURES_SETUP gate of this registry
/// (wine64_ready: port64_coro_win, 290-port64-host-cases.cmake), so a failed or unmade setup fails the dependant. Until
/// register() has wired it, and when register() has no setup gate for the fixture, the step fails with the reason: it is
/// never an empty pass. Works whether called before or after register().
pub fn fixture(b: *std.Build, step_prefix: []const u8, name: []const u8) *std.Build.Step {
    const step_name = b.fmt("{s}-fixture:{s}", .{ step_prefix, name });
    if (b.top_level_steps.get(step_name)) |existing| return &existing.step;
    const step = b.step(step_name, b.fmt("The {s} test fixture (its FIXTURES_SETUP runtime gate)", .{name}));
    step.dependOn(&b.addFail(b.fmt("test fixture {s}: runtime_gates.register has not wired its FIXTURES_SETUP gate", .{name})).step);
    if (emitted) |e| {
        wireFixture(e.r, name, step, e.made);
    } else {
        fixture_requests.put(b.allocator, b.allocator.dupe(u8, name) catch @panic("OOM"), step) catch @panic("OOM");
    }
    return step;
}

var fixture_requests: std.StringArrayHashMapUnmanaged(*std.Build.Step) = .empty;
var emitted: ?struct { r: *Registry, made: []const Made } = null;

fn wireFixture(r: *Registry, name: []const u8, step: *std.Build.Step, made: []const Made) void {
    var setups: usize = 0;
    for (made) |m| {
        if (contains(m.t.fixtures_setup.items, name)) setups += 1;
    }
    step.dependencies.clearRetainingCapacity();
    if (setups == 0) {
        step.dependOn(&r.b.addFail(r.fmt("test fixture {s}: this profile registers no FIXTURES_SETUP test for it (e.g. no Windows x64 link or no P4_WINE64)", .{name})).step);
        return;
    }
    for (made) |m| {
        if (contains(m.t.fixtures_setup.items, name)) step.dependOn(m.step);
    }
}

fn makeRun(r: *Registry, t: *const Test, runner: std.Build.LazyPath, manifest: std.Build.LazyPath) *std.Build.Step.Run {
    const b = r.b;
    const o = r.o;
    const run = b.addSystemCommand(&.{o.python});
    run.setName(r.fmt("runtime gate {s}", .{t.name}));
    run.addFileArg2(runner, .{ .make_absolute = true });
    run.addArgs(&.{ "run", r.fmt("--name={s}", .{t.name}) });
    run.addArg(r.fmt("--timeout={d}", .{t.timeout orelse o.default_timeout_s}));
    if (t.skip) |v| run.addArg(r.fmt("--skip-code={d}", .{v}));
    run.addArg(r.fmt("--cwd={s}", .{binPath(r, t.cwd orelse "tests")}));
    run.addArg(r.fmt("--lock-dir={s}", .{o.lock_dir}));
    for (t.locks.items) |l| run.addArg(r.fmt("--lock={s}", .{l}));
    if (t.run_serial) run.addArg("--run-serial");
    for (t.labels.items) |l| run.addArg(r.fmt("--label={s}", .{l}));
    // Reached only with Options.gl == .software for GL-capable gates (missingInput refuses otherwise).
    if (glCapable(t)) run.addArg("--software-gl");
    if (o.retail) |ret| {
        run.addArg(r.fmt("--retail-iso={s}", .{ret.iso}));
        run.addArg(r.fmt("--retail-elf={s}", .{ret.elf}));
        if (ret.ref_dir) |d| run.addArg(r.fmt("--ref-dir={s}", .{d}));
    }
    for (t.env.items) |e| switch (e.value) {
        .lit => |v| run.addArg(r.fmt("--env={s}={s}", .{ e.key, v })),
        .bin => |v| run.addArg(r.fmt("--env={s}={s}", .{ e.key, binPath(r, v) })),
        .src => |p| addPath(run, b.path(p), r.fmt("--env={s}=", .{e.key}), false),
        .src_dir => |p| addPath(run, b.path(p), r.fmt("--env={s}=", .{e.key}), true),
    };
    run.addArg(r.fmt("--result={s}/zig-gates/{s}.json", .{ o.binary_dir, t.name }));
    var argv = t.argv;
    if (argv[0] == .cmake) {
        if (argv.len < 4 or !isLit(argv[1], "-E") or !isLit(argv[2], "env"))
            std.debug.panic("runtime_gates: {s}: only `cmake -E env` commands are supported", .{t.name});
        argv = argv[3..];
        while (argv.len != 0 and argv[0] == .lit and isEnvWord(argv[0])) : (argv = argv[1..]) {
            run.addArg(r.fmt("--cmd-env={s}", .{argv[0].lit}));
        }
        if (argv.len == 0) std.debug.panic("runtime_gates: {s}: `cmake -E env` without a command", .{t.name});
    }
    for (argv) |a| {
        if (a == .ctest) run.addFileArg2(manifest, .{ .prefix = "--registry=", .make_absolute = true });
    }
    run.addArg("--");
    for (argv) |a| addCommandArg(r, run, a);
    run.has_side_effects = true;
    run.expectExitCode(0);
    return run;
}

fn isLit(a: Arg, s: []const u8) bool {
    return a == .lit and std.mem.eql(u8, a.lit, s);
}

// ---- the manifest: CTest's `--show-only=json-v1` shape, symbolic where CMake resolves a path ----

fn jsonString(r: *Registry, out: *std.ArrayList(u8), s: []const u8) void {
    out.append(r.gpa, '"') catch @panic("OOM");
    for (s) |c| switch (c) {
        '"' => out.appendSlice(r.gpa, "\\\"") catch @panic("OOM"),
        '\\' => out.appendSlice(r.gpa, "\\\\") catch @panic("OOM"),
        '\n' => out.appendSlice(r.gpa, "\\n") catch @panic("OOM"),
        '\r' => out.appendSlice(r.gpa, "\\r") catch @panic("OOM"),
        '\t' => out.appendSlice(r.gpa, "\\t") catch @panic("OOM"),
        0...8, 11, 12, 14...0x1f => out.appendSlice(r.gpa, r.fmt("\\u{x:0>4}", .{c})) catch @panic("OOM"),
        else => out.append(r.gpa, c) catch @panic("OOM"),
    };
    out.append(r.gpa, '"') catch @panic("OOM");
}

fn jsonList(r: *Registry, out: *std.ArrayList(u8), items: []const []const u8) void {
    out.append(r.gpa, '[') catch @panic("OOM");
    for (items, 0..) |s, i| {
        if (i != 0) out.append(r.gpa, ',') catch @panic("OOM");
        jsonString(r, out, s);
    }
    out.append(r.gpa, ']') catch @panic("OOM");
}

fn symbolic(r: *Registry, a: Arg) []const u8 {
    return switch (a) {
        .lit => |s| s,
        .python => r.o.python,
        .cmake => "${CMAKE_COMMAND}",
        .wine64 => r.o.wine64 orelse "${P4_WINE64}",
        .ctest => "${CMAKE_CTEST_COMMAND}",
        .src => |s| r.fmt("{s}${{CMAKE_SOURCE_DIR}}/{s}", .{ s.prefix, s.path }),
        .bin => |rel| binPath(r, rel),
        .target, .bare => |n| r.fmt("$<TARGET_FILE:{s}>", .{n}),
        .file => |n| r.fmt("$<ZIG_ARTIFACT:{s}>", .{n}),
        .gen => |g| r.fmt("{s}${{P4_G64}}/{s}", .{ g.prefix, g.sub }),
    };
}

fn envSymbolic(r: *Registry, e: Env) []const u8 {
    return switch (e.value) {
        .lit => |v| r.fmt("{s}={s}", .{ e.key, v }),
        .src, .src_dir => |v| r.fmt("{s}=${{CMAKE_SOURCE_DIR}}/{s}", .{ e.key, v }),
        .bin => |v| r.fmt("{s}={s}", .{ e.key, binPath(r, v) }),
    };
}

fn manifestJson(r: *Registry) []const u8 {
    const g = r.gpa;
    var out: std.ArrayList(u8) = .empty;
    out.appendSlice(g, "{\"kind\":\"ctestInfo\",\"version\":{\"major\":1,\"minor\":0},\"zigRuntimeGates\":{\"binaryDir\":") catch @panic("OOM");
    jsonString(r, &out, r.o.binary_dir);
    out.appendSlice(g, ",\"python\":") catch @panic("OOM");
    jsonString(r, &out, r.o.python);
    out.appendSlice(g, ",\"wine64\":") catch @panic("OOM");
    if (r.o.wine64) |w| jsonString(r, &out, w) else out.appendSlice(g, "null") catch @panic("OOM");
    out.appendSlice(g, r.fmt(",\"defaultTimeout\":{d}}},\"tests\":[", .{r.o.default_timeout_s})) catch @panic("OOM");
    for (r.tests.items, 0..) |t, i| {
        if (i != 0) out.append(g, ',') catch @panic("OOM");
        out.appendSlice(g, "{\"name\":") catch @panic("OOM");
        jsonString(r, &out, t.name);
        out.appendSlice(g, ",\"command\":") catch @panic("OOM");
        var words: std.ArrayList([]const u8) = .empty;
        for (t.argv) |a| words.append(g, symbolic(r, a)) catch @panic("OOM");
        jsonList(r, &out, words.items);
        out.appendSlice(g, ",\"properties\":[") catch @panic("OOM");
        var first = true;
        const sep = struct {
            fn put(rr: *Registry, o: *std.ArrayList(u8), f: *bool, name: []const u8) void {
                if (!f.*) o.append(rr.gpa, ',') catch @panic("OOM");
                f.* = false;
                o.appendSlice(rr.gpa, rr.fmt("{{\"name\":\"{s}\",\"value\":", .{name})) catch @panic("OOM");
            }
        };
        if (t.env.items.len != 0) {
            var items: std.ArrayList([]const u8) = .empty;
            for (t.env.items) |e| items.append(g, envSymbolic(r, e)) catch @panic("OOM");
            sep.put(r, &out, &first, "ENVIRONMENT");
            jsonList(r, &out, items.items);
            out.append(g, '}') catch @panic("OOM");
        }
        if (t.fixtures_required.items.len != 0) {
            sep.put(r, &out, &first, "FIXTURES_REQUIRED");
            jsonList(r, &out, t.fixtures_required.items);
            out.append(g, '}') catch @panic("OOM");
        }
        if (t.fixtures_setup.items.len != 0) {
            sep.put(r, &out, &first, "FIXTURES_SETUP");
            jsonList(r, &out, t.fixtures_setup.items);
            out.append(g, '}') catch @panic("OOM");
        }
        if (t.labels.items.len != 0) {
            sep.put(r, &out, &first, "LABELS");
            jsonList(r, &out, t.labels.items);
            out.append(g, '}') catch @panic("OOM");
        }
        if (t.locks.items.len != 0) {
            sep.put(r, &out, &first, "RESOURCE_LOCK");
            jsonList(r, &out, t.locks.items);
            out.append(g, '}') catch @panic("OOM");
        }
        if (t.run_serial) {
            sep.put(r, &out, &first, "RUN_SERIAL");
            out.appendSlice(g, "true}") catch @panic("OOM");
        }
        if (t.skip) |v| {
            sep.put(r, &out, &first, "SKIP_RETURN_CODE");
            out.appendSlice(g, r.fmt("{d}}}", .{v})) catch @panic("OOM");
        }
        if (t.timeout) |v| {
            sep.put(r, &out, &first, "TIMEOUT");
            out.appendSlice(g, r.fmt("{d}}}", .{v})) catch @panic("OOM");
        }
        if (t.cwd) |v| {
            sep.put(r, &out, &first, "WORKING_DIRECTORY");
            jsonString(r, &out, binPath(r, v));
            out.append(g, '}') catch @panic("OOM");
        }
        out.appendSlice(g, "]}") catch @panic("OOM");
    }
    out.appendSlice(g, "]}\n") catch @panic("OOM");
    return out.items;
}

// ---- fragment registrations, in tests/cmake order; mechanically transcribed from the fragments ----

/// 090-movie.cmake
fn frag090(r: *Registry) void {
    r.add("sfmpvp", &.{ bare("sfmpvp_test") });
    r.props(&.{ "sfmpvp" }, .{ .skip = 77 });
    r.add("mwply", &.{ bare("mwply_test") });
    r.props(&.{ "mwply" }, .{ .skip = 77 });
    r.add("movie_smoke", &.{ bare("movie_smoke"), bin("movie_smoke.png") });
    r.props(&.{ "movie_smoke" }, .{ .skip = 77, .labels = &.{ "gl" } });
    r.add("movie_task", &.{ bare("movie_task_smoke") });
    r.props(&.{ "movie_task" }, .{ .skip = 77, .labels = &.{ "gl" }, .cwd = "" });
    r.add("movie_task_opening", &.{ bare("movie_task_smoke") });
    r.add("movie_task_p4ct001", &.{ bare("movie_task_smoke") });
    r.props(&.{ "movie_task_opening" }, .{ .skip = 77, .labels = &.{ "gl" }, .timeout = 600, .cwd = "", .env = &.{ env("P4PC_MOVIE_INDEX", "21"), env("P4PC_MOVIE_SHOTS", "0") } });
    r.props(&.{ "movie_task_p4ct001" }, .{ .skip = 77, .labels = &.{ "gl" }, .timeout = 600, .cwd = "", .env = &.{ env("P4PC_MOVIE_INDEX", "1"), env("P4PC_MOVIE_SHOTS", "0") } });
}

/// 100-scenes.cmake (shuffle and widescreen loops unrolled)
fn frag100(r: *Registry) void {
    r.add("title_mad", &.{ lit("sh"), src("tests/title_mad.sh"), tf("p4pc_viewer"), bin("title_mad.png") });
    r.props(&.{ "title_mad" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 300 });
    r.add("title_load", &.{ lit("sh"), src("tests/title_load_gate.sh"), tf("p4pc") });
    r.props(&.{ "title_load" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("title_mad_gl33", &.{ cmake, lit("-E"), lit("env"), lit("P4PC_GL_API=gl"), lit("sh"), src("tests/title_mad.sh"), tf("p4pc_viewer"), bin("title_mad_gl33.png") });
    r.props(&.{ "title_mad_gl33" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 300 });
    r.add("title_mad_ctxloss", &.{ cmake, lit("-E"), lit("env"), lit("P4PC_TEST_CONTEXT_LOSS=200"), lit("sh"), src("tests/title_mad.sh"), tf("p4pc_viewer"), bin("title_mad_ctxloss.png") });
    r.props(&.{ "title_mad_ctxloss" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 300 });
    r.add("field_mad_ctxloss", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("field_mad_ctxloss.png"), lit("field:7:2"), lit("field"), lit("1.6"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_TEST_CONTEXT_LOSS=300") });
    r.props(&.{ "field_mad_ctxloss" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("field_mad", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("field_mad.png"), lit("field:7:2"), lit("field"), lit("1.6"), lit("P4PC_DRAW_WINDOW=4") });
    r.props(&.{ "field_mad" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("battle_menu", &.{ lit("sh"), src("tests/battle_gate.sh"), tf("p4pc"), lit("20"), lit("420") });
    r.props(&.{ "battle_menu" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 300 });
    r.add("shuffle_gate_0_first", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("0:first") });
    r.props(&.{ "shuffle_gate_0_first" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_0_0", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("0:0") });
    r.props(&.{ "shuffle_gate_0_0" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_0_1", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("0:1") });
    r.props(&.{ "shuffle_gate_0_1" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_0_2", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("0:2") });
    r.props(&.{ "shuffle_gate_0_2" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_1_first", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("1:first") });
    r.props(&.{ "shuffle_gate_1_first" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_1_0", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("1:0") });
    r.props(&.{ "shuffle_gate_1_0" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_1_1", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("1:1") });
    r.props(&.{ "shuffle_gate_1_1" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_1_2", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("1:2") });
    r.props(&.{ "shuffle_gate_1_2" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_2_first", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("2:first") });
    r.props(&.{ "shuffle_gate_2_first" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_2_0", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("2:0") });
    r.props(&.{ "shuffle_gate_2_0" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_2_1", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("2:1") });
    r.props(&.{ "shuffle_gate_2_1" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_2_2", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("2:2") });
    r.props(&.{ "shuffle_gate_2_2" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_3_first", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("3:first") });
    r.props(&.{ "shuffle_gate_3_first" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_3_0", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("3:0") });
    r.props(&.{ "shuffle_gate_3_0" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_3_1", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("3:1") });
    r.props(&.{ "shuffle_gate_3_1" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_3_2", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("3:2") });
    r.props(&.{ "shuffle_gate_3_2" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_3_3", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("3:3") });
    r.props(&.{ "shuffle_gate_3_3" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_4_first", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("4:first") });
    r.props(&.{ "shuffle_gate_4_first" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_4_0", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("4:0") });
    r.props(&.{ "shuffle_gate_4_0" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_4_1", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("4:1") });
    r.props(&.{ "shuffle_gate_4_1" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("shuffle_gate_4_2", &.{ lit("sh"), src("tests/shuffle_gate.sh"), tf("p4pc"), lit("4:2") });
    r.props(&.{ "shuffle_gate_4_2" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("scene_battle_floor_a", &.{ lit("sh"), src("tests/battle_mad.sh"), tf("p4pc"), lit("512"), lit("p4-battle-floor-a"), lit("1400"), lit("14.4"), lit("cross@360:6,cross@420:6,cross@480:6,cross@540:6,cross@600:6,cross@660:6,cross@720:6,cross@780:6,cross@840:6,cross@900:6,cross@960:6,cross@1020:6,cross@1080:6,cross@1140:6,cross@1200:6"), lit("P4PC_VIRTUAL_CLOCK=1") });
    r.props(&.{ "scene_battle_floor_a" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 300 });
    r.add("scene_fog_battle", &.{ lit("sh"), src("tests/battle_mad.sh"), tf("p4pc"), lit("532"), lit("p4-fogbattle"), lit("1700"), lit("3.9"), lit("cross@360:2/150,start@500:2/300"), lit("P4PC_VIRTUAL_CLOCK=1") });
    r.props(&.{ "scene_fog_battle" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 300 });
    r.add("camp_menu", &.{ lit("sh"), src("tests/menu_gate.sh"), tf("p4pc") });
    r.props(&.{ "camp_menu" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("camp_menu_write_failure", &.{ lit("sh"), src("tests/menu_gate.sh"), tf("p4pc"), lit("--expect-write-failure") });
    r.props(&.{ "camp_menu_write_failure" }, .{ .skip = 77, .labels = &.{ "gl" }, .timeout = 600 });
    r.add("save_load", &.{ lit("sh"), src("tests/save_gate.sh"), tf("p4pc") });
    r.props(&.{ "save_load" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("retail_save_load", &.{ lit("sh"), src("tests/retail_save_gate.sh"), tf("p4pc") });
    r.props(&.{ "retail_save_load" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("fields_load", &.{ lit("sh"), src("tests/fields_gate.sh"), tf("p4pc") });
    r.props(&.{ "fields_load" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1500 });
    r.add("systems_velvet_fusion", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("fusion") });
    r.props(&.{ "systems_velvet_fusion" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("systems_velvet_compendium", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("compendium") });
    r.props(&.{ "systems_velvet_compendium" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("settings_host_velvet", &.{ lit("sh"), src("tests/settings_host_velvet_gate.sh"), tf("p4pc"), bin("settings_host_velvet") });
    r.props(&.{ "settings_host_velvet" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1500 });
    r.add("systems_club_floor_menu", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("club") });
    r.props(&.{ "systems_club_floor_menu" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("systems_minimap", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("minimap"), lit("0.7") });
    r.props(&.{ "systems_minimap" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200, .env = &.{ env("P4PC_BOARD_SEED", "0x17de") } });
    r.add("systems_game_1f", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("game1f") });
    r.props(&.{ "systems_game_1f" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200, .env = &.{ env("P4PC_BOARD_SEED", "0x16bc") } });
    r.add("systems_heaven_1f", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("heaven1f") });
    r.props(&.{ "systems_heaven_1f" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200, .env = &.{ env("P4PC_BOARD_SEED", "0x1699") } });
    r.add("systems_shop_daidara", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("daidara") });
    r.props(&.{ "systems_shop_daidara" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("systems_shop_shiroku", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("shiroku") });
    r.props(&.{ "systems_shop_shiroku" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("scene_tv_entrance", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("entrance"), lit("3.8") });
    r.props(&.{ "scene_tv_entrance" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("scene_marukyu_seats", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc"), lit("seats"), lit("3.0") });
    r.props(&.{ "scene_marukyu_seats" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("scene_near_fog", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc"), lit("near-fog"), lit("4600"), lit("4.15"), lit("card"), lit("save0411_to_near_fog_scene.walk"), lit("0,0,640,200"), lit("3.35") });
    r.props(&.{ "scene_near_fog" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_EVENT_REF", "p4-near-fog-corpus") } });
    r.add("scene_velvet_windows_blurry", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc"), lit("velvet-windows-blurry"), lit("3630"), lit("1.05"), lit("story0415"), lit("save0415_to_river_scene.walk") });
    r.props(&.{ "scene_velvet_windows_blurry" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("scene_riverbank_vignette", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc"), lit("riverbank-vignette"), lit("5880"), lit("1.30"), lit("story0415"), lit("save0415_to_river_scene.walk") });
    r.props(&.{ "scene_riverbank_vignette" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("scene_rain", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc"), lit("rain"), lit("25100"), lit("3.0"), lit("newgame"), lit("newgame_to_rain_scene.walk") });
    r.props(&.{ "scene_rain" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("scene_arcana_rankup_sparkles", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc"), lit("arcana-rankup-sparkles"), lit("7300"), lit("3.65"), lit("story0415"), lit("save0415_to_street.walk") });
    r.props(&.{ "scene_arcana_rankup_sparkles" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("scene_tv_world_enter", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc"), lit("tv-world-enter"), lit("71778"), lit("1.65"), lit("newgame"), lit("newgame_to_0413.walk") });
    r.props(&.{ "scene_tv_world_enter" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1700 });
    r.add("scene_shadow_haze", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc"), lit("shadow-haze"), lit("103408"), lit("1.65"), lit("newgame"), lit("newgame_to_0413.walk") });
    r.props(&.{ "scene_shadow_haze" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1700 });
    r.add("pad_field_run", &.{ lit("sh"), src("tests/pad_field_gate.sh"), tf("p4pc") });
    r.props(&.{ "pad_field_run" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1500 });
    r.add("rng_boot", &.{ lit("sh"), src("tests/rng_gate.sh"), tf("p4pc"), lit("boot") });
    r.props(&.{ "rng_boot" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("rng_floor", &.{ lit("sh"), src("tests/rng_gate.sh"), tf("p4pc"), lit("floor") });
    r.props(&.{ "rng_floor" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1800 });
    r.add("widescreen_field_16_9", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("widescreen_field_16_9.png"), lit("field:7:2"), lit("port-wide-field-16-9"), lit("0.1"), lit("P4PC_ASPECT=16:9"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)") });
    r.props(&.{ "widescreen_field_16_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 600 });
    r.add("widescreen_battle_16_9", &.{ lit("sh"), src("tests/battle_mad.sh"), tf("p4pc"), lit("532"), lit("port-wide-battle-16-9"), lit("0"), lit("0.1"), lit("cross@360:2/150,start@500:2/300"), lit("P4PC_ASPECT=16:9"), lit("P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)"), lit("P4PC_VIRTUAL_CLOCK=1"), lit("P4PC_CAPTURE_STATE=battle-menu,40,1,4000") });
    r.props(&.{ "widescreen_battle_16_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 600 });
    r.add("widescreen_dialogue_16_9", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc"), lit("velvet-windows-blurry"), lit("3630"), lit("0.1"), lit("story0415"), lit("save0415_to_river_scene.walk") });
    r.props(&.{ "widescreen_dialogue_16_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 900, .env = &.{ env("P4PC_ASPECT", "16:9"), env("P4PC_EVENT_REF", "port-wide-dialogue-16-9"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)") } });
    r.add("scene_resolution_title1080", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("resolution_title1080.png"), lit("title"), lit("p4-resolution-title1080"), lit("0.5"), lit("P4PC_RESOLUTION=1920x1080"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_DRAW_WINDOW=4") });
    r.props(&.{ "scene_resolution_title1080" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("scene_resolution_title1366_cells", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("resolution_title1366_cells.png"), lit("title"), lit("p4-resolution-title1366-cells"), lit("0.5"), lit("P4PC_RESOLUTION=1366x768"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_WINDOW_W=1277"), lit("P4PC_WINDOW_H=954"), lit("P4PC_CAPTURE_WINDOW=1") });
    r.add("scene_resolution_title1277_cells", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("resolution_title1277_cells.png"), lit("title"), lit("p4-resolution-title1277-cells"), lit("0.5"), lit("P4PC_RESOLUTION=1277x954"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_WINDOW_W=1277"), lit("P4PC_WINDOW_H=954"), lit("P4PC_CAPTURE_WINDOW=1") });
    r.props(&.{ "scene_resolution_title1366_cells", "scene_resolution_title1277_cells" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_TITLE_EDGES", "1") } });
    r.add("scene_resolution_field1366", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("resolution_field1366.png"), lit("field:7:2"), lit("p4-resolution-field1366"), lit("0.5"), lit("P4PC_RESOLUTION=1366x768"), lit("P4PC_DRAW_WINDOW=4") });
    r.props(&.{ "scene_resolution_field1366" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("scene_resolution_butterfly1440", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("resolution_butterfly1440.png"), lit("field:8:2"), lit("p4-resolution-butterfly1440"), lit("0.5"), lit("P4PC_RESOLUTION=2560x1440"), lit("P4PC_DRAW_WINDOW=30"), lit("P4PC_FIELD_PLAYER=311.5,0,-2216.1"), lit("P4PC_FIELD_DATE=16"), lit("P4PC_FIELD_SLOT=3"), lit("P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656") });
    r.props(&.{ "scene_resolution_butterfly1440" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("scene_resolution_fog4k", &.{ lit("sh"), src("tests/battle_mad.sh"), tf("p4pc"), lit("532"), lit("p4-resolution-fog4k"), lit("1700"), lit("0.5"), lit("cross@360:2/150,start@500:2/300"), lit("P4PC_RESOLUTION=3840x2160"), lit("P4PC_VIRTUAL_CLOCK=1") });
    r.props(&.{ "scene_resolution_fog4k" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 300, .env = &.{ env("P4PC_FOG_SEAM", "1") } });
    r.add("widescreen_title_cover_16_9", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("widescreen_title_cover_16_9.png"), lit("title"), lit("port-wide-title-cover-16-9-llvmpipe19"), lit("0.1"), lit("P4PC_CONFIG=none"), lit("P4PC_ASPECT=16:9"), lit("P4PC_WINDOW_W=1280"), lit("P4PC_WINDOW_H=720"), lit("P4PC_RESOLUTION=scale:1"), lit("P4PC_PRESENT=blit"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_VIRTUAL_CLOCK=1"), lit("P4PC_CAPTURE_WINDOW=1"), lit("P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)") });
    r.props(&.{ "widescreen_title_cover_16_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 600, .env = &.{ env("P4PC_TITLE_EDGES", "1") } });
    r.add("widescreen_allout_masked_16_9", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc"), lit("allout"), lit("16:9"), lit("port-wide-allout-masked-16-9-llvmpipe19") });
    r.props(&.{ "widescreen_allout_masked_16_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)") } });
    r.add("widescreen_shuffle_full_16_9", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc"), lit("shuffle"), lit("16:9"), lit("port-wide-shuffle-full-vu0-16-9-llvmpipe19") });
    r.props(&.{ "widescreen_shuffle_full_16_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)") } });
    r.add("widescreen_allout_intro_16_9", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc"), lit("allout-intro"), lit("16:9"), lit("port-wide-allout-intro-16-9-llvmpipe19") });
    r.props(&.{ "widescreen_allout_intro_16_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)") } });
    r.add("widescreen_cast_eyes_16_9", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc"), lit("cast-eyes"), lit("16:9"), lit("port-wide-cast-eyes-16-9-llvmpipe19") });
    r.props(&.{ "widescreen_cast_eyes_16_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)") } });
    r.add("widescreen_title_cover_21_9", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("widescreen_title_cover_21_9.png"), lit("title"), lit("port-wide-title-cover-21-9-llvmpipe19"), lit("0.1"), lit("P4PC_CONFIG=none"), lit("P4PC_ASPECT=auto"), lit("P4PC_WINDOW_W=1680"), lit("P4PC_WINDOW_H=720"), lit("P4PC_RESOLUTION=scale:1"), lit("P4PC_PRESENT=blit"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_VIRTUAL_CLOCK=1"), lit("P4PC_CAPTURE_WINDOW=1"), lit("P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)") });
    r.props(&.{ "widescreen_title_cover_21_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 600, .env = &.{ env("P4PC_TITLE_EDGES", "1") } });
    r.add("widescreen_allout_masked_21_9", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc"), lit("allout"), lit("21:9"), lit("port-wide-allout-masked-21-9-llvmpipe19") });
    r.props(&.{ "widescreen_allout_masked_21_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)") } });
    r.add("widescreen_shuffle_full_21_9", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc"), lit("shuffle"), lit("21:9"), lit("port-wide-shuffle-full-vu0-21-9-llvmpipe19") });
    r.props(&.{ "widescreen_shuffle_full_21_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)") } });
    r.add("widescreen_allout_intro_21_9", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc"), lit("allout-intro"), lit("21:9"), lit("port-wide-allout-intro-21-9-llvmpipe19") });
    r.props(&.{ "widescreen_allout_intro_21_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)") } });
    r.add("widescreen_cast_eyes_21_9", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc"), lit("cast-eyes"), lit("21:9"), lit("port-wide-cast-eyes-21-9-llvmpipe19") });
    r.props(&.{ "widescreen_cast_eyes_21_9" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)") } });
}

/// 110-tools-save.cmake
fn frag110(r: *Registry) void {
    if (!r.p.win32) {
        r.add("ps2save_psu", &.{ py, src("tools/ps2save.py"), lit("selftest") });
        r.add("seam_abi", &.{ py, src("tools/port64/seam_abi.py"), lit("--check") });
    }
}

/// 111-import-max.cmake
fn frag111(r: *Registry) void {
    r.add("import_max", &.{ bare("import_max_test") });
}

/// 112-import-save.cmake
fn frag112(r: *Registry) void {
    r.add("import_cbs", &.{ bare("import_cbs_test") });
    if (!r.p.win32) {
        r.add("save_import_formats", &.{ py, src("tests/save_import_fixtures.py"), lit("--driver"), tf("save_import_test") });
        r.props(&.{ "save_import_formats" }, .{ .timeout = 600 });
    }
    r.add("save_import_load", &.{ lit("sh"), src("tests/save_import_load_gate.sh"), tf("p4pc") });
    r.props(&.{ "save_import_load" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("import_settings_native", &.{ lit("sh"), src("tests/import_settings_native_gate.sh"), tf("p4pc") });
    r.props(&.{ "import_settings_native" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
}

/// 120-story-early.cmake
fn frag120(r: *Registry) void {
    r.add("newgame_to_field", &.{ lit("sh"), src("tests/newgame_gate.sh"), tf("p4pc"), lit("5.0"), lit("30500") });
    r.props(&.{ "newgame_to_field" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("story_to_0415", &.{ lit("sh"), src("tests/story_gate.sh"), tf("p4pc"), lit("140000"), lit("7:2,4/15") });
    r.props(&.{ "story_to_0415" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1800 });
    r.add("street_0417", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("60000"), lit("8:2,4/17"), lit("story0415"), lit("save0415_to_street.walk") });
    r.props(&.{ "street_0417" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("castle_gate_0417", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("60000"), lit("23:1,4/17"), lit("story0415"), lit("save0415_to_castle.walk") });
    r.props(&.{ "castle_gate_0417" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("castle_2f_0417", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("90000"), lit("23:3,4/17"), lit("story0415"), lit("save0415_to_castle_2f.walk") });
    r.props(&.{ "castle_2f_0417" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1500 });
    r.add("home_0417", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("110000"), lit("7:2,4/17"), lit("story0415"), lit("save0415_to_home_0417.walk") });
    r.props(&.{ "home_0417" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1800 });
    r.add("save_play", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("14000"), lit("68:1") });
    r.props(&.{ "save_play" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("walk_digest", &.{ lit("sh"), src("tests/walk_digest_gate.sh"), tf("p4pc") });
    r.props(&.{ "walk_digest" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
}

/// 130-scenes-corpus.cmake (p4_scene_gate expanded)
fn frag130(r: *Registry) void {
    r.add("scene_sky_black_tint", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("scene_sky-black-tint.png"), lit("field:10:1"), lit("p4-sky-black-tint"), lit("2.8"), lit("P4PC_FIELD_DATE=17"), lit("P4PC_FIELD_SLOT=4"), lit("P4PC_FIELD_CAMERA=0.34202,0.0,-0.939693,0.0,0.0,1.0,0.0,0.0,0.939693,0.0,0.34202,0.0,491.663574,48.057514,-7614.540039,-131819.28125,0.511802,0.383852,50.000393,200000.0") });
    r.props(&.{ "scene_sky_black_tint" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    if (r.hasTarget("p4pc_viewer64")) {
        r.add("scene_sky_black_tint64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_scene_sky-black-tint.png"), lit("field:10:1"), lit("p4-sky-black-tint"), lit("2.8"), lit("P4PC_FIELD_DATE=17"), lit("P4PC_FIELD_SLOT=4"), lit("P4PC_FIELD_CAMERA=0.34202,0.0,-0.939693,0.0,0.0,1.0,0.0,0.0,0.939693,0.0,0.34202,0.0,491.663574,48.057514,-7614.540039,-131819.28125,0.511802,0.383852,50.000393,200000.0") });
        r.props(&.{ "scene_sky_black_tint64" }, .{ .skip = 77, .labels = &.{ "gl", "port64" }, .timeout = 600 });
        if (r.p.port64_arch == .aarch64) {
            r.appendLabels("scene_sky_black_tint64", &.{"slow"});
        }
    }
    r.add("scene_save_butterfly_glow", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer"), bin("scene_save-butterfly-glow.png"), lit("field:8:2"), lit("p4-save-butterfly-glow"), lit("3.4"), lit("P4PC_DRAW_WINDOW=30"), lit("P4PC_FIELD_PLAYER=311.5,0,-2216.1"), lit("P4PC_FIELD_DATE=16"), lit("P4PC_FIELD_SLOT=3"), lit("P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656") });
    r.props(&.{ "scene_save_butterfly_glow" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    if (r.hasTarget("p4pc_viewer64")) {
        r.add("scene_save_butterfly_glow64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_scene_save-butterfly-glow.png"), lit("field:8:2"), lit("p4-save-butterfly-glow"), lit("3.4"), lit("P4PC_DRAW_WINDOW=30"), lit("P4PC_FIELD_PLAYER=311.5,0,-2216.1"), lit("P4PC_FIELD_DATE=16"), lit("P4PC_FIELD_SLOT=3"), lit("P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656") });
        r.props(&.{ "scene_save_butterfly_glow64" }, .{ .skip = 77, .labels = &.{ "gl", "port64" }, .timeout = 600 });
    }
}

/// 140-movie-audio.cmake
fn frag140(r: *Registry) void {
    r.add("ffmpeg_license", &.{ bare("ffmpeg_license_test") });
    if (r.p.release) {
        r.props(&.{ "ffmpeg_license" }, .{ .env = &.{ env("P4PC_REQUIRE_LGPL", "1") } });
    }
    r.add("movie_audio", &.{ bare("movie_audio_test") });
}

/// 150-cri-audio.cmake
fn frag150(r: *Registry) void {
    r.add("cri_cpu_adapters", &.{ bare("cri_cpu_adapter_test") });
    r.props(&.{ "cri_cpu_adapters" }, .{ .skip = 77 });
    r.add("cri_sfd_callbacks", &.{ bare("cri_sfd_callback_test") });
    r.props(&.{ "cri_sfd_callbacks" }, .{ .skip = 77 });
    r.add("cri_rofs_metadata", &.{ bare("cri_rofs_metadata_test") });
    r.props(&.{ "cri_rofs_metadata" }, .{ .skip = 77 });
    r.add("axrna", &.{ bare("axrna_test") });
    r.props(&.{ "axrna" }, .{ .skip = 77 });
    r.add("adxt_playend", &.{ bare("adxt_playend_test") });
    r.props(&.{ "adxt_playend" }, .{ .skip = 77 });
    r.add("movie_start", &.{ bare("movie_start_test") });
    r.props(&.{ "movie_start" }, .{ .skip = 77 });
    r.add("cri_adxi_rpc", &.{ bare("cri_adxi_rpc_test") });
    r.props(&.{ "cri_adxi_rpc" }, .{ .skip = 77 });
    r.add("mixer_device_dummy", &.{ bare("mixer_device_test"), lit("dummy") });
    r.add("mixer_device_pacer", &.{ bare("mixer_device_test"), lit("none") });
    r.add("mixer_virtual_silent", &.{ bare("mixer_device_test"), lit("dummy"), lit("virtual") });
    r.add("mixer_virtual_live", &.{ bare("mixer_device_test"), lit("dummy"), lit("live") });
    r.add("mixer_virtual_live_no_device", &.{ bare("mixer_device_test"), lit("none"), lit("live") });
    r.add("mixer_zero_pitch", &.{ bare("mixer_device_test"), lit("dummy"), lit("zero-pitch") });
    r.add("mixer_diag", &.{ bare("mixer_device_test"), lit("dummy"), lit("diag") });
    r.props(&.{ "mixer_virtual_silent", "mixer_virtual_live", "mixer_virtual_live_no_device" }, .{ .timeout = 90 });
    r.add("sebank", &.{ bare("sebank_test") });
    r.props(&.{ "sebank" }, .{ .skip = 77 });
    r.add("cri_acs", &.{ bare("cri_acs_test") });
    r.add("adx_wav", &.{ bare("adx_wav_test"), bin("adx_wav_test.wav") });
    r.props(&.{ "adx_wav" }, .{ .skip = 77 });
    r.add("ahx_wav", &.{ bare("ahx_wav_test"), bin("ahx_wav_test.wav") });
    r.props(&.{ "ahx_wav" }, .{ .skip = 77 });
    r.add("aixp_wav", &.{ bare("aixp_wav_test"), bin("aixp_wav_test.wav") });
    r.props(&.{ "aixp_wav" }, .{ .skip = 77 });
}

/// 160-tools-ovdiff.cmake
fn frag160(r: *Registry) void {
    if (!r.p.win32) {
        r.add("ovdiff_subset", &.{ bare("ovdiff"), lit("-n"), lit("40"), lit("env"), srcp("@", "tests/ovdiff_subset.txt") });
        r.add("ovdiff_dblcores", &.{ bare("ovdiff"), lit("-n"), lit("100000"), lit("-Q"), lit("env"), lit("all") });
        r.props(&.{ "ovdiff_dblcores" }, .{ .skip = 77, .timeout = 600, .cwd = "", .env = &.{ esrcdir("P4_MIPSDIFF_DIR", "tools/mipsdiff") } });
        r.props(&.{ "ovdiff_subset" }, .{ .skip = 77, .timeout = 600, .cwd = "", .env = &.{ esrcdir("P4_MIPSDIFF_DIR", "tools/mipsdiff") } });
    }
}

/// 170-cri-state.cmake
fn frag170(r: *Registry) void {
    r.add("cri_acssnd", &.{ bare("cri_acssnd_test") });
    r.add("cri_acs_step", &.{ bare("cri_acs_step_test") });
    r.add("cri_adxm", &.{ bare("cri_adxm_test") });
    r.props(&.{ "cri_adxm" }, .{ .skip = 77 });
    r.add("cri_init", &.{ bare("cri_init_test") });
    r.props(&.{ "cri_init" }, .{ .skip = 77 });
    r.add("cri_stm", &.{ bare("cri_stm_test") });
    r.props(&.{ "cri_stm" }, .{ .skip = 77 });
    r.add("cri_cvfs", &.{ bare("cri_cvfs_test") });
    r.props(&.{ "cri_cvfs" }, .{ .skip = 77 });
    r.add("cri_adxt_start", &.{ bare("cri_adxt_start_test") });
    r.props(&.{ "cri_adxt_start" }, .{ .skip = 77 });
    r.add("rofs_open", &.{ bare("rofs_open_test"), src("tests/data/rofs_retail_opens.txt") });
    r.props(&.{ "rofs_open" }, .{ .skip = 77 });
    r.add("cri_file_size", &.{ bare("cri_file_size_test") });
    r.props(&.{ "cri_file_size" }, .{ .skip = 77 });
    r.add("adx_stream_event", &.{ bare("adx_wav_test"), bin("adx_stream_event.wav") });
    r.props(&.{ "adx_stream_event" }, .{ .skip = 77, .env = &.{ env("P4PC_ADX", "DATA/SOUND/EVENT/EVE_521.ADX"), env("P4PC_ADX_STREAM", "VOL:EVE_521.ADX"), env("P4PC_ADX_DIR", "SOUND/EVENT") } });
    r.add("adx_stream_voice", &.{ bare("adx_wav_test"), bin("adx_stream_voice.wav") });
    r.props(&.{ "adx_stream_voice" }, .{ .skip = 77, .env = &.{ env("P4PC_ADX", "DATA/SOUND/V176001.AFS"), env("P4PC_ADX_STREAM", "VOL:V176001.AFS"), env("P4PC_ADX_AFS_MEMBER", "1") } });
    r.add("adx_stream_env", &.{ bare("adx_wav_test"), bin("adx_stream_env.wav") });
    r.props(&.{ "adx_stream_env" }, .{ .skip = 77, .env = &.{ env("P4PC_ADX", "ENV/ENV_346.ADX"), env("P4PC_ADX_STREAM", "ENV:ENV_346.ADX") } });
}

/// 180-story.cmake
fn frag180(r: *Registry) void {
    r.add("voiced_event_0417", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("25000"), lit("6:6,4/18"), lit("story0417"), lit("repro_0417_night.walk") });
    r.props(&.{ "voiced_event_0417" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("school_0418", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,4/18"), lit("story0417"), lit("save0417_to_school_0418.walk") });
    r.props(&.{ "school_0418" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0418", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("10000"), lit("20:1,4/18"), lit("story0418"), lit("school_to_tv.walk") });
    r.props(&.{ "tv_0418" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_3f_0418", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("25000"), lit("20:1,4/18"), lit("story0418tv"), lit("save0418tv_to_castle_3f.walk") });
    r.props(&.{ "castle_3f_0418" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0419", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("12000"), lit("6:6,4/19"), lit("story0418_3f"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0419" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0419", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,4/19"), lit("story0419"), lit("save0419_to_tv_0419.walk") });
    r.props(&.{ "tv_0419" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_4f_0419", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("26000"), lit("20:1,4/19"), lit("story0419tv"), lit("save0419tv_to_castle_4f.walk") });
    r.props(&.{ "castle_4f_0419" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0420", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("12000"), lit("6:6,4/20"), lit("story0419_4f"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0420" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0420", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("10000"), lit("20:1,4/20"), lit("story0420"), lit("school_to_tv.walk") });
    r.props(&.{ "tv_0420" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_5f_0420", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("23000"), lit("20:1,4/20"), lit("story0420tv"), lit("save0420tv_to_castle_5f.walk") });
    r.props(&.{ "castle_5f_0420" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_training_0420", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("38000"), lit("20:1,4/20"), lit("story0420_5f"), lit("save0420_5f_to_training.walk") });
    r.props(&.{ "castle_training_0420" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0421", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("13000"), lit("6:6,4/21"), lit("story0420trained"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0421" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0421_armored", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("35000"), lit("20:1,4/21"), lit("story0421"), lit("save0421_to_tv_0421.walk") });
    r.props(&.{ "tv_0421_armored" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_training_0421", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("50000"), lit("20:1,4/21"), lit("story0421tv"), lit("save0421tv_to_training.walk") });
    r.props(&.{ "castle_training_0421" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0422", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,4/22"), lit("story0421trained"), lit("save0421training_to_school_0422.walk") });
    r.props(&.{ "school_0422" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0422", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("11000"), lit("20:1,4/22"), lit("story0422"), lit("school_to_tv.walk") });
    r.props(&.{ "tv_0422" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("orobas_0422", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("16000"), lit("20:1,4/22"), lit("story0422tv"), lit("save0422tv_to_orobas.walk") });
    r.props(&.{ "orobas_0422" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_personas_0422", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("33000"), lit("20:1,4/22"), lit("story0422orobas"), lit("save0422orobas_to_personas.walk") });
    r.props(&.{ "castle_personas_0422" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("omoikane_0422", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("24000"), lit("20:1,4/22"), lit("story0422personas"), lit("save0422personas_to_omoikane.walk") });
    r.props(&.{ "omoikane_0422" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tactics_0422", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("26000"), lit("20:1,4/22"), lit("story0422omoikane"), lit("save0422omoikane_to_tactics.walk") });
    r.props(&.{ "tactics_0422" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0423", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("15000"), lit("6:6,4/23"), lit("story0422tactics"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0423" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0423_supplied", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("28000"), lit("20:1,4/23"), lit("story0423"), lit("save0423_to_tv_0423.walk") });
    r.props(&.{ "tv_0423_supplied" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("avenger_knight_0423", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("42000"), lit("20:1,4/23"), lit("story0423tv"), lit("save0423tv_to_knight.walk") });
    r.props(&.{ "avenger_knight_0423" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0424", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,4/24"), lit("story0423knight"), lit("save0423knight_to_tv_0424.walk") });
    r.props(&.{ "tv_0424" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_6f_0424", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,4/24"), lit("story0424tv"), lit("save0424tv_to_castle_6f.walk") });
    r.props(&.{ "castle_6f_0424" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("forneus_0424", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,4/24"), lit("story0424_6f"), lit("save0424_6f_to_forneus.walk") });
    r.props(&.{ "forneus_0424" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_7f_0424", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("20:1,4/24"), lit("story0424forneus"), lit("save0424forneus_to_castle_7f.walk") });
    r.props(&.{ "castle_7f_0424" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_8f_0424", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("32000"), lit("20:1,4/24"), lit("story0424_7f"), lit("save0424_7f_to_castle_8f.walk") });
    r.props(&.{ "castle_8f_0424" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("senri_0424", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("20000"), lit("20:1,4/24"), lit("story0424_8f"), lit("save0424_8f_to_senri.walk") });
    r.props(&.{ "senri_0424" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("dekaja_0424", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,4/24"), lit("story0424senri"), lit("save0424senri_to_dekaja.walk") });
    r.props(&.{ "dekaja_0424" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_training_0424", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("35000"), lit("20:1,4/24"), lit("story0424dekaja"), lit("save0424dekaja_to_training.walk") });
    r.props(&.{ "castle_training_0424" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0425", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("22000"), lit("6:6,4/25"), lit("story0424trained"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0425" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0425", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("12000"), lit("20:1,4/25"), lit("story0425"), lit("school_to_tv.walk") });
    r.props(&.{ "tv_0425" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_training1_0425", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("23000"), lit("23:2,4/25"), lit("story0425tv"), lit("entrance_to_senri_training.walk") });
    r.props(&.{ "castle_training1_0425" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_training2_0425", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("23:2,4/25"), lit("story0425trained1"), lit("save0425trained1_to_training2.walk") });
    r.props(&.{ "castle_training2_0425" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0426", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("24000"), lit("6:6,4/26"), lit("story0425trained2"), lit("save0425trained2_to_school0426.walk") });
    r.props(&.{ "school_0426" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0426", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("12000"), lit("20:1,4/26"), lit("story0426"), lit("school_to_tv.walk") });
    r.props(&.{ "tv_0426" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_training3_0426", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("23:2,4/26"), lit("story0426tv"), lit("entrance_to_senri_training.walk") });
    r.props(&.{ "castle_training3_0426" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_training4_0426", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("45000"), lit("23:2,4/26"), lit("story0426trained3"), lit("save8f_to_senri_training.walk") });
    r.props(&.{ "castle_training4_0426" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("castle_training5_0426", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("45000"), lit("23:2,4/26"), lit("story0426trained4"), lit("save8f_to_senri_training.walk") });
    r.props(&.{ "castle_training5_0426" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("yukiko_rescue_0426", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("55000"), lit("6:6,4/27"), lit("story0426trained5"), lit("save0426trained5_to_yukiko_rescue.walk") });
    r.props(&.{ "yukiko_rescue_0426" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0428", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("20000"), lit("6:6,4/28"), lit("story0427rescued"), lit("school_sleep_to_school.walk") });
    r.props(&.{ "school_0428" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("holiday_0429", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("25000"), lit("7:2,4/29"), lit("story0428"), lit("school_sleep_to_holiday.walk") });
    r.props(&.{ "holiday_0429" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("yukiko_returns_0430", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("45000"), lit("7:2,4/30"), lit("story0429"), lit("home0429_to_yukiko_returns0430.walk") });
    r.props(&.{ "yukiko_returns_0430" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("holiday_0501", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("7:2,5/1"), lit("story0430"), lit("night_sleep_to_holiday.walk") });
    r.props(&.{ "holiday_0501" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0502", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("24000"), lit("6:6,5/2"), lit("story0501"), lit("home_day_sleep_to_school.walk") });
    r.props(&.{ "school_0502" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("holiday_0503", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("26000"), lit("7:2,5/3"), lit("story0502"), lit("school0502_to_holiday0503.walk") });
    r.props(&.{ "holiday_0503" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("holiday_0504", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("24000"), lit("7:2,5/4"), lit("story0503"), lit("night_sleep_to_holiday.walk") });
    r.props(&.{ "holiday_0504" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0504", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("25000"), lit("20:1,5/4"), lit("story0504"), lit("home_day_to_tv.walk") });
    r.props(&.{ "tv_0504" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("fox_0505", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("28000"), lit("7:2,5/5"), lit("story0504tv"), lit("tv0504_to_fox0505.walk") });
    r.props(&.{ "fox_0505" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0506", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("24000"), lit("6:6,5/6"), lit("story0505"), lit("night_sleep_to_school.walk") });
    r.props(&.{ "school_0506" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0507", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("22000"), lit("6:6,5/7"), lit("story0506"), lit("school_sleep_to_school.walk") });
    r.props(&.{ "school_0507" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("sunday_0508", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("24000"), lit("7:2,5/8"), lit("story0507"), lit("school0507_to_sunday0508.walk") });
    r.props(&.{ "sunday_0508" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("midterms_0512", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("24000"), lit("6:6,5/12"), lit("story0508"), lit("night_sleep_to_school.walk") });
    r.props(&.{ "midterms_0512" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0513", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("26000"), lit("6:6,5/13"), lit("story0512"), lit("school_sleep_to_school.walk") });
    r.props(&.{ "school_0513" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("weekend_0515", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("28000"), lit("7:2,5/15"), lit("story0513"), lit("school0513_to_weekend0515.walk") });
    r.props(&.{ "weekend_0515" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("story_0516", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("28000"), lit("7:2,5/16"), lit("story0515"), lit("night0515_to_story0516.walk") });
    r.props(&.{ "story_0516" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("priestess_0517", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("28000"), lit("7:2,5/17"), lit("story0516"), lit("night0516_to_priestess0517.walk") });
    r.props(&.{ "priestess_0517" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("kanji_clue_0518", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,5/18"), lit("story0517"), lit("night0517_to_kanji_clue0518.walk") });
    r.props(&.{ "kanji_clue_0518" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0519", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("28000"), lit("6:6,5/19"), lit("story0518clue"), lit("night_sleep_to_school.walk") });
    r.props(&.{ "school_0519" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("kanji_tv_0519", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("24000"), lit("20:1,5/19"), lit("story0519"), lit("school0519_to_kanji_tv.walk") });
    r.props(&.{ "kanji_tv_0519" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_discovered_0519", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("16000"), lit("20:1,5/19"), lit("story0519tv"), lit("save0519tv_to_bathhouse.walk") });
    r.props(&.{ "bathhouse_discovered_0519" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("16000"), lit("6:6,5/20"), lit("story0519bathhouse"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0520_supplied", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("26000"), lit("20:1,5/20"), lit("story0520"), lit("save0520_to_tv_0520.walk") });
    r.props(&.{ "tv_0520_supplied" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_1f_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("24000"), lit("20:1,5/20"), lit("story0520tv"), lit("save0520tv_to_bathhouse_1f.walk") });
    r.props(&.{ "bathhouse_1f_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_2f_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("32000"), lit("20:1,5/20"), lit("story0520bathhouse1f"), lit("save0520bathhouse1f_to_bathhouse_2f.walk") });
    r.props(&.{ "bathhouse_2f_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_3f_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("32000"), lit("20:1,5/20"), lit("story0520bathhouse2f"), lit("save0520bathhouse2f_to_bathhouse_3f.walk") });
    r.props(&.{ "bathhouse_3f_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("wind_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("22000"), lit("20:1,5/20"), lit("story0520bathhouse3f"), lit("save0520bathhouse3f_to_wind.walk") });
    r.props(&.{ "wind_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("kanji_glimpse_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("16000"), lit("20:1,5/20"), lit("story0520wind"), lit("save0520wind_to_kanji_glimpse.walk") });
    r.props(&.{ "kanji_glimpse_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_training_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("26000"), lit("20:1,5/20"), lit("story0520glimpse"), lit("save0520glimpse_to_training.walk") });
    r.props(&.{ "bathhouse_training_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0521", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("16000"), lit("6:6,5/21"), lit("story0520trained"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0521" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("supplied_tv_0521", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("32000"), lit("20:1,5/21"), lit("story0521"), lit("save0521_to_tv_0521.walk") });
    r.props(&.{ "supplied_tv_0521" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_training_0521", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("22000"), lit("20:1,5/21"), lit("story0521tv"), lit("save0521tv_to_gigas_training.walk") });
    r.props(&.{ "bathhouse_training_0521" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("recovered_tv_0521", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("14000"), lit("20:1,5/21"), lit("story0521trained"), lit("save0521trained_to_ready.walk") });
    r.props(&.{ "recovered_tv_0521" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_training2_0521", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("22000"), lit("20:1,5/21"), lit("story0521ready"), lit("save0521ready_to_training2.walk") });
    r.props(&.{ "bathhouse_training2_0521" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("sunday_0522", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("22000"), lit("7:2,5/22"), lit("story0521trained2"), lit("save0521trained2_to_sunday0522.walk") });
    r.props(&.{ "sunday_0522" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0522", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("22000"), lit("20:1,5/22"), lit("story0522"), lit("save0522_to_tv_0522.walk") });
    r.props(&.{ "tv_0522" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("guard_party_0522", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("22000"), lit("20:1,5/22"), lit("story0522tv"), lit("save0522tv_to_direct.walk") });
    r.props(&.{ "guard_party_0522" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_4f_0522", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("48000"), lit("20:1,5/22"), lit("story0522direct"), lit("save0522direct_to_bathhouse_4f.walk") });
    r.props(&.{ "bathhouse_4f_0522" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0523", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,5/23"), lit("story0522bathhouse4f"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0523" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("supplied_tv_0523", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("42000"), lit("20:1,5/23"), lit("story0523"), lit("save0523_to_tv_0523.walk") });
    r.props(&.{ "supplied_tv_0523" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_5f_0523", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("60000"), lit("20:1,5/23"), lit("story0523tv"), lit("save0523tv_to_bathhouse_5f.walk") });
    r.props(&.{ "bathhouse_5f_0523" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0524", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,5/24"), lit("story0523bathhouse5f"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0524" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0524", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,5/24"), lit("story0524"), lit("save0524_to_tv_0524.walk") });
    r.props(&.{ "tv_0524" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_6f_0524", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("65000"), lit("20:1,5/24"), lit("story0524tv"), lit("save0524tv_to_bathhouse_6f.walk") });
    r.props(&.{ "bathhouse_6f_0524" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0525", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,5/25"), lit("story0524bathhouse6f"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0525" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("supplied_tv_0525", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("42000"), lit("20:1,5/25"), lit("story0525"), lit("save0525_to_tv_0525.walk") });
    r.props(&.{ "supplied_tv_0525" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_7f_0525", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("60000"), lit("20:1,5/25"), lit("story0525tv"), lit("save0525tv_to_bathhouse_7f.walk") });
    r.props(&.{ "bathhouse_7f_0525" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0526", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,5/26"), lit("story0525bathhouse7f"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0526" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0526", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,5/26"), lit("story0526"), lit("save0526_to_tv_0526.walk") });
    r.props(&.{ "tv_0526" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("obariyon_0526", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("26000"), lit("20:1,5/26"), lit("story0526tv"), lit("save0526tv_to_obariyon.walk") });
    r.props(&.{ "obariyon_0526" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_guardian_0526", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("75000"), lit("20:1,5/26"), lit("story0526obariyon"), lit("save0526obariyon_to_bathhouse_8f.walk") });
    r.props(&.{ "bathhouse_guardian_0526" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0527", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,5/27"), lit("story0526bathhouse8f"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0527" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0527", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,5/27"), lit("story0527"), lit("save0527_to_tv_0527.walk") });
    r.props(&.{ "tv_0527" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0528", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,5/28"), lit("story0527tv"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0528" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("supplied_tv_0528", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("36000"), lit("20:1,5/28"), lit("story0528"), lit("save0528_to_tv_0528.walk") });
    r.props(&.{ "supplied_tv_0528" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("sunday_0529", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("22000"), lit("7:2,5/29"), lit("story0528tv"), lit("save0528tv_to_sunday0529.walk") });
    r.props(&.{ "sunday_0529" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0529", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,5/29"), lit("story0529"), lit("save0529_to_tv_0529.walk") });
    r.props(&.{ "tv_0529" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0530", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,5/30"), lit("story0529tv"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0530" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("supplied_tv_0530", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("36000"), lit("20:1,5/30"), lit("story0530"), lit("save0530_to_tv_0530.walk") });
    r.props(&.{ "supplied_tv_0530" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_9f_0530", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("65000"), lit("20:1,5/30"), lit("story0530tv"), lit("save0530tv_to_bathhouse_9f.walk") });
    r.props(&.{ "bathhouse_9f_0530" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_10f_0530", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("65000"), lit("20:1,5/30"), lit("story0530bathhouse9f"), lit("save0530bathhouse9f_to_bathhouse_10f.walk") });
    r.props(&.{ "bathhouse_10f_0530" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0531", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,5/31"), lit("story0530bathhouse10f"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0531" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0531", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,5/31"), lit("story0531"), lit("save0531_to_tv_0531.walk") });
    r.props(&.{ "tv_0531" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0601", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,6/1"), lit("story0531tv"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0601" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("supplied_tv_0601", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("36000"), lit("20:1,6/1"), lit("story0601"), lit("save0601_to_tv_0601.walk") });
    r.props(&.{ "supplied_tv_0601" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("trained_tv_0601", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("36000"), lit("20:1,6/1"), lit("story0601tv"), lit("save0601tv_to_training.walk") });
    r.props(&.{ "trained_tv_0601" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0602", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,6/2"), lit("story0601trained"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0602" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("supplied_tv_0602", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("36000"), lit("20:1,6/2"), lit("story0602"), lit("save0602_to_tv_0602.walk") });
    r.props(&.{ "supplied_tv_0602" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("bathhouse_11f_0602", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("65000"), lit("20:1,6/2"), lit("story0602tv"), lit("save0602tv_to_bathhouse_11f.walk") });
    r.props(&.{ "bathhouse_11f_0602" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0603", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("6:6,6/3"), lit("story0602bathhouse11f"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0603" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("tv_0603", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("18000"), lit("20:1,6/3"), lit("story0603"), lit("save0603_to_tv_0603.walk") });
    r.props(&.{ "tv_0603" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("trained_tv_0603", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("120000"), lit("20:1,6/3"), lit("story0603tv"), lit("save0603tv_to_training.walk") });
    r.props(&.{ "trained_tv_0603" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("school_0604", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("20000"), lit("6:6,6/4"), lit("story0603trained"), lit("entrance_sleep_to_school.walk") });
    r.props(&.{ "school_0604" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("supplied_tv_0604", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("36000"), lit("20:1,6/4"), lit("story0604"), lit("save0604_to_tv_0604.walk") });
    r.props(&.{ "supplied_tv_0604" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("healer_tv_0604", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("48000"), lit("20:1,6/4"), lit("story0604"), lit("save0604_to_healer_tv_0604.walk") });
    r.props(&.{ "healer_tv_0604" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_kanji_andras_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("90000"), lit("20:1,5/20"), lit("playthrough0520kanji"), lit("collection0520_to_andras_support.walk") });
    r.props(&.{ "collection_kanji_andras_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_kanji_support111_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("90000"), lit("20:1,5/20"), lit("playthrough0520andra"), lit("collection0520andras_to_support111.walk") });
    r.props(&.{ "collection_kanji_support111_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_kanji_eligor_0520", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("35000"), lit("20:1,5/20"), lit("playthrough0520phoenix"), lit("collection0520support111_to_eligor.walk") });
    r.props(&.{ "collection_kanji_eligor_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_kanji_supplies_0521", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("50000"), lit("20:1,5/21"), lit("playthrough0520eligor"), lit("collection0520eligor_to_funded_tv0521.walk") });
    r.props(&.{ "collection_kanji_supplies_0521" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_kanji_round_0520", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("8000"), lit("241:2,5/20"), lit("playthrough0520bathhouse11"), lit("collection0520_bathhouse11_to_kanji_round.walk") });
    r.props(&.{ "collection_kanji_round_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_kanji_victory_0520", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("34000"), lit("24:2,5/20"), lit("playthrough0520bathhouse11"), lit("collection0520_bathhouse11_to_kanji_victory.walk") });
    r.props(&.{ "collection_kanji_victory_0520" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_kanji_aftermath_0604", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("90000"), lit("7:2,6/4"), lit("playthrough0520bathhouse11"), lit("collection0520_bathhouse11_to_kanji_aftermath0604.walk") });
    r.props(&.{ "collection_kanji_aftermath_0604" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_sunday_0605", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,6/5"), lit("playthrough0604postkanji"), lit("night_sleep_to_holiday.walk") });
    r.props(&.{ "collection_sunday_0605" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_kanji_reunion_0606", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,6/6"), lit("playthrough0605postkanji"), lit("collection0605_to_kanji_reunion0606.walk") });
    r.props(&.{ "collection_kanji_reunion_0606" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_school_0607", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("6:6,6/7"), lit("playthrough0606postkanji"), lit("night_sleep_to_school.walk") });
    r.props(&.{ "collection_school_0607" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_camping_0620", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("50000"), lit("6:6,6/20"), lit("playthrough0615evening"), lit("collection0615_to_school0620.walk") });
    r.props(&.{ "collection_camping_0620" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_0621", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,6/21"), lit("playthrough0619home"), lit("collection0619_to_home0621.walk") });
    r.props(&.{ "collection_evening_0621" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_0623", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("45000"), lit("7:2,6/23"), lit("playthrough0621evening"), lit("collection0621_to_home0623.walk") });
    r.props(&.{ "collection_evening_0623" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_junes_0624", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,6/24"), lit("playthrough0623evening"), lit("collection0623_to_home0624.walk") });
    r.props(&.{ "collection_junes_0624" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_rise_search_0625", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("32000"), lit("20:1,6/25"), lit("playthrough0623evening"), lit("collection0623_to_rise_tv0625.walk") });
    r.props(&.{ "collection_rise_search_0625" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_marukyu_entry_0625", &.{ lit("sh"), src("tests/dungeon_digest_gate.sh"), tf("p4pc"), lit("30000"), lit("62:2,6/25"), lit("playthrough0625tv"), lit("collection0625_tv_to_marukyu3f.walk") });
    r.props(&.{ "collection_marukyu_entry_0625" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_TARGET", "windows") } });
    r.add("collection_marukyu_entry_0625_linux", &.{ lit("sh"), src("tests/dungeon_digest_gate.sh"), tf("p4pc"), lit("30000"), lit("62:2,6/25"), lit("playthrough0625tv"), lit("collection0625_tv_to_marukyu3f.walk") });
    r.props(&.{ "collection_marukyu_entry_0625_linux" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_TARGET", "linux") } });
    r.add("marukyu_allocator", &.{ lit("sh"), src("tests/marukyu_allocator_gate.sh"), tf("p4pc") });
    r.props(&.{ "marukyu_allocator" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("collection_teddie_victory_0625", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("82000"), lit("25:2,6/25"), lit("playthrough0625marukyu11f"), lit("collection0625_marukyu11f_to_teddie_victory.walk") });
    r.props(&.{ "collection_teddie_victory_0625" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("collection_teddie_aftermath_0626", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("92000"), lit("7:2,6/26"), lit("playthrough0625marukyu11f"), lit("collection0625_marukyu11f_to_teddie_aftermath0626.walk") });
    r.props(&.{ "collection_teddie_aftermath_0626" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_school_story_0627", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,6/27"), lit("playthrough0626postmarukyu"), lit("collection0626_to_school0627.walk") });
    r.props(&.{ "collection_school_story_0627" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_0708", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("80000"), lit("7:2,7/8"), lit("playthrough0626postmarukyu"), lit("collection0626_to_home0708.walk") });
    r.props(&.{ "collection_evening_0708" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_school_0709", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("6:6,7/9"), lit("playthrough0708evening"), lit("collection0708_to_school0709.walk") });
    r.props(&.{ "collection_school_0709" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_0718", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("90000"), lit("7:2,7/18"), lit("playthrough0709evening"), lit("collection0709_to_home0718.walk") });
    r.props(&.{ "collection_evening_0718" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_final_exams_0724", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,7/24"), lit("playthrough0718exameve"), lit("collection0718_to_exams0719.walk") });
    r.props(&.{ "collection_final_exams_0724" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_0801", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("40000"), lit("7:2,8/1"), lit("playthrough0726evening"), lit("collection0726_to_home0801.walk") });
    r.props(&.{ "collection_evening_0801" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_0803", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,8/3"), lit("playthrough0801home"), lit("collection0801_to_home0803.walk") });
    r.props(&.{ "collection_evening_0803" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_voidquest_entry_0803", &.{ lit("sh"), src("tests/dungeon_digest_gate.sh"), tf("p4pc"), lit("12000"), lit("43:0,8/3"), lit("playthrough0803tv"), lit("collection0803_tv_to_voidquest2f.walk") });
    r.props(&.{ "collection_voidquest_entry_0803" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_TARGET", "windows") } });
    r.add("collection_voidquest_entry_0803_linux", &.{ lit("sh"), src("tests/dungeon_digest_gate.sh"), tf("p4pc"), lit("12000"), lit("43:0,8/3"), lit("playthrough0803tv"), lit("collection0803_tv_to_voidquest2f.walk") });
    r.props(&.{ "collection_voidquest_entry_0803_linux" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_TARGET", "linux") } });
    r.add("collection_voidquest_opening_0803", &.{ lit("sh"), src("tests/voidquest_opening_gate.sh"), tf("p4pc") });
    r.props(&.{ "collection_voidquest_opening_0803" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_mitsuo_victory_0803", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("70000"), lit("26:2,8/3"), lit("playthrough0803voidquest"), lit("collection0803_voidquest_to_mitsuo_victory.walk") });
    r.props(&.{ "collection_mitsuo_victory_0803" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("collection_mitsuo_aftermath_0812", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("114000"), lit("7:2,8/12"), lit("playthrough0803voidquest"), lit("collection0803_voidquest_to_mitsuo_aftermath0812.walk") });
    r.props(&.{ "collection_mitsuo_aftermath_0812" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1800, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_summer_break_0814", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,8/14"), lit("playthrough0812summer"), lit("night_sleep_to_holiday.walk") });
    r.props(&.{ "collection_summer_break_0814" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_summer_festival_0822", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("40000"), lit("7:2,8/22"), lit("playthrough0814festival"), lit("collection0814_to_festival0822.walk") });
    r.props(&.{ "collection_summer_festival_0822" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_0907", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("60000"), lit("7:2,9/7"), lit("playthrough0830evening"), lit("collection0830_to_home0907.walk") });
    r.props(&.{ "collection_evening_0907" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_school_trip_0911", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("45000"), lit("7:2,9/11"), lit("playthrough0907trip"), lit("collection0907_to_schooltrip0911.walk") });
    r.props(&.{ "collection_school_trip_0911" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_0915", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("40000"), lit("7:2,9/15"), lit("playthrough0911home"), lit("collection0911_to_home0915.walk") });
    r.props(&.{ "collection_evening_0915" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_0918", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,9/18"), lit("playthrough0915evening"), lit("collection0915_to_home0918.walk") });
    r.props(&.{ "collection_evening_0918" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_secretlab_entrance_0918", &.{ lit("sh"), src("tests/secretlab_entrance_gate.sh"), tf("p4pc") });
    r.props(&.{ "collection_secretlab_entrance_0918" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_shadow_naoto_opening_0918", &.{ lit("sh"), src("tests/naoto_opening_gate.sh"), tf("p4pc") });
    r.props(&.{ "collection_shadow_naoto_opening_0918" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_naoto_victory_0918", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("70000"), lit("27:2,9/18"), lit("playthrough0918lab9f"), lit("collection0918_lab9f_to_naoto_victory.walk") });
    r.props(&.{ "collection_naoto_victory_0918" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 900 });
    r.add("collection_naoto_aftermath_1005", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("132000"), lit("7:2,10/5"), lit("playthrough0918lab9f"), lit("collection0918_lab9f_to_naoto_aftermath1005.walk") });
    r.props(&.{ "collection_naoto_aftermath_1005" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1800, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_return_1006", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,10/6"), lit("playthrough1005evening"), lit("collection1005_to_return1006.walk") });
    r.props(&.{ "collection_return_1006" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_1010", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,10/10"), lit("playthrough1006evening"), lit("collection1006_to_home1010.walk") });
    r.props(&.{ "collection_evening_1010" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_midterms_1020", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("50000"), lit("6:6,10/20"), lit("playthrough1010midterms"), lit("collection1010_to_midterms1020.walk") });
    r.props(&.{ "collection_midterms_1020" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_school_1021", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("6:6,10/21"), lit("playthrough1020school"), lit("collection1020_to_school1021.walk") });
    r.props(&.{ "collection_school_1021" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_1026", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("50000"), lit("7:2,10/26"), lit("playthrough1021school"), lit("collection1021_to_home1026.walk") });
    r.props(&.{ "collection_evening_1026" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_1028", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,10/28"), lit("playthrough1026evening"), lit("collection1026_to_home1028.walk") });
    r.props(&.{ "collection_evening_1028" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_culture_festival_1029", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,10/29"), lit("playthrough1028festival"), lit("collection1028_to_festival1029.walk") });
    r.props(&.{ "collection_culture_festival_1029" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_culture_festival_1101", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("50000"), lit("6:6,11/1"), lit("playthrough1029festival"), lit("collection1029_to_festival1101.walk") });
    r.props(&.{ "collection_culture_festival_1101" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_school_1104", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("70000"), lit("6:6,11/4"), lit("playthrough1029festival"), lit("collection1029_to_school1104.walk") });
    r.props(&.{ "collection_school_1104" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_heaven_entrance_1106", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("60000"), lit("28:1,11/6"), lit("playthrough1104school"), lit("collection1104_to_heaven1106.walk") });
    r.props(&.{ "collection_heaven_entrance_1106" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_kunino_sagiri_opening_1106", &.{ lit("sh"), src("tests/kunino_sagiri_opening_gate.sh"), tf("p4pc") });
    r.props(&.{ "collection_kunino_sagiri_opening_1106" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_kunino_sagiri_victory_1106", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("92000"), lit("28:2,11/6"), lit("playthrough1106heaven10f"), lit("collection1106_heaven10f_to_kunino_sagiri_victory.walk") });
    r.props(&.{ "collection_kunino_sagiri_victory_1106" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1200 });
    r.add("collection_kunino_sagiri_aftermath_1120", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("140000"), lit("7:2,11/20"), lit("playthrough1106heaven10f"), lit("collection1106_heaven10f_to_kunino_sagiri_aftermath1120.walk") });
    r.props(&.{ "collection_kunino_sagiri_aftermath_1120" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 1800, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_return_1121", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("30000"), lit("7:2,11/21"), lit("playthrough1120evening"), lit("collection1120_to_return1121.walk") });
    r.props(&.{ "collection_return_1121" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_1202", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("60000"), lit("7:2,12/2"), lit("playthrough1120evening"), lit("collection1120_to_home1202.walk") });
    r.props(&.{ "collection_evening_1202" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_hospital_bad_1203", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("45000"), lit("-1:-1,4/10"), lit("playthrough1202max18657"), lit("collection1202_max18657_to_hospital_bad.walk") });
    r.props(&.{ "collection_hospital_bad_1203" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_hospital_continue_1203", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("60000"), lit("7:2,12/3"), lit("playthrough1202max18657"), lit("collection1202_max18657_to_hospital_continue.walk") });
    r.props(&.{ "collection_hospital_continue_1203" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_deduction_1204", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("45000"), lit("7:2,12/4"), lit("playthrough1203evening"), lit("collection1203_to_home1204.walk") });
    r.props(&.{ "collection_deduction_1204" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_culprit_1205", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("42000"), lit("7:2,12/5"), lit("playthrough1204evening"), lit("collection1204_to_culprit1205.walk") });
    r.props(&.{ "collection_culprit_1205" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_adachi_1206", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("46000"), lit("7:2,12/6"), lit("playthrough1205shopping"), lit("collection1205_to_home1206.walk") });
    r.props(&.{ "collection_adachi_1206" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_evening_1207", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("35000"), lit("7:2,12/7"), lit("playthrough1206evening"), lit("collection1206_to_home1207.walk") });
    r.props(&.{ "collection_evening_1207" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_adachi_opening_1223", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("30000"), lit("246:3,12/23"), lit("playthrough1223magatsu"), lit("collection1223_magatsu_to_adachi.walk") });
    r.props(&.{ "collection_adachi_opening_1223" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_adachi_victory_1223", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("48000"), lit("246:2,12/23"), lit("playthrough1223magatsu"), lit("collection1223_adachi_to_ameno.walk") });
    r.props(&.{ "collection_adachi_victory_1223" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_departure_calendar_0320", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("30000"), lit("1:1,3/20"), lit("playthrough1224evening"), lit("collection1224_to_departure0320.walk") });
    r.props(&.{ "collection_departure_calendar_0320" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_normal_ending_0320", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("50000"), lit("-1:-1,4/10"), lit("playthrough0320farewell"), lit("collection0320_to_normal_ending.walk") });
    r.props(&.{ "collection_normal_ending_0320" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_true_trigger_0320", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("40000"), lit("20:1,3/20"), lit("playthrough0320farewell"), lit("collection0320_farewell_to_true_tv.walk") });
    r.props(&.{ "collection_true_trigger_0320" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_attendant_0320", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("20000"), lit("20:1,3/20"), lit("playthrough0320shopping3"), lit("collection0320_attendant_to_tv.walk") });
    r.props(&.{ "collection_attendant_0320" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("collection_yomotsu_entrance_0320", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("12000"), lit("30:2,3/20"), lit("playthrough0320tv"), lit("collection0320_tv_to_yomotsu_entrance.walk") });
    r.props(&.{ "collection_yomotsu_entrance_0320" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_izanami_opening_0320", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("15000"), lit("247:2,3/20"), lit("playthrough0320yomotsu9f"), lit("collection0320_yomotsu9f_to_izanami.walk") });
    r.props(&.{ "collection_izanami_opening_0320" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_margaret_0320", &.{ lit("sh"), src("tests/completed_walk_gate.sh"), tf("p4pc"), lit("15000"), lit("28:2,3/20"), lit("playthrough0320margaret18506"), lit("collection0320_cbs18506_margaret_victory.walk") });
    r.props(&.{ "collection_margaret_0320" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600 });
    r.add("collection_new_cycle_0411", &.{ lit("sh"), src("tests/save_play_gate.sh"), tf("p4pc"), lit("50000"), lit("7:2,4/11"), lit("playthrough0410newcycle"), lit("collection0410_newcycle_to_home0411.walk") });
    r.props(&.{ "collection_new_cycle_0411" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 600, .env = &.{ env("P4PC_GATE_REQUIRE_SAVE", "1") } });
    r.add("battle_rakukaja", &.{ lit("sh"), src("tests/battle_gate.sh"), tf("p4pc"), lit("532"), lit("1800") });
    r.props(&.{ "battle_rakukaja" }, .{ .skip = 77, .labels = &.{ "gl", "scenes" }, .timeout = 300, .env = &.{ esrc("P4PC_WALK_SCRIPT", "tests/walks/dream_rakukaja.walk") } });
}

/// 190-platform-walk.cmake
fn frag190(r: *Registry) void {
    r.add("walk_choice", &.{ bare("walk_choice_test") });
    r.add("dungeon_digest", &.{ bare("dungeon_digest_test") });
    r.add("walk_battle_rules", &.{ bare("walk_battle_rules_test") });
}

/// 191-nav-probe.cmake
fn frag191(r: *Registry) void {
    r.add("nav_probe", &.{ bare("nav_probe_test"), src("tests/nav_probe.walk") });
}

/// 220-port64-host.cmake (viewer/game twins, port frames, ovdiff_subset64; loops and functions expanded)
fn frag220(r: *Registry) void {
    if (r.hasTarget("p4pc_viewer64")) {
        r.add("title_mad64", &.{ lit("sh"), src("tests/title_mad.sh"), tf("p4pc_viewer64"), bin("64_title_mad.png") });
        r.add("field_mad64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_field_mad.png"), lit("field:7:2"), lit("field"), lit("1.9"), lit("P4PC_DRAW_WINDOW=4") });
        r.add("title_mad_gl3364", &.{ cmake, lit("-E"), lit("env"), lit("P4PC_GL_API=gl"), lit("sh"), src("tests/title_mad.sh"), tf("p4pc_viewer64"), bin("64_title_mad_gl33.png") });
        r.add("title_mad_ctxloss64", &.{ cmake, lit("-E"), lit("env"), lit("P4PC_TEST_CONTEXT_LOSS=200"), lit("sh"), src("tests/title_mad.sh"), tf("p4pc_viewer64"), bin("64_title_mad_ctxloss.png") });
        r.add("field_mad_ctxloss64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_field_mad_ctxloss.png"), lit("field:7:2"), lit("field"), lit("1.6"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_TEST_CONTEXT_LOSS=300") });
        r.props(&.{ "title_mad_gl3364", "title_mad_ctxloss64" }, .{ .skip = 77, .labels = &.{ "gl", "port64" }, .timeout = 300 });
        r.props(&.{ "field_mad_ctxloss64" }, .{ .skip = 77, .labels = &.{ "gl", "port64" }, .timeout = 600 });
        r.props(&.{ "title_mad64" }, .{ .skip = 77, .labels = &.{ "gl", "port64" }, .timeout = 300 });
        r.props(&.{ "field_mad64" }, .{ .skip = 77, .labels = &.{ "gl", "port64" }, .timeout = 600 });
        r.add("widescreen_field_16_964", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_widescreen_field_16_9.png"), lit("field:7:2"), lit("port-wide-field-16-9"), lit("0.1"), lit("P4PC_ASPECT=16:9"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)") });
        r.props(&.{ "widescreen_field_16_964" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen", "port64" }, .timeout = 600 });
        r.add("scene_resolution_title108064", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_resolution_title1080.png"), lit("title"), lit("p4-resolution-title1080"), lit("0.5"), lit("P4PC_RESOLUTION=1920x1080"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_DRAW_WINDOW=4") });
        r.add("scene_resolution_field136664", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_resolution_field1366.png"), lit("field:7:2"), lit("p4-resolution-field1366"), lit("0.5"), lit("P4PC_RESOLUTION=1366x768"), lit("P4PC_DRAW_WINDOW=4") });
        r.add("scene_resolution_butterfly144064", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_resolution_butterfly1440.png"), lit("field:8:2"), lit("p4-resolution-butterfly1440"), lit("0.5"), lit("P4PC_RESOLUTION=2560x1440"), lit("P4PC_DRAW_WINDOW=30"), lit("P4PC_FIELD_PLAYER=311.5,0,-2216.1"), lit("P4PC_FIELD_DATE=16"), lit("P4PC_FIELD_SLOT=3"), lit("P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656") });
        r.props(&.{ "scene_resolution_title108064", "scene_resolution_field136664", "scene_resolution_butterfly144064" }, .{ .skip = 77, .labels = &.{ "gl", "port64" }, .timeout = 600 });
        r.add("scene_resolution_title1366_cells64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_resolution_title1366_cells.png"), lit("title"), lit("p4-resolution-title1366-cells"), lit("0.5"), lit("P4PC_RESOLUTION=1366x768"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_WINDOW_W=1277"), lit("P4PC_WINDOW_H=954"), lit("P4PC_CAPTURE_WINDOW=1") });
        r.add("scene_resolution_title1277_cells64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_resolution_title1277_cells.png"), lit("title"), lit("p4-resolution-title1277-cells"), lit("0.5"), lit("P4PC_RESOLUTION=1277x954"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_WINDOW_W=1277"), lit("P4PC_WINDOW_H=954"), lit("P4PC_CAPTURE_WINDOW=1") });
        r.props(&.{ "scene_resolution_title1366_cells64", "scene_resolution_title1277_cells64" }, .{ .skip = 77, .labels = &.{ "gl", "port64" }, .timeout = 600, .env = &.{ env("P4PC_TITLE_EDGES", "1") } });
        r.add("widescreen_field_16_9_nvidia64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_widescreen_field_16_9_nvidia.png"), lit("field:7:2"), lit("port-wide-field-16-9-nvidia-rtx4080"), lit("0.1"), lit("P4PC_ASPECT=16:9"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_GATE_RENDERER=NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") });
        r.props(&.{ "widescreen_field_16_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 600 });
        r.add("widescreen_title_cover_16_964", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_widescreen_title_cover_16_964.png"), lit("title"), lit("port-wide-title-cover-16-9-llvmpipe19"), lit("0.1"), lit("P4PC_CONFIG=none"), lit("P4PC_ASPECT=16:9"), lit("P4PC_WINDOW_W=1280"), lit("P4PC_WINDOW_H=720"), lit("P4PC_RESOLUTION=scale:1"), lit("P4PC_PRESENT=blit"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_VIRTUAL_CLOCK=1"), lit("P4PC_CAPTURE_WINDOW=1"), lit("P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)") });
        r.props(&.{ "widescreen_title_cover_16_964" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen", "port64" }, .timeout = 600, .env = &.{ env("P4PC_TITLE_EDGES", "1") } });
        r.add("widescreen_title_cover_16_9_nvidia64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_widescreen_title_cover_16_9_nvidia64.png"), lit("title"), lit("port-wide-title-cover-16-9-nvidia-rtx4080"), lit("0.1"), lit("P4PC_CONFIG=none"), lit("P4PC_ASPECT=16:9"), lit("P4PC_WINDOW_W=1280"), lit("P4PC_WINDOW_H=720"), lit("P4PC_RESOLUTION=scale:1"), lit("P4PC_PRESENT=blit"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_VIRTUAL_CLOCK=1"), lit("P4PC_CAPTURE_WINDOW=1"), lit("P4PC_GATE_RENDERER=NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") });
        r.props(&.{ "widescreen_title_cover_16_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 600, .env = &.{ env("P4PC_TITLE_EDGES", "1") } });
        r.add("widescreen_title_cover_21_964", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_widescreen_title_cover_21_964.png"), lit("title"), lit("port-wide-title-cover-21-9-llvmpipe19"), lit("0.1"), lit("P4PC_CONFIG=none"), lit("P4PC_ASPECT=auto"), lit("P4PC_WINDOW_W=1680"), lit("P4PC_WINDOW_H=720"), lit("P4PC_RESOLUTION=scale:1"), lit("P4PC_PRESENT=blit"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_VIRTUAL_CLOCK=1"), lit("P4PC_CAPTURE_WINDOW=1"), lit("P4PC_GATE_RENDERER=llvmpipe (LLVM 19.1.7, 256 bits)") });
        r.props(&.{ "widescreen_title_cover_21_964" }, .{ .skip = 77, .labels = &.{ "gl", "scenes", "widescreen", "port64" }, .timeout = 600, .env = &.{ env("P4PC_TITLE_EDGES", "1") } });
        r.add("widescreen_title_cover_21_9_nvidia64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("64_widescreen_title_cover_21_9_nvidia64.png"), lit("title"), lit("port-wide-title-cover-21-9-nvidia-rtx4080"), lit("0.1"), lit("P4PC_CONFIG=none"), lit("P4PC_ASPECT=auto"), lit("P4PC_WINDOW_W=1680"), lit("P4PC_WINDOW_H=720"), lit("P4PC_RESOLUTION=scale:1"), lit("P4PC_PRESENT=blit"), lit("P4PC_TITLE_FRAME=400"), lit("P4PC_VIRTUAL_CLOCK=1"), lit("P4PC_CAPTURE_WINDOW=1"), lit("P4PC_GATE_RENDERER=NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") });
        r.props(&.{ "widescreen_title_cover_21_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 600, .env = &.{ env("P4PC_TITLE_EDGES", "1") } });
    }
    if (r.hasTarget("p4pc64")) {
        r.add("widescreen_battle_16_9_nvidia64", &.{ lit("sh"), src("tests/battle_mad.sh"), tf("p4pc64"), lit("532"), lit("port-wide-battle-16-9-nvidia-rtx4080"), lit("0"), lit("0.1"), lit("cross@360:2/150,start@500:2/300"), lit("P4PC_ASPECT=16:9"), lit("P4PC_GATE_RENDERER=NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), lit("P4PC_VIRTUAL_CLOCK=1"), lit("P4PC_CAPTURE_STATE=battle-menu,40,1,4000") });
        r.props(&.{ "widescreen_battle_16_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 600 });
        r.add("widescreen_dialogue_16_9_nvidia64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("velvet-windows-blurry"), lit("3630"), lit("0.1"), lit("story0415"), lit("save0415_to_river_scene.walk") });
        r.props(&.{ "widescreen_dialogue_16_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 900, .env = &.{ env("P4PC_ASPECT", "16:9"), env("P4PC_EVENT_REF", "port-wide-dialogue-16-9-nvidia-rtx4080"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") } });
        r.add("widescreen_allout_masked_16_9_nvidia64", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc64"), lit("allout"), lit("16:9"), lit("port-wide-allout-masked-16-9-nvidia-rtx4080") });
        r.props(&.{ "widescreen_allout_masked_16_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") } });
        r.add("widescreen_shuffle_full_16_9_nvidia64", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc64"), lit("shuffle"), lit("16:9"), lit("port-wide-shuffle-full-vu0-16-9-nvidia-rtx4080") });
        r.props(&.{ "widescreen_shuffle_full_16_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") } });
        r.add("widescreen_allout_intro_16_9_nvidia64", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc64"), lit("allout-intro"), lit("16:9"), lit("port-wide-allout-intro-16-9-nvidia-rtx4080") });
        r.props(&.{ "widescreen_allout_intro_16_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") } });
        r.add("widescreen_cast_eyes_16_9_nvidia64", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc64"), lit("cast-eyes"), lit("16:9"), lit("port-wide-cast-eyes-16-9-nvidia-rtx4080") });
        r.props(&.{ "widescreen_cast_eyes_16_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") } });
        r.add("widescreen_allout_masked_21_9_nvidia64", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc64"), lit("allout"), lit("21:9"), lit("port-wide-allout-masked-21-9-nvidia-rtx4080") });
        r.props(&.{ "widescreen_allout_masked_21_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") } });
        r.add("widescreen_shuffle_full_21_9_nvidia64", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc64"), lit("shuffle"), lit("21:9"), lit("port-wide-shuffle-full-vu0-21-9-nvidia-rtx4080") });
        r.props(&.{ "widescreen_shuffle_full_21_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") } });
        r.add("widescreen_allout_intro_21_9_nvidia64", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc64"), lit("allout-intro"), lit("21:9"), lit("port-wide-allout-intro-21-9-nvidia-rtx4080") });
        r.props(&.{ "widescreen_allout_intro_21_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") } });
        r.add("widescreen_cast_eyes_21_9_nvidia64", &.{ lit("sh"), src("tests/widescreen_saved_gate.sh"), tf("p4pc64"), lit("cast-eyes"), lit("21:9"), lit("port-wide-cast-eyes-21-9-nvidia-rtx4080") });
        r.props(&.{ "widescreen_cast_eyes_21_9_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "widescreen", "port64" }, .timeout = 900, .env = &.{ env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2") } });
    }
    if (r.hasTarget("p4pc64")) {
        r.add("scene_port_fog_battle_llvmpipe64", &.{ lit("sh"), src("tests/battle_mad.sh"), tf("p4pc64"), lit("532"), lit("port-scene-fog-battle-llvmpipe19"), lit("1700"), lit("0.05"), lit("cross@360:2/150,start@500:2/300") });
        r.mkdirBin("save/scene_port_fog_battle_llvmpipe64");
        r.props(&.{ "scene_port_fog_battle_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 300, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_fog_battle_llvmpipe64") } });
        r.add("scene_port_tv_entrance_llvmpipe64", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc64"), lit("entrance"), lit("0.05"), lit("port-scene-tv-entrance-llvmpipe19") });
        r.mkdirBin("save/scene_port_tv_entrance_llvmpipe64");
        r.props(&.{ "scene_port_tv_entrance_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 1200, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_tv_entrance_llvmpipe64") } });
        r.add("scene_port_near_fog_llvmpipe64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("near-fog"), lit("4600"), lit("0.05"), lit("card"), lit("save0411_to_near_fog_scene.walk"), lit("0,0,640,200"), lit("0.05") });
        r.mkdirBin("save/scene_port_near_fog_llvmpipe64");
        r.props(&.{ "scene_port_near_fog_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 900, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_near_fog_llvmpipe64"), env("P4PC_EVENT_REF", "port-scene-near-fog-llvmpipe19") } });
        r.add("scene_port_velvet_windows_blurry_llvmpipe64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("velvet-windows-blurry"), lit("3630"), lit("0.05"), lit("story0415"), lit("save0415_to_river_scene.walk") });
        r.mkdirBin("save/scene_port_velvet_windows_blurry_llvmpipe64");
        r.props(&.{ "scene_port_velvet_windows_blurry_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 900, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_velvet_windows_blurry_llvmpipe64"), env("P4PC_EVENT_REF", "port-scene-velvet-windows-blurry-llvmpipe19") } });
        r.add("scene_port_riverbank_vignette_llvmpipe64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("riverbank-vignette"), lit("5880"), lit("0.05"), lit("story0415"), lit("save0415_to_river_scene.walk") });
        r.mkdirBin("save/scene_port_riverbank_vignette_llvmpipe64");
        r.props(&.{ "scene_port_riverbank_vignette_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 900, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_riverbank_vignette_llvmpipe64"), env("P4PC_EVENT_REF", "port-scene-riverbank-vignette-llvmpipe19") } });
        r.add("scene_port_rain_llvmpipe64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("rain"), lit("25100"), lit("0.05"), lit("newgame"), lit("newgame_to_rain_scene.walk") });
        r.mkdirBin("save/scene_port_rain_llvmpipe64");
        r.props(&.{ "scene_port_rain_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 1200, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_rain_llvmpipe64"), env("P4PC_EVENT_REF", "port-scene-rain-llvmpipe19") } });
        r.add("scene_port_arcana_rankup_sparkles_llvmpipe64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("arcana-rankup-sparkles"), lit("7300"), lit("0.05"), lit("story0415"), lit("save0415_to_street.walk") });
        r.mkdirBin("save/scene_port_arcana_rankup_sparkles_llvmpipe64");
        r.props(&.{ "scene_port_arcana_rankup_sparkles_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 900, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_arcana_rankup_sparkles_llvmpipe64"), env("P4PC_EVENT_REF", "port-scene-arcana-rankup-sparkles-llvmpipe19") } });
        r.add("scene_port_tv_world_enter_llvmpipe64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("tv-world-enter"), lit("71778"), lit("0.05"), lit("newgame"), lit("newgame_to_0413.walk") });
        r.mkdirBin("save/scene_port_tv_world_enter_llvmpipe64");
        r.props(&.{ "scene_port_tv_world_enter_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64", "slow" }, .timeout = 1700, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_tv_world_enter_llvmpipe64"), env("P4PC_EVENT_REF", "port-scene-tv-world-enter-llvmpipe19") } });
        r.add("scene_port_shadow_haze_llvmpipe64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("shadow-haze"), lit("103408"), lit("0.05"), lit("newgame"), lit("newgame_to_0413.walk") });
        r.mkdirBin("save/scene_port_shadow_haze_llvmpipe64");
        r.props(&.{ "scene_port_shadow_haze_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64", "slow" }, .timeout = 1700, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_shadow_haze_llvmpipe64"), env("P4PC_EVENT_REF", "port-scene-shadow-haze-realtime-voice-llvmpipe19") } });
    }
    if (r.hasTarget("p4pc_viewer64")) {
        r.add("scene_port_field_llvmpipe64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("scene_port_field_llvmpipe64.png"), lit("field:7:2"), lit("port-scene-field-llvmpipe19"), lit("0.05"), lit("P4PC_DRAW_WINDOW=4") });
        r.mkdirBin("save/scene_port_field_llvmpipe64");
        r.props(&.{ "scene_port_field_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 600, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_field_llvmpipe64") } });
        r.add("scene_port_field_ctxloss_llvmpipe64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("scene_port_field_ctxloss_llvmpipe64.png"), lit("field:7:2"), lit("port-scene-field-llvmpipe19"), lit("0.05"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_TEST_CONTEXT_LOSS=300") });
        r.mkdirBin("save/scene_port_field_ctxloss_llvmpipe64");
        r.props(&.{ "scene_port_field_ctxloss_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 600, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_field_ctxloss_llvmpipe64") } });
        r.add("scene_port_sky_black_tint_llvmpipe64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("scene_port_sky_black_tint_llvmpipe64.png"), lit("field:10:1"), lit("port-scene-sky-black-tint-llvmpipe19"), lit("0.05"), lit("P4PC_FIELD_DATE=17"), lit("P4PC_FIELD_SLOT=4"), lit("P4PC_FIELD_CAMERA=0.34202,0.0,-0.939693,0.0,0.0,1.0,0.0,0.0,0.939693,0.0,0.34202,0.0,491.663574,48.057514,-7614.540039,-131819.28125,0.511802,0.383852,50.000393,200000.0") });
        r.mkdirBin("save/scene_port_sky_black_tint_llvmpipe64");
        r.props(&.{ "scene_port_sky_black_tint_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 600, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_sky_black_tint_llvmpipe64") } });
        r.add("scene_port_save_butterfly_glow_llvmpipe64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("scene_port_save_butterfly_glow_llvmpipe64.png"), lit("field:8:2"), lit("port-scene-save-butterfly-glow-llvmpipe19"), lit("0.05"), lit("P4PC_DRAW_WINDOW=30"), lit("P4PC_FIELD_PLAYER=311.5,0,-2216.1"), lit("P4PC_FIELD_DATE=16"), lit("P4PC_FIELD_SLOT=3"), lit("P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656") });
        r.mkdirBin("save/scene_port_save_butterfly_glow_llvmpipe64");
        r.props(&.{ "scene_port_save_butterfly_glow_llvmpipe64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 600, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "llvmpipe (LLVM 19.1.7, 256 bits)"), ebin("P4PC_SAVE_DIR", "save/scene_port_save_butterfly_glow_llvmpipe64") } });
    }
    if (r.hasTarget("p4pc64")) {
        r.add("scene_port_fog_battle_nvidia64", &.{ lit("sh"), src("tests/battle_mad.sh"), tf("p4pc64"), lit("532"), lit("port-scene-fog-battle-nvidia-rtx4080"), lit("1700"), lit("0.05"), lit("cross@360:2/150,start@500:2/300") });
        r.mkdirBin("save/scene_port_fog_battle_nvidia64");
        r.props(&.{ "scene_port_fog_battle_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 300, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_fog_battle_nvidia64") } });
        r.add("scene_port_tv_entrance_nvidia64", &.{ lit("sh"), src("tests/systems_gate.sh"), tf("p4pc64"), lit("entrance"), lit("0.05"), lit("port-scene-tv-entrance-nvidia-rtx4080") });
        r.mkdirBin("save/scene_port_tv_entrance_nvidia64");
        r.props(&.{ "scene_port_tv_entrance_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 1200, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_tv_entrance_nvidia64") } });
        r.add("scene_port_near_fog_nvidia64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("near-fog"), lit("4600"), lit("0.05"), lit("card"), lit("save0411_to_near_fog_scene.walk"), lit("0,0,640,200"), lit("0.05") });
        r.mkdirBin("save/scene_port_near_fog_nvidia64");
        r.props(&.{ "scene_port_near_fog_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 900, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_near_fog_nvidia64"), env("P4PC_EVENT_REF", "port-scene-near-fog-nvidia-rtx4080") } });
        r.add("scene_port_velvet_windows_blurry_nvidia64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("velvet-windows-blurry"), lit("3630"), lit("0.05"), lit("story0415"), lit("save0415_to_river_scene.walk") });
        r.mkdirBin("save/scene_port_velvet_windows_blurry_nvidia64");
        r.props(&.{ "scene_port_velvet_windows_blurry_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 900, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_velvet_windows_blurry_nvidia64"), env("P4PC_EVENT_REF", "port-scene-velvet-windows-blurry-nvidia-rtx4080") } });
        r.add("scene_port_riverbank_vignette_nvidia64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("riverbank-vignette"), lit("5880"), lit("0.05"), lit("story0415"), lit("save0415_to_river_scene.walk") });
        r.mkdirBin("save/scene_port_riverbank_vignette_nvidia64");
        r.props(&.{ "scene_port_riverbank_vignette_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 900, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_riverbank_vignette_nvidia64"), env("P4PC_EVENT_REF", "port-scene-riverbank-vignette-nvidia-rtx4080") } });
        r.add("scene_port_rain_nvidia64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("rain"), lit("25100"), lit("0.05"), lit("newgame"), lit("newgame_to_rain_scene.walk") });
        r.mkdirBin("save/scene_port_rain_nvidia64");
        r.props(&.{ "scene_port_rain_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 1200, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_rain_nvidia64"), env("P4PC_EVENT_REF", "port-scene-rain-nvidia-rtx4080") } });
        r.add("scene_port_arcana_rankup_sparkles_nvidia64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("arcana-rankup-sparkles"), lit("7300"), lit("0.05"), lit("story0415"), lit("save0415_to_street.walk") });
        r.mkdirBin("save/scene_port_arcana_rankup_sparkles_nvidia64");
        r.props(&.{ "scene_port_arcana_rankup_sparkles_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 900, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_arcana_rankup_sparkles_nvidia64"), env("P4PC_EVENT_REF", "port-scene-arcana-rankup-sparkles-nvidia-rtx4080") } });
        r.add("scene_port_tv_world_enter_nvidia64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("tv-world-enter"), lit("71778"), lit("0.05"), lit("newgame"), lit("newgame_to_0413.walk") });
        r.mkdirBin("save/scene_port_tv_world_enter_nvidia64");
        r.props(&.{ "scene_port_tv_world_enter_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64", "slow" }, .timeout = 1700, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_tv_world_enter_nvidia64"), env("P4PC_EVENT_REF", "port-scene-tv-world-enter-nvidia-rtx4080") } });
        r.add("scene_port_shadow_haze_nvidia64", &.{ lit("sh"), src("tests/event_scene_gate.sh"), tf("p4pc64"), lit("shadow-haze"), lit("103408"), lit("0.05"), lit("newgame"), lit("newgame_to_0413.walk") });
        r.mkdirBin("save/scene_port_shadow_haze_nvidia64");
        r.props(&.{ "scene_port_shadow_haze_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64", "slow" }, .timeout = 1700, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_shadow_haze_nvidia64"), env("P4PC_EVENT_REF", "port-scene-shadow-haze-realtime-voice-nvidia-rtx4080") } });
    }
    if (r.hasTarget("p4pc_viewer64")) {
        r.add("scene_port_field_nvidia64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("scene_port_field_nvidia64.png"), lit("field:7:2"), lit("port-scene-field-nvidia-rtx4080"), lit("0.05"), lit("P4PC_DRAW_WINDOW=4") });
        r.mkdirBin("save/scene_port_field_nvidia64");
        r.props(&.{ "scene_port_field_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 600, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_field_nvidia64") } });
        r.add("scene_port_field_ctxloss_nvidia64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("scene_port_field_ctxloss_nvidia64.png"), lit("field:7:2"), lit("port-scene-field-nvidia-rtx4080"), lit("0.05"), lit("P4PC_DRAW_WINDOW=4"), lit("P4PC_TEST_CONTEXT_LOSS=300") });
        r.mkdirBin("save/scene_port_field_ctxloss_nvidia64");
        r.props(&.{ "scene_port_field_ctxloss_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 600, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_field_ctxloss_nvidia64") } });
        r.add("scene_port_sky_black_tint_nvidia64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("scene_port_sky_black_tint_nvidia64.png"), lit("field:10:1"), lit("port-scene-sky-black-tint-nvidia-rtx4080"), lit("0.05"), lit("P4PC_FIELD_DATE=17"), lit("P4PC_FIELD_SLOT=4"), lit("P4PC_FIELD_CAMERA=0.34202,0.0,-0.939693,0.0,0.0,1.0,0.0,0.0,0.939693,0.0,0.34202,0.0,491.663574,48.057514,-7614.540039,-131819.28125,0.511802,0.383852,50.000393,200000.0") });
        r.mkdirBin("save/scene_port_sky_black_tint_nvidia64");
        r.props(&.{ "scene_port_sky_black_tint_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 600, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_sky_black_tint_nvidia64") } });
        r.add("scene_port_save_butterfly_glow_nvidia64", &.{ lit("sh"), src("tests/scene_mad.sh"), tf("p4pc_viewer64"), bin("scene_port_save_butterfly_glow_nvidia64.png"), lit("field:8:2"), lit("port-scene-save-butterfly-glow-nvidia-rtx4080"), lit("0.05"), lit("P4PC_DRAW_WINDOW=30"), lit("P4PC_FIELD_PLAYER=311.5,0,-2216.1"), lit("P4PC_FIELD_DATE=16"), lit("P4PC_FIELD_SLOT=3"), lit("P4PC_FIELD_CAMERA=-0.753238,0.0,0.657748,0.0,-0.181959,0.960974,-0.208375,1.0,-0.632078,-0.276639,-0.723842,0.0,616.131653,329.264832,-1461.691772,-172110.5625,0.511799,0.383849,100.0,63520.722656") });
        r.mkdirBin("save/scene_port_save_butterfly_glow_nvidia64");
        r.props(&.{ "scene_port_save_butterfly_glow_nvidia64" }, .{ .skip = 77, .labels = &.{ "gl", "scenes64", "port64" }, .timeout = 600, .env = &.{ env("P4PC_VIRTUAL_CLOCK", "1"), env("P4PC_ASPECT", "4:3"), env("P4PC_RESOLUTION", "scale:1"), env("P4PC_GATE_RENDERER", "NVIDIA GeForce RTX 4080 Laptop GPU/PCIe/SSE2"), ebin("P4PC_SAVE_DIR", "save/scene_port_save_butterfly_glow_nvidia64") } });
    }
    if (!r.p.apple) {
        r.add("ovdiff_subset64", &.{ bare("ovdiff64"), lit("-n"), lit("40"), lit("env"), srcp("@", "tests/ovdiff_subset.txt") });
        r.props(&.{ "ovdiff_subset64" }, .{ .skip = 77, .timeout = 900, .cwd = "", .labels = &.{ "port64" }, .env = &.{ esrcdir("P4_MIPSDIFF_DIR", "tools/mipsdiff") } });
    }
}

comptime {
    _ = &fixture;
}
