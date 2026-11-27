//! Inlining / loop unrolling cost model.
//!
//! Translated from third_party/luau/Compiler/src/CostModel.cpp (Luau, MIT
//! License; see third_party/luau/LICENSE.txt).

const std = @import("std");
const Allocator = std.mem.Allocator;
const ast = @import("ast.zig");
const an = @import("analysis.zig");
const Expr = ast.Expr;
const Stat = ast.Stat;

fn parallelAddSat(x: u64, y: u64) u64 {
    const r = x +% y;
    const s = r & 0x8080808080808080; // saturation mask
    return (r ^ s) | (s -% (s >> 7));
}

fn parallelMulSat(a: u64, b: i32) u64 {
    const bs: u64 = @intCast(if (b < 127) b else 127);

    // multiply every other value by b, yielding 14-bit products
    const l = bs *% ((a >> 0) & 0x007f007f007f007f);
    const h = bs *% ((a >> 8) & 0x007f007f007f007f);

    // each product is 14-bit, so adding 32768-128 sets high bit iff the sum is 128 or larger without an overflow
    const ls = l +% 0x7f807f807f807f80;
    const hs = h +% 0x7f807f807f807f80;

    // we now merge saturation bits as well as low 7-bits of each product into one
    const s = (hs & 0x8000800080008000) | ((ls & 0x8000800080008000) >> 8);
    const r = ((h & 0x007f007f007f007f) << 8) | (l & 0x007f007f007f007f);

    // the low bits are now correct for values that didn't saturate, and we simply need to mask them if high bit is 1
    return r | (s -% (s >> 7));
}

const Cost = struct {
    const kLiteral: u64 = ~@as(u64, 0);

    // cost model: 8 bytes, where first byte is the baseline cost, and the next 7 bytes are discounts for when variable #i is constant
    model: u64 = 0,
    // constant mask: 8-byte 0xff mask; equal to all ff's for literals, for variables only byte #i (1+) is set to align with model
    constant: u64 = 0,

    fn init(cost: i32, constant: u64) Cost {
        // cost is never negative in this pass
        return .{ .model = @intCast(if (cost < 0x7f) cost else 0x7f), .constant = constant };
    }

    fn of(cost: i32) Cost {
        return init(cost, 0);
    }

    fn add(self: Cost, other: Cost) Cost {
        return .{ .model = parallelAddSat(self.model, other.model) };
    }

    fn addAssign(self: *Cost, other: Cost) void {
        self.model = parallelAddSat(self.model, other.model);
        self.constant = 0;
    }

    fn mul(self: Cost, other: i32) Cost {
        return .{ .model = parallelMulSat(self.model, other) };
    }

    fn fold(x: Cost, y: Cost) Cost {
        const newmodel = parallelAddSat(x.model, y.model);
        const newconstant = x.constant & y.constant;

        // the extra cost for folding is 1; the discount is 1 for the variable that is shared by x&y (or whichever one is used in x/y if the other is
        // literal)
        const extra: u64 = if (newconstant == kLiteral) 0 else (1 | (0x0101010101010101 & newconstant));

        return .{ .model = parallelAddSat(newmodel, extra), .constant = newconstant };
    }
};

const CostVisitor = struct {
    gpa: Allocator,
    builtins: *const an.Builtins,
    constants: *const an.Constants,
    vars: std.AutoHashMapUnmanaged(*ast.Local, u64) = .empty,
    result: Cost = .{},

    fn model(self: *CostVisitor, node: *Expr) an.Error!Cost {
        if (self.constants.contains(node)) return Cost.init(0, Cost.kLiteral);

        switch (node.data) {
            .group => |g| return (try self.model(g)),
            .constant_nil, .constant_bool, .constant_number, .constant_string, .constant_integer => return Cost.init(0, Cost.kLiteral),
            .local => |l| {
                // locals typically don't require extra instructions to compute
                return Cost.init(0, self.vars.get(l.local) orelse 0);
            },
            .global => return Cost.of(1),
            .varargs => return Cost.of(3),
            .call => |c| {
                // builtin cost modeling is different from regular calls because we use FASTCALL to compile these
                // thus we use a cheaper baseline, don't account for function, and assume constant/local copy is free
                const bfid = self.builtins.get(node);
                const builtin = bfid != null and bfid.? != .NONE;
                const builtin_short = builtin and c.args.len <= 3; // FASTCALL1/2/3

                var cost = Cost.of(if (builtin) 2 else 3);

                if (!builtin) cost.addAssign((try self.model(c.func)));

                for (c.args) |arg| {
                    const ac = (try self.model(arg));
                    // for constants/locals we still need to copy them to the argument list
                    cost.addAssign(if (ac.model == 0 and !builtin_short) Cost.of(1) else ac);
                }

                return cost;
            },
            .index_name => |i| return (try self.model(i.expr)).add(Cost.of(1)),
            .index_expr => |i| return (try self.model(i.expr)).add((try self.model(i.index))).add(Cost.of(1)),
            .function => return Cost.of(10), // high baseline cost due to allocation
            .table => |items| {
                var cost = Cost.of(10); // high baseline cost due to allocation
                for (items) |item| {
                    if (item.key) |k| cost.addAssign((try self.model(k)));
                    cost.addAssign((try self.model(item.value)));
                    cost.addAssign(Cost.of(1));
                }
                return cost;
            },
            .unary => |u| return Cost.fold((try self.model(u.expr)), Cost.init(0, Cost.kLiteral)),
            .binary => |b| return Cost.fold((try self.model(b.left)), (try self.model(b.right))),
            .type_assertion => |t| return (try self.model(t.expr)),
            .if_else => |ie| {
                const cond = (try self.model(ie.condition));

                // propagate constant mask from condition to the local variable if it exists
                if (ie.condition_local) |cl| {
                    if (cond.constant != 0) try self.vars.put(self.gpa, cl, cond.constant);
                }

                return cond.add((try self.model(ie.true_expr))).add((try self.model(ie.false_expr))).add(Cost.of(2));
            },
            .interp_string => |is| {
                // Baseline cost of string.format
                var cost = Cost.of(3);
                for (is.expressions) |e| cost.addAssign((try self.model(e)));
                return cost;
            },
            .instantiate => |i| return (try self.model(i.expr)),
        }
    }

    fn assign(self: *CostVisitor, expr: *Expr) void {
        // variable assignments reset variable mask, so that further uses of this variable aren't discounted
        // this doesn't work perfectly with backwards control flow like loops, but is good enough for a single pass
        if (expr.data == .local) {
            if (self.vars.getPtr(expr.data.local.local)) |i| i.* = 0;
        }
    }

    fn loop(self: *CostVisitor, body: *Stat, iter_cost: Cost, factor: i32) an.Error!void {
        const before = self.result;

        self.result = .{};
        try an.walkStat(CostVisitor, self, body);

        self.result = before.add(self.result.add(iter_cost).mul(factor));
    }

    fn getNumber(self: *CostVisitor, node: *Expr) ?f64 {
        if (self.constants.get(node)) |c| {
            if (c == .number) return c.number;
        }
        return null;
    }

    pub fn visitExpr(self: *CostVisitor, e: *Expr) an.Error!bool {
        // note: we short-circuit the visitor traversal through any expression trees by returning false
        // recursive traversal is happening inside model() which makes it easier to get the resulting value of the subexpression
        self.result.addAssign((try self.model(e)));
        return false;
    }

    pub fn visitStat(self: *CostVisitor, s: *Stat) an.Error!bool {
        switch (s.data) {
            .@"for" => |n| {
                self.result.addAssign((try self.model(n.from)));
                self.result.addAssign((try self.model(n.to)));
                if (n.step) |st| self.result.addAssign((try self.model(st)));

                var trip_count: i32 = -1;
                const from = self.getNumber(n.from);
                const to = self.getNumber(n.to);
                const step: ?f64 = if (n.step) |st| self.getNumber(st) else 1.0;

                if (from != null and to != null and step != null) trip_count = getTripCount(from.?, to.?, step.?);

                try self.loop(n.body, Cost.of(1), if (trip_count < 0) 3 else trip_count);
                return false;
            },
            .for_in => |n| {
                for (n.values) |v| self.result.addAssign((try self.model(v)));
                try self.loop(n.body, Cost.of(1), 3);
                return false;
            },
            .@"while" => |n| {
                const condition = (try self.model(n.condition));
                try self.loop(n.body, condition, 3);
                return false;
            },
            .repeat => |n| {
                const condition = (try self.model(n.condition));
                try self.loop(n.body, condition, 3);
                return false;
            },
            .@"if" => |n| {
                if (n.condition_local) |cl| {
                    const arg = (try self.model(n.condition));
                    if (arg.constant != 0) try self.vars.put(self.gpa, cl, arg.constant);
                }

                if (an.isConstantFalse(self.constants, n.condition)) {
                    if (n.elsebody) |e| try an.walkStat(CostVisitor, self, e);
                    return false;
                }

                if (an.isConstantTrue(self.constants, n.condition)) {
                    try an.walkStat(CostVisitor, self, n.thenbody);
                    return false;
                }

                // unconditional 'else' may require a jump after the 'if' body
                // note: this ignores cases when 'then' always terminates and also assumes comparison requires an extra instruction which may be false
                const else_jump: i32 = if (n.elsebody != null and n.elsebody.?.data != .@"if") 1 else 0;
                self.result.addAssign(Cost.of(1 + else_jump));

                return true;
            },
            .local => |n| {
                for (n.values, 0..) |v, i| {
                    const arg = (try self.model(v));

                    // propagate constant mask from expression through variables
                    if (arg.constant != 0 and i < n.vars.len) try self.vars.put(self.gpa, n.vars[i], arg.constant);

                    self.result.addAssign(arg);
                }
                return false;
            },
            .assign => |n| {
                for (n.vars) |v| self.assign(v);

                var i: usize = 0;
                while (i < n.vars.len or i < n.values.len) : (i += 1) {
                    var ac: Cost = .{};
                    if (i < n.vars.len) ac.addAssign((try self.model(n.vars[i])));
                    if (i < n.values.len) ac.addAssign((try self.model(n.values[i])));
                    // local->local or constant->local assignment is not free
                    self.result.addAssign(if (ac.model == 0) Cost.of(1) else ac);
                }
                return false;
            },
            .compound_assign => |n| {
                self.assign(n.variable);

                // if lhs is not a local, setting it requires an extra table operation
                self.result.addAssign(Cost.of(if (n.variable.data == .local) 1 else 2));

                return true;
            },
            .@"break", .@"continue" => {
                self.result.addAssign(Cost.of(1));
                return false;
            },
            .block => |b| {
                for (b.body) |stat| {
                    try an.walkStat(CostVisitor, self, stat);
                    if (an.alwaysTerminates(self.constants, stat)) break;
                }
                return false;
            },
            else => return true,
        }
    }
};

pub fn modelCost(gpa: Allocator, root: *Stat, vars: []const *ast.Local, builtins: *const an.Builtins, constants: *const an.Constants) an.Error!u64 {
    var visitor: CostVisitor = .{ .gpa = gpa, .builtins = builtins, .constants = constants };
    for (vars, 0..) |v, i| {
        if (i >= 7) break;
        try visitor.vars.put(gpa, v, @as(u64, 0xff) << @intCast(i * 8 + 8));
    }

    try an.walkStat(CostVisitor, &visitor, root);

    return visitor.result.model;
}

pub fn computeCost(model: u64, vars_const: []const bool) i32 {
    var cost: i32 = @intCast(model & 0x7f);

    // don't apply discounts to what is likely a saturated sum
    if (cost == 0x7f) return cost;

    for (vars_const, 0..) |c, i| {
        if (i >= 7) break;
        cost -= @as(i32, @intCast((model >> @intCast(i * 8 + 8)) & 0x7f)) * @intFromBool(c);
    }

    return cost;
}

pub fn getTripCount(from: f64, to: f64, step: f64) i32 {
    // we compute trip count in integers because that way we know that the loop math (repeated addition) is precise
    const fromi = smallInt(from) orelse return -1;
    const toi = smallInt(to) orelse return -1;
    const stepi = smallInt(step) orelse return -1;

    if (stepi == 0) return -1;

    if ((stepi < 0 and toi > fromi) or (stepi > 0 and toi < fromi)) return 0;

    return @divTrunc(toi - fromi, stepi) + 1;
}

fn smallInt(v: f64) ?i32 {
    if (v >= -32767 and v <= 32767 and @as(f64, @floatFromInt(@as(i32, @intFromFloat(v)))) == v) return @intFromFloat(v);
    return null;
}
