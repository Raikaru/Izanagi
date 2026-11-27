//! p4-zig-clang / p4-zig-clang++: the one C/C++ compiler and linker path of the `zig build` graph (zigbuild/core.zig).
//!
//! The repository's tools take a single compiler executable (unit64.py --clang, ee_float_cc.py and printf_rewrite.py
//! launchers, crt_flow.py's compile-database replay, which finds the compiler by a `*-clang` name). This program is that
//! executable: it runs the pinned Zig's `zig cc` (or `zig c++` when invoked as `...++`) with the caller's clang
//! arguments, after mapping the clang spellings Zig does not accept onto Zig's own:
//!
//! * `--target=`/`-target` clang triples (i386/i686, *-w64-mingw32, *-linux-android<API>, *-apple-macos<ver>) become
//!   Zig target queries, and `-m32`/`-m64` select the x86 width of that target, as clang does (cmake/zig/zig_object.py
//!   uses the same mapping for the Zig objects of the same profile). A compile or link without a target fails: the
//!   graph never builds for the host CPU by accident.
//! * Preprocessed C (`-x cpp-output`, or a `.i` input without `-x`) and LLVM IR (`-x ir`) are inputs `zig cc` refuses.
//!   For those the program asks `zig cc -###` for the job of the same command with `-x c`, requires exactly one
//!   `-cc1` job containing exactly one `-x c` and the original input, replaces that language with the requested one
//!   and executes the job: Zig's target, CPU and libc options are kept exactly. Anything else fails.
//! * Flags with no meaning for Zig's driver, whose effect Zig always provides, are removed: `-fuse-ld=lld` (Zig links
//!   with its own LLD), `-static-libstdc++`/`-static-libgcc` (Zig links libc++ and compiler-rt statically),
//!   `--gcc-install-dir=` and, on Linux targets, `-isystem /usr/...` (Zig supplies the glibc headers of the target;
//!   cmake/zig/zig_object.py drops the same host directories), `-isysroot <sdk>` (Zig ships the macOS headers), and on
//!   Android `--sysroot` (replaced by the libc installation file the graph wrote for that sysroot, ZIG_LIBC).
//! * Zig's Debug-mode UBSan default is turned off (`-fno-sanitize=undefined`) unless the command names a sanitizer:
//!   the clang the CMake build used has no such default, and its runtime calls are outside the reviewed CRT boundary.
//! * Record-layout dumps with no explicit debug option use `-g0`: Zig otherwise enables debug information by default,
//!   causing clang to dump additional debug-only records that the CMake compiler's oracle does not see.
//! * Clang's default stack-protector policy and assertion macros are preserved: absent an explicit caller policy,
//!   Zig's implicit stack protection is disabled. Release-mode NDEBUG is undefined before caller -D/-U arguments.
//! * For the C driver, selected MinGW runtime libraries' matching include directory precedes Zig's bundled libc SYSTEM
//!   headers, after caller -I paths. C++ keeps Zig's paired libc++/CRT headers; other targets keep their own libc headers.
//!
//! * With `-Dimage-clang` (the CI image's llvm-mingw clang) the Linux and Windows x86_64 C++ compiles and links run in image runtime mode
//!   (imageRuntime below): the CMake build's C++ headers and runtime providers, not Zig's bundled libc++/libc++abi/crt. Linux x86_64
//!   links with the image's GCC runtime; Windows x86_64 (MinGW) with llvm-mingw's crt2/crtbegin, libc++, libunwind and libmingw32.
//!
//! Queries (`--version`, `-dumpversion`, `-print-*`) run unchanged. Exit status and output are the child's.
const std = @import("std");
const builtin = @import("builtin");
const config = @import("p4_zig_cc_config");

const Allocator = std.mem.Allocator;

fn fail(comptime format: []const u8, args: anytype) noreturn {
    std.process.fatal("p4-zig-clang: " ++ format, args);
}

const Target = struct {
    query: []const u8,
    os: enum { linux, windows, android, macos },
    arch: []const u8,
    api: ?[]const u8,
};

fn omitNoPic(t: Target, arg: []const u8) bool {
    return t.os == .windows and std.mem.eql(u8, arg, "-fno-pic");
}

test "only Windows omits the redundant no-pic flag" {
    inline for (.{ .linux, .windows, .android, .macos }) |os| {
        const t: Target = .{ .query = "", .os = os, .arch = "", .api = null };
        try std.testing.expectEqual(os == .windows, omitNoPic(t, "-fno-pic"));
        try std.testing.expect(!omitNoPic(t, "-fPIC"));
        try std.testing.expect(!omitNoPic(t, "-fno-pie"));
    }
}

fn mapArch(name: []const u8) ?[]const u8 {
    const table = [_][2][]const u8{
        .{ "i386", "x86" },     .{ "i486", "x86" },       .{ "i586", "x86" },        .{ "i686", "x86" },
        .{ "x86", "x86" },      .{ "x86_64", "x86_64" },  .{ "amd64", "x86_64" },    .{ "aarch64", "aarch64" },
        .{ "arm64", "aarch64" },
    };
    for (table) |row| if (std.mem.eql(u8, row[0], name)) return row[1];
    return null;
}

fn endsWithDigitsAfter(rest: []const u8, marker: []const u8) ?[]const u8 {
    const at = std.mem.lastIndexOf(u8, rest, marker) orelse return null;
    const tail = rest[at + marker.len ..];
    for (tail) |c| if (!std.ascii.isDigit(c) and c != '.') return null;
    return tail;
}

/// The same mapping as cmake/zig/zig_object.py zig_target(): the Zig query for a clang triple and an -m32/-m64 width.
fn zigTarget(arena: Allocator, triple: []const u8, width: ?u8) Target {
    const dash = std.mem.indexOfScalar(u8, triple, '-') orelse fail("unsupported target {s}", .{triple});
    var arch = mapArch(triple[0..dash]) orelse fail("unsupported architecture in target {s}", .{triple});
    if (width) |w| {
        if (std.mem.eql(u8, arch, "aarch64")) fail("-m{d} does not apply to {s}", .{ w, triple });
        arch = if (w == 32) "x86" else "x86_64";
    }
    const rest = triple[dash + 1 ..];
    if (std.mem.indexOf(u8, rest, "linux-android") != null) {
        const api = endsWithDigitsAfter(rest, "linux-android") orelse fail("unsupported Android target {s}", .{triple});
        if (api.len == 0) fail("an Android target needs its API level in the triple (e.g. aarch64-linux-android24): {s}", .{triple});
        return .{ .query = std.fmt.allocPrint(arena, "{s}-linux-android.{s}", .{ arch, api }) catch @panic("OOM"), .os = .android, .arch = arch, .api = api };
    }
    if (std.mem.indexOf(u8, rest, "mingw32") != null or std.mem.endsWith(u8, rest, "windows-gnu")) {
        return .{ .query = std.fmt.allocPrint(arena, "{s}-windows-gnu", .{arch}) catch @panic("OOM"), .os = .windows, .arch = arch, .api = null };
    }
    if (std.mem.indexOf(u8, rest, "apple") != null) {
        inline for (.{ "macosx", "macos", "darwin" }) |marker| {
            if (endsWithDigitsAfter(rest, marker)) |version| {
                const query = if (version.len == 0)
                    std.fmt.allocPrint(arena, "{s}-macos-none", .{arch}) catch @panic("OOM")
                else
                    std.fmt.allocPrint(arena, "{s}-macos.{s}-none", .{ arch, version }) catch @panic("OOM");
                return .{ .query = query, .os = .macos, .arch = arch, .api = null };
            }
        }
    }
    if (std.mem.endsWith(u8, rest, "linux-gnu")) {
        const query = if (config.glibc.len != 0)
            std.fmt.allocPrint(arena, "{s}-linux-gnu.{s}", .{ arch, config.glibc }) catch @panic("OOM")
        else
            std.fmt.allocPrint(arena, "{s}-linux-gnu", .{arch}) catch @panic("OOM");
        return .{ .query = query, .os = .linux, .arch = arch, .api = null };
    }
    fail("unsupported target {s} (supported: Linux glibc, mingw-w64, Android, macOS)", .{triple});
}

fn isQuery(args: []const []const u8) bool {
    for (args) |a| {
        if (std.mem.eql(u8, a, "--version") or std.mem.eql(u8, a, "-dumpversion") or
            std.mem.eql(u8, a, "-dumpmachine") or std.mem.startsWith(u8, a, "-print-")) return true;
    }
    return false;
}

fn langFromInput(path: []const u8) ?[]const u8 {
    if (std.mem.endsWith(u8, path, ".i")) return "cpp-output";
    return null;
}

const Language = struct {
    /// index of the `-x` argument (or of the joined `-xLANG`), or null for an input classified by its suffix
    x_index: ?usize,
    joined: bool,
    lang: []const u8,
};

/// Every argument that takes a separate value in the commands the graph and the tools issue: their values are not inputs.
const value_flags = [_][]const u8{
    "-o",       "-MF",      "-MT",         "-MQ",       "-I",       "-isystem",  "-include", "-iquote", "-idirafter",
    "-imacros", "-Xclang",  "-Xlinker",    "-target",   "--target", "-isysroot", "--sysroot", "-arch",  "-D",
    "-U",       "-L",       "-l",          "-x",        "-z",       "-mllvm",    "-MJ",      "-Xpreprocessor",
    "-Xassembler", "-framework", "-install_name", "-rpath", "-e", "-u", "-T",
};

fn takesValue(a: []const u8) bool {
    for (value_flags) |f| if (std.mem.eql(u8, a, f)) return true;
    return false;
}

fn execOrRun(io: std.Io, argv: []const []const u8, environ: ?*const std.process.Environ.Map) noreturn {
    if (std.process.can_replace) {
        const err = std.process.replace(io, .{ .argv = argv, .environ_map = environ });
        fail("cannot execute {s}: {t}", .{ argv[0], err });
    }
    var child = std.process.spawn(io, .{ .argv = argv, .environ_map = environ }) catch |err|
        fail("cannot execute {s}: {t}", .{ argv[0], err });
    const term = child.wait(io) catch |err| fail("waiting for {s}: {t}", .{ argv[0], err });
    switch (term) {
        .exited => |code| std.process.exit(code),
        else => std.process.exit(1),
    }
}

/// Splits one `-###` job line: space separated, each argument double quoted with backslash escapes.
fn splitJob(arena: Allocator, line: []const u8) ![]const []const u8 {
    var out: std.ArrayList([]const u8) = .empty;
    var i: usize = 0;
    while (i < line.len) {
        while (i < line.len and line[i] == ' ') i += 1;
        if (i >= line.len) break;
        if (line[i] != '"') return error.UnquotedArgument;
        i += 1;
        var arg: std.ArrayList(u8) = .empty;
        while (true) {
            if (i >= line.len) return error.UnterminatedArgument;
            const c = line[i];
            if (c == '\\') {
                if (i + 1 >= line.len) return error.UnterminatedArgument;
                try arg.append(arena, line[i + 1]);
                i += 2;
            } else if (c == '"') {
                i += 1;
                break;
            } else {
                try arg.append(arena, c);
                i += 1;
            }
        }
        try out.append(arena, try arg.toOwnedSlice(arena));
    }
    return out.toOwnedSlice(arena);
}

fn hasCxxSuffix(path: []const u8) bool {
    for ([_][]const u8{ ".cpp", ".cc", ".cxx", ".c++", ".C" }) |suffix| if (std.mem.endsWith(u8, path, suffix)) return true;
    return false;
}

fn shellQuote(arena: Allocator, word: []const u8) ![]const u8 {
    var out: std.ArrayList(u8) = .empty;
    try out.append(arena, '\'');
    for (word) |c| {
        if (c == '\'') try out.appendSlice(arena, "'\\''") else try out.append(arena, c);
    }
    try out.append(arena, '\'');
    return out.toOwnedSlice(arena);
}

/// POSIX-style words of one line (single and double quotes, backslash), as lld prints its `-###` lines and Python's shlex reads them.
fn shellSplit(arena: Allocator, line: []const u8) ![]const []const u8 {
    var out: std.ArrayList([]const u8) = .empty;
    var cur: std.ArrayList(u8) = .empty;
    var in_word = false;
    var quote: u8 = 0;
    var i: usize = 0;
    while (i < line.len) : (i += 1) {
        const c = line[i];
        if (quote == 0) {
            if (c == ' ' or c == '\t') {
                if (in_word) try out.append(arena, try cur.toOwnedSlice(arena));
                cur = .empty;
                in_word = false;
            } else if (c == '\'' or c == '"') {
                quote = c;
                in_word = true;
            } else if (c == '\\' and i + 1 < line.len) {
                i += 1;
                try cur.append(arena, line[i]);
                in_word = true;
            } else {
                try cur.append(arena, c);
                in_word = true;
            }
        } else if (quote == '\'') {
            if (c == '\'') quote = 0 else try cur.append(arena, c);
        } else if (c == '"') {
            quote = 0;
        } else if (c == '\\' and i + 1 < line.len and (line[i + 1] == '"' or line[i + 1] == '\\' or line[i + 1] == '$')) {
            i += 1;
            try cur.append(arena, line[i]);
        } else {
            try cur.append(arena, c);
        }
    }
    if (quote != 0) return error.UnterminatedQuote;
    if (in_word) try out.append(arena, try cur.toOwnedSlice(arena));
    return out.toOwnedSlice(arena);
}

/// Image runtime mode (Linux and Windows x86_64 with `-Dimage-clang`): the CMake build compiles C and C++ with the CI image's clang and links C++ with it
/// with that clang's GCC runtime (libstdc++ headers, crt1/crti/crtbegin, `-Bstatic -lstdc++ -Bdynamic`, libgcc/libgcc_eh, the
/// system libc). The graph compiles and links C++ the same way, so the product's runtime providers are CMake's and not Zig's
/// bundled libc++/libc++abi/libunwind/crt. Zig supplies the Zig and C objects and the final LLD (`zig ld.lld`) of the link:
/// the clang driver (`-###`, so it builds and links nothing) names the LLD argv, which Zig's LLD then runs. The argv is
/// printed as `ld.lld ...` under ZIG_VERBOSE_LINK exactly as Zig's own driver does for tools/zig_link_map.py. Returns for
/// everything this mode does not handle (C compiles, C links, preprocessed input).
fn imageRuntime(arena: Allocator, io: std.Io, environ_map: *const std.process.Environ.Map, driver: []const u8, windows: bool, in_args: []const []const u8) !void {
    var compile_only = false;
    var has_output = false;
    var cxx_input = false;
    var cxx_language = false;
    var other_language = false;
    var i: usize = 0;
    while (i < in_args.len) : (i += 1) {
        const a = in_args[i];
        if (std.mem.eql(u8, a, "-c") or std.mem.eql(u8, a, "-S") or std.mem.eql(u8, a, "-E") or std.mem.eql(u8, a, "-fsyntax-only") or
            std.mem.eql(u8, a, "-emit-llvm") or std.mem.eql(u8, a, "-M") or std.mem.eql(u8, a, "-MM")) compile_only = true;
        if (std.mem.eql(u8, a, "-o")) has_output = true;
        if (std.mem.eql(u8, a, "-x") or (std.mem.startsWith(u8, a, "-x") and a.len > 2)) {
            const lang = if (std.mem.eql(u8, a, "-x")) (if (i + 1 < in_args.len) in_args[i + 1] else "") else a[2..];
            if (std.mem.eql(u8, lang, "c++")) cxx_language = true else other_language = true;
        }
        if (takesValue(a)) {
            i += 1;
            continue;
        }
        if (a.len > 0 and a[0] != '-' and hasCxxSuffix(a)) cxx_input = true;
    }
    const is_cxx = std.mem.eql(u8, driver, "c++") or cxx_language or cxx_input;
    // Every compile, syntax check, preprocess and dependency run of these targets is the image clang's, C as well as C++: the
    // CMake build's own compiler with the CMake build's own headers, so the objects are the ones CMake's compile makes
    // (Zig's clang inlines the same C differently and Zig's bundled headers differ from the image's).
    if (compile_only) {
        const compiler = try std.fmt.allocPrint(arena, "{s}/{s}", .{ config.image_clang, if (is_cxx) "clang++" else "clang" });
        var compile: std.ArrayList([]const u8) = .empty;
        try compile.append(arena, compiler);
        try compile.appendSlice(arena, in_args);
        execOrRun(io, compile.items, null);
    }
    if (other_language) return;
    const clang = try std.fmt.allocPrint(arena, "{s}/clang++", .{config.image_clang});
    var argv: std.ArrayList([]const u8) = .empty;
    try argv.append(arena, clang);
    if (!std.mem.eql(u8, driver, "c++") or !has_output) return;
    // The link: the argv CMake's clang++ gets. The MinGW PE link names LLD explicitly (cmake/reproducible.cmake); the ELF link
    // names no linker, so clang picks the image's GNU ld, the CMake build's linker (a caller -fuse-ld still wins).
    if (windows) try argv.append(arena, "-fuse-ld=lld");
    for (in_args) |a| {
        // The graph's Zig-driver spelling of reproducible PE links; the MinGW driver's is --no-insert-timestamp.
        if (windows and std.mem.eql(u8, a, "-Wl,/Brepro")) try argv.append(arena, "-Wl,--no-insert-timestamp") else try argv.append(arena, a);
    }
    try argv.append(arena, "-###");
    const result = std.process.run(arena, io, .{ .argv = argv.items }) catch |err|
        fail("cannot run {s} -###: {t}", .{ clang, err });
    switch (result.term) {
        .exited => |code| if (code != 0) fail("{s} -### failed ({d}):\n{s}", .{ clang, code, result.stderr }),
        else => fail("{s} -### crashed:\n{s}", .{ clang, result.stderr }),
    }
    var job: ?[]const []const u8 = null;
    var jobs: usize = 0;
    var lines = std.mem.splitScalar(u8, result.stderr, '\n');
    while (lines.next()) |raw| {
        const line = std.mem.trimEnd(u8, raw, "\r");
        if (!std.mem.startsWith(u8, line, " \"")) continue;
        jobs += 1;
        job = splitJob(arena, line) catch |err| fail("cannot parse the -### job ({t}): {s}", .{ err, line });
    }
    if (jobs != 1) fail("expected exactly one linker job from {s} -###, got {d}:\n{s}", .{ clang, jobs, result.stderr });
    const words = job.?;
    const lld_job = std.mem.eql(u8, std.fs.path.basename(words[0]), "ld.lld");
    if (words.len < 2 or (windows and !lld_job)) fail("the -### job is not an ld.lld job:\n{s}", .{result.stderr});
    if (windows) {
        // The MinGW driver of clang names LLD in its ld.lld spelling (`-m i386pep`); LLD translates it to the lld-link argv
        // that `-###` prints, the same translation the CMake build's COFF link goes through. That argv is the link.
        // Zig's own `ld.lld` is the ELF driver and does not dispatch `-m i386pep`: the image's ld.lld (named by clang) translates.
        var dry: std.ArrayList([]const u8) = .empty;
        try dry.append(arena, words[0]);
        try dry.appendSlice(arena, words[1..]);
        try dry.append(arena, "-###");
        const translated = std.process.run(arena, io, .{ .argv = dry.items }) catch |err|
            fail("cannot run {s} -###: {t}", .{ words[0], err });
        switch (translated.term) {
            .exited => |code| if (code != 0) fail("{s} -### failed ({d}):\n{s}", .{ words[0], code, translated.stderr }),
            else => fail("{s} -### crashed:\n{s}", .{ words[0], translated.stderr }),
        }
        var coff: ?[]const []const u8 = null;
        var coffs: usize = 0;
        var coff_lines = std.mem.splitScalar(u8, translated.stderr, '\n');
        while (coff_lines.next()) |raw| {
            const line = std.mem.trimEnd(u8, raw, "\r");
            if (!std.mem.startsWith(u8, line, "lld-link ")) continue;
            coffs += 1;
            coff = shellSplit(arena, line) catch |err| fail("cannot parse the lld-link line ({t}): {s}", .{ err, line });
        }
        if (coffs != 1) fail("expected exactly one lld-link line from ld.lld -###, got {d}:\n{s}", .{ coffs, translated.stderr });
        const coff_words = coff.?;
        var run_coff: std.ArrayList([]const u8) = .empty;
        try run_coff.appendSlice(arena, &.{ config.zig_exe, "lld-link" });
        try run_coff.appendSlice(arena, coff_words[1..]);
        if (environ_map.get("ZIG_VERBOSE_LINK") != null) {
            var line: std.ArrayList(u8) = .empty;
            try line.appendSlice(arena, "lld-link");
            for (coff_words[1..]) |w| {
                try line.append(arena, ' ');
                try line.appendSlice(arena, try shellQuote(arena, w));
            }
            try line.append(arena, '\n');
            std.debug.print("{s}", .{line.items});
        }
        execOrRun(io, run_coff.items, null);
    }
    // ELF: LLD runs as Zig's (`zig ld.lld`), as `ld.lld ...` on ZIG_VERBOSE_LINK exactly like Zig's driver; any other linker
    // clang named (the image's GNU ld) runs as that executable and is reported with its path, for tools/zig_link_map.py.
    var lld: std.ArrayList([]const u8) = .empty;
    if (lld_job) try lld.appendSlice(arena, &.{ config.zig_exe, "ld.lld" }) else try lld.append(arena, words[0]);
    try lld.appendSlice(arena, words[1..]);
    if (environ_map.get("ZIG_VERBOSE_LINK") != null) {
        var line: std.ArrayList(u8) = .empty;
        try line.appendSlice(arena, if (lld_job) "ld.lld" else words[0]);
        for (words[1..]) |w| {
            try line.append(arena, ' ');
            try line.appendSlice(arena, try shellQuote(arena, w));
        }
        try line.append(arena, '\n');
        std.debug.print("{s}", .{line.items});
    }
    execOrRun(io, lld.items, null);
}

pub fn main(init: std.process.Init) !void {
    const arena = init.arena.allocator();
    const io = init.io;
    const raw_argv = try init.minimal.args.toSlice(arena);
    if (raw_argv.len == 0) fail("no arguments", .{});
    const argv = try arena.alloc([]const u8, raw_argv.len);
    for (raw_argv, argv) |a, *d| d.* = a;
    const tool_name = std.fs.path.basename(argv[0]);
    const driver: []const u8 = if (std.mem.endsWith(u8, tool_name, "++") or std.mem.endsWith(u8, tool_name, "++.exe")) "c++" else "cc";
    const in_args: []const []const u8 = argv[1..];

    var head: std.ArrayList([]const u8) = .empty;
    try head.appendSlice(arena, &.{ config.zig_exe, driver });

    if (isQuery(in_args)) {
        try head.appendSlice(arena, in_args);
        execOrRun(io, head.items, null);
    }

    // First pass: the target and width (the last of each wins, as in clang).
    var triple: ?[]const u8 = null;
    var width: ?u8 = null;
    {
        var i: usize = 0;
        while (i < in_args.len) : (i += 1) {
            const a = in_args[i];
            if (std.mem.startsWith(u8, a, "--target=")) {
                triple = a["--target=".len..];
            } else if (std.mem.eql(u8, a, "-target") or std.mem.eql(u8, a, "--target")) {
                i += 1;
                if (i >= in_args.len) fail("{s} needs a value", .{a});
                triple = in_args[i];
            } else if (std.mem.eql(u8, a, "-m32")) {
                width = 32;
            } else if (std.mem.eql(u8, a, "-m64")) {
                width = 64;
            } else if (takesValue(a)) {
                i += 1;
            }
        }
    }
    const t = zigTarget(arena, triple orelse fail("no --target: every compile and link of the graph names its target", .{}), width);
    if (config.image_clang.len != 0 and (t.os == .linux or t.os == .windows) and std.mem.eql(u8, t.arch, "x86_64"))
        try imageRuntime(arena, io, init.environ_map, driver, t.os == .windows, in_args);

    // Second pass: the arguments for Zig's driver.
    var args: std.ArrayList([]const u8) = .empty;
    try args.appendSlice(arena, &.{ "-target", t.query, "-UNDEBUG" });
    var language: ?Language = null;
    var inputs: std.ArrayList(usize) = .empty;
    var sanitizer = false;
    var stack_policy = false;
    var layout_dump = false;
    var explicit_debug = false;
    for (in_args) |a| {
        layout_dump = layout_dump or std.mem.eql(u8, a, "-fdump-record-layouts");
        explicit_debug = explicit_debug or std.mem.startsWith(u8, a, "-g");
    }
    var android_sysroot: ?[]const u8 = null;
    {
        var i: usize = 0;
        while (i < in_args.len) : (i += 1) {
            const a = in_args[i];
            if (std.mem.startsWith(u8, a, "--target=") or std.mem.eql(u8, a, "-m32") or std.mem.eql(u8, a, "-m64")) continue;
            if (std.mem.eql(u8, a, "-target") or std.mem.eql(u8, a, "--target")) {
                i += 1;
                continue;
            }
            if (std.mem.eql(u8, a, "-fuse-ld=lld") or std.mem.eql(u8, a, "-static-libstdc++") or std.mem.eql(u8, a, "-static-libgcc") or
                std.mem.startsWith(u8, a, "--gcc-install-dir=")) continue;
            // llvm-mingw 23.1.3 forces the same COFF relocation model with/without -fno-pic:
            // i686/x64 O0/O2 probes and production rw_device_register.c/crt.c objects were byte-identical.
            // Zig rejects that redundant flag for Windows. Keep it unchanged for ELF targets.
            if (omitNoPic(t, a)) continue;
            if (std.mem.eql(u8, a, "-isysroot")) {
                i += 1;
                continue;
            }
            if (std.mem.startsWith(u8, a, "--sysroot=") or std.mem.eql(u8, a, "--sysroot")) {
                const value = if (std.mem.eql(u8, a, "--sysroot")) blk: {
                    i += 1;
                    if (i >= in_args.len) fail("--sysroot needs a value", .{});
                    break :blk in_args[i];
                } else a["--sysroot=".len..];
                if (t.os != .android) fail("--sysroot is only meaningful for the Android profile (got {s} for {s})", .{ value, t.query });
                android_sysroot = value;
                continue;
            }
            if (t.os == .linux and (std.mem.eql(u8, a, "-isystem") or std.mem.startsWith(u8, a, "-isystem"))) {
                const value = if (std.mem.eql(u8, a, "-isystem")) blk: {
                    if (i + 1 >= in_args.len) fail("-isystem needs a value", .{});
                    break :blk in_args[i + 1];
                } else a["-isystem".len..];
                if (std.mem.startsWith(u8, value, "/usr/")) {
                    if (std.mem.eql(u8, a, "-isystem")) i += 1;
                    continue;
                }
            }
            if (std.mem.startsWith(u8, a, "-fsanitize=")) sanitizer = true;
            stack_policy = stack_policy or std.mem.startsWith(u8, a, "-fstack-protector") or
                std.mem.eql(u8, a, "-fno-stack-protector");
            if (std.mem.eql(u8, a, "-x")) {
                if (i + 1 >= in_args.len) fail("-x needs a value", .{});
                language = .{ .x_index = args.items.len, .joined = false, .lang = in_args[i + 1] };
                try args.appendSlice(arena, &.{ a, in_args[i + 1] });
                i += 1;
                continue;
            }
            if (std.mem.startsWith(u8, a, "-x") and a.len > 2) {
                language = .{ .x_index = args.items.len, .joined = true, .lang = a[2..] };
                try args.append(arena, a);
                continue;
            }
            if (takesValue(a)) {
                if (i + 1 >= in_args.len) fail("{s} needs a value", .{a});
                try args.appendSlice(arena, &.{ a, in_args[i + 1] });
                i += 1;
                continue;
            }
            if (a.len > 0 and a[0] != '-') try inputs.append(arena, args.items.len);
            try args.append(arena, a);
        }
    }
    if (layout_dump and !explicit_debug) try args.append(arena, "-g0");
    if (!sanitizer) try args.append(arena, "-fno-sanitize=undefined");
    if (!stack_policy) try args.append(arena, "-fno-stack-protector");
    if (t.os == .windows and std.mem.eql(u8, driver, "cc")) {
        const include = if (std.mem.eql(u8, t.arch, "x86")) config.mingw32_include else config.mingw64_include;
        // Zig puts bundled SYSTEM paths before caller -isystem paths. A trailing
        // -I selects the measured standalone C headers. For C++ it would precede
        // libc++'s required wrappers and mismatch its bundled CRT header pair.
        if (include.len != 0) try args.appendSlice(arena, &.{ "-I", include });
    }

    var environ: ?*std.process.Environ.Map = null;
    if (t.os == .android) {
        const sysroot = android_sysroot orelse fail("the Android target {s} needs --sysroot=<NDK sysroot>", .{t.query});
        const exe_dir = try std.process.executableDirPathAlloc(io, arena);
        const triple_dir: []const u8 = if (std.mem.eql(u8, t.arch, "x86")) "i686-linux-android" else if (std.mem.eql(u8, t.arch, "x86_64")) "x86_64-linux-android" else "aarch64-linux-android";
        const libc_file = try std.fs.path.join(arena, &.{ exe_dir, "..", "zig-libc", try std.fmt.allocPrint(arena, "{s}{s}.txt", .{ triple_dir, t.api.? }) });
        std.Io.Dir.cwd().access(io, libc_file, .{}) catch
            fail("no libc installation file {s} for the Android sysroot {s} (written by the zig build configure step)", .{ libc_file, sysroot });
        const map = try arena.create(std.process.Environ.Map);
        map.* = try init.environ_map.clone(arena);
        try map.put("ZIG_LIBC", libc_file);
        environ = map;
    }

    // Preprocessed C and LLVM IR: the single cc1 job of the same command as C, with the language put back.
    var wanted: ?[]const u8 = null;
    if (language) |l| {
        if (std.mem.eql(u8, l.lang, "cpp-output") or std.mem.eql(u8, l.lang, "ir")) wanted = l.lang;
    } else {
        for (inputs.items) |index| {
            if (langFromInput(args.items[index])) |lang| {
                if (wanted != null) fail("more than one preprocessed input", .{});
                wanted = lang;
            }
        }
    }
    // `zig cc -fsyntax-only` (the AST dumps of gptr_rewrite.py and crt_flow.py) prints the frontend's output but then fails
    // looking for an object it never asked the frontend to write ("failed to rename '...-a.o' ...: FileNotFound", exit 1):
    // it, too, runs as its single cc1 job, whose own status is the compile's.
    var syntax_only = false;
    var emit_llvm = false;
    for (args.items) |a| {
        syntax_only = syntax_only or std.mem.eql(u8, a, "-fsyntax-only");
        emit_llvm = emit_llvm or std.mem.eql(u8, a, "-emit-llvm");
    }
    // Zig's -S -emit-llvm driver also leaves the requested file unwritten.
    // Replay its cc1 job with the caller's output, as for preprocessed inputs.
    if (wanted == null and !syntax_only and !emit_llvm) {
        try head.appendSlice(arena, args.items);
        execOrRun(io, head.items, environ);
    }
    if (inputs.items.len != 1) fail("a {s} compile needs exactly one input (got {d})", .{ wanted orelse "-fsyntax-only", inputs.items.len });
    const source = args.items[inputs.items[0]];
    var probe: std.ArrayList([]const u8) = .empty;
    try probe.appendSlice(arena, head.items);
    if (wanted == null) {
        try probe.appendSlice(arena, args.items);
    } else if (language) |l| {
        for (args.items, 0..) |a, index| {
            if (index == l.x_index.?) {
                if (l.joined) try probe.append(arena, "-xc") else try probe.append(arena, a);
            } else if (!l.joined and index == l.x_index.? + 1) {
                try probe.append(arena, "c");
            } else try probe.append(arena, a);
        }
    } else {
        for (args.items, 0..) |a, index| {
            if (index == inputs.items[0]) try probe.appendSlice(arena, &.{ "-x", "c" });
            try probe.append(arena, a);
        }
    }
    try probe.append(arena, "-###");
    const result = std.process.run(arena, io, .{ .argv = probe.items, .environ_map = environ }) catch |err|
        fail("cannot run {s} -###: {t}", .{ config.zig_exe, err });
    switch (result.term) {
        .exited => |code| if (code != 0) {
            // Accepted only for -fsyntax-only, and only when Zig's missing-object complaint is its sole error.
            if (!syntax_only) fail("{s} {s} -### failed ({d}):\n{s}", .{ config.zig_exe, driver, code, result.stderr });
            var errors = std.mem.splitScalar(u8, result.stderr, '\n');
            var known = false;
            while (errors.next()) |raw| {
                const line = std.mem.trimEnd(u8, raw, "\r");
                if (std.mem.startsWith(u8, line, " \"") or std.mem.indexOf(u8, line, "error:") == null) continue;
                if (std.mem.indexOf(u8, line, "error: failed to rename '") != null and std.mem.endsWith(u8, line, "': FileNotFound")) {
                    known = true;
                    continue;
                }
                fail("{s} {s} -### failed ({d}):\n{s}", .{ config.zig_exe, driver, code, result.stderr });
            }
            if (!known) fail("{s} {s} -### failed ({d}) without its known -fsyntax-only object error:\n{s}", .{ config.zig_exe, driver, code, result.stderr });
        },
        else => fail("{s} {s} -### crashed:\n{s}", .{ config.zig_exe, driver, result.stderr }),
    }
    var job: ?[]const []const u8 = null;
    var jobs: usize = 0;
    var lines = std.mem.splitScalar(u8, result.stderr, '\n');
    while (lines.next()) |raw| {
        const line = std.mem.trimEnd(u8, raw, "\r");
        if (!std.mem.startsWith(u8, line, " \"")) continue;
        jobs += 1;
        job = splitJob(arena, line) catch |err| fail("cannot parse the -### job ({t}): {s}", .{ err, line });
    }
    if (jobs != 1) fail("expected exactly one job from -### for {s}, got {d}:\n{s}", .{ source, jobs, result.stderr });
    const words = job.?;
    if (words.len < 2 or !std.mem.eql(u8, words[1], "-cc1")) fail("the -### job is not a -cc1 job:\n{s}", .{result.stderr});
    if (syntax_only) {
        var has_syntax_only = false;
        for (words) |w| has_syntax_only = has_syntax_only or std.mem.eql(u8, w, "-fsyntax-only");
        if (!has_syntax_only) fail("the -cc1 job of a -fsyntax-only compile is not -fsyntax-only:\n{s}", .{result.stderr});
    }
    var x_at: ?usize = null;
    var has_source = false;
    for (words, 0..) |w, index| {
        if (std.mem.eql(u8, w, "-x") and index + 1 < words.len and std.mem.eql(u8, words[index + 1], "c")) {
            if (x_at != null) fail("the -cc1 job has more than one `-x c`:\n{s}", .{result.stderr});
            x_at = index + 1;
        }
        if (std.mem.eql(u8, w, source)) has_source = true;
    }
    if (!has_source) fail("the -cc1 job does not name the input {s}:\n{s}", .{ source, result.stderr });
    const exec_argv = try arena.dupe([]const u8, words);
    // Zig's -### frontend job writes to /dev/null even when the driver was
    // given -o. The replay must write the caller's artifact, not an empty
    // placeholder left by a launcher.
    var output: ?[]const u8 = null;
    var arg_index: usize = 0;
    while (arg_index < args.items.len) : (arg_index += 1) {
        const arg = args.items[arg_index];
        if (std.mem.eql(u8, arg, "-o")) {
            if (arg_index + 1 >= args.items.len) fail("-o needs a value", .{});
            output = args.items[arg_index + 1];
            arg_index += 1;
        } else if (takesValue(arg)) {
            arg_index += 1;
        }
    }
    if (output) |path| {
        var output_at: ?usize = null;
        for (words, 0..) |word, index| {
            if (std.mem.eql(u8, word, "-o") and index + 1 < words.len) {
                if (output_at != null) fail("the -cc1 job has more than one output:\n{s}", .{result.stderr});
                output_at = index + 1;
            }
        }
        exec_argv[output_at orelse fail("the -cc1 job has no output:\n{s}", .{result.stderr})] = path;
    } else if (!syntax_only) {
        fail("a {s} compile needs an explicit -o", .{wanted orelse "-fsyntax-only"});
    }
    if (wanted) |lang| {
        exec_argv[x_at orelse fail("the -cc1 job has no `-x c`:\n{s}", .{result.stderr})] = lang;
    }
    execOrRun(io, exec_argv, environ);
}
