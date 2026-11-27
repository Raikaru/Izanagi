//! `.luaurc` configuration model and JSON-subset parser.
//!
//! Port of upstream Config/src/Config.cpp, Config/include/Luau/Config.h and the lint
//! name table of Config/include/Luau/LinterConfig.h + Config/src/LinterConfig.cpp
//! (Luau 0.744, commit 32c28089305f488f901aaf1dbf3fd1cb65fa3d11). Portions derived
//! from Luau (MIT, Copyright (c) 2019-2026 Roblox Corporation); see
//! third_party/luau/LICENSE.txt.
//!
//! Require only consumes `aliases`, but a configuration file is accepted or rejected as
//! a whole, so every key upstream understands is parsed and validated with upstream's
//! messages (languageMode, lint, lintErrors, typeErrors, globals, aliases).
//!
//! The parser tokenizes with the Zig port of the Luau lexer, exactly as upstream: string
//! values are the raw text between the quotes (escapes are not processed), `//` skips
//! the rest of a line, a NUL byte ends the input, numbers/null/nested arrays are
//! rejected, and Lua `--` comments are reported as unexpected "comment" tokens.
//!
//! Memory: everything is allocated from the caller's allocator and never freed
//! individually (use an arena).

const std = @import("std");
const Allocator = std.mem.Allocator;
const lexer = @import("../compiler/lexer.zig");

/// Luau::kConfigName
pub const config_name = ".luaurc";

/// Optional error message: null on success.
pub const Error = ?[]const u8;

pub const Mode = enum { no_check, nonstrict, strict };

/// LintWarning::Code names, indexed by code (kWarningNames).
pub const warning_names = [_][]const u8{
    "Unknown",
    "UnknownGlobal",
    "DeprecatedGlobal",
    "GlobalUsedAsLocal",
    "LocalShadow",
    "SameLineStatement",
    "MultiLineStatement",
    "LocalUnused",
    "FunctionUnused",
    "ImportUnused",
    "BuiltinGlobalWrite",
    "PlaceholderRead",
    "UnreachableCode",
    "UnknownType",
    "ForRange",
    "UnbalancedAssignment",
    "ImplicitReturn",
    "DuplicateLocal",
    "FormatString",
    "TableLiteral",
    "UninitializedLocal",
    "DuplicateFunction",
    "DeprecatedApi",
    "TableOperations",
    "DuplicateCondition",
    "MisleadingAndOr",
    "CommentDirective",
    "IntegerParsing",
    "ComparisonPrecedence",
    "RedundantNativeAttribute",
};
const code_unknown: u6 = 0;

/// LintWarning::parseName: the code for `name`, or 0 (Unknown) when unrecognized.
pub fn parseLintName(name: []const u8) u6 {
    for (warning_names, 0..) |candidate, code| {
        if (std.mem.eql(u8, name, candidate)) return @intCast(code);
    }
    return code_unknown;
}

pub const LintOptions = struct {
    warning_mask: u64 = 0,

    /// LintOptions::setDefaults: every warning enabled.
    pub const defaults: LintOptions = .{ .warning_mask = ~@as(u64, 0) };

    pub fn enableWarning(self: *LintOptions, code: u6) void {
        self.warning_mask |= @as(u64, 1) << code;
    }
    pub fn disableWarning(self: *LintOptions, code: u6) void {
        self.warning_mask &= ~(@as(u64, 1) << code);
    }
    pub fn isEnabled(self: LintOptions, code: u6) bool {
        return self.warning_mask & (@as(u64, 1) << code) != 0;
    }
};

pub const AliasInfo = struct {
    value: []const u8,
    /// Empty when the alias was set without a configuration location.
    config_location: []const u8 = "",
    /// The alias in its original case.
    original_case: []const u8,
};

pub const Config = struct {
    mode: Mode = .nonstrict,
    enabled_lint: LintOptions = .defaults,
    fatal_lint: LintOptions = .{},
    lint_errors: bool = false,
    type_errors: bool = true,
    globals: std.ArrayList([]const u8) = .empty,
    /// Keyed by the ASCII-lowercased alias.
    aliases: std.StringHashMapUnmanaged(AliasInfo) = .empty,

    pub fn setAlias(self: *Config, a: Allocator, alias: []const u8, value: []const u8, config_location: ?[]const u8) Allocator.Error!void {
        const gop = try self.aliases.getOrPut(a, try toLower(a, alias));
        gop.value_ptr.* = .{
            .value = value,
            .original_case = alias,
            .config_location = config_location orelse "",
        };
    }

    pub fn containsAlias(self: *const Config, alias: []const u8) bool {
        return self.aliases.contains(alias);
    }

    pub fn getAlias(self: *const Config, alias: []const u8) ?AliasInfo {
        return self.aliases.get(alias);
    }
};

pub const AliasOptions = struct {
    config_location: ?[]const u8 = null,
    overwrite_aliases: bool,
};

pub const ConfigOptions = struct {
    compat: bool = false,
    alias_options: ?AliasOptions = null,
};

pub fn toLower(a: Allocator, s: []const u8) Allocator.Error![]u8 {
    const out = try a.dupe(u8, s);
    for (out) |*c| {
        if ('A' <= c.* and c.* <= 'Z') c.* += 'a' - 'A';
    }
    return out;
}

fn fmtError(a: Allocator, comptime fmt: []const u8, args: anytype) Allocator.Error!Error {
    return try std.fmt.allocPrint(a, fmt, args);
}

fn parseBoolean(a: Allocator, result: *bool, value: []const u8) Allocator.Error!Error {
    if (std.mem.eql(u8, value, "true")) {
        result.* = true;
    } else if (std.mem.eql(u8, value, "false")) {
        result.* = false;
    } else {
        return fmtError(a, "Bad setting '{s}'.  Valid options are true and false", .{value});
    }
    return null;
}

pub fn parseModeString(a: Allocator, mode: *Mode, mode_string: []const u8, compat: bool) Allocator.Error!Error {
    if (std.mem.eql(u8, mode_string, "nocheck")) {
        mode.* = .no_check;
    } else if (std.mem.eql(u8, mode_string, "strict")) {
        mode.* = .strict;
    } else if (std.mem.eql(u8, mode_string, "nonstrict")) {
        mode.* = .nonstrict;
    } else if (std.mem.eql(u8, mode_string, "noinfer") and compat) {
        mode.* = .no_check;
    } else {
        return fmtError(a, "Bad mode \"{s}\".  Valid options are nocheck, nonstrict, and strict", .{mode_string});
    }
    return null;
}

fn parseLintRuleStringForCode(a: Allocator, enabled: *LintOptions, fatal: *LintOptions, code: u6, value: []const u8, compat: bool) Allocator.Error!Error {
    if (std.mem.eql(u8, value, "true")) {
        enabled.enableWarning(code);
    } else if (std.mem.eql(u8, value, "false")) {
        enabled.disableWarning(code);
    } else if (compat) {
        if (std.mem.eql(u8, value, "enabled")) {
            enabled.enableWarning(code);
            fatal.disableWarning(code);
        } else if (std.mem.eql(u8, value, "disabled")) {
            enabled.disableWarning(code);
            fatal.disableWarning(code);
        } else if (std.mem.eql(u8, value, "fatal")) {
            enabled.enableWarning(code);
            fatal.enableWarning(code);
        } else {
            return fmtError(a, "Bad setting '{s}'.  Valid options are enabled, disabled, and fatal", .{value});
        }
    } else {
        return fmtError(a, "Bad setting '{s}'.  Valid options are true and false", .{value});
    }
    return null;
}

pub fn parseLintRuleString(a: Allocator, enabled: *LintOptions, fatal: *LintOptions, warning_name: []const u8, value: []const u8, compat: bool) Allocator.Error!Error {
    if (std.mem.eql(u8, warning_name, "*")) {
        for (0..warning_names.len) |code| {
            if (try parseLintRuleStringForCode(a, enabled, fatal, @intCast(code), value, compat)) |err|
                return fmtError(a, "In key {s}: {s}", .{ warning_name, err });
        }
    } else {
        const code = parseLintName(warning_name);
        if (code == code_unknown) return fmtError(a, "Unknown lint {s}", .{warning_name});

        if (try parseLintRuleStringForCode(a, enabled, fatal, code, value, compat)) |err|
            return fmtError(a, "In key {s}: {s}", .{ warning_name, err });
    }
    return null;
}

/// Non-empty, not "." or "..", no path separators; letters, digits, '-', '_', '.' only
/// (a leading '@' is tolerated).
pub fn isValidAlias(alias: []const u8) bool {
    if (alias.len == 0) return false;

    const alias_is_not_a_path = !std.mem.eql(u8, alias, ".") and !std.mem.eql(u8, alias, "..") and
        std.mem.indexOfAny(u8, alias, "\\/") == null;
    if (!alias_is_not_a_path) return false;

    for (alias, 0..) |ch, i| {
        if (i == 0 and ch == '@') continue;
        const ok = std.ascii.isUpper(ch) or std.ascii.isLower(ch) or std.ascii.isDigit(ch) or ch == '-' or ch == '_' or ch == '.';
        if (!ok) return false;
    }
    return true;
}

pub fn parseAlias(a: Allocator, config: *Config, alias_key: []const u8, alias_value: []const u8, alias_options: ?AliasOptions) Allocator.Error!Error {
    if (!isValidAlias(alias_key)) return fmtError(a, "Invalid alias {s}", .{alias_key});

    const options = alias_options orelse return "Cannot parse aliases without alias options";

    // Upstream looks the key up in its original case while the map is lowercase-keyed:
    // a later differently-cased duplicate therefore overwrites unless it is already lowercase.
    if (options.overwrite_aliases or !config.containsAlias(alias_key))
        try config.setAlias(a, alias_key, alias_value, options.config_location);

    return null;
}

const Json = struct {
    a: Allocator,
    lx: lexer.Lexer,

    fn tok(c: u8) lexer.Type {
        return lexer.Type.char(c);
    }

    fn current(self: *const Json) lexer.Lexeme {
        return self.lx.current();
    }

    /// Advances, skipping C-style `//` comments to the end of the line (upstream `next`).
    fn next(self: *Json) Allocator.Error!void {
        _ = try self.lx.next();
        while (self.lx.current().type == .floor_div) {
            // Lexer::nextline
            while (self.lx.offset < self.lx.buffer.len) : (self.lx.offset += 1) {
                const ch = self.lx.buffer[self.lx.offset];
                if (ch == 0 or ch == '\r' or ch == '\n') break;
            }
            _ = try self.lx.next();
        }
    }

    fn fail(self: *Json, message: []const u8) Allocator.Error!Error {
        const cur = self.current();
        return fmtError(self.a, "Expected {s} at line {d}, got {s} instead", .{ message, cur.location.begin.line + 1, try cur.toString(self.a) });
    }

    fn data(self: *const Json) []const u8 {
        return self.current().data orelse "";
    }
};

/// Upstream `parseJson`: walks the document calling `action.act(keys, value)` for every
/// string/true/false leaf with the stack of enclosing keys.
fn parseJson(a: Allocator, contents: []const u8, action: anytype) Allocator.Error!Error {
    var names = try lexer.NameTable.init(a);
    var p: Json = .{ .a = a, .lx = lexer.Lexer.init(a, contents, &names) };
    try p.next();

    var keys: std.ArrayList([]const u8) = .empty;
    var array_top = false; // we don't support nested arrays

    if (p.current().type != Json.tok('{')) return p.fail("'{'");
    try p.next();

    while (true) {
        const t = p.current().type;
        if (array_top) {
            if (t == Json.tok(']')) {
                try p.next();
                array_top = false;

                std.debug.assert(keys.items.len != 0);
                _ = keys.pop();

                if (p.current().type == Json.tok(',')) {
                    try p.next();
                } else if (p.current().type != Json.tok('}')) {
                    return p.fail("',' or '}'");
                }
            } else if (t == .quoted_string) {
                const value = p.data();
                try p.next();

                if (try action.act(keys.items, value)) |err| return err;

                if (p.current().type == Json.tok(',')) {
                    try p.next();
                } else if (p.current().type != Json.tok(']')) {
                    return p.fail("',' or ']'");
                }
            } else {
                return p.fail("array element or ']'");
            }
        } else {
            if (t == Json.tok('}')) {
                try p.next();

                if (keys.items.len == 0) {
                    if (p.current().type != .eof) return p.fail("end of file");
                    return null;
                } else {
                    _ = keys.pop();
                }

                if (p.current().type == Json.tok(',')) {
                    try p.next();
                } else if (p.current().type != Json.tok('}')) {
                    return p.fail("',' or '}'");
                }
            } else if (t == .quoted_string) {
                const key = p.data();
                try p.next();

                try keys.append(a, key);

                if (p.current().type != Json.tok(':')) return p.fail("':'");
                try p.next();

                const vt = p.current().type;
                if (vt == Json.tok('{') or vt == Json.tok('[')) {
                    array_top = vt == Json.tok('[');
                    try p.next();
                } else if (vt == .quoted_string or vt == .reserved_true or vt == .reserved_false) {
                    const value = if (vt == .quoted_string) p.data() else if (vt == .reserved_true) "true" else "false";
                    try p.next();

                    if (try action.act(keys.items, value)) |err| return err;

                    _ = keys.pop();

                    if (p.current().type == Json.tok(',')) {
                        try p.next();
                    } else if (p.current().type != Json.tok('}')) {
                        return p.fail("',' or '}'");
                    }
                } else {
                    return p.fail("field value");
                }
            } else {
                return p.fail("field key");
            }
        }
    }
}

/// Luau::parseConfig: parses `.luaurc` contents into `config`; returns the first error.
pub fn parseConfig(a: Allocator, contents: []const u8, config: *Config, options: ConfigOptions) Allocator.Error!Error {
    const Action = struct {
        a: Allocator,
        config: *Config,
        options: ConfigOptions,

        fn act(self: @This(), keys: []const []const u8, value: []const u8) Allocator.Error!Error {
            const c = self.config;
            if (keys.len == 1 and std.mem.eql(u8, keys[0], "languageMode")) {
                return parseModeString(self.a, &c.mode, value, self.options.compat);
            } else if (keys.len == 2 and std.mem.eql(u8, keys[0], "lint")) {
                return parseLintRuleString(self.a, &c.enabled_lint, &c.fatal_lint, keys[1], value, self.options.compat);
            } else if (keys.len == 1 and std.mem.eql(u8, keys[0], "lintErrors")) {
                return parseBoolean(self.a, &c.lint_errors, value);
            } else if (keys.len == 1 and std.mem.eql(u8, keys[0], "typeErrors")) {
                return parseBoolean(self.a, &c.type_errors, value);
            } else if (keys.len == 1 and std.mem.eql(u8, keys[0], "globals")) {
                try c.globals.append(self.a, value);
                return null;
            } else if (keys.len == 2 and std.mem.eql(u8, keys[0], "aliases")) {
                return parseAlias(self.a, c, keys[1], value, self.options.alias_options);
            } else if (self.options.compat and keys.len == 2 and std.mem.eql(u8, keys[0], "language") and std.mem.eql(u8, keys[1], "mode")) {
                return parseModeString(self.a, &c.mode, value, self.options.compat);
            } else {
                return fmtError(self.a, "Unknown key {s}", .{try std.mem.join(self.a, "/", keys)});
            }
        }
    };
    return parseJson(a, contents, Action{ .a = a, .config = config, .options = options });
}

test "parseConfig aliases" {
    var arena_state = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena_state.deinit();
    const a = arena_state.allocator();

    var config: Config = .{};
    const opts: ConfigOptions = .{ .alias_options = .{ .config_location = "/default/location", .overwrite_aliases = true } };
    const err = try parseConfig(a,
        \\{
        \\    "aliases": {
        \\        "MyAlias": "/my/alias/path",
        \\    }
        \\}
    , &config, opts);
    try std.testing.expect(err == null);
    try std.testing.expectEqual(@as(u32, 1), config.aliases.count());
    const info = config.getAlias("myalias").?;
    try std.testing.expectEqualStrings("/my/alias/path", info.value);
    try std.testing.expectEqualStrings("MyAlias", info.original_case);
    try std.testing.expectEqualStrings("/default/location", info.config_location);
}

test "parseConfig errors" {
    var arena_state = std.heap.ArenaAllocator.init(std.testing.allocator);
    defer arena_state.deinit();
    const a = arena_state.allocator();
    const opts: ConfigOptions = .{ .alias_options = .{ .overwrite_aliases = false } };

    var config: Config = .{};
    try std.testing.expectEqualStrings("Unknown key foo/bar", (try parseConfig(a, "{\"foo\": {\"bar\": \"x\"}}", &config, opts)).?);
    try std.testing.expectEqualStrings("Unknown lint Nope", (try parseConfig(a, "{\"lint\": {\"Nope\": true}}", &config, opts)).?);
    try std.testing.expectEqualStrings("Invalid alias @@", (try parseConfig(a, "{\"aliases\": {\"@@\": \"./x\"}}", &config, opts)).?);
    try std.testing.expectEqualStrings("Expected field value at line 1, got '1' instead", (try parseConfig(a, "{\"x\": 1}", &config, opts)).?);
    try std.testing.expect((try parseConfig(a, "// comment\n{\"languageMode\": \"strict\"} // trailing", &config, opts)) == null);
    try std.testing.expectEqual(Mode.strict, config.mode);
}
