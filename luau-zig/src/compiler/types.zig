//! Type map construction for bytecode type information.
//!
//! Translated from third_party/luau/Compiler/src/Types.cpp and Types.h (Luau,
//! MIT License; see third_party/luau/LICENSE.txt), with LuauIntegerFastcalls
//! at its default (off) value.

const std = @import("std");
const Allocator = std.mem.Allocator;
const ast = @import("ast.zig");
const bc = @import("bytecode.zig");
const an = @import("analysis.zig");
const builder_mod = @import("builder.zig");
const flags_mod = @import("flags.zig");
const Expr = ast.Expr;
const Stat = ast.Stat;
const AType = ast.Type;

/// Upstream LibraryMemberTypeCallback: returns an LBC_TYPE_* value for `library.member`.
pub const LibraryMemberTypeCallback = *const fn (library: []const u8, member: []const u8) u8;

fn syntheticRef(comptime name: []const u8) AType {
    return .{ .location = .{}, .data = .{ .reference = .{ .prefix = null, .name = name, .has_parameters = false, .parameters = &.{}, .synthetic = true } } };
}

/// Upstream BuiltinAstTypes.
pub const BuiltinAstTypes = struct {
    boolean_type: AType = syntheticRef("boolean"),
    number_type: AType = syntheticRef("number"),
    integer_type: AType = syntheticRef("integer"),
    string_type: AType = syntheticRef("string"),
    vector_type: AType = syntheticRef("vector"),
};

pub const FunctionTypes = std.AutoHashMapUnmanaged(*Expr, []const u8);
pub const LocalTypes = std.AutoHashMapUnmanaged(*ast.Local, u8);
pub const ExprTypes = std.AutoHashMapUnmanaged(*Expr, u8);
pub const UserdataTypes = std.StringHashMapUnmanaged(u8);

const LBC_TYPE_INVALID: u16 = bc.LBC_TYPE_INVALID;

const TypeAliases = std.StringHashMapUnmanaged(?*Stat);
const SeenAliases = std.StringHashMapUnmanaged(void);

fn isGeneric(name: []const u8, generics: []const *ast.GenericType) bool {
    for (generics) |gt| if (std.mem.eql(u8, gt.name, name)) return true;
    return false;
}

fn getPrimitiveType(name: []const u8) u16 {
    const eq = std.mem.eql;
    if (eq(u8, name, "nil")) return bc.LBC_TYPE_NIL;
    if (eq(u8, name, "boolean")) return bc.LBC_TYPE_BOOLEAN;
    if (eq(u8, name, "number")) return bc.LBC_TYPE_NUMBER;
    if (eq(u8, name, "integer")) return bc.LBC_TYPE_INTEGER;
    if (eq(u8, name, "string")) return bc.LBC_TYPE_STRING;
    if (eq(u8, name, "thread")) return bc.LBC_TYPE_THREAD;
    if (eq(u8, name, "buffer")) return bc.LBC_TYPE_BUFFER;
    if (eq(u8, name, "vector")) return bc.LBC_TYPE_VECTOR;
    if (eq(u8, name, "any") or eq(u8, name, "unknown")) return bc.LBC_TYPE_ANY;
    return LBC_TYPE_INVALID;
}

const TypeContext = struct {
    gpa: Allocator,
    type_aliases: *const TypeAliases,
    host_vector_type: ?[]const u8,
    userdata_types: *const UserdataTypes,
    bytecode: *builder_mod.BytecodeBuilder,
};

fn getType(ctx: TypeContext, ty: *const AType, generics: []const *ast.GenericType, seen: *SeenAliases) an.Error!u8 {
    switch (ty.data) {
        .reference => |ref| {
            if (ref.prefix != null) return bc.LBC_TYPE_ANY;

            if (!ref.synthetic) {
                if (ctx.type_aliases.get(ref.name)) |alias_opt| {
                    if (alias_opt) |alias| {
                        const a = alias.data.type_alias;
                        if (seen.contains(a.name)) return bc.LBC_TYPE_ANY;
                        try seen.put(ctx.gpa, ref.name, {});
                        const t = try getType(ctx, a.type, a.generics, seen);
                        _ = seen.remove(ref.name);
                        return t;
                    }
                }
            }

            if (isGeneric(ref.name, generics)) return bc.LBC_TYPE_ANY;

            if (ctx.host_vector_type) |hvt| {
                if (std.mem.eql(u8, ref.name, hvt)) return bc.LBC_TYPE_VECTOR;
            }

            const prim = getPrimitiveType(ref.name);
            if (prim != LBC_TYPE_INVALID) return @intCast(prim);

            if (!ref.synthetic) {
                if (ctx.userdata_types.get(ref.name)) |index| {
                    ctx.bytecode.useUserdataType(index);
                    return bc.LBC_TYPE_TAGGED_USERDATA_BASE + index;
                }
            }

            // not primitive or alias or generic => host-provided, we assume userdata for now
            return bc.LBC_TYPE_USERDATA;
        },
        .table => return bc.LBC_TYPE_TABLE,
        .function => return bc.LBC_TYPE_FUNCTION,
        .@"union" => |types| {
            var optional = false;
            var result: u16 = LBC_TYPE_INVALID;

            for (types) |t| {
                const et = try getType(ctx, t, generics, seen);

                if (et == bc.LBC_TYPE_NIL) {
                    optional = true;
                    continue;
                }

                if (result == LBC_TYPE_INVALID) {
                    result = et;
                    continue;
                }

                if (result != et) return bc.LBC_TYPE_ANY;
            }

            if (result == LBC_TYPE_INVALID) return bc.LBC_TYPE_ANY;

            const r: u8 = @intCast(result);
            return r | (if (optional and r != bc.LBC_TYPE_ANY) bc.LBC_TYPE_OPTIONAL_BIT else 0);
        },
        .intersection => return bc.LBC_TYPE_ANY,
        .group => |g| return getType(ctx, g, generics, seen),
        .optional => return bc.LBC_TYPE_NIL,
        .singleton_bool => return bc.LBC_TYPE_BOOLEAN,
        .singleton_string => return bc.LBC_TYPE_STRING,
        .typeof => return bc.LBC_TYPE_ANY,
    }
}

fn getFunctionType(ctx: TypeContext, func: *const ast.Function) an.Error![]const u8 {
    const has_self = func.self != null;

    var type_info: std.ArrayList(u8) = .empty;
    try type_info.ensureTotalCapacity(ctx.gpa, func.args.len + @intFromBool(has_self) + 2);

    try type_info.append(ctx.gpa, bc.LBC_TYPE_FUNCTION);
    try type_info.append(ctx.gpa, @truncate(@intFromBool(has_self) + func.args.len));

    if (has_self) try type_info.append(ctx.gpa, bc.LBC_TYPE_TABLE);

    var have_non_any_param = false;
    for (func.args) |arg| {
        var seen: SeenAliases = .empty;
        const ty: u8 = if (arg.annotation) |ann| try getType(ctx, ann, func.generics, &seen) else bc.LBC_TYPE_ANY;

        if (ty != bc.LBC_TYPE_ANY) have_non_any_param = true;

        try type_info.append(ctx.gpa, ty);
    }

    // If all parameters simplify to any, we can just omit type info for this function
    if (!have_non_any_param) return "";

    return type_info.items;
}

fn isMatchingGlobal(globals: *const an.Globals, node: *Expr, name: []const u8) bool {
    if (node.data == .global) {
        const g = node.data.global;
        return an.getGlobalState(globals, g) == .default and std.mem.eql(u8, g, name);
    }
    return false;
}

fn isMatchingGlobalMember(globals: *const an.Globals, expr: ast.IndexName, library: []const u8, member: []const u8) bool {
    if (expr.expr.data == .global) {
        const o = expr.expr.data.global;
        return an.getGlobalState(globals, o) == .default and std.mem.eql(u8, o, library) and std.mem.eql(u8, expr.index, member);
    }
    return false;
}

const AliasEntry = struct { name: []const u8, prev: ?*Stat };

const TypeMapVisitor = struct {
    gpa: Allocator,
    function_types: *FunctionTypes,
    local_types: *LocalTypes,
    expr_types: *ExprTypes,
    host_vector_type: ?[]const u8,
    userdata_types: *const UserdataTypes,
    builtin_types: *const BuiltinAstTypes,
    builtin_calls: *const an.Builtins,
    globals: *const an.Globals,
    library_member_type_cb: ?LibraryMemberTypeCallback,
    integer_fastcalls: bool,
    bytecode: *builder_mod.BytecodeBuilder,

    type_aliases: TypeAliases = .empty,
    type_alias_stack: std.ArrayList(AliasEntry) = .empty,
    resolved_locals: std.AutoHashMapUnmanaged(*ast.Local, *const AType) = .empty,
    resolved_exprs: std.AutoHashMapUnmanaged(*Expr, *const AType) = .empty,
    function_return_types: std.AutoHashMapUnmanaged(*ast.Local, *const AType) = .empty,

    fn ctx(self: *TypeMapVisitor) TypeContext {
        return .{ .gpa = self.gpa, .type_aliases = &self.type_aliases, .host_vector_type = self.host_vector_type, .userdata_types = self.userdata_types, .bytecode = self.bytecode };
    }

    fn pushTypeAliases(self: *TypeMapVisitor, block: *Stat) an.Error!usize {
        const top = self.type_alias_stack.items.len;

        for (block.data.block.body) |stat| {
            if (stat.data == .type_alias) {
                const name = stat.data.type_alias.name;
                const gop = try self.type_aliases.getOrPut(self.gpa, name);
                const prev: ?*Stat = if (gop.found_existing) gop.value_ptr.* else null;
                try self.type_alias_stack.append(self.gpa, .{ .name = name, .prev = prev });
                gop.value_ptr.* = stat;
            }
        }

        return top;
    }

    fn popTypeAliases(self: *TypeMapVisitor, top: usize) an.Error!void {
        while (self.type_alias_stack.items.len > top) {
            const e = self.type_alias_stack.pop().?;
            try self.type_aliases.put(self.gpa, e.name, e.prev);
        }
    }

    fn resolveAliases(self: *TypeMapVisitor, ty: *const AType) *const AType {
        if (ty.data == .reference) {
            const ref = ty.data.reference;
            if (ref.prefix != null or ref.synthetic) return ty;
            if (self.type_aliases.get(ref.name)) |alias_opt| {
                if (alias_opt) |alias| return alias.data.type_alias.type;
            }
        }
        return ty;
    }

    fn tryGetTableIndexer(self: *TypeMapVisitor, expr: *Expr) ?*ast.TableIndexer {
        if (self.resolved_exprs.get(expr)) |ty| {
            if (ty.data == .table) return ty.data.table.indexer;
        }
        return null;
    }

    fn recordExprType(self: *TypeMapVisitor, expr: *Expr, ty_in: *const AType) an.Error!u8 {
        const ty = self.resolveAliases(ty_in);
        try self.resolved_exprs.put(self.gpa, expr, ty);

        var seen: SeenAliases = .empty;
        const bty = try getType(self.ctx(), ty, &.{}, &seen);
        try self.expr_types.put(self.gpa, expr, bty);
        return bty;
    }

    fn recordLocalType(self: *TypeMapVisitor, local: *ast.Local, ty_in: *const AType) an.Error!u8 {
        const ty = self.resolveAliases(ty_in);
        try self.resolved_locals.put(self.gpa, local, ty);

        var seen: SeenAliases = .empty;
        const bty = try getType(self.ctx(), ty, &.{}, &seen);
        if (bty != bc.LBC_TYPE_ANY) try self.local_types.put(self.gpa, local, bty);
        return bty;
    }

    fn walk(self: *TypeMapVisitor, e: *Expr) an.Error!void {
        try an.walkExpr(TypeMapVisitor, self, e);
    }

    pub fn visitStat(self: *TypeMapVisitor, s: *Stat) an.Error!bool {
        const bt = self.builtin_types;
        switch (s.data) {
            .@"if" => |n| {
                const cl = n.condition_local orelse return true;
                if (cl.annotation != null) return true;

                try self.walk(n.condition);

                // Propagate from the value that's being assigned
                if (self.resolved_exprs.get(n.condition)) |ty| try self.resolved_locals.put(self.gpa, cl, ty);

                try an.walkStat(TypeMapVisitor, self, n.thenbody);
                if (n.elsebody) |e| try an.walkStat(TypeMapVisitor, self, e);
                return false;
            },
            .block => |b| {
                const top = try self.pushTypeAliases(s);
                for (b.body) |stat| try an.walkStat(TypeMapVisitor, self, stat);
                try self.popTypeAliases(top);
                return false;
            },
            // repeat..until scoping rules are such that condition (along with any possible functions declared in it) has aliases from repeat body in scope
            .repeat => |n| {
                const top = try self.pushTypeAliases(n.body);
                for (n.body.data.block.body) |stat| try an.walkStat(TypeMapVisitor, self, stat);
                try self.walk(n.condition);
                try self.popTypeAliases(top);
                return false;
            },
            .@"for" => |n| {
                _ = try self.recordLocalType(n.variable, &bt.number_type);
                return true; // Let generic visitor step into all expressions
            },
            // for...in statement can contain type annotations on locals (we might even infer some for ipairs/pairs/generalized iteration)
            .for_in => |n| {
                for (n.values) |v| try self.walk(v);

                // This is similar to how Compiler matches builtin iteration, but we also handle generalized iteration case
                if (n.vars.len == 2 and n.values.len == 1) {
                    const v0 = n.values[0];
                    if (v0.data == .call and v0.data.call.args.len == 1) {
                        const func = v0.data.call.func;
                        const arg = v0.data.call.args[0];

                        if (isMatchingGlobal(self.globals, func, "ipairs")) {
                            if (self.tryGetTableIndexer(arg)) |indexer| {
                                _ = try self.recordLocalType(n.vars[0], &bt.number_type);
                                _ = try self.recordLocalType(n.vars[1], indexer.result_type);
                            }
                        } else if (isMatchingGlobal(self.globals, func, "pairs")) {
                            if (self.tryGetTableIndexer(arg)) |indexer| {
                                _ = try self.recordLocalType(n.vars[0], indexer.index_type);
                                _ = try self.recordLocalType(n.vars[1], indexer.result_type);
                            }
                        }
                    } else if (self.tryGetTableIndexer(v0)) |indexer| {
                        _ = try self.recordLocalType(n.vars[0], indexer.index_type);
                        _ = try self.recordLocalType(n.vars[1], indexer.result_type);
                    }
                }

                for (n.vars) |v| {
                    if (v.annotation) |ann| _ = try self.recordLocalType(v, ann);
                }

                try an.walkStat(TypeMapVisitor, self, n.body);
                return false;
            },
            .local_function => |n| {
                if (n.func.data.function.return_annotation) |ra| {
                    if (ra.data == .explicit and ra.data.explicit.types.len >= 1)
                        try self.function_return_types.put(self.gpa, n.name, ra.data.explicit.types[0]);
                }
                return true; // Let generic visitor step into all expressions
            },
            .local => |n| {
                for (n.values) |v| try self.walk(v);

                for (n.vars, 0..) |v, i| {
                    // Propagate from the value that's being assigned
                    // This simple propagation doesn't handle type packs in tail position
                    if (v.annotation == null and i < n.values.len) {
                        if (self.resolved_exprs.get(n.values[i])) |ty| try self.resolved_locals.put(self.gpa, v, ty);
                    }
                }
                return false;
            },
            else => return true,
        }
    }

    pub fn visitExpr(self: *TypeMapVisitor, node: *Expr) an.Error!bool {
        const bt = self.builtin_types;
        switch (node.data) {
            .function => |*f| {
                const t = try getFunctionType(self.ctx(), f);
                if (t.len != 0) try self.function_types.put(self.gpa, node, t);
                return true; // Let generic visitor step into all expressions
            },
            .local => |l| {
                const local = l.local;
                if (local.annotation) |ann| {
                    const ty = try self.recordExprType(node, ann);
                    if (ty != bc.LBC_TYPE_ANY) try self.local_types.put(self.gpa, local, ty);
                } else if (self.resolved_locals.get(local)) |ty| {
                    try self.local_types.put(self.gpa, local, try self.recordExprType(node, ty));
                }
                return false;
            },
            .index_expr => |i| {
                try self.walk(i.expr);
                try self.walk(i.index);
                if (self.tryGetTableIndexer(i.expr)) |indexer| _ = try self.recordExprType(node, indexer.result_type);
                return false;
            },
            .index_name => |i| {
                try self.walk(i.expr);

                if (self.resolved_exprs.get(i.expr)) |ty| {
                    if (ty.data == .table) {
                        for (ty.data.table.props) |prop| {
                            if (std.mem.eql(u8, prop.name, i.index)) {
                                _ = try self.recordExprType(node, prop.type);
                                return false;
                            }
                        }
                    }
                }

                if (self.expr_types.get(i.expr)) |bty| {
                    if (bty == bc.LBC_TYPE_VECTOR) {
                        const idx = i.index;
                        const eq = std.mem.eql;
                        if (eq(u8, idx, "X") or eq(u8, idx, "Y") or eq(u8, idx, "Z") or eq(u8, idx, "x") or eq(u8, idx, "y") or eq(u8, idx, "z")) {
                            _ = try self.recordExprType(node, &bt.number_type);
                            return false;
                        }
                    }
                }

                if (isMatchingGlobalMember(self.globals, i, "vector", "zero") or isMatchingGlobalMember(self.globals, i, "vector", "one")) {
                    _ = try self.recordExprType(node, &bt.vector_type);
                    return false;
                }

                if (self.library_member_type_cb) |cb| {
                    if (i.expr.data == .global) {
                        const ty = cb(i.expr.data.global, i.index);
                        if (ty != bc.LBC_TYPE_ANY) {
                            // TODO: 'resolvedExprs' is more limited than 'exprTypes' which limits full inference of more complex types that a user
                            // callback can return
                            const resolved: ?*const AType = switch (ty) {
                                bc.LBC_TYPE_BOOLEAN => &bt.boolean_type,
                                bc.LBC_TYPE_NUMBER => &bt.number_type,
                                bc.LBC_TYPE_INTEGER => &bt.integer_type,
                                bc.LBC_TYPE_STRING => &bt.string_type,
                                bc.LBC_TYPE_VECTOR => &bt.vector_type,
                                else => null,
                            };
                            if (resolved) |r| try self.resolved_exprs.put(self.gpa, node, r);

                            try self.expr_types.put(self.gpa, node, ty);
                            return false;
                        }
                    }
                }

                return false;
            },
            .unary => |u| {
                try self.walk(u.expr);

                switch (u.op) {
                    .not => _ = try self.recordExprType(node, &bt.boolean_type),
                    .minus => {
                        const ty = self.resolved_exprs.get(u.expr) orelse return false;
                        const bty = self.expr_types.get(u.expr) orelse return false;
                        if (bty == bc.LBC_TYPE_VECTOR or bty == bc.LBC_TYPE_NUMBER) _ = try self.recordExprType(node, ty);
                    },
                    .len => _ = try self.recordExprType(node, &bt.number_type),
                }
                return false;
            },
            .binary => |b| {
                try self.walk(b.left);
                try self.walk(b.right);

                // Comparisons result in a boolean
                switch (b.op) {
                    .compare_ne, .compare_eq, .compare_lt, .compare_le, .compare_gt, .compare_ge => {
                        _ = try self.recordExprType(node, &bt.boolean_type);
                        return false;
                    },
                    .concat, .@"and", .@"or" => return false,
                    else => {},
                }

                const left_ty = self.resolved_exprs.get(b.left) orelse return false;
                const left_bty = self.expr_types.get(b.left) orelse return false;
                const right_ty = self.resolved_exprs.get(b.right) orelse return false;
                const right_bty = self.expr_types.get(b.right) orelse return false;

                if (left_bty == bc.LBC_TYPE_VECTOR) {
                    _ = try self.recordExprType(node, left_ty);
                } else if (right_bty == bc.LBC_TYPE_VECTOR) {
                    _ = try self.recordExprType(node, right_ty);
                } else if (left_bty == bc.LBC_TYPE_NUMBER and right_bty == bc.LBC_TYPE_NUMBER) {
                    _ = try self.recordExprType(node, left_ty);
                }
                return false;
            },
            .group => |g| {
                try self.walk(g);
                if (self.resolved_exprs.get(g)) |ty| _ = try self.recordExprType(node, ty);
                return false;
            },
            .type_assertion => |t| {
                try self.walk(t.expr);
                _ = try self.recordExprType(node, t.annotation);
                return false;
            },
            .constant_bool => {
                _ = try self.recordExprType(node, &bt.boolean_type);
                return false;
            },
            .constant_number => {
                _ = try self.recordExprType(node, &bt.number_type);
                return false;
            },
            .constant_integer => {
                _ = try self.recordExprType(node, &bt.integer_type);
                return false;
            },
            .constant_string, .interp_string => {
                _ = try self.recordExprType(node, &bt.string_type);
                return false;
            },
            .if_else => |ie| {
                try self.walk(ie.condition);

                // propagate the condition's resolved type to the binding (unless annotated)
                if (ie.condition_local) |cl| {
                    if (cl.annotation == null) {
                        if (self.resolved_exprs.get(ie.condition)) |ty| try self.resolved_locals.put(self.gpa, cl, ty);
                    }
                }

                try self.walk(ie.true_expr);
                try self.walk(ie.false_expr);

                const true_ty = self.resolved_exprs.get(ie.true_expr);
                const true_bty = self.expr_types.get(ie.true_expr);
                const false_bty = self.expr_types.get(ie.false_expr);

                // Optimistic check that both expressions are of the same kind, as AstType* cannot be compared
                if (true_ty != null and true_bty != null and false_bty != null and true_bty.? == false_bty.?) _ = try self.recordExprType(node, true_ty.?);
                return false;
            },
            .call => |c| {
                if (self.builtin_calls.get(node)) |bfid| {
                    const kind: ?*const AType = switch (bfid) {
                        .MATH_ABS, .MATH_ACOS, .MATH_ASIN, .MATH_ATAN2, .MATH_ATAN, .MATH_CEIL, .MATH_COSH, .MATH_COS, .MATH_DEG, .MATH_EXP, .MATH_FLOOR, .MATH_FMOD, .MATH_FREXP, .MATH_LDEXP, .MATH_LOG10, .MATH_LOG, .MATH_MAX, .MATH_MIN, .MATH_MODF, .MATH_POW, .MATH_RAD, .MATH_SINH, .MATH_SIN, .MATH_SQRT, .MATH_TANH, .MATH_TAN, .BIT32_ARSHIFT, .BIT32_BAND, .BIT32_BNOT, .BIT32_BOR, .BIT32_BXOR, .BIT32_BTEST, .BIT32_EXTRACT, .BIT32_LROTATE, .BIT32_LSHIFT, .BIT32_REPLACE, .BIT32_RROTATE, .BIT32_RSHIFT, .STRING_BYTE, .STRING_LEN, .MATH_CLAMP, .MATH_SIGN, .MATH_ROUND, .BIT32_COUNTLZ, .BIT32_COUNTRZ, .RAWLEN, .BIT32_EXTRACTK, .TONUMBER, .BIT32_BYTESWAP, .BUFFER_READI8, .BUFFER_READU8, .BUFFER_READI16, .BUFFER_READU16, .BUFFER_READI32, .BUFFER_READU32, .BUFFER_READF32, .BUFFER_READF64, .VECTOR_MAGNITUDE, .VECTOR_DOT, .MATH_LERP => &bt.number_type,
                        .TYPE, .STRING_CHAR, .TYPEOF, .STRING_SUB, .TOSTRING => &bt.string_type,
                        .MATH_ISNAN, .MATH_ISINF, .MATH_ISFINITE, .RAWEQUAL => &bt.boolean_type,
                        .VECTOR, .VECTOR_NORMALIZE, .VECTOR_CROSS, .VECTOR_FLOOR, .VECTOR_CEIL, .VECTOR_ABS, .VECTOR_SIGN, .VECTOR_CLAMP, .VECTOR_MIN, .VECTOR_MAX, .VECTOR_LERP => &bt.vector_type,
                        // integer builtins are only typed with LuauIntegerFastcalls; without it upstream returns before recording
                        .INTEGER_ADD, .INTEGER_SUB, .INTEGER_MOD, .INTEGER_MUL, .INTEGER_DIV, .INTEGER_IDIV, .INTEGER_UDIV, .INTEGER_REM, .INTEGER_UREM, .INTEGER_MAX, .INTEGER_MIN, .INTEGER_BAND, .INTEGER_BOR, .INTEGER_BNOT, .INTEGER_BXOR, .INTEGER_LSHIFT, .INTEGER_RSHIFT, .INTEGER_ARSHIFT, .INTEGER_LROTATE, .INTEGER_RROTATE, .INTEGER_EXTRACT, .INTEGER_COUNTLZ, .INTEGER_COUNTRZ, .INTEGER_BSWAP, .INTEGER_CLAMP, .INTEGER_NEG, .INTEGER_CREATE, .BUFFER_READINTEGER => if (self.integer_fastcalls) &bt.integer_type else return true,
                        .INTEGER_TONUMBER => if (self.integer_fastcalls) &bt.number_type else return true,
                        .INTEGER_LT, .INTEGER_LE, .INTEGER_GT, .INTEGER_GE, .INTEGER_ULT, .INTEGER_ULE, .INTEGER_UGT, .INTEGER_UGE, .INTEGER_BTEST => if (self.integer_fastcalls) &bt.boolean_type else return true,
                        else => null,
                    };
                    if (kind) |k| _ = try self.recordExprType(node, k);
                } else {
                    if (c.func.data == .local) {
                        if (self.function_return_types.get(c.func.data.local.local)) |ty| _ = try self.recordExprType(node, ty);
                    }
                }
                return true; // Let generic visitor step into all expressions
            },
            else => return true,
        }
    }
};

pub const TypeMapOptions = struct {
    host_vector_type: ?[]const u8,
    userdata_types: *const UserdataTypes,
    builtin_types: *const BuiltinAstTypes,
    builtin_calls: *const an.Builtins,
    globals: *const an.Globals,
    library_member_type_cb: ?LibraryMemberTypeCallback,
    flags: flags_mod.Flags = .{},
};

pub fn buildTypeMap(gpa: Allocator, function_types: *FunctionTypes, local_types: *LocalTypes, expr_types: *ExprTypes, root: *Stat, options: TypeMapOptions, bytecode: *builder_mod.BytecodeBuilder) an.Error!void {
    var visitor: TypeMapVisitor = .{
        .gpa = gpa,
        .function_types = function_types,
        .local_types = local_types,
        .expr_types = expr_types,
        .host_vector_type = options.host_vector_type,
        .userdata_types = options.userdata_types,
        .builtin_types = options.builtin_types,
        .builtin_calls = options.builtin_calls,
        .globals = options.globals,
        .library_member_type_cb = options.library_member_type_cb,
        .integer_fastcalls = options.flags.LuauIntegerFastcalls,
        .bytecode = bytecode,
    };
    try an.walkStat(TypeMapVisitor, &visitor, root);
}
