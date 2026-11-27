//! p4mod, the modder's CLI, and its libraries and fixtures for one tools/luau/CMakeLists.txt p4_luau_profile model
//! (core.luauProfile), through core's host-tool API (core.addHostStaticLibrary / core.addHostExecutable: the profile
//! flags of cmake/host-cxx.cmake, no inherited directory options, tools/luau/ object paths, part of `tools`, installed):
//!
//!  * the custom command composing `p4mod<sfx>/p4.d.luau` and the capability index `p4mod<sfx>/p4mod-capabilities.txt`
//!    (tools/p4_capabilities.py compose / index over include/p4pc/p4.d.luau and the include/p4pc/p4.d/*.luau fragments);
//!  * the archives p4mod_miniz<sfx> and p4mod_gamedata<sfx>;
//!  * p4mod<sfx> (tools/p4mod.cpp; P4MOD_API_MINOR / P4MOD_ELF_SHA256 read at configure as CMake does), linked with
//!    p4_luau_Analysis<sfx>, p4_luau<sfx> (and its p4_host_cxx_runtime link options), the two archives and the Luau
//!    archives they pull, in RUNTIME_OUTPUT_DIRECTORY tools/luau/p4mod<sfx>/ beside the generated files;
//!  * p4mod_game_fixture<sfx> and p4mod_zip32_test<sfx> (tests/*.c, the profile flags only).
const std = @import("std");
const core = @import("core.zig");

/// The products of one Luau profile model.
pub const Targets = struct {
    p4mod: core.Artifact,
    game_fixture: core.Artifact,
    zip32_test: core.Artifact,
};

/// The configure facts CMake reads (FATAL_ERROR when unreadable).
const Facts = struct { api_minor: []const u8, elf_sha256: []const u8 };

/// Registers the targets of `model`; call only for a core.luauProfile model, before core.finish.
pub fn register(b: *std.Build, graph: *const core.Graph, model: core.Model) Targets {
    if (!core.luauProfile(graph, model)) std.debug.panic("p4mod: {s} is not a Luau profile model", .{@tagName(model)});
    const sfx = model.suffix();
    const a = b.allocator;
    const facts = readFacts(b, graph.root);
    const generated = capabilities(b, graph, model);

    const root = graph.root;
    const src = struct {
        fn f(bb: *std.Build, r: []const u8, rel: []const u8) []const u8 {
            return bb.pathJoin(&.{ r, rel });
        }
    }.f;
    const luau = src(b, root, "third_party/luau");
    const mods = src(b, root, "src/platform/mods");
    const miniz = src(b, root, "third_party/miniz");
    const yxml = src(b, root, "third_party/yxml");
    const miniz_lib = b.fmt("p4mod_miniz{s}", .{sfx});
    const gamedata_lib = b.fmt("p4mod_gamedata{s}", .{sfx});

    // add_library(p4mod_miniz STATIC miniz.c miniz_tdef.c): PRIVATE ${flags} -O2 -g1, PUBLIC its own directory
    _ = core.addHostStaticLibrary(graph, model, .{
        .name = miniz_lib,
        .dir = "tools/luau/",
        .sources = strs(a, &.{ src(b, root, "third_party/miniz/miniz.c"), src(b, root, "third_party/miniz/miniz_tdef.c") }),
        .includes = strs(a, &.{miniz}),
        .compile_options = &.{ "-O2", "-g1" },
    }).?;
    // add_library(p4mod_gamedata STATIC ...): the package rules built as host code
    _ = core.addHostStaticLibrary(graph, model, .{
        .name = gamedata_lib,
        .dir = "tools/luau/",
        .sources = strs(a, &.{
            src(b, root, "tools/p4mod_data_edits.c"),      src(b, root, "tools/p4mod_package.c"),
            src(b, root, "tools/p4mod_tables.c"),          src(b, root, "tools/p4mod_os.c"),
            b.pathJoin(&.{ mods, "game_data_edit_xml.c" }), b.pathJoin(&.{ mods, "game_data_edit.c" }),
            b.pathJoin(&.{ mods, "game_data.c" }),         b.pathJoin(&.{ mods, "game_table_manifest.c" }),
            b.pathJoin(&.{ mods, "game_table_xml.c" }),    b.pathJoin(&.{ mods, "file_index.c" }),
            b.pathJoin(&.{ mods, "compendium.c" }),        b.pathJoin(&.{ mods, "package_memory.c" }),
            src(b, root, "src/platform/content_digest.c"), b.pathJoin(&.{ mods, "mod_setting_gate.c" }),
            b.pathJoin(&.{ mods, "mod_setting_store.c" }), src(b, root, "third_party/yxml/yxml.c"),
        }),
        .defines = &.{ "P4_DATA_EDIT_CHECK=1", "P4_TABLE_XML_CHECK=1", "P4_MOD_SETTING_CHECK=1" },
        .includes = strs(a, &.{ src(b, root, "include"), mods, src(b, root, "src/platform/files"), yxml }),
        .compile_options = &.{ "-O2", "-g1" },
    }).?;

    // target_link_libraries(p4mod PRIVATE p4_luau_Analysis p4_luau p4mod_miniz p4mod_gamedata), then the archives those
    // pull (Analysis: Ast Config | Compiler VM; p4_luau: VM Compiler; and theirs); advapi32 for the 64w test-folder ACL
    var libs: std.ArrayList([]const u8) = .empty;
    for ([_][]const u8{ "p4_luau_Analysis", "p4_luau", "p4mod_miniz", "p4mod_gamedata", "p4_luau_Ast", "p4_luau_Config", "p4_luau_Compiler", "p4_luau_VM", "p4_luau_Bytecode", "p4_luau_Common" }) |n|
        libs.append(a, b.fmt("{s}{s}", .{ n, sfx })) catch @panic("OOM");
    const advapi32: []const []const u8 = if (model == .m64w) &.{"-ladvapi32"} else &.{};
    const p4mod = core.addHostExecutable(graph, model, .{
        .name = b.fmt("p4mod{s}", .{sfx}),
        .dir = "tools/luau/",
        .output_dir = b.fmt("tools/luau/p4mod{s}/", .{sfx}),
        .sources = strs(a, &.{src(b, root, "tools/p4mod.cpp")}),
        // P4MOD_API_MINOR / P4MOD_ELF_SHA256; the usage requirements of p4_luau_VM (LUA_USE_LONGJMP, LUA_API for C++)
        // and p4_luau_Compiler (LUACODE_API)
        .defines = strs(a, &.{ b.fmt("P4MOD_API_MINOR={s}", .{facts.api_minor}), b.fmt("P4MOD_ELF_SHA256=\"{s}\"", .{facts.elf_sha256}), "LUA_USE_LONGJMP=1", "LUA_API=extern \"C\"", "LUACODE_API=extern \"C\"" }),
        // PRIVATE package_identifier.h; PUBLIC of p4_luau_Analysis (Analysis, Ast, Config, Common), p4_luau (include/,
        // VM, Compiler, Bytecode), p4mod_miniz and p4mod_gamedata (yxml)
        .includes = strs(a, &.{
            mods,
            b.pathJoin(&.{ luau, "Analysis", "include" }), b.pathJoin(&.{ luau, "Ast", "include" }),
            b.pathJoin(&.{ luau, "Common", "include" }),   b.pathJoin(&.{ luau, "Config", "include" }),
            src(b, root, "include"),                       b.pathJoin(&.{ luau, "VM", "include" }),
            b.pathJoin(&.{ luau, "Compiler", "include" }), b.pathJoin(&.{ luau, "Bytecode", "include" }),
            miniz,                                         yxml,
        }),
        // p4_host_cxx_target: cxx_std_17, -O2 -g1 -fno-rtti -ffp-contract=off -ffunction-sections -fdata-sections, PIE
        .compile_options = &.{ "-O2", "-g1", "-fno-rtti", "-ffp-contract=off", "-ffunction-sections", "-fdata-sections", "-fPIE", "-std=gnu++17" },
        .data_files = (a.dupe(core.GeneratedFiles, &.{generated}) catch @panic("OOM")),
        .libs = libs.items,
        .system_libs = advapi32,
        .cxx_runtime = true,
        .cxx = true,
    }).?;
    // target_compile_options / target_link_options ${flags}
    const fixture = core.addHostExecutable(graph, model, .{
        .name = b.fmt("p4mod_game_fixture{s}", .{sfx}),
        .dir = "tools/luau/",
        .sources = strs(a, &.{src(b, root, "tests/p4mod_game_fixture.c")}),
    }).?;
    const zip32 = core.addHostExecutable(graph, model, .{
        .name = b.fmt("p4mod_zip32_test{s}", .{sfx}),
        .dir = "tools/luau/",
        .sources = strs(a, &.{src(b, root, "tests/p4mod_zip32_test.c")}),
    }).?;
    return .{ .p4mod = p4mod, .game_fixture = fixture, .zip32_test = zip32 };
}

/// add_custom_command(OUTPUT p4mod<sfx>/p4.d.luau p4mod<sfx>/p4mod-capabilities.txt): compose and index into the cache,
/// then place both at the build-tree paths. Inputs: the script, the base declaration and every fragment (the
/// CONFIGURE_DEPENDS glob).
fn capabilities(b: *std.Build, graph: *const core.Graph, model: core.Model) core.GeneratedFiles {
    const sfx = model.suffix();
    const root = graph.root;
    const script = b.pathJoin(&.{ root, "tools", "p4_capabilities.py" });
    const base = b.pathJoin(&.{ root, "include", "p4pc", "p4.d.luau" });
    const fragments = listFragments(b, b.pathJoin(&.{ root, "include", "p4pc", "p4.d" }));
    const dir = b.pathJoin(&.{ graph.build_dir, "tools", "luau", b.fmt("p4mod{s}", .{sfx}) });
    const stage = b.addSystemCommand(&.{ graph.python, b.pathJoin(&.{ root, "tools", "zig_build_step.py" }), "stage", "--" });
    stage.setName(b.fmt("p4mod{s}: composing p4.d.luau and the capability index", .{sfx}));
    stage.has_side_effects = true;
    var paths: std.ArrayList([]const u8) = .empty;
    var cached: std.ArrayList(std.Build.LazyPath) = .empty;
    for ([_]struct { cmd: []const u8, out: []const u8 }{
        .{ .cmd = "compose", .out = "p4.d.luau" },
        .{ .cmd = "index", .out = "p4mod-capabilities.txt" },
    }) |g| {
        const run = b.addSystemCommand(&.{ graph.python, script, g.cmd });
        run.setName(b.fmt("p4mod{s}: p4_capabilities.py {s}", .{ sfx, g.cmd }));
        const out = run.addOutputFileArg(g.out);
        run.addFileInput(b.graph.cwdRelativePath(script));
        run.addFileInput(b.graph.cwdRelativePath(base));
        for (fragments) |f| run.addFileInput(b.graph.cwdRelativePath(f));
        run.expectExitCode(0);
        const fixed = b.pathJoin(&.{ dir, g.out });
        stage.addFileArg2(out, .{ .prefix = b.fmt("{s}=", .{fixed}) });
        paths.append(b.allocator, fixed) catch @panic("OOM");
        cached.append(b.allocator, out) catch @panic("OOM");
    }
    return .{ .paths = paths.items, .ready = &stage.step, .cached = cached.items };
}

fn listFragments(b: *std.Build, dir_path: []const u8) []const []const u8 {
    const io = b.graph.io;
    const dir = std.Io.Dir.cwd().openDir(io, dir_path, .{ .iterate = true }) catch |e| std.process.fatal("p4mod: cannot open {s}: {t}", .{ dir_path, e });
    defer dir.close(io);
    var out: std.ArrayList([]const u8) = .empty;
    var it = dir.iterate();
    while (it.next(io) catch |e| std.process.fatal("p4mod: cannot list {s}: {t}", .{ dir_path, e })) |entry| {
        if (entry.kind == .file and std.mem.endsWith(u8, entry.name, ".luau"))
            out.append(b.allocator, b.pathJoin(&.{ dir_path, entry.name })) catch @panic("OOM");
    }
    std.mem.sort([]const u8, out.items, {}, struct {
        fn f(_: void, x: []const u8, y: []const u8) bool {
            return std.mem.lessThan(u8, x, y);
        }
    }.f);
    return out.items;
}

/// P4_PACKAGE_API_MINOR (`#define P4_PACKAGE_API_MINOR <n>` of include/p4pc/packages.h) and the retail ELF sha256
/// (`sha256 <hex>` of config/retail_fnptr_words.txt); configure fails as CMake's FATAL_ERROR does when either is absent.
fn readFacts(b: *std.Build, root: []const u8) Facts {
    const minor = matchLine(b, b.pathJoin(&.{ root, "include", "p4pc", "packages.h" }), "#define P4_PACKAGE_API_MINOR ", std.ascii.isDigit);
    const sha = matchLine(b, b.pathJoin(&.{ root, "config", "retail_fnptr_words.txt" }), "sha256 ", isLowerHex);
    if (minor == null or sha == null)
        std.process.fatal("p4mod: cannot read P4_PACKAGE_API_MINOR (include/p4pc/packages.h) or the ELF sha256 (config/retail_fnptr_words.txt)", .{});
    return .{ .api_minor = minor.?, .elf_sha256 = sha.? };
}

fn isLowerHex(ch: u8) bool {
    return std.ascii.isDigit(ch) or (ch >= 'a' and ch <= 'f');
}

/// file(STRINGS ... REGEX "^<prefix>[class]+$") then the value check: exactly one matching line (CMake joins several
/// with `;`, which fails the check).
fn matchLine(b: *std.Build, path: []const u8, prefix: []const u8, comptime class: fn (u8) bool) ?[]const u8 {
    const text = std.Io.Dir.cwd().readFileAlloc(b.graph.io, path, b.allocator, .limited(16 << 20)) catch return null;
    var found: ?[]const u8 = null;
    // A second matching line makes the configure fact ambiguous.
    var lines = std.mem.splitScalar(u8, text, '\n');
    while (lines.next()) |raw| {
        const line = std.mem.trimEnd(u8, raw, "\r");
        if (!std.mem.startsWith(u8, line, prefix)) continue;
        const value = line[prefix.len..];
        if (value.len == 0) continue;
        for (value) |ch| {
            if (!class(ch)) break;
        } else {
            if (found != null) return null;
            found = value;
        }
    }
    return found;
}

fn strs(a: std.mem.Allocator, items: []const []const u8) []const []const u8 {
    return a.dupe([]const u8, items) catch @panic("OOM");
}
