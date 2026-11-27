//! Luau parser (including the typed syntax).
//!
//! Translated from third_party/luau/Ast/src/Parser.cpp (Luau, MIT License; see
//! third_party/luau/LICENSE.txt) with all upstream feature flags at their
//! default (off) values: no `export` value syntax, no user-defined classes, no
//! `if local`, no integer literals, no exact table types, no declaration
//! syntax (ParseOptions::allowDeclarationSyntax defaults to false).
//!
//! Upstream recovers from errors and records them; luau_compile only reports
//! the first one. This parser stops at the first reported error, which yields
//! the identical first diagnostic (message and location) because no recovery
//! can have happened before the first report.

const std = @import("std");
const Allocator = std.mem.Allocator;
const ast = @import("ast.zig");
const lex = @import("lexer.zig");
const Lexer = lex.Lexer;
const Lexeme = lex.Lexeme;
const T = lex.Type;
const Location = lex.Location;
const Position = lex.Position;
const Expr = ast.Expr;
const Stat = ast.Stat;
const AType = ast.Type;
const TypePack = ast.TypePack;
const flags_mod = @import("flags.zig");

pub const Error = error{ ParseError, OutOfMemory };

pub const Diagnostic = struct {
    location: Location = .{},
    message: []const u8 = "",
};

const kRecursionLimit: u32 = 1000;
const kTypeLengthLimit: u32 = 1000;
const kParseNameError = "%error-id%";

pub const ParseResult = struct {
    root: *Stat,
    hotcomments: []ast.HotComment,
};

const Name = struct {
    name: ast.Name,
    location: Location,
};

const Binding = struct {
    name: Name,
    annotation: ?*AType,
    is_const: bool = false,
};

const FunctionState = struct {
    vararg: bool,
    loop_depth: u32 = 0,
};

const MatchLexeme = struct {
    type: T,
    position: Position,

    fn from(l: Lexeme) MatchLexeme {
        return .{ .type = l.type, .position = l.location.begin };
    }
};

pub const Parser = struct {
    arena: Allocator,
    lexer: Lexer,
    names: *lex.NameTable,
    diag: *Diagnostic,
    flags: flags_mod.Flags,

    function_stack: std.ArrayList(FunctionState) = .empty,
    type_function_depth: usize = 0,

    local_map: std.StringHashMapUnmanaged(?*ast.Local) = .empty,
    local_stack: std.ArrayList(*ast.Local) = .empty,

    hotcomments: std.ArrayList(ast.HotComment) = .empty,
    hotcomment_header: bool = true,

    recursion_counter: u32 = 0,

    name_self: ast.Name = "",
    name_number: ast.Name = "",
    name_error: ast.Name = "",
    name_nil: ast.Name = "",

    end_mismatch_suspect: MatchLexeme = .{ .type = .eof, .position = .{} },

    // LuauExportValueSyntax state
    declared_export_bindings: std.StringHashMapUnmanaged(void) = .empty,
    has_module_return: bool = false,

    // DebugLuauUserDefinedClasses state
    in_class_method_body: bool = false,
    classes_within_module: std.StringHashMapUnmanaged(void) = .empty,

    pub fn parse(arena: Allocator, source: []const u8, names: *lex.NameTable, diag: *Diagnostic, flags: flags_mod.Flags) Error!ParseResult {
        var p: Parser = .{
            .arena = arena,
            .lexer = Lexer.init(arena, source, names),
            .names = names,
            .diag = diag,
            .flags = flags,
        };

        try p.function_stack.append(arena, .{ .vararg = true });

        p.name_self = try names.getOrAdd("self");
        p.name_number = try names.getOrAdd("number");
        p.name_error = try names.getOrAdd(kParseNameError);
        p.name_nil = try names.getOrAdd("nil");

        // required for lookahead() to work across a comment boundary and for nextLexeme() to work when captureComments is false
        p.lexer.skip_comments = true;

        // read first lexeme (any hot comments get .header = true)
        try p.nextLexeme();

        // all hot comments parsed after the first non-comment lexeme are special in that they don't affect type checking / linting mode
        p.hotcomment_header = false;

        const root = try p.parseChunk();
        return .{ .root = root, .hotcomments = p.hotcomments.items };
    }

    // ---------------------------------------------------------------- helpers

    fn cur(self: *const Parser) Lexeme {
        return self.lexer.current();
    }

    fn curType(self: *const Parser) T {
        return self.lexer.current().type;
    }

    fn isName(self: *const Parser, comptime text: []const u8) bool {
        const c = self.cur();
        return c.type == .name and std.mem.eql(u8, c.data.?, text);
    }

    fn newExpr(self: *Parser, location: Location, data: Expr.Data) Error!*Expr {
        const e = try self.arena.create(Expr);
        e.* = .{ .location = location, .data = data };
        return e;
    }

    fn newStat(self: *Parser, location: Location, data: Stat.Data) Error!*Stat {
        const s = try self.arena.create(Stat);
        s.* = .{ .location = location, .data = data };
        return s;
    }

    fn newType(self: *Parser, location: Location, data: AType.Data) Error!*AType {
        const t = try self.arena.create(AType);
        t.* = .{ .location = location, .data = data };
        return t;
    }

    fn newPack(self: *Parser, location: Location, data: TypePack.Data) Error!*TypePack {
        const t = try self.arena.create(TypePack);
        t.* = .{ .location = location, .data = data };
        return t;
    }

    fn copy(self: *Parser, comptime E: type, items: []const E) Error![]E {
        return self.arena.dupe(E, items);
    }

    fn report(self: *Parser, location: Location, comptime fmt: []const u8, args: anytype) Error {
        const message = std.fmt.allocPrint(self.arena, fmt, args) catch |e| return e;
        self.diag.* = .{ .location = location, .message = message };
        return error.ParseError;
    }

    fn tokStr(self: *Parser, l: Lexeme) Error![]const u8 {
        return l.toString(self.arena);
    }

    fn typeStr(self: *Parser, t: T) Error![]const u8 {
        return Lexeme.init(.{}, t).toString(self.arena);
    }

    fn nextLexeme(self: *Parser) Error!void {
        var t = (try self.lexer.nextWith(false, true)).type;

        while (t == .broken_comment or t == .comment or t == .block_comment) {
            const lexeme = self.lexer.current();

            // Subtlety: Broken comments are weird because we record them as comments AND pass them to the parser as a lexeme.
            // The parser will turn this into a proper syntax error.
            if (lexeme.type == .broken_comment) return;

            // Comments starting with ! are called "hot comments" and contain directives for type checking / linting / compiling
            if (lexeme.type == .comment and lexeme.getLength() != 0 and lexeme.data.?[0] == '!') {
                const text = lexeme.data.?;
                var end: usize = text.len;
                while (end > 0 and lex.isSpace(text[end - 1])) end -= 1;
                try self.hotcomments.append(self.arena, .{ .header = self.hotcomment_header, .location = lexeme.location, .content = text[1..@max(end, 1)] });
            }

            t = (try self.lexer.nextWith(false, false)).type;
        }
    }

    fn incrementRecursionCounter(self: *Parser, comptime context: []const u8) Error!void {
        self.recursion_counter += 1;
        if (self.recursion_counter > kRecursionLimit)
            return self.report(self.cur().location, "Exceeded allowed recursion depth; simplify your " ++ context ++ " to make the code compile", .{});
    }

    fn expectAndConsume(self: *Parser, t: T, context: ?[]const u8) Error!void {
        if (self.curType() != t) return self.expectAndConsumeFail(t, context);
        try self.nextLexeme();
    }

    fn expectAndConsumeFail(self: *Parser, t: T, context: ?[]const u8) Error {
        const type_string = self.typeStr(t) catch |e| return e;
        const curr = self.tokStr(self.cur()) catch |e| return e;
        if (context) |ctx|
            return self.report(self.cur().location, "Expected {s} when parsing {s}, got {s}", .{ type_string, ctx, curr });
        return self.report(self.cur().location, "Expected {s}, got {s}", .{ type_string, curr });
    }

    fn expectMatchAndConsume(self: *Parser, value: u8, begin: MatchLexeme) Error!void {
        const t = T.char(value);
        if (self.curType() != t) return self.expectMatchAndConsumeFail(t, begin, null);
        try self.nextLexeme();
    }

    fn expectMatchAndConsumeFail(self: *Parser, t: T, begin: MatchLexeme, extra: ?[]const u8) Error {
        const type_string = self.typeStr(t) catch |e| return e;
        const match_string = self.typeStr(begin.type) catch |e| return e;
        const curr = self.tokStr(self.cur()) catch |e| return e;
        if (self.cur().location.begin.line == begin.position.line)
            return self.report(self.cur().location, "Expected {s} (to close {s} at column {d}), got {s}{s}", .{ type_string, match_string, begin.position.column + 1, curr, extra orelse "" });
        return self.report(self.cur().location, "Expected {s} (to close {s} at line {d}), got {s}{s}", .{ type_string, match_string, begin.position.line + 1, curr, extra orelse "" });
    }

    fn expectMatchEndAndConsume(self: *Parser, t: T, begin: MatchLexeme) Error!void {
        if (self.curType() != t) {
            if (self.end_mismatch_suspect.type != .eof and self.end_mismatch_suspect.position.line > begin.position.line) {
                const match_string = try self.typeStr(self.end_mismatch_suspect.type);
                const suggestion = try std.fmt.allocPrint(self.arena, "; did you forget to close {s} at line {d}?", .{ match_string, self.end_mismatch_suspect.position.line + 1 });
                return self.expectMatchAndConsumeFail(t, begin, suggestion);
            }
            return self.expectMatchAndConsumeFail(t, begin, null);
        }

        // If the token matches on a different line and a different column, it suggests misleading indentation
        // This can be used to pinpoint the problem location for a possible future *actual* mismatch
        if (self.cur().location.begin.line != begin.position.line and self.cur().location.begin.column != begin.position.column and
            self.end_mismatch_suspect.position.line < begin.position.line) // Only replace the previous suspect with more recent suspects
        {
            self.end_mismatch_suspect = begin;
        }

        try self.nextLexeme();
    }

    fn reportNameError(self: *Parser, context: ?[]const u8) Error {
        const curr = self.tokStr(self.cur()) catch |e| return e;
        if (context) |ctx|
            return self.report(self.cur().location, "Expected identifier when parsing {s}, got {s}", .{ ctx, curr });
        return self.report(self.cur().location, "Expected identifier, got {s}", .{curr});
    }

    fn parseName(self: *Parser, context: ?[]const u8) Error!Name {
        if (self.curType() != .name) return self.reportNameError(context);
        const result: Name = .{ .name = self.cur().data.?, .location = self.cur().location };
        try self.nextLexeme();
        return result;
    }

    fn parseIndexName(self: *Parser, context: ?[]const u8, previous: Position) Error!Name {
        _ = previous;
        // Upstream accepts a same-line reserved word after reporting the missing-name error; the error is fatal here.
        return self.parseName(context);
    }

    fn saveLocals(self: *const Parser) usize {
        return self.local_stack.items.len;
    }

    fn restoreLocals(self: *Parser, offset: usize) Error!void {
        var i = self.local_stack.items.len;
        while (i > offset) : (i -= 1) {
            const l = self.local_stack.items[i - 1];
            try self.local_map.put(self.arena, l.name, l.shadow);
        }
        self.local_stack.shrinkRetainingCapacity(offset);
    }

    fn pushLocal(self: *Parser, binding: Binding) Error!*ast.Local {
        const gop = try self.local_map.getOrPut(self.arena, binding.name.name);
        const shadow: ?*ast.Local = if (gop.found_existing) gop.value_ptr.* else null;
        const fs = &self.function_stack.items[self.function_stack.items.len - 1];
        const local = try self.arena.create(ast.Local);
        local.* = .{
            .name = binding.name.name,
            .location = binding.name.location,
            .shadow = shadow,
            .function_depth = @intCast(self.function_stack.items.len - 1),
            .loop_depth = fs.loop_depth,
            .is_const = binding.is_const,
            .annotation = binding.annotation,
        };
        gop.value_ptr.* = local;
        try self.local_stack.append(self.arena, local);
        return local;
    }

    fn topFunction(self: *Parser) *FunctionState {
        return &self.function_stack.items[self.function_stack.items.len - 1];
    }

    // ---------------------------------------------------------------- statements

    fn blockFollow(l: Lexeme) bool {
        return l.type == .eof or l.type == .reserved_else or l.type == .reserved_elseif or l.type == .reserved_end or l.type == .reserved_until;
    }

    fn parseChunk(self: *Parser) Error!*Stat {
        const result = try self.parseBlock();
        if (self.curType() != .eof) return self.expectAndConsumeFail(.eof, null);
        return result;
    }

    // chunk ::= {stat [`;']} [laststat [`;']]
    // block ::= chunk
    fn parseBlock(self: *Parser) Error!*Stat {
        const locals_begin = self.saveLocals();
        const result = try self.parseBlockNoScope();
        try self.restoreLocals(locals_begin);
        return result;
    }

    fn isStatLast(stat: *Stat) bool {
        return switch (stat.data) {
            .@"break", .@"continue", .@"return" => true,
            else => false,
        };
    }

    fn parseBlockNoScope(self: *Parser) Error!*Stat {
        var body: std.ArrayList(*Stat) = .empty;

        const prev_position = self.lexer.previousLocation().end;

        while (!blockFollow(self.cur())) {
            const old_recursion_count = self.recursion_counter;
            try self.incrementRecursionCounter("block");

            const stat = try self.parseStat();

            self.recursion_counter = old_recursion_count;

            if (self.curType() == T.char(';')) {
                try self.nextLexeme();
                stat.has_semicolon = true;
                stat.location.end = self.lexer.previousLocation().end;
            }

            try body.append(self.arena, stat);

            if (isStatLast(stat)) break;
        }

        const location = Location.span(prev_position, self.cur().location.begin);
        return self.newStat(location, .{ .block = .{ .body = body.items, .has_end = true } });
    }

    fn parseStat(self: *Parser) Error!*Stat {
        // guess the type from the token type
        switch (self.curType()) {
            .reserved_if => return self.parseIf(),
            .reserved_while => return self.parseWhile(),
            .reserved_do => return self.parseDo(),
            .reserved_for => return self.parseFor(),
            .reserved_repeat => return self.parseRepeat(),
            .reserved_function => return self.parseFunctionStat(&.{}),
            .reserved_local => {
                const start = self.cur().location;
                return self.parseLocal(start, &.{}, false);
            },
            .reserved_return => return self.parseReturn(),
            .reserved_break => return self.parseBreak(),
            .attribute, .attribute_open => return self.parseAttributeStat(),
            else => {},
        }

        const start = self.cur().location;

        // we need to disambiguate a few cases, primarily assignment (lvalue = ...) vs statements-that-are calls
        const expr = try self.parsePrimaryExpr(true);

        if (expr.data == .call) return self.newStat(expr.location, .{ .expr = expr });

        // if the next token is , or =, it's an assignment (, means it's an assignment with multiple variables)
        if (self.curType() == T.char(',') or self.curType() == T.char('=')) return self.parseAssignment(expr);

        // if the next token is a compound assignment operator, it's a compound assignment (these don't support multiple variables)
        if (parseCompoundOp(self.cur())) |op| return self.parseCompoundAssignment(expr, op);

        // we know this isn't a call or an assignment; therefore it must be a context-sensitive keyword such as `type` or `continue`
        const ident: ?ast.Name = switch (expr.data) {
            .global => |g| g,
            .local => |l| l.local.name,
            else => null,
        };

        if (ident) |id| {
            if (std.mem.eql(u8, id, "type")) return self.parseTypeAlias(expr.location, false);

            if (self.flags.DebugLuauUserDefinedClasses and std.mem.eql(u8, id, "class")) {
                return self.parseClassStat(start, false, false);
            } else if (self.flags.DebugLuauUserDefinedClasses and std.mem.eql(u8, id, "open")) {
                if (!self.isName("class"))
                    return self.report(expr.location, "Incomplete statement: expected a class definition after 'open'", .{});

                try self.nextLexeme(); // consume `class`
                return self.parseClassStat(start, false, true);
            }

            if (std.mem.eql(u8, id, "export")) {
                if (self.flags.LuauExportValueSyntax) {
                    var is_export_value = self.curType() == .reserved_local or self.curType() == .reserved_function or self.isName("const");

                    if (self.flags.DebugLuauUserDefinedClasses)
                        is_export_value = is_export_value or self.isName("class") or self.isName("open");

                    if (is_export_value) return self.parseExportValue(expr.location, &.{});

                    if (self.isName("type")) {
                        try self.nextLexeme();
                        return self.parseTypeAlias(expr.location, true);
                    }
                } else if (self.flags.DebugLuauUserDefinedClasses and self.isName("class")) {
                    try self.nextLexeme();
                    return self.parseClassStat(start, true, false);
                } else if (self.flags.DebugLuauUserDefinedClasses and self.isName("open")) {
                    try self.nextLexeme(); // consume `open`

                    if (!self.isName("class"))
                        return self.report(expr.location, "Incomplete statement: expected a class definition after 'open'", .{});

                    try self.nextLexeme(); // consume `class`
                    return self.parseClassStat(start, true, true);
                } else if (self.isName("type")) {
                    try self.nextLexeme();
                    return self.parseTypeAlias(expr.location, true);
                }
            }

            if (std.mem.eql(u8, id, "continue")) return self.parseContinue(expr.location);

            if (std.mem.eql(u8, id, "const")) return self.parseLocal(expr.location, &.{}, true);
        }

        return self.report(expr.location, "Incomplete statement: expected assignment or a function call", .{});
    }

    fn isIfLocalStart(self: *Parser) Error!bool {
        if (!self.flags.LuauExperimentalIfLocalSyntax) return false;
        if (self.curType() == .reserved_local) return true;
        return self.isName("const") and (try self.lexer.lookahead()).type == .name;
    }

    // if exp then block {elseif exp then block} [else block] end
    // (`if' | `elseif') (`local' | `const') binding `=' exp then block {elseif exp then block} [else block] end
    fn parseIf(self: *Parser) Error!*Stat {
        const start = self.cur().location;

        try self.nextLexeme(); // if / elseif

        var cond_local: ?*ast.Local = null;
        var cond_is_const = false;
        var locals_begin: usize = 0;
        var cond: *Expr = undefined;

        if (try self.isIfLocalStart()) {
            cond_is_const = self.isName("const");
            try self.nextLexeme(); // consume 'local' or 'const'

            const binding = try self.parseBinding(cond_is_const);

            if (self.curType() == T.char(','))
                return self.report(self.cur().location, "Expected '=' after variable name in 'if local', got ','; only a single binding is allowed", .{});

            try self.expectAndConsume(T.char('='), "if local declaration");

            cond = try self.parseExpr(0);

            locals_begin = self.saveLocals();
            cond_local = try self.pushLocal(binding);
        } else {
            cond = try self.parseExpr(0);
        }

        const match_then = self.cur();
        try self.expectAndConsume(.reserved_then, "if statement");

        const thenbody = try self.parseBlock();

        // Restore locals after then-block so condLocal is not visible in else/elseif
        if (cond_local != null) try self.restoreLocals(locals_begin);

        var elsebody: ?*Stat = null;
        var end = start;

        if (self.curType() == .reserved_elseif) {
            thenbody.data.block.has_end = true;
            const old_recursion_count = self.recursion_counter;
            try self.incrementRecursionCounter("elseif");
            const e = try self.parseIf();
            elsebody = e;
            end = e.location;
            self.recursion_counter = old_recursion_count;
        } else {
            var match_then_else = MatchLexeme.from(match_then);

            if (self.curType() == .reserved_else) {
                thenbody.data.block.has_end = true;
                match_then_else = MatchLexeme.from(self.cur());
                const else_end = self.cur().location.end;
                try self.nextLexeme();

                const e = try self.parseBlock();
                e.location.begin = else_end;
                elsebody = e;
            }

            end = self.cur().location;

            try self.expectMatchEndAndConsume(.reserved_end, match_then_else);
        }

        return self.newStat(Location.join(start, end), .{ .@"if" = .{ .condition = cond, .thenbody = thenbody, .elsebody = elsebody, .condition_local = cond_local, .condition_is_const = cond_is_const } });
    }

    // while exp do block end
    fn parseWhile(self: *Parser) Error!*Stat {
        const start = self.cur().location;

        try self.nextLexeme(); // while

        const cond = try self.parseExpr(0);

        const match_do = self.cur();
        try self.expectAndConsume(.reserved_do, "while loop");

        self.topFunction().loop_depth += 1;
        const body = try self.parseBlock();
        self.topFunction().loop_depth -= 1;

        const end = self.cur().location;

        try self.expectMatchEndAndConsume(.reserved_end, MatchLexeme.from(match_do));

        return self.newStat(Location.join(start, end), .{ .@"while" = .{ .condition = cond, .body = body } });
    }

    // repeat block until exp
    fn parseRepeat(self: *Parser) Error!*Stat {
        const start = self.cur().location;

        const match_repeat = self.cur();
        try self.nextLexeme(); // repeat

        const locals_begin = self.saveLocals();

        self.topFunction().loop_depth += 1;
        const body = try self.parseBlockNoScope();
        self.topFunction().loop_depth -= 1;

        try self.expectMatchEndAndConsume(.reserved_until, MatchLexeme.from(match_repeat));

        const cond = try self.parseExpr(0);

        try self.restoreLocals(locals_begin);

        return self.newStat(Location.join(start, cond.location), .{ .repeat = .{ .condition = cond, .body = body } });
    }

    // do block end
    fn parseDo(self: *Parser) Error!*Stat {
        const start = self.cur().location;

        const match_do = self.cur();
        try self.nextLexeme(); // do

        const body = try self.parseBlock();

        body.location.begin = start.begin;

        const end_location = self.cur().location;
        try self.expectMatchEndAndConsume(.reserved_end, MatchLexeme.from(match_do));
        body.location.end = end_location.end;

        return body;
    }

    // break
    fn parseBreak(self: *Parser) Error!*Stat {
        const start = self.cur().location;

        try self.nextLexeme(); // break

        if (self.topFunction().loop_depth == 0) return self.report(start, "break statement must be inside a loop", .{});

        return self.newStat(start, .@"break");
    }

    // continue
    fn parseContinue(self: *Parser, start: Location) Error!*Stat {
        if (self.topFunction().loop_depth == 0) return self.report(start, "continue statement must be inside a loop", .{});

        // note: the token is already parsed for us!
        return self.newStat(start, .@"continue");
    }

    // for binding `=' exp `,' exp [`,' exp] do block end |
    // for bindinglist in explist do block end |
    fn parseFor(self: *Parser) Error!*Stat {
        const start = self.cur().location;

        try self.nextLexeme(); // for

        const varname = try self.parseBinding(false);

        if (self.curType() == T.char('=')) {
            try self.nextLexeme();

            const from = try self.parseExpr(0);

            try self.expectAndConsume(T.char(','), "index range");

            const to = try self.parseExpr(0);

            var step: ?*Expr = null;

            if (self.curType() == T.char(',')) {
                try self.nextLexeme();
                step = try self.parseExpr(0);
            }

            const match_do = self.cur();
            try self.expectAndConsume(.reserved_do, "for loop");

            const locals_begin = self.saveLocals();

            self.topFunction().loop_depth += 1;

            const variable = try self.pushLocal(varname);

            const body = try self.parseBlock();

            self.topFunction().loop_depth -= 1;

            try self.restoreLocals(locals_begin);

            const end = self.cur().location;

            try self.expectMatchEndAndConsume(.reserved_end, MatchLexeme.from(match_do));

            return self.newStat(Location.join(start, end), .{ .@"for" = .{ .variable = variable, .from = from, .to = to, .step = step, .body = body } });
        } else {
            var names: std.ArrayList(Binding) = .empty;
            try names.append(self.arena, varname);

            if (self.curType() == T.char(',')) {
                try self.nextLexeme();
                _ = try self.parseBindingList(&names, false, false);
            }

            try self.expectAndConsume(.reserved_in, "for loop");

            var values: std.ArrayList(*Expr) = .empty;
            try self.parseExprList(&values);

            const match_do = self.cur();
            try self.expectAndConsume(.reserved_do, "for loop");

            const locals_begin = self.saveLocals();

            self.topFunction().loop_depth += 1;

            var vars: std.ArrayList(*ast.Local) = .empty;
            for (names.items) |n| try vars.append(self.arena, try self.pushLocal(n));

            const body = try self.parseBlock();

            self.topFunction().loop_depth -= 1;

            try self.restoreLocals(locals_begin);

            const end = self.cur().location;

            try self.expectMatchEndAndConsume(.reserved_end, MatchLexeme.from(match_do));

            return self.newStat(Location.join(start, end), .{ .for_in = .{ .vars = vars.items, .values = values.items, .body = body } });
        }
    }

    const FunctionName = struct {
        expr: *Expr,
        has_self: bool = false,
        debug_name: ?ast.Name = null,
    };

    // funcname ::= Name {`.' Name} [`:' Name]
    fn parseFunctionName(self: *Parser) Error!FunctionName {
        var debugname: ?ast.Name = if (self.curType() == .name) self.cur().data.? else null;

        // parse funcname into a chain of indexing operators
        var expr = try self.parseNameExpr("function name");

        const old_recursion_count = self.recursion_counter;

        while (self.curType() == T.char('.')) {
            const op_position = self.cur().location.begin;
            try self.nextLexeme();

            const name = try self.parseName("field name");

            // while we could concatenate the name chain, for now let's just write the short name
            debugname = name.name;

            expr = try self.newExpr(Location.join(expr.location, name.location), .{ .index_name = .{ .expr = expr, .index = name.name, .index_location = name.location, .op_position = op_position, .op = '.' } });

            // note: while the parser isn't recursive here, we're generating recursive structures of unbounded depth
            try self.incrementRecursionCounter("function name");
        }

        self.recursion_counter = old_recursion_count;

        // finish with :
        if (self.curType() == T.char(':')) {
            const op_position = self.cur().location.begin;
            try self.nextLexeme();

            const name = try self.parseName("method name");

            debugname = name.name;

            expr = try self.newExpr(Location.join(expr.location, name.location), .{ .index_name = .{ .expr = expr, .index = name.name, .index_location = name.location, .op_position = op_position, .op = ':' } });

            return .{ .expr = expr, .has_self = true, .debug_name = debugname };
        }

        return .{ .expr = expr, .debug_name = debugname };
    }

    fn isExprLValue(expr: *Expr) bool {
        return switch (expr.data) {
            .local => |l| !l.local.is_const,
            .global, .index_expr, .index_name => true,
            else => false,
        };
    }

    // function funcname funcbody
    fn parseFunctionStat(self: *Parser, attributes: []const *ast.Attr) Error!*Stat {
        const start = if (attributes.len > 0) attributes[0].location else self.cur().location;

        const match_function = self.cur();
        try self.nextLexeme();

        const name = try self.parseFunctionName();

        if (!isExprLValue(name.expr)) return self.reportLValueError(name.expr);

        const body = try self.parseFunctionBody(name.has_self, match_function, name.debug_name, null, attributes, false);

        return self.newStat(Location.join(start, body.func.location), .{ .function = .{ .name = name.expr, .func = body.func } });
    }

    fn validateAttribute(self: *Parser, loc: Location, attribute_name: []const u8, attributes: []const *ast.Attr, args: []const *Expr) Error!ast.AttrType {
        // check if the attribute name is valid
        const t: ?ast.AttrType = if (std.mem.eql(u8, attribute_name, "checked"))
            .checked
        else if (std.mem.eql(u8, attribute_name, "native"))
            .native
        else if (std.mem.eql(u8, attribute_name, "deprecated"))
            .deprecated
        else if (self.flags.DebugLuauNoInline and std.mem.eql(u8, attribute_name, "debugnoinline"))
            .debug_noinline
        else
            null;

        if (t == null) {
            if (attribute_name.len == 0) return self.report(loc, "Attribute name is missing", .{});
            return self.report(loc, "Invalid attribute '@{s}'", .{attribute_name});
        }

        // check that attribute is not duplicated
        for (attributes) |attr| {
            if (attr.type == t.?) return self.report(loc, "Cannot duplicate attribute '@{s}'", .{attribute_name});
        }

        if (t.? == .deprecated) try self.validateDeprecatedArgs(loc, args);

        return t.?;
    }

    fn validateDeprecatedArgs(self: *Parser, attr_loc: Location, args: []const *Expr) Error!void {
        if (args.len == 0) return;
        if (args.len > 1) return self.report(attr_loc, "@deprecated can be parametrized only by 1 argument", .{});

        const items = switch (args[0].data) {
            .table => |t| t,
            else => return self.report(args[0].location, "Unknown argument type for @deprecated", .{}),
        };

        for (items) |item| {
            if (item.kind == .record) {
                const key = item.key.?.data.constant_string;
                if (!std.mem.eql(u8, key, "use") and !std.mem.eql(u8, key, "reason")) {
                    return self.report(item.key.?.location, "Unknown argument '{s}' for @deprecated. Only string constants for 'use' and 'reason' are allowed", .{cstr(key)});
                } else if (item.value.data != .constant_string) {
                    return self.report(item.value.location, "Only constant string allowed as value for '{s}'", .{cstr(key)});
                }
            } else {
                return self.report(item.value.location, "Only constants keys 'use' and 'reason' are allowed for @deprecated attribute", .{});
            }
        }
    }

    fn isConstantLiteral(expr: *const Expr) bool {
        return switch (expr.data) {
            .constant_nil, .constant_bool, .constant_number, .constant_string => true,
            else => false,
        };
    }

    fn isLiteralTable(expr: *const Expr) bool {
        const items = switch (expr.data) {
            .table => |t| t,
            else => return false,
        };
        for (items) |item| {
            switch (item.kind) {
                .general => return false,
                .record, .list => if (!isConstantLiteral(item.value) and !isLiteralTable(item.value)) return false,
            }
        }
        return true;
    }

    // attrlist = '@[' parattr {',' parattr} ']'
    fn parseAttrList(self: *Parser, attributes: *std.ArrayList(*ast.Attr)) Error!void {
        const open = self.cur();
        try self.nextLexeme();

        if (self.curType() != T.char(']')) {
            while (true) {
                const name = try self.parseName("attribute name");

                const name_loc = name.location;
                const attr_name = name.name;

                const arg_open_type = self.curType();

                if (arg_open_type == .raw_string or arg_open_type == .quoted_string or arg_open_type == T.char('{') or arg_open_type == T.char('(')) {
                    const call_list = try self.parseCallList();

                    for (call_list.args) |arg| {
                        if (!isConstantLiteral(arg) and !isLiteralTable(arg))
                            return self.report(call_list.args_location, "Only literals can be passed as arguments for attributes", .{});
                    }

                    const t = try self.validateAttribute(name_loc, attr_name, attributes.items, call_list.args);

                    const node = try self.arena.create(ast.Attr);
                    node.* = .{ .location = Location.join(name_loc, call_list.args_location), .type = t, .args = call_list.args, .name = attr_name };
                    try attributes.append(self.arena, node);
                } else {
                    const t = try self.validateAttribute(name_loc, attr_name, attributes.items, &.{});

                    const node = try self.arena.create(ast.Attr);
                    node.* = .{ .location = name_loc, .type = t, .args = &.{}, .name = attr_name };
                    try attributes.append(self.arena, node);
                }

                if (self.curType() == T.char(',')) {
                    try self.nextLexeme();
                } else {
                    break;
                }
            }
        } else {
            return self.report(Location.join(open.location, self.cur().location), "Attribute list cannot be empty", .{});
        }

        try self.expectMatchAndConsume(']', MatchLexeme.from(open));
    }

    // attribute ::= '@' NAME
    fn parseAttribute(self: *Parser, attributes: *std.ArrayList(*ast.Attr)) Error!void {
        const loc = self.cur().location;
        const name = self.cur().data.?;
        const t = try self.validateAttribute(loc, name, attributes.items, &.{});

        try self.nextLexeme();

        const node = try self.arena.create(ast.Attr);
        node.* = .{ .location = loc, .type = t, .args = &.{}, .name = name };
        try attributes.append(self.arena, node);
    }

    // attributes ::= {attribute}
    fn parseAttributes(self: *Parser) Error![]const *ast.Attr {
        var attributes: std.ArrayList(*ast.Attr) = .empty;

        while (self.curType() == .attribute or self.curType() == .attribute_open) {
            if (self.curType() == .attribute)
                try self.parseAttribute(&attributes)
            else
                try self.parseAttrList(&attributes);
        }

        return attributes.items;
    }

    // attributes local function Name funcbody
    // attributes function funcname funcbody
    fn parseAttributeStat(self: *Parser) Error!*Stat {
        const start_location = self.cur().location;

        const attributes = try self.parseAttributes();

        const attr_start = if (attributes.len > 0) attributes[0].location else start_location;

        switch (self.curType()) {
            .reserved_function => return self.parseFunctionStat(attributes),
            .reserved_local => return self.parseLocal(attr_start, attributes, false),
            .name => {
                if (self.flags.LuauExportValueSyntax and std.mem.eql(u8, "export", self.cur().data.?)) {
                    try self.nextLexeme();
                    return self.parseExportValue(attr_start, attributes);
                }

                if (std.mem.eql(u8, "const", self.cur().data.?)) {
                    try self.nextLexeme();
                    return self.parseLocal(attr_start, attributes, true);
                }
            },
            else => {},
        }

        return self.report(self.cur().location, "Expected 'function', 'local function', 'const function', 'declare function' or a function type declaration after attribute, but got {s} instead", .{try self.tokStr(self.cur())});
    }

    fn isEnoughValues(values: []const *Expr, expected: usize) bool {
        if (values.len > 0) {
            const last = values[values.len - 1];
            if (last.data == .call or last.data == .varargs) return true;
        }
        return values.len == expected;
    }

    fn parseLocal(self: *Parser, start: Location, attributes: []const *ast.Attr, is_const: bool) Error!*Stat {
        if (!is_const) try self.nextLexeme(); // local

        if (self.curType() == .reserved_function) {
            var match_function = self.cur();
            try self.nextLexeme();

            // matchFunction is only used for diagnostics; to make it suitable for detecting missed indentation between
            // `local function` and `end`, we patch the token to begin at the column where `local` starts
            if (match_function.location.begin.line == start.begin.line) match_function.location.begin.column = start.begin.column;

            const name = try self.parseName("variable name");

            const result = try self.parseFunctionBody(false, match_function, name.name, name, attributes, is_const);

            const location = Location.span(start.begin, result.func.location.end);

            return self.newStat(location, .{ .local_function = .{ .name = result.local.?, .func = result.func, .is_const = is_const } });
        } else {
            if (attributes.len != 0) {
                return self.report(self.cur().location, "Expected 'function' after local declaration with attribute, but got {s} instead", .{try self.tokStr(self.cur())});
            }

            var names: std.ArrayList(Binding) = .empty;
            _ = try self.parseBindingList(&names, false, is_const);

            var vars: std.ArrayList(*ast.Local) = .empty;
            var values: std.ArrayList(*Expr) = .empty;

            if (self.curType() == T.char('=')) {
                try self.nextLexeme();
                try self.parseExprList(&values);
            }

            for (names.items) |n| try vars.append(self.arena, try self.pushLocal(n));

            const end = if (values.items.len == 0) self.lexer.previousLocation() else values.items[values.items.len - 1].location;

            const node = try self.newStat(Location.join(start, end), .{ .local = .{ .vars = vars.items, .values = values.items, .is_const = is_const } });

            // It is a syntax error when a const declaration *definitely* does not have enough values
            if (is_const and !isEnoughValues(values.items, vars.items.len))
                return self.report(node.location, "Missing initializer in const declaration", .{});

            return node;
        }
    }

    // return [explist]
    fn parseReturn(self: *Parser) Error!*Stat {
        const start = self.cur().location;

        try self.nextLexeme();

        var list: std.ArrayList(*Expr) = .empty;

        if (!blockFollow(self.cur()) and self.curType() != T.char(';')) try self.parseExprList(&list);

        const end = if (list.items.len == 0) start else list.items[list.items.len - 1].location;

        const node = try self.newStat(Location.join(start, end), .{ .@"return" = list.items });

        if (self.flags.LuauExportValueSyntax and self.function_stack.items.len == 1) {
            if (self.declared_export_bindings.count() != 0)
                return self.report(node.location, "Exporting values is not compatible with top-level return (export/return conflict)", .{});

            self.has_module_return = true;
        }

        return node;
    }

    // `export' (`local' bindinglist [`=' explist] | `const' bindinglist `=' explist | `function' Name funcbody)
    fn parseExportValue(self: *Parser, start: Location, attributes: []const *ast.Attr) Error!*Stat {
        if (self.function_stack.items.len != 1 or self.recursion_counter != 1)
            return self.report(start, "'export' may only be applied to top-level statements", .{});

        if (self.has_module_return)
            return self.report(start, "Exporting values is not compatible with top-level return (export/return conflict)", .{});

        if (attributes.len != 0 and self.curType() != .reserved_function)
            return self.report(self.cur().location, "Expected 'function' after export declaration with attribute, but got {s} instead", .{try self.tokStr(self.cur())});

        if (self.curType() == .reserved_local) {
            if ((try self.lexer.lookahead()).type == .reserved_function)
                return self.report(start, "'export' must be followed by an identifier or 'function'; try removing 'local'", .{});

            return self.exportLocalStat(try self.parseLocal(start, &.{}, false));
        } else if (self.curType() == .reserved_function) {
            const func_stat = try self.parseLocal(start, attributes, true);
            const name = func_stat.data.local_function.name;

            if (!try self.checkDuplicateExport(name.name))
                return self.report(name.location, "Duplicate exported identifier '{s}'", .{cstr(name.name)});

            name.is_exported = true;
            name.is_const = true;
            return func_stat;
        } else if (self.isName("const")) {
            try self.nextLexeme();

            if (self.curType() == .reserved_function)
                return self.report(start, "'export' must be followed by an identifier or 'function'", .{});

            return self.exportLocalStat(try self.parseLocal(start, &.{}, true));
        } else if (self.flags.DebugLuauUserDefinedClasses and (self.isName("class") or self.isName("open"))) {
            const open = self.isName("open");

            try self.nextLexeme();
            if (open) {
                if (!self.isName("class"))
                    return self.report(start, "Incomplete statement: expected a class definition after 'open'", .{});

                try self.nextLexeme(); // consume 'class' after 'open'
            }

            const stat = try self.parseClassStat(start, true, open);
            const class_local = stat.data.class.name;
            if (!try self.checkDuplicateExport(class_local.name))
                return self.report(class_local.location, "Duplicate exported class '{s}'", .{cstr(class_local.name)});

            class_local.is_exported = true;
            return stat;
        }

        return self.report(start, "'export' must be followed by an identifier or 'function'", .{});
    }

    // classStatement ::= [`open`] `class` Name [`extends` classrefexp] classProps `end`
    // classProps ::= classProp [classProps]
    fn parseClassStat(self: *Parser, start: Location, exported: bool, open: bool) Error!*Stat {
        std.debug.assert(self.flags.DebugLuauUserDefinedClasses);
        const name = try self.parseName("type name");

        // We save the locals here as part of error recovery later.
        const saved_locals = self.saveLocals();

        const name_local = try self.pushLocal(.{ .name = name, .annotation = null, .is_const = true });

        var super: ?*Expr = null;
        if (self.isName("extends")) {
            try self.nextLexeme();

            super = try self.parseClassRefExpr();
        }

        var declarations: std.ArrayList(Stat.ClassMember) = .empty;

        // properties and methods share a namespace
        var class_member_namespace: std.StringHashMapUnmanaged(void) = .empty;

        var private_field_count: usize = 0;
        var private_method_count: usize = 0;

        while (self.curType() != .reserved_end and self.curType() != .eof) {
            var access: ?Stat.ClassAccess = null;
            if (self.isName("public")) {
                access = .public;
                try self.nextLexeme();
            } else if (self.isName("private")) {
                access = .private;
                try self.nextLexeme();
            }

            // If we saw a qualifier _and_ the current token is not `function`, assume this is a property.
            if (access != null and self.curType() != .reserved_function) {
                const prop_name = try self.parseName("class property name");

                var prop_type: ?*AType = null;
                if (self.curType() == T.char(':')) {
                    try self.nextLexeme();
                    prop_type = try self.parseType(false);
                }

                if (getClassFieldNameError(prop_name.name, access.?)) |name_error|
                    return self.report(prop_name.location, "{s}", .{name_error});
                if (std.mem.startsWith(u8, prop_name.name, "__"))
                    return self.report(prop_name.location, "Class properties cannot start with '__'", .{});
                if (class_member_namespace.contains(prop_name.name))
                    return self.report(prop_name.location, "Duplicate class member '{s}'", .{cstr(prop_name.name)});

                try class_member_namespace.put(self.arena, prop_name.name, {});

                if (access.? == .private) private_field_count += 1;

                try declarations.append(self.arena, .{ .property = .{ .name = prop_name.name, .name_location = prop_name.location, .type = prop_type, .access = access.? } });
            } else if (self.curType() == .reserved_function) {
                const method_access = access orelse .public;

                const match_function = self.cur();
                try self.nextLexeme();

                const method_name = try self.parseName("method name");
                // `print` inside a method refers to the global, not to the class method, so no local name is bound
                const old_in_class_method_body = self.in_class_method_body;
                self.in_class_method_body = true;

                const body = try self.parseFunctionBody(false, match_function, method_name.name, null, &.{}, false);

                self.in_class_method_body = old_in_class_method_body;

                const args = body.func.data.function.args;
                if (args.len > 0 and std.mem.eql(u8, args[0].name, "self")) {
                    if (args[0].annotation) |ann|
                        return self.report(ann.location, "The 'self' parameter cannot have a type annotation", .{});
                }

                if (getClassFieldNameError(method_name.name, method_access)) |name_error| {
                    return self.report(method_name.location, "{s}", .{name_error});
                } else if (std.mem.startsWith(u8, method_name.name, "__")) {
                    if (isNameIn(method_name.name, &kExplicitlyDisallowedMetamethods))
                        return self.report(method_name.location, "Classes cannot define '{s}' as a metamethod", .{cstr(method_name.name)})
                    else if (!isNameIn(method_name.name, &kAllowedMetamethods))
                        return self.report(method_name.location, "Cannot use '{s}' as a method name: names starting with '__' are reserved", .{cstr(method_name.name)});
                }

                if (class_member_namespace.contains(method_name.name))
                    return self.report(method_name.location, "Duplicate class member '{s}'", .{cstr(method_name.name)});

                try class_member_namespace.put(self.arena, method_name.name, {});

                if (method_access == .private) private_method_count += 1;

                try declarations.append(self.arena, .{ .method = .{ .name = method_name.name, .name_location = method_name.location, .function = body.func, .access = method_access } });
            } else {
                return self.report(self.cur().location, "Only class properties and functions can be declared within a class", .{});
            }
        }

        if (private_field_count > kMaxClassPrivateProperties)
            return self.report(name.location, "Exceeded the maximum of {d} private fields in a class", .{kMaxClassPrivateProperties});

        if (private_method_count > kMaxClassPrivateProperties)
            return self.report(name.location, "Exceeded the maximum of {d} private methods in a class", .{kMaxClassPrivateProperties});

        const end = self.cur().location;
        try self.expectAndConsume(.reserved_end, "class");

        // We only allow classes at the top level: we can make use of the recursion counter to check this
        if (self.recursion_counter > 1)
            return self.report(name_local.location, "Cannot declare class '{s}' inside another statement or expression", .{cstr(name_local.name)});

        if (self.classes_within_module.contains(name_local.name)) {
            // We do not allow shadowing classes with the same name
            try self.restoreLocals(saved_locals);
            return self.report(name_local.location, "A class named '{s}' has already been declared in this module", .{cstr(name_local.name)});
        }
        try self.classes_within_module.put(self.arena, name_local.name, {});

        return self.newStat(Location.join(start, end), .{ .class = .{ .name = name_local, .super = super, .members = declarations.items, .exported = exported, .open = open } });
    }

    // classrefexp -> NAME { `.' NAME | `[' exp `]' }
    fn parseClassRefExpr(self: *Parser) Error!*Expr {
        const start = self.cur().location.begin;

        const name = try self.parseNameExpr("class reference expression");

        if (self.curType() == T.char('.')) {
            const dot_position = self.cur().location.begin;
            try self.nextLexeme();
            const index = try self.parseIndexName("class reference expression", dot_position);

            return self.newExpr(Location.span(start, index.location.end), .{ .index_name = .{ .expr = name, .index = index.name, .index_location = index.location, .op_position = dot_position, .op = '.' } });
        } else if (self.curType() == T.char('[')) {
            try self.nextLexeme();
            const key = try self.parseExpr(0);
            try self.expectAndConsume(T.char(']'), "class reference expression");
            return self.newExpr(Location.span(start, self.lexer.previousLocation().end), .{ .index_expr = .{ .expr = name, .index = key } });
        }

        return name;
    }

    fn checkDuplicateExport(self: *Parser, name: ast.Name) Error!bool {
        const gop = try self.declared_export_bindings.getOrPut(self.arena, name);
        return !gop.found_existing;
    }

    fn exportLocalStat(self: *Parser, stat: *Stat) Error!*Stat {
        // parseLocal without 'function' always produces a local statement
        for (stat.data.local.vars) |local| {
            if (!try self.checkDuplicateExport(local.name))
                return self.report(local.location, "Duplicate exported identifier '{s}'", .{cstr(local.name)});

            local.is_exported = true;
        }
        return stat;
    }

    // type Name [`<' varlist `>'] `=' Type
    fn parseTypeAlias(self: *Parser, start: Location, exported: bool) Error!*Stat {
        // parsing a type function
        if (self.curType() == .reserved_function) return self.parseTypeFunction(start, exported);

        // note: `type` token is already parsed for us, so we just need to parse the rest
        const name = try self.parseName("type name");

        const generics = try self.parseGenericTypeList(true);

        try self.expectAndConsume(T.char('='), "type alias");

        const ty = try self.parseType(false);

        return self.newStat(Location.join(start, ty.location), .{ .type_alias = .{ .name = name.name, .generics = generics.types, .generic_packs = generics.packs, .type = ty, .exported = exported } });
    }

    fn parseTypeFunction(self: *Parser, start: Location, exported: bool) Error!*Stat {
        const match_fn = self.cur();
        try self.nextLexeme();

        // parse the name of the type function
        const fn_name = try self.parseName("type function name");

        const old_type_function_depth = self.type_function_depth;
        self.type_function_depth = self.function_stack.items.len;

        const body = try self.parseFunctionBody(false, match_fn, fn_name.name, null, &.{}, false);

        self.type_function_depth = old_type_function_depth;

        return self.newStat(Location.join(start, body.func.location), .{ .type_function = .{ .name = fn_name.name, .body = body.func, .exported = exported } });
    }

    fn reportLValueError(self: *Parser, expr: *Expr) Error {
        if (self.flags.LuauExportValueSyntax and expr.data == .local and expr.data.local.local.is_const)
            return self.report(expr.location, "Variable '{s}' is constant and may not be reassigned", .{cstr(expr.data.local.local.name)});

        return self.report(expr.location, "Assigned expression must be a variable or a field", .{});
    }

    // varlist `=' explist
    fn parseAssignment(self: *Parser, initial: *Expr) Error!*Stat {
        if (!isExprLValue(initial)) return self.reportLValueError(initial);

        var vars: std.ArrayList(*Expr) = .empty;
        try vars.append(self.arena, initial);

        while (self.curType() == T.char(',')) {
            try self.nextLexeme();

            const expr = try self.parsePrimaryExpr(true);

            if (!isExprLValue(expr)) return self.reportLValueError(expr);

            try vars.append(self.arena, expr);
        }

        try self.expectAndConsume(T.char('='), "assignment");

        var values: std.ArrayList(*Expr) = .empty;
        try self.parseExprList(&values);

        return self.newStat(Location.join(initial.location, values.items[values.items.len - 1].location), .{ .assign = .{ .vars = vars.items, .values = values.items } });
    }

    fn parseCompoundAssignment(self: *Parser, initial: *Expr, op: ast.BinaryOp) Error!*Stat {
        if (!isExprLValue(initial)) return self.reportLValueError(initial);

        try self.nextLexeme();

        const value = try self.parseExpr(0);

        return self.newStat(Location.join(initial.location, value.location), .{ .compound_assign = .{ .op = op, .variable = initial, .value = value } });
    }

    const FunctionBodyResult = struct { func: *Expr, local: ?*ast.Local };

    // funcbody ::= `(' [parlist] `)' [`:' ReturnType] block end
    // parlist ::= bindinglist [`,' `...'] | `...'
    fn parseFunctionBody(self: *Parser, hasself: bool, match_function: Lexeme, debugname: ?ast.Name, local_name: ?Name, attributes: []const *ast.Attr, is_const: bool) Error!FunctionBodyResult {
        var start = match_function.location;

        if (attributes.len > 0) start = attributes[0].location;

        const generics = try self.parseGenericTypeList(false);

        const match_paren = MatchLexeme.from(self.cur());
        try self.expectAndConsume(T.char('('), "function");

        var args: std.ArrayList(Binding) = .empty;

        var vararg = false;
        var vararg_location: Location = .{};
        var vararg_annotation: ?*TypePack = null;

        if (self.curType() != T.char(')')) {
            const r = try self.parseBindingList(&args, true, false);
            vararg = r.vararg;
            vararg_location = r.location;
            vararg_annotation = r.annotation;
        }

        try self.expectMatchAndConsume(')', match_paren);

        const typelist = try self.parseOptionalReturnType();

        var fun_local: ?*ast.Local = null;

        if (local_name) |ln| fun_local = try self.pushLocal(.{ .name = ln, .annotation = null, .is_const = is_const });

        const locals_begin = self.saveLocals();

        try self.function_stack.append(self.arena, .{ .vararg = vararg });

        var self_local: ?*ast.Local = null;
        if (hasself) self_local = try self.pushLocal(.{ .name = .{ .name = self.name_self, .location = start }, .annotation = null });

        var vars: std.ArrayList(*ast.Local) = .empty;
        for (args.items) |a| try vars.append(self.arena, try self.pushLocal(a));

        const body = try self.parseBlock();

        _ = self.function_stack.pop();

        try self.restoreLocals(locals_begin);

        const end = self.cur().location;

        try self.expectMatchEndAndConsume(.reserved_end, MatchLexeme.from(match_function));

        const node = try self.newExpr(Location.join(start, end), .{ .function = .{
            .attributes = attributes,
            .generics = generics.types,
            .generic_packs = generics.packs,
            .self = self_local,
            .args = vars.items,
            .return_annotation = typelist,
            .vararg = vararg,
            .vararg_location = vararg_location,
            .vararg_annotation = vararg_annotation,
            .body = body,
            .function_depth = @intCast(self.function_stack.items.len),
            .debugname = debugname,
        } });

        return .{ .func = node, .local = fun_local };
    }

    // explist ::= {exp `,'} exp
    fn parseExprList(self: *Parser, result: *std.ArrayList(*Expr)) Error!void {
        try result.append(self.arena, try self.parseExpr(0));

        while (self.curType() == T.char(',')) {
            try self.nextLexeme();

            if (self.curType() == T.char(')'))
                return self.report(self.cur().location, "Expected expression after ',' but got ')' instead", .{});

            try result.append(self.arena, try self.parseExpr(0));
        }
    }

    fn parseBinding(self: *Parser, is_const: bool) Error!Binding {
        const name = try self.parseName("variable name");
        const annotation = try self.parseOptionalType();
        return .{ .name = name, .annotation = annotation, .is_const = is_const };
    }

    const BindingListResult = struct { vararg: bool, location: Location, annotation: ?*TypePack };

    // bindinglist ::= (binding | `...') [`,' bindinglist]
    fn parseBindingList(self: *Parser, result: *std.ArrayList(Binding), allow_dot3: bool, is_const: bool) Error!BindingListResult {
        while (true) {
            if (self.curType() == .dot3 and allow_dot3) {
                const vararg_location = self.cur().location;
                try self.nextLexeme();

                var tail_annotation: ?*TypePack = null;
                if (self.curType() == T.char(':')) {
                    try self.nextLexeme();
                    tail_annotation = try self.parseVariadicArgumentTypePack();
                }

                return .{ .vararg = true, .location = vararg_location, .annotation = tail_annotation };
            }

            try result.append(self.arena, try self.parseBinding(is_const));

            if (self.curType() != T.char(',')) break;
            try self.nextLexeme();
        }

        return .{ .vararg = false, .location = .{}, .annotation = null };
    }

    // ---------------------------------------------------------------- types

    fn parseOptionalType(self: *Parser) Error!?*AType {
        if (self.curType() == T.char(':')) {
            try self.nextLexeme();
            return try self.parseType(false);
        }
        return null;
    }

    fn shouldParseTypePack(self: *Parser) Error!bool {
        if (self.curType() == .dot3) return true;
        if (self.curType() == .name and (try self.lexer.lookahead()).type == .dot3) return true;
        return false;
    }

    // TypeList ::= Type [`,' TypeList] | ...Type
    fn parseTypeList(self: *Parser, result: *std.ArrayList(*AType), result_names: *std.ArrayList(?ast.ArgumentName)) Error!?*TypePack {
        while (true) {
            if (try self.shouldParseTypePack()) return try self.parseTypePack();

            if (self.curType() == .name and (try self.lexer.lookahead()).type == T.char(':')) {
                // Fill in previous argument names with empty slots
                while (result_names.items.len < result.items.len) try result_names.append(self.arena, null);

                try result_names.append(self.arena, .{ .name = self.cur().data.?, .location = self.cur().location });
                try self.nextLexeme();

                try self.expectAndConsume(T.char(':'), null);
            } else if (result_names.items.len != 0) {
                // If we have a type with named arguments, provide elements for all types
                try result_names.append(self.arena, null);
            }

            try result.append(self.arena, try self.parseType(false));
            if (self.curType() != T.char(',')) break;

            try self.nextLexeme();

            if (self.curType() == T.char(')'))
                return self.report(self.cur().location, "Expected type after ',' but got ')' instead", .{});
        }

        return null;
    }

    fn parseOptionalReturnType(self: *Parser) Error!?*TypePack {
        if (self.curType() == T.char(':') or self.curType() == .skinny_arrow) {
            if (self.curType() == .skinny_arrow)
                return self.report(self.cur().location, "Function return type annotations are written after ':' instead of '->'", .{});

            try self.nextLexeme();

            const old_recursion_count = self.recursion_counter;

            const result = try self.parseReturnType();

            // At this point, if we find a , character, it indicates that there are multiple return types
            // in this type annotation, but the list wasn't wrapped in parentheses.
            if (self.curType() == T.char(','))
                return self.report(self.cur().location, "Expected a statement, got ','; did you forget to wrap the list of return types in parentheses?", .{});

            self.recursion_counter = old_recursion_count;

            return result;
        }

        return null;
    }

    fn isTypeFollow(c: T) bool {
        return c == T.char('|') or c == T.char('?') or c == T.char('&');
    }

    // ReturnType ::= Type | `(' TypeList `)'
    fn parseReturnType(self: *Parser) Error!*TypePack {
        try self.incrementRecursionCounter("type annotation");

        const begin = self.cur();

        if (self.curType() != T.char('(')) {
            if (try self.shouldParseTypePack()) return self.parseTypePack();

            const ty = try self.parseType(false);
            const types = try self.copy(*AType, &.{ty});
            return self.newPack(ty.location, .{ .explicit = .{ .types = types, .tail_type = null } });
        }

        try self.nextLexeme(); // Consume '('

        var result: std.ArrayList(*AType) = .empty;
        var result_names: std.ArrayList(?ast.ArgumentName) = .empty;
        var vararg_annotation: ?*TypePack = null;

        // possibly () -> ReturnType
        if (self.curType() != T.char(')')) vararg_annotation = try self.parseTypeList(&result, &result_names);

        const location = Location.join(begin.location, self.cur().location);
        try self.expectMatchAndConsume(')', MatchLexeme.from(begin));

        if (self.curType() != .skinny_arrow and result_names.items.len == 0) {
            // If it turns out that it's just '(A)', it's possible that there are unions/intersections to follow, so fold over it.
            if (result.items.len == 1) {
                var inner: *AType = result.items[0];
                if (self.flags.LuauSingleTypeOptionalPackReturnsAttributeParens) {
                    if (vararg_annotation == null and isTypeFollow(self.curType())) inner = try self.newType(location, .{ .group = result.items[0] });
                } else {
                    if (vararg_annotation == null) inner = try self.newType(location, .{ .group = result.items[0] });
                }

                const return_type = try self.parseTypeSuffix(inner, begin.location);

                // If parseType parses nothing, then returnType->location.end only points at the last non-type-pack
                // type to successfully parse.  We need the span of the whole annotation.
                const end_pos = location.end;

                const types = try self.copy(*AType, &.{return_type});
                return self.newPack(Location.span(location.begin, end_pos), .{ .explicit = .{ .types = types, .tail_type = vararg_annotation } });
            }

            return self.newPack(location, .{ .explicit = .{ .types = result.items, .tail_type = vararg_annotation } });
        }

        const tail = try self.parseFunctionTypeTail(begin, &.{}, .{}, result.items, result_names.items, vararg_annotation);

        const types = try self.copy(*AType, &.{tail});
        return self.newPack(Location.join(location, tail.location), .{ .explicit = .{ .types = types, .tail_type = null } });
    }

    // TableIndexer ::= `[' Type `]' `:' Type
    fn parseTableIndexer(self: *Parser, access: ast.TableAccess, begin: Lexeme) Error!*ast.TableIndexer {
        const index = try self.parseType(false);

        try self.expectMatchAndConsume(']', MatchLexeme.from(begin));

        try self.expectAndConsume(T.char(':'), "table field");

        const result = try self.parseType(false);

        const node = try self.arena.create(ast.TableIndexer);
        node.* = .{ .index_type = index, .result_type = result, .location = Location.join(begin.location, result.location), .access = access };
        return node;
    }

    // TableProp ::= Name `:' Type
    // TablePropOrIndexer ::= TableProp | TableIndexer
    // PropList ::= TablePropOrIndexer {fieldsep TablePropOrIndexer} [fieldsep]
    // TableType ::= `{' PropList `}'
    fn parseTableType(self: *Parser, in_declaration_context: bool) Error!*AType {
        try self.incrementRecursionCounter("type annotation");

        var props: std.ArrayList(ast.TableProp) = .empty;
        var indexer: ?*ast.TableIndexer = null;

        const start = self.cur().location;

        const match_brace = MatchLexeme.from(self.cur());
        try self.expectAndConsume(T.char('{'), "table type");

        while (self.curType() != T.char('}')) {
            var access: ast.TableAccess = .read_write;

            if (self.flags.DebugLuauParseExactTables and self.curType() == .dot3) {
                try self.nextLexeme();
                // ... is always last.
                break;
            }

            if (self.curType() == .name and (try self.lexer.lookahead()).type != T.char(':')) {
                if (std.mem.eql(u8, self.cur().data.?, "read")) {
                    access = .read;
                    _ = try self.lexer.next();
                } else if (std.mem.eql(u8, self.cur().data.?, "write")) {
                    access = .write;
                    _ = try self.lexer.next();
                }
            }

            if (self.curType() == T.char('[')) {
                const begin = self.cur();
                try self.nextLexeme(); // [

                if ((self.curType() == .raw_string or self.curType() == .quoted_string) and (try self.lexer.lookahead()).type == T.char(']')) {
                    const chars = try self.parseCharArray();

                    try self.expectMatchAndConsume(']', MatchLexeme.from(begin));
                    try self.expectAndConsume(T.char(':'), "table field");

                    const ty = try self.parseType(false);

                    // since AstName contains a char*, it can't contain null
                    const contains_null = if (chars) |c| std.mem.indexOfScalar(u8, c, 0) != null else false;

                    if (chars != null and !contains_null) {
                        try props.append(self.arena, .{ .name = try self.names.getOrAdd(chars.?), .location = begin.location, .type = ty, .access = access });
                    } else {
                        return self.report(begin.location, "String literal contains malformed escape sequence or \\0", .{});
                    }
                } else {
                    if (indexer != null) {
                        // maybe we don't need to parse the entire badIndexer...
                        const bad_indexer = try self.parseTableIndexer(access, begin);
                        return self.report(bad_indexer.location, "Cannot have more than one table indexer", .{});
                    } else {
                        indexer = try self.parseTableIndexer(access, begin);
                    }
                }
            } else if (props.items.len == 0 and indexer == null and !(self.curType() == .name and (try self.lexer.lookahead()).type == T.char(':'))) {
                const ty = try self.parseType(false);

                // array-like table type: {T} desugars into {[number]: T}
                const null_type_location = Location.withLength(start.begin, 0);
                const index = try self.newType(null_type_location, .{ .reference = .{ .prefix = null, .name = self.name_number, .has_parameters = false, .parameters = &.{} } });
                const idx = try self.arena.create(ast.TableIndexer);
                idx.* = .{ .index_type = index, .result_type = ty, .location = ty.location, .access = access };
                indexer = idx;

                if (self.flags.DebugLuauParseExactTables) {
                    if (self.curType() == T.char(',') and (try self.lexer.lookahead()).type == .dot3) {
                        try self.nextLexeme();
                        try self.nextLexeme();
                    }
                }

                break;
            } else {
                const name = try self.parseName("table field");

                try self.expectAndConsume(T.char(':'), "table field");

                const ty = try self.parseType(in_declaration_context);

                try props.append(self.arena, .{ .name = name.name, .location = name.location, .type = ty, .access = access });
            }

            if (self.curType() == T.char(',') or self.curType() == T.char(';')) {
                try self.nextLexeme();
            } else {
                if (self.curType() != T.char('}')) break;
            }
        }

        const end = self.cur().location;

        try self.expectMatchAndConsume('}', match_brace);

        return self.newType(Location.join(start, end), .{ .table = .{ .props = props.items, .indexer = indexer } });
    }

    const Generics = struct { types: []const *ast.GenericType = &.{}, packs: []const *ast.GenericTypePack = &.{} };

    const TypeOrPackResult = struct { type: ?*AType = null, pack: ?*TypePack = null };

    // ReturnType ::= Type | `(' TypeList `)'
    // FunctionType ::= [`<' varlist `>'] `(' [TypeList] `)' `->` ReturnType
    fn parseFunctionType(self: *Parser, allow_pack: bool, attributes: []const *ast.Attr) Error!TypeOrPackResult {
        try self.incrementRecursionCounter("type annotation");

        var force_function_type = self.curType() == T.char('<');

        const begin = self.cur();

        const generics = try self.parseGenericTypeList(false);

        const parameter_start = self.cur();

        try self.expectAndConsume(T.char('('), "function parameters");

        var params: std.ArrayList(*AType) = .empty;
        var names: std.ArrayList(?ast.ArgumentName) = .empty;
        var vararg_annotation: ?*TypePack = null;

        if (self.curType() != T.char(')')) vararg_annotation = try self.parseTypeList(&params, &names);

        const close_args_location = self.cur().location;
        try self.expectMatchAndConsume(')', MatchLexeme.from(parameter_start));

        if (names.items.len != 0) force_function_type = true;

        const return_type_introducer = self.curType() == .skinny_arrow or self.curType() == T.char(':');

        // Not a function at all. Just a parenthesized type. Or maybe a type pack with a single element
        if (params.items.len == 1 and vararg_annotation == null and !force_function_type and !return_type_introducer) {
            if (allow_pack) {
                return .{ .pack = try self.newPack(begin.location, .{ .explicit = .{ .types = params.items, .tail_type = null } }) };
            } else {
                return .{ .type = try self.newType(Location.join(parameter_start.location, close_args_location), .{ .group = params.items[0] }) };
            }
        }

        if (!force_function_type and !return_type_introducer and allow_pack) {
            return .{ .pack = try self.newPack(begin.location, .{ .explicit = .{ .types = params.items, .tail_type = vararg_annotation } }) };
        }

        return .{ .type = try self.parseFunctionTypeTail(begin, attributes, generics, params.items, names.items, vararg_annotation) };
    }

    fn parseFunctionTypeTail(self: *Parser, begin: Lexeme, attributes: []const *ast.Attr, generics: Generics, params: []const *AType, param_names: []const ?ast.ArgumentName, vararg_annotation: ?*TypePack) Error!*AType {
        try self.incrementRecursionCounter("type annotation");

        if (self.curType() == T.char(':')) {
            return self.report(self.cur().location, "Return types in function type annotations are written after '->' instead of ':'", .{});
        } else if (self.curType() != .skinny_arrow and generics.types.len == 0 and generics.packs.len == 0 and params.len == 0) {
            // Users occasionally write '()' as the 'unit' type when they actually want to use 'nil', here we'll try to give a more specific error
            return self.report(Location.join(begin.location, self.lexer.previousLocation()), "Expected '->' after '()' when parsing function type; did you mean 'nil'?", .{});
        } else {
            try self.expectAndConsume(.skinny_arrow, "function type");
        }

        const return_type = try self.parseReturnType();

        return self.newType(Location.join(begin.location, return_type.location), .{ .function = .{
            .attributes = attributes,
            .generics = generics.types,
            .generic_packs = generics.packs,
            .arg_types = .{ .types = params, .tail_type = vararg_annotation },
            .arg_names = param_names,
            .return_types = return_type,
        } });
    }

    // Type ::=
    //      nil |
    //      Name[`.' Name] [`<' namelist `>'] |
    //      `{' [PropList] `}' |
    //      `(' [TypeList] `)' `->` ReturnType
    //      `typeof` Type
    fn parseTypeSuffix(self: *Parser, ty: ?*AType, begin: Location) Error!*AType {
        var parts: std.ArrayList(*AType) = .empty;

        if (ty) |t| try parts.append(self.arena, t);

        try self.incrementRecursionCounter("type annotation");

        var is_union = false;
        var is_intersection = false;
        var optional_count: u32 = 0;

        var location = begin;

        while (true) {
            const c = self.curType();
            if (c == T.char('|')) {
                try self.nextLexeme();

                const old_recursion_count = self.recursion_counter;
                const part = (try self.parseSimpleType(false, false)).type.?;
                try parts.append(self.arena, part);
                self.recursion_counter = old_recursion_count;

                is_union = true;
            } else if (c == T.char('?')) {
                const loc = self.cur().location;
                try self.nextLexeme();

                try parts.append(self.arena, try self.newType(loc, .optional));
                optional_count += 1;

                is_union = true;
            } else if (c == T.char('&')) {
                try self.nextLexeme();

                const old_recursion_count = self.recursion_counter;
                const part = (try self.parseSimpleType(false, false)).type.?;
                try parts.append(self.arena, part);
                self.recursion_counter = old_recursion_count;

                is_intersection = true;
            } else if (c == .dot3) {
                return self.report(self.cur().location, "Unexpected '...' after type annotation", .{});
            } else {
                break;
            }

            if (parts.items.len > kTypeLengthLimit + optional_count)
                return self.report(parts.items[parts.items.len - 1].location, "Exceeded allowed type length; simplify your type annotation to make the code compile", .{});
        }

        if (parts.items.len == 1 and !is_union and !is_intersection) return parts.items[0];
        if (is_union and is_intersection) {
            return self.report(Location.join(begin, parts.items[parts.items.len - 1].location), "Mixing union and intersection types is not allowed; consider wrapping in parentheses.", .{});
        }

        location.end = parts.items[parts.items.len - 1].location.end;

        if (is_union) return self.newType(location, .{ .@"union" = parts.items });
        if (is_intersection) return self.newType(location, .{ .intersection = parts.items });

        return self.report(begin, "Composite type was not an intersection or union.", .{});
    }

    fn parseSimpleTypeOrPack(self: *Parser) Error!TypeOrPackResult {
        const old_recursion_count = self.recursion_counter;
        // recursion counter is incremented in parseSimpleType

        const begin = self.cur().location;

        const r = try self.parseSimpleType(true, false);

        if (r.pack) |p| return .{ .pack = p };

        self.recursion_counter = old_recursion_count;

        return .{ .type = try self.parseTypeSuffix(r.type, begin) };
    }

    fn parseType(self: *Parser, in_declaration_context: bool) Error!*AType {
        const old_recursion_count = self.recursion_counter;
        // recursion counter is incremented in parseSimpleType and/or parseTypeSuffix

        const begin = self.cur().location;

        var ty: ?*AType = null;

        const c = self.curType();
        if (c != T.char('|') and c != T.char('&')) {
            ty = (try self.parseSimpleType(false, in_declaration_context)).type;
            self.recursion_counter = old_recursion_count;
        }

        const type_with_suffix = try self.parseTypeSuffix(ty, begin);
        self.recursion_counter = old_recursion_count;

        return type_with_suffix;
    }

    // Type ::= nil | Name[`.' Name] [ `<' Type [`,' ...] `>' ] | `typeof' `(' expr `)' | `{' [PropList] `}'
    //   | [`<' varlist `>'] `(' [TypeList] `)' `->` ReturnType
    fn parseSimpleType(self: *Parser, allow_pack: bool, in_declaration_context: bool) Error!TypeOrPackResult {
        try self.incrementRecursionCounter("type annotation");

        const start = self.cur().location;

        switch (self.curType()) {
            .attribute, .attribute_open => {
                if (!in_declaration_context) return self.report(start, "attributes are not allowed in declaration context", .{});
                const attributes = try self.parseAttributes();
                return self.parseFunctionType(allow_pack, attributes);
            },
            .reserved_nil => {
                try self.nextLexeme();
                return .{ .type = try self.newType(start, .{ .reference = .{ .prefix = null, .name = self.name_nil, .has_parameters = false, .parameters = &.{} } }) };
            },
            .reserved_true => {
                try self.nextLexeme();
                return .{ .type = try self.newType(start, .{ .singleton_bool = true }) };
            },
            .reserved_false => {
                try self.nextLexeme();
                return .{ .type = try self.newType(start, .{ .singleton_bool = false }) };
            },
            .raw_string, .quoted_string => {
                if (try self.parseCharArray()) |value| {
                    return .{ .type = try self.newType(start, .{ .singleton_string = value }) };
                }
                return self.report(start, "String literal contains malformed escape sequence", .{});
            },
            .interp_string_begin, .interp_string_simple => {
                _ = try self.parseInterpString();
                return self.report(start, "Interpolated string literals cannot be used as types", .{});
            },
            .broken_string => {
                try self.nextLexeme();
                return self.report(start, "Malformed string; did you forget to finish it?", .{});
            },
            .name => {
                var prefix: ?ast.Name = null;
                var name = try self.parseName("type name");

                if (self.curType() == T.char('.')) {
                    const prefix_point_position = self.cur().location.begin;
                    try self.nextLexeme();

                    prefix = name.name;

                    name = try self.parseIndexName("field name", prefix_point_position);
                } else if (self.curType() == .dot3) {
                    return self.report(self.cur().location, "Unexpected '...' after type name; type pack is not allowed in this context", .{});
                } else if (std.mem.eql(u8, name.name, "typeof")) {
                    const typeof_begin = self.cur();
                    try self.expectAndConsume(T.char('('), "typeof type");

                    const expr = try self.parseExpr(0);

                    const end = self.cur().location;

                    try self.expectMatchAndConsume(')', MatchLexeme.from(typeof_begin));

                    return .{ .type = try self.newType(Location.join(start, end), .{ .typeof = expr }) };
                }

                var has_parameters = false;
                var parameters: []const ast.TypeOrPack = &.{};

                if (self.curType() == T.char('<')) {
                    has_parameters = true;
                    parameters = try self.parseTypeParams();
                }

                const end = self.lexer.previousLocation();

                return .{ .type = try self.newType(Location.join(start, end), .{ .reference = .{ .prefix = prefix, .name = name.name, .has_parameters = has_parameters, .parameters = parameters } }) };
            },
            else => {
                const c = self.curType();
                if (c == T.char('{')) {
                    return .{ .type = try self.parseTableType(in_declaration_context) };
                } else if (c == T.char('(') or c == T.char('<')) {
                    return self.parseFunctionType(allow_pack, &.{});
                } else if (c == .reserved_function) {
                    try self.nextLexeme();
                    return self.report(start, "Using 'function' as a type annotation is not supported, consider replacing with a function type annotation e.g. '(...any) -> ...any'", .{});
                }

                // The parse error includes the next lexeme to make it easier to display where the error is (e.g. in an IDE or a CLI error message).
                // Including the current lexeme also makes the parse error consistent with other parse errors returned by Luau.
                const parse_error_location = Location.span(self.lexer.previousLocation().end, start.end);
                return self.report(parse_error_location, "Expected type, got {s}", .{try self.tokStr(self.cur())});
            },
        }
    }

    fn parseVariadicArgumentTypePack(self: *Parser) Error!*TypePack {
        // Generic: a...
        if (self.curType() == .name and (try self.lexer.lookahead()).type == .dot3) {
            const name = try self.parseName("generic name");
            const end = self.cur().location;

            // This will not fail because of the lookahead guard.
            try self.expectAndConsume(.dot3, "generic type pack annotation");
            return self.newPack(Location.join(name.location, end), .{ .generic = name.name });
        }
        // Variadic: T
        const variadic_annotation = try self.parseType(false);
        return self.newPack(variadic_annotation.location, .{ .variadic = variadic_annotation });
    }

    fn parseTypePack(self: *Parser) Error!*TypePack {
        // Variadic: ...T
        if (self.curType() == .dot3) {
            const start = self.cur().location;
            try self.nextLexeme();
            const vararg_ty = try self.parseType(false);
            return self.newPack(Location.join(start, vararg_ty.location), .{ .variadic = vararg_ty });
        }
        // Generic: a...
        const name = try self.parseName("generic name");
        const end = self.cur().location;

        // This will not fail because of the lookahead guard.
        try self.expectAndConsume(.dot3, "generic type pack annotation");
        return self.newPack(Location.join(name.location, end), .{ .generic = name.name });
    }

    fn parseGenericTypeList(self: *Parser, with_default_values: bool) Error!Generics {
        var names: std.ArrayList(*ast.GenericType) = .empty;
        var name_packs: std.ArrayList(*ast.GenericTypePack) = .empty;

        if (self.curType() == T.char('<')) {
            const begin = self.cur();
            try self.nextLexeme();

            var seen_pack = false;
            var seen_default = false;

            while (true) {
                const name_location = self.cur().location;
                const name = (try self.parseName(null)).name;
                if (self.curType() == .dot3 or seen_pack) {
                    seen_pack = true;

                    if (self.curType() != .dot3)
                        return self.report(self.cur().location, "Generic types come before generic type packs", .{});
                    try self.nextLexeme();

                    if (with_default_values and self.curType() == T.char('=')) {
                        seen_default = true;
                        try self.nextLexeme();

                        if (try self.shouldParseTypePack()) {
                            const type_pack = try self.parseTypePack();
                            const node = try self.arena.create(ast.GenericTypePack);
                            node.* = .{ .location = name_location, .name = name, .default_value = type_pack };
                            try name_packs.append(self.arena, node);
                        } else {
                            const r = try self.parseSimpleTypeOrPack();

                            if (r.type) |t| return self.report(t.location, "Expected type pack after '=', got type", .{});

                            const node = try self.arena.create(ast.GenericTypePack);
                            node.* = .{ .location = name_location, .name = name, .default_value = r.pack };
                            try name_packs.append(self.arena, node);
                        }
                    } else {
                        if (seen_default) return self.report(self.cur().location, "Expected default type pack after type pack name", .{});

                        const node = try self.arena.create(ast.GenericTypePack);
                        node.* = .{ .location = name_location, .name = name, .default_value = null };
                        try name_packs.append(self.arena, node);
                    }
                } else {
                    if (with_default_values and self.curType() == T.char('=')) {
                        seen_default = true;
                        try self.nextLexeme();

                        const default_type = try self.parseType(false);

                        const node = try self.arena.create(ast.GenericType);
                        node.* = .{ .location = name_location, .name = name, .default_value = default_type };
                        try names.append(self.arena, node);
                    } else {
                        if (seen_default) return self.report(self.cur().location, "Expected default type after type name", .{});

                        const node = try self.arena.create(ast.GenericType);
                        node.* = .{ .location = name_location, .name = name, .default_value = null };
                        try names.append(self.arena, node);
                    }
                }

                if (self.curType() == T.char(',')) {
                    try self.nextLexeme();

                    if (self.curType() == T.char('>'))
                        return self.report(self.cur().location, "Expected type after ',' but got '>' instead", .{});
                } else break;
            }

            try self.expectMatchAndConsume('>', MatchLexeme.from(begin));
        }

        return .{ .types = names.items, .packs = name_packs.items };
    }

    fn parseTypeParams(self: *Parser) Error![]ast.TypeOrPack {
        var parameters: std.ArrayList(ast.TypeOrPack) = .empty;

        if (self.curType() == T.char('<')) {
            const begin = self.cur();
            try self.nextLexeme();

            while (true) {
                if (try self.shouldParseTypePack()) {
                    const type_pack = try self.parseTypePack();
                    try parameters.append(self.arena, .{ .type_pack = type_pack });
                } else if (self.curType() == T.char('(')) {
                    const begin_loc = self.cur().location;
                    const r = try self.parseSimpleType(true, false);

                    if (r.pack) |type_pack| {
                        const is_group = switch (type_pack.data) {
                            .explicit => |e| e.tail_type == null and e.types.len == 1 and isTypeFollow(self.curType()),
                            else => false,
                        };
                        if (is_group) {
                            // If we parsed an explicit type pack with a single type in it (something of the form `(T)`), and
                            // the next lexeme is one that follows a type (&, |, ?), then assume that this was actually a
                            // parenthesized type.
                            const parenthesized_type = type_pack.data.explicit.types[0];
                            const type_group = try self.newType(parenthesized_type.location, .{ .group = parenthesized_type });
                            try parameters.append(self.arena, .{ .type = try self.parseTypeSuffix(type_group, begin_loc) });
                        } else {
                            // Otherwise, it's a type pack.
                            try parameters.append(self.arena, .{ .type_pack = type_pack });
                        }
                    } else {
                        try parameters.append(self.arena, .{ .type = try self.parseTypeSuffix(r.type, begin_loc) });
                    }
                } else if (self.curType() == T.char('>') and parameters.items.len == 0) {
                    break;
                } else {
                    try parameters.append(self.arena, .{ .type = try self.parseType(false) });
                }

                if (self.curType() == T.char(',')) {
                    try self.nextLexeme();
                } else break;
            }

            try self.expectMatchAndConsume('>', MatchLexeme.from(begin));
        }

        return parameters.items;
    }

    // ---------------------------------------------------------------- expressions

    fn parseUnaryOp(l: Lexeme) ?ast.UnaryOp {
        if (l.type == .reserved_not) return .not;
        if (l.type == T.char('-')) return .minus;
        if (l.type == T.char('#')) return .len;
        return null;
    }

    fn parseBinaryOp(l: Lexeme) ?ast.BinaryOp {
        return switch (l.type) {
            .floor_div => .floor_div,
            .dot2 => .concat,
            .not_equal => .compare_ne,
            .equal => .compare_eq,
            .less_equal => .compare_le,
            .greater_equal => .compare_ge,
            .reserved_and => .@"and",
            .reserved_or => .@"or",
            else => {
                const v = @intFromEnum(l.type);
                return switch (v) {
                    '+' => .add,
                    '-' => .sub,
                    '*' => .mul,
                    '/' => .div,
                    '%' => .mod,
                    '^' => .pow,
                    '<' => .compare_lt,
                    '>' => .compare_gt,
                    else => null,
                };
            },
        };
    }

    fn parseCompoundOp(l: Lexeme) ?ast.BinaryOp {
        return switch (l.type) {
            .add_assign => .add,
            .sub_assign => .sub,
            .mul_assign => .mul,
            .div_assign => .div,
            .floor_div_assign => .floor_div,
            .mod_assign => .mod,
            .pow_assign => .pow,
            .concat_assign => .concat,
            else => null,
        };
    }

    const BinaryOpPriority = struct { left: u8, right: u8 };

    const binary_priority = [_]BinaryOpPriority{
        .{ .left = 6, .right = 6 }, // '+'
        .{ .left = 6, .right = 6 }, // '-'
        .{ .left = 7, .right = 7 }, // '*'
        .{ .left = 7, .right = 7 }, // '/'
        .{ .left = 7, .right = 7 }, // '//'
        .{ .left = 7, .right = 7 }, // `%'
        .{ .left = 10, .right = 9 }, // power (right associative)
        .{ .left = 5, .right = 4 }, // concat (right associative)
        .{ .left = 3, .right = 3 }, // inequality
        .{ .left = 3, .right = 3 }, // equality
        .{ .left = 3, .right = 3 }, // '<'
        .{ .left = 3, .right = 3 }, // '<='
        .{ .left = 3, .right = 3 }, // '>'
        .{ .left = 3, .right = 3 }, // '>='
        .{ .left = 2, .right = 2 }, // logical and
        .{ .left = 1, .right = 1 }, // logical or
    };

    fn checkUnaryConfusables(self: *Parser) Error!?ast.UnaryOp {
        if (self.curType() != T.char('!')) return null;
        return self.report(self.cur().location, "Unexpected '!'; did you mean 'not'?", .{});
    }

    fn checkBinaryConfusables(self: *Parser, limit: u32) Error!?ast.BinaryOp {
        const curr = self.cur();

        // early-out: need to check if this is a possible confusable quickly
        if (curr.type != T.char('&') and curr.type != T.char('|') and curr.type != T.char('!')) return null;

        // slow path: possible confusable
        const start = curr.location;
        const nxt = try self.lexer.lookahead();

        if (curr.type == T.char('&') and nxt.type == T.char('&') and curr.location.end.eql(nxt.location.begin) and binary_priority[@intFromEnum(ast.BinaryOp.@"and")].left > limit) {
            return self.report(Location.join(start, nxt.location), "Unexpected '&&'; did you mean 'and'?", .{});
        } else if (curr.type == T.char('|') and nxt.type == T.char('|') and curr.location.end.eql(nxt.location.begin) and binary_priority[@intFromEnum(ast.BinaryOp.@"or")].left > limit) {
            return self.report(Location.join(start, nxt.location), "Unexpected '||'; did you mean 'or'?", .{});
        } else if (curr.type == T.char('!') and nxt.type == T.char('=') and curr.location.end.eql(nxt.location.begin) and binary_priority[@intFromEnum(ast.BinaryOp.compare_ne)].left > limit) {
            return self.report(Location.join(start, nxt.location), "Unexpected '!='; did you mean '~='?", .{});
        }

        return null;
    }

    // subexpr -> (asexp | unop subexpr) { binop subexpr }
    // where `binop' is any binary operator with a priority higher than `limit'
    pub fn parseExpr(self: *Parser, limit: u32) Error!*Expr {
        const old_recursion_count = self.recursion_counter;

        // this handles recursive calls to parseSubExpr/parseExpr
        try self.incrementRecursionCounter("expression");

        const unary_priority: u32 = 8;

        const start = self.cur().location;

        var expr: *Expr = undefined;

        var uop = parseUnaryOp(self.cur());

        if (uop == null) uop = try self.checkUnaryConfusables();

        if (uop) |op| {
            try self.nextLexeme();

            const subexpr = try self.parseExpr(unary_priority);

            expr = try self.newExpr(Location.join(start, subexpr.location), .{ .unary = .{ .op = op, .expr = subexpr } });
        } else {
            expr = try self.parseAssertionExpr();
        }

        // expand while operators have priorities higher than `limit'
        var op = parseBinaryOp(self.cur());

        if (op == null) op = try self.checkBinaryConfusables(limit);

        while (op != null and binary_priority[@intFromEnum(op.?)].left > limit) {
            try self.nextLexeme();

            // read sub-expression with higher priority
            const nxt = try self.parseExpr(binary_priority[@intFromEnum(op.?)].right);

            expr = try self.newExpr(Location.join(start, nxt.location), .{ .binary = .{ .op = op.?, .left = expr, .right = nxt } });
            op = parseBinaryOp(self.cur());

            if (op == null) op = try self.checkBinaryConfusables(limit);

            // note: while the parser isn't recursive here, we're generating recursive structures of unbounded depth
            try self.incrementRecursionCounter("expression");
        }

        self.recursion_counter = old_recursion_count;

        return expr;
    }

    // NAME
    fn parseNameExpr(self: *Parser, context: ?[]const u8) Error!*Expr {
        const name = try self.parseName(context);

        if (self.local_map.get(name.name)) |value| {
            if (value) |local| {
                if (local.function_depth < self.type_function_depth)
                    return self.report(self.cur().location, "Type function cannot reference outer local '{s}'", .{cstr(local.name)});

                return self.newExpr(name.location, .{ .local = .{ .local = local, .upvalue = local.function_depth != self.function_stack.items.len - 1 } });
            }
        }

        return self.newExpr(name.location, .{ .global = name.name });
    }

    // prefixexp -> NAME | '(' expr ')'
    fn parsePrefixExpr(self: *Parser) Error!*Expr {
        if (self.curType() == T.char('(')) {
            const start = self.cur().location.begin;

            const match_paren = MatchLexeme.from(self.cur());
            try self.nextLexeme();

            const expr = try self.parseExpr(0);

            const end = self.cur().location.end;

            if (self.curType() != T.char(')')) {
                const suggestion: ?[]const u8 = if (self.curType() == T.char('=')) "; did you mean to use '{' when defining a table?" else null;
                return self.expectMatchAndConsumeFail(T.char(')'), match_paren, suggestion);
            }

            try self.nextLexeme();

            return self.newExpr(Location.span(start, end), .{ .group = expr });
        }
        return self.parseNameExpr("expression");
    }

    // primaryexp -> prefixexp { `.' NAME | `[' exp `]' | `:' NAME funcargs | funcargs }
    fn parsePrimaryExpr(self: *Parser, as_statement: bool) Error!*Expr {
        const start = self.cur().location.begin;

        var expr = try self.parsePrefixExpr();

        const old_recursion_count = self.recursion_counter;

        while (true) {
            const c = self.curType();
            if (c == T.char('.')) {
                const op_position = self.cur().location.begin;
                try self.nextLexeme();

                const index = try self.parseIndexName(null, op_position);

                expr = try self.newExpr(Location.span(start, index.location.end), .{ .index_name = .{ .expr = expr, .index = index.name, .index_location = index.location, .op_position = op_position, .op = '.', .private_access = self.in_class_method_body and isPrivateFieldName(index.name) } });
            } else if (c == T.char('[')) {
                expr = try self.parseIndexExpr(start, expr);
            } else if (c == T.char(':')) {
                expr = try self.parseMethodCall(start, expr);
            } else if (c == T.char('(')) {
                // This error is handled inside 'parseFunctionArgs' as well, but for better error recovery we need to break out the current loop here
                if (!as_statement and expr.location.end.line != self.cur().location.begin.line) return self.reportAmbiguousCallError();

                expr = try self.parseFunctionArgs(expr, false);
            } else if (c == T.char('{') or c == .raw_string or c == .quoted_string) {
                expr = try self.parseFunctionArgs(expr, false);
            } else if (c == T.char('<') and (try self.lexer.lookahead()).type == T.char('<')) {
                expr = try self.parseExplicitTypeInstantiationExpr(start, expr);
            } else {
                break;
            }

            // note: while the parser isn't recursive here, we're generating recursive structures of unbounded depth
            try self.incrementRecursionCounter("expression");
        }

        self.recursion_counter = old_recursion_count;

        return expr;
    }

    fn parseIndexExpr(self: *Parser, start: Position, expr: *Expr) Error!*Expr {
        const match_bracket = MatchLexeme.from(self.cur());
        try self.nextLexeme();

        const index = try self.parseExpr(0);

        const end = self.cur().location.end;

        try self.expectMatchAndConsume(']', match_bracket);

        return self.newExpr(Location.span(start, end), .{ .index_expr = .{ .expr = expr, .index = index } });
    }

    fn parseMethodCall(self: *Parser, start: Position, expr: *Expr) Error!*Expr {
        const op_position = self.cur().location.begin;
        try self.nextLexeme();

        const index = try self.parseIndexName("method name", op_position);

        const func = try self.newExpr(Location.span(start, index.location.end), .{ .index_name = .{ .expr = expr, .index = index.name, .index_location = index.location, .op_position = op_position, .op = ':', .private_access = self.in_class_method_body and isPrivateFieldName(index.name) } });

        var type_arguments: []const ast.TypeOrPack = &.{};

        if (self.curType() == T.char('<') and (try self.lexer.lookahead()).type == T.char('<')) {
            type_arguments = (try self.parseTypeInstantiationExpr()).types;
        }

        const call = try self.parseFunctionArgs(func, true);

        // If we have an AstExprCall, fill in the type arguments
        if (call.data == .call and type_arguments.len > 0) call.data.call.type_arguments = type_arguments;

        return call;
    }

    // asexp -> simpleexp [`::' Type]
    fn parseAssertionExpr(self: *Parser) Error!*Expr {
        const start = self.cur().location;
        const expr = try self.parseSimpleExpr();

        if (self.curType() == .double_colon) {
            try self.nextLexeme();
            const annotation = try self.parseType(false);
            return self.newExpr(Location.join(start, annotation.location), .{ .type_assertion = .{ .expr = expr, .annotation = annotation } });
        }
        return expr;
    }

    fn parseAttributedFunction(self: *Parser, start: Location) Error!*Expr {
        const attributes = try self.parseAttributes();

        if (self.curType() != .reserved_function) {
            return self.report(start, "Expected 'function' declaration after attribute, but got {s} instead", .{try self.tokStr(self.cur())});
        }

        const match_function = self.cur();
        try self.nextLexeme();

        return (try self.parseFunctionBody(false, match_function, null, null, attributes, false)).func;
    }

    // simpleexp -> NUMBER | STRING | NIL | true | false | ... | constructor | [attributes] FUNCTION body | primaryexp
    fn parseSimpleExpr(self: *Parser) Error!*Expr {
        const start = self.cur().location;

        switch (self.curType()) {
            .attribute, .attribute_open => return self.parseAttributedFunction(start),
            .reserved_nil => {
                try self.nextLexeme();
                return self.newExpr(start, .constant_nil);
            },
            .reserved_true => {
                try self.nextLexeme();
                return self.newExpr(start, .{ .constant_bool = true });
            },
            .reserved_false => {
                try self.nextLexeme();
                return self.newExpr(start, .{ .constant_bool = false });
            },
            .reserved_function => {
                const match_function = self.cur();
                try self.nextLexeme();
                return (try self.parseFunctionBody(false, match_function, null, null, &.{}, false)).func;
            },
            .number => return self.parseNumber(),
            .raw_string, .quoted_string, .interp_string_simple => return self.parseString(),
            .interp_string_begin => return self.parseInterpString(),
            .broken_string => {
                try self.nextLexeme();
                return self.report(start, "Malformed string; did you forget to finish it?", .{});
            },
            .broken_interp_double_brace => {
                try self.nextLexeme();
                return self.report(start, "Double braces are not permitted within interpolated strings; did you mean '\\{{'?", .{});
            },
            .dot3 => {
                if (self.topFunction().vararg) {
                    try self.nextLexeme();
                    return self.newExpr(start, .varargs);
                }
                try self.nextLexeme();
                return self.report(start, "Cannot use '...' outside of a vararg function", .{});
            },
            .reserved_if => return self.parseIfElseExpr(),
            else => {
                if (self.curType() == T.char('{')) return self.parseTableConstructor();
                return self.parsePrimaryExpr(false);
            },
        }
    }

    const CallList = struct { args: []const *Expr, args_location: Location };

    fn parseCallList(self: *Parser) Error!CallList {
        if (self.curType() == T.char('(')) {
            const arg_start = self.cur().location.end;

            const match_paren = MatchLexeme.from(self.cur());
            try self.nextLexeme();

            var args: std.ArrayList(*Expr) = .empty;

            if (self.curType() != T.char(')')) try self.parseExprList(&args);

            const end = self.cur().location;
            const arg_end = end.end;

            try self.expectMatchAndConsume(')', match_paren);

            return .{ .args = args.items, .args_location = Location.span(arg_start, arg_end) };
        } else if (self.curType() == T.char('{')) {
            const arg_start = self.cur().location.end;
            const expr = try self.parseTableConstructor();
            const arg_end = self.lexer.previousLocation().end;

            return .{ .args = try self.copy(*Expr, &.{expr}), .args_location = Location.span(arg_start, arg_end) };
        } else {
            const arg_location = self.cur().location;
            const expr = try self.parseString();
            return .{ .args = try self.copy(*Expr, &.{expr}), .args_location = arg_location };
        }
    }

    // args ::=  `(' [explist] `)' | tableconstructor | String
    fn parseFunctionArgs(self: *Parser, func: *Expr, is_self: bool) Error!*Expr {
        if (self.curType() == T.char('(')) {
            const arg_start = self.cur().location.end;
            if (func.location.end.line != self.cur().location.begin.line) return self.reportAmbiguousCallError();

            const match_paren = MatchLexeme.from(self.cur());
            try self.nextLexeme();

            var args: std.ArrayList(*Expr) = .empty;

            if (self.curType() != T.char(')')) try self.parseExprList(&args);

            const end = self.cur().location;
            const arg_end = end.end;

            try self.expectMatchAndConsume(')', match_paren);

            return self.newExpr(Location.join(func.location, end), .{ .call = .{ .func = func, .args = args.items, .self = is_self, .table_call = false, .type_arguments = &.{}, .arg_location = Location.span(arg_start, arg_end) } });
        } else if (self.curType() == T.char('{')) {
            const arg_start = self.cur().location.end;
            const expr = try self.parseTableConstructor();
            const arg_end = self.lexer.previousLocation().end;

            return self.newExpr(Location.join(func.location, expr.location), .{ .call = .{ .func = func, .args = try self.copy(*Expr, &.{expr}), .self = is_self, .table_call = true, .type_arguments = &.{}, .arg_location = Location.span(arg_start, arg_end) } });
        } else if (self.curType() == .raw_string or self.curType() == .quoted_string) {
            const arg_location = self.cur().location;
            const expr = try self.parseString();

            return self.newExpr(Location.join(func.location, expr.location), .{ .call = .{ .func = func, .args = try self.copy(*Expr, &.{expr}), .self = is_self, .table_call = false, .type_arguments = &.{}, .arg_location = arg_location } });
        } else {
            return self.reportFunctionArgsError(func, is_self);
        }
    }

    fn reportFunctionArgsError(self: *Parser, func: *Expr, is_self: bool) Error {
        if (is_self and self.cur().location.begin.line != func.location.end.line) {
            return self.report(func.location, "Expected function call arguments after '('", .{});
        }
        const curr = self.tokStr(self.cur()) catch |e| return e;
        return self.report(Location.span(func.location.begin, self.cur().location.begin), "Expected '(', '{{' or <string> when parsing function call, got {s}", .{curr});
    }

    fn reportAmbiguousCallError(self: *Parser) Error {
        return self.report(self.cur().location, "Ambiguous syntax: this looks like an argument list for a function call, but could also be a start of " ++
            "new statement; use ';' to separate statements", .{});
    }

    // tableconstructor ::= `{' [fieldlist] `}'
    // fieldlist ::= field {fieldsep field} [fieldsep]
    // field ::= `[' exp `]' `=' exp | Name `=' exp | exp
    // fieldsep ::= `,' | `;'
    fn parseTableConstructor(self: *Parser) Error!*Expr {
        var items: std.ArrayList(ast.TableItem) = .empty;

        const start = self.cur().location;

        const match_brace = MatchLexeme.from(self.cur());
        try self.expectAndConsume(T.char('{'), "table literal");

        while (self.curType() != T.char('}')) {
            if (self.curType() == T.char('[')) {
                const match_location_bracket = MatchLexeme.from(self.cur());
                try self.nextLexeme();

                const key = try self.parseExpr(0);

                try self.expectMatchAndConsume(']', match_location_bracket);

                try self.expectAndConsume(T.char('='), "table field");

                const value = try self.parseExpr(0);

                try items.append(self.arena, .{ .kind = .general, .key = key, .value = value });
            } else if (self.curType() == .name and (try self.lexer.lookahead()).type == T.char('=')) {
                const name = try self.parseName("table field");

                try self.expectAndConsume(T.char('='), "table field");

                const key = try self.newExpr(name.location, .{ .constant_string = name.name });
                const value = try self.parseExpr(0);

                if (value.data == .function) value.data.function.debugname = name.name;

                try items.append(self.arena, .{ .kind = .record, .key = key, .value = value });
            } else {
                const expr = try self.parseExpr(0);

                try items.append(self.arena, .{ .kind = .list, .key = null, .value = expr });
            }

            if (self.curType() == T.char(',') or self.curType() == T.char(';')) {
                try self.nextLexeme();
            } else if (self.curType() == T.char('[') or self.curType() == .name) {
                return self.report(self.cur().location, "Expected ',' after table constructor element", .{});
            } else if (self.curType() != T.char('}')) {
                break;
            }
        }

        const end = self.cur().location;

        try self.expectMatchAndConsume('}', match_brace);

        return self.newExpr(Location.join(start, end), .{ .table = items.items });
    }

    fn parseIfElseExpr(self: *Parser) Error!*Expr {
        const start = self.cur().location;

        try self.nextLexeme(); // skip if / elseif

        var cond_local: ?*ast.Local = null;
        var cond_is_const = false;
        var condition: *Expr = undefined;
        var true_expr: *Expr = undefined;

        if (try self.isIfLocalStart()) {
            cond_is_const = self.isName("const");
            try self.nextLexeme(); // consume 'local' or 'const'

            const binding = try self.parseBinding(cond_is_const);

            if (self.curType() == T.char(','))
                return self.report(self.cur().location, "Expected '=' after variable name in 'if local', got ','; only a single binding is allowed", .{});

            try self.expectAndConsume(T.char('='), "if local declaration");

            condition = try self.parseExpr(0);

            try self.expectAndConsume(.reserved_then, "if then else expression");

            // Push the binding after the condition so the condition cannot reference it, and restore after the
            // true expression so it is not visible in the else/elseif branch.
            const locals_begin = self.saveLocals();
            cond_local = try self.pushLocal(binding);

            true_expr = try self.parseExpr(0);

            try self.restoreLocals(locals_begin);
        } else {
            condition = try self.parseExpr(0);

            try self.expectAndConsume(.reserved_then, "if then else expression");

            true_expr = try self.parseExpr(0);
        }

        var false_expr: *Expr = undefined;
        if (self.curType() == .reserved_elseif) {
            const old_recursion_count = self.recursion_counter;
            try self.incrementRecursionCounter("expression");
            false_expr = try self.parseIfElseExpr();
            self.recursion_counter = old_recursion_count;
        } else {
            try self.expectAndConsume(.reserved_else, "if then else expression");
            false_expr = try self.parseExpr(0);
        }

        return self.newExpr(Location.join(start, false_expr.location), .{ .if_else = .{ .condition = condition, .has_then = true, .true_expr = true_expr, .has_else = true, .false_expr = false_expr, .condition_local = cond_local, .condition_is_const = cond_is_const } });
    }

    /// Returns the unescaped string contents, or null when the escape sequences are malformed.
    fn parseCharArray(self: *Parser) Error!?[]const u8 {
        const c = self.cur();
        const data = try self.arena.dupe(u8, c.data.?);

        var len: usize = undefined;
        if (c.type == .quoted_string or c.type == .interp_string_simple) {
            if (lex.fixupQuotedString(data)) |l| {
                len = l;
            } else {
                try self.nextLexeme();
                return null;
            }
        } else {
            len = lex.fixupMultilineString(data);
        }

        try self.nextLexeme();
        return data[0..len];
    }

    fn parseString(self: *Parser) Error!*Expr {
        const location = self.cur().location;

        if (try self.parseCharArray()) |value| return self.newExpr(location, .{ .constant_string = value });
        return self.report(location, "String literal contains malformed escape sequence", .{});
    }

    fn parseInterpString(self: *Parser) Error!*Expr {
        var strings: std.ArrayList([]const u8) = .empty;
        var expressions: std.ArrayList(*Expr) = .empty;

        const start_location = self.cur().location;
        var end_location: Location = .{};

        while (true) {
            const current_lexeme = self.cur();

            end_location = current_lexeme.location;

            const data = try self.arena.dupe(u8, current_lexeme.data.?);

            const len = lex.fixupQuotedString(data) orelse {
                try self.nextLexeme();
                return self.report(Location.join(start_location, end_location), "Interpolated string literal contains malformed escape sequence", .{});
            };

            try self.nextLexeme();

            try strings.append(self.arena, data[0..len]);

            if (current_lexeme.type == .interp_string_end or current_lexeme.type == .interp_string_simple) break;

            switch (self.curType()) {
                .interp_string_mid, .interp_string_end => {
                    try self.nextLexeme();
                    return self.report(end_location, "Malformed interpolated string, expected expression inside '{{}}'", .{});
                },
                .broken_string => {
                    try self.nextLexeme();
                    return self.report(end_location, "Malformed interpolated string; did you forget to add a '`'?", .{});
                },
                else => try expressions.append(self.arena, try self.parseExpr(0)),
            }

            switch (self.curType()) {
                .interp_string_begin, .interp_string_mid, .interp_string_end => {},
                .broken_interp_double_brace => {
                    try self.nextLexeme();
                    return self.report(end_location, "Double braces are not permitted within interpolated strings; did you mean '\\{{'?", .{});
                },
                .broken_string, .eof => {
                    if (self.curType() == .broken_string) try self.nextLexeme();
                    if (self.lexer.peekBraceStackTop()) |top| {
                        // We are in a broken interpolated string, the top of the stack is non empty, we are missing '}'
                        if (top == .interpolated_string)
                            return self.report(self.lexer.previousLocation(), "Malformed interpolated string; did you forget to add a '}}'?", .{});
                        // Upstream reports nothing here and returns the partial node; the next parse step then reports.
                        return self.newExpr(Location.join(start_location, self.lexer.previousLocation()), .{ .interp_string = .{ .strings = strings.items, .expressions = expressions.items } });
                    }
                    // We are in a broken interpolated string, the top of the stack is empty, we are missing '`'.
                    return self.report(self.lexer.previousLocation(), "Malformed interpolated string; did you forget to add a '`'?", .{});
                },
                else => return self.report(end_location, "Malformed interpolated string, got {s}", .{try self.tokStr(self.cur())}),
            }
        }

        return self.newExpr(Location.join(start_location, end_location), .{ .interp_string = .{ .strings = strings.items, .expressions = expressions.items } });
    }

    fn parseExplicitTypeInstantiationExpr(self: *Parser, start: Position, based_on: *Expr) Error!*Expr {
        const r = try self.parseTypeInstantiationExpr();
        return self.newExpr(Location.span(start, r.end_location.end), .{ .instantiate = .{ .expr = based_on, .type_arguments = r.types } });
    }

    const TypeInstantiation = struct { types: []const ast.TypeOrPack, end_location: Location };

    fn parseTypeInstantiationExpr(self: *Parser) Error!TypeInstantiation {
        const begin = self.cur();
        _ = try self.lexer.next();

        const type_or_packs = try self.parseTypeParams();

        const end_location = self.cur().location;

        try self.expectMatchAndConsume('>', MatchLexeme.from(begin));
        return .{ .types = type_or_packs, .end_location = end_location };
    }

    fn parseNumber(self: *Parser) Error!*Expr {
        const start = self.cur().location;

        // Remove all internal _ - they don't hold any meaning
        const raw = self.cur().data.?;
        const buf = try self.arena.alloc(u8, raw.len);
        var n: usize = 0;
        for (raw) |ch| {
            if (ch != '_') {
                buf[n] = ch;
                n += 1;
            }
        }
        const text = buf[0..n];

        if (self.flags.LuauIntegerType2 and text.len > 0 and text[text.len - 1] == 'i') {
            const result = parseInteger64(text);
            try self.nextLexeme();

            return switch (result) {
                .ok => |v| self.newExpr(start, .{ .constant_integer = v }),
                .malformed => self.report(start, "Malformed integer", .{}),
                .overflow => self.report(start, "Integer overflow", .{}),
            };
        }

        const value = try parseDouble(self.arena, text);
        try self.nextLexeme();

        if (value) |v| return self.newExpr(start, .{ .constant_number = v });
        return self.report(start, "Malformed number", .{});
    }
};

const kAllowedMetamethods = [_][]const u8{
    "__init", "__call", "__concat", "__unm", "__add", "__sub", "__mul", "__div", "__mod",
    "__pow",  "__tostring", "__eq", "__lt", "__le", "__iter", "__len", "__idiv",
};

const kExplicitlyDisallowedMetamethods = [_][]const u8{ "__index", "__newindex", "__mode", "__metatable", "__type" };

const kMaxClassPrivateProperties: usize = 255;

fn isNameIn(name: []const u8, list: []const []const u8) bool {
    for (list) |n| if (std.mem.eql(u8, n, cstr(name))) return true;
    return false;
}

/// A single leading underscore marks a private member. Metamethod names begin with '__' and are public.
fn isPrivateFieldName(name: []const u8) bool {
    return name.len >= 1 and name[0] == '_' and (name.len < 2 or name[1] != '_');
}

/// Returns the diagnostic for a class member whose name is illegal for its access modifier, or null when the name is acceptable.
fn getClassFieldNameError(name: []const u8, access: Stat.ClassAccess) ?[]const u8 {
    if (access == .private) {
        if (std.mem.startsWith(u8, name, "__"))
            return "Private class properties cannot start with '__'"
        else if (name.len == 0 or name[0] != '_')
            return "Private class properties must begin with '_'";
    } else if (isPrivateFieldName(name)) {
        return "Public class properties cannot begin with '_'";
    }

    return null;
}

const Integer64Result = union(enum) { ok: i64, malformed, overflow };

/// Upstream parseInteger64: `data` ends in 'i'; hex/binary literals cover the full uint64 bit-pattern range.
fn parseInteger64(data: []const u8) Integer64Result {
    const body = data[0 .. data.len - 1];
    var base: u8 = 10;
    var digits = body;
    if (body.len >= 2 and body[0] == '0' and (body[1] == 'x' or body[1] == 'X')) {
        // strtoull accepts the 0x prefix; it needs at least one hex digit after it
        base = 16;
        digits = body[2..];
        if (digits.len == 0 or !lex.isHexDigit(digits[0])) return .malformed;
    } else if (body.len >= 2 and body[0] == '0' and (body[1] == 'b' or body[1] == 'B')) {
        base = 2;
        digits = body[2..];
        // Some libc implementations accept an optional 0b prefix for base-2 parsing; upstream rejects it
        if (digits.len >= 2 and digits[0] == '0' and (digits[1] == 'b' or digits[1] == 'B')) return .malformed;
    }

    if (digits.len == 0) return .malformed;

    var value: u64 = 0;
    var overflow = false;
    for (digits) |ch| {
        const d: u8 = if (lex.isDigit(ch)) ch - '0' else if (base == 16 and lex.isHexDigit(ch)) (ch | ' ') - 'a' + 10 else return .malformed;
        if (d >= base) return .malformed;
        const mul = @mulWithOverflow(value, base);
        const add = @addWithOverflow(mul[0], d);
        if (mul[1] != 0 or add[1] != 0) overflow = true;
        value = add[0];
    }

    if (overflow) return .overflow;
    // decimal literals are parsed with strtoll and are limited to INT64_MAX
    if (base == 10 and value > std.math.maxInt(i64)) return .overflow;
    return .{ .ok = @bitCast(value) };
}

/// AstName values are C strings in upstream; formatting stops at an embedded NUL.
fn cstr(s: []const u8) []const u8 {
    if (std.mem.indexOfScalar(u8, s, 0)) |i| return s[0..i];
    return s;
}

/// Upstream parseInteger (strtoull-based) for base 2 and 16. `data` includes the 0x prefix for base 16.
/// Returns null when malformed; overflow saturates to ULLONG_MAX like strtoull.
fn parseInteger(data: []const u8, base: u8) ?f64 {
    var digits = data;
    if (base == 2) {
        // Some libc implementations accept an optional 0b prefix for base-2 parsing.
        if (digits.len >= 2 and digits[0] == '0' and (digits[1] == 'b' or digits[1] == 'B')) return null;
    } else {
        // strtoull accepts an optional 0x prefix in base 16
        if (digits.len >= 2 and digits[0] == '0' and (digits[1] == 'x' or digits[1] == 'X')) {
            const after = digits[2..];
            if (after.len == 0 or !lex.isHexDigit(after[0])) {
                // strtoull parses the leading "0" and stops at 'x'
                return null;
            }
            digits = after;
        }
    }

    if (digits.len == 0) return null;

    var value: u64 = 0;
    var overflow = false;
    for (digits) |ch| {
        const d: u8 = if (lex.isDigit(ch)) ch - '0' else if (base == 16 and lex.isHexDigit(ch)) (ch | ' ') - 'a' + 10 else return null;
        if (d >= base) return null;
        const mul = @mulWithOverflow(value, base);
        const add = @addWithOverflow(mul[0], d);
        if (mul[1] != 0 or add[1] != 0) overflow = true;
        value = add[0];
    }

    if (overflow) value = std.math.maxInt(u64);
    return @floatFromInt(value);
}

/// Upstream parseDouble: binary/hex literals via strtoull, decimal via strtod with full-string match.
fn parseDouble(arena: Allocator, data: []const u8) Error!?f64 {
    // binary literal
    if (data.len > 2 and data[0] == '0' and (data[1] == 'b' or data[1] == 'B')) return parseInteger(data[2..], 2);

    // hexadecimal literal
    if (data.len > 2 and data[0] == '0' and (data[1] == 'x' or data[1] == 'X')) return parseInteger(data, 16);

    // strtod decimal grammar: digits [. digits] [(e|E) [+|-] digits]; at least one mantissa digit
    var i: usize = 0;
    while (i < data.len and lex.isDigit(data[i])) i += 1;
    const int_part = data[0..i];
    var frac_part: []const u8 = "";
    if (i < data.len and data[i] == '.') {
        i += 1;
        const frac_start = i;
        while (i < data.len and lex.isDigit(data[i])) i += 1;
        frac_part = data[frac_start..i];
    }
    if (int_part.len + frac_part.len == 0) return null;
    var exp_part: []const u8 = "0";
    if (i < data.len and (data[i] == 'e' or data[i] == 'E')) {
        var j = i + 1;
        if (j < data.len and (data[j] == '+' or data[j] == '-')) j += 1;
        const exp_digits = j;
        while (j < data.len and lex.isDigit(data[j])) j += 1;
        // without exponent digits strtod stops before the 'e', leaving trailing characters
        if (j == exp_digits) return null;
        exp_part = data[i + 1 .. j];
        i = j;
    }
    if (i != data.len) return null;

    // Feed a canonical "I.FeX" spelling to the correctly rounded std parser (same value as strtod).
    const canonical = try std.fmt.allocPrint(arena, "{s}.{s}e{s}", .{
        if (int_part.len > 0) int_part else "0",
        if (frac_part.len > 0) frac_part else "0",
        exp_part,
    });
    return std.fmt.parseFloat(f64, canonical) catch null;
}
