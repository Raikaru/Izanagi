//! Luau lexer.
//!
//! Translated from third_party/luau/Ast/src/Lexer.cpp and Ast/include/Luau/Lexer.h
//! (Luau, MIT License; see third_party/luau/LICENSE.txt).

const std = @import("std");
const Allocator = std.mem.Allocator;
const confusables = @import("confusables.zig");

pub const Position = struct {
    line: u32 = 0,
    column: u32 = 0,

    pub fn eql(a: Position, b: Position) bool {
        return a.line == b.line and a.column == b.column;
    }
    pub fn lessThan(a: Position, b: Position) bool {
        return a.line < b.line or (a.line == b.line and a.column < b.column);
    }
};

pub const Location = struct {
    begin: Position = .{},
    end: Position = .{},

    pub fn span(begin: Position, end: Position) Location {
        return .{ .begin = begin, .end = end };
    }
    /// Location(begin, length) in upstream: end is on the same line.
    pub fn withLength(begin: Position, length: u32) Location {
        return .{ .begin = begin, .end = .{ .line = begin.line, .column = begin.column + length } };
    }
    /// Location(const Location& begin, const Location& end)
    pub fn join(a: Location, b: Location) Location {
        return .{ .begin = a.begin, .end = b.end };
    }
    pub fn eql(a: Location, b: Location) bool {
        return a.begin.eql(b.begin) and a.end.eql(b.end);
    }
};

pub const Type = enum(u16) {
    eof = 0,

    // 1..255 means actual character values
    char_end = 256,

    equal,
    less_equal,
    greater_equal,
    not_equal,
    dot2,
    dot3,
    skinny_arrow,
    double_colon,
    floor_div,

    interp_string_begin,
    interp_string_mid,
    interp_string_end,
    // An interpolated string with no expressions (like `x`)
    interp_string_simple,

    add_assign,
    sub_assign,
    mul_assign,
    div_assign,
    floor_div_assign,
    mod_assign,
    pow_assign,
    concat_assign,

    raw_string,
    quoted_string,
    number,
    name,

    comment,
    block_comment,

    attribute,
    attribute_open,

    broken_string,
    broken_comment,
    broken_unicode,
    broken_interp_double_brace,
    @"error",

    reserved_and,
    reserved_break,
    reserved_do,
    reserved_else,
    reserved_elseif,
    reserved_end,
    reserved_false,
    reserved_for,
    reserved_function,
    reserved_if,
    reserved_in,
    reserved_local,
    reserved_nil,
    reserved_not,
    reserved_or,
    reserved_repeat,
    reserved_return,
    reserved_then,
    reserved_true,
    reserved_until,
    reserved_while,
    reserved_end_marker,
    _,

    pub const reserved_begin: Type = .reserved_and;

    pub fn char(c: u8) Type {
        return @enumFromInt(c);
    }

    pub fn isReserved(t: Type) bool {
        const v = @intFromEnum(t);
        return v >= @intFromEnum(Type.reserved_and) and v < @intFromEnum(Type.reserved_end_marker);
    }
};

pub const kReserved = [_][]const u8{ "and", "break", "do", "else", "elseif", "end", "false", "for", "function", "if", "in", "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while" };

pub const QuoteStyle = enum { single, double };

pub const Lexeme = struct {
    type: Type,
    location: Location,
    /// String, Number, Comment data (slice into the source buffer); Name/Attribute/Reserved: interned name.
    data: ?[]const u8 = null,
    /// BrokenUnicode
    codepoint: u32 = 0,

    pub fn init(location: Location, t: Type) Lexeme {
        return .{ .type = t, .location = location };
    }

    pub fn getLength(self: Lexeme) u32 {
        return @intCast(if (self.data) |d| d.len else 0);
    }

    /// Upstream Lexeme::toString; allocated in `arena`.
    pub fn toString(self: Lexeme, arena: Allocator) error{OutOfMemory}![]const u8 {
        return switch (self.type) {
            .eof => "<eof>",
            .equal => "'=='",
            .less_equal => "'<='",
            .greater_equal => "'>='",
            .not_equal => "'~='",
            .dot2 => "'..'",
            .dot3 => "'...'",
            .skinny_arrow => "'->'",
            .double_colon => "'::'",
            .floor_div => "'//'",
            .add_assign => "'+='",
            .sub_assign => "'-='",
            .mul_assign => "'*='",
            .div_assign => "'/='",
            .floor_div_assign => "'//='",
            .mod_assign => "'%='",
            .pow_assign => "'^='",
            .concat_assign => "'..='",
            .raw_string, .quoted_string => if (self.data) |d| try std.fmt.allocPrint(arena, "\"{s}\"", .{d}) else "string",
            .interp_string_begin => if (self.data) |d| try std.fmt.allocPrint(arena, "`{s}{{", .{d}) else "the beginning of an interpolated string",
            .interp_string_mid => if (self.data) |d| try std.fmt.allocPrint(arena, "}}{s}{{", .{d}) else "the middle of an interpolated string",
            .interp_string_end => if (self.data) |d| try std.fmt.allocPrint(arena, "}}{s}`", .{d}) else "the end of an interpolated string",
            .interp_string_simple => if (self.data) |d| try std.fmt.allocPrint(arena, "`{s}`", .{d}) else "interpolated string",
            .number => if (self.data) |d| try std.fmt.allocPrint(arena, "'{s}'", .{d}) else "number",
            .name => if (self.data) |d| try std.fmt.allocPrint(arena, "'{s}'", .{d}) else "identifier",
            .comment => "comment",
            .attribute => if (self.data) |d| try std.fmt.allocPrint(arena, "'{s}'", .{d}) else "attribute",
            .attribute_open => "'@['",
            .broken_string => "malformed string",
            .broken_comment => "unfinished comment",
            .broken_interp_double_brace => "'{{', which is invalid (did you mean '\\{'?)",
            .broken_unicode => blk: {
                if (self.codepoint != 0) {
                    if (confusables.find(self.codepoint)) |c|
                        break :blk try std.fmt.allocPrint(arena, "Unicode character U+{x} (did you mean '{s}'?)", .{ self.codepoint, c });
                    break :blk try std.fmt.allocPrint(arena, "Unicode character U+{x}", .{self.codepoint});
                }
                break :blk "invalid UTF-8 sequence";
            },
            else => blk: {
                const v = @intFromEnum(self.type);
                if (v < 256) {
                    // format("'%c'", type): a zero char terminates the C string early
                    if (v == 0) break :blk "'";
                    break :blk try std.fmt.allocPrint(arena, "'{c}'", .{@as(u8, @intCast(v))});
                } else if (self.type.isReserved()) {
                    break :blk try std.fmt.allocPrint(arena, "'{s}'", .{kReserved[v - @intFromEnum(Type.reserved_and)]});
                }
                break :blk "<unknown>";
            },
        };
    }
};

pub const NameTable = struct {
    const Entry = struct { name: []const u8, type: Type };

    arena: Allocator,
    map: std.StringHashMapUnmanaged(Entry) = .empty,

    pub fn init(arena: Allocator) !NameTable {
        var t: NameTable = .{ .arena = arena };
        for (kReserved, 0..) |word, i| {
            try t.map.put(arena, word, .{ .name = word, .type = @enumFromInt(@intFromEnum(Type.reserved_and) + i) });
        }
        return t;
    }

    pub fn getOrAddWithType(self: *NameTable, name: []const u8) !Entry {
        const gop = try self.map.getOrPut(self.arena, name);
        if (gop.found_existing) return gop.value_ptr.*;
        const copy = try self.arena.dupe(u8, name);
        gop.key_ptr.* = copy;
        gop.value_ptr.* = .{ .name = copy, .type = if (name.len > 0 and name[0] == '@') .attribute else .name };
        return gop.value_ptr.*;
    }

    pub fn getOrAdd(self: *NameTable, name: []const u8) ![]const u8 {
        return (try self.getOrAddWithType(name)).name;
    }

    /// Returns the interned name, or null when the name was never seen.
    pub fn get(self: *const NameTable, name: []const u8) ?[]const u8 {
        if (self.map.get(name)) |e| return e.name;
        return null;
    }
};

pub const BraceType = enum { interpolated_string, normal };

pub inline fn isSpace(ch: u8) bool {
    return ch == ' ' or ch == '\t' or ch == '\r' or ch == '\n' or ch == 0x0b or ch == 0x0c;
}

pub inline fn isAlpha(ch: u8) bool {
    return (ch | ' ') -% 'a' < 26;
}

pub inline fn isDigit(ch: u8) bool {
    return ch -% '0' < 10;
}

pub inline fn isHexDigit(ch: u8) bool {
    return ch -% '0' < 10 or (ch | ' ') -% 'a' < 6;
}

inline fn isNewline(ch: u8) bool {
    return ch == '\n';
}

fn unescape(ch: u8) u8 {
    return switch (ch) {
        'a' => 0x07,
        'b' => 0x08,
        'f' => 0x0c,
        'n' => '\n',
        'r' => '\r',
        't' => '\t',
        'v' => 0x0b,
        else => ch,
    };
}

pub const Lexer = struct {
    buffer: []const u8,
    offset: u32 = 0,
    line: u32 = 0,
    line_offset: u32 = 0,
    lexeme: Lexeme,
    prev_location: Location = .{},
    names: *NameTable,
    skip_comments: bool = false,
    read_names: bool = true,
    brace_stack: std.ArrayList(BraceType) = .empty,
    arena: Allocator,

    pub fn init(arena: Allocator, buffer: []const u8, names: *NameTable) Lexer {
        return .{
            .buffer = buffer,
            .lexeme = Lexeme.init(Location.withLength(.{}, 0), .eof),
            .names = names,
            .arena = arena,
        };
    }

    pub fn current(self: *const Lexer) Lexeme {
        return self.lexeme;
    }

    pub fn previousLocation(self: *const Lexer) Location {
        return self.prev_location;
    }

    pub fn next(self: *Lexer) !Lexeme {
        return self.nextWith(self.skip_comments, true);
    }

    pub fn nextWith(self: *Lexer, skip_comments: bool, update_prev_location_in: bool) !Lexeme {
        var update_prev_location = update_prev_location_in;
        // in skipComments mode we reject valid comments
        while (true) {
            // consume whitespace before the token
            while (isSpace(self.peekch())) self.consumeAny();

            if (update_prev_location) self.prev_location = self.lexeme.location;

            self.lexeme = try self.readNext();
            update_prev_location = false;

            if (!(skip_comments and (self.lexeme.type == .comment or self.lexeme.type == .block_comment))) break;
        }
        return self.lexeme;
    }

    pub fn lookahead(self: *Lexer) !Lexeme {
        const current_offset = self.offset;
        const current_line = self.line;
        const current_line_offset = self.line_offset;
        const current_lexeme = self.lexeme;
        const current_prev_location = self.prev_location;
        const current_brace_stack_size = self.brace_stack.items.len;
        const current_brace_type: BraceType = if (self.brace_stack.items.len == 0) .normal else self.brace_stack.items[self.brace_stack.items.len - 1];

        const result = try self.next();

        self.offset = current_offset;
        self.line = current_line;
        self.line_offset = current_line_offset;
        self.lexeme = current_lexeme;
        self.prev_location = current_prev_location;

        if (self.brace_stack.items.len < current_brace_stack_size) {
            try self.brace_stack.append(self.arena, current_brace_type);
        } else if (self.brace_stack.items.len > current_brace_stack_size) {
            _ = self.brace_stack.pop();
        }

        return result;
    }

    pub fn peekBraceStackTop(self: *const Lexer) ?BraceType {
        if (self.brace_stack.items.len == 0) return null;
        return self.brace_stack.items[self.brace_stack.items.len - 1];
    }

    inline fn peekch(self: *const Lexer) u8 {
        return if (self.offset < self.buffer.len) self.buffer[self.offset] else 0;
    }

    inline fn peekchAt(self: *const Lexer, la: u32) u8 {
        return if (self.offset + la < self.buffer.len) self.buffer[self.offset + la] else 0;
    }

    inline fn position(self: *const Lexer) Position {
        return .{ .line = self.line, .column = self.offset -% self.line_offset };
    }

    inline fn consume(self: *Lexer) void {
        self.offset += 1;
    }

    inline fn consumeAny(self: *Lexer) void {
        if (isNewline(self.buffer[self.offset])) {
            self.line += 1;
            self.line_offset = self.offset + 1;
        }
        self.offset += 1;
    }

    fn slice(self: *const Lexer, start: u32, len: u32) []const u8 {
        return self.buffer[start .. start + len];
    }

    fn readCommentBody(self: *Lexer) Lexeme {
        const start = self.position();
        self.consume();
        self.consume();

        const start_offset = self.offset;

        if (self.peekch() == '[') {
            const sep = self.skipLongSeparator();
            if (sep >= 0) return self.readLongString(start, sep, .block_comment, .broken_comment);
        }

        // fall back to single-line comment
        while (self.peekch() != 0 and self.peekch() != '\r' and !isNewline(self.peekch())) self.consume();

        var l = Lexeme.init(Location.span(start, self.position()), .comment);
        l.data = self.slice(start_offset, self.offset - start_offset);
        return l;
    }

    // Given a sequence [===[ or ]===], returns:
    // 1. number of equal signs (or 0 if none present) between the brackets
    // 2. -1 if this is not a long comment/string separator
    // 3. -N if this is a malformed separator
    // Does *not* consume the closing brace.
    fn skipLongSeparator(self: *Lexer) i32 {
        const start = self.peekch();
        self.consume();
        var count: i32 = 0;
        while (self.peekch() == '=') {
            self.consume();
            count += 1;
        }
        return if (start == self.peekch()) count else (-count) - 1;
    }

    fn readLongString(self: *Lexer, start: Position, sep: i32, ok: Type, broken: Type) Lexeme {
        // skip (second) [
        self.consume();
        const start_offset = self.offset;

        while (self.peekch() != 0) {
            if (self.peekch() == ']') {
                if (self.skipLongSeparator() == sep) {
                    self.consume(); // skip (second) ]
                    const end_offset: u32 = self.offset - @as(u32, @intCast(sep)) - 2;
                    var l = Lexeme.init(Location.span(start, self.position()), ok);
                    l.data = self.slice(start_offset, end_offset - start_offset);
                    return l;
                }
            } else {
                self.consumeAny();
            }
        }

        return Lexeme.init(Location.span(start, self.position()), broken);
    }

    fn readBackslashInString(self: *Lexer) void {
        self.consume();
        switch (self.peekch()) {
            '\r' => {
                self.consume();
                if (self.peekch() == '\n') self.consumeAny();
            },
            0 => {},
            'z' => {
                self.consume();
                while (isSpace(self.peekch())) self.consumeAny();
            },
            else => self.consumeAny(),
        }
    }

    fn readQuotedString(self: *Lexer) Lexeme {
        const start = self.position();
        const delimiter = self.peekch();
        self.consume();
        const start_offset = self.offset;

        while (self.peekch() != delimiter) {
            switch (self.peekch()) {
                0, '\r', '\n' => return Lexeme.init(Location.span(start, self.position()), .broken_string),
                '\\' => self.readBackslashInString(),
                else => self.consume(),
            }
        }

        self.consume();

        var l = Lexeme.init(Location.span(start, self.position()), .quoted_string);
        l.data = self.slice(start_offset, self.offset - start_offset - 1);
        return l;
    }

    fn readInterpolatedStringBegin(self: *Lexer) !Lexeme {
        const start = self.position();
        self.consume();
        return self.readInterpolatedStringSection(start, .interp_string_begin, .interp_string_simple);
    }

    fn readInterpolatedStringSection(self: *Lexer, start: Position, format_type: Type, end_type: Type) !Lexeme {
        const start_offset = self.offset;

        while (self.peekch() != '`') {
            switch (self.peekch()) {
                0, '\r', '\n' => return Lexeme.init(Location.span(start, self.position()), .broken_string),
                '\\' => {
                    // Allow for \u{}, which would otherwise be consumed by looking for {
                    if (self.peekchAt(1) == 'u' and self.peekchAt(2) == '{') {
                        self.consume(); // backslash
                        self.consume(); // u
                        self.consume(); // {
                        continue;
                    }
                    self.readBackslashInString();
                },
                '{' => {
                    try self.brace_stack.append(self.arena, .interpolated_string);

                    if (self.peekchAt(1) == '{') {
                        var broken = Lexeme.init(Location.span(start, self.position()), .broken_interp_double_brace);
                        broken.data = self.slice(start_offset, self.offset - start_offset);
                        self.consume();
                        self.consume();
                        return broken;
                    }

                    self.consume();
                    var l = Lexeme.init(Location.span(start, self.position()), format_type);
                    l.data = self.slice(start_offset, self.offset - start_offset - 1);
                    return l;
                },
                else => self.consume(),
            }
        }

        self.consume();

        var l = Lexeme.init(Location.span(start, self.position()), end_type);
        l.data = self.slice(start_offset, self.offset - start_offset - 1);
        return l;
    }

    fn readNumber(self: *Lexer, start: Position, start_offset: u32) Lexeme {
        // This function does not do the number parsing - it only skips a number-like pattern.
        // It uses the same logic as Lua stock lexer; the resulting string is later converted
        // to a number with proper verification.
        while (true) {
            self.consume();
            if (!(isDigit(self.peekch()) or self.peekch() == '.' or self.peekch() == '_')) break;
        }

        if (self.peekch() == 'e' or self.peekch() == 'E') {
            self.consume();
            if (self.peekch() == '+' or self.peekch() == '-') self.consume();
        }

        while (isAlpha(self.peekch()) or isDigit(self.peekch()) or self.peekch() == '_') self.consume();

        var l = Lexeme.init(Location.span(start, self.position()), .number);
        l.data = self.slice(start_offset, self.offset - start_offset);
        return l;
    }

    fn readName(self: *Lexer) !NameTable.Entry {
        const start_offset = self.offset;
        while (true) {
            self.consume();
            if (!(isAlpha(self.peekch()) or isDigit(self.peekch()) or self.peekch() == '_')) break;
        }
        const text = self.slice(start_offset, self.offset - start_offset);
        if (self.read_names) return self.names.getOrAddWithType(text);
        if (self.names.map.get(text)) |e| return e;
        return .{ .name = "", .type = .name };
    }

    fn readNext(self: *Lexer) !Lexeme {
        const start = self.position();

        switch (self.peekch()) {
            0 => return Lexeme.init(Location.withLength(start, 0), .eof),
            '-' => {
                if (self.peekchAt(1) == '>') {
                    self.consume();
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .skinny_arrow);
                } else if (self.peekchAt(1) == '=') {
                    self.consume();
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .sub_assign);
                } else if (self.peekchAt(1) == '-') {
                    return self.readCommentBody();
                } else {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 1), Type.char('-'));
                }
            },
            '[' => {
                const sep = self.skipLongSeparator();
                if (sep >= 0) {
                    return self.readLongString(start, sep, .raw_string, .broken_string);
                } else if (sep == -1) {
                    return Lexeme.init(Location.withLength(start, 1), Type.char('['));
                } else {
                    return Lexeme.init(Location.span(start, self.position()), .broken_string);
                }
            },
            '{' => {
                self.consume();
                if (self.brace_stack.items.len != 0) try self.brace_stack.append(self.arena, .normal);
                return Lexeme.init(Location.withLength(start, 1), Type.char('{'));
            },
            '}' => {
                self.consume();
                if (self.brace_stack.items.len == 0) return Lexeme.init(Location.withLength(start, 1), Type.char('}'));
                const top = self.brace_stack.pop().?;
                if (top != .interpolated_string) return Lexeme.init(Location.withLength(start, 1), Type.char('}'));
                return self.readInterpolatedStringSection(start, .interp_string_mid, .interp_string_end);
            },
            '=' => {
                self.consume();
                if (self.peekch() == '=') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .equal);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char('='));
            },
            '<' => {
                self.consume();
                if (self.peekch() == '=') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .less_equal);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char('<'));
            },
            '>' => {
                self.consume();
                if (self.peekch() == '=') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .greater_equal);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char('>'));
            },
            '~' => {
                self.consume();
                if (self.peekch() == '=') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .not_equal);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char('~'));
            },
            '"', '\'' => return self.readQuotedString(),
            '`' => return self.readInterpolatedStringBegin(),
            '.' => {
                self.consume();
                if (self.peekch() == '.') {
                    self.consume();
                    if (self.peekch() == '.') {
                        self.consume();
                        return Lexeme.init(Location.withLength(start, 3), .dot3);
                    } else if (self.peekch() == '=') {
                        self.consume();
                        return Lexeme.init(Location.withLength(start, 3), .concat_assign);
                    }
                    return Lexeme.init(Location.withLength(start, 2), .dot2);
                }
                if (isDigit(self.peekch())) return self.readNumber(start, self.offset - 1);
                return Lexeme.init(Location.withLength(start, 1), Type.char('.'));
            },
            '+' => {
                self.consume();
                if (self.peekch() == '=') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .add_assign);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char('+'));
            },
            '/' => {
                self.consume();
                const ch = self.peekch();
                if (ch == '=') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .div_assign);
                } else if (ch == '/') {
                    self.consume();
                    if (self.peekch() == '=') {
                        self.consume();
                        return Lexeme.init(Location.withLength(start, 3), .floor_div_assign);
                    }
                    return Lexeme.init(Location.withLength(start, 2), .floor_div);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char('/'));
            },
            '*' => {
                self.consume();
                if (self.peekch() == '=') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .mul_assign);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char('*'));
            },
            '%' => {
                self.consume();
                if (self.peekch() == '=') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .mod_assign);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char('%'));
            },
            '^' => {
                self.consume();
                if (self.peekch() == '=') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .pow_assign);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char('^'));
            },
            ':' => {
                self.consume();
                if (self.peekch() == ':') {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .double_colon);
                }
                return Lexeme.init(Location.withLength(start, 1), Type.char(':'));
            },
            '(', ')', ']', ';', ',', '#', '?', '&', '|' => {
                const ch = self.peekch();
                self.consume();
                return Lexeme.init(Location.withLength(start, 1), Type.char(ch));
            },
            '@' => {
                if (self.peekchAt(1) == '[') {
                    self.consume();
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 2), .attribute_open);
                }
                // consume @ first
                self.consume();
                var l = Lexeme.init(Location.span(start, self.position()), .attribute);
                if (isAlpha(self.peekch()) or self.peekch() == '_') {
                    const attribute = try self.readName();
                    l.location = Location.span(start, self.position());
                    l.data = attribute.name;
                } else {
                    l.data = "";
                }
                return l;
            },
            else => {
                const ch = self.peekch();
                if (isDigit(ch)) {
                    return self.readNumber(start, self.offset);
                } else if (isAlpha(ch) or ch == '_') {
                    const name = try self.readName();
                    var l = Lexeme.init(Location.span(start, self.position()), name.type);
                    l.data = name.name;
                    return l;
                } else if ((ch & 0x80) != 0) {
                    return self.readUtf8Error();
                } else {
                    self.consume();
                    return Lexeme.init(Location.withLength(start, 1), Type.char(ch));
                }
            },
        }
    }

    fn readUtf8Error(self: *Lexer) Lexeme {
        const start = self.position();
        var codepoint: u32 = 0;
        var size: u32 = 0;
        const ch = self.peekch();

        if ((ch & 0b10000000) == 0b00000000) {
            size = 1;
            codepoint = ch & 0x7F;
        } else if ((ch & 0b11100000) == 0b11000000) {
            size = 2;
            codepoint = ch & 0b11111;
        } else if ((ch & 0b11110000) == 0b11100000) {
            size = 3;
            codepoint = ch & 0b1111;
        } else if ((ch & 0b11111000) == 0b11110000) {
            size = 4;
            codepoint = ch & 0b111;
        } else {
            self.consume();
            return Lexeme.init(Location.span(start, self.position()), .broken_unicode);
        }

        self.consume();

        var i: u32 = 1;
        while (i < size) : (i += 1) {
            if ((self.peekch() & 0b11000000) != 0b10000000) return Lexeme.init(Location.span(start, self.position()), .broken_unicode);
            codepoint = codepoint << 6;
            codepoint |= (self.peekch() & 0b00111111);
            self.consume();
        }

        var result = Lexeme.init(Location.span(start, self.position()), .broken_unicode);
        result.codepoint = codepoint;
        return result;
    }
};

fn toUtf8(data: []u8, code: u32) usize {
    if (code < 0x80) {
        data[0] = @intCast(code);
        return 1;
    } else if (code < 0x800) {
        data[0] = @intCast(0xC0 | (code >> 6));
        data[1] = @intCast(0x80 | (code & 0x3F));
        return 2;
    } else if (code < 0x10000) {
        data[0] = @intCast(0xE0 | (code >> 12));
        data[1] = @intCast(0x80 | ((code >> 6) & 0x3F));
        data[2] = @intCast(0x80 | (code & 0x3F));
        return 3;
    } else if (code < 0x110000) {
        data[0] = @intCast(0xF0 | (code >> 18));
        data[1] = @intCast(0x80 | ((code >> 12) & 0x3F));
        data[2] = @intCast(0x80 | ((code >> 6) & 0x3F));
        data[3] = @intCast(0x80 | (code & 0x3F));
        return 4;
    }
    return 0;
}

/// Upstream Lexer::fixupQuotedString; rewrites `data` in place and returns the new length, or null when malformed.
pub fn fixupQuotedString(data: []u8) ?usize {
    if (data.len == 0 or std.mem.indexOfScalar(u8, data, '\\') == null) return data.len;

    const size = data.len;
    var write: usize = 0;
    var i: usize = 0;

    while (i < size) {
        if (data[i] != '\\') {
            data[write] = data[i];
            write += 1;
            i += 1;
            continue;
        }

        if (i + 1 == size) return null;

        const escape = data[i + 1];
        i += 2; // skip \e

        switch (escape) {
            '\n' => {
                data[write] = '\n';
                write += 1;
            },
            '\r' => {
                data[write] = '\n';
                write += 1;
                if (i < size and data[i] == '\n') i += 1;
            },
            0 => return null,
            'x' => {
                // hex escape codes are exactly 2 hex digits long
                if (i + 2 > size) return null;
                var code: u32 = 0;
                for (0..2) |j| {
                    const ch = data[i + j];
                    if (!isHexDigit(ch)) return null;
                    // use or trick to convert to lower case
                    code = 16 * code + (if (isDigit(ch)) ch - '0' else (ch | ' ') - 'a' + 10);
                }
                data[write] = @intCast(code);
                write += 1;
                i += 2;
            },
            'z' => {
                while (i < size and isSpace(data[i])) i += 1;
            },
            'u' => {
                // unicode escape codes are at least 3 characters including braces
                if (i + 3 > size) return null;
                if (data[i] != '{') return null;
                i += 1;
                if (data[i] == '}') return null;

                var code: u32 = 0;
                for (0..16) |_| {
                    if (i == size) return null;
                    const ch = data[i];
                    if (ch == '}') break;
                    if (!isHexDigit(ch)) return null;
                    // use or trick to convert to lower case
                    code = 16 *% code +% (if (isDigit(ch)) ch - '0' else (ch | ' ') - 'a' + 10);
                    i += 1;
                }

                if (i == size or data[i] != '}') return null;
                i += 1;

                // toUtf8 writes at most 4 bytes; the escape itself consumed at least 4 bytes so this cannot overrun
                var tmp: [4]u8 = undefined;
                const utf8 = toUtf8(&tmp, code);
                if (utf8 == 0) return null;
                @memcpy(data[write .. write + utf8], tmp[0..utf8]);
                write += utf8;
            },
            else => {
                if (isDigit(escape)) {
                    var code: u32 = escape - '0';
                    for (0..2) |_| {
                        if (i == size or !isDigit(data[i])) break;
                        code = 10 * code + (data[i] - '0');
                        i += 1;
                    }
                    if (code > 255) return null;
                    data[write] = @intCast(code);
                    write += 1;
                } else {
                    data[write] = unescape(escape);
                    write += 1;
                }
            },
        }
    }

    return write;
}

/// Upstream Lexer::fixupMultilineString. Note that, like upstream (which walks a NUL-terminated
/// std::string), processing stops at the first embedded zero byte.
pub fn fixupMultilineString(data: []u8) usize {
    if (data.len == 0) return 0;

    var src: usize = 0;
    var dst: usize = 0;
    const at = struct {
        fn f(d: []const u8, idx: usize) u8 {
            return if (idx < d.len) d[idx] else 0;
        }
    }.f;

    // skip leading newline
    if (at(data, 0) == '\r' and at(data, 1) == '\n') {
        src += 2;
    } else if (at(data, 0) == '\n') {
        src += 1;
    }

    // parse the rest of the string, converting newlines as we go
    while (at(data, src) != 0) {
        if (at(data, src) == '\r' and at(data, src + 1) == '\n') {
            data[dst] = '\n';
            dst += 1;
            src += 2;
        } else {
            data[dst] = data[src];
            dst += 1;
            src += 1;
        }
    }

    return dst;
}
