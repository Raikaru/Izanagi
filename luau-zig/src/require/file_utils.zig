//! Filesystem and path helpers used by the CLI require resolver.
//!
//! Port of the subset of upstream CLI/src/FileUtils.cpp (Luau 0.744, commit
//! 32c28089305f488f901aaf1dbf3fd1cb65fa3d11) that require-by-string relies on:
//! isAbsolutePath, normalizePath, resolvePath, joinPaths, getParentPath,
//! splitPath, isFile, isDirectory, readFile and getCurrentWorkingDirectory.
//! Portions derived from Luau (MIT, Copyright (c) 2019-2026 Roblox Corporation);
//! see third_party/luau/LICENSE.txt.
//!
//! Filesystem access goes through `std.Io` (no libc). Paths are UTF-8 (WTF-8 on
//! Windows), exactly the encoding upstream converts with MultiByteToWideChar.
//! Every returned slice is allocated from the caller's allocator.

const std = @import("std");
const builtin = @import("builtin");
const Allocator = std.mem.Allocator;
const Io = std.Io;

const is_windows = builtin.os.tag == .windows;

/// Windows: begins with "X:/", "X:\", "/" or "\" (X a drive letter). Elsewhere: begins with '/'.
pub fn isAbsolutePath(path: []const u8) bool {
    if (is_windows) {
        return (path.len >= 3 and std.ascii.isAlphabetic(path[0]) and path[1] == ':' and (path[2] == '/' or path[2] == '\\')) or
            (path.len >= 1 and (path[0] == '/' or path[0] == '\\'));
    }
    return path.len >= 1 and path[0] == '/';
}

/// Normalizes `path`: '\' and '/' both separate components, "." and empty components
/// disappear, ".." erases the previous erasable component (kept for relative paths that
/// run out of components, dropped at the root of absolute paths). Relative results begin
/// with "./" or "../"; a trailing ".." gains a '/'.
pub fn normalizePath(a: Allocator, path: []const u8) Allocator.Error![]u8 {
    var components: std.ArrayList([]const u8) = .empty;
    defer components.deinit(a);
    var it = std.mem.splitAny(u8, path, "\\/");
    while (it.next()) |component| try components.append(a, component);

    var normalized: std.ArrayList([]const u8) = .empty;
    defer normalized.deinit(a);

    const is_absolute = isAbsolutePath(path);

    // 1. Normalize path components
    const start_index: usize = if (is_absolute) 1 else 0;
    for (components.items[start_index..]) |component| {
        if (std.mem.eql(u8, component, "..")) {
            if (normalized.items.len == 0) {
                if (!is_absolute) try normalized.append(a, "..");
            } else if (std.mem.eql(u8, normalized.items[normalized.items.len - 1], "..")) {
                try normalized.append(a, "..");
            } else {
                _ = normalized.pop();
            }
        } else if (component.len != 0 and !std.mem.eql(u8, component, ".")) {
            try normalized.append(a, component);
        }
    }

    var out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(a);

    // 2. Add correct prefix to formatted path
    if (is_absolute) {
        try out.appendSlice(a, components.items[0]);
        try out.append(a, '/');
    } else if (normalized.items.len == 0 or !std.mem.eql(u8, normalized.items[0], "..")) {
        try out.appendSlice(a, "./");
    }

    // 3. Join path components to form the normalized path
    for (normalized.items, 0..) |component, i| {
        if (i != 0) try out.append(a, '/');
        try out.appendSlice(a, component);
    }
    const n = out.items.len;
    if (n >= 2 and out.items[n - 1] == '.' and out.items[n - 2] == '.')
        try out.append(a, '/');

    return out.toOwnedSlice(a);
}

/// Resolves `path` relative to the directory containing `base_file_path`.
pub fn resolvePath(a: Allocator, path: []const u8, base_file_path: []const u8) Allocator.Error!?[]u8 {
    const parent = getParentPath(base_file_path) orelse return null;
    const joined = try joinPaths(a, parent, path);
    defer a.free(joined);
    return try normalizePath(a, joined);
}

/// Joins with a single '/' unless `lhs` is empty or already ends in a separator.
pub fn joinPaths(a: Allocator, lhs: []const u8, rhs: []const u8) Allocator.Error![]u8 {
    const sep = lhs.len != 0 and lhs[lhs.len - 1] != '/' and lhs[lhs.len - 1] != '\\';
    return std.mem.concat(a, u8, &.{ lhs, if (sep) "/" else "", rhs });
}

/// Returns the parent of `path` (a slice of it, "/" or ""), or null at a root.
pub fn getParentPath(path: []const u8) ?[]const u8 {
    if (path.len == 0 or std.mem.eql(u8, path, ".") or std.mem.eql(u8, path, "/")) return null;
    if (is_windows and path.len == 2 and path[1] == ':') return null;

    const slash = std.mem.lastIndexOfAny(u8, path, "\\/") orelse return "";
    if (slash == 0) return "/";
    return path[0..slash];
}

fn kindOf(io: Io, path: []const u8) ?Io.File.Kind {
    // Upstream uses lstat on POSIX (symbolic links are neither files nor directories) and
    // GetFileAttributesW on Windows (which reports the link target's file/directory bit).
    const st = Io.Dir.cwd().statFile(io, path, .{ .follow_symlinks = is_windows }) catch return null;
    return st.kind;
}

pub fn isFile(io: Io, path: []const u8) bool {
    const kind = kindOf(io, path) orelse return false;
    if (is_windows) return kind != .directory;
    return kind == .file;
}

pub fn isDirectory(io: Io, path: []const u8) bool {
    return kindOf(io, path) == .directory;
}

/// Reads the whole file, or null when it cannot be opened/read. A leading "#!" line is
/// dropped up to (not including) its newline, or entirely when there is none.
/// The result is caller-owned (free with `a`).
pub fn readFile(io: Io, a: Allocator, path: []const u8) Allocator.Error!?[]u8 {
    const bytes = Io.Dir.cwd().readFileAlloc(io, path, a, .unlimited) catch |err| switch (err) {
        error.OutOfMemory => return error.OutOfMemory,
        else => return null,
    };
    if (bytes.len > 2 and bytes[0] == '#' and bytes[1] == '!') {
        defer a.free(bytes);
        const newline = std.mem.indexOfScalar(u8, bytes, '\n') orelse bytes.len;
        return try a.dupe(u8, bytes[newline..]);
    }
    return bytes;
}

/// The process working directory (caller-owned), or null when it cannot be determined.
pub fn getCurrentWorkingDirectory(io: Io, a: Allocator) Allocator.Error!?[:0]u8 {
    return std.process.currentPathAlloc(io, a) catch |err| switch (err) {
        error.OutOfMemory => return error.OutOfMemory,
        else => return null,
    };
}

// Upstream tests/RequireByString.test.cpp: PathResolution and PathNormalization.

const root_prefix = if (is_windows) "C:/" else "/";

test "normalizePath" {
    const a = std.testing.allocator;
    const cases = [_][2][]const u8{
        .{ "", "./" },
        .{ ".", "./" },
        .{ "..", "../" },
        .{ "a/relative/path", "./a/relative/path" },
        .{ "./remove/extraneous/symbols/", "./remove/extraneous/symbols" },
        .{ "./remove/extraneous//symbols", "./remove/extraneous/symbols" },
        .{ "./remove/extraneous/symbols/.", "./remove/extraneous/symbols" },
        .{ "./remove/extraneous/./symbols", "./remove/extraneous/symbols" },
        .{ "../remove/extraneous/symbols/", "../remove/extraneous/symbols" },
        .{ "../remove/extraneous//symbols", "../remove/extraneous/symbols" },
        .{ "../remove/extraneous/symbols/.", "../remove/extraneous/symbols" },
        .{ "../remove/extraneous/./symbols", "../remove/extraneous/symbols" },
        .{ root_prefix ++ "remove/extraneous/symbols/", root_prefix ++ "remove/extraneous/symbols" },
        .{ root_prefix ++ "remove/extraneous//symbols", root_prefix ++ "remove/extraneous/symbols" },
        .{ root_prefix ++ "remove/extraneous/symbols/.", root_prefix ++ "remove/extraneous/symbols" },
        .{ root_prefix ++ "remove/extraneous/./symbols", root_prefix ++ "remove/extraneous/symbols" },
        .{ "./remove/me/..", "./remove" },
        .{ "./remove/me/../", "./remove" },
        .{ "../remove/me/..", "../remove" },
        .{ "../remove/me/../", "../remove" },
        .{ root_prefix ++ "remove/me/..", root_prefix ++ "remove" },
        .{ root_prefix ++ "remove/me/../", root_prefix ++ "remove" },
        .{ "./..", "../" },
        .{ "./../", "../" },
        .{ "../..", "../../" },
        .{ "../../", "../../" },
        .{ root_prefix ++ "..", root_prefix },
    };
    for (cases) |c| {
        const got = try normalizePath(a, c[0]);
        defer a.free(got);
        try std.testing.expectEqualStrings(c[1], got);
    }
}

test "resolvePath" {
    const a = std.testing.allocator;
    const cases = [_][3][]const u8{
        .{ "./dep", "./src/modules/module.luau", "./src/modules/dep" },
        .{ "../dep", "./src/modules/module.luau", "./src/dep" },
        .{ "../../dep", "./src/modules/module.luau", "./dep" },
        .{ "../../", "./src/modules/module.luau", "./" },
        .{ "./dep", "../src/modules/module.luau", "../src/modules/dep" },
        .{ "../dep", "../src/modules/module.luau", "../src/dep" },
        .{ "../../dep", "../src/modules/module.luau", "../dep" },
        .{ "../../", "../src/modules/module.luau", "../" },
        .{ "./dep", root_prefix ++ "src/modules/module.luau", root_prefix ++ "src/modules/dep" },
        .{ "../dep", root_prefix ++ "src/modules/module.luau", root_prefix ++ "src/dep" },
        .{ "../../dep", root_prefix ++ "src/modules/module.luau", root_prefix ++ "dep" },
        .{ "../../", root_prefix ++ "src/modules/module.luau", root_prefix },
        .{ "../../../", "./src/modules/module.luau", "../" },
        .{ "../../../", "../src/modules/module.luau", "../../" },
        .{ "../../../", root_prefix ++ "src/modules/module.luau", root_prefix },
    };
    for (cases) |c| {
        const got = (try resolvePath(a, c[0], c[1])).?;
        defer a.free(got);
        try std.testing.expectEqualStrings(c[2], got);
    }
}
