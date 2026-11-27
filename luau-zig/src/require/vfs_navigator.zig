//! Filesystem navigation state for the CLI require resolver.
//!
//! Port of upstream CLI/src/VfsNavigator.cpp and CLI/include/Luau/VfsNavigator.h
//! (Luau 0.744, commit 32c28089305f488f901aaf1dbf3fd1cb65fa3d11). Portions derived
//! from Luau (MIT, Copyright (c) 2019-2026 Roblox Corporation); see
//! third_party/luau/LICENSE.txt.
//!
//! A module path is a file path without its ".luau"/".lua" or "/init.luau"/"/init.lua"
//! suffix; the navigator tracks it both as given (relative or absolute) and as an
//! absolute path, and resolves each to a real file or directory on every move:
//!   * "x" resolves to "x.luau" or "x.lua" (both present: ambiguous), and/or to the
//!     directory "x" whose module is "x/init.luau" or "x/init.lua" (a file and a
//!     directory, or both init files: ambiguous). A directory without an init file is
//!     still a valid navigation target (it can hold configuration files).
//!   * a component named "init" never resolves to init.luau/init.lua directly.
//!   * ".config" can never be navigated into, so ".config.luau" is not requirable.
//! On Windows the drive ("C:") is kept apart in `absolute_path_prefix`, module paths
//! begin at the first '/'.
//!
//! Memory: every path is allocated from the allocator passed to the navigating call and
//! is never freed individually; the owner supplies an arena that outlives a resolution.

const std = @import("std");
const Allocator = std.mem.Allocator;
const Io = std.Io;
const fu = @import("file_utils.zig");

pub const NavigationStatus = enum { success, ambiguous, not_found };

pub const ConfigStatus = enum { absent, ambiguous, present_json, present_luau };

/// Luau::kConfigName
pub const config_name = ".luaurc";
/// Luau::kLuauConfigName
pub const luau_config_name = ".config.luau";

const suffixes = [_][]const u8{ ".luau", ".lua" };
const init_suffixes = [_][]const u8{ "/init.luau", "/init.lua" };

const ResolvedRealPath = struct {
    status: NavigationStatus,
    real_path: []const u8 = "",
};

fn concat(a: Allocator, parts: []const []const u8) Allocator.Error![]u8 {
    return std.mem.concat(a, u8, parts);
}

fn getRealPath(io: Io, a: Allocator, module_path: []const u8) Allocator.Error!ResolvedRealPath {
    var found = false;
    var suffix: []const u8 = "";

    // Upstream asserts a '/' is present; with none, npos + 1 wraps to the whole string.
    const last_component = if (std.mem.lastIndexOfScalar(u8, module_path, '/')) |i| module_path[i + 1 ..] else module_path;

    if (!std.mem.eql(u8, last_component, "init")) {
        for (suffixes) |potential| {
            if (fu.isFile(io, try concat(a, &.{ module_path, potential }))) {
                if (found) return .{ .status = .ambiguous };
                suffix = potential;
                found = true;
            }
        }
    }
    if (fu.isDirectory(io, module_path)) {
        if (found) return .{ .status = .ambiguous };

        for (init_suffixes) |potential| {
            if (fu.isFile(io, try concat(a, &.{ module_path, potential }))) {
                if (found) return .{ .status = .ambiguous };
                suffix = potential;
                found = true;
            }
        }

        found = true;
    }

    if (!found) return .{ .status = .not_found };

    return .{ .status = .success, .real_path = try concat(a, &.{ module_path, suffix }) };
}

fn firstSlashOrLen(path: []const u8) usize {
    // std::string::substr(0, npos) keeps the whole string.
    return std.mem.indexOfScalar(u8, path, '/') orelse path.len;
}

fn getModulePath(a: Allocator, file_path: []const u8) Allocator.Error![]const u8 {
    const owned = try a.dupe(u8, file_path);
    std.mem.replaceScalar(u8, owned, '\\', '/');

    var view: []const u8 = owned;
    if (fu.isAbsolutePath(view)) view = view[firstSlashOrLen(view)..];

    for (init_suffixes) |suffix| {
        if (std.mem.endsWith(u8, view, suffix)) return view[0 .. view.len - suffix.len];
    }
    for (suffixes) |suffix| {
        if (std.mem.endsWith(u8, view, suffix)) return view[0 .. view.len - suffix.len];
    }
    return view;
}

pub const VfsNavigator = struct {
    io: Io,
    real_path: []const u8 = "",
    absolute_real_path: []const u8 = "",
    absolute_path_prefix: []const u8 = "",
    module_path: []const u8 = "",
    absolute_module_path: []const u8 = "",

    pub fn init(io: Io) VfsNavigator {
        return .{ .io = io };
    }

    fn updateRealPaths(self: *VfsNavigator, a: Allocator) Allocator.Error!NavigationStatus {
        const result = try getRealPath(self.io, a, self.module_path);
        const absolute_result = try getRealPath(self.io, a, self.absolute_module_path);
        if (result.status != .success or absolute_result.status != .success)
            return result.status;

        self.real_path = if (fu.isAbsolutePath(result.real_path))
            try concat(a, &.{ self.absolute_path_prefix, result.real_path })
        else
            result.real_path;
        self.absolute_real_path = try concat(a, &.{ self.absolute_path_prefix, absolute_result.real_path });
        return .success;
    }

    /// Points at the REPL's virtual "./stdin" module in the working directory.
    pub fn resetToStdIn(self: *VfsNavigator, a: Allocator) Allocator.Error!NavigationStatus {
        const cwd = try fu.getCurrentWorkingDirectory(self.io, a) orelse return .not_found;

        self.real_path = "./stdin";
        self.absolute_real_path = try fu.normalizePath(a, try concat(a, &.{ cwd, "/stdin" }));
        self.module_path = "./stdin";
        self.absolute_module_path = try getModulePath(a, self.absolute_real_path);
        self.absolute_path_prefix = self.absolute_real_path[0..firstSlashOrLen(self.absolute_real_path)];
        return .success;
    }

    /// Points at the module whose file path (relative to the working directory or absolute) is `path`.
    pub fn resetToPath(self: *VfsNavigator, a: Allocator, path: []const u8) Allocator.Error!NavigationStatus {
        const normalized = try fu.normalizePath(a, path);

        if (fu.isAbsolutePath(normalized)) {
            self.module_path = try getModulePath(a, normalized);
            self.absolute_module_path = self.module_path;
            self.absolute_path_prefix = normalized[0..firstSlashOrLen(normalized)];
        } else {
            const cwd = try fu.getCurrentWorkingDirectory(self.io, a) orelse return .not_found;

            self.module_path = try getModulePath(a, normalized);
            const joined = try fu.normalizePath(a, try concat(a, &.{ cwd, "/", normalized }));
            self.absolute_module_path = try getModulePath(a, joined);
            self.absolute_path_prefix = joined[0..firstSlashOrLen(joined)];
        }

        return self.updateRealPaths(a);
    }

    pub fn toParent(self: *VfsNavigator, a: Allocator) Allocator.Error!NavigationStatus {
        if (std.mem.eql(u8, self.absolute_module_path, "/")) return .not_found;

        const num_slashes = std.mem.count(u8, self.absolute_module_path, "/");
        std.debug.assert(num_slashes > 0);
        if (num_slashes == 1) return .not_found;

        self.module_path = try fu.normalizePath(a, try concat(a, &.{ self.module_path, "/.." }));
        self.absolute_module_path = try fu.normalizePath(a, try concat(a, &.{ self.absolute_module_path, "/.." }));

        // There is no ambiguity when navigating up in a tree.
        const status = try self.updateRealPaths(a);
        return if (status == .ambiguous) .success else status;
    }

    pub fn toChild(self: *VfsNavigator, a: Allocator, name: []const u8) Allocator.Error!NavigationStatus {
        if (std.mem.eql(u8, name, ".config")) return .not_found;

        self.module_path = try fu.normalizePath(a, try concat(a, &.{ self.module_path, "/", name }));
        self.absolute_module_path = try fu.normalizePath(a, try concat(a, &.{ self.absolute_module_path, "/", name }));

        return self.updateRealPaths(a);
    }

    pub fn getFilePath(self: *const VfsNavigator) []const u8 {
        return self.real_path;
    }

    pub fn getAbsoluteFilePath(self: *const VfsNavigator) []const u8 {
        return self.absolute_real_path;
    }

    fn getConfigPath(self: *const VfsNavigator, a: Allocator, filename: []const u8) Allocator.Error![]u8 {
        var directory = self.real_path;
        for (init_suffixes) |suffix| {
            if (std.mem.endsWith(u8, directory, suffix))
                return concat(a, &.{ directory[0 .. directory.len - suffix.len], "/", filename });
        }
        for (suffixes) |suffix| {
            if (std.mem.endsWith(u8, directory, suffix)) {
                directory = directory[0 .. directory.len - suffix.len];
                return concat(a, &.{ directory, "/", filename });
            }
        }
        return concat(a, &.{ directory, "/", filename });
    }

    /// Which configuration file sits next to the current module (in its directory, or
    /// beside the file for a file module). Both .luaurc and .config.luau: ambiguous.
    pub fn getConfigStatus(self: *const VfsNavigator, a: Allocator) Allocator.Error!ConfigStatus {
        const luaurc_exists = fu.isFile(self.io, try self.getConfigPath(a, config_name));
        const luau_config_exists = fu.isFile(self.io, try self.getConfigPath(a, luau_config_name));

        if (luaurc_exists and luau_config_exists) return .ambiguous;
        if (luau_config_exists) return .present_luau;
        if (luaurc_exists) return .present_json;
        return .absent;
    }

    /// Contents of the present configuration file. Called after getConfigStatus reported
    /// present_json or present_luau; if the files changed in between, the read fails (null).
    pub fn getConfig(self: *const VfsNavigator, a: Allocator) Allocator.Error!?[]u8 {
        return switch (try self.getConfigStatus(a)) {
            .present_json => fu.readFile(self.io, a, try self.getConfigPath(a, config_name)),
            .present_luau => fu.readFile(self.io, a, try self.getConfigPath(a, luau_config_name)),
            .absent, .ambiguous => null,
        };
    }
};
