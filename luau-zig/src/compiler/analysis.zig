//! Compiler analysis passes: AST walking, value tracking, constant folding,
//! builtin call detection and table shape prediction.
//!
//! Translated from third_party/luau/Compiler/src/{ValueTracking,ConstantFolding,
//! Builtins,TableShape}.cpp and Utils.h (Luau, MIT License; see
//! third_party/luau/LICENSE.txt), with all upstream feature flags at their
//! default (off) values. Builtin constant folding (BuiltinFolding.cpp) lives in
//! builtin_folding.zig and is used when upstream enables it (optimization level 2).

const std = @import("std");
const Allocator = std.mem.Allocator;
const ast = @import("ast.zig");
const bc = @import("bytecode.zig");
const lex = @import("lexer.zig");
const builtin_folding = @import("builtin_folding.zig");
const flags_mod = @import("flags.zig");
const Expr = ast.Expr;
const Stat = ast.Stat;

pub const Error = error{ OutOfMemory, CompileError };

// ------------------------------------------------------------------ AST walking

/// Generic AstVisitor traversal. `V` provides `visitExpr(*V, *Expr) Error!bool` and
/// `visitStat(*V, *Stat) Error!bool`; returning true descends into children in the
/// same order as upstream Ast.cpp. Type annotations are never visited (upstream
/// AstVisitor::visit(AstType*) returns false by default).
pub fn walkExpr(comptime V: type, v: *V, e: *Expr) Error!void {
    if (!try v.visitExpr(e)) return;
    switch (e.data) {
        .group => |x| try walkExpr(V, v, x),
        .call => |c| {
            try walkExpr(V, v, c.func);
            for (c.args) |a| try walkExpr(V, v, a);
        },
        .index_name => |x| try walkExpr(V, v, x.expr),
        .index_expr => |x| {
            try walkExpr(V, v, x.expr);
            try walkExpr(V, v, x.index);
        },
        .function => |f| try walkStat(V, v, f.body),
        .table => |items| for (items) |item| {
            if (item.key) |k| try walkExpr(V, v, k);
            try walkExpr(V, v, item.value);
        },
        .unary => |x| try walkExpr(V, v, x.expr),
        .binary => |x| {
            try walkExpr(V, v, x.left);
            try walkExpr(V, v, x.right);
        },
        .type_assertion => |x| try walkExpr(V, v, x.expr),
        .if_else => |x| {
            try walkExpr(V, v, x.condition);
            try walkExpr(V, v, x.true_expr);
            try walkExpr(V, v, x.false_expr);
        },
        .interp_string => |x| for (x.expressions) |a| try walkExpr(V, v, a),
        .instantiate => |x| try walkExpr(V, v, x.expr),
        .constant_nil, .constant_bool, .constant_number, .constant_string, .constant_integer, .local, .global, .varargs => {},
    }
}

pub fn walkStat(comptime V: type, v: *V, s: *Stat) Error!void {
    if (!try v.visitStat(s)) return;
    switch (s.data) {
        .block => |b| for (b.body) |x| try walkStat(V, v, x),
        .@"if" => |x| {
            try walkExpr(V, v, x.condition);
            try walkStat(V, v, x.thenbody);
            if (x.elsebody) |e| try walkStat(V, v, e);
        },
        .@"while" => |x| {
            try walkExpr(V, v, x.condition);
            try walkStat(V, v, x.body);
        },
        .repeat => |x| {
            try walkStat(V, v, x.body);
            try walkExpr(V, v, x.condition);
        },
        .@"return" => |list| for (list) |e| try walkExpr(V, v, e),
        .expr => |e| try walkExpr(V, v, e),
        .local => |x| for (x.values) |e| try walkExpr(V, v, e),
        .@"for" => |x| {
            try walkExpr(V, v, x.from);
            try walkExpr(V, v, x.to);
            if (x.step) |st| try walkExpr(V, v, st);
            try walkStat(V, v, x.body);
        },
        .for_in => |x| {
            for (x.values) |e| try walkExpr(V, v, e);
            try walkStat(V, v, x.body);
        },
        .assign => |x| {
            for (x.vars) |e| try walkExpr(V, v, e);
            for (x.values) |e| try walkExpr(V, v, e);
        },
        .compound_assign => |x| {
            try walkExpr(V, v, x.variable);
            try walkExpr(V, v, x.value);
        },
        .function => |x| {
            try walkExpr(V, v, x.name);
            try walkExpr(V, v, x.func);
        },
        .local_function => |x| try walkExpr(V, v, x.func),
        .type_function => |x| try walkExpr(V, v, x.body),
        .class => |x| {
            if (x.super) |sup| try walkExpr(V, v, sup);
            for (x.members) |m| switch (m) {
                .property => {},
                .method => |method| try walkExpr(V, v, method.function),
            };
        },
        .type_alias, .@"break", .@"continue" => {},
    }
}

/// Utils.h unwrapExprOfType: strips groups and type assertions.
pub fn unwrapExpr(node: *Expr) *Expr {
    var e = node;
    while (true) {
        switch (e.data) {
            .group => |g| e = g,
            .type_assertion => |t| e = t.expr,
            else => return e,
        }
    }
}

// ------------------------------------------------------------------ value tracking

pub const Global = enum {
    default,
    mutable, // builtin that has contents unknown at compile time, blocks GETIMPORT for chains
    written, // written in the code which means we can't reason about the value
};

pub const Variable = struct {
    init: ?*Expr = null, // initial value of the variable; filled by trackValues
    owner: ?*Stat = null, // block statement in which the variable was defined (not necessarily directly); LuauCompileReuseLocalRegs
    last_used: ?*Expr = null, // last lexical expression where the variable was read; LuauCompileReuseLocalRegs
    written: bool = false, // is the variable ever assigned to? filled by trackValues
    constant: bool = false, // is the variable's value a compile-time constant? filled by constantFold
    non_lexical_use: bool = false, // variable may be used by expressions which do not lexically mention it (like function calls)
};

pub const Globals = std.StringHashMapUnmanaged(Global);
pub const Variables = std.AutoHashMapUnmanaged(*ast.Local, Variable);

pub fn getGlobalState(globals: *const Globals, name: []const u8) Global {
    return globals.get(name) orelse .default;
}

pub fn assignMutable(gpa: Allocator, globals: *Globals, names: *const lex.NameTable, mutable_globals: []const []const u8) Error!void {
    if (names.get("_G")) |name| try globals.put(gpa, name, .mutable);
    for (mutable_globals) |g| {
        if (names.get(g)) |name| try globals.put(gpa, name, .mutable);
    }
}

/// Exported bindings collected by trackValues when LuauOptimizeExportTable is enabled
pub const ExportedLocals = struct {
    /// insertion ordered (upstream iterates a DenseHashSet, whose order depends on pointer hashes)
    functions: std.AutoArrayHashMapUnmanaged(*ast.Local, void) = .empty,
    variables: std.ArrayList(*ast.Local) = .empty,
};

const ValueVisitor = struct {
    gpa: Allocator,
    globals: *Globals,
    variables: *Variables,
    exported: ?*ExportedLocals = null,
    reuse_local_regs: bool = false,
    block_owner_stack: std.ArrayList(*Stat) = .empty,

    fn variable(self: *ValueVisitor, local: *ast.Local) Error!*Variable {
        const gop = try self.variables.getOrPut(self.gpa, local);
        if (!gop.found_existing) gop.value_ptr.* = .{};
        return gop.value_ptr;
    }

    fn assign(self: *ValueVisitor, v: *Expr) Error!void {
        switch (v.data) {
            .local => |l| {
                const variable_ = try self.variable(l.local);
                variable_.written = true;

                if (self.reuse_local_regs) {
                    variable_.last_used = v;

                    if (l.upvalue or l.local.is_exported) variable_.non_lexical_use = true;
                }
            },
            .global => |g| try self.globals.put(self.gpa, g, .written),
            // we need to be able to track assignments in all expressions, including crazy ones like t[function() t = nil end] = 5
            else => try walkExpr(ValueVisitor, self, v),
        }
    }

    pub fn visitExpr(self: *ValueVisitor, e: *Expr) Error!bool {
        switch (e.data) {
            .local => |l| {
                if (self.reuse_local_regs) {
                    const variable_ = try self.variable(l.local);
                    variable_.last_used = e;

                    if (l.upvalue or l.local.is_exported) variable_.non_lexical_use = true;

                    return false;
                }
            },
            .function => |f| for (f.args) |arg| {
                (try self.variable(arg)).init = null;
            },
            .if_else => |ie| if (ie.condition_local) |cl| {
                (try self.variable(cl)).init = ie.condition;
            },
            else => {},
        }
        return true;
    }

    pub fn visitStat(self: *ValueVisitor, s: *Stat) Error!bool {
        switch (s.data) {
            .block => |b| {
                if (self.reuse_local_regs) {
                    try self.block_owner_stack.append(self.gpa, s);
                    for (b.body) |stat| try walkStat(ValueVisitor, self, stat);
                    _ = self.block_owner_stack.pop();
                    return false;
                }
                return true;
            },
            .@"if" => |n| {
                if (n.condition_local) |cl| (try self.variable(cl)).init = n.condition;
                return true;
            },
            .local => |n| {
                for (n.vars, 0..) |v, i| {
                    (try self.variable(v)).init = if (i < n.values.len) n.values[i] else null;
                }
                if (self.exported) |ex| {
                    for (n.vars) |v| {
                        if (v.is_exported) try ex.variables.append(self.gpa, v);
                    }
                }
                if (self.reuse_local_regs) {
                    for (n.vars) |v| (try self.variable(v)).owner = self.block_owner_stack.getLast();
                }
                return true;
            },
            .assign => |n| {
                for (n.vars) |v| try self.assign(v);
                for (n.values) |v| try walkExpr(ValueVisitor, self, v);
                return false;
            },
            .compound_assign => |n| {
                try self.assign(n.variable);
                try walkExpr(ValueVisitor, self, n.value);
                return false;
            },
            .local_function => |n| {
                const variable_ = try self.variable(n.name);
                variable_.init = n.func;

                if (self.reuse_local_regs) {
                    variable_.owner = self.block_owner_stack.getLast();

                    if (n.name.is_exported) variable_.non_lexical_use = true;
                }

                if (self.exported) |ex| {
                    if (n.name.is_exported) {
                        try ex.functions.put(self.gpa, n.name, {});
                        try ex.variables.append(self.gpa, n.name);
                    }
                }
                return true;
            },
            .function => |n| {
                try self.assign(n.name);
                try walkExpr(ValueVisitor, self, n.func);
                return false;
            },
            else => return true,
        }
    }
};

pub fn trackValues(gpa: Allocator, globals: *Globals, variables: *Variables, exported: ?*ExportedLocals, reuse_local_regs: bool, root: *Stat) Error!void {
    var visitor: ValueVisitor = .{ .gpa = gpa, .globals = globals, .variables = variables, .exported = exported, .reuse_local_regs = reuse_local_regs };
    try walkStat(ValueVisitor, &visitor, root);
}

// ------------------------------------------------------------------ constants

pub const Constant = union(enum) {
    unknown,
    nil,
    boolean: bool,
    number: f64,
    integer: i64,
    vectorf: [4]f32,
    vectord: [4]f64,
    string: []const u8,
    /// index into the constant table list of the folding pass
    table: usize,

    pub fn isTruthful(self: Constant) bool {
        std.debug.assert(self != .unknown);
        return switch (self) {
            .nil => false,
            .boolean => |b| b,
            else => true,
        };
    }

    pub fn known(self: Constant) bool {
        return self != .unknown;
    }
};

pub const Constants = std.AutoHashMapUnmanaged(*Expr, Constant);
pub const LocalConstants = std.AutoHashMapUnmanaged(*ast.Local, Constant);
/// Locals holding constant tables that are never mutated (TableConstantKind::ConstantTable)
pub const TableConstants = std.AutoHashMapUnmanaged(*ast.Local, void);

pub fn isConstantTrue(constants: *const Constants, node: *Expr) bool {
    const cv = constants.get(node) orelse return false;
    return cv.known() and cv.isTruthful();
}

pub fn isConstantFalse(constants: *const Constants, node: *Expr) bool {
    const cv = constants.get(node) orelse return false;
    return cv.known() and !cv.isTruthful();
}

/// true iff all execution paths through node subtree result in return/break/continue
/// note: because this function doesn't visit loop nodes, it (correctly) only detects break/continue that refer to the outer control flow
pub fn alwaysTerminates(constants: *const Constants, node: *Stat) bool {
    switch (node.data) {
        .block => |b| {
            for (b.body) |s| if (alwaysTerminates(constants, s)) return true;
            return false;
        },
        .@"return", .@"break", .@"continue" => return true,
        .@"if" => |s| {
            if (isConstantTrue(constants, s.condition)) return alwaysTerminates(constants, s.thenbody);
            if (isConstantFalse(constants, s.condition) and s.elsebody != null) return alwaysTerminates(constants, s.elsebody.?);
            return s.elsebody != null and alwaysTerminates(constants, s.thenbody) and alwaysTerminates(constants, s.elsebody.?);
        },
        else => return false,
    }
}

const kConstantFoldStringLimit = 4096;

fn constantsEqual(la: Constant, ra: Constant, fl: flags_mod.Flags) bool {
    return switch (la) {
        .nil => ra == .nil,
        .boolean => |b| ra == .boolean and ra.boolean == b,
        .number => |n| ra == .number and ra.number == n,
        .vectorf => |v| ra == .vectorf and v[0] == ra.vectorf[0] and v[1] == ra.vectorf[1] and v[2] == ra.vectorf[2] and v[3] == ra.vectorf[3],
        .vectord => |v| ra == .vectord and v[0] == ra.vectord[0] and v[1] == ra.vectord[1] and v[2] == ra.vectord[2] and v[3] == ra.vectord[3],
        .string => |s| ra == .string and std.mem.eql(u8, ra.string, s),
        .table => |t| ra == .table and ra.table == t,
        // without LuauIntegerType2 upstream release builds report "not equal" for integer constants
        .integer => |n| fl.LuauIntegerType2 and ra == .integer and ra.integer == n,
        .unknown => unreachable,
    };
}

fn foldUnary(op: ast.UnaryOp, arg: Constant) Constant {
    switch (op) {
        .not => if (arg.known()) return .{ .boolean = !arg.isTruthful() },
        .minus => switch (arg) {
            .number => |n| return .{ .number = -n },
            .vectorf => |v| return .{ .vectorf = .{ -v[0], -v[1], -v[2], -v[3] } },
            .vectord => |v| return .{ .vectord = .{ -v[0], -v[1], -v[2], -v[3] } },
            else => {},
        },
        .len => if (arg == .string) return .{ .number = @floatFromInt(arg.string.len) },
    }
    return .unknown;
}

const VecOp = enum { add, sub, mul, div, idiv };

fn vecApply(comptime T: type, op: VecOp, a: T, b: T) T {
    return switch (op) {
        .add => a + b,
        .sub => a - b,
        .mul => a * b,
        .div => a / b,
        .idiv => @floor(a / b),
    };
}

/// Component-wise vector folding. For mul/div/idiv upstream only folds when the w component of the result is zero
/// or one of the vector operands had a non-zero w (`had_w`), since w may be invisible at runtime.
fn vecFold(comptime T: type, op: VecOp, a: [4]T, b: [4]T, had_w: bool, check_w: bool) ?[4]T {
    const result_w = vecApply(T, op, a[3], b[3]);
    if (check_w and !(result_w == 0 or had_w)) return null;
    return .{ vecApply(T, op, a[0], b[0]), vecApply(T, op, a[1], b[1]), vecApply(T, op, a[2], b[2]), result_w };
}

fn foldVector(op: VecOp, la: Constant, ra: Constant) Constant {
    switch (op) {
        .add, .sub => {
            if (la == .vectorf and ra == .vectorf) return .{ .vectorf = vecFold(f32, op, la.vectorf, ra.vectorf, false, false).? };
            if (la == .vectord and ra == .vectord) return .{ .vectord = vecFold(f64, op, la.vectord, ra.vectord, false, false).? };
        },
        .mul, .div, .idiv => {
            if (la == .vectorf and ra == .vectorf) {
                if (vecFold(f32, op, la.vectorf, ra.vectorf, la.vectorf[3] != 0 or ra.vectorf[3] != 0, true)) |r| return .{ .vectorf = r };
            } else if (la == .number and ra == .vectorf) {
                const n: f32 = @floatCast(la.number);
                if (vecFold(f32, op, @splat(n), ra.vectorf, ra.vectorf[3] != 0, true)) |r| return .{ .vectorf = r };
            } else if (la == .vectorf and ra == .number) {
                const n: f32 = @floatCast(ra.number);
                if (vecFold(f32, op, la.vectorf, @splat(n), la.vectorf[3] != 0, true)) |r| return .{ .vectorf = r };
            } else if (la == .vectord and ra == .vectord) {
                if (vecFold(f64, op, la.vectord, ra.vectord, la.vectord[3] != 0 or ra.vectord[3] != 0, true)) |r| return .{ .vectord = r };
            } else if (la == .number and ra == .vectord) {
                if (vecFold(f64, op, @splat(la.number), ra.vectord, ra.vectord[3] != 0, true)) |r| return .{ .vectord = r };
            } else if (la == .vectord and ra == .number) {
                if (vecFold(f64, op, la.vectord, @splat(ra.number), la.vectord[3] != 0, true)) |r| return .{ .vectord = r };
            }
        },
    }
    return .unknown;
}

// vector component 'w' is not visible to VM runtime configured with LUA_VECTOR_SIZE == 3, so vectors that only differ in 'w' can't be compared
fn vectorsDifferOnlyInW(la: Constant, ra: Constant) bool {
    if (la == .vectorf and ra == .vectorf) {
        const a = la.vectorf;
        const b = ra.vectorf;
        return a[0] == b[0] and a[1] == b[1] and a[2] == b[2] and a[3] != b[3];
    }
    if (la == .vectord and ra == .vectord) {
        const a = la.vectord;
        const b = ra.vectord;
        return a[0] == b[0] and a[1] == b[1] and a[2] == b[2] and a[3] != b[3];
    }
    return false;
}

fn foldBinary(names: *lex.NameTable, op: ast.BinaryOp, la: Constant, ra: Constant, fl: flags_mod.Flags) Error!Constant {
    const nums = la == .number and ra == .number;
    switch (op) {
        .add => return if (nums) .{ .number = la.number + ra.number } else foldVector(.add, la, ra),
        .sub => return if (nums) .{ .number = la.number - ra.number } else foldVector(.sub, la, ra),
        .mul => return if (nums) .{ .number = la.number * ra.number } else foldVector(.mul, la, ra),
        .div => return if (nums) .{ .number = la.number / ra.number } else foldVector(.div, la, ra),
        .floor_div => return if (nums) .{ .number = @floor(la.number / ra.number) } else foldVector(.idiv, la, ra),
        .mod => if (nums) return .{ .number = la.number - @floor(la.number / ra.number) * ra.number },
        .pow => if (nums) return .{ .number = std.math.pow(f64, la.number, ra.number) },
        .concat => if (la == .string and ra == .string and la.string.len + ra.string.len <= kConstantFoldStringLimit) {
            if (la.string.len == 0) return .{ .string = ra.string };
            if (ra.string.len == 0) return .{ .string = la.string };
            const tmp = try names.arena.alloc(u8, la.string.len + ra.string.len);
            @memcpy(tmp[0..la.string.len], la.string);
            @memcpy(tmp[la.string.len..], ra.string);
            return .{ .string = try names.getOrAdd(tmp) };
        },
        .compare_ne => if (la.known() and ra.known() and !(fl.LuauCompileNoFoldVectorEqW and vectorsDifferOnlyInW(la, ra))) return .{ .boolean = !constantsEqual(la, ra, fl) },
        .compare_eq => if (la.known() and ra.known() and !(fl.LuauCompileNoFoldVectorEqW and vectorsDifferOnlyInW(la, ra))) return .{ .boolean = constantsEqual(la, ra, fl) },
        .compare_lt => if (nums) return .{ .boolean = la.number < ra.number },
        .compare_le => if (nums) return .{ .boolean = la.number <= ra.number },
        .compare_gt => if (nums) return .{ .boolean = la.number > ra.number },
        .compare_ge => if (nums) return .{ .boolean = la.number >= ra.number },
        .@"and" => if (la.known()) return if (la.isTruthful()) ra else la,
        .@"or" => if (la.known()) return if (la.isTruthful()) la else ra,
    }
    return .unknown;
}

// Pass to detect which tables are mutated or 'escape'
const TableMutationTracker = struct {
    gpa: Allocator,
    flags: flags_mod.Flags,
    escaped: std.AutoHashMapUnmanaged(*ast.Local, void) = .empty,

    fn markEscaped(self: *TableMutationTracker, expr_in: *Expr) Error!void {
        var expr = expr_in;
        while (true) {
            switch (expr.data) {
                .local => |l| {
                    try self.escaped.put(self.gpa, l.local, {});
                    return;
                },
                .group => |g| expr = g,
                .type_assertion => |t| expr = t.expr,
                .instantiate => |i| expr = i.expr,
                .if_else => |ie| {
                    try self.markEscaped(ie.true_expr); // recurse through true branch
                    expr = ie.false_expr; // continue loop with false branch
                },
                .binary => |b| {
                    if (b.op == .@"and" or b.op == .@"or") {
                        try self.markEscaped(b.left); // recurse through lhs
                        expr = b.right; // continue loop with rhs
                    } else return;
                },
                else => return,
            }
        }
    }

    fn markEscapedTableIndex(self: *TableMutationTracker, expr: *Expr, is_lvalue: bool) Error!void {
        switch (expr.data) {
            .index_name => |i| try self.markEscaped(i.expr),
            .index_expr => |i| {
                try self.markEscaped(i.expr);
                if (!self.flags.LuauCompileTableFoldFix and is_lvalue) try self.markEscaped(i.index);
            },
            else => {},
        }
    }

    pub fn visitExpr(self: *TableMutationTracker, e: *Expr) Error!bool {
        switch (e.data) {
            .call => |c| {
                // Values passed in as arguments can escape
                for (c.args) |a| try self.markEscaped(a);
                // Table indexed in a self call escapes through 'self'
                if (c.self) try self.markEscapedTableIndex(c.func, false);
            },
            .table => |items| for (items) |item| {
                // Values stored inside a table constructor can escape
                if (item.key) |k| try self.markEscaped(k);
                try self.markEscaped(item.value);
            },
            .binary => |b| if (self.flags.LuauCompileTableFoldFix) try self.markEscapedBinaryOp(b.left, b.right, b.op),
            .index_expr => |i| if (self.flags.LuauCompileTableFoldFix) try self.markEscaped(i.index),
            else => {},
        }
        return true;
    }

    pub fn visitStat(self: *TableMutationTracker, s: *Stat) Error!bool {
        switch (s.data) {
            .local => |n| {
                // Aliasing a table reference marks the source as escaped
                for (n.values, 0..) |v, i| if (i < n.vars.len) try self.markEscaped(v);
            },
            .assign => |n| {
                // RHS values that are table locals are being aliased
                for (n.values) |v| try self.markEscaped(v);
                // LHS index expressions mutate the table being indexed
                for (n.vars) |v| try self.markEscapedTableIndex(v, true);
            },
            .compound_assign => |n| {
                // LHS index expressions mutate the table
                try self.markEscapedTableIndex(n.variable, true);
                if (self.flags.LuauCompileTableFoldFix) try self.markEscapedBinaryOp(n.variable, n.value, n.op);
            },
            .function => |n| try self.markEscapedTableIndex(n.name, true),
            .for_in => |n| for (n.values) |v| try self.markEscaped(v),
            .@"return" => |list| for (list) |v| try self.markEscaped(v),
            else => {},
        }
        return true;
    }

    fn markEscapedBinaryOp(self: *TableMutationTracker, lhs: *Expr, rhs: *Expr, op: ast.BinaryOp) Error!void {
        // Comparisons do not support operands with different metamethods, and/or don't have a metamethod
        switch (op) {
            .compare_lt, .compare_le, .compare_gt, .compare_ge, .compare_eq, .compare_ne, .@"and", .@"or" => return,
            else => {},
        }
        try self.markEscaped(lhs);
        try self.markEscaped(rhs);
    }
};

pub fn buildTableConstantMap(gpa: Allocator, result: *TableConstants, variables: *const Variables, root: *Stat, fl: flags_mod.Flags) Error!void {
    var tracker: TableMutationTracker = .{ .gpa = gpa, .flags = fl };
    try walkStat(TableMutationTracker, &tracker, root);

    var it = variables.iterator();
    while (it.next()) |entry| {
        const v = entry.value_ptr.*;
        if (v.written) continue;
        const init_expr = v.init orelse continue;
        if (unwrapExpr(init_expr).data != .table) continue;
        if (!tracker.escaped.contains(entry.key_ptr.*)) try result.put(gpa, entry.key_ptr.*, {});
    }
}

pub const ExprConstantChange = struct { key: *Expr, old_value: Constant, was_absent: bool };
pub const LocalConstantChange = struct { key: *ast.Local, old_value: Constant, was_absent: bool };
pub const ExprChangeLog = std.ArrayList(ExprConstantChange);
pub const LocalChangeLog = std.ArrayList(LocalConstantChange);

/// Value returned by a library member constant callback (upstream setCompileConstant*).
pub const LibraryConstant = union(enum) {
    nil,
    boolean: bool,
    number: f64,
    integer: i64,
    vector: [4]f32,
    vectord: [4]f64,
    /// copied into compiler-owned memory before use
    string: []const u8,
};

/// Upstream LibraryMemberConstantCallback: return null to leave the member unknown.
pub const LibraryMemberConstantCallback = *const fn (library: []const u8, member: []const u8) ?LibraryConstant;

pub const FoldOptions = struct {
    /// builtins used for constant folding (upstream `builtinsFold`, only set at optimization level 2)
    builtins: ?*const Builtins = null,
    fold_library_k: bool = false,
    vector_double_precision: bool = false,
    library_member_constant_cb: ?LibraryMemberConstantCallback = null,
    flags: flags_mod.Flags = .{},
};

const ConstantVisitor = struct {
    gpa: Allocator,
    constants: *Constants,
    variables: *Variables,
    locals: *LocalConstants,
    names: *lex.NameTable,
    options: FoldOptions,
    constant_tables: std.ArrayList(std.StringHashMapUnmanaged(Constant)) = .empty,
    constant_table_locals: *const TableConstants,
    table_locals: LocalConstants = .empty,
    was_empty: bool,
    builtin_args: std.ArrayList(Constant) = .empty,
    expr_change_log: ?*ExprChangeLog,
    local_change_log: ?*LocalChangeLog,

    fn analyze(self: *ConstantVisitor, node: *Expr) Error!Constant {
        var result: Constant = .unknown;

        switch (node.data) {
            .group => |g| result = try self.analyze(g),
            .constant_nil => result = .nil,
            .constant_bool => |b| result = .{ .boolean = b },
            .constant_number => |n| result = .{ .number = n },
            .constant_string => |s| result = .{ .string = s },
            .constant_integer => |n| result = .{ .integer = n },
            .local => |l| {
                if (self.locals.get(l.local)) |c| {
                    result = c;
                } else if (self.table_locals.get(l.local)) |c| {
                    result = c;
                }
            },
            .global, .varargs => {},
            .call => |c| {
                _ = try self.analyze(c.func);

                const bfid: ?bc.Builtin = if (self.options.builtins) |b| b.get(node) else null;

                if (bfid != null and bfid.? != .NONE) {
                    // since recursive calls to analyze() may reuse the vector we need to be careful and preserve existing contents
                    const offset = self.builtin_args.items.len;
                    var can_fold = true;

                    for (c.args) |arg| {
                        const ac = try self.analyze(arg);
                        if (ac == .unknown or ac == .table)
                            can_fold = false
                        else
                            try self.builtin_args.append(self.gpa, ac);
                    }

                    if (can_fold) {
                        std.debug.assert(self.builtin_args.items.len == offset + c.args.len);
                        result = try builtin_folding.foldBuiltin(self.names, bfid.?, self.builtin_args.items[offset..], self.options.vector_double_precision);
                    }

                    self.builtin_args.shrinkRetainingCapacity(offset);
                } else {
                    for (c.args) |a| _ = try self.analyze(a);
                }
            },
            .index_name => |i| {
                const value = try self.analyze(i.expr);
                switch (value) {
                    .table => |t| {
                        if (t < self.constant_tables.items.len) {
                            if (self.constant_tables.items[t].get(i.index)) |prop| result = prop;
                        }
                    },
                    // Do not handle 'w' component because it isn't known if the runtime will be configured in 3-wide or 4-wide mode
                    .vectorf => |v| {
                        if (vectorComponent(i.index)) |ci| result = .{ .number = v[ci] };
                    },
                    .vectord => |v| {
                        if (vectorComponent(i.index)) |ci| result = .{ .number = v[ci] };
                    },
                    else => if (self.options.fold_library_k and i.expr.data == .global) {
                        const lib = i.expr.data.global;
                        if (std.mem.eql(u8, lib, "math")) result = builtin_folding.foldBuiltinMath(i.index);

                        // if we have a custom handler and the constant hasn't been resolved
                        if (self.options.library_member_constant_cb) |cb| {
                            if (result == .unknown) {
                                if (cb(lib, i.index)) |lc| result = try self.libraryConstant(lc);

                                if (self.options.vector_double_precision and result == .vectorf) {
                                    const v = result.vectorf;
                                    result = .{ .vectord = .{ v[0], v[1], v[2], v[3] } };
                                } else if (!self.options.vector_double_precision and result == .vectord) {
                                    const v = result.vectord;
                                    result = .{ .vectorf = .{ @floatCast(v[0]), @floatCast(v[1]), @floatCast(v[2]), @floatCast(v[3]) } };
                                }
                            }
                        }
                    },
                }
            },
            .index_expr => |i| {
                const index_val = try self.analyze(i.index);
                const table_val = try self.analyze(i.expr);
                if (table_val == .table and index_val == .string) {
                    if (table_val.table < self.constant_tables.items.len and index_val.string.len != 0) {
                        const index_name = try self.names.getOrAdd(index_val.string);
                        if (self.constant_tables.items[table_val.table].get(index_name)) |prop| result = prop;
                    }
                }
            },
            .function => |f| {
                // this is necessary to propagate constant information in all child functions
                try walkStat(ConstantVisitor, self, f.body);
            },
            .table => |items| {
                // If expr is a constant table, update result to be a table constant, and insert it into constantTables
                var props: std.StringHashMapUnmanaged(Constant) = .empty;
                for (items) |item| {
                    const value_val = try self.analyze(item.value);
                    if (item.key) |key| {
                        const key_val = try self.analyze(key);
                        if (key_val == .string and value_val.known() and value_val != .table and key_val.string.len != 0) {
                            const const_key = try self.names.getOrAdd(key_val.string);
                            try props.put(self.gpa, const_key, value_val);
                        }
                    }
                }
                if (props.count() == items.len) {
                    result = .{ .table = self.constant_tables.items.len };
                    try self.constant_tables.append(self.gpa, props);
                }
            },
            .unary => |u| {
                const arg = try self.analyze(u.expr);
                if (arg.known()) result = foldUnary(u.op, arg);
            },
            .binary => |b| {
                const la = try self.analyze(b.left);
                const ra = try self.analyze(b.right);
                // note: ra doesn't need to be constant to fold and/or
                if (la.known()) result = try foldBinary(self.names, b.op, la, ra, self.options.flags);
            },
            .type_assertion => |t| result = try self.analyze(t.expr),
            .if_else => |ie| {
                const cond = try self.analyze(ie.condition);
                const true_expr = try self.analyze(ie.true_expr);
                const false_expr = try self.analyze(ie.false_expr);
                if (cond.known()) result = if (cond.isTruthful()) true_expr else false_expr;
            },
            .interp_string => |is| {
                var only_constant = true;
                for (is.expressions) |e| {
                    if ((try self.analyze(e)) != .string) only_constant = false;
                }
                if (only_constant) result = try self.foldInterpString(is.strings, is.expressions);
            },
            .instantiate => |i| result = try self.analyze(i.expr),
        }

        try self.recordExprConstant(node, result);

        return result;
    }

    fn vectorComponent(index: []const u8) ?usize {
        if (std.mem.eql(u8, index, "x") or std.mem.eql(u8, index, "X")) return 0;
        if (std.mem.eql(u8, index, "y") or std.mem.eql(u8, index, "Y")) return 1;
        if (std.mem.eql(u8, index, "z") or std.mem.eql(u8, index, "Z")) return 2;
        return null;
    }

    fn libraryConstant(self: *ConstantVisitor, lc: LibraryConstant) Error!Constant {
        return switch (lc) {
            .nil => .nil,
            .boolean => |b| .{ .boolean = b },
            .number => |n| .{ .number = n },
            .integer => |n| .{ .integer = n },
            .vector => |v| .{ .vectorf = v },
            .vectord => |v| .{ .vectord = v },
            .string => |s| .{ .string = try self.names.arena.dupe(u8, s) },
        };
    }

    fn foldInterpString(self: *ConstantVisitor, strings: []const []const u8, expressions: []const *Expr) Error!Constant {
        var result_length: usize = 0;
        for (strings, 0..) |s, index| {
            result_length += s.len;
            if (index < expressions.len) result_length += self.constants.get(expressions[index]).?.string.len;
        }

        if (result_length > kConstantFoldStringLimit) return .unknown;
        if (result_length == 0) return .{ .string = "" };

        const tmp = try self.names.arena.alloc(u8, result_length);
        var w: usize = 0;
        for (strings, 0..) |s, index| {
            @memcpy(tmp[w .. w + s.len], s);
            w += s.len;
            if (index < expressions.len) {
                const c = self.constants.get(expressions[index]).?.string;
                @memcpy(tmp[w .. w + c.len], c);
                w += c.len;
            }
        }
        return .{ .string = try self.names.getOrAdd(tmp) };
    }

    fn recordExprConstant(self: *ConstantVisitor, key: *Expr, value: Constant) Error!void {
        if (value == .table) {
            // Table constants are recorded in a separate map
        } else if (value.known()) {
            if (self.expr_change_log) |log| {
                const old = self.constants.get(key);
                try log.append(self.gpa, .{ .key = key, .old_value = old orelse .unknown, .was_absent = old == null });
            }
            try self.constants.put(self.gpa, key, value);
        } else if (self.was_empty) {
            // No need to clear out entries if we started with empty maps
        } else if (self.constants.getPtr(key)) |old| {
            if (self.expr_change_log) |log| try log.append(self.gpa, .{ .key = key, .old_value = old.*, .was_absent = false });
            old.* = .unknown;
        }
    }

    fn recordLocalConstant(self: *ConstantVisitor, key: *ast.Local, value: Constant) Error!void {
        if (value == .table) {
            // Table constants are recorded in a separate map
        } else if (value.known()) {
            if (self.local_change_log) |log| {
                const old = self.locals.get(key);
                try log.append(self.gpa, .{ .key = key, .old_value = old orelse .unknown, .was_absent = old == null });
            }
            try self.locals.put(self.gpa, key, value);
        } else if (self.was_empty) {
            // No need to clear out entries if we started with empty maps
        } else if (self.locals.getPtr(key)) |old| {
            if (self.local_change_log) |log| try log.append(self.gpa, .{ .key = key, .old_value = old.*, .was_absent = false });
            old.* = .unknown;
        }
    }

    fn recordValue(self: *ConstantVisitor, local: *ast.Local, value: Constant) Error!void {
        // note: we rely on trackValues to have been run before us
        const v = self.variables.getPtr(local).?;
        if (!v.written) {
            if (value == .table) {
                v.constant = false;
                try self.table_locals.put(self.gpa, local, value);
            } else {
                v.constant = value.known();
                try self.recordLocalConstant(local, value);
            }
        }
    }

    fn recordLocal(self: *ConstantVisitor, local: *ast.Local, value: *Expr) Error!void {
        const arg = try self.analyze(value);

        if (arg == .table) {
            // If this table could be mutated later, record Constant_Unknown instead of Constant_Table
            if (self.constant_table_locals.contains(local))
                try self.recordValue(local, arg)
            else
                try self.recordValue(local, .unknown);
        } else {
            try self.recordValue(local, arg);
        }
    }

    pub fn visitExpr(self: *ConstantVisitor, e: *Expr) Error!bool {
        if (e.data == .if_else) {
            if (e.data.if_else.condition_local) |cl| try self.recordLocal(cl, e.data.if_else.condition);
        }

        // note: we short-circuit the visitor traversal through any expression trees by returning false
        // recursive traversal is happening inside analyze() which makes it easier to get the resulting value of the subexpression
        _ = try self.analyze(e);
        return false;
    }

    pub fn visitStat(self: *ConstantVisitor, s: *Stat) Error!bool {
        switch (s.data) {
            .@"if" => |n| {
                if (n.condition_local) |cl| {
                    try self.recordLocal(cl, n.condition);

                    try walkStat(ConstantVisitor, self, n.thenbody);
                    if (n.elsebody) |e| try walkStat(ConstantVisitor, self, e);

                    return false;
                }
                return true;
            },
            .local => |n| {
                // all values that align wrt indexing are simple - we just match them 1-1
                var i: usize = 0;
                while (i < n.vars.len and i < n.values.len) : (i += 1) {
                    const arg = try self.analyze(n.values[i]);
                    if (arg == .table) {
                        // If this table could be mutated later, record Constant_Unknown instead of Constant_Table
                        if (self.constant_table_locals.contains(n.vars[i]))
                            try self.recordValue(n.vars[i], arg)
                        else
                            try self.recordValue(n.vars[i], .unknown);
                    } else {
                        try self.recordValue(n.vars[i], arg);
                    }
                }

                if (n.vars.len > n.values.len) {
                    // if we have trailing variables, then depending on whether the last value is capable of returning multiple values
                    // (aka call or varargs), we either don't know anything about these vars, or we know they're nil
                    const last: ?*Expr = if (n.values.len > 0) n.values[n.values.len - 1] else null;
                    const mult_ret = if (last) |l| (l.data == .call or l.data == .varargs) else false;
                    if (!mult_ret) {
                        for (n.vars[n.values.len..]) |v| try self.recordValue(v, .nil);
                    }
                } else {
                    // we can have more values than variables; in this case we still need to analyze them to make sure we do constant propagation inside
                    // them
                    for (n.values[n.vars.len..]) |v| _ = try self.analyze(v);
                }
                return false;
            },
            else => return true,
        }
    }
};

pub fn foldConstants(
    gpa: Allocator,
    constants: *Constants,
    variables: *Variables,
    locals: *LocalConstants,
    options: FoldOptions,
    root: *Stat,
    names: *lex.NameTable,
    table_constants: *const TableConstants,
    expr_change_log: ?*ExprChangeLog,
    local_change_log: ?*LocalChangeLog,
) Error!void {
    var visitor: ConstantVisitor = .{
        .gpa = gpa,
        .constants = constants,
        .variables = variables,
        .locals = locals,
        .names = names,
        .options = options,
        .constant_table_locals = table_constants,
        // since we do a single pass over the tree, if the initial state was empty we don't need to clear out old entries
        .was_empty = constants.count() == 0 and locals.count() == 0,
        .expr_change_log = expr_change_log,
        .local_change_log = local_change_log,
    };
    try walkStat(ConstantVisitor, &visitor, root);
}

pub fn undoExprChanges(gpa: Allocator, constants: *Constants, changes: []const ExprConstantChange) Error!void {
    var i = changes.len;
    while (i > 0) : (i -= 1) {
        const c = changes[i - 1];
        if (c.was_absent) {
            if (constants.getPtr(c.key)) |old| old.* = .unknown;
        } else {
            try constants.put(gpa, c.key, c.old_value);
        }
    }
}

pub fn undoLocalChanges(gpa: Allocator, locals: *LocalConstants, changes: []const LocalConstantChange) Error!void {
    var i = changes.len;
    while (i > 0) : (i -= 1) {
        const c = changes[i - 1];
        if (c.was_absent) {
            if (locals.getPtr(c.key)) |old| old.* = .unknown;
        } else {
            try locals.put(gpa, c.key, c.old_value);
        }
    }
}


// ------------------------------------------------------------------ builtins

pub const Builtin = struct {
    object: ?[]const u8 = null,
    method: ?[]const u8 = null,

    fn empty(self: Builtin) bool {
        return self.method == null;
    }
    fn isGlobal(self: Builtin, name: []const u8) bool {
        return self.object == null and self.method != null and std.mem.eql(u8, self.method.?, name);
    }
    fn isMethod(self: Builtin, table: []const u8, name: []const u8) bool {
        return self.object != null and self.method != null and std.mem.eql(u8, self.object.?, table) and std.mem.eql(u8, self.method.?, name);
    }
    fn objectIs(self: Builtin, name: []const u8) bool {
        return self.object != null and std.mem.eql(u8, self.object.?, name);
    }
    fn methodIs(self: Builtin, name: []const u8) bool {
        return self.method != null and std.mem.eql(u8, self.method.?, name);
    }
};

pub fn getBuiltin(node: *Expr, globals: *const Globals, variables: *const Variables) Builtin {
    switch (node.data) {
        .local => |l| {
            const v = variables.get(l.local) orelse return .{};
            return if (!v.written and v.init != null) getBuiltin(v.init.?, globals, variables) else .{};
        },
        .index_name => |i| {
            if (i.expr.data == .local) {
                if (variables.get(i.expr.data.local.local)) |v| {
                    // Local that is initialized and not modified might hold the built-in library itself
                    if (!v.written and v.init != null) {
                        var object: ?[]const u8 = null;
                        // Look for patterns like 'local m = math' and 'local math = math or replacement'
                        switch (v.init.?.data) {
                            .global => |g| object = g,
                            .binary => |b| if (b.op == .@"or" and b.left.data == .global) {
                                object = b.left.data.global;
                            },
                            else => {},
                        }
                        if (object) |o| return if (getGlobalState(globals, o) == .default) .{ .object = o, .method = i.index } else .{};
                    }
                }
            }
            if (i.expr.data == .global) {
                const o = i.expr.data.global;
                return if (getGlobalState(globals, o) == .default) .{ .object = o, .method = i.index } else .{};
            }
            return .{};
        },
        .global => |g| return if (getGlobalState(globals, g) == .default) .{ .method = g } else .{},
        else => return .{},
    }
}

pub const BuiltinOptions = struct {
    vector_lib: ?[]const u8 = null,
    vector_ctor: ?[]const u8 = null,
    flags: flags_mod.Flags = .{},
};

fn getBuiltinFunctionId(builtin: Builtin, options: BuiltinOptions) ?bc.Builtin {
    const B = bc.Builtin;
    const globals_map = [_]struct { []const u8, B }{
        .{ "assert", .ASSERT },     .{ "type", .TYPE },                 .{ "typeof", .TYPEOF },           .{ "rawset", .RAWSET },
        .{ "rawget", .RAWGET },     .{ "rawequal", .RAWEQUAL },         .{ "rawlen", .RAWLEN },           .{ "unpack", .TABLE_UNPACK },
        .{ "select", .SELECT_VARARG }, .{ "getmetatable", .GETMETATABLE }, .{ "setmetatable", .SETMETATABLE }, .{ "tonumber", .TONUMBER },
        .{ "tostring", .TOSTRING },
    };
    for (globals_map) |e| if (builtin.isGlobal(e[0])) return e[1];

    const Lib = struct { []const u8, B };
    const math = [_]Lib{
        .{ "abs", .MATH_ABS },     .{ "acos", .MATH_ACOS },   .{ "asin", .MATH_ASIN },     .{ "atan2", .MATH_ATAN2 }, .{ "atan", .MATH_ATAN },
        .{ "ceil", .MATH_CEIL },   .{ "cosh", .MATH_COSH },   .{ "cos", .MATH_COS },       .{ "deg", .MATH_DEG },     .{ "exp", .MATH_EXP },
        .{ "floor", .MATH_FLOOR }, .{ "fmod", .MATH_FMOD },   .{ "frexp", .MATH_FREXP },   .{ "ldexp", .MATH_LDEXP }, .{ "log10", .MATH_LOG10 },
        .{ "log", .MATH_LOG },     .{ "max", .MATH_MAX },     .{ "min", .MATH_MIN },       .{ "modf", .MATH_MODF },   .{ "pow", .MATH_POW },
        .{ "rad", .MATH_RAD },     .{ "sinh", .MATH_SINH },   .{ "sin", .MATH_SIN },       .{ "sqrt", .MATH_SQRT },   .{ "tanh", .MATH_TANH },
        .{ "tan", .MATH_TAN },     .{ "clamp", .MATH_CLAMP }, .{ "sign", .MATH_SIGN },     .{ "round", .MATH_ROUND }, .{ "lerp", .MATH_LERP },
        .{ "isnan", .MATH_ISNAN }, .{ "isinf", .MATH_ISINF }, .{ "isfinite", .MATH_ISFINITE },
    };
    const bit32 = [_]Lib{
        .{ "arshift", .BIT32_ARSHIFT }, .{ "band", .BIT32_BAND },       .{ "bnot", .BIT32_BNOT },         .{ "bor", .BIT32_BOR },
        .{ "bxor", .BIT32_BXOR },       .{ "btest", .BIT32_BTEST },     .{ "extract", .BIT32_EXTRACT },   .{ "lrotate", .BIT32_LROTATE },
        .{ "lshift", .BIT32_LSHIFT },   .{ "replace", .BIT32_REPLACE }, .{ "rrotate", .BIT32_RROTATE },   .{ "rshift", .BIT32_RSHIFT },
        .{ "countlz", .BIT32_COUNTLZ }, .{ "countrz", .BIT32_COUNTRZ }, .{ "byteswap", .BIT32_BYTESWAP },
    };
    const string = [_]Lib{ .{ "byte", .STRING_BYTE }, .{ "char", .STRING_CHAR }, .{ "len", .STRING_LEN }, .{ "sub", .STRING_SUB } };
    // table.create / table.clear are gated behind LuauCompileTableBuiltinsCreateClear (off by default)
    const table_base = [_]Lib{ .{ "insert", .TABLE_INSERT }, .{ "unpack", .TABLE_UNPACK } };
    const table_create_clear = [_]Lib{ .{ "insert", .TABLE_INSERT }, .{ "unpack", .TABLE_UNPACK }, .{ "create", .TABLE_CREATE }, .{ "clear", .TABLE_CLEAR } };
    const table: []const Lib = if (options.flags.LuauCompileTableBuiltinsCreateClear) &table_create_clear else &table_base;
    const buffer = [_]Lib{
        .{ "readi8", .BUFFER_READI8 },     .{ "readu8", .BUFFER_READU8 },     .{ "writei8", .BUFFER_WRITEU8 },   .{ "writeu8", .BUFFER_WRITEU8 },
        .{ "readi16", .BUFFER_READI16 },   .{ "readu16", .BUFFER_READU16 },   .{ "writei16", .BUFFER_WRITEU16 }, .{ "writeu16", .BUFFER_WRITEU16 },
        .{ "readi32", .BUFFER_READI32 },   .{ "readu32", .BUFFER_READU32 },   .{ "writei32", .BUFFER_WRITEU32 }, .{ "writeu32", .BUFFER_WRITEU32 },
        .{ "readf32", .BUFFER_READF32 },   .{ "writef32", .BUFFER_WRITEF32 }, .{ "readf64", .BUFFER_READF64 },   .{ "writef64", .BUFFER_WRITEF64 },
    };
    const vector = [_]Lib{
        .{ "create", .VECTOR },       .{ "magnitude", .VECTOR_MAGNITUDE }, .{ "normalize", .VECTOR_NORMALIZE }, .{ "cross", .VECTOR_CROSS },
        .{ "dot", .VECTOR_DOT },      .{ "floor", .VECTOR_FLOOR },         .{ "ceil", .VECTOR_CEIL },           .{ "abs", .VECTOR_ABS },
        .{ "sign", .VECTOR_SIGN },    .{ "clamp", .VECTOR_CLAMP },         .{ "min", .VECTOR_MIN },             .{ "max", .VECTOR_MAX },
        .{ "lerp", .VECTOR_LERP },
    };
    const libs = [_]struct { []const u8, []const Lib }{
        .{ "math", &math }, .{ "bit32", &bit32 }, .{ "string", &string }, .{ "table", table }, .{ "buffer", &buffer }, .{ "vector", &vector },
    };
    for (libs) |lib| {
        if (builtin.objectIs(lib[0])) {
            for (lib[1]) |e| if (builtin.methodIs(e[0])) return e[1];
        }
    }

    if (options.flags.LuauIntegerFastcalls and options.flags.LuauIntegerBufferFastcalls and builtin.objectIs("buffer")) {
        if (builtin.methodIs("readinteger")) return .BUFFER_READINTEGER;
        if (builtin.methodIs("writeinteger")) return .BUFFER_WRITEINTEGER;
    }

    if (options.flags.LuauIntegerFastcalls and builtin.objectIs("integer")) {
        const integer = [_]Lib{
            .{ "add", .INTEGER_ADD },         .{ "sub", .INTEGER_SUB },         .{ "mod", .INTEGER_MOD },         .{ "mul", .INTEGER_MUL },
            .{ "div", .INTEGER_DIV },         .{ "idiv", .INTEGER_IDIV },       .{ "udiv", .INTEGER_UDIV },       .{ "rem", .INTEGER_REM },
            .{ "urem", .INTEGER_UREM },       .{ "min", .INTEGER_MIN },         .{ "max", .INTEGER_MAX },         .{ "neg", .INTEGER_NEG },
            .{ "create", .INTEGER_CREATE },   .{ "clamp", .INTEGER_CLAMP },     .{ "band", .INTEGER_BAND },       .{ "bor", .INTEGER_BOR },
            .{ "bxor", .INTEGER_BXOR },       .{ "bnot", .INTEGER_BNOT },       .{ "btest", .INTEGER_BTEST },     .{ "bswap", .INTEGER_BSWAP },
            .{ "lt", .INTEGER_LT },           .{ "le", .INTEGER_LE },           .{ "ult", .INTEGER_ULT },         .{ "ule", .INTEGER_ULE },
            .{ "gt", .INTEGER_GT },           .{ "ge", .INTEGER_GE },           .{ "ugt", .INTEGER_UGT },         .{ "uge", .INTEGER_UGE },
            .{ "lshift", .INTEGER_LSHIFT },   .{ "rshift", .INTEGER_RSHIFT },   .{ "arshift", .INTEGER_ARSHIFT }, .{ "lrotate", .INTEGER_LROTATE },
            .{ "rrotate", .INTEGER_RROTATE }, .{ "countrz", .INTEGER_COUNTRZ }, .{ "countlz", .INTEGER_COUNTLZ }, .{ "extract", .INTEGER_EXTRACT },
            .{ "tonumber", .INTEGER_TONUMBER },
        };
        for (integer) |e| if (builtin.methodIs(e[0])) return e[1];
    }

    if (options.vector_ctor) |ctor| {
        if (options.vector_lib) |vlib| {
            if (builtin.isMethod(vlib, ctor)) return .VECTOR;
        } else {
            if (builtin.isGlobal(ctor)) return .VECTOR;
        }
    }

    return null;
}

pub const Builtins = std.AutoHashMapUnmanaged(*Expr, bc.Builtin);

const BuiltinVisitor = struct {
    gpa: Allocator,
    result: *Builtins,
    disabled: [256]bool,
    globals: *const Globals,
    variables: *const Variables,
    options: BuiltinOptions,

    pub fn visitExpr(self: *BuiltinVisitor, e: *Expr) Error!bool {
        if (e.data != .call) return true;
        const node = e.data.call;

        const builtin: Builtin = if (node.self) .{} else getBuiltin(node.func, self.globals, self.variables);
        if (builtin.empty()) return true;

        var bfid = getBuiltinFunctionId(builtin, self.options);

        if (bfid) |id| {
            if (self.disabled[@intFromEnum(id)]) bfid = null;
        }

        // getBuiltinFunctionId optimistically assumes all select() calls are builtin but actually the second argument must be a vararg
        if (bfid == .SELECT_VARARG and !(node.args.len == 2 and node.args[1].data == .varargs)) bfid = null;

        if (bfid) |id| try self.result.put(self.gpa, e, id);

        return true; // propagate to nested calls
    }

    pub fn visitStat(_: *BuiltinVisitor, _: *Stat) Error!bool {
        return true;
    }
};

pub fn analyzeBuiltins(gpa: Allocator, result: *Builtins, globals: *const Globals, variables: *const Variables, options: BuiltinOptions, disabled_builtins: []const []const u8, root: *Stat, names: *const lex.NameTable) Error!void {
    var visitor: BuiltinVisitor = .{ .gpa = gpa, .result = result, .disabled = @splat(false), .globals = globals, .variables = variables, .options = options };

    for (disabled_builtins) |entry| {
        if (std.mem.indexOfScalar(u8, entry, '.')) |dot| {
            const library = names.get(entry[0..dot]);
            const name = names.get(entry[dot + 1 ..]);
            if (library != null and name != null and getGlobalState(globals, name.?) == .default) {
                if (getBuiltinFunctionId(.{ .object = library, .method = name }, options)) |id| visitor.disabled[@intFromEnum(id)] = true;
            }
        } else {
            if (names.get(entry)) |name| {
                if (getGlobalState(globals, name) == .default) {
                    if (getBuiltinFunctionId(.{ .method = name }, options)) |id| visitor.disabled[@intFromEnum(id)] = true;
                }
            }
        }
    }

    try walkStat(BuiltinVisitor, &visitor, root);
}

// ------------------------------------------------------------------ table shapes

pub const TableShape = struct {
    array_size: u32 = 0,
    hash_size: u32 = 0,
};

pub const TableShapes = std.AutoHashMapUnmanaged(*Expr, TableShape);

// conservative limit for the loop bound that establishes table array size
const kMaxLoopBound = 16;

fn getTableHint(expr: *Expr) ?*Expr {
    // unadorned table literal
    if (expr.data == .table) return expr;

    // setmetatable(table literal, ...)
    if (expr.data == .call) {
        const call = expr.data.call;
        if (!call.self and call.args.len == 2 and call.func.data == .global and std.mem.eql(u8, call.func.data.global, "setmetatable")) {
            if (call.args[0].data == .table) return call.args[0];
        }
    }
    return null;
}

const ShapeVisitor = struct {
    const Field = struct { table: *Expr, name: []const u8 };
    const FieldContext = struct {
        pub fn hash(_: FieldContext, f: Field) u64 {
            return std.hash.Wyhash.hash(@intFromPtr(f.table), f.name);
        }
        pub fn eql(_: FieldContext, a: Field, b: Field) bool {
            return a.table == b.table and std.mem.eql(u8, a.name, b.name);
        }
    };

    gpa: Allocator,
    shapes: *TableShapes,
    tables: std.AutoHashMapUnmanaged(*ast.Local, *Expr) = .empty,
    fields: std.HashMapUnmanaged(Field, void, FieldContext, 80) = .empty,
    loops: std.AutoHashMapUnmanaged(*ast.Local, u32) = .empty, // iterator => upper bound for 1..k

    fn shape(self: *ShapeVisitor, table: *Expr) Error!*TableShape {
        const gop = try self.shapes.getOrPut(self.gpa, table);
        if (!gop.found_existing) gop.value_ptr.* = .{};
        return gop.value_ptr;
    }

    fn assignFieldName(self: *ShapeVisitor, expr: *Expr, index: []const u8) Error!void {
        if (expr.data != .local) return;
        const table = self.tables.get(expr.data.local.local) orelse return;
        const field: Field = .{ .table = table, .name = index };
        if (!self.fields.contains(field)) {
            try self.fields.put(self.gpa, field, {});
            (try self.shape(table)).hash_size += 1;
        }
    }

    fn assignFieldExpr(self: *ShapeVisitor, expr: *Expr, index: *Expr) Error!void {
        if (expr.data != .local) return;
        const table = self.tables.get(expr.data.local.local) orelse return;

        switch (index.data) {
            .constant_number => |n| {
                const s = try self.shape(table);
                if (n == @as(f64, @floatFromInt(s.array_size + 1))) s.array_size += 1;
            },
            .local => |iter| {
                if (self.loops.get(iter.local)) |bound| {
                    const s = try self.shape(table);
                    if (s.array_size == 0) s.array_size = bound;
                }
            },
            else => {},
        }
    }

    fn assign(self: *ShapeVisitor, v: *Expr) Error!void {
        switch (v.data) {
            .index_name => |i| try self.assignFieldName(i.expr, i.index),
            .index_expr => |i| try self.assignFieldExpr(i.expr, i.index),
            else => {},
        }
    }

    pub fn visitExpr(_: *ShapeVisitor, _: *Expr) Error!bool {
        return true;
    }

    pub fn visitStat(self: *ShapeVisitor, s: *Stat) Error!bool {
        switch (s.data) {
            .local => |n| {
                // track local -> table association so that we can update table size prediction in assignField
                if (n.vars.len == 1 and n.values.len == 1) {
                    if (getTableHint(n.values[0])) |table| {
                        if (table.data.table.len == 0) try self.tables.put(self.gpa, n.vars[0], table);
                    }
                }
                return true;
            },
            .assign => |n| {
                for (n.vars) |v| try self.assign(v);
                for (n.values) |v| try walkExpr(ShapeVisitor, self, v);
                return false;
            },
            .function => |n| {
                try self.assign(n.name);
                try walkExpr(ShapeVisitor, self, n.func);
                return false;
            },
            .@"for" => |n| {
                if (n.from.data == .constant_number and n.to.data == .constant_number and n.step == null) {
                    const from = n.from.data.constant_number;
                    const to = n.to.data.constant_number;
                    if (from == 1.0 and to >= 1.0 and to <= @as(f64, kMaxLoopBound)) try self.loops.put(self.gpa, n.variable, @intFromFloat(to));
                }
                return true;
            },
            else => return true,
        }
    }
};

pub fn predictTableShapes(gpa: Allocator, shapes: *TableShapes, root: *Stat) Error!void {
    var visitor: ShapeVisitor = .{ .gpa = gpa, .shapes = shapes };
    try walkStat(ShapeVisitor, &visitor, root);
}
