//! The native game graph of `zig build` (build.zig): the same targets, flags, generators and gates as the CMake build
//! (CMakeLists.txt, cmake/sources/*.cmake, tools/mods/CMakeLists.txt, tools/luau/CMakeLists.txt), compiled and linked with
//! the pinned Zig nightly (`zig cc`/`zig c++` through zigbuild/zig_cc.zig) and driven by the repository's Python generators
//! directly. CMake and Ninja take no part.
//!
//! One `zig build` profile is one CMake build directory: `-Dplatform` (linux, windows, android, macos) with
//! `-Dport64-arch` (Linux/macOS) or `-Dandroid-abi`. The build tree (`-Dbuild-dir`, default build/zig-<profile>) has the
//! layout of a CMake build directory, because the tools read it by layout: gen/ (gen_port.py --build: the mirror, units.txt,
//! tables, CRT reports), gen/obj64{,w}/units and abi (tools/port64/unit64.py), CMakeFiles/<target>.dir/<source>.o objects
//! (tools/crt_guard.py, tools/gen_port.py tables) and compile_commands.json (tools/crt_guard.py, tools/crt_flow.py). Every
//! compile and generator runs as a cached step whose outputs are in the Zig cache; an always-run `stage` step
//! (tools/zig_build_step.py) puts them at those fixed paths, so consumers re-run on content changes and see the CMake tree.
//!
//! Configure (this file) runs on every invocation: it runs `gen_port.py units` and writes the configure-time files
//! (package_boot_providers.c, battle_sim_exports.h, mod_luau_api.h, object lists, compile_commands.json, the graph manifest)
//! exactly as CMake's configure does; a missing prerequisite or an unsupported profile is a configure error.
const std = @import("std");
const builtin = @import("builtin");
const Build = std.Build;
const Step = Build.Step;
const Run = Step.Run;
const LazyPath = Build.LazyPath;
const Io = std.Io;

pub const Platform = enum { linux, windows, android, macos };
pub const Arch = enum { x86_64, aarch64 };
pub const AndroidAbi = enum { @"arm64-v8a", x86_64, x86 };

/// The pointer models of cmake/host-cxx.cmake p4_host_cxx_profiles: "" (i386), "64" (the 64-bit model of the profile) and
/// "64w" (x86_64 Windows, cross-built from a Linux tree).
pub const Model = enum {
    i386,
    m64,
    m64w,

    pub fn suffix(m: Model) []const u8 {
        return switch (m) {
            .i386 => "",
            .m64 => "64",
            .m64w => "64w",
        };
    }
};

/// The libraries a final executable or shared library of `model` links when it contains p4port<suffix>'s objects, in link
/// order, as cmake/host-cxx.cmake p4_host_adapter_attach appends them (p4_mod_game and its transitive usage requirements).
/// Empty when the profile registered no host adapters. `luau_active` is P4PC_MOD_SUPPORT AND P4PC_LUAU.
pub fn adapterLibraries(model: Model, mod_support_profile: bool, luau_active: bool) []const []const u8 {
    if (!mod_support_profile) return &.{};
    const with_luau = [_][]const u8{ "p4_mod_game", "p4_mod_luau", "p4_mod_core", "p4_luau", "p4_luau_Compiler", "p4_luau_VM", "p4_luau_Ast", "p4_luau_Bytecode", "p4_luau_Common" };
    const c_only = [_][]const u8{ "p4_mod_game", "p4_mod_core" };
    _ = model;
    return if (luau_active) &with_luau else &c_only;
}

/// Whether a consumer of the adapters links as C++ (P4_HOST_ADAPTER_CXX_<object>: p4_mod_luau registers CXX).
pub fn adapterLinksCxx(luau_active: bool) bool {
    return luau_active;
}

/// The adapter registry key a final target attaches by: the p4port<suffix> object library it consumes (null: none).
pub fn adapterObjectOf(consumes_p4port: bool, model: Model) ?Model {
    return if (consumes_p4port) model else null;
}

pub const Artifact = struct {
    /// The CMake build-tree path of the product (`<build-dir>/p4pc`, `<build-dir>/win64/p4pc64w.exe`, ...).
    path: []const u8,
    /// The product at `path`, reached through a cached link to its build-tree directory: depending on it orders after the
    /// product and everything beside it is placed (the DLLs / ANGLE it loads, see `view`), so running it is running the
    /// build-tree executable, never a lone copy in the Zig cache.
    lazy: LazyPath,
    /// The linker's own output in the Zig cache (contents identical to `path`; for hashing, never for running).
    cached: LazyPath,
    /// Builds and places it at `path` (including its pre-link CRT audit).
    step: *Step,
    model: Model,
    /// The step that makes `lazy` usable; extra runtime files the executable loads are added as its dependencies.
    view: *Step,
};

pub const Steps = struct {
    artifacts: *Step,
    compile_only: *Step,
    census: *Step,
    census_generate: *Step,
    crt: *Step,
    tables: *Step,
    tools: *Step,
    mirror: *Step,
    check: *Step,
};

pub const Graph = struct {
    profile: []const u8,
    platform: Platform,
    root: []const u8,
    build_dir: []const u8,
    gen_dir: []const u8,
    python: []const u8,
    /// The compiler executables (fixed paths in the build tree) and the step that places them there.
    cc: []const u8,
    cxx: []const u8,
    cc_ready: *Step,
    toolchain_id: []const u8,
    compile_commands: []const u8,
    /// JSON description of every target: kind, model, sources, objects, attached host adapters, linker language, product.
    manifest: []const u8,
    steps: Steps,
    /// Every built executable by CMake target name; native test targets added through this Graph register here too.
    artifacts: *std.StringArrayHashMapUnmanaged(Artifact),
    ctx: *Ctx,
    /// Library and runtime files placed beside the executables (Windows DLLs, macOS ANGLE).
    runtime_files: []const []const u8,
    models: []const Model,
    mod_support: bool,
    luau: bool,
    census_check: bool,
    llvm_nm: []const u8,
    llvm_objdump: []const u8,
    win32: bool,
    apple: bool,
    android: bool,
    android64: bool,
    has_i386: bool,
    port64_arch: Arch,
    /// The rewritten 64-bit mirror is built (P4_PORT64_OBJS), its Windows x64 copy (P4_PORT64W_OBJS), and the 64-bit links.
    port64: bool,
    port64w: bool,
    link64: bool,
    link64w: bool,
    /// ${P4_G64} and the census baseline (absolute; empty without the 64-bit mirror).
    g64: []const u8,
    census_baseline: []const u8,
    /// -Dretail (P4_RETAIL).
    retail: ?[]const u8,
    /// ${P4_G64} as a lazy path that is ready only after the 64-bit units, their ABI table and abi.json are staged
    /// (a cached symbolic link to the fixed tree): consumers that read the tree depend on it through this path.
    g64_view: ?LazyPath,
};

fn fatal(comptime format: []const u8, args: anytype) noreturn {
    std.process.fatal("zig build (P4): " ++ format, args);
}

const HostArg = union(enum) {
    str: []const u8,
    file: LazyPath,
};

const Obj = struct {
    fixed: []const u8,
    cached: LazyPath,
    /// the compiled source (empty for files that are not compile outputs)
    source: []const u8 = "",
};

/// Files compiled or produced by cached steps, placed at fixed build-tree paths by one always-run stage step.
const Set = struct {
    name: []const u8,
    items: std.ArrayList(Obj) = .empty,
    stage: *Run,
};

const Lang = enum { c, cxx, asm_, zig };

const CompdbEntry = struct {
    file: []const u8,
    output: []const u8,
    arguments: []const []const u8,
};

const ManifestTarget = struct {
    name: []const u8,
    kind: []const u8,
    model: Model,
    sources: []const []const u8,
    objects: []const []const u8,
    libraries: []const []const u8,
    adapters: []const []const u8,
    linker: []const u8,
    product: []const u8,
};

const Options = struct {
    platform: Platform,
    arch: Arch,
    android_abi: AndroidAbi,
    android_api: []const u8,
    android_sysroot: ?[]const u8,
    android_prefix: []const u8,
    android_unwind: ?[]const u8,
    decomp: []const u8,
    retail: ?[]const u8,
    libc32: []const u8,
    win32_prefix: []const u8,
    win64_prefix: []const u8,
    mingw32_lib_dir: ?[]const u8,
    mingw64_lib_dir: ?[]const u8,
    linux64_prefix: ?[]const u8,
    macos_prefix: ?[]const u8,
    macos_deployment: []const u8,
    glibc: []const u8,
    image_clang: []const u8,
    opt: []const u8,
    debug_info: []const u8,
    auto_init: ?[]const u8,
    ee_float: bool,
    ee_float_ops: []const u8,
    rw_clean: []const u8,
    port64: bool,
    port64_opt: []const u8,
    port64_link: bool,
    port64_win: ?bool,
    port64w_opt: []const u8,
    census_check: bool,
    gles: bool,
    egl_native: bool,
    mod_support: bool,
    luau: bool,
    zig_optimize: []const u8,
    python: []const u8,
    llvm_nm: []const u8,
    llvm_objdump: []const u8,
    build_dir: ?[]const u8,
    android_phone_runner: bool,
};

const Ctx = struct {
    b: *Build,
    a: std.mem.Allocator,
    io: Io,
    o: Options,
    root: []const u8,
    bdir: []const u8,
    gen: []const u8,
    profile: []const u8,
    cc: []const u8,
    cxx: []const u8,
    cc_bin: LazyPath,
    cc_stage: *Run,
    toolchain_id: []const u8,
    mirror: *Run,
    helper: []const u8,
    compdb: std.ArrayList(CompdbEntry) = .empty,
    manifest: std.ArrayList(ManifestTarget) = .empty,
    artifacts: std.StringArrayHashMapUnmanaged(Artifact) = .empty,
    runtime_files: std.ArrayList([]const u8) = .empty,
    steps: Steps,
    // profile facts
    android64: bool = false,
    apple: bool = false,
    win32: bool = false,
    android: bool = false,
    has_i386: bool = false,
    obj_ext: []const u8 = ".o",
    base_c: []const []const u8 = &.{},
    base_link: []const []const u8 = &.{},
    target_flags: []const []const u8 = &.{},
    target_link_flags: []const []const u8 = &.{},
    pe_repro: []const []const u8 = &.{},
    port_defines: []const []const u8 = &.{},
    global_c: []const []const u8 = &.{},
    global_zig: []const []const u8 = &.{},
    global_includes: []const []const u8 = &.{},
    global_system_includes: []const []const u8 = &.{},
    /// 030-generation-setup: the port transcriptions the selected RenderWare clean subsystems retire (P4_RW_CLEAN_PORT_DROP).
    rw_clean_drop: []const []const u8 = &.{},
    default_triple: []const u8 = "",
    ffmpeg_inc: []const u8 = "",
    os_src: []const u8 = "",
    // libraries of the i386/native profile
    sdl3: []const u8 = "",
    avcodec: []const u8 = "",
    avutil: []const u8 = "",
    swscale: []const u8 = "",
    host_lib_args: std.ArrayList(HostArg) = .empty,
    gen_tool_args: []const []const u8 = &.{},
    crt_flags: []const []const u8 = &.{},
    units: std.ArrayList(Unit) = .empty,
    g64_view: ?LazyPath = null,
    /// The game image of each model (objects, tables, adapters, libraries), for the native test targets.
    images: [3]?Image = .{ null, null, null },
    pm: PortModel = undefined,
    /// Static libraries by CMake target name ($<TARGET_FILE:name>).
    libraries: std.StringArrayHashMapUnmanaged(Library) = .empty,
    mod_fixture: ?GeneratedFiles = null,
    movie_libs: [3]?*ObjectLibrary = .{ null, null, null },
    test_names: std.StringHashMapUnmanaged(void) = .empty,
    finished: bool = false,
    emulator: []const []const u8 = &.{},
    dlls_ready: ?*Step = null,
    runtime_ready: ?*Step = null,
    /// setTwinLibrary: the ${obj}${suffix} a fragment built itself, by base object-library name
    twin_libraries: std.StringHashMapUnmanaged([3]?*ObjectLibrary) = .empty,
    /// "<name>|<suffix>" of every library a host twin cloned
    cloned: std.StringHashMapUnmanaged(void) = .empty,
    /// requireTwinLibrary: object libraries whose host-twin copies must come from setTwinLibrary, never a generic clone
    twin_required: std.StringHashMapUnmanaged(void) = .empty,
    /// the host adapters of each model (p4_mod_profile / p4_luau_profile), for the mod VM fixtures
    adapters: [3]?Adapters = .{ null, null, null },
    mod_luau_tests: [3]?ModLuauTests = .{ null, null, null },

    fn fmt(c: *Ctx, comptime format: []const u8, args: anytype) []const u8 {
        return c.b.fmt(format, args);
    }

    fn src(c: *Ctx, rel: []const u8) []const u8 {
        return c.join(&.{ c.root, rel });
    }

    fn join(c: *Ctx, parts: []const []const u8) []const u8 {
        return std.fs.path.join(c.a, parts) catch @panic("OOM");
    }

    fn lp(c: *Ctx, abs: []const u8) LazyPath {
        return c.b.graph.cwdRelativePath(abs);
    }

    /// A heap copy of a list whose elements are only known at configure time (never a pointer to a stack temporary).
    fn strs(c: *Ctx, items: []const []const u8) []const []const u8 {
        return c.a.dupe([]const u8, items) catch @panic("OOM");
    }

    fn list(c: *Ctx, parts: []const []const []const u8) []const []const u8 {
        var n: usize = 0;
        for (parts) |p| n += p.len;
        const out = c.a.alloc([]const u8, n) catch @panic("OOM");
        var i: usize = 0;
        for (parts) |p| {
            @memcpy(out[i..][0..p.len], p);
            i += p.len;
        }
        return out;
    }

    fn exists(c: *Ctx, path: []const u8) bool {
        Io.Dir.cwd().access(c.io, path, .{}) catch return false;
        return true;
    }

    fn read(c: *Ctx, path: []const u8) []u8 {
        return Io.Dir.cwd().readFileAlloc(c.io, path, c.a, .limited(256 << 20)) catch |e| fatal("cannot read {s}: {t}", .{ path, e });
    }

    /// file(CONFIGURE/GENERATE) semantics: written only when the text changes.
    fn writeIfChanged(c: *Ctx, path: []const u8, data: []const u8) void {
        if (Io.Dir.cwd().readFileAlloc(c.io, path, c.a, .limited(256 << 20))) |old| {
            if (std.mem.eql(u8, old, data)) return;
        } else |_| {}
        if (std.fs.path.dirname(path)) |dir| Io.Dir.cwd().createDirPath(c.io, dir) catch |e| fatal("cannot create {s}: {t}", .{ dir, e });
        Io.Dir.cwd().writeFile(c.io, .{ .sub_path = path, .data = data }) catch |e| fatal("cannot write {s}: {t}", .{ path, e });
    }

    fn listDir(c: *Ctx, dir_path: []const u8, suffix: []const u8) []const []const u8 {
        const dir = Io.Dir.cwd().openDir(c.io, dir_path, .{ .iterate = true }) catch |e| fatal("cannot open {s}: {t}", .{ dir_path, e });
        defer dir.close(c.io);
        var names: std.ArrayList([]const u8) = .empty;
        var it = dir.iterate();
        while (it.next(c.io) catch |e| fatal("cannot list {s}: {t}", .{ dir_path, e })) |entry| {
            if (entry.kind == .file and std.mem.endsWith(u8, entry.name, suffix))
                names.append(c.a, c.a.dupe(u8, entry.name) catch @panic("OOM")) catch @panic("OOM");
        }
        std.mem.sort([]const u8, names.items, {}, lessString);
        return names.items;
    }

    /// find_library(NAMES <names> PATHS <dirs> NO_DEFAULT_PATH REQUIRED): the first existing candidate.
    fn findLibrary(c: *Ctx, what: []const u8, names: []const []const u8, dirs: []const []const u8) []const u8 {
        for (names) |name| for (dirs) |dir| {
            const path = c.join(&.{ dir, name });
            if (c.exists(path)) return path;
        };
        fatal("{s} not found (looked for {s} in {s})", .{ what, joinWords(c, names), joinWords(c, dirs) });
    }

    fn findPath(c: *Ctx, what: []const u8, file: []const u8, dirs: []const []const u8) []const u8 {
        for (dirs) |dir| if (c.exists(c.join(&.{ dir, file }))) return dir;
        fatal("{s} not found ({s} in {s})", .{ what, file, joinWords(c, dirs) });
    }

    /// The fixed object path CMake gives a source of `target` (CMakeFiles/<target>.dir/<path relative to the source or build
    /// directory>.o), under `dir_prefix` for targets of a subdirectory (tools/mods/, tools/luau/).
    fn objectPath(c: *Ctx, dir_prefix: []const u8, target: []const u8, source: []const u8) []const u8 {
        const rel = if (std.mem.startsWith(u8, source, c.bdir) and source.len > c.bdir.len and source[c.bdir.len] == '/')
            source[c.bdir.len + 1 ..]
        else if (std.mem.startsWith(u8, source, c.root) and source.len > c.root.len and source[c.root.len] == '/')
            source[c.root.len + 1 ..]
        else
            std.mem.trimStart(u8, source, "/");
        return c.fmt("{s}/{s}CMakeFiles/{s}.dir/{s}{s}", .{ c.bdir, dir_prefix, target, rel, c.obj_ext });
    }

    fn newSet(c: *Ctx, name: []const u8, skip_suffixes: []const []const u8) *Set {
        const run = Run.create(c.b, c.fmt("stage {s}", .{name}));
        run.addArgs(&.{ c.o.python, c.helper, "stage" });
        for (skip_suffixes) |s| run.addArgs(&.{ "--skip-suffix", s });
        run.addArg("--");
        run.has_side_effects = true;
        const set = c.a.create(Set) catch @panic("OOM");
        set.* = .{ .name = name, .stage = run };
        return set;
    }

    fn addToSet(c: *Ctx, set: *Set, fixed: []const u8, cached: LazyPath) Obj {
        set.stage.addFileArg2(cached, .{ .prefix = c.fmt("{s}=", .{fixed}) });
        const obj: Obj = .{ .fixed = fixed, .cached = cached };
        set.items.append(c.a, obj) catch @panic("OOM");
        return obj;
    }

    /// A consumer of a set's fixed files: ordered after its stage step, re-run when any of its files' contents change.
    fn needs(c: *Ctx, run: *Run, set: *Set) void {
        _ = c;
        run.step.dependOn(&set.stage.step);
        for (set.items.items) |o| run.addFileInput(o.cached);
    }

    fn needObjs(c: *Ctx, run: *Run, set: *Set, objs: []const Obj) void {
        _ = c;
        run.step.dependOn(&set.stage.step);
        for (objs) |o| run.addFileInput(o.cached);
    }

    fn useCompiler(c: *Ctx, run: *Run) void {
        run.step.dependOn(&c.cc_stage.step);
        run.addFileInput(c.cc_bin);
        run.setEnvironmentVariable("P4PC_ZIG_CC", "1");
        run.setEnvironmentVariable("P4PC_ZIG_CC_WRAPPER", c.cc);
        run.setEnvironmentVariable("P4PC_BUILD_TOOLCHAIN_ID", c.toolchain_id);
    }

    fn inputs(c: *Ctx, run: *Run, rels: []const []const u8) void {
        for (rels) |rel| run.addFileInput(c.lp(c.src(rel)));
    }

    fn absInputs(c: *Ctx, run: *Run, paths: []const []const u8) void {
        for (paths) |p| run.addFileInput(c.lp(p));
    }

    /// `python tools/zig_build_step.py exec [--stamp S] [--depfile D --depfile-from-out] -- python [pre] <script>`; the
    /// caller appends the script's arguments.
    fn pyTool(c: *Ctx, name: []const u8, pre: []const []const u8, script_rel: []const u8, opts: struct { stamp: bool = false, depfile_out: bool = false }) *Run {
        const run = Run.create(c.b, name);
        run.addArgs(&.{ c.o.python, c.helper, "exec" });
        if (opts.stamp) {
            run.addArg("--stamp");
            _ = run.addOutputFileArg("stamp");
        }
        if (opts.depfile_out) {
            run.addArg("--depfile");
            _ = run.addDepFileOutputArg("deps.d");
            run.addArg("--depfile-from-out");
        }
        run.addArgs(&.{ "--", c.o.python });
        run.addArgs(pre);
        run.addArg(c.src(script_rel));
        run.addFileInput(c.lp(c.src(script_rel)));
        run.addFileInput(c.lp(c.helper));
        run.setName(name);
        return run;
    }

    fn hostArgs(c: *Ctx, run: *Run, args: []const HostArg) void {
        _ = c;
        for (args) |arg| switch (arg) {
            .str => |s| run.addArg(s),
            .file => |f| run.addFileArg(f),
        };
    }
};

fn lessString(_: void, a: []const u8, b: []const u8) bool {
    return std.mem.lessThan(u8, a, b);
}

fn joinWords(c: *Ctx, words: []const []const u8) []const u8 {
    return std.mem.join(c.a, " ", words) catch @panic("OOM");
}

const Unit = struct {
    rel: []const u8, // path below the mirror (src/...)
    abs: []const u8, // <gen>/decomp/<rel>
    flags: []const []const u8,
};

/// What a compile needs besides its source: the CMake target it belongs to, its compile line and launcher.
const CompileSpec = struct {
    target: []const u8,
    dir_prefix: []const u8 = "",
    lang: Lang = .c,
    defines: []const []const u8 = &.{},
    includes: []const []const u8 = &.{},
    system_includes: []const []const u8 = &.{},
    flags: []const []const u8 = &.{},
    launcher: []const []const u8 = &.{},
    launcher_inputs: []const []const u8 = &.{},
    after: []const *Step = &.{},
    file_inputs: []const LazyPath = &.{},
    /// the Zig objects' default target (cmake/zig/zig_object.py --default-target)
    zig_default_target: []const u8 = "",
};

fn langOf(path: []const u8) Lang {
    if (std.mem.endsWith(u8, path, ".zig")) return .zig;
    if (std.mem.endsWith(u8, path, ".cpp") or std.mem.endsWith(u8, path, ".cc")) return .cxx;
    if (std.mem.endsWith(u8, path, ".S") or std.mem.endsWith(u8, path, ".s")) return .asm_;
    return .c;
}

/// Compiles one source into `set` and records its compile command.
fn compile(c: *Ctx, set: *Set, source: []const u8, spec: CompileSpec) Obj {
    const lang = if (spec.lang == .c) langOf(source) else spec.lang;
    const fixed = c.objectPath(spec.dir_prefix, spec.target, source);
    const base = std.fs.path.basename(source);
    const rel_name = if (std.mem.startsWith(u8, source, c.root)) source[c.root.len + 1 ..] else source;
    const run = Run.create(c.b, c.fmt("compile {s} {s}", .{ spec.target, rel_name }));
    run.rename_step_with_output_arg = false;
    var defs: std.ArrayList([]const u8) = .empty;
    for (spec.defines) |d| defs.append(c.a, c.fmt("-D{s}", .{d})) catch @panic("OOM");
    var incs: std.ArrayList([]const u8) = .empty;
    for (spec.includes) |i| incs.append(c.a, c.fmt("-I{s}", .{i})) catch @panic("OOM");
    for (spec.system_includes) |i| incs.appendSlice(c.a, &.{ "-isystem", i }) catch @panic("OOM");
    var cached: LazyPath = undefined;
    switch (lang) {
        .zig => {
            run.addArgs(&.{ c.o.python, c.src("cmake/zig/zig_object.py"), "--zig", c.b.graph.zig_exe, "--default-target", spec.zig_default_target, "--optimize", c.o.zig_optimize, "--cache-dir", c.join(&.{ c.bdir, "zig-cache" }), "--global-cache-dir", c.join(&.{ c.bdir, "zig-global-cache" }) });
            if (c.android) run.addArgs(&.{ "--android-sysroot", c.o.android_sysroot.? });
            run.addArg("-o");
            cached = run.addOutputFileArg(c.fmt("{s}{s}", .{ base, c.obj_ext }));
            run.addArgs(&.{ "-c", source, "--" });
            run.addArgs(defs.items);
            run.addArgs(incs.items);
            run.addArgs(spec.flags);
            run.addArg("--depfile");
            _ = run.addDepFileOutputArg("deps.d");
            c.inputs(run, &.{"cmake/zig/zig_object.py"});
            run.setEnvironmentVariable("P4PC_BUILD_TOOLCHAIN_ID", c.toolchain_id);
        },
        .c, .cxx, .asm_ => {
            const compiler = if (lang == .cxx) c.cxx else c.cc;
            var args: std.ArrayList([]const u8) = .empty;
            args.appendSlice(c.a, spec.launcher) catch @panic("OOM");
            args.append(c.a, compiler) catch @panic("OOM");
            args.appendSlice(c.a, defs.items) catch @panic("OOM");
            args.appendSlice(c.a, incs.items) catch @panic("OOM");
            args.appendSlice(c.a, spec.flags) catch @panic("OOM");
            run.addArgs(args.items);
            run.addArgs(&.{ "-MD", "-MF" });
            _ = run.addDepFileOutputArg("deps.d");
            run.addArg("-o");
            cached = run.addOutputFileArg(c.fmt("{s}{s}", .{ base, c.obj_ext }));
            run.addArgs(&.{ "-c", source });
            c.useCompiler(run);
            c.inputs(run, spec.launcher_inputs);
            var entry = args;
            entry.appendSlice(c.a, &.{ "-MD", "-MF", c.fmt("{s}.d", .{fixed}), "-o", fixed, "-c", source }) catch @panic("OOM");
            c.compdb.append(c.a, .{ .file = source, .output = fixed, .arguments = entry.items }) catch @panic("OOM");
        },
    }
    run.addFileInput(c.lp(source));
    for (spec.file_inputs) |f| run.addFileInput(f);
    for (spec.after) |s| run.step.dependOn(s);
    const obj = c.addToSet(set, fixed, cached);
    set.items.items[set.items.items.len - 1].source = source;
    return .{ .fixed = obj.fixed, .cached = obj.cached, .source = source };
}

const Library = struct {
    name: []const u8,
    fixed: []const u8,
    cached: LazyPath,
    set: *Set,
    objects: []const Obj,
};

/// A static library (CMake add_library STATIC): `zig ar` of the objects, placed at <build>/<dir_prefix>lib<name>.a.
fn archive(c: *Ctx, name: []const u8, dir_prefix: []const u8, objs_set: *Set, objs: []const Obj, out_set: *Set) Library {
    const run = Run.create(c.b, c.fmt("archive {s}", .{name}));
    run.rename_step_with_output_arg = false;
    run.addArgs(&.{ c.b.graph.zig_exe, "ar", "qcs" });
    const cached = run.addOutputFileArg(c.fmt("lib{s}.a", .{name}));
    for (objs) |o| run.addArg(o.fixed);
    c.needObjs(run, objs_set, objs);
    const fixed = c.fmt("{s}/{s}lib{s}.a", .{ c.bdir, dir_prefix, name });
    _ = c.addToSet(out_set, fixed, cached);
    const lib: Library = .{ .name = name, .fixed = fixed, .cached = cached, .set = out_set, .objects = objs };
    c.libraries.put(c.a, name, lib) catch @panic("OOM");
    return lib;
}

fn useLibrary(c: *Ctx, run: *Run, lib: Library) void {
    _ = c;
    run.step.dependOn(&lib.set.stage.step);
    run.addFileInput(lib.cached);
}

// ---------------------------------------------------------------------------------------------------------------------
// Options and profile

/// Parse the graph's effective CLI profile without constructing the game graph.
pub fn readOptions(b: *Build) Options {
    const platform = b.option(Platform, "platform", "Target platform of this build directory (CMake: CMAKE_SYSTEM_NAME / toolchain file)") orelse .linux;
    return .{
        .platform = platform,
        .arch = b.option(Arch, "port64-arch", "Architecture of the 64-bit build: x86_64 or aarch64 (P4_PORT64_ARCH)") orelse .x86_64,
        .android_abi = b.option(AndroidAbi, "android-abi", "Android ABI (ANDROID_ABI)") orelse .@"arm64-v8a",
        .android_api = b.option([]const u8, "android-api", "Android API level of the target triple (ANDROID_PLATFORM)") orelse "24",
        .android_sysroot = b.option([]const u8, "android-sysroot", "NDK sysroot (CMAKE_SYSROOT): bionic headers and libraries"),
        .android_prefix = b.option([]const u8, "android-prefix", "ci/android/build_deps.sh output, per-ABI subdirectories (P4_ANDROID_PREFIX)") orelse "/android-cache/deps",
        .android_unwind = b.option([]const u8, "android-unwind", "The NDK's libunwind.a for the ABI (clang -print-file-name=libunwind.a)"),
        .decomp = b.option([]const u8, "decomp", "Persona4-Decompilation checkout (P4_DECOMP; default <root>/decomp)") orelse "",
        .retail = b.option([]const u8, "retail", "Optional retail SLUS_217.82 executable (P4_RETAIL)"),
        .libc32 = b.option([]const u8, "libc32", "32-bit libc for the undefined-name set (P4_LIBC32)") orelse "/usr/lib32/libc.so.6",
        .win32_prefix = b.option([]const u8, "win32-prefix", "SDL3 and FFmpeg for i686-w64-mingw32 (P4_WIN32_PREFIX)") orelse "/opt/p4win32",
        .win64_prefix = b.option([]const u8, "win64-prefix", "SDL3 and FFmpeg for x86_64-w64-mingw32 (P4_WIN64_PREFIX)") orelse "/opt/p4win64",
        .mingw32_lib_dir = b.option([]const u8, "mingw32-lib-dir", "mingw-w64 UCRT import/runtime libraries for i686 (libucrt.a, libmingwex.a, libmingw32.a, libwinpthread.a)"),
        .mingw64_lib_dir = b.option([]const u8, "mingw64-lib-dir", "mingw-w64 UCRT import/runtime libraries for x86_64 (libucrt.a ... libshell32.a)"),
        .linux64_prefix = b.option([]const u8, "linux64-prefix", "x86_64 Linux SDL3 and FFmpeg prefix of the player package (P4_LINUX64_PREFIX)"),
        .macos_prefix = b.option([]const u8, "macos-prefix", "ci/macos/build_deps.sh output: SDL3, FFmpeg and ANGLE (P4_MACOS_PREFIX)"),
        .macos_deployment = b.option([]const u8, "macos-deployment-target", "Minimum macOS version (CMAKE_OSX_DEPLOYMENT_TARGET)") orelse "12.0",
        .glibc = b.option([]const u8, "glibc", "glibc version of the Linux targets (the CI image's: Debian trixie)") orelse "2.41",
        .image_clang = b.option([]const u8, "image-clang", "Directory of the CI image's clang/clang++ (llvm-mingw): Linux x86_64 C++ then compiles and links with its GCC runtime as CMake does; empty (no such directory) keeps Zig's bundled libc++") orelse defaultImageClang(b),
        .opt = b.option([]const u8, "opt", "Optimisation level of every unit: O0, O1, O2, O3 (P4_OPT)") orelse "O2",
        .debug_info = b.option([]const u8, "debug-info", "Debug info of every unit: g, g1, or none (P4_DEBUG_INFO)") orelse "g1",
        .auto_init = b.option([]const u8, "auto-init", "clang -ftrivial-auto-var-init mode: pattern or zero (P4_AUTO_INIT)"),
        .ee_float = b.option(bool, "ee-float", "Rewrite add/sub/div/sqrt in the decomp mirror to the EE's arithmetic (P4_EE_FLOAT)") orelse true,
        .ee_float_ops = b.option([]const u8, "ee-float-ops", "Which rewrites tools/ee_float_cc.py applies (P4_EE_FLOAT_OPS)") orelse "div,sqrt,add",
        .rw_clean = b.option([]const u8, "rw-clean", "RenderWare clean subsystems to compile (config/rw_clean.json), comma-separated; empty: none (P4_RW_CLEAN; a lane's P4PC_RW_CLEAN is passed here)") orelse "",
        .port64 = b.option(bool, "port64", "Rewrite and compile the decomp mirror for the 64-bit model (P4_PORT64)") orelse true,
        .port64_opt = b.option([]const u8, "port64-opt", "Optimisation level of the 64-bit mirror (qualified P4_PORT64_OPT)") orelse "O2",
        .port64_link = b.option(bool, "port64-link", "Link the 64-bit executables (P4_PORT64_LINK)") orelse false,
        .port64_win = b.option(bool, "port64-win", "Also compile the decomp mirror for x86_64-w64-mingw32 (P4_PORT64_WIN; Linux x86_64 profile)"),
        .port64w_opt = b.option([]const u8, "port64w-opt", "Optimisation level of the Windows x64 mirror (qualified P4_PORT64W_OPT)") orelse "O2",
        .census_check = b.option(bool, "census-check", "Fail the 64-bit mirror when the rewriter's rule counts differ from the baseline (P4_CENSUS_CHECK)") orelse true,
        .gles = b.option(bool, "gles", "Default to the OpenGL ES 3.0 renderer path (P4PC_GLES)") orelse true,
        .egl_native = b.option(bool, "egl-native", "GL ES context through the port's EGL on a CAMetalLayer (P4PC_EGL_NATIVE)") orelse false,
        .mod_support = b.option(bool, "mod-support", "Compile opt-in typed mod entry guards (P4PC_MOD_SUPPORT)") orelse true,
        .luau = b.option(bool, "luau", "Build the standalone sandboxed Luau provider and tests (P4PC_LUAU)") orelse true,
        .zig_optimize = b.option([]const u8, "zig-optimize", "Zig build mode of the battle simulator: auto, debug, safe, fast, small (P4_ZIG_OPTIMIZE)") orelse "auto",
        .python = b.option([]const u8, "python", "Python 3 interpreter (P4_PYTHON)") orelse "python3",
        .llvm_nm = b.option([]const u8, "llvm-nm", "llvm-nm for the target objects (P4_CRT_NM)") orelse "llvm-nm",
        .llvm_objdump = b.option([]const u8, "llvm-objdump", "llvm-objdump for the target objects (P4_CRT_OBJDUMP)") orelse "llvm-objdump",
        .build_dir = b.option([]const u8, "build-dir", "The build tree (CMAKE_BINARY_DIR; default <root>/build/zig-<profile>)"),
        .android_phone_runner = b.option(bool, "android-phone-runner", "Build the separate headless adb-shell game runner (P4_ANDROID_PHONE_RUNNER)") orelse false,
    };
}

fn absolutePath(c: *Ctx, path: []const u8) []const u8 {
    if (std.fs.path.isAbsolute(path)) return path;
    const cwd = std.process.currentPathAlloc(c.io, c.a) catch |e| fatal("cannot read the current directory: {t}", .{e});
    return std.fs.path.resolveAlloc(c.a, &.{ cwd, path }) catch @panic("OOM");
}

fn checkPin(c: *Ctx) void {
    const pin = c.read(c.src("cmake/zig/pin.cmake"));
    const key = "set(P4_ZIG_VERSION \"";
    const at = std.mem.indexOf(u8, pin, key) orelse fatal("cmake/zig/pin.cmake has no P4_ZIG_VERSION", .{});
    const rest = pin[at + key.len ..];
    const end = std.mem.indexOfScalar(u8, rest, '"') orelse fatal("cmake/zig/pin.cmake: unterminated P4_ZIG_VERSION", .{});
    const pinned = rest[0..end];
    if (!std.mem.eql(u8, pinned, builtin.zig_version_string))
        fatal("this is Zig {s}, not the pinned {s} (cmake/zig/pin.cmake); install it with `sh ci/zig/fetch_zig.sh <dir>` and run <dir>/zig build", .{ builtin.zig_version_string, pinned });
}

fn checkChoice(name: []const u8, value: []const u8, allowed: []const []const u8) void {
    for (allowed) |a| if (std.mem.eql(u8, a, value)) return;
    fatal("-D{s}={s}: expected one of the CMake values", .{ name, value });
}

fn profileName(o: Options) []const u8 {
    return switch (o.platform) {
        .linux => if (o.arch == .x86_64) "linux" else "linux-aarch64",
        .windows => "windows",
        .android => switch (o.android_abi) {
            .@"arm64-v8a" => "android-arm64-v8a",
            .x86_64 => "android-x86_64",
            .x86 => "android-x86",
        },
        .macos => if (o.arch == .x86_64) "macos-x86_64" else "macos-aarch64",
    };
}

fn androidTriple(abi: AndroidAbi) []const u8 {
    return switch (abi) {
        .@"arm64-v8a" => "aarch64-linux-android",
        .x86_64 => "x86_64-linux-android",
        .x86 => "i686-linux-android",
    };
}

/// cmake/sources/050-platform-flags.cmake and 060-platform-libraries.cmake.
fn platformFlags(c: *Ctx) void {
    const o = c.o;
    // Zig's cc driver accepts the native LLD spelling, not --no-insert-timestamp.
    c.pe_repro = &.{ "-fuse-ld=lld", "-Wl,/Brepro" };
    var defines: std.ArrayList([]const u8) = .empty;
    switch (o.platform) {
        .windows => {
            c.win32 = true;
            c.has_i386 = true;
            c.obj_ext = ".obj";
            c.default_triple = "i686-w64-mingw32";
            c.base_c = &.{"--target=i686-w64-mingw32"};
            c.base_link = &.{"--target=i686-w64-mingw32"};
            c.target_flags = &.{ "-DP4_PC=1", "-mfpmath=sse", "-msse2", "-ffp-contract=off", "-D_FILE_OFFSET_BITS=64" };
            c.target_link_flags = c.pe_repro;
            defines.append(c.a, "_FILE_OFFSET_BITS=64") catch @panic("OOM");
            c.os_src = c.src("src/platform/os/os_win32.c");
        },
        .android => {
            c.android = true;
            c.android64 = o.android_abi != .x86;
            c.has_i386 = o.android_abi == .x86;
            const sysroot = o.android_sysroot orelse fatal("the Android profile needs -Dandroid-sysroot=<NDK sysroot> (CMAKE_SYSROOT)", .{});
            if (!c.exists(c.join(&.{ sysroot, "usr/include/stdio.h" })))
                fatal("the Android Zig objects need the NDK sysroot; found '{s}'", .{sysroot});
            c.default_triple = c.fmt("{s}{s}", .{ androidTriple(o.android_abi), o.android_api });
            // The NDK toolchain file's CMAKE_C_FLAGS (C, C++ and assembler only; not the Zig language's).
            const ndk: []const []const u8 = if (o.android_abi == .x86)
                &.{ "-DANDROID", "-fdata-sections", "-ffunction-sections", "-funwind-tables", "-fstack-protector-strong", "-D_FORTIFY_SOURCE=2", "-Wformat", "-Werror=format-security", "-mstackrealign" }
            else
                &.{ "-DANDROID", "-fdata-sections", "-ffunction-sections", "-funwind-tables", "-fstack-protector-strong", "-D_FORTIFY_SOURCE=2", "-Wformat", "-Werror=format-security" };
            c.base_c = c.list(&.{ &.{ c.fmt("--target={s}", .{c.default_triple}), c.fmt("--sysroot={s}", .{sysroot}) }, ndk });
            c.base_link = c.strs(&.{ c.fmt("--target={s}", .{c.default_triple}), c.fmt("--sysroot={s}", .{sysroot}) });
            c.target_flags = if (o.android_abi == .x86)
                &.{ "-DP4_PC=1", "-ffp-contract=off", "-D_FILE_OFFSET_BITS=64", "-fPIC", "-Wno-error=return-type", "-Wno-return-type", "-mfpmath=sse", "-msse2" }
            else
                &.{ "-DP4_PC=1", "-ffp-contract=off", "-D_FILE_OFFSET_BITS=64", "-fPIC", "-Wno-error=return-type", "-Wno-return-type" };
            c.target_link_flags = &.{};
            defines.append(c.a, "_FILE_OFFSET_BITS=64") catch @panic("OOM");
            c.os_src = c.src("src/platform/os/os_posix.c");
            // Zig's libc installation file for the NDK's bionic (zigbuild/zig_cc.zig reads it beside its bin directory).
            const tdir = androidTriple(o.android_abi);
            const crt = c.join(&.{ sysroot, "usr/lib", tdir, o.android_api });
            if (!c.exists(crt)) fatal("the NDK sysroot has no {s}", .{crt});
            c.writeIfChanged(c.join(&.{ c.bdir, "zig-libc", c.fmt("{s}{s}.txt", .{ tdir, o.android_api }) }), c.fmt(
                "include_dir={s}\nsys_include_dir={s}\ncrt_dir={s}\nmsvc_lib_dir=\nkernel32_lib_dir=\ncc_dir={s}\ndarwin_sdk_dir=\n",
                .{ c.join(&.{ sysroot, "usr/include" }), c.join(&.{ sysroot, "usr/include", tdir }), crt, crt },
            ));
            if (o.android_abi == .x86_64 or o.android_abi == .@"arm64-v8a") {
                // the i386 layout oracle of the rewrite (P4_PORT32_TARGET)
                const t32 = "i686-linux-android";
                const crt32 = c.join(&.{ sysroot, "usr/lib", t32, o.android_api });
                if (c.exists(crt32)) c.writeIfChanged(c.join(&.{ c.bdir, "zig-libc", c.fmt("{s}{s}.txt", .{ t32, o.android_api }) }), c.fmt(
                    "include_dir={s}\nsys_include_dir={s}\ncrt_dir={s}\nmsvc_lib_dir=\nkernel32_lib_dir=\ncc_dir={s}\ndarwin_sdk_dir=\n",
                    .{ c.join(&.{ sysroot, "usr/include" }), c.join(&.{ sysroot, "usr/include", t32 }), crt32, crt32 },
                ));
            }
        },
        .macos => {
            c.apple = true;
            const arch: []const u8 = if (o.arch == .aarch64) "arm64" else "x86_64";
            c.default_triple = c.fmt("{s}-apple-macos{s}", .{ arch, o.macos_deployment });
            c.base_c = c.strs(&.{c.fmt("--target={s}", .{c.default_triple})});
            c.base_link = c.base_c;
            c.target_flags = &.{ "-DP4_PC=1", "-ffp-contract=off", "-D_FILE_OFFSET_BITS=64", "-D_FORTIFY_SOURCE=0" };
            c.target_link_flags = &.{"-Wl,-U,_kwlnTaskDumpAll"};
            defines.append(c.a, "_FILE_OFFSET_BITS=64") catch @panic("OOM");
            c.os_src = c.src("src/platform/os/os_posix.c");
        },
        .linux => {
            c.os_src = c.src("src/platform/os/os_posix.c");
            defines.appendSlice(c.a, &.{ "_FILE_OFFSET_BITS=64", "_TIME_BITS=64" }) catch @panic("OOM");
            if (o.arch == .x86_64) {
                c.has_i386 = true;
                c.default_triple = "i386-linux-gnu";
                c.target_flags = &.{ "--target=i386-linux-gnu", "-m32", "-DP4_PC=1", "-mfpmath=sse", "-msse2", "-ffp-contract=off", "-D_FILE_OFFSET_BITS=64" };
                c.target_link_flags = &.{ "--target=i386-linux-gnu", "-m32", "-no-pie" };
            } else {
                // an AArch64 build directory builds only the 64-bit targets
                c.default_triple = "aarch64-linux-gnu";
                c.target_flags = &.{ "-DP4_PC=1", "-ffp-contract=off", "-D_FILE_OFFSET_BITS=64" };
                c.target_link_flags = &.{};
            }
        },
    }
    if (o.gles) defines.append(c.a, "P4PC_GL_API_ES_DEFAULT=1") catch @panic("OOM");
    if (o.egl_native) defines.append(c.a, "P4PC_GL_VIA_EGL=1") catch @panic("OOM");
    c.port_defines = defines.items;

    var opt: std.ArrayList([]const u8) = .empty;
    opt.appendSlice(c.a, &.{ c.fmt("-{s}", .{o.opt}), "-fno-strict-aliasing", "-fwrapv", "-fno-omit-frame-pointer" }) catch @panic("OOM");
    if (o.debug_info.len != 0 and !std.mem.eql(u8, o.debug_info, "none")) opt.append(c.a, c.fmt("-{s}", .{o.debug_info})) catch @panic("OOM");
    const nopic: []const []const u8 = if (c.android) &.{} else &.{"-fno-pic"};
    const auto_init: []const []const u8 = if (o.auto_init) |m| &.{ c.fmt("-ftrivial-auto-var-init={s}", .{m}), "-Wuninitialized", "-Wsometimes-uninitialized" } else &.{};
    // add_compile_options order: P4_TARGET_FLAGS, -O.., -g.., -include p4pc/compat.h (C only), -fno-pic, auto-init
    c.global_c = c.list(&.{ c.base_c, c.target_flags, opt.items, &.{ "-include", "p4pc/compat.h" }, nopic, auto_init });
    // the Zig language takes the add_compile_options too, but not the C-only include nor the toolchain's CMAKE_C_FLAGS
    const zig_base: []const []const u8 = if (c.base_c.len > 0) c.base_c[0..1] else &.{};
    c.global_zig = c.list(&.{ zig_base, c.target_flags, opt.items, nopic, auto_init });
    // 050's include/ then 052-capabilities.cmake's generated registry directory
    c.global_includes = c.strs(&.{ c.src("include"), c.join(&.{ c.gen, "capabilities" }) });
    c.crt_flags = if (c.win32) &.{"--coff32"} else &.{};
}

/// 060-platform-libraries.cmake: the i386/native libraries and the names the host provides for the tables and CRT audits.
fn platformLibraries(c: *Ctx) void {
    const o = c.o;
    switch (o.platform) {
        .windows => {
            const lib = c.join(&.{ o.win32_prefix, "lib" });
            c.sdl3 = c.findLibrary("SDL3 (P4_WIN32_PREFIX)", &.{ "libSDL3.dll.a", "libSDL3.a" }, &.{lib});
            c.avcodec = c.findLibrary("avcodec", &.{ "libavcodec.dll.a", "libavcodec.a" }, &.{lib});
            c.avutil = c.findLibrary("avutil", &.{ "libavutil.dll.a", "libavutil.a" }, &.{lib});
            c.swscale = c.findLibrary("swscale", &.{ "libswscale.dll.a", "libswscale.a" }, &.{lib});
            c.ffmpeg_inc = c.findPath("FFmpeg headers", "libavcodec/avcodec.h", &.{c.join(&.{ o.win32_prefix, "include" })});
            c.global_system_includes = c.strs(&.{c.join(&.{ o.win32_prefix, "include" })});
            const mingw = o.mingw32_lib_dir orelse fatal("the Windows profile needs -Dmingw32-lib-dir=<llvm-mingw>/i686-w64-mingw32/lib: the UCRT/mingw-w64 import libraries name the host functions for the tables (clang -print-file-name)", .{});
            var args: std.ArrayList(HostArg) = .empty;
            for ([_][]const u8{ "libucrt.a", "libmingwex.a", "libmingw32.a" }) |n|
                args.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .str = c.findLibrary("mingw-w64 runtime", &.{n}, &.{mingw}) } }) catch @panic("OOM");
            args.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .file = compilerRt(c, "x86-windows-gnu") } }) catch @panic("OOM");
            args.appendSlice(c.a, &.{
                .{ .str = "--host-lib" }, .{ .str = c.findLibrary("winpthreads", &.{"libwinpthread.a"}, &.{mingw}) },
                .{ .str = "--host-lib" }, .{ .str = c.sdl3 },
                .{ .str = "--host-lib" }, .{ .str = c.avcodec },
                .{ .str = "--host-lib" }, .{ .str = c.swscale },
                .{ .str = "--host-lib" }, .{ .str = c.avutil },
            }) catch @panic("OOM");
            c.host_lib_args = args;
            c.gen_tool_args = c.strs(&.{ "--coff", "--nm", o.llvm_nm, "--objdump", o.llvm_objdump });
            for (c.listDir(c.join(&.{ o.win32_prefix, "bin" }), ".dll")) |dll| c.runtime_files.append(c.a, c.join(&.{ o.win32_prefix, "bin", dll })) catch @panic("OOM");
        },
        .android => {
            const abi_prefix = c.join(&.{ o.android_prefix, @tagName(o.android_abi) });
            const lib = c.join(&.{ abi_prefix, "lib" });
            c.sdl3 = c.findLibrary("SDL3 (P4_ANDROID_PREFIX)", &.{"libSDL3.so"}, &.{lib});
            c.avcodec = c.findLibrary("avcodec", &.{"libavcodec.so"}, &.{lib});
            c.avutil = c.findLibrary("avutil", &.{"libavutil.so"}, &.{lib});
            c.swscale = c.findLibrary("swscale", &.{"libswscale.so"}, &.{lib});
            c.ffmpeg_inc = c.findPath("FFmpeg headers", "libavcodec/avcodec.h", &.{c.join(&.{ abi_prefix, "include" })});
            c.global_system_includes = c.strs(&.{c.join(&.{ abi_prefix, "include" })});
            const sysroot_lib = c.join(&.{ o.android_sysroot.?, "usr/lib", androidTriple(o.android_abi), o.android_api });
            var args: std.ArrayList(HostArg) = .empty;
            for ([_][]const u8{ "c", "m", "dl", "log" }) |n|
                args.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .str = c.findLibrary("bionic stub", &.{c.fmt("lib{s}.so", .{n})}, &.{sysroot_lib}) } }) catch @panic("OOM");
            args.appendSlice(c.a, &.{
                .{ .str = "--host-lib" },  .{ .str = c.sdl3 },          .{ .str = "--host-lib" }, .{ .str = c.avcodec },
                .{ .str = "--host-lib" },  .{ .str = c.swscale },       .{ .str = "--host-lib" }, .{ .str = c.avutil },
                .{ .str = "--host-name" }, .{ .str = "atexit" },        .{ .str = "--host-name" }, .{ .str = "at_quick_exit" },
                .{ .str = "--host-name" }, .{ .str = "pthread_atfork" }, .{ .str = "--host-lib" }, .{ .file = compilerRt(c, c.default_triple) },
            }) catch @panic("OOM");
            const unwind = o.android_unwind orelse fatal("the Android profile needs -Dandroid-unwind=<NDK>/.../lib/clang/<v>/lib/linux/<arch>/libunwind.a (clang -print-file-name=libunwind.a)", .{});
            if (!c.exists(unwind)) fatal("-Dandroid-unwind: {s} does not exist", .{unwind});
            args.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .str = unwind } }) catch @panic("OOM");
            c.host_lib_args = args;
            c.gen_tool_args = c.strs(&.{ "--pic", "--nm", o.llvm_nm, "--objdump", o.llvm_objdump });
        },
        .macos => {
            const prefix = o.macos_prefix orelse fatal("the macOS profile needs -Dmacos-prefix=<ci/macos/build_deps.sh prefix>", .{});
            const lib = c.join(&.{ prefix, "lib" });
            if (!c.exists(c.join(&.{ lib, "libSDL3.0.dylib" }))) fatal("-Dmacos-prefix is not a ci/macos/build_deps.sh prefix: '{s}'", .{prefix});
            c.sdl3 = c.findLibrary("SDL3", &.{"libSDL3.dylib"}, &.{lib});
            c.avcodec = c.findLibrary("avcodec", &.{"libavcodec.dylib"}, &.{lib});
            c.avutil = c.findLibrary("avutil", &.{"libavutil.dylib"}, &.{lib});
            c.swscale = c.findLibrary("swscale", &.{"libswscale.dylib"}, &.{lib});
            c.ffmpeg_inc = c.findPath("FFmpeg headers", "libavcodec/avcodec.h", &.{c.join(&.{ prefix, "include" })});
            c.global_system_includes = c.strs(&.{c.join(&.{ prefix, "include" })});
            // libSystem: the stub Zig links against (it ships the macOS SDK stubs), not an Xcode SDK's.
            var args: std.ArrayList(HostArg) = .empty;
            args.appendSlice(c.a, &.{
                .{ .str = "--macho" },    .{ .str = "--host-lib" }, .{ .file = LazyPath.zig_lib.path(c.b, "libc/darwin/libSystem.tbd") },
                .{ .str = "--host-lib" }, .{ .str = c.sdl3 },        .{ .str = "--host-lib" },
                .{ .str = c.avcodec },    .{ .str = "--host-lib" }, .{ .str = c.swscale },
                .{ .str = "--host-lib" }, .{ .str = c.avutil },
            }) catch @panic("OOM");
            c.host_lib_args = args;
            c.gen_tool_args = c.strs(&.{ "--macho", "--nm", o.llvm_nm, "--objdump", o.llvm_objdump });
            for ([_][]const u8{ "libEGL.dylib", "libGLESv2.dylib" }) |n| c.runtime_files.append(c.a, c.join(&.{ lib, n })) catch @panic("OOM");
        },
        .linux => {
            if (o.arch == .aarch64) {
                // the 64-bit block (350) finds this profile's libraries; the include directory is 060's i386 find_path
                c.ffmpeg_inc = c.findPath("FFmpeg headers", "libavcodec/avcodec.h", &.{ "/usr/include/i386-linux-gnu", "/usr/include" });
                return;
            }
            const dirs: []const []const u8 = &.{ "/usr/lib/i386-linux-gnu", "/usr/lib32" };
            c.sdl3 = c.findLibrary("i386 SDL3", &.{"libSDL3.so"}, dirs);
            const libm = c.findLibrary("i386 libm", &.{"libm.so.6"}, dirs);
            c.avcodec = c.findLibrary("i386 avcodec", &.{"libavcodec.so"}, dirs);
            c.avutil = c.findLibrary("i386 avutil", &.{"libavutil.so"}, dirs);
            c.swscale = c.findLibrary("i386 swscale", &.{"libswscale.so"}, dirs);
            c.ffmpeg_inc = c.findPath("i386 FFmpeg headers", "libavcodec/avcodec.h", &.{ "/usr/include/i386-linux-gnu", "/usr/include" });
            if (!c.exists(o.libc32)) fatal("-Dlibc32: {s} does not exist (P4_LIBC32)", .{o.libc32});
            var args: std.ArrayList(HostArg) = .empty;
            args.appendSlice(c.a, &.{
                .{ .str = "--host-lib" }, .{ .str = o.libc32 },   .{ .str = "--host-lib" }, .{ .str = libm },
                .{ .str = "--host-lib" }, .{ .str = c.sdl3 },     .{ .str = "--host-lib" }, .{ .str = c.avcodec },
                .{ .str = "--host-lib" }, .{ .str = c.swscale },  .{ .str = "--host-lib" }, .{ .str = c.avutil },
            }) catch @panic("OOM");
            c.host_lib_args = args;
            c.gen_tool_args = &.{};
        },
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Source lists (cmake/sources/100..305)

const p4port_sources_head = [_][]const u8{
    // 100-native-core
    "src/crt.c",                                   "src/image.c",                                "src/vu0.c",
    "src/platform/guest.c",                        "src/platform/ee_float.c",                    "src/platform/ps2_kernel.c",
    "src/platform/kernel_init.c",                  "src/platform/kernel_heap.c",                 "src/platform/kernel_thread.c",
    "src/platform/cri_asm_callees.c",              "src/platform/rx_heap_realloc.c",             "src/platform/ps2_libc.c",
    "src/platform/ee_bytes.c",                     "src/platform/ps2_dma.c",
    // 110-platform-shell
    "src/platform/config/p4cfg.c",                 "src/platform/config/game_mode.c",
    "src/platform/config/p4iso.c",                "src/platform/pad/ps2_pad.c",
    "src/platform/pad/pad_sdl.c",                  "src/platform/pad/walk_script.c",             "src/platform/nav_probe.c",
    "src/platform/gport.c",                        "src/platform/diag/p4diag.c",
    // 120-files
    "src/platform/files/vfs.c",                    "src/platform/files/iso9660.c",               "src/platform/files/sce_file.c",
    "src/platform/files/sce_cdvd.c",               "src/platform/files/sce_adxf.c",              "src/platform/files/adxf_partition.c",
    "src/platform/files/adxf_fsvr.c",              "src/platform/files/rofs_host.c",
    // 130-memcard
    "src/platform/memcard/ps2_memcard.c",
    // 140-sound
    "src/platform/sound/ps2_sndcom.c",             "src/platform/sound/p4snd_mixer.c",           "src/platform/sound/p4snd_reverb.c",
    "src/platform/sound/axrna.c",                  "src/platform/sound/sdr.c",                   "src/platform/sound/sdr_mem.c",
    "src/platform/sound/spu_pitch.c",              "src/platform/sound/hd_bank.c",               "src/platform/sound/vag.c",
    "src/platform/sound/movie_audio.c",
};
/// 150-renderware (P4_PORT_RW_SRCS): a selected RenderWare clean subsystem retires the ones its port_sources name.
const port_rw_sources = [_][]const u8{
    "src/platform/ps2_fpbit.c",                    "src/platform/rw_device_handle.c",            "src/platform/rw_frame_register.c",
    "src/platform/rw_plugin_constructors.c",       "src/platform/rw_freelist_thunks.c",          "src/platform/rw_device_register.c",
    "src/platform/gc_compat.c",                    "src/platform/rw_p2_stalac.c",                "src/platform/cdvd_device.c",
    "src/platform/rw_image_stream.c",
};
const p4port_sources_tail = [_][]const u8{
    // 160-rofs
    "src/platform/files/sce_rofs.c",
    // 170-game-driver
    "src/platform/battle_harness.c",               "src/platform/battle_trace.c",                "src/platform/shuffle_harness.c",
    "src/platform/menu_harness.c",                 "src/platform/newgame_gate.c",                "src/platform/title_driver.c",
    "src/platform/battle_driver.c",                "src/platform/profile.c",                     "src/platform/state_dump.c",
    // 185, 186
    "src/platform/capture_state.c",                "src/platform/content_digest.c",
    // 187-battle-sim (Zig)
    "src/platform/battle_sim/session.zig",         "src/platform/battle_sim/entry.zig",          "src/platform/battle_sim/graphics.zig",
    "src/platform/battle_sim/result_prompt.zig",   "src/platform/battle_sim/events.zig",         "src/platform/battle_sim/events_native.zig",
    "src/platform/battle_sim/policy.zig",          "src/platform/battle_sim/strategy.zig",       "src/platform/battle_sim/prepare.zig",
    "src/platform/battle_sim/prepare_native.zig",  "src/platform/battle_sim/live.zig",           "src/platform/battle_sim/live_entry.zig",
    "src/platform/battle_sim/observation.c",       "src/platform/battle_sim/expert.zig",
    // 190-settings
    "src/platform/config/p4settings.c",            "src/platform/config/p4settings_native.c",    "src/platform/config/mod_status_native.c",
    "src/platform/config/ui_overlay_native.c",     "src/platform/config/native_rows_native.c",   "src/platform/mods/mod_settings.c",
    // 200-soft-double
    "src/platform/ee_heap.c",                      "src/platform/ee_dfloat.c",                   "src/platform/ee_dlibm.c",
    "src/platform/ee_decimal.c",                   "src/platform/ee_dtoa.c",                     "src/platform/ee_printf.c",
    "src/platform/ee_strtod.c",                    "src/platform/ee_scanf.c",
    // 210-mods
    "src/platform/mods/mods.c",                    "src/platform/mods/mods_config.c",            "src/platform/mods/packs.c",
    "src/platform/mods/packs_config.c",            "src/platform/mods/package_registry.c",       "src/platform/mods/package_memory.c",
    "src/platform/mods/mod_setting_store.c",       "src/platform/mods/mod_setting_gate.c",       "src/platform/mods/lifecycle.c",
    "src/platform/mods/mod_artifacts.c",           "src/platform/mods/party_native.c",
    "src/platform/mods/persona_native.c",          "src/platform/mods/forge_native.c",
    "src/platform/mods/equipment_native.c",        "src/platform/mods/velvet_native.c",
    "src/platform/mods/compendium.c",
    "src/platform/mods/file_index.c",              "third_party/yxml/yxml.c",
    // 212-mod-services
    "src/platform/mods/services_native.c",
    // 215-mod-save
    "src/platform/memcard/mod_save.c",             "src/platform/memcard/mod_save_hash.c",       "src/platform/memcard/mod_save_format.c",
    "src/platform/memcard/save_transaction.c",     "src/platform/memcard/mod_save_custody.c",    "src/platform/memcard/mod_save_transport.c",
    "src/platform/memcard/mod_save_native.c",
    // 220-tables
    "src/platform/mods/game_tables.c",             "src/platform/mods/game_tables_prepare.c",    "src/platform/mods/game_table_inputs.c",
    "src/platform/mods/game_table_admission.c",    "src/platform/mods/game_table_golden_item.c",
    "src/platform/mods/game_table_derived.c",     "src/platform/mods/game_table_extents.c",
    "src/platform/mods/game_table_overlay.c",      "src/platform/mods/game_skills.c",            "src/platform/mods/game_items.c",
    "src/platform/mods/game_enemies.c",            "src/platform/mods/game_personas.c",          "src/platform/mods/game_items_save.c",
    "src/platform/mods/game_table_sources.c",      "src/platform/mods/game_table_refs.c",        "src/platform/mods/game_table_inputs_use.c",
    "src/platform/mods/game_skills_native.c",      "src/platform/mods/game_enemies_native.c",    "src/platform/mods/game_personas_native.c",
    "src/platform/mods/game_personas_save.c",      "src/platform/mods/game_skills_save.c",       "src/platform/mods/game_tables_packed.c",
    "src/platform/mods/game_table_derived_native.c", "src/platform/mods/game_elements_native.c", "src/platform/mods/game_elements_affinity.c",
    "src/platform/mods/game_elements_action.c",    "src/platform/mods/game_elements_route.c",    "src/platform/mods/game_stage.c",
    "src/platform/mods/game_damage_stage.c",       "src/platform/mods/game_recovery_stage.c",    "src/platform/mods/game_cost_stage.c",
    "src/platform/mods/game_chance_stage.c",       "src/platform/mods/game_wearoff_stage.c",     "src/platform/mods/game_buff_stage.c",
    "src/platform/mods/game_afflictions.c",        "src/platform/mods/game_reward_stage.c",      "src/platform/mods/game_unit_stage.c",
    "src/platform/mods/game_hit_stage.c",          "src/platform/mods/game_damage_cover.c",      "src/platform/mods/game_passive_grant.c",
    "src/platform/mods/game_side_ailment.c",       "src/platform/mods/game_side_ailment_battle.c", "src/platform/mods/game_items_native.c",
    "src/platform/mods/game_items_ui.c",           "src/platform/mods/game_items_equipment.c",   "src/platform/mods/game_items_battle.c",
    "src/platform/mods/game_items_list.c",         "src/platform/mods/game_items_shop.c",        "src/platform/mods/game_items_text.c",
    "src/platform/mods/game_table_builder.c",      "src/platform/mods/game_table_xml.c",         "src/platform/mods/game_table_manifest.c",
    "src/platform/mods/game_table_boot.c",         "src/platform/mods/game_table_delta.c",       "src/platform/mods/game_items_ui_boot.c",
    "src/platform/mods/game_data.c",               "src/platform/mods/game_data_catalog.c",      "src/platform/mods/game_data_edit.c",
    "src/platform/mods/game_data_edit_xml.c",      "src/platform/mods/game_data_session.c",
    "src/platform/mods/game_knowledge.c",          "src/platform/mods/game_knowledge_native.c",  "src/platform/mods/game_knowledge_save.c",
    "src/platform/mods/game_knowledge_boot.c",     "src/platform/mods/game_elements_ai_sources.c", "src/platform/mods/game_elements_ai_native.c",
    "src/platform/mods/game_elements_ai_vm.c",     "src/platform/mods/game_elements_ai_decision.c", "src/platform/mods/game_elements_ai_roles.c",
    "src/platform/mods/game_elements_ai_boot.c",   "src/platform/mods/game_text.c",              "src/platform/mods/game_text_native.c",
    "src/platform/mods/native_glyphs.c",           "src/platform/mods/native_glyphs_native.c",   "src/platform/mods/mod_resources.c",
    "src/platform/mods/mod_resources_native.c",
    // 221-elements-ui
    "src/platform/mods/game_elements_ui.c",        "src/platform/mods/game_elements_ui_native.c", "src/platform/mods/game_elements_ui_boot.c",
    // 222-battle-decide
    "src/platform/mods/battle_decide.c",
    // 222-golden
    "src/platform/golden/golden_formats.c",        "src/platform/golden/golden_source.c",        "src/platform/golden/golden_tables.c",
    "src/platform/golden/golden_producer.c",
    "src/platform/golden/golden_domains.c", "src/platform/golden/golden_exp.c", "src/platform/golden/golden_mode_declarations.c",
    "src/platform/golden/golden_mode_projection.c", "src/platform/golden/golden_mode_identity.c",
    "src/platform/golden/golden_mode_identity_source.c", "src/platform/golden/golden_mode_boot.c",
    "src/platform/golden/golden_mode_sources.c", "src/platform/golden/golden_mode_declare_builder.c",
    "src/platform/golden/golden_mode_cache_publish.c", "src/platform/golden/golden_mode_declare_cached.c",
    "src/platform/golden/golden_mode_outputs.c",
    "src/platform/golden/golden_inheritance.c", "src/platform/golden/golden_inheritance_source.c",
    "src/platform/golden/golden_recipe.c", "src/platform/golden/golden_cmm.c", "src/platform/golden/golden_cmm_source.c",
    "src/platform/golden/golden_skill_convert.c", "src/platform/golden/golden_item_convert.c",
    "src/platform/golden/golden_names.c", "src/platform/golden/golden_encounter_convert.c",
    "src/platform/golden/golden_persona_convert.c", "src/platform/golden/golden_enemy_convert.c",
    "src/platform/golden/golden_fusion_convert.c", "src/platform/golden/golden_fusion_source.c",
    "src/platform/golden/golden_extra_source.c", "src/platform/golden/golden_cache.c",
    "src/platform/golden/golden_table_cache.c",
    // 223-battle-present, 224-battle-notify
    "src/platform/mods/battle_present.c",          "src/platform/mods/battle_notify.c",
    // 230-lockstep
    "src/platform/lockstep.c",                     "src/platform/lockstep_draw.c",               "src/platform/lockstep_align.c",
    "src/platform/lockstep_disc.c",                "src/platform/lockstep_filer.c",              "src/platform/lockstep_online.c",
    "src/platform/lockstep_uninit.c",
    // 240-save-import
    "src/platform/memcard/save_import.c",          "src/platform/memcard/import_parse.c",        "src/platform/memcard/save_image.c",
    "src/platform/memcard/import_card.c",          "src/platform/memcard/import_entry.c",        "src/platform/memcard/import_max.c",
    "src/platform/memcard/import_settings.c",      "src/platform/memcard/import_cbs.c",          "third_party/miniz/miniz_tinfl.c",
};

const movie_sources = [_][]const u8{ "src/movie/sfmpvp.c", "src/movie/mpeg2_dec.c", "src/movie/movie_upload.c", "src/movie/mwply.c", "src/movie/sfd_demux.c", "src/movie/adx_dec.c", "src/platform/sfd_condition_guard.c" };
const png_sources = [_][]const u8{ "src/viewer/p4png.c", "third_party/miniz/miniz.c", "third_party/miniz/miniz_tdef.c" };
const render_sources = [_][]const u8{
    "src/render/p4gl_atomic.c", "src/render/p4gl_camera.c", "src/render/p4gl_context.c", "src/render/p4gl_device.c",   "src/render/p4gl_gl.c", "src/render/p4gl_rt.c",
    "src/render/p4gl_profile.c", "src/render/p4gl_im2d.c",  "src/render/p4gl_image.c",   "src/render/p4gl_im3d.c",     "src/render/p4gl_mdleffect.c",
    "src/render/p4gl_native.c", "src/render/p4gl_nativetex.c", "src/render/p4gl_ptank.c", "src/render/p4gl_raster.c",  "src/render/p4gl_yuv.c",
    "src/render/p4gl_readback.c", "src/render/p4gl_state.c", "src/render/p4tmx.c",
};
const content_sources = [_][]const u8{
    "src/render/p4texture_key.c",           "src/platform/p4texture_dump.c",  "src/platform/p4content_source.c", "src/platform/p4content.c",
    "src/platform/mods/p4content_image.c", "src/platform/mods/resource_png.c", "src/platform/mods/mod_resources_boot.c", "third_party/lodepng/lodepng.c",
};
const model_sources = [_][]const u8{
    "src/platform/models/p4model_cook.c", "src/platform/models/p4model_cache.c",
    "src/platform/models/p4model_stream.c", "src/platform/models/p4model_build.c", "src/platform/mods/mod_models_boot.c",
};
const package_boot_sources = [_][]const u8{ "src/platform/mods/package_boot.c", "src/platform/mods/failure_native.c", "src/platform/mods/failure_config.c", "src/platform/mods/boot_health.c" };
const lodepng_defines = [_][]const u8{ "LODEPNG_NO_COMPILE_DISK", "LODEPNG_NO_COMPILE_ENCODER", "LODEPNG_NO_COMPILE_ANCILLARY_CHUNKS", "LODEPNG_NO_COMPILE_ALLOCATORS" };
/// 290-cri-ee: the hand-written natives with EE float arithmetic.
const p4portee_sources = [_][]const u8{
    "src/cri/aixp.c",            "src/cri/acs.c",         "src/cri/cri_glue.c",            "src/platform/rw_skin_callees.c", "src/platform/rw_rtanim.c",
    "src/platform/rw_rtanim_record.c", "src/platform/rw_normalize2.c", "src/platform/rw_normalize3.c", "src/platform/rw_skytran.c", "src/platform/ee_libm.c",
};
/// Registration order of p4_register_package_boot_provider across the owner fragments (210, 220, 221, 285, 286, 305).
const package_boot_providers = [_][]const u8{
    "p4_mod_settings_boot_provider",       "p4_game_tables_sources_boot_provider", "p4_game_tables_items_ui_boot_provider",
    "p4_game_data_boot_provider",          "p4_game_knowledge_boot_provider",      "p4_game_elements_ai_boot_provider",
    "p4_game_elements_ui_boot_provider",   "p4_resources_images_boot_provider",
    "p4_resources_models_boot_provider",  "p4_package_tables_completion_boot_provider",
};
const mod_core_sources = [_][]const u8{ "src/platform/mods/hooks.c", "src/platform/mods/events.c", "src/platform/mods/failure.c", "src/platform/mods/numbers.c", "src/platform/mods/memory_policy.c" };
const mod_binding_units = [_][]const u8{ "lua_api", "lua_runtime", "lua_memory", "lua_hooks", "lua_events", "lua_input", "lua_files", "lua_settings", "lua_lifecycle", "lua_contract", "lua_artifacts", "lua_save", "failure_script" };
const mod_game_luau_sources = [_][]const u8{
    "src/platform/mods/lua_ui.c",            "src/platform/mods/lua_overlay.c",          "src/platform/mods/lua_rows.c",
    "src/platform/mods/lua_authorize.c",     "src/platform/mods/lua_stages.c",           "src/platform/mods/lua_passive.c",
    "src/platform/mods/lua_services.c",      "src/platform/mods/game_services.c",        "src/platform/mods/lua_battle_present.c",
    "src/platform/mods/battle_defeat.c",
    "src/platform/mods/lua_elements.c",      "src/platform/mods/lua_data.c",             "src/platform/mods/lua_data_edit.c",
    "src/platform/mods/lua_audio.c",         "src/platform/mods/lua_tables.c",           "src/platform/mods/lua_battle.c",
    "src/platform/mods/lua_battle_substitute.c", "src/platform/mods/battle_menu.c",      "src/platform/mods/lua_party.c",
    "src/platform/mods/battle_semantic.c",   "src/platform/mods/lua_semantic_events.c",  "src/platform/mods/lua_battle_decide.c",
    "src/platform/mods/game_inventory.c",    "src/platform/mods/game_party.c",           "src/platform/mods/lua_inventory.c",
    "src/platform/mods/lua_persona.c", "src/platform/mods/lua_story.c",
    "src/platform/mods/lua_forge.c", "src/platform/mods/lua_equipment.c", "src/platform/mods/lua_status.c",
};
const luau_components = [_][]const u8{ "Common", "Ast", "Bytecode", "Compiler", "VM", "Config", "Analysis" };

const crt_tools = [_][]const u8{
    "tools/crt_guard.py",            "tools/gen_port.py",            "config/host_symbols_allow.txt", "config/crt_host_adapters.json",
    "config/call_traps.txt",         "tools/crt_flow.py",            "config/port_symbols.txt",       "config/port_symbols_manual.txt",
    "config/native_names.txt",       "include/p4pc/libc_rename.h",
};
const tables_tools = [_][]const u8{
    "tools/gen_port.py",                "config/inactive_movie_decoder.json", "config/port_symbols.txt", "config/port_symbols_manual.txt",
    "config/port_owned.txt",            "config/native_names.txt",            "config/unbound_natives.txt", "include/p4pc/libc_rename.h",
    "config/call_traps.txt",            "config/retail_fnptr_words.txt",
};
const port64_tools = [_][]const u8{ "tools/port64/unit64.py", "tools/port64/gptr_rewrite.py" };

/// Whether the selected RenderWare clean subsystems retire this port transcription (rw_clean.json port_sources, 150/290).
fn rwCleanDropped(c: *Ctx, rel: []const u8) bool {
    for (c.rw_clean_drop) |d| if (std.mem.eql(u8, d, rel)) return true;
    return false;
}

/// The p4port / p4portee sources of a profile, in CMake SOURCES order (os source per platform, android coroutine, CRI glob,
/// movie, PNG/renderer, content packs, generated package boot table).
fn p4portSources(c: *Ctx, os_src: []const u8) []const []const u8 {
    var out: std.ArrayList([]const u8) = .empty;
    for (p4port_sources_head) |s| out.append(c.a, c.src(s)) catch @panic("OOM");
    for (port_rw_sources) |s| if (!rwCleanDropped(c, s)) out.append(c.a, c.src(s)) catch @panic("OOM");
    for (p4port_sources_tail) |s| out.append(c.a, c.src(s)) catch @panic("OOM");
    out.appendSlice(c.a, &.{ os_src, c.src("src/platform/os/restart.c") }) catch @panic("OOM");
    if (c.android) out.append(c.a, c.src("src/platform/os/coro_asm.c")) catch @panic("OOM");
    out.append(c.a, c.join(&.{ c.gen, "p4_build_id.c" })) catch @panic("OOM"); // 251-build-id
    for (c.listDir(c.src("src/cri"), ".c")) |name| {
        if (std.mem.eql(u8, name, "aixp.c") or std.mem.eql(u8, name, "acs.c") or std.mem.eql(u8, name, "cri_glue.c")) continue;
        out.append(c.a, c.src(c.fmt("src/cri/{s}", .{name}))) catch @panic("OOM");
    }
    for (movie_sources) |s| out.append(c.a, c.src(s)) catch @panic("OOM");
    for (png_sources) |s| out.append(c.a, c.src(s)) catch @panic("OOM");
    for (render_sources) |s| out.append(c.a, c.src(s)) catch @panic("OOM");
    for (content_sources) |s| out.append(c.a, c.src(s)) catch @panic("OOM");
    for (model_sources) |s| out.append(c.a, c.src(s)) catch @panic("OOM");
    out.append(c.a, c.join(&.{ c.gen, "package_boot_providers.c" })) catch @panic("OOM");
    for (package_boot_sources) |s| out.append(c.a, c.src(s)) catch @panic("OOM");
    return out.items;
}

/// 290-cri-ee P4_PORTEE_SRCS, less what a selected RenderWare clean subsystem retires.
fn p4porteeSources(c: *Ctx) []const []const u8 {
    var out: std.ArrayList([]const u8) = .empty;
    for (p4portee_sources) |s| if (!rwCleanDropped(c, s)) out.append(c.a, c.src(s)) catch @panic("OOM");
    return out.items;
}

fn p4portDefines(c: *Ctx) []const []const u8 {
    var d: std.ArrayList([]const u8) = .empty;
    d.appendSlice(c.a, c.port_defines) catch @panic("OOM");
    d.append(c.a, "P4_MOD_GAME_BINDINGS=1") catch @panic("OOM");
    if (c.o.mod_support) d.append(c.a, "P4_MOD_GUARDS=1") catch @panic("OOM");
    if (c.o.mod_support and c.o.luau) d.append(c.a, "P4_MOD_LUAU=1") catch @panic("OOM");
    return d.items;
}

/// INCLUDE_DIRECTORIES of p4port / p4portee: the directory's (include/, the prefix's SYSTEM include) then the target's.
fn p4portIncludes(c: *Ctx) []const []const u8 {
    return c.list(&.{ c.global_includes, &.{ c.src("src/movie"), c.src("src/render"), c.src("src/platform/files"), c.ffmpeg_inc, c.src("third_party") } });
}

// ---------------------------------------------------------------------------------------------------------------------
// The graph

pub fn build(b: *Build) *Graph {
    const o = readOptions(b);
    const io = b.graph.io;
    // configure reads and writes the build tree and runs gen_port.py units: it is repeated by every invocation, as
    // CMake's CONFIGURE_DEPENDS globbing re-checks at every build
    b.graph.poisonCache();
    const c = b.allocator.create(Ctx) catch @panic("OOM");
    c.* = .{
        .b = b,
        .a = b.allocator,
        .io = io,
        .o = o,
        .root = "",
        .bdir = "",
        .gen = "",
        .profile = profileName(o),
        .cc = "",
        .cxx = "",
        .cc_bin = undefined,
        .cc_stage = undefined,
        .toolchain_id = "",
        .mirror = undefined,
        .helper = "",
        .steps = .{
            .artifacts = b.step("artifacts", "The game executables and libraries of this profile"),
            .compile_only = b.step("compile-only", "Every object of the profile, including the compile-only 64-bit mirrors and their census"),
            .census = b.step("census", "The rewriter rule-count census of every 64-bit mirror (unit64.py report --census-baseline)"),
            .census_generate = b.step("census-generate", "The rewrite/ABI/census of the 64-bit mirror without object compilation (p4decomp64_generate)"),
            .crt = b.step("crt", "Every CRT audit of the profile (p4crt and the pre-link entry audits)"),
            .tables = b.step("tables", "The port tables of every model (gen_port.py tables)"),
            .tools = b.step("tools", "The build tools: the zig cc compiler path and the Luau provider tools"),
            .mirror = b.step("mirror", "Mirror the decomp (gen_port.py mirror)"),
            .check = b.step("build-gates", "Build-time gates only: compile-only, census, CRT and tables (no tests)"),
        },
    };
    c.root = absolutePath(c, b.root.toString(c.a) catch @panic("OOM"));
    c.helper = c.src("tools/zig_build_step.py");
    checkPin(c);
    checkChoice("opt", o.opt, &.{ "O0", "O1", "O2", "O3" });
    checkChoice("port64-opt", o.port64_opt, &.{ "O0", "O1", "O2", "O3" });
    checkChoice("port64w-opt", o.port64w_opt, &.{ "O0", "O1", "O2", "O3" });
    checkChoice("zig-optimize", o.zig_optimize, &.{ "auto", "debug", "safe", "fast", "small" });
    checkChoice("debug-info", o.debug_info, &.{ "g", "g1", "none" });
    if (o.auto_init) |m| checkChoice("auto-init", m, &.{ "pattern", "zero" });
    c.bdir = absolutePath(c, o.build_dir orelse c.join(&.{ c.root, "build", c.fmt("zig-{s}", .{c.profile}) }));
    c.gen = c.join(&.{ c.bdir, "gen" });
    c.o.decomp = absolutePath(c, if (o.decomp.len != 0) o.decomp else c.join(&.{ c.root, "decomp" }));
    if (!c.exists(c.join(&.{ c.o.decomp, "config/symbol_addrs.txt" }))) fatal("-Ddecomp is not a decomp checkout: {s}", .{c.o.decomp});
    if (o.retail) |r| if (!c.exists(r)) fatal("-Dretail is missing: '{s}'", .{r});

    toolchain(c);
    platformFlags(c);
    platformLibraries(c);
    generation(c);

    const port = p4portModel(c);
    c.pm = port;
    if (b.option([]const []const u8, "cross-emulator", "CMAKE_CROSSCOMPILING_EMULATOR words for the test executables (empty: none, as CMake)")) |e| c.emulator = e;
    nativeGraph(c, port);
    writeCompileCommands(c);
    const manifest = writeManifest(c, port);

    b.getInstallStep().dependOn(c.steps.artifacts);
    c.steps.check.dependOn(c.steps.compile_only);
    c.steps.check.dependOn(c.steps.census);
    c.steps.check.dependOn(c.steps.crt);
    c.steps.check.dependOn(c.steps.tables);

    const graph = b.allocator.create(Graph) catch @panic("OOM");
    graph.* = .{
        .profile = c.profile,
        .platform = o.platform,
        .root = c.root,
        .build_dir = c.bdir,
        .gen_dir = c.gen,
        .python = o.python,
        .cc = c.cc,
        .cxx = c.cxx,
        .cc_ready = &c.cc_stage.step,
        .toolchain_id = c.toolchain_id,
        .compile_commands = c.join(&.{ c.bdir, "compile_commands.json" }),
        .manifest = manifest,
        .steps = c.steps,
        .artifacts = &c.artifacts,
        .ctx = c,
        .runtime_files = c.runtime_files.items,
        .models = port.models,
        .mod_support = o.mod_support,
        .luau = o.luau,
        .census_check = o.census_check,
        .llvm_nm = o.llvm_nm,
        .llvm_objdump = o.llvm_objdump,
        .win32 = c.win32,
        .apple = c.apple,
        .android = c.android,
        .android64 = c.android64,
        .has_i386 = c.has_i386,
        .port64_arch = if (c.android) (if (o.android_abi == .@"arm64-v8a") Arch.aarch64 else Arch.x86_64) else o.arch,
        .port64 = port.port64,
        .port64w = port.port64w,
        .link64 = port.link64,
        .link64w = port.link64w,
        .g64 = if (port.port64) c.join(&.{ c.gen, "obj64" }) else "",
        .census_baseline = if (port.port64) port.census_baseline else "",
        .retail = o.retail,
        .g64_view = c.g64_view,
    };
    return graph;
}

/// The CI image's llvm-mingw clang directory when it exists on this host (a Linux x86_64 build); otherwise none.
fn defaultImageClang(b: *Build) []const u8 {
    if (builtin.os.tag != .linux) return "";
    const dir = "/opt/llvm-mingw/bin";
    std.Io.Dir.cwd().access(b.graph.io, dir ++ "/clang++", .{}) catch return "";
    return dir;
}

/// The compiler path every compile, link and tool takes: zigbuild/zig_cc.zig, built for the host by this Zig and placed at
/// <build>/bin/p4-zig-clang{,++}. Its identity (P4PC_BUILD_TOOLCHAIN_ID) is the Zig version and the wrapper's source.
fn toolchain(c: *Ctx) void {
    const b = c.b;
    const config = b.addOptions();
    config.addOption([]const u8, "zig_exe", b.graph.zig_exe);
    config.addOption([]const u8, "glibc", c.o.glibc);
    config.addOption([]const u8, "image_clang", c.o.image_clang);
    // The selected MinGW runtime and its headers are one toolchain. Zig's bundled
    // stdio.h has extra legacy process declarations, changing the Windows census.
    const mingw32_include = if (c.o.mingw32_lib_dir) |lib| c.join(&.{ lib, "..", "include" }) else "";
    const mingw64_include = if (c.o.mingw64_lib_dir) |lib| c.join(&.{ lib, "..", "include" }) else "";
    for ([_][]const u8{ mingw32_include, mingw64_include }) |include| {
        if (include.len != 0 and !c.exists(c.join(&.{ include, "stdio.h" })))
            fatal("the selected MinGW runtime needs its matching headers at {s}", .{include});
    }
    config.addOption([]const u8, "mingw32_include", mingw32_include);
    config.addOption([]const u8, "mingw64_include", mingw64_include);
    const module = b.createModule(.{
        .root_source_file = b.path("zigbuild/zig_cc.zig"),
        .target = b.graph.host,
        .optimize = .ReleaseSafe,
    });
    module.addOptions("p4_zig_cc_config", config);
    const exe = b.addExecutable(.{ .name = "p4-zig-clang", .root_module = module });
    c.cc_bin = exe.getEmittedBin();
    const exe_suffix: []const u8 = if (builtin.os.tag == .windows) ".exe" else "";
    c.cc = c.join(&.{ c.bdir, "bin", c.fmt("p4-zig-clang{s}", .{exe_suffix}) });
    c.cxx = c.join(&.{ c.bdir, "bin", c.fmt("p4-zig-clang++{s}", .{exe_suffix}) });
    const stage = Run.create(b, "stage p4-zig-clang");
    stage.addArgs(&.{ c.o.python, c.helper, "stage", "--" });
    stage.addFileArg2(c.cc_bin, .{ .prefix = c.fmt("{s}=", .{c.cc}) });
    stage.addFileArg2(c.cc_bin, .{ .prefix = c.fmt("{s}=", .{c.cxx}) });
    stage.has_side_effects = true;
    c.cc_stage = stage;
    c.steps.tools.dependOn(&stage.step);
    var digest: [32]u8 = undefined;
    var h = std.crypto.hash.sha2.Sha256.init(.{});
    h.update(builtin.zig_version_string);
    h.update(b.graph.zig_exe);
    h.update(c.read(c.src("zigbuild/zig_cc.zig")));
    h.update(c.o.glibc);
    h.update(c.o.image_clang);
    h.update(mingw32_include);
    h.update(mingw64_include);
    h.final(&digest);
    c.toolchain_id = c.fmt("zig-{s}-{s}", .{ builtin.zig_version_string, std.fmt.bytesToHex(digest, .lower)[0..16] });
}

/// The compiler runtime Zig links into every executable of `triple`, as an archive: the host-lib that names those helpers
/// for the tables/CRT audits (clang -print-libgcc-file-name in the CMake build).
fn compilerRt(c: *Ctx, triple: []const u8) LazyPath {
    const b = c.b;
    const query = Build.parseTargetQuery(.{ .arch_os_abi = zigTripleForRt(c, triple) }) catch fatal("unsupported compiler-rt target {s}", .{triple});
    const wf = b.addWriteFiles();
    const root = wf.add("p4_compiler_rt.zig", "// the compiler runtime archive of zigbuild/core.zig compilerRt\n");
    const lib = b.addLibrary(.{
        .linkage = .static,
        .name = c.fmt("p4_compiler_rt_{s}", .{std.mem.replaceOwned(u8, c.a, triple, ".", "_") catch @panic("OOM")}),
        .root_module = b.createModule(.{ .root_source_file = root, .target = b.resolveTargetQuery(query), .optimize = .ReleaseFast }),
    });
    lib.bundle_compiler_rt = true;
    return lib.getEmittedBin();
}

fn zigTripleForRt(c: *Ctx, triple: []const u8) []const u8 {
    if (std.mem.indexOf(u8, triple, "android") != null) {
        const arch = if (std.mem.startsWith(u8, triple, "aarch64")) "aarch64" else if (std.mem.startsWith(u8, triple, "x86_64")) "x86_64" else "x86";
        return c.fmt("{s}-linux-android", .{arch});
    }
    if (std.mem.startsWith(u8, triple, "aarch64-linux-gnu")) return c.fmt("aarch64-linux-gnu.{s}", .{c.o.glibc});
    return triple;
}

/// P4_RW_CLEAN_PORT_DROP of <gen>/rw_clean.cmake (gen_port.py units): the port transcriptions the selected RenderWare clean
/// subsystems retire, as repository-relative paths (150-renderware.cmake and 290-cri-ee.cmake remove them).
fn rwCleanPortDrop(c: *Ctx, text: []const u8) []const []const u8 {
    const key = "set(P4_RW_CLEAN_PORT_DROP \"";
    const at = std.mem.indexOf(u8, text, key) orelse fatal("{s}/rw_clean.cmake has no P4_RW_CLEAN_PORT_DROP", .{c.gen});
    const rest = text[at + key.len ..];
    const end = std.mem.indexOfScalar(u8, rest, '"') orelse fatal("{s}/rw_clean.cmake: unterminated P4_RW_CLEAN_PORT_DROP", .{c.gen});
    var out: std.ArrayList([]const u8) = .empty;
    var it = std.mem.splitScalar(u8, rest[0..end], ';');
    while (it.next()) |p| if (p.len != 0) out.append(c.a, p) catch @panic("OOM");
    return out.items;
}

/// 052-capabilities.cmake: <gen>/capabilities/p4_capabilities.inc, one P4_CAPABILITY("<key>", <revision>u) per
/// include/p4pc/p4.d/<key>.luau whose first line is `--!capability <key>@<revision>`, ordered by key; written only when the
/// registry changes. A fragment whose first line does not name its own key fails configure, as there.
fn capabilityRegistry(c: *Ctx) void {
    const dir = c.src("include/p4pc/p4.d");
    var entries: std.ArrayList([]const u8) = .empty;
    for (c.listDir(dir, ".luau")) |name| {
        const text = c.read(c.join(&.{ dir, name }));
        var lines = std.mem.splitScalar(u8, text, '\n');
        var first: []const u8 = "";
        while (lines.next()) |raw| {
            const line = std.mem.trimEnd(u8, raw, "\r");
            if (line.len != 0) {
                first = line;
                break;
            }
        }
        const prefix = "--!capability ";
        const at = std.mem.lastIndexOfScalar(u8, first, '@');
        const key = if (std.mem.startsWith(u8, first, prefix) and at != null and at.? > prefix.len) first[prefix.len..at.?] else "";
        const revision = if (at) |i| first[i + 1 ..] else "";
        if (!capabilityKey(key) or !capabilityRevision(revision) or !std.mem.eql(u8, c.fmt("{s}.luau", .{key}), name))
            fatal("include/p4pc/p4.d/{s}: the first line must be `--!capability <key>@<revision>` naming this file's key (python tools/p4_capabilities.py check)", .{name});
        entries.append(c.a, c.fmt("{s} {s}", .{ key, revision })) catch @panic("OOM");
    }
    std.mem.sort([]const u8, entries.items, {}, lessString);
    var s: std.ArrayList(u8) = .empty;
    s.appendSlice(c.a, "/* Generated from include/p4pc/p4.d/<key>.luau by cmake/sources/052-capabilities.cmake. */\n") catch @panic("OOM");
    for (entries.items) |e| {
        const space = std.mem.indexOfScalar(u8, e, ' ').?;
        s.appendSlice(c.a, c.fmt("P4_CAPABILITY(\"{s}\", {s}u)\n", .{ e[0..space], e[space + 1 ..] })) catch @panic("OOM");
    }
    c.writeIfChanged(c.join(&.{ c.gen, "capabilities", "p4_capabilities.inc" }), s.items);
}

/// [a-z][a-z0-9_-]*(\.[a-z][a-z0-9_-]*)*
fn capabilityKey(key: []const u8) bool {
    var it = std.mem.splitScalar(u8, key, '.');
    while (it.next()) |part| {
        if (part.len == 0 or !std.ascii.isLower(part[0])) return false;
        for (part[1..]) |ch| if (!(std.ascii.isLower(ch) or std.ascii.isDigit(ch) or ch == '_' or ch == '-')) return false;
    }
    return true;
}

/// [1-9][0-9]*
fn capabilityRevision(revision: []const u8) bool {
    if (revision.len == 0 or revision[0] == '0') return false;
    for (revision) |ch| if (!std.ascii.isDigit(ch)) return false;
    return true;
}

/// 030-generation-setup, 036/mods.cmake, 040-mirror, 187 (battle_sim_exports.h), 251 (p4_build_id.c), 305 (package_boot_providers.c).
fn generation(c: *Ctx) void {
    const b = c.b;
    const d = c.o.decomp;
    // 030: the RenderWare clean switch (empty: none, the decomp's RenderWare exactly as before)
    _ = b.run(&.{ c.o.python, c.src("tools/gen_port.py"), "units", "--decomp", d, "--build", c.gen, "--rw-clean", c.o.rw_clean });
    c.rw_clean_drop = rwCleanPortDrop(c, c.read(c.join(&.{ c.gen, "rw_clean.cmake" })));
    const text = c.read(c.join(&.{ c.gen, "units.txt" }));
    var lines = std.mem.splitScalar(u8, text, '\n');
    const prefix = c.join(&.{ c.gen, "decomp" });
    while (lines.next()) |raw| {
        const line = std.mem.trimEnd(u8, raw, "\r");
        if (line.len == 0) continue;
        const bar = std.mem.indexOfScalar(u8, line, '|') orelse fatal("units.txt: no '|' in {s}", .{line});
        const unit = line[0..bar];
        var flags: std.ArrayList([]const u8) = .empty;
        var it = std.mem.splitScalar(u8, line[bar + 1 ..], '\t');
        while (it.next()) |f| if (f.len != 0) flags.append(c.a, f) catch @panic("OOM");
        if (!std.mem.startsWith(u8, unit, prefix)) fatal("units.txt: {s} is not under {s}", .{ unit, prefix });
        c.units.append(c.a, .{ .rel = unit[prefix.len + 1 ..], .abs = unit, .flags = flags.items }) catch @panic("OOM");
    }
    if (c.units.items.len == 0) fatal("gen_port.py units listed no units", .{});

    // mods.cmake: the trusted Luau layout API as a C string
    {
        const luau_api = c.read(c.src("src/platform/mods/layout_api.luau"));
        var s: std.ArrayList(u8) = .empty;
        s.appendSlice(c.a, "static const char p4_mod_layout_api[] =\n\"") catch @panic("OOM");
        for (luau_api) |ch| switch (ch) {
            '\\' => s.appendSlice(c.a, "\\\\") catch @panic("OOM"),
            '"' => s.appendSlice(c.a, "\\\"") catch @panic("OOM"),
            '\r' => {},
            '\n' => s.appendSlice(c.a, "\\n\"\n\"") catch @panic("OOM"),
            else => s.append(c.a, ch) catch @panic("OOM"),
        };
        s.appendSlice(c.a, "\";\n") catch @panic("OOM");
        c.writeIfChanged(c.join(&.{ c.gen, "mod_luau_api.h" }), s.items);
    }
    capabilityRegistry(c);
    c.writeIfChanged(c.join(&.{ c.gen, "battle_sim_exports.h" }),
        \\/* Generated by cmake/sources/187-battle-sim.cmake: the C ABI of the battle simulator's Zig exports. */
        \\#include "p4pc/battle_sim.h"
        \\#include "p4pc/battle_events.h"
        \\#include "p4pc/battle_events_native.h"
        \\#include "p4pc/battle_strategy.h"
        \\#include "p4pc/battle_prepare.h"
        \\int p4_battle_sim_is_boss(void);
        \\int p4_battle_sim_opening_active(void);
        \\void *p4_battle_sim_decision_trace(void);
        \\struct bsim_observation;
        \\struct bsim_choice;
        \\int bsim_choose(const struct bsim_observation *, int, struct bsim_choice *);
        \\#include "p4pc/battle_live.h"
        \\
    );
    {
        var s: std.ArrayList(u8) = .empty;
        s.appendSlice(c.a, "#include \"p4pc/packages.h\"\n#include \"p4pc/package_boot.h\"\n") catch @panic("OOM");
        for (package_boot_providers) |p| s.appendSlice(c.a, c.fmt("extern const P4PackageHandler {s};\n", .{p})) catch @panic("OOM");
        s.appendSlice(c.a, "static const P4PackageHandler *const providers[] = {\n") catch @panic("OOM");
        for (package_boot_providers) |p| s.appendSlice(c.a, c.fmt("    &{s},\n", .{p})) catch @panic("OOM");
        s.appendSlice(c.a, "    0\n};\nint p4_package_boot_install(void)\n{\n    for (size_t i = 0; providers[i]; ++i)\n        if (p4_package_handler_register(providers[i])) return -1;\n    p4_package_boot_installed();\n    return 0;\n}\n") catch @panic("OOM");
        c.writeIfChanged(c.join(&.{ c.gen, "package_boot_providers.c" }), s.items);
    }
    // 251-build-id: tools/build_id.py reads the source tree's commit and dirty flag; its text depends on the tree alone (no time, host or path).
    c.writeIfChanged(c.join(&.{ c.gen, "p4_build_id.c" }), b.run(&.{ c.o.python, c.src("tools/build_id.py"), "--source", c.root }));
    if (c.win32) c.writeIfChanged(c.join(&.{ c.bdir, "ps2ram_layout.txt" }), ".ps2ram\n");

    // The mirror: every file of the decomp's src/ and include/ into gen/decomp (write-if-changed), the mod bridges, the
    // contracts. Always run: its consumers hash what it wrote.
    const mirror = Run.create(b, "gen_port mirror");
    mirror.addArgs(&.{ c.o.python, c.src("tools/gen_port.py"), "mirror", "--decomp", d, "--build", c.gen });
    if (c.o.mod_support) mirror.addArg("--mod-hooks");
    mirror.has_side_effects = true;
    c.mirror = mirror;
    c.steps.mirror.dependOn(&mirror.step);
}

// ---------------------------------------------------------------------------------------------------------------------

const PortModel = struct {
    models: []const Model,
    port64: bool,
    port64w: bool,
    link64: bool,
    link64w: bool,
    adapters32: bool,
    adapters64: bool,
    adapters64w: bool,
    x64_triple: []const u8 = "",
    x64_target_flags: []const []const u8 = &.{},
    x64_extra: []const []const u8 = &.{},
    x64_link: []const []const u8 = &.{},
    x64_gen_tools: []const []const u8 = &.{},
    x64_env: []const [2][]const u8 = &.{},
    census_baseline: []const u8 = "",
};

/// 080-port64-profile: which models this profile builds and their flags.
fn p4portModel(c: *Ctx) PortModel {
    const o = c.o;
    var m: PortModel = .{ .models = &.{}, .port64 = false, .port64w = false, .link64 = false, .link64w = false, .adapters32 = false, .adapters64 = false, .adapters64w = false };
    var models: std.ArrayList(Model) = .empty;
    if (c.has_i386) models.append(c.a, .i386) catch @panic("OOM");
    m.port64 = o.port64 and !c.win32 and (c.android64 or c.apple or o.platform == .linux);
    if (c.android64) {
        const t = c.default_triple;
        m.x64_triple = t;
        m.x64_target_flags = c.strs(&.{ c.fmt("--target={s}", .{t}), "-fsigned-char", "-ffp-contract=off", "-fPIC" });
        m.x64_extra = if (o.android_abi == .@"arm64-v8a") &.{"-mno-outline-atomics"} else &.{};
        m.x64_link = &.{};
        m.x64_gen_tools = if (o.android_abi == .@"arm64-v8a") c.strs(&.{ "--nm", o.llvm_nm, "--objdump", o.llvm_objdump, "--arm64" }) else c.strs(&.{ "--nm", o.llvm_nm, "--objdump", o.llvm_objdump });
        const sysroot = c.fmt("--sysroot={s}", .{o.android_sysroot.?});
        const x32 = c.fmt("i686-{s}", .{t[std.mem.indexOfScalar(u8, t, '-').? + 1 ..]});
        m.x64_env = c.a.dupe([2][]const u8, &.{
            .{ "P4_PORT64_TARGET", c.fmt("--target={s} -fsigned-char -ffp-contract=off {s}", .{ t, sysroot }) },
            .{ "P4_PORT64_PIC", "1" },
            .{ "P4_PORT32_TARGET", c.fmt("--target={s} {s}", .{ x32, sysroot }) },
        }) catch @panic("OOM");
        m.census_baseline = if (o.android_abi == .@"arm64-v8a") c.src("config/port64android_aarch64_census.txt") else c.src(c.fmt("config/port64android_{s}_census.txt", .{@tagName(o.android_abi)}));
    } else if (c.apple) {
        const t = c.default_triple;
        m.x64_triple = t;
        m.x64_target_flags = c.strs(&.{ c.fmt("--target={s}", .{t}), "-fsigned-char", "-ffp-contract=off", "-D_FORTIFY_SOURCE=0", "-fno-blocks", "-fno-stack-protector" });
        m.x64_link = c.strs(&.{ c.fmt("--target={s}", .{t}), "-Wl,-rpath,@executable_path/../Frameworks", c.fmt("-Wl,-rpath,{s}/lib", .{o.macos_prefix.?}), "-Wl,-headerpad_max_install_names" });
        m.x64_gen_tools = if (o.arch == .aarch64) c.strs(&.{ "--macho", "--nm", o.llvm_nm, "--objdump", o.llvm_objdump, "--arm64" }) else c.strs(&.{ "--macho", "--nm", o.llvm_nm, "--objdump", o.llvm_objdump });
        m.x64_env = c.a.dupe([2][]const u8, &.{.{ "P4_PORT64_TARGET", joinWords(c, m.x64_target_flags) }}) catch @panic("OOM");
        m.census_baseline = c.src(c.fmt("config/port64macos_{s}_census.txt", .{@tagName(o.arch)}));
    } else if (o.arch == .aarch64) {
        m.x64_triple = "aarch64-linux-gnu";
        m.x64_target_flags = &.{ "--target=aarch64-linux-gnu", "-fsigned-char", "-ffp-contract=off" };
        m.x64_extra = &.{"-mno-outline-atomics"};
        m.x64_link = &.{ "--target=aarch64-linux-gnu", "-no-pie" };
        m.x64_gen_tools = c.strs(&.{ "--arm64", "--nm", o.llvm_nm, "--objdump", o.llvm_objdump });
        m.x64_env = c.a.dupe([2][]const u8, &.{.{ "P4_PORT64_TARGET", joinWords(c, m.x64_target_flags) }}) catch @panic("OOM");
        m.census_baseline = c.src("config/port64a64_census.txt");
    } else {
        m.x64_triple = "x86_64-linux-gnu";
        m.x64_target_flags = &.{ "--target=x86_64-linux-gnu", "-m64" };
        m.x64_link = &.{ "--target=x86_64-linux-gnu", "-m64", "-no-pie" };
        m.census_baseline = c.src("config/port64_census.txt");
    }
    if (m.port64) models.append(c.a, .m64) catch @panic("OOM");
    // M6: the Windows x64 compile-only mirror, from a Linux x86_64 tree (the CMake default probe succeeds there)
    m.port64w = m.port64 and (o.port64_win orelse (o.platform == .linux and o.arch == .x86_64));
    if (m.port64w) models.append(c.a, .m64w) catch @panic("OOM");
    if (c.android64 and !(o.port64_link and m.port64)) fatal("the 64-bit Android build needs -Dport64-link=true and -Dport64=true", .{});
    if (!c.has_i386 and !m.port64) fatal("profile {s} builds only the 64-bit model: -Dport64=false leaves nothing to build", .{c.profile});
    m.link64 = o.port64_link and m.port64;
    m.link64w = o.port64_link and m.port64w and c.exists(c.join(&.{ o.win64_prefix, "lib" }));
    m.adapters32 = c.has_i386;
    m.adapters64 = m.link64;
    m.adapters64w = m.link64w;
    m.models = models.items;
    return m;
}

/// Flags of a host-adapter profile (p4_host_cxx_profiles callback ARGN).
fn adapterFlags(c: *Ctx, m: PortModel, model: Model) []const []const u8 {
    return switch (model) {
        .i386 => c.list(&.{ c.base_c, c.target_flags }),
        .m64 => c.list(&.{ c.base_c, m.x64_target_flags, m.x64_extra }),
        .m64w => &.{ "--target=x86_64-w64-mingw32", "-m64" },
    };
}

const Adapters = struct {
    libs: []const Library, // link order
    host_args: []const HostArg, // P4_MOD_HOST_ARGS<sfx>
    project: []const HostArg, // P4_MOD_CRT_PROJECT<sfx>
    project_lib: ?Library,
    cxx: bool,
    link_flags: []const []const u8,
    /// p4_host_cxx_runtime's INTERFACE link options on p4_luau<sfx> for the model (whether or not mod support links it)
    runtime_link: []const []const u8,
};

/// tools/luau/CMakeLists.txt p4_luau_profile and tools/mods/CMakeLists.txt p4_mod_profile for one model.
fn hostAdapters(c: *Ctx, m: PortModel, model: Model) Adapters {
    const o = c.o;
    const sfx = model.suffix();
    const set = objectSet(c, c.fmt("host adapter objects{s}", .{sfx}));
    const libset = c.newSet(c.fmt("host adapter libraries{s}", .{sfx}), &.{});
    c.steps.tools.dependOn(&libset.stage.step);
    const flags = adapterFlags(c, m, model);
    const luau_active = o.mod_support and o.luau;
    const windows_like = c.win32 or model == .m64w;
    var link_flags: std.ArrayList([]const u8) = .empty;
    if (o.platform == .linux or o.platform == .windows) link_flags.appendSlice(c.a, &.{ "-static-libstdc++", "-static-libgcc" }) catch @panic("OOM");
    if (windows_like) {
        link_flags.appendSlice(c.a, c.pe_repro) catch @panic("OOM");
        link_flags.appendSlice(c.a, &.{ "-Wl,-Bstatic", "-lwinpthread", "-Wl,-Bdynamic" }) catch @panic("OOM");
    }
    link_flags.append(c.a, if (c.apple) "-Wl,-dead_strip" else "-Wl,--gc-sections") catch @panic("OOM");
    link_flags.appendSlice(c.a, flags) catch @panic("OOM");

    const luau = c.src("third_party/luau");
    var luau_libs: std.StringHashMapUnmanaged(Library) = .empty;
    var luau_host: std.ArrayList(HostArg) = .empty;
    const sys = if (model == .m64w) &[_][]const u8{} else c.global_system_includes;
    if (o.luau) {
        const cxx_common = c.list(&.{ flags, &.{ "-O2", "-g1", "-fno-rtti", "-ffp-contract=off", "-ffunction-sections", "-fdata-sections", "-fPIC", "-std=gnu++17" } });
        const inc_of = struct {
            fn f(cc: *Ctx, l: []const u8, comp: []const u8) []const []const u8 {
                const deps: []const []const u8 = if (std.mem.eql(u8, comp, "Common")) &.{"Common"} else if (std.mem.eql(u8, comp, "Ast")) &.{ "Ast", "Common" } else if (std.mem.eql(u8, comp, "Bytecode")) &.{ "Bytecode", "Common" } else if (std.mem.eql(u8, comp, "Compiler")) &.{ "Compiler", "Ast", "Bytecode", "Common" } else if (std.mem.eql(u8, comp, "VM")) &.{ "VM", "Common" } else if (std.mem.eql(u8, comp, "Config")) &.{ "Config", "Ast", "Common", "Compiler", "Bytecode", "VM" } else &.{ "Analysis", "Ast", "Common", "Config", "Compiler", "Bytecode", "VM" };
                var out: std.ArrayList([]const u8) = .empty;
                for (deps) |dd| out.append(cc.a, cc.join(&.{ l, dd, "include" })) catch @panic("OOM");
                return out.items;
            }
        }.f;
        for (luau_components) |comp| {
            const name = c.fmt("p4_luau_{s}{s}", .{ comp, sfx });
            var defs: std.ArrayList([]const u8) = .empty;
            defs.append(c.a, "NDEBUG") catch @panic("OOM");
            const uses_vm = std.mem.eql(u8, comp, "VM") or std.mem.eql(u8, comp, "Config") or std.mem.eql(u8, comp, "Analysis");
            const uses_compiler = std.mem.eql(u8, comp, "Compiler") or std.mem.eql(u8, comp, "Config") or std.mem.eql(u8, comp, "Analysis");
            if (uses_vm) defs.appendSlice(c.a, &.{ "LUA_USE_LONGJMP=1", "LUA_API=extern \"C\"" }) catch @panic("OOM");
            if (uses_compiler) defs.append(c.a, "LUACODE_API=extern \"C\"") catch @panic("OOM");
            const extra: []const []const u8 = if (std.mem.eql(u8, comp, "VM")) &.{"-fno-exceptions"} else &.{};
            var objs: std.ArrayList(Obj) = .empty;
            for (c.listDir(c.join(&.{ luau, comp, "src" }), ".cpp")) |file| {
                objs.append(c.a, compile(c, set, c.join(&.{ luau, comp, "src", file }), .{
                    .target = name,
                    .dir_prefix = "tools/luau/",
                    .lang = .cxx,
                    .defines = defs.items,
                    .includes = c.list(&.{ c.global_includes, inc_of(c, luau, comp) }),
                    .system_includes = sys,
                    .flags = c.list(&.{ cxx_common, extra }),
                })) catch @panic("OOM");
            }
            luau_libs.put(c.a, comp, archive(c, name, "tools/luau/", set, objs.items, libset)) catch @panic("OOM");
        }
        // p4_luau: the C boundary
        const provider_obj = compile(c, set, c.src("src/platform/mods/luau_c.cpp"), .{
            .target = c.fmt("p4_luau{s}", .{sfx}),
            .dir_prefix = "tools/luau/",
            .lang = .cxx,
            .defines = &.{ "LUA_USE_LONGJMP=1", "LUA_API=extern \"C\"", "LUACODE_API=extern \"C\"" },
            .includes = c.list(&.{ c.global_includes, &.{c.src("include")}, inc_of(c, luau, "VM"), inc_of(c, luau, "Compiler") }),
            .system_includes = sys,
            .flags = cxx_common,
        });
        const provider = archive(c, c.fmt("p4_luau{s}", .{sfx}), "tools/luau/", set, &.{provider_obj}, libset);
        luau_libs.put(c.a, "provider", provider) catch @panic("OOM");
        luau_host.appendSlice(c.a, &.{
            .{ .str = "--host-lib" }, .{ .str = provider.fixed },
            .{ .str = "--host-lib" }, .{ .str = luau_libs.get("VM").?.fixed },
            .{ .str = "--host-lib" }, .{ .str = luau_libs.get("Compiler").?.fixed },
        }) catch @panic("OOM");
        luauTools(c, model, set, flags, link_flags.items, &luau_libs);
    }

    // p4_mod_core: the model definitions; p4_mod_luau and p4_mod_game (linked PUBLIC to it) also see P4_MOD_LUAU=1 and the
    // VM's C usage requirement LUA_USE_LONGJMP=1 (LUA_API/LUACODE_API are C++-only generator expressions)
    var core_defs: std.ArrayList([]const u8) = .empty;
    core_defs.appendSlice(c.a, c.port_defines) catch @panic("OOM");
    if (model != .i386) core_defs.appendSlice(c.a, &.{ "P4_GUEST64=1", "P4_GUEST_CHECK=1" }) catch @panic("OOM");
    var defs: std.ArrayList([]const u8) = .empty;
    defs.appendSlice(c.a, core_defs.items) catch @panic("OOM");
    if (luau_active) defs.appendSlice(c.a, &.{ "P4_MOD_LUAU=1", "LUA_USE_LONGJMP=1" }) catch @panic("OOM");
    const mod_flags = c.list(&.{ flags, &.{ "-O2", "-g1", "-fno-strict-aliasing", "-ffp-contract=off", "-ffunction-sections", "-fdata-sections", "-fPIC" } });
    const luau_inc: []const []const u8 = if (luau_active) c.list(&.{ &.{c.src("include")}, &.{ c.join(&.{ luau, "VM", "include" }), c.join(&.{ luau, "Common", "include" }), c.join(&.{ luau, "Compiler", "include" }), c.join(&.{ luau, "Ast", "include" }), c.join(&.{ luau, "Bytecode", "include" }) } }) else &.{};
    const incs = c.list(&.{ c.global_includes, &.{c.src("include")}, &.{c.gen}, luau_inc });
    const mod_spec = struct {
        fn f(cc: *Ctx, target: []const u8, d: []const []const u8, i: []const []const u8, s: []const []const u8, fl: []const []const u8) CompileSpec {
            return .{ .target = target, .dir_prefix = "tools/mods/", .defines = d, .includes = i, .system_includes = s, .flags = fl, .after = cc.a.dupe(*Step, &.{&cc.mirror.step}) catch @panic("OOM") };
        }
    }.f;
    var core_objs: std.ArrayList(Obj) = .empty;
    const core_luau_free = core_defs.items;
    var core_srcs: std.ArrayList([]const u8) = .empty;
    for (mod_core_sources) |s| core_srcs.append(c.a, c.src(s)) catch @panic("OOM");
    // Without entry guards the generated table is empty and only the core's hook lookup reads it: the overrides'
    // always-compiled seams pull authorize.c, whose failure path pulls hooks.c after the game archive, so it lives here.
    const bridges = c.join(&.{ c.gen, "mod_bridges.c" });
    if (!c.o.mod_support) core_srcs.append(c.a, bridges) catch @panic("OOM");
    for (core_srcs.items) |s| core_objs.append(c.a, compile(c, set, s, mod_spec(c, c.fmt("p4_mod_core{s}", .{sfx}), core_luau_free, incs, sys, mod_flags))) catch @panic("OOM");
    const core = archive(c, c.fmt("p4_mod_core{s}", .{sfx}), "tools/mods/", set, core_objs.items, libset);
    var game_objs: std.ArrayList(Obj) = .empty;
    var game_srcs: std.ArrayList([]const u8) = .empty;
    // field_host.c: the field trigger slot overrides/001747d0.c calls in every build
    if (c.o.mod_support) game_srcs.append(c.a, bridges) catch @panic("OOM");
    game_srcs.appendSlice(c.a, &.{ c.src("src/platform/mods/native_bindings.c"), c.src("src/platform/mods/authorize.c"), c.src("src/platform/mods/field_host.c") }) catch @panic("OOM");
    if (luau_active) for (mod_game_luau_sources) |s| game_srcs.append(c.a, c.src(s)) catch @panic("OOM");
    for (game_srcs.items) |s| game_objs.append(c.a, compile(c, set, s, mod_spec(c, c.fmt("p4_mod_game{s}", .{sfx}), defs.items, incs, sys, mod_flags))) catch @panic("OOM");
    const game = archive(c, c.fmt("p4_mod_game{s}", .{sfx}), "tools/mods/", set, game_objs.items, libset);
    var libs: std.ArrayList(Library) = .empty;
    libs.append(c.a, game) catch @panic("OOM");
    var host: std.ArrayList(HostArg) = .empty;
    if (luau_active) {
        var bind_objs: std.ArrayList(Obj) = .empty;
        for (mod_binding_units) |u| bind_objs.append(c.a, compile(c, set, c.src(c.fmt("src/platform/mods/{s}.c", .{u})), mod_spec(c, c.fmt("p4_mod_luau{s}", .{sfx}), defs.items, incs, sys, mod_flags))) catch @panic("OOM");
        const binding = archive(c, c.fmt("p4_mod_luau{s}", .{sfx}), "tools/mods/", set, bind_objs.items, libset);
        // CMake's order: the mods, the Luau provider and its libraries, then mod_core last.
        libs.appendSlice(c.a, &.{ binding, luau_libs.get("provider").?, luau_libs.get("VM").?, luau_libs.get("Compiler").?, luau_libs.get("Ast").?, luau_libs.get("Bytecode").?, luau_libs.get("Common").?, core }) catch @panic("OOM");
        host.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .str = binding.fixed } }) catch @panic("OOM");
    } else {
        libs.append(c.a, core) catch @panic("OOM");
    }
    host.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .str = core.fixed } }) catch @panic("OOM");
    if (luau_active) host.appendSlice(c.a, luau_host.items) catch @panic("OOM");
    const project = c.a.dupe(HostArg, &.{ .{ .str = "--project-lib" }, .{ .str = game.fixed } }) catch @panic("OOM");
    return .{
        .libs = libs.items,
        .host_args = host.items,
        .project = project,
        .project_lib = game,
        .cxx = adapterLinksCxx(luau_active),
        .link_flags = if (luau_active) link_flags.items else &.{},
        .runtime_link = link_flags.items,
    };
}

/// The Luau provider tools of one model (tools/luau/CMakeLists.txt): p4_luau_check, luau_sandbox_test, luau_script_runner,
/// and the size probes (EXCLUDE_FROM_ALL: their own step).
fn luauTools(c: *Ctx, model: Model, set: *Set, flags: []const []const u8, runtime_link: []const []const u8, libs: *std.StringHashMapUnmanaged(Library)) void {
    const sfx = model.suffix();
    const exe_suffix: []const u8 = if (c.win32 or model == .m64w) ".exe" else "";
    const luau = c.src("third_party/luau");
    const vm_inc = &[_][]const u8{ c.join(&.{ luau, "VM", "include" }), c.join(&.{ luau, "Common", "include" }), c.join(&.{ luau, "Compiler", "include" }), c.join(&.{ luau, "Ast", "include" }), c.join(&.{ luau, "Bytecode", "include" }) };
    const sys = if (model == .m64w) &[_][]const u8{} else c.global_system_includes;
    const provider_chain = [_][]const u8{ "provider", "Compiler", "VM", "Ast", "Bytecode", "Common" };
    const check_chain = [_][]const u8{ "Analysis", "Config", "provider", "Compiler", "VM", "Ast", "Bytecode", "Common" };
    const Tool = struct { name: []const u8, srcs: []const []const u8, cxx_src: bool, chain: []const []const u8, defs: []const []const u8, all: bool };
    const tools = [_]Tool{
        .{ .name = c.fmt("p4_luau_check{s}", .{sfx}), .srcs = &.{c.src("tools/luau_check.cpp")}, .cxx_src = true, .chain = &check_chain, .defs = &.{ "LUA_USE_LONGJMP=1", "LUA_API=extern \"C\"", "LUACODE_API=extern \"C\"" }, .all = true },
        .{ .name = c.fmt("luau_sandbox_test{s}", .{sfx}), .srcs = &.{c.src("tests/luau_sandbox_test.c")}, .cxx_src = false, .chain = &provider_chain, .defs = &.{"LUA_USE_LONGJMP=1"}, .all = true },
        .{ .name = c.fmt("luau_script_runner{s}", .{sfx}), .srcs = &.{ c.src("tests/luau_script_runner.c"), c.src("src/platform/content_digest.c") }, .cxx_src = false, .chain = &provider_chain, .defs = &.{"LUA_USE_LONGJMP=1"}, .all = true },
        .{ .name = c.fmt("p4_luau_size_probe{s}", .{sfx}), .srcs = &.{c.src("tools/luau_size_probe.c")}, .cxx_src = false, .chain = &provider_chain, .defs = &.{"LUA_USE_LONGJMP=1"}, .all = false },
        .{ .name = c.fmt("p4_luau_size_baseline{s}", .{sfx}), .srcs = &.{c.src("tools/luau_size_probe.c")}, .cxx_src = false, .chain = &provider_chain, .defs = &.{ "LUA_USE_LONGJMP=1", "P4_LUAU_SIZE_BASELINE=1" }, .all = false },
    };
    const size_step = c.b.step(c.fmt("luau-size{s}", .{sfx}), "The Luau provider size probe and baseline (EXCLUDE_FROM_ALL)");
    for (tools) |t| {
        var objs: std.ArrayList(Obj) = .empty;
        var tool_flags: std.ArrayList([]const u8) = .empty;
        tool_flags.appendSlice(c.a, flags) catch @panic("OOM");
        if (t.cxx_src) {
            tool_flags.appendSlice(c.a, &.{ "-O2", "-g1", "-fno-rtti", "-ffp-contract=off", "-ffunction-sections", "-fdata-sections", "-fPIE", "-std=gnu++17" }) catch @panic("OOM");
        } else {
            tool_flags.appendSlice(c.a, &.{ "-O2", "-g1" }) catch @panic("OOM");
        }
        var incs: std.ArrayList([]const u8) = .empty;
        incs.appendSlice(c.a, c.global_includes) catch @panic("OOM");
        incs.append(c.a, c.src("include")) catch @panic("OOM");
        incs.appendSlice(c.a, vm_inc) catch @panic("OOM");
        if (t.cxx_src) for ([_][]const u8{ "Analysis", "Config" }) |extra| incs.append(c.a, c.join(&.{ luau, extra, "include" })) catch @panic("OOM");
        for (t.srcs) |s| objs.append(c.a, compile(c, set, s, .{ .target = t.name, .dir_prefix = "tools/luau/", .defines = t.defs, .includes = incs.items, .system_includes = sys, .flags = tool_flags.items })) catch @panic("OOM");
        var lib_list: std.ArrayList(Library) = .empty;
        for (t.chain) |n| lib_list.append(c.a, libs.get(n).?) catch @panic("OOM");
        const art = link(c, .{
            .name = t.name,
            .out_rel = c.fmt("tools/luau/{s}{s}", .{ t.name, exe_suffix }),
            .model = model,
            .cxx = true,
            .objs_set = set,
            .objs = objs.items,
            .libs = lib_list.items,
            .link_flags = c.list(&.{ if (model == .m64w) &.{} else c.base_link, runtime_link }),
            .system_libs = &.{},
            .install_dir = "bin",
        });
        _ = c.artifacts.put(c.a, t.name, art) catch @panic("OOM");
        if (t.all) c.steps.tools.dependOn(art.step) else size_step.dependOn(art.step);
        manifestTarget(c, t.name, "executable", model, t.srcs, objs.items, lib_list.items, &.{}, true, art.path);
    }
}

const LinkSpec = struct {
    name: []const u8,
    out_rel: []const u8,
    model: Model,
    cxx: bool,
    shared: bool = false,
    objs_set: *Set,
    objs: []const Obj,
    extra_sets: []const *Set = &.{},
    extra_objs: []const []const Obj = &.{},
    libs: []const Library = &.{},
    link_flags: []const []const u8,
    system_libs: []const []const u8,
    after: []const *Step = &.{},
    map_file: ?[]const u8 = null,
    install_dir: []const u8 = "bin",
    /// test targets: not installed, and their step is `exe:<name>` (the bare name stays free for the test modules)
    test_target: bool = false,
    /// CMake's order for the 64-bit players: the project's units and port objects, then the target's own objects
    /// (loader, tables), then the libraries; the default puts the target's own objects first.
    own_objs_last: bool = false,
};

/// A final executable or shared library: `<compiler> <link flags> <objects> -o <out> <libraries>`, placed at
/// <build>/<out_rel> and installed.
fn link(c: *Ctx, spec: LinkSpec) Artifact {
    const out_set = c.newSet(spec.name, &.{});
    const run = Run.create(c.b, c.fmt("link {s}", .{spec.name}));
    run.rename_step_with_output_arg = false;
    if (spec.map_file != null and !c.apple) {
        const m = spec.map_file.?;
        run.addArgs(&.{ c.o.python, c.src("tools/zig_link_map.py"), c.b.graph.zig_exe });
        const dir = run.addOutputDirectoryArg2("map", .{ .suffix = c.fmt("/{s}", .{std.fs.path.basename(m)}) });
        out_set.stage.addDirectoryArg2(dir, .{ .prefix = c.fmt("{s}=", .{std.fs.path.dirname(m).?}) });
        run.addArg("--");
        run.addFileInput(c.lp(c.src("tools/zig_link_map.py")));
    }
    run.addArg(if (spec.cxx) c.cxx else c.cc);
    if (spec.shared) run.addArg("-shared");
    run.addArgs(spec.link_flags);
    if (spec.map_file) |m| if (c.apple) {
        // Zig bundles no ld64.lld; genuine Mach-O map unavailable.
        const dir = run.addOutputDirectoryArg2("map", .{ .prefix = "-Wl,-map,", .suffix = c.fmt("/{s}", .{std.fs.path.basename(m)}) });
        out_set.stage.addDirectoryArg2(dir, .{ .prefix = c.fmt("{s}=", .{std.fs.path.dirname(m).?}) });
    };
    if (!spec.own_objs_last) {
        for (spec.objs) |o| run.addArg(o.fixed);
        c.needObjs(run, spec.objs_set, spec.objs);
    }
    for (spec.extra_sets, spec.extra_objs) |s, objs| {
        for (objs) |o| run.addArg(o.fixed);
        c.needObjs(run, s, objs);
    }
    if (spec.own_objs_last) {
        for (spec.objs) |o| run.addArg(o.fixed);
        c.needObjs(run, spec.objs_set, spec.objs);
    }
    run.addArg("-o");
    // The mapped-link helper returns only after both final links agree. Its
    // directory owns the final executable AND PDB, never the discovery product.
    const mapped_coff = spec.map_file != null and
        (std.mem.endsWith(u8, spec.out_rel, ".exe") or std.mem.endsWith(u8, spec.out_rel, ".dll"));
    const product_dir: ?LazyPath = if (mapped_coff)
        run.addOutputDirectoryArg2("product", .{ .suffix = c.fmt("/{s}", .{std.fs.path.basename(spec.out_rel)}) })
    else
        null;
    const out = if (product_dir) |dir| dir.path(c.b, std.fs.path.basename(spec.out_rel)) else run.addOutputFileArg(std.fs.path.basename(spec.out_rel));
    for (spec.libs) |l| {
        run.addArg(l.fixed);
        useLibrary(c, run, l);
    }
    for (spec.system_libs) |l| {
        run.addArg(l);
        if (std.fs.path.isAbsolute(l)) run.addFileInput(c.lp(l));
    }
    for (spec.after) |s| run.step.dependOn(s);
    c.useCompiler(run);
    const fixed = c.join(&.{ c.bdir, spec.out_rel });
    _ = c.addToSet(out_set, fixed, out);
    const top = c.b.step(if (spec.test_target) c.fmt("exe:{s}", .{spec.name}) else spec.name, c.fmt("Build {s}", .{spec.out_rel}));
    top.dependOn(&out_set.stage.step);
    // The Windows x64 link of the image runtime mode is the CMake build's own: DWARF debug info, no PDB.
    var image_coff = false;
    for (spec.link_flags) |flag| image_coff = image_coff or (c.o.image_clang.len != 0 and std.mem.eql(u8, flag, "--target=x86_64-w64-mingw32"));
    if (product_dir) |dir| if (!image_coff) {
        const pdb_name = c.fmt("{s}.pdb", .{std.fs.path.stem(spec.out_rel)});
        const pdb = dir.path(c.b, pdb_name);
        _ = c.addToSet(out_set, c.join(&.{ std.fs.path.dirname(fixed).?, pdb_name }), pdb);
        if (!spec.test_target) {
            top.dependOn(&c.b.addInstallFileWithDir(pdb, .{ .custom = spec.install_dir }, pdb_name).step);
        }
    };
    if (!spec.test_target) {
        const install = c.b.addInstallFileWithDir(out, .{ .custom = spec.install_dir }, spec.out_rel[(if (std.mem.lastIndexOfScalar(u8, spec.out_rel, '/')) |i| i + 1 else 0)..]);
        top.dependOn(&install.step);
    }
    // `lazy`: <cache>/view/d -> the build-tree directory of the product, after the product is placed there
    const view = Run.create(c.b, c.fmt("view {s}", .{spec.name}));
    const dir = std.fs.path.dirname(fixed).?;
    view.addArgs(&.{ c.o.python, c.helper, "alias", dir });
    const view_out = view.addOutputDirectoryArg("view");
    view.addArg("d");
    view.addFileInput(c.lp(c.helper));
    view.addFileInput(out);
    view.step.dependOn(&out_set.stage.step);
    top.dependOn(&view.step);
    return .{ .path = fixed, .lazy = view_out.path(c.b, c.fmt("d/{s}", .{std.fs.path.basename(fixed)})), .cached = out, .step = top, .model = spec.model, .view = &view.step };
}

// ---------------------------------------------------------------------------------------------------------------------
// The models


fn filterNative(c: *Ctx, objs: []const Obj) []const Obj {
    var out: std.ArrayList(Obj) = .empty;
    for (objs) |o| {
        if (std.mem.indexOf(u8, o.fixed, "/src/platform/") != null or std.mem.indexOf(u8, o.fixed, "/third_party/") != null) continue;
        out.append(c.a, o) catch @panic("OOM");
    }
    return out.items;
}

fn writeList(c: *Ctx, path: []const u8, groups: []const []const Obj) void {
    var s: std.ArrayList(u8) = .empty;
    for (groups) |g| for (g) |o| {
        s.appendSlice(c.a, o.fixed) catch @panic("OOM");
        s.append(c.a, '\n') catch @panic("OOM");
    };
    if (s.items.len == 0) s.append(c.a, '\n') catch @panic("OOM");
    c.writeIfChanged(path, s.items);
}

/// The p4port and p4portee objects of a model: 100..305 for i386, 350/360 for the 64-bit copies.
fn portObjects(c: *Ctx, model: Model, m: PortModel, set: *Set) struct { port: []const Obj, portee: []const Obj } {
    const sfx = model.suffix();
    const os_src = if (model == .m64w) c.src("src/platform/os/os_win32.c") else c.os_src;
    var model_flags: []const []const u8 = &.{};
    var incs = p4portIncludes(c);
    var sys = c.global_system_includes;
    var zig_flags = c.global_zig;
    var c_flags = c.global_c;
    switch (model) {
        .i386 => {},
        .m64 => {
            model_flags = c.list(&.{ m.x64_target_flags, m.x64_extra, &.{"-DP4_GUEST_CHECK=1"} });
            if (c.o.platform == .linux) {
                const native_inc = if (c.o.linux64_prefix) |prefix| c.join(&.{ prefix, "include" }) else if (c.o.arch == .aarch64) "/usr/include/aarch64-linux-gnu" else "/usr/include/x86_64-linux-gnu";
                var native_incs: std.ArrayList([]const u8) = .empty;
                for (incs) |inc| native_incs.append(c.a, if (std.mem.eql(u8, inc, c.ffmpeg_inc)) native_inc else inc) catch @panic("OOM");
                incs = native_incs.items;
            }
        },
        .m64w => {
            model_flags = &.{ "--target=x86_64-w64-mingw32", "-m64", "-DP4_GUEST_CHECK=1" };
            // INCLUDE_DIRECTORIES without /usr/... ; the Windows x64 prefix's headers as SYSTEM
            var filtered: std.ArrayList([]const u8) = .empty;
            for (incs) |i| if (!std.mem.startsWith(u8, i, "/usr/")) filtered.append(c.a, i) catch @panic("OOM");
            incs = filtered.items;
            sys = &.{c.join(&.{ c.o.win64_prefix, "include" })};
        },
    }
    if (c.o.platform == .linux and model != .m64w) c_flags = c.list(&.{ c_flags, &.{ "-idirafter", "/usr/include" } });
    c_flags = c.list(&.{ c_flags, model_flags });
    zig_flags = c.list(&.{ zig_flags, model_flags });
    const defs = p4portDefines(c);
    const target = c.fmt("p4port{s}", .{sfx});
    var port: std.ArrayList(Obj) = .empty;
    for (p4portSources(c, os_src)) |s| {
        const is_zig = std.mem.endsWith(u8, s, ".zig");
        const is_lodepng = std.mem.endsWith(u8, s, "third_party/lodepng/lodepng.c");
        port.append(c.a, compile(c, set, s, .{
            .target = target,
            .defines = if (is_lodepng) c.list(&.{ defs, &lodepng_defines }) else defs,
            .includes = if (is_zig) c.list(&.{ incs, &.{c.src("src/platform")} }) else incs,
            .system_includes = sys,
            .flags = if (is_zig) zig_flags else c_flags,
            .zig_default_target = c.default_triple,
            .after = &.{&c.mirror.step},
        })) catch @panic("OOM");
    }
    const ee_launcher: []const []const u8 = if (c.o.ee_float) &.{ c.o.python, c.src("tools/ee_float_cc.py"), c.fmt("--modes={s}", .{c.o.ee_float_ops}) } else &.{};
    var portee: std.ArrayList(Obj) = .empty;
    const portee_incs = c.list(&.{ incs, &.{} });
    for (p4porteeSources(c)) |s| portee.append(c.a, compile(c, set, s, .{
        .target = c.fmt("p4portee{s}", .{sfx}),
        .defines = c.port_defines,
        .includes = portee_incs,
        .system_includes = sys,
        .flags = c_flags,
        .launcher = ee_launcher,
        .launcher_inputs = if (c.o.ee_float) &.{"tools/ee_float_cc.py"} else &.{},
    })) catch @panic("OOM");
    return .{ .port = port.items, .portee = portee.items };
}

fn crtGuard(c: *Ctx, name: []const u8, objects: []const []const Obj, objects_sets: []const *Set, definitions: []const []const Obj, args: []const HostArg, extra: []const HostArg, project: ?Library, after: []const *Step) *Run {
    const obj_file = c.join(&.{ c.gen, c.fmt("crt-{s}-objects.txt", .{name}) });
    const def_file = c.join(&.{ c.gen, c.fmt("crt-{s}-definitions.txt", .{name}) });
    writeList(c, obj_file, objects);
    writeList(c, def_file, definitions);
    const run = c.pyTool(c.fmt("crt {s}", .{name}), &.{}, "tools/crt_guard.py", .{});
    run.addArgs(&.{ "--objects-file", obj_file, "--definitions-file", def_file, "--decomp", c.o.decomp, "--build", c.gen, "--nm", c.o.llvm_nm, "--objdump", c.o.llvm_objdump, "--compile-commands", c.join(&.{ c.bdir, "compile_commands.json" }), "--report" });
    stageReport(c, run, c.fmt("crt-{s}.json", .{name}));
    c.hostArgs(run, args);
    c.hostArgs(run, extra);
    for (objects_sets) |s| c.needs(run, s);
    c.absInputs(run, &.{ obj_file, def_file, c.join(&.{ c.bdir, "compile_commands.json" }), c.join(&.{ c.o.decomp, "config/symbol_addrs.txt" }), c.join(&.{ c.o.decomp, "config/symbol_data_addrs.txt" }), c.join(&.{ c.o.decomp, "config/symbols_recovered.txt" }) });
    c.inputs(run, &crt_tools);
    if (project) |p| useLibrary(c, run, p);
    for (after) |s| run.step.dependOn(s);
    run.step.dependOn(&c.mirror.step);
    c.useCompiler(run);
    c.steps.crt.dependOn(&run.step);
    return run;
}

/// p4_crt_entry: the pre-link audit of a final target's own compile-command objects, before its link.
fn crtEntry(c: *Ctx, target: []const u8, definitions_file: []const u8, sets: []const *Set, args: []const HostArg) *Run {
    const run = c.pyTool(c.fmt("crt entry {s}", .{target}), &.{}, "tools/crt_guard.py", .{});
    run.addArgs(&.{ "--compile-commands", c.join(&.{ c.bdir, "compile_commands.json" }), "--compile-target", target, "--definitions-file", definitions_file, "--decomp", c.o.decomp, "--build", c.gen, "--nm", c.o.llvm_nm, "--objdump", c.o.llvm_objdump, "--report" });
    stageReport(c, run, c.fmt("crt-entry-{s}.json", .{target}));
    c.hostArgs(run, args);
    for (sets) |s| c.needs(run, s);
    c.absInputs(run, &.{ definitions_file, c.join(&.{ c.bdir, "compile_commands.json" }) });
    c.inputs(run, &crt_tools);
    run.step.dependOn(&c.mirror.step);
    c.useCompiler(run);
    c.steps.crt.dependOn(&run.step);
    return run;
}

/// The report of a CRT audit: written into the step's cached output directory (with its .tsv beside it) and placed in gen/.
fn stageReport(c: *Ctx, run: *Run, report: []const u8) void {
    const dir = run.addOutputDirectoryArg2("report", .{ .suffix = c.fmt("/{s}", .{report}) });
    const set = c.newSet(c.fmt("report {s}", .{report}), &.{});
    set.stage.addDirectoryArg2(dir, .{ .prefix = c.fmt("{s}=", .{c.gen}) });
    set.items.append(c.a, .{ .fixed = c.join(&.{ c.gen, report }), .cached = dir.path(c.b, report) }) catch @panic("OOM");
    c.steps.crt.dependOn(&set.stage.step);
}

/// A generated file of the cache placed at its fixed build-tree path: consumers order after `ready` and hash `cached`.
const Generated = struct {
    run: *Run,
    ready: *Step,
    cached: LazyPath,
};

fn generatedFile(c: *Ctx, run: *Run, fixed: []const u8, cached: LazyPath) Generated {
    const set = c.newSet(std.fs.path.basename(fixed), &.{});
    _ = c.addToSet(set, fixed, cached);
    return .{ .run = run, .ready = &set.stage.step, .cached = cached };
}

fn tables(c: *Ctx, name: []const u8, out: []const u8, objects_file: []const u8, flags: []const []const u8, host: []const HostArg, sets: []const *Set, after: []const *Step, abi: ?Generated) Generated {
    const run = c.pyTool(c.fmt("tables {s}", .{name}), &.{}, "tools/gen_port.py", .{});
    run.addArgs(&.{"tables"});
    run.addArgs(flags);
    run.addArgs(&.{ "--decomp", c.o.decomp, "--build", c.gen, "--objects-file", objects_file });
    if (c.o.retail) |r| run.addArgs(&.{ "--retail", r });
    run.addArg("--out");
    const cached = run.addOutputFileArg(std.fs.path.basename(out));
    if (abi) |a| {
        run.addArg("--abi");
        run.addFileArg(a.cached);
    }
    c.hostArgs(run, host);
    for (sets) |s| c.needs(run, s);
    c.absInputs(run, &.{ objects_file, c.join(&.{ c.gen, "units.txt" }) });
    c.inputs(run, &tables_tools);
    for (after) |s| run.step.dependOn(s);
    run.step.dependOn(&c.mirror.step);
    const gen = generatedFile(c, run, out, cached);
    c.steps.tables.dependOn(gen.ready);
    return gen;
}

/// The whole native graph of the profile.
fn nativeGraph(c: *Ctx, m: PortModel) void {
    // One stage set per model and per product: a target waits only for what it really consumes.
    const runtime = c.newSet("runtime files", &.{});
    for (c.runtime_files.items) |f| {
        const install = c.b.addInstallFileWithDir(c.lp(f), .bin, std.fs.path.basename(f));
        c.steps.artifacts.dependOn(&install.step);
        // p4_dlls / p4_macos_runtime: beside the executables of the build directory and of tests/
        _ = c.addToSet(runtime, c.join(&.{ c.bdir, std.fs.path.basename(f) }), c.lp(f));
        _ = c.addToSet(runtime, c.join(&.{ c.bdir, "tests", std.fs.path.basename(f) }), c.lp(f));
    }
    c.steps.artifacts.dependOn(&runtime.stage.step);
    c.runtime_ready = &runtime.stage.step;

    // host adapters per model (cmake/host-cxx.cmake p4_host_cxx_profiles order: "", 64, 64w)
    var ad32: ?Adapters = null;
    var ad64: ?Adapters = null;
    var ad64w: ?Adapters = null;
    if (m.adapters32) ad32 = hostAdapters(c, m, .i386);
    if (m.adapters64) ad64 = hostAdapters(c, m, .m64);
    if (m.adapters64w) ad64w = hostAdapters(c, m, .m64w);
    c.adapters = .{ ad32, ad64, ad64w };

    if (c.has_i386) i386Graph(c, m, ad32);
    if (m.port64) port64Graph(c, m, ad64, ad64w);
}

fn objectSet(c: *Ctx, name: []const u8) *Set {
    const set = c.newSet(name, &.{});
    c.steps.compile_only.dependOn(&set.stage.step);
    return set;
}

fn i386Graph(c: *Ctx, m: PortModel, ad: ?Adapters) void {
    const o = c.o;
    const objs = objectSet(c, "i386 objects");
    // 070-mirror-compile + 090-ee-launcher: p4decomp
    var decomp: std.ArrayList(Obj) = .empty;
    const modes = if (o.ee_float) o.ee_float_ops else "none";
    var launcher: std.ArrayList([]const u8) = .empty;
    launcher.appendSlice(c.a, &.{ o.python, c.src("tools/ee_float_cc.py"), c.fmt("--modes={s}", .{modes}), c.fmt("--printf-root={s}", .{c.join(&.{ c.gen, "decomp" })}) }) catch @panic("OOM");
    if (c.win32) launcher.append(c.a, "--printf-symbol-prefix=_") catch @panic("OOM");
    const decomp_flags = c.list(&.{ c.global_c, &.{ "-malign-double", "-Wno-int-conversion", "-Wno-incompatible-function-pointer-types", "-Wno-return-mismatch", "-DP4_DECOMP_UNIT=1", "-mno-implicit-float" } });
    for (c.units.items) |u| decomp.append(c.a, compile(c, objs, u.abs, .{
        .target = "p4decomp",
        .includes = c.global_includes,
        .system_includes = c.global_system_includes,
        .flags = c.list(&.{ decomp_flags, u.flags }),
        .launcher = launcher.items,
        .launcher_inputs = &.{ "tools/ee_float_cc.py", "tools/port64/printf_rewrite.py", "tools/port64/gptr_rewrite.py" },
        .after = &.{&c.mirror.step},
    })) catch @panic("OOM");
    const port = portObjects(c, .i386, m, objs);
    manifestTarget(c, "p4decomp", "object", .i386, &.{}, decomp.items, &.{}, &.{}, false, "");
    manifestTarget(c, "p4port", "object", .i386, &.{}, port.port, &.{}, &.{}, false, "");
    manifestTarget(c, "p4portee", "object", .i386, &.{}, port.portee, &.{}, &.{}, false, "");

    // 300-crt: p4crt_i386
    var host: std.ArrayList(HostArg) = .empty;
    host.appendSlice(c.a, c.host_lib_args.items) catch @panic("OOM");
    var crt_flags: std.ArrayList(HostArg) = .empty;
    for (c.crt_flags) |f| crt_flags.append(c.a, .{ .str = f }) catch @panic("OOM");
    if (ad) |a| host.appendSlice(c.a, a.host_args) catch @panic("OOM");
    var with_project: std.ArrayList(HostArg) = .empty;
    with_project.appendSlice(c.a, crt_flags.items) catch @panic("OOM");
    with_project.appendSlice(c.a, host.items) catch @panic("OOM");
    if (ad) |a| with_project.appendSlice(c.a, a.project) catch @panic("OOM");
    var ad_libs_for_inputs: []const Library = &.{};
    if (ad) |a| ad_libs_for_inputs = a.libs;
    const crt = crtGuard(c, "i386", &.{ decomp.items, filterNative(c, port.port), filterNative(c, port.portee) }, &.{objs}, &.{ port.port, port.portee }, with_project.items, &.{}, if (ad) |a| a.project_lib else null, &.{});
    for (ad_libs_for_inputs) |l| useLibrary(c, crt, l);

    // 310-tables
    const objects_file = c.join(&.{ c.gen, "objects.txt" });
    writeList(c, objects_file, &.{ decomp.items, port.port, port.portee });
    const tables_c = c.join(&.{ c.gen, "p4pc_tables.c" });
    var table_host: std.ArrayList(HostArg) = .empty;
    table_host.appendSlice(c.a, host.items) catch @panic("OOM");
    if (ad) |a| table_host.appendSlice(c.a, a.project) catch @panic("OOM");
    const tab = tables(c, "i386", tables_c, objects_file, c.gen_tool_args, table_host.items, &.{objs}, &.{&crt.step}, null);
    if (ad) |a| for (a.libs) |l| useLibrary(c, tab.run, l);
    c.images[@intFromEnum(Model.i386)] = .{
        .decomp = decomp.items,
        .decomp_set = objs,
        .port = port.port,
        .portee = port.portee,
        .port_set = objs,
        .tables = tab,
        .tables_c = tables_c,
        .ad = ad,
        .sdl3 = c.sdl3,
        .avcodec = c.avcodec,
        .avutil = c.avutil,
        .swscale = c.swscale,
        .after = &.{},
    };

    // final targets: p4pc / p4pc_viewer (330, 370), or Android x86's libmain.so (320)
    const port_defs = c.port_defines;
    const final_flags = c.list(&.{ c.global_c, &.{"-fno-builtin"} });
    const exe_sfx: []const u8 = if (c.win32) ".exe" else "";
    const ffmpeg_libs = &[_][]const u8{ c.avcodec, c.swscale, c.avutil };
    var crt_entry_args: std.ArrayList(HostArg) = .empty;
    crt_entry_args.appendSlice(c.a, crt_flags.items) catch @panic("OOM");
    crt_entry_args.appendSlice(c.a, host.items) catch @panic("OOM");

    const Final = struct { name: []const u8, srcs: []const []const u8, viewer_main: bool, includes_files: bool };
    const finals: []const Final = if (c.android) &.{
        .{ .name = "main", .srcs = &.{ c.src("src/loader.c"), c.src("src/viewer/p4pc_viewer.c"), c.src("src/platform/android/android_main.c"), tables_c }, .viewer_main = true, .includes_files = true },
    } else &.{
        .{ .name = "p4pc", .srcs = &.{ c.src("src/loader.c"), tables_c }, .viewer_main = false, .includes_files = false },
        .{ .name = "p4pc_viewer", .srcs = &.{ c.src("src/viewer/p4pc_viewer.c"), tables_c }, .viewer_main = false, .includes_files = true },
    };
    for (finals) |f| {
        const own = c.newSet(f.name, &.{});
        var own_objs: std.ArrayList(Obj) = .empty;
        var srcs: std.ArrayList([]const u8) = .empty;
        srcs.appendSlice(c.a, f.srcs) catch @panic("OOM");
        if (c.win32) srcs.append(c.a, c.src("src/platform/os/win32_ps2ram.S")) catch @panic("OOM");
        for (srcs.items) |s| {
            const is_asm = std.mem.endsWith(u8, s, ".S");
            var defs: std.ArrayList([]const u8) = .empty;
            defs.appendSlice(c.a, port_defs) catch @panic("OOM");
            if (f.viewer_main and std.mem.endsWith(u8, s, "p4pc_viewer.c")) defs.append(c.a, "main=p4pc_viewer_main") catch @panic("OOM");
            const incs = if (f.includes_files) c.list(&.{ c.global_includes, &.{c.src("src/platform/files")} }) else c.global_includes;
            const flags = if (is_asm) c.list(&.{ c.base_c, c.target_flags, removeCompat(c, c.global_c[c.base_c.len + c.target_flags.len ..]), &.{"-fno-builtin"} }) else final_flags;
            own_objs.append(c.a, compile(c, own, s, .{
                .target = f.name,
                .defines = defs.items,
                .includes = incs,
                .system_includes = c.global_system_includes,
                .flags = flags,
                .after = if (std.mem.eql(u8, s, tables_c)) &.{ tab.ready, &c.mirror.step } else &.{&c.mirror.step},
                .file_inputs = if (std.mem.eql(u8, s, tables_c)) &.{tab.cached} else &.{},
            })) catch @panic("OOM");
        }
        // p4_crt_entry before the link
        const entry = crtEntry(c, f.name, objects_file, &.{ own, objs }, crt_entry_args.items);
        var link_flags: std.ArrayList([]const u8) = .empty;
        link_flags.appendSlice(c.a, c.base_link) catch @panic("OOM");
        link_flags.appendSlice(c.a, c.target_link_flags) catch @panic("OOM");
        var system: std.ArrayList([]const u8) = .empty;
        if (c.android) {
            link_flags.appendSlice(c.a, &.{ "-Wl,-z,max-page-size=16384", "-Wl,--build-id=none", "-Wl,--wrap=exit" }) catch @panic("OOM");
            system.appendSlice(c.a, &.{ "-lm", "-ldl", "-llog", "-landroid", c.sdl3 }) catch @panic("OOM");
            system.appendSlice(c.a, ffmpeg_libs) catch @panic("OOM");
        } else {
            system.appendSlice(c.a, &.{ "-lm", c.sdl3 }) catch @panic("OOM");
            system.appendSlice(c.a, ffmpeg_libs) catch @panic("OOM");
        }
        if (c.win32) {
            // 330/370: a GUI subsystem keeps Windows from allocating a visible console for an automated launch (the ordinary
            // main() startup and redirected stdio stay); then p4_ps2_exe: the PS2 RAM image layout;
            // link_libraries(-Wl,-Bstatic winpthread -Wl,-Bdynamic)
            link_flags.appendSlice(c.a, &.{ "-Wl,--subsystem,windows", "-Wl,-e,mainCRTStartup" }) catch @panic("OOM");
            link_flags.appendSlice(c.a, &.{ "-Wl,--image-base=0xF0000", "-Wl,--section-alignment=0x10000", "-Wl,--disable-dynamicbase", "-Wl,--disable-reloc-section", "-Wl,--stack=8388608", c.fmt("-Wl,--Xlink=/sectionlayout:{s}", .{c.join(&.{ c.bdir, "ps2ram_layout.txt" })}) }) catch @panic("OOM");
            system.appendSlice(c.a, &.{ "-Wl,-Bstatic", "-lwinpthread", "-Wl,-Bdynamic" }) catch @panic("OOM");
        }
        var adapter_libs: []const Library = &.{};
        var cxx = false;
        var adapter_names: []const []const u8 = &.{};
        if (ad) |a| {
            adapter_libs = a.libs;
            cxx = a.cxx;
            link_flags.appendSlice(c.a, a.link_flags) catch @panic("OOM");
            adapter_names = adapterLibraries(.i386, true, o.mod_support and o.luau);
        }
        const art = link(c, .{
            .name = f.name,
            .out_rel = if (c.android) "libmain.so" else c.fmt("{s}{s}", .{ f.name, exe_sfx }),
            .model = .i386,
            .cxx = cxx,
            .shared = c.android,
            .objs_set = own,
            .objs = own_objs.items,
            .extra_sets = &.{ objs, objs, objs },
            .extra_objs = &.{ decomp.items, port.port, port.portee },
            .libs = adapter_libs,
            .link_flags = link_flags.items,
            .system_libs = system.items,
            .after = &.{&entry.step},
            .map_file = c.join(&.{ c.gen, c.fmt("crt-link-{s}.map", .{f.name}) }),
        });
        _ = c.artifacts.put(c.a, f.name, art) catch @panic("OOM");
        if (c.runtime_ready) |s| art.view.dependOn(s);
        c.steps.artifacts.dependOn(art.step);
        manifestTarget(c, f.name, if (c.android) "shared_library" else "executable", .i386, srcs.items, own_objs.items, adapter_libs, adapter_names, cxx, art.path);
    }
    if (c.android) return; // 320: the Android project ends here

    // 380-movie-player: p4movie + p4pc_sfdview
    const movie = c.newSet("p4movie", &.{});
    var movie_objs: std.ArrayList(Obj) = .empty;
    for ([_][]const u8{ "src/movie/sfd_demux.c", "src/movie/mpeg2_dec.c", "src/movie/adx_dec.c" }) |s| movie_objs.append(c.a, compile(c, movie, c.src(s), .{
        .target = "p4movie",
        .defines = c.port_defines,
        .includes = c.list(&.{ c.global_includes, &.{ c.src("src/platform/files"), c.ffmpeg_inc } }),
        .system_includes = c.global_system_includes,
        .flags = c.global_c,
    })) catch @panic("OOM");
    const defs_file = c.join(&.{ c.gen, "crt-sfdview-definitions.txt" });
    writeList(c, defs_file, &.{movie_objs.items});
    const sfd = c.newSet("p4pc_sfdview", &.{});
    var sfd_objs: std.ArrayList(Obj) = .empty;
    var sfd_srcs: std.ArrayList([]const u8) = .empty;
    sfd_srcs.append(c.a, c.src("src/movie/p4pc_sfdview.c")) catch @panic("OOM");
    for (png_sources) |s| sfd_srcs.append(c.a, c.src(s)) catch @panic("OOM");
    sfd_srcs.appendSlice(c.a, &.{ c.src("src/platform/files/vfs.c"), c.src("src/platform/files/iso9660.c"), c.os_src }) catch @panic("OOM");
    for (sfd_srcs.items) |s| sfd_objs.append(c.a, compile(c, sfd, s, .{
        .target = "p4pc_sfdview",
        .defines = c.port_defines,
        .includes = c.list(&.{ c.global_includes, &.{ c.src("src/movie"), c.src("src/viewer"), c.src("src/platform/files") } }),
        .system_includes = c.global_system_includes,
        .flags = c.global_c,
        .after = &.{&c.mirror.step},
    })) catch @panic("OOM");
    const entry = crtEntry(c, "p4pc_sfdview", defs_file, &.{ sfd, movie }, crt_entry_args.items);
    var sfd_system: std.ArrayList([]const u8) = .empty;
    sfd_system.appendSlice(c.a, ffmpeg_libs) catch @panic("OOM");
    sfd_system.append(c.a, "-lm") catch @panic("OOM");
    if (c.win32) sfd_system.appendSlice(c.a, &.{ "-Wl,-Bstatic", "-lwinpthread", "-Wl,-Bdynamic" }) catch @panic("OOM");
    const art = link(c, .{
        .name = "p4pc_sfdview",
        .out_rel = c.fmt("p4pc_sfdview{s}", .{exe_sfx}),
        .model = .i386,
        .cxx = false,
        .objs_set = sfd,
        .objs = sfd_objs.items,
        .extra_sets = &.{movie},
        .extra_objs = &.{movie_objs.items},
        .link_flags = c.list(&.{ c.base_link, c.target_link_flags }),
        .system_libs = sfd_system.items,
        .after = &.{&entry.step},
        .map_file = c.join(&.{ c.gen, "crt-link-p4pc_sfdview.map" }),
    });
    _ = c.artifacts.put(c.a, "p4pc_sfdview", art) catch @panic("OOM");
    c.steps.artifacts.dependOn(art.step);
    manifestTarget(c, "p4pc_sfdview", "executable", .i386, sfd_srcs.items, sfd_objs.items, &.{}, &.{}, false, art.path);
    c.steps.compile_only.dependOn(&movie.stage.step);
    const movie_lib = c.a.create(ObjectLibrary) catch @panic("OOM");
    movie_lib.* = .{ .spec = movieSpec(c), .model = .i386, .objs = movie_objs.items, .set = movie };
    c.movie_libs[@intFromEnum(Model.i386)] = movie_lib;
}

fn removeCompat(c: *Ctx, flags: []const []const u8) []const []const u8 {
    var out: std.ArrayList([]const u8) = .empty;
    var i: usize = 0;
    while (i < flags.len) : (i += 1) {
        if (std.mem.eql(u8, flags[i], "-include") and i + 1 < flags.len and std.mem.eql(u8, flags[i + 1], "p4pc/compat.h")) {
            i += 1;
            continue;
        }
        out.append(c.a, flags[i]) catch @panic("OOM");
    }
    return out.items;
}

const Mirror64 = struct {
    units: []const Obj,
    set: *Set,
    census: ?*Step,
    crt: ?*Run,
};

/// 080 (ABI per unit, rewrite+compile per unit, census), 340 (port ABIs, merge), 300 (CRT mirror64/mirror64w).
fn port64Graph(c: *Ctx, m: PortModel, ad64: ?Adapters, ad64w: ?Adapters) void {
    const o = c.o;
    const g64 = c.join(&.{ c.gen, "obj64" });
    const env = m.x64_env;
    const pre: []const []const u8 = &.{ "-W", "ignore" };

    // per-unit ABI
    const abi_set = c.newSet("abi64", &.{".d"});
    var abi_files: std.ArrayList([]const u8) = .empty;
    var abi_cached: std.ArrayList(LazyPath) = .empty;
    var active: std.ArrayList(u8) = .empty;
    for (c.units.items) |u| {
        const base = std.fs.path.basename(u.rel);
        const dir_rel = std.fs.path.dirname(u.rel) orelse "";
        const run = c.pyTool(c.fmt("abi64 {s}", .{u.rel}), pre, "tools/port64/unit64.py", .{ .depfile_out = true });
        run.addArgs(&.{ "abi", "--gen", c.gen, "--root", c.root, "--unit", u.rel });
        const dir = run.addOutputDirectoryArg2("abi", .{ .prefix = "--out=", .suffix = c.fmt("/{s}.abi.json", .{base}) });
        run.addArgs(&.{ "--clang", c.cc });
        for (env) |e| run.setEnvironmentVariable(e[0], e[1]);
        c.useCompiler(run);
        c.inputs(run, &port64_tools);
        run.addFileInput(c.lp(u.abs));
        run.step.dependOn(&c.mirror.step);
        abi_set.stage.addDirectoryArg2(dir, .{ .prefix = c.fmt("{s}=", .{c.join(&.{ g64, "abi", dir_rel })}) });
        const cached = dir.path(c.b, c.fmt("{s}.abi.json", .{base}));
        abi_set.items.append(c.a, .{ .fixed = c.join(&.{ g64, "abi", c.fmt("{s}.abi.json", .{u.rel}) }), .cached = cached }) catch @panic("OOM");
        abi_files.append(c.a, c.join(&.{ g64, "abi", c.fmt("{s}.abi.json", .{u.rel}) })) catch @panic("OOM");
        abi_cached.append(c.a, cached) catch @panic("OOM");
        active.appendSlice(c.a, u.rel) catch @panic("OOM");
        active.append(c.a, '\n') catch @panic("OOM");
    }

    // 340: the port's ABIs (p4port, p4portee .c sources outside third_party/; the Zig exports by prototype)
    var port_abi_files: std.ArrayList([]const u8) = .empty;
    {
        var zig_srcs: std.ArrayList([]const u8) = .empty;
        var zig_cflags: std.ArrayList([]const u8) = .empty;
        const Tgt = struct { srcs: []const []const u8, incs: []const []const u8, defs: []const []const u8 };
        const port_srcs = p4portSources(c, c.os_src);
        const portee_srcs = p4porteeSources(c);
        var port_incs: std.ArrayList([]const u8) = .empty;
        for (c.list(&.{ c.global_includes, c.global_system_includes, p4portIncludes(c)[c.global_includes.len..] })) |inc| {
            if (o.platform == .linux and std.mem.eql(u8, inc, c.ffmpeg_inc)) {
                const native_inc = if (o.linux64_prefix) |prefix| c.join(&.{ prefix, "include" }) else if (o.arch == .aarch64) "/usr/include/aarch64-linux-gnu" else "/usr/include/x86_64-linux-gnu";
                port_incs.append(c.a, native_inc) catch @panic("OOM");
            } else {
                port_incs.append(c.a, inc) catch @panic("OOM");
            }
        }
        const tgts = [_]Tgt{
            .{ .srcs = port_srcs, .incs = port_incs.items, .defs = p4portDefines(c) },
            .{ .srcs = portee_srcs, .incs = port_incs.items, .defs = c.port_defines },
        };
        for (tgts, 0..) |t, ti| {
            var cflags: std.ArrayList([]const u8) = .empty;
            for (t.incs) |i| cflags.append(c.a, c.fmt("--cflag=-I{s}", .{i})) catch @panic("OOM");
            for (t.defs) |d| cflags.append(c.a, c.fmt("--cflag=-D{s}", .{d})) catch @panic("OOM");
            // Zig's bundled libc does not add clang's host system include path.
            if (o.platform == .linux) cflags.appendSlice(c.a, &.{ "--cflag=-idirafter", "--cflag=/usr/include" }) catch @panic("OOM");
            if (ti == 0) {
                zig_cflags.appendSlice(c.a, cflags.items) catch @panic("OOM");
                zig_cflags.append(c.a, c.fmt("--cflag=-I{s}", .{c.src("src/platform")})) catch @panic("OOM");
            }
            for (t.srcs) |s| {
                if (std.mem.endsWith(u8, s, ".zig")) {
                    zig_srcs.append(c.a, s) catch @panic("OOM");
                    continue;
                }
                if (!std.mem.endsWith(u8, s, ".c") or std.mem.indexOf(u8, s, "third_party/") != null) continue;
                const fixed = if (std.mem.startsWith(u8, s, c.gen))
                    c.join(&.{ g64, "abi", "generated", c.fmt("{s}.abi.json", .{s[c.gen.len + 1 ..]}) })
                else
                    c.join(&.{ g64, "abi", "port", c.fmt("{s}.abi.json", .{s[c.root.len + 1 ..]}) });
                const run = c.pyTool(c.fmt("abi64 {s}", .{s}), pre, "tools/port64/unit64.py", .{ .depfile_out = true });
                run.addArgs(&.{ "abi", "--gen", c.gen, "--root", c.root, "--src", s });
                const out = run.addOutputDirectoryArg2("abi", .{ .prefix = "--out=", .suffix = c.fmt("/{s}", .{std.fs.path.basename(fixed)}) });
                run.addArgs(&.{ "--clang", c.cc });
                run.addArgs(cflags.items);
                for (env) |e| run.setEnvironmentVariable(e[0], e[1]);
                c.useCompiler(run);
                c.inputs(run, &port64_tools);
                run.addFileInput(c.lp(s));
                run.step.dependOn(&c.mirror.step);
                const cached = out.path(c.b, std.fs.path.basename(fixed));
                _ = c.addToSet(abi_set, fixed, cached);
                port_abi_files.append(c.a, fixed) catch @panic("OOM");
                abi_cached.append(c.a, cached) catch @panic("OOM");
            }
        }
        if (zig_srcs.items.len != 0) {
            const fixed = c.join(&.{ g64, "abi", "port", "zig-exports.abi.json" });
            const run = c.pyTool("abi64 zig exports", pre, "tools/port64/unit64.py", .{ .depfile_out = true });
            run.addArgs(&.{ "abi", "--gen", c.gen, "--root", c.root, "--zig-header", c.join(&.{ c.gen, "battle_sim_exports.h" }) });
            for (zig_srcs.items) |z| {
                run.addArgs(&.{ "--zig-source", z });
                run.addFileInput(c.lp(z));
            }
            const out = run.addOutputDirectoryArg2("abi", .{ .prefix = "--out=", .suffix = "/zig-exports.abi.json" });
            run.addArgs(&.{ "--clang", c.cc });
            run.addArgs(zig_cflags.items);
            for (env) |e| run.setEnvironmentVariable(e[0], e[1]);
            c.useCompiler(run);
            c.inputs(run, &port64_tools);
            run.addFileInput(c.lp(c.join(&.{ c.gen, "battle_sim_exports.h" })));
            const cached = out.path(c.b, "zig-exports.abi.json");
            _ = c.addToSet(abi_set, fixed, cached);
            port_abi_files.append(c.a, fixed) catch @panic("OOM");
            abi_cached.append(c.a, cached) catch @panic("OOM");
        }
    }
    {
        var s: std.ArrayList(u8) = .empty;
        for (port_abi_files.items) |f| s.appendSlice(c.a, c.fmt("{s}\n", .{f})) catch @panic("OOM");
        if (s.items.len == 0) s.append(c.a, '\n') catch @panic("OOM");
        c.writeIfChanged(c.join(&.{ c.gen, "crt-native-abi-files.txt" }), s.items);
        var all: std.ArrayList(u8) = .empty;
        for (abi_files.items) |f| all.appendSlice(c.a, c.fmt("{s}\n", .{f})) catch @panic("OOM");
        all.appendSlice(c.a, s.items) catch @panic("OOM");
        c.writeIfChanged(c.join(&.{ g64, "abi-files.txt" }), all.items);
    }
    const abi_json = c.join(&.{ g64, "abi.json" });
    const merge_run = c.pyTool("abi64 merge", &.{}, "tools/port64/unit64.py", .{});
    merge_run.addArgs(&.{ "merge", "--clang", c.cc, "--gen", c.gen, "--root", c.root, "--decomp", c.o.decomp, "--abi-files", c.join(&.{ g64, "abi-files.txt" }), "--out" });
    const merged = merge_run.addOutputFileArg("abi.json");
    for (env) |e| merge_run.setEnvironmentVariable(e[0], e[1]);
    c.useCompiler(merge_run);
    merge_run.step.dependOn(&abi_set.stage.step);
    for (abi_cached.items) |a| merge_run.addFileInput(a);
    c.inputs(merge_run, &.{ "tools/port64/unit64.py", "tools/port64/gptr_rewrite.py", "tools/mipsdiff/sigs.txt", "tools/gen_port.py", "config/native_names.txt" });
    c.absInputs(merge_run, &.{ c.join(&.{ c.gen, "units.txt" }), c.join(&.{ g64, "abi-files.txt" }) });
    c.absInputs(merge_run, &.{ c.join(&.{ c.o.decomp, "config/symbol_addrs.txt" }), c.join(&.{ c.o.decomp, "config/symbol_data_addrs.txt" }), c.join(&.{ c.o.decomp, "config/symbols_recovered.txt" }) });
    const merge = generatedFile(c, merge_run, abi_json, merged);

    // 080: the rewritten mirror for the 64-bit model, the Windows x64 one, and the generation-only census tree
    const launcher: []const u8 = if (o.ee_float) c.fmt("--launcher={s}|{s}|--modes={s}", .{ o.python, c.src("tools/ee_float_cc.py"), o.ee_float_ops }) else "";
    const m64 = mirror64(c, "obj64", "p4decomp64", m.census_baseline, env, o.port64_opt, launcher, false, merge, abi_json, active.items);
    c.b.step("p4decomp64", "The rewritten 64-bit mirror objects, their census and CRT audit").dependOn(m64.census orelse &m64.set.stage.step);
    {
        const view = Run.create(c.b, "g64 view");
        view.addArgs(&.{ o.python, c.helper, "alias", g64 });
        const out = view.addOutputDirectoryArg("view");
        view.addArg("obj64");
        view.addFileInput(c.lp(c.helper));
        view.step.dependOn(&m64.set.stage.step);
        view.step.dependOn(&abi_set.stage.step);
        view.step.dependOn(merge.ready);
        c.g64_view = out.path(c.b, "obj64");
    }
    const gen_only = mirror64(c, "census64", "p4decomp64_generate", m.census_baseline, env, o.port64_opt, "", true, merge, abi_json, active.items);
    c.b.step("p4decomp64_generate", "The 64-bit rewrite, ABI and census without objects (the mobile land guard)").dependOn(gen_only.census orelse &gen_only.set.stage.step);
    c.steps.census_generate.dependOn(gen_only.census orelse &gen_only.set.stage.step);
    var m64w: ?Mirror64 = null;
    if (m.port64w) {
        const env_w = c.a.dupe([2][]const u8, &.{.{ "P4_PORT64_TARGET", "--target=x86_64-w64-mingw32" }}) catch @panic("OOM");
        const launcher_w: []const u8 = if (o.ee_float) c.fmt("--launcher={s}|{s}|--modes={s}", .{ o.python, c.src("tools/ee_float_cc.py"), o.ee_float_ops }) else "";
        m64w = mirror64(c, "obj64w", "p4decomp64w", c.src("config/port64w_census.txt"), env_w, o.port64w_opt, launcher_w, false, merge, abi_json, active.items);
    }

    // 300: mirror64 / mirror64w CRT audits (host names of the i386 profile, mod project of the 64 adapters)
    var host32: std.ArrayList(HostArg) = .empty;
    host32.appendSlice(c.a, c.host_lib_args.items) catch @panic("OOM");
    const crt64 = crtGuard(c, "mirror64", &.{m64.units}, &.{m64.set}, &.{}, host32.items, if (ad64) |a| a.project else &.{}, if (ad64) |a| a.project_lib else null, &.{&abi_set.stage.step});
    for (abi_cached.items) |a| crt64.addFileInput(a);
    c.absInputs(crt64, &.{c.join(&.{ c.gen, "crt-native-abi-files.txt" })});
    const p4decomp64_step = c.b.top_level_steps.get("p4decomp64").?;
    p4decomp64_step.step.dependOn(&crt64.step);
    c.steps.compile_only.dependOn(&p4decomp64_step.step);
    var crt64w: ?*Run = null;
    if (m64w) |w| {
        // a compile-only COFF mirror still has guarded game objects: the real generated bridge, compiled for its audit
        var project_w: []const HostArg = &.{};
        var project_w_lib: ?Library = null;
        const bridge_set = c.newSet("p4_mod_bridge64w", &.{});
        if (ad64w) |a| {
            project_w = a.project;
            project_w_lib = a.project_lib;
        } else {
            const bridge = compile(c, bridge_set, c.join(&.{ c.gen, "mod_bridges.c" }), .{
                .target = "p4_mod_bridge64w",
                .dir_prefix = "tools/mods/",
                .defines = c.list(&.{ c.port_defines, &.{ "P4_GUEST64=1", "P4_GUEST_CHECK=1" } }),
                .includes = &.{ c.src("include"), c.gen },
                .flags = &.{ "--target=x86_64-w64-mingw32", "-m64", "-O2" },
                .after = &.{&c.mirror.step},
            });
            project_w = c.a.dupe(HostArg, &.{ .{ .str = "--project-lib" }, .{ .str = bridge.fixed } }) catch @panic("OOM");
        }
        const crtw = crtGuard(c, "mirror64w", &.{w.units}, &.{ w.set, bridge_set }, &.{}, host32.items, project_w, project_w_lib, &.{&abi_set.stage.step});
        for (abi_cached.items) |a| crtw.addFileInput(a);
        c.absInputs(crtw, &.{c.join(&.{ c.gen, "crt-native-abi-files.txt" })});
        const stepw = c.b.step("p4decomp64w", "The Windows x64 rewritten mirror objects, their census and CRT audit");
        stepw.dependOn(w.census orelse &w.set.stage.step);
        stepw.dependOn(&crtw.step);
        c.steps.compile_only.dependOn(stepw);
        crt64w = crtw;
    }

    if (m.link64) link64(c, m, m64, crt64, ad64, merge);
    if (m.link64w) link64w(c, m, m64w.?, crt64w.?, ad64w, merge);
}

fn mirror64(c: *Ctx, tree: []const u8, name: []const u8, baseline: []const u8, env: []const [2][]const u8, opt: []const u8, launcher: []const u8, generate_only: bool, merge: Generated, abi_json: []const u8, active: []const u8) Mirror64 {
    const o = c.o;
    const root = c.join(&.{ c.gen, tree });
    const units_dir = c.join(&.{ root, "units" });
    const set = c.newSet(name, &.{".d"});
    var units: std.ArrayList(Obj) = .empty;
    var census_files: std.ArrayList(LazyPath) = .empty;
    for (c.units.items) |u| {
        const base = std.fs.path.basename(u.rel);
        const dir_rel = std.fs.path.dirname(u.rel) orelse "";
        const run = c.pyTool(c.fmt("{s} {s}", .{ name, u.rel }), &.{ "-W", "ignore" }, "tools/port64/unit64.py", .{ .depfile_out = true });
        run.addArgs(&.{ "build", "--gen", c.gen, "--root", c.root, "--decomp", c.o.decomp, "--unit", u.rel, "--abi", abi_json });
        const dir = run.addOutputDirectoryArg2("unit", .{ .prefix = "--out=", .suffix = c.fmt("/{s}.64.o", .{base}) });
        run.addArgs(&.{ "--clang", c.cc, "--check" });
        if (generate_only) {
            run.addArg("--generate-only");
        } else {
            run.addArgs(&.{ "--opt", opt });
            if (launcher.len != 0) run.addArg(launcher);
            if (o.auto_init) |m| run.addArg(c.fmt("--auto-init={s}", .{m}));
        }
        for (env) |e| run.setEnvironmentVariable(e[0], e[1]);
        c.useCompiler(run);
        c.inputs(run, &port64_tools);
        if (launcher.len != 0) c.inputs(run, &.{ "tools/ee_float_cc.py", "tools/port64/printf_rewrite.py" });
        c.absInputs(run, &.{ c.join(&.{ c.o.decomp, "config/symbol_addrs.txt" }), c.join(&.{ c.o.decomp, "config/symbol_data_addrs.txt" }), c.join(&.{ c.o.decomp, "config/symbols_recovered.txt" }) });
        run.addFileInput(c.lp(u.abs));
        run.addFileInput(merge.cached);
        run.step.dependOn(merge.ready);
        run.step.dependOn(&c.mirror.step);
        set.stage.addDirectoryArg2(dir, .{ .prefix = c.fmt("{s}=", .{c.join(&.{ units_dir, dir_rel })}) });
        const file = if (generate_only) c.fmt("{s}.64.o.i", .{base}) else c.fmt("{s}.64.o", .{base});
        const obj: Obj = .{ .fixed = c.join(&.{ units_dir, c.fmt("{s}.64.o{s}", .{ u.rel, if (generate_only) ".i" else "" }) }), .cached = dir.path(c.b, file) };
        set.items.append(c.a, obj) catch @panic("OOM");
        units.append(c.a, obj) catch @panic("OOM");
        census_files.append(c.a, dir.path(c.b, c.fmt("{s}.64.o.census", .{base}))) catch @panic("OOM");
    }
    // p4_census_check
    const active_file = c.join(&.{ root, "active_units.txt" });
    c.writeIfChanged(active_file, active);
    var census: ?*Step = null;
    if (!c.o.census_check) {
        std.log.warn("port64: -Dcensus-check=false: the rewriter's rule counts of {s} are not checked against {s}", .{ name, baseline });
        c.steps.census.dependOn(&c.b.addFail(c.fmt("census of {s} disabled by -Dcensus-check=false", .{name})).step);
    } else {
        if (!c.exists(baseline)) fatal("port64: no reviewed census baseline '{s}'; rule counts must not go unchecked", .{baseline});
        const run = c.pyTool(c.fmt("census {s}", .{name}), &.{ "-W", "ignore" }, "tools/port64/unit64.py", .{ .stamp = true });
        const rw_clean_units = c.join(&.{ c.gen, "rw_clean_units.txt" });
        run.addArgs(&.{ "report", "--dir", units_dir, "--census-baseline", baseline, "--clang", c.cc, "--active-units", active_file, "--rw-clean-units", rw_clean_units, "--rw-clean-stripped", c.join(&.{ c.gen, "decomp", "src", "rwclean", "stripped_units.txt" }) });
        c.useCompiler(run);
        run.step.dependOn(&set.stage.step);
        for (census_files.items) |f| run.addFileInput(f);
        run.addFileInput(c.lp(baseline));
        run.addFileInput(c.lp(active_file));
        run.addFileInput(c.lp(rw_clean_units));
        census = &run.step;
        if (generate_only) c.steps.census_generate.dependOn(&run.step) else c.steps.census.dependOn(&run.step);
    }
    if (!generate_only) c.steps.compile_only.dependOn(census orelse &set.stage.step);
    return .{ .units = units.items, .set = set, .census = census, .crt = null };
}

/// 350-port64-player: p4port64/p4portee64, CRT native64, tables64 and the 64-bit executables (or Android's libmain.so).
fn link64(c: *Ctx, m: PortModel, m64: Mirror64, crt_mirror: *Run, ad: ?Adapters, merge: Generated) void {
    const objs = objectSet(c, "port64 objects");
    const o = c.o;
    var sdl3: []const u8 = undefined;
    var ffmpeg: []const []const u8 = undefined;
    var host: std.ArrayList(HostArg) = .empty;
    if (c.android64) {
        sdl3 = c.sdl3;
        ffmpeg = c.strs(&.{ c.avcodec, c.swscale, c.avutil });
        host.append(c.a, .{ .str = "--pic" }) catch @panic("OOM");
        host.appendSlice(c.a, c.host_lib_args.items) catch @panic("OOM");
    } else if (c.apple) {
        sdl3 = c.sdl3;
        ffmpeg = c.strs(&.{ c.avcodec, c.swscale, c.avutil });
        host.appendSlice(c.a, c.host_lib_args.items) catch @panic("OOM");
    } else {
        const libdir: []const u8 = if (o.arch == .aarch64) "/usr/lib/aarch64-linux-gnu" else "/usr/lib/x86_64-linux-gnu";
        const libdir2: []const u8 = if (o.arch == .aarch64) "/lib/aarch64-linux-gnu" else "/lib/x86_64-linux-gnu";
        const av = if (o.linux64_prefix) |p| c.join(&.{ p, "lib" }) else libdir;
        sdl3 = c.findLibrary("64-bit SDL3", &.{"libSDL3.so"}, &.{av});
        const libm = c.findLibrary("64-bit libm", &.{"libm.so.6"}, &.{ libdir, libdir2 });
        const libc = c.findLibrary("64-bit libc", &.{"libc.so.6"}, &.{ libdir, libdir2 });
        const avcodec = c.findLibrary("64-bit avcodec", &.{"libavcodec.so"}, &.{av});
        const avutil = c.findLibrary("64-bit avutil", &.{"libavutil.so"}, &.{av});
        const swscale = c.findLibrary("64-bit swscale", &.{"libswscale.so"}, &.{av});
        ffmpeg = c.a.dupe([]const u8, &.{ avcodec, swscale, avutil }) catch @panic("OOM");
        host.appendSlice(c.a, &.{
            .{ .str = "--host-lib" }, .{ .str = libc },    .{ .str = "--host-lib" }, .{ .str = libm },
            .{ .str = "--host-lib" }, .{ .str = sdl3 },    .{ .str = "--host-lib" }, .{ .str = avcodec },
            .{ .str = "--host-lib" }, .{ .str = swscale }, .{ .str = "--host-lib" }, .{ .str = avutil },
        }) catch @panic("OOM");
        if (o.arch == .aarch64) host.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .file = compilerRt(c, "aarch64-linux-gnu") } }) catch @panic("OOM");
    }
    if (ad) |a| host.appendSlice(c.a, a.host_args) catch @panic("OOM");
    const x64_flags = c.list(&.{ m.x64_target_flags, m.x64_extra, &.{"-DP4_GUEST_CHECK=1"} });
    const port = portObjects(c, .m64, m, objs);
    manifestTarget(c, "p4port64", "object", .m64, &.{}, port.port, &.{}, &.{}, false, "");
    manifestTarget(c, "p4portee64", "object", .m64, &.{}, port.portee, &.{}, &.{}, false, "");
    const project: []const HostArg = if (ad) |a| a.project else &.{};
    const crt = crtGuard(c, "native64", &.{ m64.units, filterNative(c, port.port), filterNative(c, port.portee) }, &.{ m64.set, objs }, &.{ port.port, port.portee }, host.items, project, if (ad) |a| a.project_lib else null, &.{});
    const objects_file = c.join(&.{ c.gen, "objects64.txt" });
    writeList(c, objects_file, &.{ m64.units, port.port, port.portee });
    const tables_c = c.join(&.{ c.gen, "p4pc_tables64.c" });
    var thost: std.ArrayList(HostArg) = .empty;
    thost.appendSlice(c.a, host.items) catch @panic("OOM");
    thost.appendSlice(c.a, project) catch @panic("OOM");
    var tflags: std.ArrayList([]const u8) = .empty;
    tflags.append(c.a, "--ptr64") catch @panic("OOM");
    tflags.appendSlice(c.a, m.x64_gen_tools) catch @panic("OOM");
    const after_tables = &[_]*Step{ &crt.step, &crt_mirror.step, merge.ready, m64.census orelse &m64.set.stage.step };
    const tab = tables(c, "64", tables_c, objects_file, tflags.items, thost.items, &.{ m64.set, objs }, after_tables, merge);
    if (ad) |a| for (a.libs) |l| useLibrary(c, tab.run, l);
    c.images[@intFromEnum(Model.m64)] = .{
        .decomp = m64.units,
        .decomp_set = m64.set,
        .port = port.port,
        .portee = port.portee,
        .port_set = objs,
        .tables = tab,
        .tables_c = tables_c,
        .ad = ad,
        .sdl3 = sdl3,
        .avcodec = ffmpeg[0],
        .avutil = ffmpeg[2],
        .swscale = ffmpeg[1],
        .after = c.a.dupe(*Step, &.{ m64.census orelse &m64.set.stage.step, &crt_mirror.step }) catch @panic("OOM"),
    };

    const final_flags = c.list(&.{ c.global_c, x64_flags, &.{"-fno-builtin"} });
    var entry_args: std.ArrayList(HostArg) = .empty;
    entry_args.appendSlice(c.a, host.items) catch @panic("OOM");
    const Final = struct { name: []const u8, srcs: []const []const u8, viewer_main: bool, files: bool, shared: bool, wrap_exit: bool };
    var finals: std.ArrayList(Final) = .empty;
    if (c.android64) {
        finals.append(c.a, .{ .name = "main", .srcs = &.{ c.src("src/loader.c"), c.src("src/viewer/p4pc_viewer.c"), c.src("src/platform/android/android_main.c"), tables_c }, .viewer_main = true, .files = true, .shared = true, .wrap_exit = true }) catch @panic("OOM");
        if (o.android_phone_runner) finals.append(c.a, .{ .name = "p4pc_phone", .srcs = &.{ c.src("src/loader.c"), c.src("src/platform/android/phone_main.c"), tables_c }, .viewer_main = false, .files = true, .shared = false, .wrap_exit = false }) catch @panic("OOM");
    } else {
        finals.append(c.a, .{ .name = "p4pc64", .srcs = &.{ c.src("src/loader.c"), tables_c }, .viewer_main = false, .files = false, .shared = false, .wrap_exit = false }) catch @panic("OOM");
        finals.append(c.a, .{ .name = "p4pc_viewer64", .srcs = &.{ c.src("src/viewer/p4pc_viewer.c"), tables_c }, .viewer_main = false, .files = true, .shared = false, .wrap_exit = false }) catch @panic("OOM");
    }
    for (finals.items) |f| {
        const own = c.newSet(f.name, &.{});
        var own_objs: std.ArrayList(Obj) = .empty;
        for (f.srcs) |s| {
            var defs: std.ArrayList([]const u8) = .empty;
            defs.appendSlice(c.a, c.port_defines) catch @panic("OOM");
            if (f.viewer_main and std.mem.endsWith(u8, s, "p4pc_viewer.c")) defs.append(c.a, "main=p4pc_viewer_main") catch @panic("OOM");
            own_objs.append(c.a, compile(c, own, s, .{
                .target = f.name,
                .defines = defs.items,
                .includes = if (f.files) c.list(&.{ c.global_includes, &.{c.src("src/platform/files")} }) else c.global_includes,
                .system_includes = c.global_system_includes,
                .flags = final_flags,
                .after = if (std.mem.eql(u8, s, tables_c)) &.{ tab.ready, &c.mirror.step } else &.{&c.mirror.step},
                .file_inputs = if (std.mem.eql(u8, s, tables_c)) &.{tab.cached} else &.{},
            })) catch @panic("OOM");
        }
        if (o.platform == .linux and o.image_clang.len == 0) {
            // Zig's ELF linker otherwise synthesizes __dso_handle at the image
            // header.  Executable crtbegin instead provides zero-valued storage.
            // (The image runtime mode links the image's own crtbegin.o, which defines it.)
            own_objs.append(c.a, compile(c, own, c.src("zigbuild/dso_handle.c"), .{
                .target = f.name,
                .includes = c.global_includes,
                .system_includes = c.global_system_includes,
                .flags = final_flags,
            })) catch @panic("OOM");
        }
        const entry = crtEntry(c, f.name, objects_file, &.{ own, objs, m64.set }, entry_args.items);
        var lf: std.ArrayList([]const u8) = .empty;
        lf.appendSlice(c.a, c.base_link) catch @panic("OOM");
        lf.appendSlice(c.a, c.target_link_flags) catch @panic("OOM");
        lf.appendSlice(c.a, m.x64_link) catch @panic("OOM");
        var system: std.ArrayList([]const u8) = .empty;
        if (c.android64) {
            lf.appendSlice(c.a, &.{ "-Wl,-z,max-page-size=16384", "-Wl,--build-id=none" }) catch @panic("OOM");
            if (f.wrap_exit) lf.append(c.a, "-Wl,--wrap=exit") catch @panic("OOM");
            system.appendSlice(c.a, &.{ "-lm", "-ldl", "-llog", "-landroid", sdl3 }) catch @panic("OOM");
        } else {
            system.appendSlice(c.a, &.{ "-lm", sdl3 }) catch @panic("OOM");
        }
        system.appendSlice(c.a, ffmpeg) catch @panic("OOM");
        var ad_libs: []const Library = &.{};
        var cxx = false;
        var names: []const []const u8 = &.{};
        if (ad) |a| {
            ad_libs = a.libs;
            cxx = a.cxx;
            lf.appendSlice(c.a, a.link_flags) catch @panic("OOM");
            names = adapterLibraries(.m64, true, o.mod_support and o.luau);
        }
        const art = link(c, .{
            .name = f.name,
            .out_rel = if (f.shared) "libmain.so" else f.name,
            .model = .m64,
            .cxx = cxx,
            .shared = f.shared,
            .objs_set = own,
            .objs = own_objs.items,
            .own_objs_last = true,
            .extra_sets = &.{ m64.set, objs, objs },
            .extra_objs = &.{ m64.units, port.port, port.portee },
            .libs = ad_libs,
            .link_flags = lf.items,
            .system_libs = system.items,
            .after = &.{ &entry.step, m64.census orelse &m64.set.stage.step, &crt_mirror.step },
            .map_file = c.join(&.{ c.gen, c.fmt("crt-link-{s}.map", .{f.name}) }),
        });
        _ = c.artifacts.put(c.a, f.name, art) catch @panic("OOM");
        c.steps.artifacts.dependOn(art.step);
        manifestTarget(c, f.name, if (f.shared) "shared_library" else "executable", .m64, f.srcs, own_objs.items, ad_libs, names, cxx, art.path);
    }
}

/// 360-port64-win: p4port64w/p4portee64w, CRT native64w, the Windows x64 tables and executables, the license probe, DLLs.
fn link64w(c: *Ctx, m: PortModel, w: Mirror64, crt_mirror: *Run, ad: ?Adapters, merge: Generated) void {
    const objs = objectSet(c, "port64w objects");
    const dlls = c.newSet("win64 DLLs", &.{});
    c.steps.artifacts.dependOn(&dlls.stage.step);
    c.dlls_ready = &dlls.stage.step;
    const o = c.o;
    const lib = c.join(&.{ o.win64_prefix, "lib" });
    const sdl3 = c.findLibrary("Windows x64 SDL3", &.{"libSDL3.dll.a"}, &.{lib});
    const avcodec = c.findLibrary("Windows x64 avcodec", &.{"libavcodec.dll.a"}, &.{lib});
    const avutil = c.findLibrary("Windows x64 avutil", &.{"libavutil.dll.a"}, &.{lib});
    const swscale = c.findLibrary("Windows x64 swscale", &.{"libswscale.dll.a"}, &.{lib});
    const mingw = o.mingw64_lib_dir orelse fatal("the Windows x64 link needs -Dmingw64-lib-dir=<llvm-mingw>/x86_64-w64-mingw32/lib: its import libraries name the host functions for the tables", .{});
    var host: std.ArrayList(HostArg) = .empty;
    for ([_][]const u8{ "libucrt.a", "libmingwex.a", "libmingw32.a" }) |n| host.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .str = c.findLibrary("mingw-w64 x64 runtime", &.{n}, &.{mingw}) } }) catch @panic("OOM");
    host.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .file = compilerRt(c, "x86_64-windows-gnu") } }) catch @panic("OOM");
    for ([_][]const u8{ "libwinpthread.a", "libkernel32.a", "libuser32.a", "libadvapi32.a", "libshell32.a" }) |n| host.appendSlice(c.a, &.{ .{ .str = "--host-lib" }, .{ .str = c.findLibrary("mingw-w64 x64 runtime", &.{n}, &.{mingw}) } }) catch @panic("OOM");
    host.appendSlice(c.a, &.{
        .{ .str = "--host-lib" }, .{ .str = sdl3 },    .{ .str = "--host-lib" }, .{ .str = avcodec },
        .{ .str = "--host-lib" }, .{ .str = swscale }, .{ .str = "--host-lib" }, .{ .str = avutil },
        .{ .str = "--host-name" }, .{ .str = "__ImageBase" },
    }) catch @panic("OOM");
    if (ad) |a| host.appendSlice(c.a, a.host_args) catch @panic("OOM");
    const port = portObjects(c, .m64w, m, objs);
    manifestTarget(c, "p4port64w", "object", .m64w, &.{}, port.port, &.{}, &.{}, false, "");
    manifestTarget(c, "p4portee64w", "object", .m64w, &.{}, port.portee, &.{}, &.{}, false, "");
    const project: []const HostArg = if (ad) |a| a.project else &.{};
    const crt = crtGuard(c, "native64w", &.{ w.units, filterNative(c, port.port), filterNative(c, port.portee) }, &.{ w.set, objs }, &.{ port.port, port.portee }, host.items, project, if (ad) |a| a.project_lib else null, &.{});
    const objects_file = c.join(&.{ c.gen, "objects64w.txt" });
    writeList(c, objects_file, &.{ w.units, port.port, port.portee });
    const tables_c = c.join(&.{ c.gen, "p4pc_tables64w.c" });
    var thost: std.ArrayList(HostArg) = .empty;
    thost.appendSlice(c.a, &.{ .{ .str = "--nm" }, .{ .str = o.llvm_nm }, .{ .str = "--objdump" }, .{ .str = o.llvm_objdump } }) catch @panic("OOM");
    thost.appendSlice(c.a, host.items) catch @panic("OOM");
    thost.appendSlice(c.a, project) catch @panic("OOM");
    const tab = tables(c, "64w", tables_c, objects_file, &.{ "--ptr64", "--coff64" }, thost.items, &.{ w.set, objs }, &.{ &crt.step, &crt_mirror.step, merge.ready, w.census orelse &w.set.stage.step }, merge);
    if (ad) |a| for (a.libs) |l| useLibrary(c, tab.run, l);
    c.images[@intFromEnum(Model.m64w)] = .{
        .decomp = w.units,
        .decomp_set = w.set,
        .port = port.port,
        .portee = port.portee,
        .port_set = objs,
        .tables = tab,
        .tables_c = tables_c,
        .ad = ad,
        .sdl3 = sdl3,
        .avcodec = avcodec,
        .avutil = avutil,
        .swscale = swscale,
        .after = c.a.dupe(*Step, &.{ w.census orelse &w.set.stage.step, &crt_mirror.step }) catch @panic("OOM"),
    };

    const wflags = &[_][]const u8{ "--target=x86_64-w64-mingw32", "-m64", "-DP4_GUEST_CHECK=1" };
    const wlink = c.list(&.{ &.{ "--target=x86_64-w64-mingw32", "-m64" }, c.pe_repro });
    const sys = &[_][]const u8{c.join(&.{ o.win64_prefix, "include" })};
    var incs_nousr: std.ArrayList([]const u8) = .empty;
    for (c.global_includes) |i| if (!std.mem.startsWith(u8, i, "/usr/")) incs_nousr.append(c.a, i) catch @panic("OOM");
    const final_flags = c.list(&.{ c.global_c, wflags, &.{"-fno-builtin"} });
    const Final = struct { name: []const u8, srcs: []const []const u8, files: bool };
    const finals = [_]Final{
        .{ .name = "p4pc64w", .srcs = &.{ c.src("src/loader.c"), tables_c }, .files = false },
        .{ .name = "p4pc_viewer64w", .srcs = &.{ c.src("src/viewer/p4pc_viewer.c"), tables_c }, .files = true },
    };
    // the DLLs the exes load, next to them
    var dll_steps: std.ArrayList(*Step) = .empty;
    for (c.listDir(c.join(&.{ o.win64_prefix, "bin" }), ".dll")) |dll| {
        const path = c.join(&.{ o.win64_prefix, "bin", dll });
        _ = c.addToSet(dlls, c.join(&.{ c.bdir, "win64", dll }), c.lp(path));
        const install = c.b.addInstallFileWithDir(c.lp(path), .{ .custom = "bin/win64" }, dll);
        dll_steps.append(c.a, &install.step) catch @panic("OOM");
        c.runtime_files.append(c.a, c.join(&.{ c.bdir, "win64", dll })) catch @panic("OOM");
    }
    var entry_args: std.ArrayList(HostArg) = .empty;
    entry_args.appendSlice(c.a, host.items) catch @panic("OOM");
    for (finals) |f| {
        const own = c.newSet(f.name, &.{});
        var own_objs: std.ArrayList(Obj) = .empty;
        for (f.srcs) |s| own_objs.append(c.a, compile(c, own, s, .{
            .target = f.name,
            .defines = c.port_defines,
            .includes = if (f.files) c.list(&.{ incs_nousr.items, &.{c.src("src/platform/files")} }) else incs_nousr.items,
            .system_includes = c.global_system_includes,
            .flags = final_flags,
            .after = if (std.mem.eql(u8, s, tables_c)) &.{ tab.ready, &c.mirror.step } else &.{&c.mirror.step},
            .file_inputs = if (std.mem.eql(u8, s, tables_c)) &.{tab.cached} else &.{},
        })) catch @panic("OOM");
        const entry = crtEntry(c, f.name, objects_file, &.{ own, objs, w.set }, entry_args.items);
        var lf: std.ArrayList([]const u8) = .empty;
        lf.appendSlice(c.a, wlink) catch @panic("OOM");
        lf.appendSlice(c.a, &.{ "-Wl,--stack=8388608", "-Wl,--subsystem,windows", "-Wl,-e,mainCRTStartup" }) catch @panic("OOM");
        var ad_libs: []const Library = &.{};
        var cxx = false;
        var names: []const []const u8 = &.{};
        if (ad) |a| {
            ad_libs = a.libs;
            cxx = a.cxx;
            lf.appendSlice(c.a, a.link_flags) catch @panic("OOM");
            names = adapterLibraries(.m64w, true, o.mod_support and o.luau);
        }
        const art = link(c, .{
            .name = f.name,
            .out_rel = c.fmt("win64/{s}.exe", .{f.name}),
            .model = .m64w,
            .cxx = cxx,
            .objs_set = own,
            .objs = own_objs.items,
            .own_objs_last = true,
            .extra_sets = &.{ w.set, objs, objs },
            .extra_objs = &.{ w.units, port.port, port.portee },
            .libs = ad_libs,
            .link_flags = lf.items,
            .system_libs = &.{ sdl3, avcodec, swscale, avutil, "-Wl,-Bstatic", "-lwinpthread", "-Wl,-Bdynamic" },
            .after = &.{ &entry.step, w.census orelse &w.set.stage.step, &crt_mirror.step },
            .map_file = c.join(&.{ c.gen, c.fmt("crt-link-{s}.map", .{f.name}) }),
            .install_dir = "bin/win64",
        });
        for (dll_steps.items) |s| art.step.dependOn(s);
        art.view.dependOn(&dlls.stage.step);
        _ = c.artifacts.put(c.a, f.name, art) catch @panic("OOM");
        c.steps.artifacts.dependOn(art.step);
        manifestTarget(c, f.name, "executable", .m64w, f.srcs, own_objs.items, ad_libs, names, cxx, art.path);
    }
    // ffmpeg_license_test64w (a package check on the shipped x64 DLLs)
    const lic = c.newSet("ffmpeg_license_test64w", &.{});
    const lic_obj = compile(c, lic, c.src("tests/ffmpeg_license_test.c"), .{
        .target = "ffmpeg_license_test64w",
        .includes = incs_nousr.items,
        .system_includes = sys,
        .flags = c.list(&.{ c.global_c, wflags }),
    });
    const art = link(c, .{
        .name = "ffmpeg_license_test64w",
        .out_rel = "win64/ffmpeg_license_test64w.exe",
        .model = .m64w,
        .cxx = false,
        .objs_set = lic,
        .objs = &.{lic_obj},
        .link_flags = wlink,
        .system_libs = &.{ avcodec, avutil, swscale },
        .install_dir = "bin/win64",
    });
    _ = c.artifacts.put(c.a, "ffmpeg_license_test64w", art) catch @panic("OOM");
    c.artifacts.get("p4pc64w").?.step.dependOn(art.step);
    manifestTarget(c, "ffmpeg_license_test64w", "executable", .m64w, &.{c.src("tests/ffmpeg_license_test.c")}, &.{lic_obj}, &.{}, &.{}, false, art.path);
}

// ---------------------------------------------------------------------------------------------------------------------
// compile_commands.json and the graph manifest

fn jsonString(s: *std.ArrayList(u8), a: std.mem.Allocator, text: []const u8) void {
    s.append(a, '"') catch @panic("OOM");
    for (text) |ch| switch (ch) {
        '"' => s.appendSlice(a, "\\\"") catch @panic("OOM"),
        '\\' => s.appendSlice(a, "\\\\") catch @panic("OOM"),
        '\n' => s.appendSlice(a, "\\n") catch @panic("OOM"),
        '\r' => s.appendSlice(a, "\\r") catch @panic("OOM"),
        '\t' => s.appendSlice(a, "\\t") catch @panic("OOM"),
        0...8, 11, 12, 14...0x1f => s.appendSlice(a, std.fmt.allocPrint(a, "\\u{x:0>4}", .{ch}) catch @panic("OOM")) catch @panic("OOM"),
        else => s.append(a, ch) catch @panic("OOM"),
    };
    s.append(a, '"') catch @panic("OOM");
}

fn jsonStrings(s: *std.ArrayList(u8), a: std.mem.Allocator, items: []const []const u8) void {
    s.append(a, '[') catch @panic("OOM");
    for (items, 0..) |item, i| {
        if (i != 0) s.appendSlice(a, ", ") catch @panic("OOM");
        jsonString(s, a, item);
    }
    s.append(a, ']') catch @panic("OOM");
}

fn shellQuote(c: *Ctx, arg: []const u8) []const u8 {
    for (arg) |ch| {
        if (!(std.ascii.isAlphanumeric(ch) or std.mem.indexOfScalar(u8, "-_./=:+,@%", ch) != null)) {
            const escaped = std.mem.replaceOwned(u8, c.a, arg, "'", "'\\''") catch @panic("OOM");
            return c.fmt("'{s}'", .{escaped});
        }
    }
    return arg;
}

fn writeCompileCommands(c: *Ctx) void {
    var s: std.ArrayList(u8) = .empty;
    s.appendSlice(c.a, "[\n") catch @panic("OOM");
    for (c.compdb.items, 0..) |e, i| {
        if (i != 0) s.appendSlice(c.a, ",\n") catch @panic("OOM");
        s.appendSlice(c.a, "{\n  \"directory\": ") catch @panic("OOM");
        jsonString(&s, c.a, c.bdir);
        s.appendSlice(c.a, ",\n  \"command\": ") catch @panic("OOM");
        var words: std.ArrayList([]const u8) = .empty;
        for (e.arguments) |arg| words.append(c.a, shellQuote(c, arg)) catch @panic("OOM");
        jsonString(&s, c.a, joinWords(c, words.items));
        s.appendSlice(c.a, ",\n  \"arguments\": ") catch @panic("OOM");
        jsonStrings(&s, c.a, e.arguments);
        s.appendSlice(c.a, ",\n  \"file\": ") catch @panic("OOM");
        jsonString(&s, c.a, e.file);
        s.appendSlice(c.a, ",\n  \"output\": ") catch @panic("OOM");
        jsonString(&s, c.a, e.output);
        s.appendSlice(c.a, "\n}") catch @panic("OOM");
    }
    s.appendSlice(c.a, "\n]\n") catch @panic("OOM");
    c.writeIfChanged(c.join(&.{ c.bdir, "compile_commands.json" }), s.items);
}

fn manifestTarget(c: *Ctx, name: []const u8, kind: []const u8, model: Model, sources: []const []const u8, objects: []const Obj, libs: []const Library, adapters: []const []const u8, cxx: bool, product: []const u8) void {
    var obj_paths: std.ArrayList([]const u8) = .empty;
    for (objects) |o| obj_paths.append(c.a, o.fixed) catch @panic("OOM");
    var lib_paths: std.ArrayList([]const u8) = .empty;
    for (libs) |l| lib_paths.append(c.a, l.fixed) catch @panic("OOM");
    c.manifest.append(c.a, .{
        .name = name,
        .kind = kind,
        .model = model,
        .sources = if (sources.len != 0) c.strs(sources) else blk: {
            // object libraries: their sources in SOURCES order, from the compiles themselves
            var from: std.ArrayList([]const u8) = .empty;
            for (objects) |o| if (o.source.len != 0) from.append(c.a, o.source) catch @panic("OOM");
            break :blk from.items;
        },
        .objects = obj_paths.items,
        .libraries = lib_paths.items,
        .adapters = adapters,
        .linker = if (cxx) "CXX" else "C",
        .product = product,
    }) catch @panic("OOM");
}

fn writeManifest(c: *Ctx, m: PortModel) []const u8 {
    var s: std.ArrayList(u8) = .empty;
    s.appendSlice(c.a, "{\n  \"profile\": ") catch @panic("OOM");
    jsonString(&s, c.a, c.profile);
    s.appendSlice(c.a, ",\n  \"build_dir\": ") catch @panic("OOM");
    jsonString(&s, c.a, c.bdir);
    s.appendSlice(c.a, ",\n  \"toolchain_id\": ") catch @panic("OOM");
    jsonString(&s, c.a, c.toolchain_id);
    const optimizations = [_]struct { []const u8, []const u8 }{
        .{ "opt", c.o.opt }, .{ "port64_opt", c.o.port64_opt }, .{ "port64w_opt", c.o.port64w_opt },
    };
    for (optimizations) |optimization| {
        s.appendSlice(c.a, ",\n  ") catch @panic("OOM");
        jsonString(&s, c.a, optimization[0]);
        s.appendSlice(c.a, ": ") catch @panic("OOM");
        jsonString(&s, c.a, optimization[1]);
    }
    s.appendSlice(c.a, ",\n  \"mod_support\": ") catch @panic("OOM");
    s.appendSlice(c.a, if (c.o.mod_support) "true" else "false") catch @panic("OOM");
    s.appendSlice(c.a, ",\n  \"luau\": ") catch @panic("OOM");
    s.appendSlice(c.a, if (c.o.luau) "true" else "false") catch @panic("OOM");
    const facts = [_]struct { []const u8, bool }{
        .{ "win32", c.win32 },           .{ "apple", c.apple },             .{ "android", c.android },
        .{ "android64", c.android64 },   .{ "has_i386", c.has_i386 },       .{ "port64", m.port64 },
        .{ "port64w", m.port64w },       .{ "link64", m.link64 },           .{ "link64w", m.link64w },
        .{ "adapters32", m.adapters32 }, .{ "adapters64", m.adapters64 },   .{ "adapters64w", m.adapters64w },
    };
    for (facts) |f| {
        s.appendSlice(c.a, c.fmt(",\n  \"{s}\": {s}", .{ f[0], if (f[1]) "true" else "false" })) catch @panic("OOM");
    }
    s.appendSlice(c.a, ",\n  \"gen_dir\": ") catch @panic("OOM");
    jsonString(&s, c.a, c.gen);
    s.appendSlice(c.a, ",\n  \"root\": ") catch @panic("OOM");
    jsonString(&s, c.a, c.root);
    s.appendSlice(c.a, ",\n  \"rw_clean_port_drop\": ") catch @panic("OOM");
    jsonStrings(&s, c.a, c.rw_clean_drop);
    s.appendSlice(c.a, ",\n  \"targets\": [") catch @panic("OOM");
    for (c.manifest.items, 0..) |t, i| {
        s.appendSlice(c.a, if (i == 0) "\n    {" else ",\n    {") catch @panic("OOM");
        s.appendSlice(c.a, "\"name\": ") catch @panic("OOM");
        jsonString(&s, c.a, t.name);
        s.appendSlice(c.a, ", \"kind\": ") catch @panic("OOM");
        jsonString(&s, c.a, t.kind);
        s.appendSlice(c.a, ", \"suffix\": ") catch @panic("OOM");
        jsonString(&s, c.a, t.model.suffix());
        s.appendSlice(c.a, ", \"linker_language\": ") catch @panic("OOM");
        jsonString(&s, c.a, t.linker);
        s.appendSlice(c.a, ", \"product\": ") catch @panic("OOM");
        jsonString(&s, c.a, t.product);
        s.appendSlice(c.a, ", \"host_adapters\": ") catch @panic("OOM");
        jsonStrings(&s, c.a, t.adapters);
        s.appendSlice(c.a, ", \"libraries\": ") catch @panic("OOM");
        jsonStrings(&s, c.a, t.libraries);
        s.appendSlice(c.a, ", \"sources\": ") catch @panic("OOM");
        jsonStrings(&s, c.a, t.sources);
        s.appendSlice(c.a, ", \"objects\": ") catch @panic("OOM");
        jsonStrings(&s, c.a, t.objects);
        s.append(c.a, '}') catch @panic("OOM");
    }
    s.appendSlice(c.a, "\n  ]\n}\n") catch @panic("OOM");
    const path = c.join(&.{ c.bdir, "zig-graph.json" });
    c.writeIfChanged(path, s.items);
    return path;
}

// ---------------------------------------------------------------------------------------------------------------------
// Native test targets (tests/cmake/*.cmake): add_executable / add_library(OBJECT) in the tests directory, which inherits the
// root's add_compile_options, include_directories, add_link_options and link_libraries, the 64-bit host twins of
// 220-port64-host.cmake (p4_host64_exe, p4_ovr_test64) and the mod hook fixture of cmake/mods.cmake.

/// A target_link_libraries item. The library names resolve to the model's own files (the i386 multiarch, 64-bit or Windows
/// x64 SDL3/FFmpeg); `.target` is a static library target of the graph ($<TARGET_FILE:name>); `.raw` is passed as given.
pub const Lib = union(enum) {
    m,
    sdl3,
    avcodec,
    avutil,
    swscale,
    /// ${P4_FFMPEG_LIBS}: avcodec swscale avutil
    ffmpeg,
    pthread,
    dl,
    rt,
    target: []const u8,
    raw: []const u8,
};

/// set_source_files_properties of one source: COMPILE_DEFINITIONS, INCLUDE_DIRECTORIES, COMPILE_OPTIONS.
pub const SourceProps = struct {
    path: []const u8,
    defines: []const []const u8 = &.{},
    includes: []const []const u8 = &.{},
    compile_options: []const []const u8 = &.{},
};

/// C_COMPILER_LAUNCHER: tools/ee_float_cc.py with its modes (P4_EE_FLOAT's), or p4_printf_launcher(target root modes).
pub const Launcher = union(enum) {
    none,
    ee_float: []const u8,
    printf: struct { root: []const u8, modes: []const u8 },
};

/// Files generated at make time and placed at fixed build-tree paths: compile `paths`, order after `ready`, hash `cached`.
pub const GeneratedFiles = struct {
    paths: []const []const u8,
    ready: *Step,
    cached: []const LazyPath,
};

/// $<TARGET_OBJECTS:...> of a target's sources, or ${P4_TABLES} (the model's port tables compiled into the target).
pub const ObjectsRef = union(enum) {
    p4decomp,
    p4port,
    p4portee,
    p4movie,
    tables,
    library: *ObjectLibrary,
    /// objects produced by a custom command (their fixed paths, the step that places them, their cached files)
    files: GeneratedFiles,
};

/// One item of a target's SOURCES list in CMake order: a source file (absolute, or relative to the source root; a
/// generated path listed in `generated`) or a $<TARGET_OBJECTS>/${P4_TABLES} reference. The link line follows this order.
pub const LinkInput = union(enum) {
    source: []const u8,
    objects: ObjectsRef,
};

pub const ObjectLibrarySpec = struct {
    name: []const u8,
    /// the CMake binary subdirectory of the target ("tests/" for tests/cmake, "" for the root)
    dir: []const u8 = "tests/",
    /// absolute, or relative to the source root
    sources: []const []const u8 = &.{},
    generated: []const GeneratedFiles = &.{},
    source_props: []const SourceProps = &.{},
    defines: []const []const u8 = &.{},
    includes: []const []const u8 = &.{},
    system_includes: []const []const u8 = &.{},
    compile_options: []const []const u8 = &.{},
    launcher: Launcher = .none,
    after: []const *Step = &.{},
};

pub const ObjectLibrary = struct {
    spec: ObjectLibrarySpec,
    model: Model,
    objs: []const Obj,
    set: *Set,
    /// the per-model copies p4_host64_exe makes (${obj}${suffix})
    clones: [3]?*ObjectLibrary = .{ null, null, null },
};

pub const ExecutableSpec = struct {
    /// the CMake target name; the artifact key (graph.artifacts) and the exe:<name> step
    name: []const u8,
    dir: []const u8 = "tests/",
    /// RUNTIME_OUTPUT_DIRECTORY relative to the build tree (default: `dir`)
    output_dir: ?[]const u8 = null,
    /// SUFFIX (default: ".exe" for Windows models, none otherwise)
    suffix: ?[]const u8 = null,
    sources: []const []const u8 = &.{},
    generated: []const GeneratedFiles = &.{},
    source_props: []const SourceProps = &.{},
    objects: []const ObjectsRef = &.{},
    /// The SOURCES list in CMake order when compiled sources and object references interleave (e.g. test.c, ${P4_TABLES},
    /// r5900.c, $<TARGET_OBJECTS:p4decomp>...); it replaces `sources` and `objects`, which must then be empty. Without
    /// it the link order is `sources`, then `objects`, then the compiled `generated` files not named in `sources`.
    link_order: []const LinkInput = &.{},
    defines: []const []const u8 = &.{},
    includes: []const []const u8 = &.{},
    system_includes: []const []const u8 = &.{},
    compile_options: []const []const u8 = &.{},
    link_options: []const []const u8 = &.{},
    /// tools/mods clears inherited directory compile and link options.
    host_adapter: bool = false,
    libs: []const Lib = &.{},
    /// Extra import libraries only for Windows models of this executable.
    windows_libs: []const []const u8 = &.{},
    /// p4_ps2_exe: the `.ps2ram` section and image layout of the 32-bit Windows build (nothing elsewhere)
    ps2_exe: bool = false,
    /// LINKER_LANGUAGE CXX
    cxx: bool = false,
    after: []const *Step = &.{},
};

/// p4_ovr_test64(<exe> <test source> OVERRIDES <addr>... [FLAGS <flag>...] [SOURCES <source>...]).
pub const OverrideTestSpec = struct {
    exe: []const u8,
    /// relative to the source root, e.g. "tests/qsort_test.c"
    test_source: []const u8,
    overrides: []const []const u8,
    flags: []const []const u8 = &.{},
    sources: []const []const u8 = &.{},
};

pub const LibraryRef = struct {
    name: []const u8,
    path: []const u8,
    lazy: LazyPath,
    ready: *Step,
};

/// The configure variables the test fragments read, for one model.
pub const Vars = struct {
    root: []const u8,
    build_dir: []const u8,
    gen_dir: []const u8,
    decomp: []const u8,
    python: []const u8,
    cc: []const u8,
    cxx: []const u8,
    /// ${P4_OS_SRC} of the model (os_win32.c for the Windows models)
    os_src: []const u8,
    /// ${P4_PORT_DEFINES}
    port_defines: []const []const u8,
    /// ${P4_TABLES} / ${P4_TABLES64} / ${P4W_TABLES} (null: the model has no game image)
    tables_c: ?[]const u8,
    /// ${P4_PNG_SOURCES}
    png_sources: []const []const u8,
    /// ${P4_FFMPEG_INC}
    ffmpeg_inc: []const u8,
    /// ${P4_PS2RAM_SRC} (null outside the 32-bit Windows build)
    ps2ram_src: ?[]const u8,
    /// ${P4_EE_FLOAT_OPS} when P4_EE_FLOAT is on, else null
    ee_float_ops: ?[]const u8,
    /// ${P4_PE_REPRO_LINK_FLAGS}
    pe_repro_link_flags: []const []const u8,
    /// ${P4_MOD_BRIDGES}, written by the mirror (order after `mirror`)
    mod_bridges: []const u8,
    mirror: *Step,
    /// the 64-bit mirror tree (${P4_G64}) as a dependency-carrying path, when built
    g64: ?LazyPath,
    /// what addExecutable(model) already applies to every compile and link of the model, after the tree's own options:
    /// P4_X64_FLAGS / P4W_FLAGS (empty for i386) and P4_X64_LINK_FLAGS / P4W_LINK_FLAGS
    model_flags: []const []const u8,
    model_link_flags: []const []const u8,
    /// P4_X64_TARGET_FLAGS / --target=x86_64-w64-mingw32 -m64 (empty for i386)
    target_flags: []const []const u8,
    /// P4_PORT64_OPT / P4_PORT64W_OPT
    port64_opt: []const u8,
    /// P4_PORT64_LAUNCHER / P4_PORT64W_LAUNCHER (one word, empty when P4_EE_FLOAT is off)
    port64_launcher: []const u8,
    ee_float: bool,
    /// the model's executable directory relative to the build tree (P4W_OUT: "win64/")
    out_dir: []const u8,
    /// ${P4_PORT64_TOOLS}
    port64_tools: []const []const u8,
    /// the DLLs beside the model's executables (p4pc64w_dlls; the i386 Windows DLLs are part of `artifacts`), if any
    runtime_ready: ?*Step,
};

const Image = struct {
    decomp: []const Obj,
    decomp_set: *Set,
    port: []const Obj,
    portee: []const Obj,
    port_set: *Set,
    tables: Generated,
    tables_c: []const u8,
    ad: ?Adapters,
    sdl3: []const u8,
    avcodec: []const u8,
    avutil: []const u8,
    swscale: []const u8,
    after: []const *Step,
};

const ptr_errors = [_][]const u8{ "-Werror=pointer-to-int-cast", "-Werror=int-to-pointer-cast", "-Werror=int-conversion", "-Werror=void-pointer-to-int-cast", "-Werror=int-to-void-pointer-cast" };

fn testAvailable(c: *Ctx, model: Model) ?[]const u8 {
    return switch (model) {
        .i386 => if (c.has_i386 or c.apple) null else if (c.android64)
            "the 64-bit Android build ends at 350-port64-player.cmake: no test targets exist"
        else
            c.fmt("profile {s} builds only the 64-bit model: its plain (host) test targets are not modelled by the zig build graph", .{c.profile}),
        .m64 => if (c.pm.link64) null else "the 64-bit test targets need the linked 64-bit model (-Dport64-link=true, -Dport64=true)",
        .m64w => if (c.pm.link64w) null else "the Windows x64 test targets need -Dport64-link=true, the Windows x64 mirror and -Dwin64-prefix with lib/",
    };
}

fn modelOsSrc(c: *Ctx, model: Model) []const u8 {
    return if (model == .m64w) c.src("src/platform/os/os_win32.c") else c.os_src;
}

const ModelFlags = struct { c: []const []const u8, zig: []const []const u8, link: []const []const u8, system: []const []const u8 };

/// The directory-level compile and link options every target of the tree inherits, then the model's own (P4_X64_FLAGS /
/// P4W_FLAGS and their link flags), as a 64-bit target of the tests directory has them.
fn modelFlags(c: *Ctx, model: Model) ModelFlags {
    const root_link = c.list(&.{ c.base_link, c.target_link_flags });
    return switch (model) {
        .i386 => .{ .c = c.global_c, .zig = c.global_zig, .link = root_link, .system = c.global_system_includes },
        .m64 => blk: {
            const f = c.list(&.{ c.pm.x64_target_flags, c.pm.x64_extra, &.{"-DP4_GUEST_CHECK=1"} });
            break :blk .{ .c = c.list(&.{ c.global_c, f }), .zig = c.list(&.{ c.global_zig, f }), .link = c.list(&.{ root_link, c.pm.x64_link }), .system = c.global_system_includes };
        },
        .m64w => blk: {
            const f: []const []const u8 = &.{ "--target=x86_64-w64-mingw32", "-m64", "-DP4_GUEST_CHECK=1" };
            break :blk .{
                .c = c.list(&.{ c.global_c, f }),
                .zig = c.list(&.{ c.global_zig, f }),
                .link = c.list(&.{ root_link, &.{ "--target=x86_64-w64-mingw32", "-m64" }, c.pe_repro }),
                .system = c.list(&.{ c.global_system_includes, &.{c.join(&.{ c.o.win64_prefix, "include" })} }),
            };
        },
    };
}

fn absSource(c: *Ctx, s: []const u8) []const u8 {
    return if (std.fs.path.isAbsolute(s)) s else c.src(s);
}

fn launcherWords(c: *Ctx, l: Launcher, model: Model) []const []const u8 {
    return switch (l) {
        .none => &.{},
        .ee_float => |modes| c.strs(&.{ c.o.python, c.src("tools/ee_float_cc.py"), c.fmt("--modes={s}", .{modes}) }),
        .printf => |p| if (c.win32 and model == .i386)
            c.strs(&.{ c.o.python, c.src("tools/ee_float_cc.py"), c.fmt("--modes={s}", .{p.modes}), c.fmt("--printf-root={s}", .{p.root}), "--printf-symbol-prefix=_" })
        else
            c.strs(&.{ c.o.python, c.src("tools/ee_float_cc.py"), c.fmt("--modes={s}", .{p.modes}), c.fmt("--printf-root={s}", .{p.root}) }),
    };
}

const CompileTarget = struct {
    name: []const u8,
    dir: []const u8,
    sources: []const []const u8,
    generated: []const GeneratedFiles,
    props: []const SourceProps,
    defines: []const []const u8,
    includes: []const []const u8,
    system: []const []const u8,
    options: []const []const u8,
    launcher: Launcher,
    host_adapter: bool = false,
    after: []const *Step,
};

/// The sources of one target, each with the directory's and model's flags, then the target's, then the source's own.
fn compileTarget(c: *Ctx, set: *Set, model: Model, t: CompileTarget) []const Obj {
    const model_flags = modelFlags(c, model);
    const adapter_flags = if (t.host_adapter) adapterFlags(c, c.pm, model) else &.{};
    const mf: ModelFlags = if (t.host_adapter)
        .{ .c = adapter_flags, .zig = adapter_flags, .link = adapter_flags, .system = model_flags.system }
    else
        model_flags;
    var objs: std.ArrayList(Obj) = .empty;
    const Item = struct { path: []const u8, gen: ?GeneratedFiles };
    var items: std.ArrayList(Item) = .empty;
    // A generated file named in `sources` keeps that position (and its generator's dependencies); the others follow.
    for (t.sources) |s| {
        const path = absSource(c, s);
        var gen: ?GeneratedFiles = null;
        for (t.generated) |g| for (g.paths) |p| {
            if (std.mem.eql(u8, p, path)) gen = g;
        };
        items.append(c.a, .{ .path = path, .gen = gen }) catch @panic("OOM");
    }
    for (t.generated) |g| for (g.paths) |p| {
        var listed = false;
        for (t.sources) |s| listed = listed or std.mem.eql(u8, absSource(c, s), p);
        if (!listed) items.append(c.a, .{ .path = p, .gen = g }) catch @panic("OOM");
    };
    for (items.items) |item| {
        const path = item.path;
        var defines: std.ArrayList([]const u8) = .empty;
        defines.appendSlice(c.a, t.defines) catch @panic("OOM");
        var includes: std.ArrayList([]const u8) = .empty;
        includes.appendSlice(c.a, c.global_includes) catch @panic("OOM");
        includes.appendSlice(c.a, t.includes) catch @panic("OOM");
        var options: std.ArrayList([]const u8) = .empty;
        const lang = langOf(path);
        options.appendSlice(c.a, if (lang == .zig) mf.zig else if (lang == .asm_) removeCompat(c, mf.c) else mf.c) catch @panic("OOM");
        options.appendSlice(c.a, t.options) catch @panic("OOM");
        for (t.props) |p| if (std.mem.eql(u8, absSource(c, p.path), path)) {
            defines.appendSlice(c.a, p.defines) catch @panic("OOM");
            includes.appendSlice(c.a, p.includes) catch @panic("OOM");
            options.appendSlice(c.a, p.compile_options) catch @panic("OOM");
        };
        var after: std.ArrayList(*Step) = .empty;
        after.appendSlice(c.a, t.after) catch @panic("OOM");
        // a source of the mirror or another generated file of gen/: written by the mirror (CMake: the p4_mirror target)
        if (std.mem.startsWith(u8, path, c.gen)) after.append(c.a, &c.mirror.step) catch @panic("OOM");
        // an include directory or forced include of gen/ (the mirror's decomp/include): every compile waits for the mirror
        for ([_][]const []const u8{ t.includes, t.options }) |words| for (words) |w| {
            if (std.mem.indexOf(u8, w, c.gen) != null) after.append(c.a, &c.mirror.step) catch @panic("OOM");
        };
        var inputs: std.ArrayList(LazyPath) = .empty;
        if (item.gen) |g| {
            after.append(c.a, g.ready) catch @panic("OOM");
            inputs.appendSlice(c.a, g.cached) catch @panic("OOM");
        }
        objs.append(c.a, compile(c, set, path, .{
            .target = t.name,
            .dir_prefix = t.dir,
            .defines = defines.items,
            .includes = includes.items,
            .system_includes = c.list(&.{ mf.system, t.system }),
            .flags = options.items,
            .launcher = if (lang == .c) launcherWords(c, t.launcher, model) else &.{},
            .launcher_inputs = switch (t.launcher) {
                .none => &.{},
                .ee_float => &.{"tools/ee_float_cc.py"},
                .printf => &.{ "tools/ee_float_cc.py", "tools/port64/printf_rewrite.py", "tools/port64/gptr_rewrite.py" },
            },
            .after = after.items,
            .file_inputs = inputs.items,
            .zig_default_target = c.default_triple,
        })) catch @panic("OOM");
    }
    return objs.items;
}

fn addObjectLibraryImpl(c: *Ctx, model: Model, spec: ObjectLibrarySpec) *ObjectLibrary {
    if (c.finished) std.debug.panic("zigbuild/core.zig: object library {s} added after graph.finish()", .{spec.name});
    const name = if (model == .i386) spec.name else c.fmt("{s}{s}", .{ spec.name, model.suffix() });
    const set = objectSet(c, c.fmt("objects {s}", .{name}));
    const objs = compileTarget(c, set, model, .{
        .name = name,
        .dir = spec.dir,
        .sources = spec.sources,
        .generated = spec.generated,
        .props = spec.source_props,
        .defines = spec.defines,
        .includes = spec.includes,
        .system = spec.system_includes,
        .options = spec.compile_options,
        .launcher = spec.launcher,
        .after = spec.after,
    });
    const lib = c.a.create(ObjectLibrary) catch @panic("OOM");
    lib.* = .{ .spec = spec, .model = model, .objs = objs, .set = set };
    manifestTarget(c, name, if (std.mem.startsWith(u8, spec.dir, "tests")) "test_object" else "object", model, &.{}, objs, &.{}, &.{}, false, "");
    return lib;
}

/// ${obj}${suffix} of p4_host64_exe: the object library's sources for the model (os_win32.c for Windows x64), its own
/// options and the model flags, P4_PORT_DEFINES with its definitions, its include directories (no /usr/ ones for Windows
/// x64) and its launcher.
fn libraryFor(c: *Ctx, lib: *ObjectLibrary, model: Model) *ObjectLibrary {
    if (lib.model == model) return lib;
    if (lib.clones[@intFromEnum(model)]) |clone| return clone;
    if (c.twin_libraries.get(lib.spec.name)) |given| if (given[@intFromEnum(model)]) |clone| {
        lib.clones[@intFromEnum(model)] = clone;
        return clone;
    };
    if (c.twin_required.contains(lib.spec.name))
        std.debug.panic("zigbuild/core.zig: a {s} host twin needs {s}{s}, which no fragment registered with setTwinLibrary (call it before the twin)", .{ model.suffix(), lib.spec.name, model.suffix() });
    c.cloned.put(c.a, c.fmt("{s}|{s}", .{ lib.spec.name, model.suffix() }), {}) catch @panic("OOM");
    var spec = lib.spec;
    var srcs: std.ArrayList([]const u8) = .empty;
    for (spec.sources) |s| srcs.append(c.a, if (model == .m64w) osWin32(c, absSource(c, s)) else absSource(c, s)) catch @panic("OOM");
    spec.sources = srcs.items;
    spec.defines = c.list(&.{ c.port_defines, spec.defines });
    if (model == .m64w) spec.includes = withoutUsr(c, spec.includes);
    const clone = addObjectLibraryImpl(c, model, spec);
    lib.clones[@intFromEnum(model)] = clone;
    return clone;
}

fn osWin32(c: *Ctx, s: []const u8) []const u8 {
    if (std.mem.endsWith(u8, s, "os/os_posix.c")) return c.fmt("{s}os_win32.c", .{s[0 .. s.len - "os_posix.c".len]});
    return s;
}

fn withoutUsr(c: *Ctx, dirs: []const []const u8) []const []const u8 {
    var out: std.ArrayList([]const u8) = .empty;
    for (dirs) |d| if (!std.mem.startsWith(u8, d, "/usr/")) out.append(c.a, d) catch @panic("OOM");
    return out.items;
}

fn movieSpec(c: *Ctx) ObjectLibrarySpec {
    return .{
        .name = "p4movie",
        .dir = "",
        .sources = c.strs(&.{ c.src("src/movie/sfd_demux.c"), c.src("src/movie/mpeg2_dec.c"), c.src("src/movie/adx_dec.c") }),
        .defines = c.port_defines,
        .includes = c.strs(&.{ c.src("src/platform/files"), c.ffmpeg_inc }),
    };
}

fn movieLibrary(c: *Ctx, model: Model) *ObjectLibrary {
    if (c.movie_libs[@intFromEnum(model)]) |l| return l;
    const base = c.movie_libs[@intFromEnum(Model.i386)] orelse blk: {
        // a profile without the i386 player (macOS): the root's p4movie for its own model
        const l = addObjectLibraryImpl(c, .i386, movieSpec(c));
        c.movie_libs[@intFromEnum(Model.i386)] = l;
        break :blk l;
    };
    const l = libraryFor(c, base, model);
    c.movie_libs[@intFromEnum(model)] = l;
    return l;
}

const ExtraObjects = struct { objs: []const Obj, set: *Set };

fn addExecutableImpl(c: *Ctx, model: Model, spec: ExecutableSpec, extra: []const ExtraObjects, twin_libs: []const []const u8) ?Artifact {
    if (c.finished) std.debug.panic("zigbuild/core.zig: executable {s} added after graph.finish()", .{spec.name});
    if (testAvailable(c, model) != null) return null;
    if (c.test_names.contains(spec.name)) std.debug.panic("zigbuild/core.zig: executable {s} defined twice", .{spec.name});
    c.test_names.put(c.a, spec.name, {}) catch @panic("OOM");
    const image = c.images[@intFromEnum(model)];
    var missing: ?[]const u8 = null;

    // The link order: CMake links SOURCES in order, the compiled sources and the $<TARGET_OBJECTS>/${P4_TABLES} items
    // interleaved as listed. `link_order` gives that order; otherwise the sources come first, then `objects`.
    const Slot = union(enum) { src: usize, ref: usize };
    var order: std.ArrayList(Slot) = .empty;
    var sources: std.ArrayList([]const u8) = .empty;
    var refs: std.ArrayList(ObjectsRef) = .empty;
    if (spec.link_order.len != 0) {
        if (spec.sources.len != 0 or spec.objects.len != 0) std.debug.panic("zigbuild/core.zig: {s}: link_order replaces sources and objects", .{spec.name});
        for (spec.link_order) |input| switch (input) {
            .source => |s| {
                order.append(c.a, .{ .src = sources.items.len }) catch @panic("OOM");
                sources.append(c.a, absSource(c, s)) catch @panic("OOM");
            },
            .objects => |ref| if (ref == .tables) {
                // ${P4_TABLES} is a source of the target, compiled at this position
                if (image) |img| {
                    order.append(c.a, .{ .src = sources.items.len }) catch @panic("OOM");
                    sources.append(c.a, img.tables_c) catch @panic("OOM");
                }
                refs.append(c.a, ref) catch @panic("OOM");
            } else {
                order.append(c.a, .{ .ref = refs.items.len }) catch @panic("OOM");
                refs.append(c.a, ref) catch @panic("OOM");
            },
        };
    } else {
        for (spec.sources) |s| {
            order.append(c.a, .{ .src = sources.items.len }) catch @panic("OOM");
            sources.append(c.a, absSource(c, s)) catch @panic("OOM");
        }
        for (spec.objects, 0..) |ref, i| {
            if (ref != .tables) order.append(c.a, .{ .ref = i }) catch @panic("OOM");
            refs.append(c.a, ref) catch @panic("OOM");
        }
    }
    var after: std.ArrayList(*Step) = .empty;
    after.appendSlice(c.a, spec.after) catch @panic("OOM");
    var link_flags: std.ArrayList([]const u8) = .empty;
    const mf = if (spec.host_adapter) adapterFlags(c, c.pm, model) else modelFlags(c, model).link;
    link_flags.appendSlice(c.a, mf) catch @panic("OOM");
    if (spec.ps2_exe and c.win32 and model == .i386) {
        // target_sources(${P4_PS2RAM_SRC}); CMake keeps a source once, so an inherited, already listed one stays in place
        const ps2ram = c.src("src/platform/os/win32_ps2ram.S");
        var listed = false;
        for (sources.items) |s| listed = listed or std.mem.eql(u8, s, ps2ram);
        if (!listed) {
            order.append(c.a, .{ .src = sources.items.len }) catch @panic("OOM");
            sources.append(c.a, ps2ram) catch @panic("OOM");
        }
        link_flags.appendSlice(c.a, &.{ "-Wl,--image-base=0xF0000", "-Wl,--section-alignment=0x10000", "-Wl,--disable-dynamicbase", "-Wl,--disable-reloc-section", "-Wl,--stack=8388608", c.fmt("-Wl,--Xlink=/sectionlayout:{s}", .{c.join(&.{ c.bdir, "ps2ram_layout.txt" })}) }) catch @panic("OOM");
    }
    link_flags.appendSlice(c.a, spec.link_options) catch @panic("OOM");

    // ${P4_TABLES} as a source: compiled with the target's flags, after the tables
    var gens: std.ArrayList(GeneratedFiles) = .empty;
    gens.appendSlice(c.a, spec.generated) catch @panic("OOM");
    // per object reference: the set and objects it links (null for ${P4_TABLES}, compiled as a source)
    const Segment = struct { set: *Set, objs: []const Obj };
    var segments: std.ArrayList(?Segment) = .empty;
    var adapters: ?Adapters = null;
    for (refs.items) |ref| switch (ref) {
        .tables => {
            segments.append(c.a, null) catch @panic("OOM");
            if (image) |img| {
                gens.append(c.a, .{ .paths = c.strs(&.{img.tables_c}), .ready = img.tables.ready, .cached = c.a.dupe(LazyPath, &.{img.tables.cached}) catch @panic("OOM") }) catch @panic("OOM");
            } else {
                missing = "${P4_TABLES}";
            }
        },
        .p4decomp, .p4port, .p4portee => if (image) |img| {
            switch (ref) {
                .p4decomp => segments.append(c.a, .{ .set = img.decomp_set, .objs = img.decomp }) catch @panic("OOM"),
                .p4port => {
                    segments.append(c.a, .{ .set = img.port_set, .objs = img.port }) catch @panic("OOM");
                    adapters = img.ad;
                },
                else => segments.append(c.a, .{ .set = img.port_set, .objs = img.portee }) catch @panic("OOM"),
            }
            after.appendSlice(c.a, img.after) catch @panic("OOM");
        } else {
            segments.append(c.a, null) catch @panic("OOM");
            missing = @tagName(ref);
        },
        .p4movie => {
            const l = movieLibrary(c, model);
            segments.append(c.a, .{ .set = l.set, .objs = l.objs }) catch @panic("OOM");
        },
        .library => |lib| {
            const l = libraryFor(c, lib, model);
            segments.append(c.a, .{ .set = l.set, .objs = l.objs }) catch @panic("OOM");
        },
        .files => |f| {
            const fset = c.newSet(c.fmt("prebuilt objects of {s}", .{spec.name}), &.{});
            var fobjs: std.ArrayList(Obj) = .empty;
            for (f.paths, f.cached) |p, cached| fobjs.append(c.a, .{ .fixed = p, .cached = cached }) catch @panic("OOM");
            fset.stage.step.dependOn(f.ready);
            segments.append(c.a, .{ .set = fset, .objs = fobjs.items }) catch @panic("OOM");
        },
    };

    const set = c.newSet(c.fmt("objects {s}", .{spec.name}), &.{});
    const own = compileTarget(c, set, model, .{
        .name = spec.name,
        .dir = spec.dir,
        .sources = sources.items,
        .generated = gens.items,
        .props = spec.source_props,
        .defines = spec.defines,
        .includes = spec.includes,
        .system = spec.system_includes,
        .options = spec.compile_options,
        .launcher = .none,
        .host_adapter = spec.host_adapter,
        .after = after.items,
    });
    c.steps.compile_only.dependOn(&set.stage.step);

    // target_link_libraries, with the model's files for the library names
    var system: std.ArrayList([]const u8) = .empty;
    const sdl3 = if (image) |img| img.sdl3 else c.sdl3;
    const avcodec = if (image) |img| img.avcodec else c.avcodec;
    const avutil = if (image) |img| img.avutil else c.avutil;
    const swscale = if (image) |img| img.swscale else c.swscale;
    var libs: std.ArrayList(Library) = .empty;
    for (spec.libs) |l| switch (l) {
        .m => system.append(c.a, "-lm") catch @panic("OOM"),
        .sdl3 => system.append(c.a, sdl3) catch @panic("OOM"),
        .avcodec => system.append(c.a, avcodec) catch @panic("OOM"),
        .avutil => system.append(c.a, avutil) catch @panic("OOM"),
        .swscale => system.append(c.a, swscale) catch @panic("OOM"),
        .ffmpeg => system.appendSlice(c.a, &.{ avcodec, swscale, avutil }) catch @panic("OOM"),
        .pthread => system.append(c.a, "-lpthread") catch @panic("OOM"),
        .dl => system.append(c.a, "-ldl") catch @panic("OOM"),
        .rt => system.append(c.a, "-lrt") catch @panic("OOM"),
        .raw => |r| system.append(c.a, r) catch @panic("OOM"),
        .target => |n| if (c.libraries.get(n)) |lib| {
            libs.append(c.a, lib) catch @panic("OOM");
        } else {
            missing = c.fmt("library target {s}", .{n});
        },
    };
    if (model == .m64w or (model == .i386 and c.win32))
        system.appendSlice(c.a, spec.windows_libs) catch @panic("OOM");
    for (twin_libs) |l| system.append(c.a, l) catch @panic("OOM");
    var cxx = spec.cxx;
    var adapter_names: []const []const u8 = &.{};
    if (adapters) |a| {
        libs.appendSlice(c.a, a.libs) catch @panic("OOM");
        link_flags.appendSlice(c.a, a.link_flags) catch @panic("OOM");
        cxx = cxx or a.cxx;
        adapter_names = adapterLibraries(model, true, c.o.mod_support and c.o.luau);
    }
    if (c.win32 and model == .i386) system.appendSlice(c.a, &.{ "-Wl,-Bstatic", "-lwinpthread", "-Wl,-Bdynamic" }) catch @panic("OOM");

    const windows = (c.win32 and model == .i386) or model == .m64w;
    const suffix = spec.suffix orelse (if (windows) ".exe" else "");
    const out_dir = spec.output_dir orelse spec.dir;
    if (missing) |what| {
        // the target needs a part of the game image this profile does not build: it exists, and fails
        after.append(c.a, &c.b.addFail(c.fmt("{s}: {s} of model {s} is not built in profile {s}", .{ spec.name, what, if (model == .i386) "i386" else model.suffix(), c.profile })).step) catch @panic("OOM");
    }
    // the link inputs in order: each slot's compiled source or referenced objects, then the compiled generated files not
    // named among the sources, then the extra (override) objects
    var ext_sets: std.ArrayList(*Set) = .empty;
    var ext_objs: std.ArrayList([]const Obj) = .empty;
    for (order.items) |slot| switch (slot) {
        .src => |i| {
            ext_sets.append(c.a, set) catch @panic("OOM");
            ext_objs.append(c.a, own[i .. i + 1]) catch @panic("OOM");
        },
        .ref => |i| if (segments.items[i]) |seg| {
            ext_sets.append(c.a, seg.set) catch @panic("OOM");
            ext_objs.append(c.a, seg.objs) catch @panic("OOM");
        },
    };
    if (own.len > sources.items.len) {
        ext_sets.append(c.a, set) catch @panic("OOM");
        ext_objs.append(c.a, own[sources.items.len..]) catch @panic("OOM");
    }
    for (extra) |e| {
        ext_sets.append(c.a, e.set) catch @panic("OOM");
        ext_objs.append(c.a, e.objs) catch @panic("OOM");
    }
    const art = link(c, .{
        .name = spec.name,
        .out_rel = c.fmt("{s}{s}{s}", .{ out_dir, spec.name, suffix }),
        .model = model,
        .cxx = cxx,
        .objs_set = set,
        .objs = &.{},
        .extra_sets = ext_sets.items,
        .extra_objs = ext_objs.items,
        .libs = libs.items,
        .link_flags = link_flags.items,
        .system_libs = system.items,
        .after = after.items,
        .test_target = true,
    });
    c.artifacts.put(c.a, spec.name, art) catch @panic("OOM");
    // the DLLs / ANGLE the executable loads from its own directory (p4_dlls, p4_macos_runtime, p4pc64w_dlls)
    if (model == .m64w) {
        if (c.dlls_ready) |s| art.view.dependOn(s);
    } else if (c.runtime_ready) |s| art.view.dependOn(s);
    manifestTarget(c, spec.name, if (std.mem.startsWith(u8, spec.dir, "tests")) "test_executable" else "executable", model, sources.items, own, libs.items, adapter_names, cxx, art.path);
    return art;
}

/// p4_host64_exe(<exe> <suffix>): the 64-bit twin <name>64 / <name>64w of an i386 test executable.
fn hostTwinImpl(c: *Ctx, spec: ExecutableSpec, model: Model) ?Artifact {
    if (model == .i386) std.debug.panic("zigbuild/core.zig: hostTwin of {s} needs .m64 or .m64w", .{spec.name});
    if (testAvailable(c, model) != null) return null;
    // p4_host64_exe walks the exe's SOURCES in order: sources stay in place (os_win32.c for Windows x64), each
    // $<TARGET_OBJECTS:obj> becomes its ${obj}${suffix} in place, the game objects and tables drop out; then
    // guest.c and the OS source when absent (no game image), or "${tables}" ${objects} (the 64-bit image) at the end.
    var input: std.ArrayList(LinkInput) = .empty;
    if (spec.link_order.len != 0) {
        input.appendSlice(c.a, spec.link_order) catch @panic("OOM");
    } else {
        for (spec.sources) |s| input.append(c.a, .{ .source = s }) catch @panic("OOM");
        for (spec.objects) |ref| input.append(c.a, .{ .objects = ref }) catch @panic("OOM");
    }
    var mirror = false;
    var order: std.ArrayList(LinkInput) = .empty;
    var sources: std.ArrayList([]const u8) = .empty;
    for (input.items) |item| switch (item) {
        .source => |s| {
            const abs = absSource(c, s);
            if (std.mem.endsWith(u8, abs, "win32_ps2ram.S")) continue;
            const mapped = if (model == .m64w) osWin32(c, abs) else abs;
            sources.append(c.a, mapped) catch @panic("OOM");
            order.append(c.a, .{ .source = mapped }) catch @panic("OOM");
        },
        .objects => |ref| switch (ref) {
            .p4decomp, .p4port, .p4portee, .tables => mirror = true,
            else => order.append(c.a, item) catch @panic("OOM"),
        },
    };
    if (mirror) {
        order.appendSlice(c.a, &.{ .{ .objects = .tables }, .{ .objects = .p4decomp }, .{ .objects = .p4port }, .{ .objects = .p4portee } }) catch @panic("OOM");
    } else for ([_][]const u8{ c.src("src/platform/guest.c"), modelOsSrc(c, model) }) |extra| {
        var present = false;
        for (sources.items) |s| present = present or std.mem.eql(u8, s, extra);
        if (!present) order.append(c.a, .{ .source = extra }) catch @panic("OOM");
    }
    var options: std.ArrayList([]const u8) = .empty;
    if (mirror) options.append(c.a, "-fno-builtin") catch @panic("OOM");
    options.appendSlice(c.a, spec.compile_options) catch @panic("OOM");
    options.appendSlice(c.a, &ptr_errors) catch @panic("OOM");
    var link_options: std.ArrayList([]const u8) = .empty;
    if (model == .m64w) link_options.append(c.a, "-Wl,--stack=8388608") catch @panic("OOM");
    for (spec.link_options) |l| {
        const gc = std.mem.eql(u8, l, "-Wl,--gc-sections") or std.mem.eql(u8, l, "LINKER:--gc-sections");
        link_options.append(c.a, if (c.apple and gc) "-Wl,-dead_strip" else if (std.mem.eql(u8, l, "LINKER:--gc-sections")) "-Wl,--gc-sections" else l) catch @panic("OOM");
    }
    var libs: std.ArrayList(Lib) = .empty;
    libs.append(c.a, .m) catch @panic("OOM");
    for (spec.libs) |l| switch (l) {
        .m => {},
        .pthread, .dl, .rt => if (model != .m64w) libs.append(c.a, l) catch @panic("OOM"),
        else => libs.append(c.a, l) catch @panic("OOM"),
    };
    if (mirror) libs.appendSlice(c.a, &.{ .sdl3, .avcodec, .swscale, .avutil }) catch @panic("OOM");
    var twin = spec;
    twin.name = c.fmt("{s}{s}", .{ spec.name, model.suffix() });
    twin.sources = &.{};
    twin.objects = &.{};
    twin.link_order = order.items;
    twin.defines = c.list(&.{ c.port_defines, spec.defines });
    twin.includes = if (model == .m64w) withoutUsr(c, spec.includes) else spec.includes;
    twin.compile_options = options.items;
    twin.link_options = link_options.items;
    twin.libs = libs.items;
    twin.ps2_exe = false;
    if (model == .m64w) {
        twin.output_dir = "win64/";
        twin.suffix = ".exe";
    }
    const extra_libs: []const []const u8 = if (model == .m64w) &.{ "-Wl,-Bstatic", "-lwinpthread", "-Wl,-Bdynamic", "-lkernel32", "-luser32" } else &.{};
    return addExecutableImpl(c, model, twin, &.{}, extra_libs);
}

fn overrideTestImpl(c: *Ctx, model: Model, spec: OverrideTestSpec) ?Artifact {
    if (model == .i386) std.debug.panic("zigbuild/core.zig: p4_ovr_test64 of {s} needs .m64 or .m64w", .{spec.exe});
    if (testAvailable(c, model) != null) return null;
    const sfx = model.suffix();
    const target_string = if (model == .m64w) "--target=x86_64-w64-mingw32 -m64" else joinWords(c, c.pm.x64_target_flags);
    const opt = if (model == .m64w) c.o.port64w_opt else c.o.port64_opt;
    const launcher: []const u8 = if (c.o.ee_float) c.fmt("--launcher={s}|{s}|--modes={s}", .{ c.o.python, c.src("tools/ee_float_cc.py"), c.o.ee_float_ops }) else "";
    const set = c.newSet(c.fmt("overrides {s}{s}", .{ spec.exe, sfx }), &.{});
    var objs: std.ArrayList(Obj) = .empty;
    for (spec.overrides) |addr| {
        const run = c.pyTool(c.fmt("override {s}{s} {s}", .{ spec.exe, sfx, addr }), &.{ "-W", "ignore" }, "tools/port64/unit64.py", .{ .depfile_out = true });
        run.addArgs(&.{ "build", "--gen", c.gen, "--root", c.root, "--decomp", c.o.decomp, "--src", c.src(c.fmt("overrides/{s}.c", .{addr})) });
        const dir = run.addOutputDirectoryArg2("ovr", .{ .prefix = "--out=", .suffix = c.fmt("/{s}.o", .{addr}) });
        run.addArgs(&.{ "--clang", c.cc, "--opt", opt, "--check" });
        if (launcher.len != 0) run.addArg(launcher);
        run.addArgs(&.{ "--cflag=-include", c.fmt("--cflag={s}", .{c.src("tests/override_test_types.h")}) });
        for (spec.flags) |f| run.addArg(c.fmt("--cflag={s}", .{f}));
        run.setEnvironmentVariable("P4_PORT64_TARGET", target_string);
        if (model == .m64) for (c.pm.x64_env) |e| if (!std.mem.eql(u8, e[0], "P4_PORT64_TARGET")) run.setEnvironmentVariable(e[0], e[1]);
        c.useCompiler(run);
        c.inputs(run, &port64_tools);
        c.inputs(run, &.{ c.fmt("overrides/{s}.c", .{addr}), "tests/override_test_types.h" });
        if (launcher.len != 0) c.inputs(run, &.{ "tools/ee_float_cc.py", "tools/port64/printf_rewrite.py" });
        run.step.dependOn(&c.mirror.step);
        const fixed = c.join(&.{ c.bdir, "tests", c.fmt("ovr{s}", .{sfx}), spec.exe, c.fmt("{s}.o", .{addr}) });
        objs.append(c.a, c.addToSet(set, fixed, dir.path(c.b, c.fmt("{s}.o", .{addr})))) catch @panic("OOM");
    }
    var sources: std.ArrayList([]const u8) = .empty;
    sources.append(c.a, c.src(spec.test_source)) catch @panic("OOM");
    for (spec.sources) |s| sources.append(c.a, absSource(c, s)) catch @panic("OOM");
    if (c.o.ee_float and launcher.len != 0) {
        const ee_float_source = c.src("src/platform/ee_float.c");
        var present = false;
        for (sources.items) |source| {
            if (std.mem.eql(u8, source, ee_float_source)) {
                present = true;
                break;
            }
        }
        if (!present) sources.append(c.a, ee_float_source) catch @panic("OOM");
    }
    // ${P4_GUEST_TEST_SRC} (os_win32.c for Windows x64)
    sources.appendSlice(c.a, &.{ c.src("tests/guest_test.c"), c.src("src/platform/gport.c"), c.src("src/platform/guest.c"), modelOsSrc(c, model) }) catch @panic("OOM");
    const extra_libs: []const []const u8 = if (model == .m64w) &.{ "-lkernel32", "-luser32" } else &.{};
    return addExecutableImpl(c, model, .{
        .name = c.fmt("{s}{s}", .{ spec.exe, sfx }),
        .output_dir = if (model == .m64w) "win64/" else null,
        .suffix = if (model == .m64w) ".exe" else null,
        .sources = sources.items,
        .defines = c.port_defines,
        .includes = c.strs(&.{ c.src("include"), c.src("tests") }),
        .compile_options = &ptr_errors,
        .libs = &.{.m},
    }, &.{.{ .objs = objs.items, .set = set }}, extra_libs);
}

fn modTestGameImpl(c: *Ctx) GeneratedFiles {
    if (c.mod_fixture) |f| return f;
    const run = c.pyTool("mod hook fixture", &.{}, "tests/mod_codegen_fixture.py", .{});
    const dir = run.addOutputDirectoryArg("mod_hook_fixture");
    c.inputs(run, &.{ "tests/mod_hooks_game.c", "tools/mod_codegen.py", "tools/gen_port.py", "include/p4pc/libc_rename.h" });
    const set = c.newSet("mod hook fixture", &.{});
    const fixed_dir = c.join(&.{ c.gen, "mod_hook_fixture" });
    set.stage.addDirectoryArg2(dir, .{ .prefix = c.fmt("{s}=", .{fixed_dir}) });
    const files = GeneratedFiles{
        .paths = c.strs(&.{ c.join(&.{ fixed_dir, "game.c" }), c.join(&.{ fixed_dir, "mod_bridges.c" }) }),
        .ready = &set.stage.step,
        .cached = c.a.dupe(LazyPath, &.{ dir.path(c.b, "game.c"), dir.path(c.b, "mod_bridges.c") }) catch @panic("OOM"),
    };
    c.mod_fixture = files;
    return files;
}


/// Null when test targets of `model` exist in this profile; otherwise why not (CMake's `if(TARGET ...)` conditions).
pub fn available(g: *const Graph, model: Model) ?[]const u8 {
    return testAvailable(g.ctx, model);
}

pub fn vars(g: *const Graph, model: Model) Vars {
    const c = g.ctx;
    const image = c.images[@intFromEnum(model)];
    var png: std.ArrayList([]const u8) = .empty;
    for (png_sources) |s| png.append(c.a, c.src(s)) catch @panic("OOM");
    return .{
        .root = c.root,
        .build_dir = c.bdir,
        .gen_dir = c.gen,
        .decomp = c.o.decomp,
        .python = c.o.python,
        .cc = c.cc,
        .cxx = c.cxx,
        .os_src = modelOsSrc(c, model),
        .port_defines = c.port_defines,
        .tables_c = if (image) |img| img.tables_c else null,
        .png_sources = png.items,
        .ffmpeg_inc = c.ffmpeg_inc,
        .ps2ram_src = if (c.win32 and model == .i386) c.src("src/platform/os/win32_ps2ram.S") else null,
        .ee_float_ops = if (c.o.ee_float) c.o.ee_float_ops else null,
        .pe_repro_link_flags = c.pe_repro,
        .mod_bridges = c.join(&.{ c.gen, "mod_bridges.c" }),
        .mirror = &c.mirror.step,
        .g64 = c.g64_view,
        .model_flags = switch (model) {
            .i386 => &.{},
            .m64 => c.list(&.{ c.pm.x64_target_flags, c.pm.x64_extra, &.{"-DP4_GUEST_CHECK=1"} }),
            .m64w => &.{ "--target=x86_64-w64-mingw32", "-m64", "-DP4_GUEST_CHECK=1" },
        },
        .model_link_flags = switch (model) {
            .i386 => &.{},
            .m64 => c.pm.x64_link,
            .m64w => c.list(&.{ &.{ "--target=x86_64-w64-mingw32", "-m64" }, c.pe_repro }),
        },
        .target_flags = switch (model) {
            .i386 => &.{},
            .m64 => c.pm.x64_target_flags,
            .m64w => &.{ "--target=x86_64-w64-mingw32", "-m64" },
        },
        .port64_opt = if (model == .m64w) c.o.port64w_opt else c.o.port64_opt,
        .port64_launcher = if (c.o.ee_float) c.fmt("--launcher={s}|{s}|--modes={s}", .{ c.o.python, c.src("tools/ee_float_cc.py"), c.o.ee_float_ops }) else "",
        .ee_float = c.o.ee_float,
        .out_dir = if (model == .m64w) "win64/" else "tests/",
        .port64_tools = c.strs(&.{ c.src("tools/port64/unit64.py"), c.src("tools/port64/gptr_rewrite.py") }),
        .runtime_ready = if (model == .m64w) c.dlls_ready else null,
    };
}

pub fn addObjectLibrary(g: *const Graph, model: Model, spec: ObjectLibrarySpec) ?*ObjectLibrary {
    if (testAvailable(g.ctx, model) != null) return null;
    return addObjectLibraryImpl(g.ctx, model, spec);
}

/// The i386 object library `spec` as the base of hostTwin `.library` references: addObjectLibrary(.i386, spec) where the
/// profile builds i386 tests, else a handle that only names the library for its twins (setTwinLibrary copies, or the
/// p4_host64_exe clone made from `spec`). The unbuilt handle's own objects are never linked: its object set fails.
pub fn twinBaseLibrary(g: *const Graph, spec: ObjectLibrarySpec) *ObjectLibrary {
    const c = g.ctx;
    if (testAvailable(c, .i386) == null) return addObjectLibraryImpl(c, .i386, spec);
    const set = c.newSet(c.fmt("unbuilt objects {s}", .{spec.name}), &.{});
    set.stage.step.dependOn(&c.b.addFail(c.fmt("{s}: the i386 object library is not built in profile {s}; only its 64-bit twins are", .{ spec.name, c.profile })).step);
    const lib = c.a.create(ObjectLibrary) catch @panic("OOM");
    lib.* = .{ .spec = spec, .model = .i386, .objs = &.{}, .set = set };
    return lib;
}

/// add_executable in the tests directory for `model` (null when the model has no test targets in this profile). A target
/// that needs a part of the game image the profile lacks exists and fails with that reason.
pub fn addExecutable(g: *const Graph, model: Model, spec: ExecutableSpec) ?Artifact {
    return addExecutableImpl(g.ctx, model, spec, &.{}, &.{});
}

pub fn hostTwin(g: *const Graph, spec: ExecutableSpec, model: Model) ?Artifact {
    return hostTwinImpl(g.ctx, spec, model);
}

pub fn addOverrideTest64(g: *const Graph, model: Model, spec: OverrideTestSpec) ?Artifact {
    return overrideTestImpl(g.ctx, model, spec);
}

pub fn modTestGame(g: *const Graph) GeneratedFiles {
    return modTestGameImpl(g.ctx);
}

/// CROSSCOMPILING_EMULATOR of the targets (CMAKE_CROSSCOMPILING_EMULATOR, -Dcross-emulator; empty by default).
pub fn emulator(g: *const Graph, model: Model) []const []const u8 {
    _ = model;
    return g.ctx.emulator;
}

/// Make `clone` the ${obj}${suffix} that hostTwin links for the object library named `base_name` in `model` (a fragment
/// that builds the 64-bit object library with its own rules, e.g. 220's rewritten printf_abi_calls64). Independent of the
/// order in which the base library and its twins are defined; it panics if a twin already cloned that library.
pub fn setTwinLibrary(g: *const Graph, base_name: []const u8, model: Model, clone: *ObjectLibrary) void {
    const c = g.ctx;
    if (model == .i386) std.debug.panic("zigbuild/core.zig: setTwinLibrary({s}) needs .m64 or .m64w", .{base_name});
    const gop = c.twin_libraries.getOrPut(c.a, base_name) catch @panic("OOM");
    if (!gop.found_existing) gop.value_ptr.* = .{ null, null, null };
    if (gop.value_ptr.*[@intFromEnum(model)] != null) std.debug.panic("zigbuild/core.zig: {s} already has a {s} twin", .{ base_name, model.suffix() });
    if (c.cloned.contains(c.fmt("{s}|{s}", .{ base_name, model.suffix() })))
        std.debug.panic("zigbuild/core.zig: setTwinLibrary({s}, {s}) after a host twin already cloned it", .{ base_name, model.suffix() });
    gop.value_ptr.*[@intFromEnum(model)] = clone;
}

/// Whether setTwinLibrary registered the `model` copy of the object library `base_name`.
pub fn hasTwinLibrary(g: *const Graph, base_name: []const u8, model: Model) bool {
    const given = g.ctx.twin_libraries.get(base_name) orelse return false;
    return given[@intFromEnum(model)] != null;
}

/// Forbid the generic ${obj}${suffix} clone of `base_name`: a host twin that needs it without a setTwinLibrary
/// registration fails configure instead of linking a copy built by the wrong rules.
pub fn requireTwinLibrary(g: *const Graph, base_name: []const u8) void {
    const c = g.ctx;
    var it = c.cloned.keyIterator();
    while (it.next()) |k| if (std.mem.startsWith(u8, k.*, base_name) and k.*.len > base_name.len and k.*[base_name.len] == '|')
        std.debug.panic("zigbuild/core.zig: requireTwinLibrary({s}) after a host twin already cloned it", .{base_name});
    c.twin_required.put(c.a, base_name, {}) catch @panic("OOM");
}

/// The p4decomp<suffix> target of a 64-bit model (its rewritten units, census and CRT audit), for aggregates such as 240's
/// p4_x64 / p4_x64w; null when the profile does not build that mirror (and for i386, whose p4decomp is in compile-only).
pub fn decompStep(g: *const Graph, model: Model) ?*Step {
    const name: []const u8 = switch (model) {
        .i386 => return null,
        .m64 => "p4decomp64",
        .m64w => "p4decomp64w",
    };
    const top = g.ctx.b.top_level_steps.get(name) orelse return null;
    return &top.step;
}

/// A static library target ($<TARGET_FILE:name>, if(TARGET name)), e.g. p4_mod_game64.
pub fn library(g: *const Graph, name: []const u8) ?LibraryRef {
    const lib = g.ctx.libraries.get(name) orelse return null;
    return .{ .name = lib.name, .path = lib.fixed, .lazy = lib.cached, .ready = &lib.set.stage.step };
}

/// A target of an isolated host-tool directory (tools/luau/CMakeLists.txt under cmake/host-cxx.cmake
/// p4_host_cxx_profiles): compiled with the model's profile flags (the callback's ARGN) and no inherited directory compile
/// or link options, the graph's include directories, and the system include directories the Luau tools use (none for
/// Windows x64).
pub const HostTargetSpec = struct {
    /// the CMake target name, model suffix included; static libraries: the archive <build>/<dir>lib<name>.a
    name: []const u8,
    /// the CMake binary subdirectory, e.g. "tools/luau/"
    dir: []const u8,
    /// absolute, or relative to the source root
    sources: []const []const u8,
    /// per-source extras (COMPILE_DEFINITIONS / INCLUDE_DIRECTORIES / COMPILE_OPTIONS after the target's)
    source_props: []const SourceProps = &.{},
    defines: []const []const u8 = &.{},
    includes: []const []const u8 = &.{},
    /// after the profile flags
    compile_options: []const []const u8 = &.{},
    /// add_custom_command outputs listed among the SOURCES without being compiled (e.g. data files written into the
    /// executable's RUNTIME_OUTPUT_DIRECTORY): generated before the compiles; for an executable also ready before the
    /// product is used and installed beside it
    data_files: []const GeneratedFiles = &.{},
    // --- executables only ---
    /// RUNTIME_OUTPUT_DIRECTORY relative to the build tree (default `dir`)
    output_dir: ?[]const u8 = null,
    /// target_link_options after the profile's
    link_options: []const []const u8 = &.{},
    /// static library targets ($<TARGET_FILE:name>: addHostStaticLibrary's, the Luau archives) in link order
    libs: []const []const u8 = &.{},
    /// raw link items after the libraries (e.g. -ladvapi32)
    system_libs: []const []const u8 = &.{},
    /// links p4_luau<sfx> or another target carrying p4_host_cxx_runtime: its INTERFACE link options (static C++
    /// runtime, PE policy and winpthread, section GC, profile flags) instead of the bare profile flags
    cxx_runtime: bool = false,
    /// LINKER_LANGUAGE CXX
    cxx: bool = false,
};

/// add_library(<name> STATIC ...) of a host-tool directory for `model`: the archive is linked with archive semantics by
/// addHostExecutable consumers (and `library(name)`). Null when p4_host_cxx_profiles has no `model` profile.
pub fn addHostStaticLibrary(g: *const Graph, model: Model, spec: HostTargetSpec) ?LibraryRef {
    const c = g.ctx;
    if (c.finished) std.debug.panic("zigbuild/core.zig: host library {s} added after graph.finish()", .{spec.name});
    if (c.adapters[@intFromEnum(model)] == null) return null;
    if (c.libraries.contains(spec.name)) std.debug.panic("zigbuild/core.zig: library {s} defined twice", .{spec.name});
    const set = objectSet(c, c.fmt("objects {s}", .{spec.name}));
    const objs = hostCompile(c, set, model, spec);
    const out = c.newSet(c.fmt("library {s}", .{spec.name}), &.{});
    c.steps.tools.dependOn(&out.stage.step);
    const lib = archive(c, spec.name, spec.dir, set, objs, out);
    return .{ .name = lib.name, .path = lib.fixed, .lazy = lib.cached, .ready = &out.stage.step };
}

/// add_executable of a host-tool directory for `model`, linked against its static library targets; part of `tools`
/// (CMake's `all`), installed to bin/ (bin/<subdir> for a RUNTIME_OUTPUT_DIRECTORY <dir><subdir>/) with its data files.
/// Null when p4_host_cxx_profiles has no `model` profile; a
/// library target the graph lacks makes the executable fail with that reason.
pub fn addHostExecutable(g: *const Graph, model: Model, spec: HostTargetSpec) ?Artifact {
    const c = g.ctx;
    if (c.finished) std.debug.panic("zigbuild/core.zig: host executable {s} added after graph.finish()", .{spec.name});
    const ad = c.adapters[@intFromEnum(model)] orelse return null;
    if (c.artifacts.contains(spec.name)) std.debug.panic("zigbuild/core.zig: executable {s} defined twice", .{spec.name});
    const set = objectSet(c, c.fmt("objects {s}", .{spec.name}));
    const objs = hostCompile(c, set, model, spec);
    var libs: std.ArrayList(Library) = .empty;
    var after: std.ArrayList(*Step) = .empty;
    for (spec.libs) |n| if (c.libraries.get(n)) |lib| {
        libs.append(c.a, lib) catch @panic("OOM");
    } else {
        after.append(c.a, &c.b.addFail(c.fmt("{s}: library target {s} is not built in profile {s}", .{ spec.name, n, c.profile })).step) catch @panic("OOM");
    };
    var link_flags: std.ArrayList([]const u8) = .empty;
    if (model != .m64w) link_flags.appendSlice(c.a, c.base_link) catch @panic("OOM");
    // the Windows x64 profile's add_link_options (p4_host_cxx_profiles): every target of it links with the PE policy;
    // p4_host_cxx_runtime's INTERFACE lists the same options, which CMake de-duplicates
    if (model == .m64w and !spec.cxx_runtime) link_flags.appendSlice(c.a, c.pe_repro) catch @panic("OOM");
    link_flags.appendSlice(c.a, if (spec.cxx_runtime) ad.runtime_link else adapterFlags(c, c.pm, model)) catch @panic("OOM");
    link_flags.appendSlice(c.a, spec.link_options) catch @panic("OOM");
    for (spec.data_files) |d| after.append(c.a, d.ready) catch @panic("OOM");
    const windows = (c.win32 and model == .i386) or model == .m64w;
    const out_dir = spec.output_dir orelse spec.dir;
    // per-model data files of one name never share an install directory
    const install_dir = if (out_dir.len > spec.dir.len and std.mem.startsWith(u8, out_dir, spec.dir))
        c.fmt("bin/{s}", .{std.mem.trimEnd(u8, out_dir[spec.dir.len..], "/")})
    else
        "bin";
    const art = link(c, .{
        .name = spec.name,
        .out_rel = c.fmt("{s}{s}{s}", .{ out_dir, spec.name, if (windows) ".exe" else "" }),
        .model = model,
        .cxx = spec.cxx,
        .objs_set = set,
        .objs = objs,
        .libs = libs.items,
        .link_flags = link_flags.items,
        .system_libs = spec.system_libs,
        .after = after.items,
        .install_dir = install_dir,
    });
    for (spec.data_files) |d| {
        art.view.dependOn(d.ready);
        for (d.paths, d.cached) |p, cached| art.step.dependOn(&c.b.addInstallFileWithDir(cached, .{ .custom = install_dir }, std.fs.path.basename(p)).step);
    }
    c.steps.tools.dependOn(art.step);
    c.artifacts.put(c.a, spec.name, art) catch @panic("OOM");
    var srcs: std.ArrayList([]const u8) = .empty;
    for (spec.sources) |s| srcs.append(c.a, absSource(c, s)) catch @panic("OOM");
    manifestTarget(c, spec.name, "executable", model, srcs.items, objs, libs.items, &.{}, spec.cxx, art.path);
    return art;
}

fn hostCompile(c: *Ctx, set: *Set, model: Model, spec: HostTargetSpec) []const Obj {
    const flags = adapterFlags(c, c.pm, model);
    const sys = if (model == .m64w) &[_][]const u8{} else c.global_system_includes;
    var after: std.ArrayList(*Step) = .empty;
    var inputs: std.ArrayList(LazyPath) = .empty;
    for (spec.data_files) |d| {
        after.append(c.a, d.ready) catch @panic("OOM");
        inputs.appendSlice(c.a, d.cached) catch @panic("OOM");
    }
    var objs: std.ArrayList(Obj) = .empty;
    for (spec.sources) |s| {
        const path = absSource(c, s);
        var defines: std.ArrayList([]const u8) = .empty;
        defines.appendSlice(c.a, spec.defines) catch @panic("OOM");
        var includes: std.ArrayList([]const u8) = .empty;
        includes.appendSlice(c.a, c.global_includes) catch @panic("OOM");
        includes.appendSlice(c.a, spec.includes) catch @panic("OOM");
        var options: std.ArrayList([]const u8) = .empty;
        options.appendSlice(c.a, flags) catch @panic("OOM");
        options.appendSlice(c.a, spec.compile_options) catch @panic("OOM");
        for (spec.source_props) |p| if (std.mem.eql(u8, absSource(c, p.path), path)) {
            defines.appendSlice(c.a, p.defines) catch @panic("OOM");
            includes.appendSlice(c.a, p.includes) catch @panic("OOM");
            options.appendSlice(c.a, p.compile_options) catch @panic("OOM");
        };
        objs.append(c.a, compile(c, set, path, .{
            .target = spec.name,
            .dir_prefix = spec.dir,
            .defines = defines.items,
            .includes = includes.items,
            .system_includes = sys,
            .flags = options.items,
            .after = after.items,
            .file_inputs = inputs.items,
        })) catch @panic("OOM");
    }
    return objs.items;
}

/// The spec p4pc_sfdview is built from (380-movie-player.cmake), for its host twins.
pub fn sfdviewSpec(g: *const Graph) ExecutableSpec {
    const c = g.ctx;
    var srcs: std.ArrayList([]const u8) = .empty;
    srcs.append(c.a, c.src("src/movie/p4pc_sfdview.c")) catch @panic("OOM");
    for (png_sources) |s| srcs.append(c.a, c.src(s)) catch @panic("OOM");
    srcs.appendSlice(c.a, &.{ c.src("src/platform/files/vfs.c"), c.src("src/platform/files/iso9660.c"), c.os_src }) catch @panic("OOM");
    return .{
        .name = "p4pc_sfdview",
        .dir = "",
        .sources = srcs.items,
        .objects = c.a.dupe(ObjectsRef, &.{.p4movie}) catch @panic("OOM"),
        .defines = c.port_defines,
        .includes = c.strs(&.{ c.src("src/movie"), c.src("src/viewer"), c.src("src/platform/files") }),
        .libs = c.a.dupe(Lib, &.{ .avcodec, .swscale, .avutil, .m }) catch @panic("OOM"),
    };
}

/// After every module registered its targets: rewrites compile_commands.json and the graph manifest with them.
pub fn finish(g: *const Graph) void {
    const c = g.ctx;
    writeCompileCommands(c);
    _ = writeManifest(c, c.pm);
    c.finished = true;
}

// The native-test API is called only by the test modules: analyze it with the graph itself, so a configure of any
// build.zig reports its errors.
comptime {
    _ = &available;
    _ = &vars;
    _ = &addObjectLibrary;
    _ = &twinBaseLibrary;
    _ = &addExecutable;
    _ = &hostTwin;
    _ = &addOverrideTest64;
    _ = &modTestGame;
    _ = &emulator;
    _ = &setTwinLibrary;
    _ = &library;
    _ = &addHostStaticLibrary;
    _ = &addHostExecutable;
    _ = &sfdviewSpec;
    _ = &finish;
}
comptime {
    _ = &decompStep;
}
comptime {
    _ = &hasTwinLibrary;
    _ = &requireTwinLibrary;
}

// ---------------------------------------------------------------------------------------------------------------------
// The mod VM fixtures of tools/mods/CMakeLists.txt (p4_mod_profile): mod_luau_test<suffix> and mod_luau_save_test<suffix>,
// with 014-mod-recovery.cmake's additions to mod_luau_test when the MOD_WITNESS recovery tests are registered.

pub const ModLuauTests = struct {
    luau_test: Artifact,
    save_test: Artifact,
    /// whether mod_luau_test carries 014-mod-recovery.cmake's native recovery sources, definition, sections and SDL
    native_recovery: bool,
};

const mod_test_core_units = [_][]const u8{ "mods", "package_registry", "package_memory", "mod_setting_store", "lifecycle", "mod_artifacts", "compendium", "file_index" };
const mod_test_tail = [_][]const u8{
    "third_party/yxml/yxml.c",                   "src/platform/mods/lua_battle_substitute.c", "src/platform/memcard/mod_save.c",
    "src/platform/memcard/mod_save_hash.c",      "src/platform/memcard/mod_save_format.c",   "src/platform/memcard/save_image.c",
    "src/platform/files/vfs.c",                  "src/platform/files/iso9660.c",             "tests/guest_test.c",
    "src/platform/gport.c",                      "src/platform/guest.c",
};
const mod_test_after_os = [_][]const u8{
    "src/platform/ee_bytes.c",                "src/platform/content_digest.c",          "src/platform/kernel_thread.c",
    "src/platform/state_dump.c",              "src/platform/ps2_kernel.c",              "tests/state_dump_test_stubs.c",
    "src/platform/mods/lua_stages.c",         "src/platform/mods/game_stage.c",         "src/platform/mods/game_damage_stage.c",
    "src/platform/mods/game_recovery_stage.c", "src/platform/mods/game_cost_stage.c",   "src/platform/mods/game_chance_stage.c",
    "src/platform/mods/game_wearoff_stage.c", "src/platform/mods/game_buff_stage.c",    "src/platform/mods/game_reward_stage.c",
    "src/platform/mods/game_unit_stage.c",    "src/platform/mods/game_hit_stage.c",     "src/platform/mods/lua_passive.c",
    "src/platform/mods/game_passive_grant.c", "src/platform/mods/game_data.c",          "tests/mod_passive_facts.c",
};
/// 014-mod-recovery.cmake: the production modules the recovery witness links into mod_luau_test<suffix>.
const mod_recovery_sources = [_][]const u8{
    "src/platform/mods/failure_native.c", "src/platform/mods/failure_config.c", "src/platform/mods/mods_config.c",
    "src/platform/mods/packs.c",          "src/platform/mods/packs_config.c",   "src/platform/mods/boot_health.c",
    "src/platform/mods/package_boot.c",   "src/crt.c",                          "src/platform/config/p4cfg.c",
    "src/platform/os/restart.c",
};

fn modFixtureExe(c: *Ctx, model: Model, a: Adapters, name: []const u8, sources: []const []const u8, fixture: GeneratedFiles, recovery: bool) Artifact {
    const sfx = model.suffix();
    const target = c.fmt("{s}{s}", .{ name, sfx });
    const flags = adapterFlags(c, c.pm, model);
    // p4_mod_luau<suffix>'s usage requirements: the model definitions, P4_MOD_LUAU, the VM's C definition, the include
    // directories of the binding, the provider and the VM/compiler; tools/mods resets the root's compile/link options
    var defines: std.ArrayList([]const u8) = .empty;
    defines.appendSlice(c.a, c.port_defines) catch @panic("OOM");
    if (model != .i386) defines.appendSlice(c.a, &.{ "P4_GUEST64=1", "P4_GUEST_CHECK=1" }) catch @panic("OOM");
    defines.appendSlice(c.a, &.{ "P4_MOD_LUAU=1", "LUA_USE_LONGJMP=1" }) catch @panic("OOM");
    if (recovery) defines.append(c.a, "P4_MOD_NATIVE_RECOVERY=1") catch @panic("OOM");
    const luau = c.src("third_party/luau");
    const includes = c.list(&.{
        c.global_includes,
        &.{ c.src("tests"), c.src("src/platform/mods"), c.src("src/platform/files"), c.src("include") },
        &.{ c.join(&.{ luau, "VM", "include" }), c.join(&.{ luau, "Common", "include" }), c.join(&.{ luau, "Compiler", "include" }), c.join(&.{ luau, "Ast", "include" }), c.join(&.{ luau, "Bytecode", "include" }) },
    });
    const options = c.list(&.{ flags, &.{ "-O2", "-g1", "-fno-strict-aliasing", "-ffp-contract=off" }, if (recovery) &.{ "-ffunction-sections", "-fdata-sections" } else &.{} });
    const sys = if (model == .m64w) &[_][]const u8{} else c.global_system_includes;
    var srcs: std.ArrayList([]const u8) = .empty;
    srcs.appendSlice(c.a, sources) catch @panic("OOM");
    var link_flags: std.ArrayList([]const u8) = .empty;
    if (c.apple) link_flags.appendSlice(c.a, c.target_link_flags) catch @panic("OOM"); // tools/mods keeps macOS's link requirement
    if (c.win32 and model == .i386) {
        // p4_ps2_exe(mod_luau_test)
        srcs.append(c.a, c.src("src/platform/os/win32_ps2ram.S")) catch @panic("OOM");
        link_flags.appendSlice(c.a, &.{ "-Wl,--image-base=0xF0000", "-Wl,--section-alignment=0x10000", "-Wl,--disable-dynamicbase", "-Wl,--disable-reloc-section", "-Wl,--stack=8388608", c.fmt("-Wl,--Xlink=/sectionlayout:{s}", .{c.join(&.{ c.bdir, "ps2ram_layout.txt" })}) }) catch @panic("OOM");
    }
    link_flags.appendSlice(c.a, flags) catch @panic("OOM");
    link_flags.appendSlice(c.a, a.link_flags) catch @panic("OOM");
    const set = c.newSet(c.fmt("objects {s}", .{target}), &.{});
    c.steps.compile_only.dependOn(&set.stage.step);
    var objs: std.ArrayList(Obj) = .empty;
    for (srcs.items) |s| {
        var after: std.ArrayList(*Step) = .empty;
        after.append(c.a, &c.mirror.step) catch @panic("OOM");
        var inputs: std.ArrayList(LazyPath) = .empty;
        for (fixture.paths, fixture.cached) |p, cached| if (std.mem.eql(u8, p, s)) {
            after.append(c.a, fixture.ready) catch @panic("OOM");
            inputs.append(c.a, cached) catch @panic("OOM");
        };
        objs.append(c.a, compile(c, set, s, .{
            .target = target,
            .dir_prefix = "tools/mods/",
            .defines = defines.items,
            .includes = includes,
            .system_includes = sys,
            .flags = if (std.mem.endsWith(u8, s, ".S")) removeCompat(c, options) else options,
            .after = after.items,
            .file_inputs = inputs.items,
        })) catch @panic("OOM");
    }
    // target_link_libraries(PRIVATE p4_mod_luau<suffix> m) and its transitive closure (p4_mod_core, p4_luau, VM, Compiler ...)
    var libs: std.ArrayList(Library) = .empty;
    for (a.libs) |l| if (!std.mem.startsWith(u8, l.name, "p4_mod_game")) libs.append(c.a, l) catch @panic("OOM");
    var system: std.ArrayList([]const u8) = .empty;
    system.append(c.a, "-lm") catch @panic("OOM");
    if (recovery) {
        const sdl = if (model == .i386) c.sdl3 else (c.images[@intFromEnum(model)] orelse std.debug.panic("zigbuild/core.zig: MOD_WITNESS profile{s} needs its native SDL link profile, not a compile-only ABI", .{sfx})).sdl3;
        system.append(c.a, sdl) catch @panic("OOM");
    }
    const art = link(c, .{
        .name = target,
        .out_rel = c.fmt("tools/mods/{s}{s}", .{ target, if ((c.win32 and model == .i386) or model == .m64w) ".exe" else "" }),
        .model = model,
        .cxx = true,
        .objs_set = set,
        .objs = objs.items,
        .libs = libs.items,
        .link_flags = link_flags.items,
        .system_libs = system.items,
        .test_target = true,
    });
    if (model == .m64w) {
        if (c.dlls_ready) |s| art.view.dependOn(s);
    } else if (c.runtime_ready) |s| art.view.dependOn(s);
    c.artifacts.put(c.a, target, art) catch @panic("OOM");
    manifestTarget(c, target, "test_executable", model, srcs.items, objs.items, libs.items, &.{}, true, art.path);
    return art;
}

/// Whether tools/mods/CMakeLists.txt creates its Luau fixtures and tests for `model` (a Luau-active p4_mod_profile).
/// For .m64w that profile also runs find_program(P4_MOD_WINE NAMES wine64 wine REQUIRED) at configure, whether or not
/// the MOD_WITNESS tests are registered: a consumer without a Wine runner must fail configure.
pub fn modLuauProfile(g: *const Graph, model: Model) bool {
    const c = g.ctx;
    return c.o.mod_support and c.o.luau and c.adapters[@intFromEnum(model)] != null;
}

/// Whether tools/luau/CMakeLists.txt's p4_luau_profile runs for `model` (P4PC_LUAU and a p4_host_cxx_profiles model):
/// its p4_luau_check<sfx>, luau_sandbox_test<sfx> and luau_script_runner<sfx> are in graph.artifacts.
pub fn luauProfile(g: *const Graph, model: Model) bool {
    const c = g.ctx;
    return c.o.luau and c.adapters[@intFromEnum(model)] != null;
}

/// mod_luau_test<suffix> and mod_luau_save_test<suffix> of tools/mods/CMakeLists.txt for `model`, with
/// 014-mod-recovery.cmake's native recovery additions on mod_luau_test when `native_recovery` (P4PC_MOD_WITNESS_TESTS).
/// Null when CMake does not create them for this profile (no Luau-active mod profile for the model). Built once per
/// model; a second call must ask for the same recovery setting.
pub fn modLuauTests(g: *const Graph, model: Model, native_recovery: bool) ?ModLuauTests {
    const c = g.ctx;
    if (c.finished) std.debug.panic("zigbuild/core.zig: modLuauTests after graph.finish()", .{});
    if (c.mod_luau_tests[@intFromEnum(model)]) |t| {
        if (t.native_recovery != native_recovery) std.debug.panic("zigbuild/core.zig: mod_luau_test{s} already built with native_recovery={}", .{ model.suffix(), t.native_recovery });
        return t;
    }
    if (!(c.o.mod_support and c.o.luau)) return null;
    if (native_recovery and c.android) std.debug.panic("zigbuild/core.zig: MOD_WITNESS tests require the desktop Luau game binding and native VM fixture", .{});
    const a = c.adapters[@intFromEnum(model)] orelse return null;
    const fixture = modTestGameImpl(c);
    const os_src = modelOsSrc(c, model);
    var base: std.ArrayList([]const u8) = .empty;
    base.appendSlice(c.a, fixture.paths) catch @panic("OOM");
    for (mod_test_core_units) |u| base.append(c.a, c.src(c.fmt("src/platform/mods/{s}.c", .{u}))) catch @panic("OOM");
    var luau_srcs: std.ArrayList([]const u8) = .empty;
    luau_srcs.append(c.a, c.src("tests/mod_luau_test.c")) catch @panic("OOM");
    luau_srcs.appendSlice(c.a, base.items) catch @panic("OOM");
    for (mod_test_tail) |s| luau_srcs.append(c.a, c.src(s)) catch @panic("OOM");
    luau_srcs.append(c.a, os_src) catch @panic("OOM");
    for (mod_test_after_os) |s| luau_srcs.append(c.a, c.src(s)) catch @panic("OOM");
    // the save witness: mod_luau_test's SOURCES as tools/mods leaves them (before 014's additions), without the test and
    // the substitute binding, plus the card transaction transport
    var save_srcs: std.ArrayList([]const u8) = .empty;
    save_srcs.append(c.a, c.src("tests/mod_luau_save_test.c")) catch @panic("OOM");
    for (luau_srcs.items) |s| {
        if (std.mem.eql(u8, s, c.src("tests/mod_luau_test.c")) or std.mem.eql(u8, s, c.src("src/platform/mods/lua_battle_substitute.c"))) continue;
        save_srcs.append(c.a, s) catch @panic("OOM");
    }
    save_srcs.appendSlice(c.a, &.{ c.src("src/platform/memcard/save_transaction.c"), c.src("src/platform/memcard/mod_save_custody.c"), c.src("src/platform/memcard/mod_save_transport.c") }) catch @panic("OOM");
    if (native_recovery) for (mod_recovery_sources) |s| luau_srcs.append(c.a, c.src(s)) catch @panic("OOM");
    const t: ModLuauTests = .{
        .luau_test = modFixtureExe(c, model, a, "mod_luau_test", luau_srcs.items, fixture, native_recovery),
        .save_test = modFixtureExe(c, model, a, "mod_luau_save_test", save_srcs.items, fixture, false),
        .native_recovery = native_recovery,
    };
    c.mod_luau_tests[@intFromEnum(model)] = t;
    return t;
}

comptime {
    _ = &modLuauTests;
}
comptime {
    _ = &modLuauProfile;
}
