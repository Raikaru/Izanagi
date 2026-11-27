//! Luau bytecode code generator.
//!
//! Translated from third_party/luau/Compiler/src/Compiler.cpp (Luau, MIT
//! License; see third_party/luau/LICENSE.txt) with every upstream feature flag
//! at its default (off) value and the default FInt tuning values
//! (LuauCompileLoopUnrollThreshold 25 / MaxBoost 300, LuauCompileInlineThreshold
//! 25 / MaxBoost 300, LuauCompileInlineDepth 5). All optimization levels
//! (0, 1, 2 including inlining, loop unrolling and builtin constant folding),
//! type information emission and native/coverage metadata are implemented.
//! Debug remarks (BytecodeBuilder::addDebugRemark) are only used for dumps and
//! are not produced.
//!
//! Upstream methods of `struct Compiler` are file-level functions taking
//! `self: *Compiler` here; RegScope objects are expressed with `defer`.

const std = @import("std");
const Allocator = std.mem.Allocator;
const ast = @import("ast.zig");
const bc = @import("bytecode.zig");
const lex = @import("lexer.zig");
const an = @import("analysis.zig");
const builder_mod = @import("builder.zig");
const BytecodeBuilder = builder_mod.BytecodeBuilder;
const Op = bc.Opcode;
const Expr = ast.Expr;
const Stat = ast.Stat;
const Location = lex.Location;
const Constant = an.Constant;
const types = @import("types.zig");
const cost_model = @import("cost_model.zig");
const builtin_info = @import("builtin_folding.zig");
const flags_mod = @import("flags.zig");

pub const Error = an.Error;

const kMaxRegisterCount = builder_mod.kMaxRegisterCount;
const kMaxUpvalueCount = builder_mod.kMaxUpvalueCount;
const kMaxLocalCount = builder_mod.kMaxLocalCount;
const kMaxInstructionCount = builder_mod.kMaxInstructionCount;
const kInvalidReg = builder_mod.kInvalidReg;
const kDefaultAllocPc: u32 = ~@as(u32, 0);

// FInt defaults from Compiler.cpp
const kLoopUnrollThreshold: i32 = 25;
const kLoopUnrollThresholdMaxBoost: i32 = 300;
const kInlineThreshold: i32 = 25;
const kInlineThresholdMaxBoost: i32 = 300;
const kInlineDepth: usize = 5;

/// Mirrors upstream Luau::CompileOptions.
pub const Options = struct {
    /// 0 - no optimization; 1 - baseline optimization level that doesn't prevent debuggability;
    /// 2 - includes optimizations that harm debuggability such as inlining
    optimization_level: u8 = 1,
    /// 0 - no debugging support; 1 - line info & function names only; 2 - full debug info with local & upvalue names
    debug_level: u8 = 1,
    /// 0 - generate for native modules; 1 - generate for all modules
    type_info_level: u8 = 0,
    /// 0 - no code coverage support; 1 - statement coverage; 2 - statement and expression coverage (verbose)
    coverage_level: u8 = 0,
    /// alternative global builtin to construct vectors, in addition to default builtin 'vector.create'
    vector_lib: ?[]const u8 = null,
    vector_ctor: ?[]const u8 = null,
    /// alternative vector type name for type tables, in addition to default type 'vector'
    vector_type: ?[]const u8 = null,
    /// 0 - 32-bit float vector components; 1 - 64-bit double vector components
    vector_precision: u8 = 0,
    /// globals that are mutable; disables the import optimization for fields accessed through these
    mutable_globals: []const []const u8 = &.{},
    /// userdata types that will be included in the type information
    userdata_types: []const []const u8 = &.{},
    /// globals which act as libraries and have members with known type and/or constant value
    libraries_with_known_members: []const []const u8 = &.{},
    library_member_type_cb: ?types.LibraryMemberTypeCallback = null,
    library_member_constant_cb: ?an.LibraryMemberConstantCallback = null,
    /// library functions that should not be compiled into a built-in fastcall ("name" "lib.name")
    disabled_builtins: []const []const u8 = &.{},
    /// upstream feature flags (all off by default, like the pinned upstream build)
    flags: flags_mod.Flags = .{},
};

pub const Diagnostic = struct {
    location: Location = .{},
    message: []const u8 = "",
};

const ReturnKind = enum {
    none, // No returns in the function
    expression, // Function returns some expression (could be a constant)
    parameter, // Function returns one of its parameters
    local, // Function returns one of its locals
    unknown, // Function has multiple conflicting return kinds or return targets (for Parameter/Local)
};

const FunctionInfo = struct {
    id: u32,
    upvals: []*ast.Local,
    cost_model: u64 = 0,
    stack_size: u32 = 0,
    can_inline: bool = false,
    returns_one: bool = false,
    // LuauCompileMoveElision
    return_kind: ReturnKind = .none,
    return_local: ?*ast.Local = null, // For Parameter/Local ReturnKind
    return_local_captured: bool = false,
};

const InlineFrame = struct {
    func: *Expr,
    local_offset: usize,
    target: u8,
    target_count: u8,
    /// LuauCompileMoveElision: local whose declaration can be computed directly into the inline target
    result_local: ?*ast.Local = null,
    return_jumps: std.ArrayList(usize) = .empty,
};

const InlineArg = struct {
    local: *ast.Local,
    reg: u8,
    value: Constant,
    allocpc: u32 = kDefaultAllocPc,
    init: ?*Expr = null,
};

const LocalInfo = struct {
    reg: u8 = 0,
    allocated: bool = false,
    captured: bool = false,
    reused: bool = false, // LuauCompileReuseLocalRegs
    debugpc: u32 = 0,
    allocpc: u32 = 0,
};

/// LuauCompileReuseLocalRegs: register of a dead local that can be given to a new local
const FreeLocal = struct {
    reg: u8 = 0,
    local: LocalInfo,
    ast_local: *ast.Local,
    is_argument: bool = false,
};

const LoopJump = struct {
    type: enum { brk, cont },
    label: usize,
};

const Loop = struct {
    local_offset: usize,
    local_offset_continue: usize,
    continue_used: ?*Stat,
};

const Capture = struct {
    type: u8,
    data: u8,
};

const LValueKind = enum { local, upvalue, global, index_name, index_number, index_expr, private_field };

/// DebugLuauUserDefinedClasses private member slot
const PrivateField = struct { offset: u8, is_static: bool };
const PrivateOffsets = std.StringHashMapUnmanaged(PrivateField);

const LValue = struct {
    kind: LValueKind,
    reg: u8 = 0, // register for local (Local) or table (Index*)
    upval: u8 = 0,
    index: u8 = 0, // register for index in IndexExpr
    number: u8 = 0, // index-1 (0-255) in IndexNumber
    name: []const u8 = "", // field name for IndexName and PrivateField
    location: Location = .{},
};

pub const Compiler = struct {
    gpa: Allocator,
    bytecode: *BytecodeBuilder,
    options: Options,
    names: *lex.NameTable,
    diag: *Diagnostic,

    functions: std.AutoHashMapUnmanaged(*Expr, FunctionInfo) = .empty,
    locals: std.AutoHashMapUnmanaged(*ast.Local, LocalInfo) = .empty,
    globals: an.Globals = .empty,
    variables: an.Variables = .empty,
    constants: an.Constants = .empty,
    locstants: an.LocalConstants = .empty,
    table_constants: an.TableConstants = .empty,
    table_shapes: an.TableShapes = .empty,
    builtins: an.Builtins = .empty,
    user_data_types: types.UserdataTypes = .empty,
    function_types: types.FunctionTypes = .empty,
    local_types: types.LocalTypes = .empty,
    expr_types: types.ExprTypes = .empty,
    builtin_types: types.BuiltinAstTypes = .{},

    inline_builtins: an.Builtins = .empty,
    inline_builtins_backup: an.Builtins = .empty,

    expr_changes: an.ExprChangeLog = .empty,
    local_changes: an.LocalChangeLog = .empty,

    builtins_fold: ?*const an.Builtins = null,
    builtins_fold_library_k: bool = false,

    // compileFunction state, gets reset for every function
    reg_top: u32 = 0,
    stack_size: u32 = 0,
    arg_count: usize = 0,
    has_loops: bool = false,
    has_multi_ret: bool = false,
    current_function: ?*Expr = null,
    /// LuauExportValueSyntax: number of nested blocks being compiled in the current function
    block_depth: usize = 0,
    exports: Exports,

    /// DebugLuauUserDefinedClasses: maps functions to the class they are enclosed in (if any), used for private field access
    enclosing_classes: std.AutoHashMapUnmanaged(*Expr, ?*Stat) = .empty,
    /// DebugLuauUserDefinedClasses: private field offsets of each class with private members
    class_private_field_offsets: std.AutoHashMapUnmanaged(*Stat, PrivateOffsets) = .empty,
    curr_class: ?*Stat = null,

    getfenv_used: bool = false,
    setfenv_used: bool = false,

    local_stack: std.ArrayList(*ast.Local) = .empty,
    upvals: std.ArrayList(*ast.Local) = .empty,
    loop_jumps: std.ArrayList(LoopJump) = .empty,
    loops: std.ArrayList(Loop) = .empty,
    inline_frames: std.ArrayList(InlineFrame) = .empty,
    captures: std.ArrayList(Capture) = .empty,

    /// LuauCompileMoveElision: registers holding captured locals
    reg_captured: [256]bool = @splat(false),
    /// LuauCompileReuseLocalRegs: number of live locals per register and dead-local registers available for reuse (sorted by register)
    local_uses: [256]u32 = @splat(0),
    free_local_regs: std.ArrayList(FreeLocal) = .empty,
    /// LuauCompileReuseLowersLimit: number of allocated locals (reused registers are not counted)
    active_locals: usize = 0,
};

const Exports = struct {
    /// synthetic local ("__EXP") holding the module export table
    table_local: ast.Local,
    /// exported class locals and the register holding the class (DebugLuauUserDefinedClasses)
    classes: std.AutoArrayHashMapUnmanaged(*ast.Local, u8) = .empty,
    /// exported local functions / variables (LuauOptimizeExportTable)
    exported: an.ExportedLocals = .{},
    table_cid: i32 = -1,
    has_exports: bool = false,
    has_type_exports: bool = false,
    has_module_return: bool = false,

    fn isEmpty(self: *const Exports) bool {
        return !self.has_exports and self.classes.count() == 0 and self.exported.functions.count() == 0 and self.exported.variables.items.len == 0;
    }
};

// ------------------------------------------------------------------ helpers

fn raise(self: *Compiler, location: Location, comptime fmt: []const u8, args: anytype) Error {
    const message = std.fmt.allocPrint(self.gpa, fmt, args) catch |e| return e;
    self.diag.* = .{ .location = location, .message = message };
    return error.CompileError;
}

fn constantLimit(self: *Compiler, location: Location) Error {
    return raise(self, location, "Exceeded constant limit; simplify the code to compile", .{});
}

fn checkedConstant(self: *Compiler, cid: i32, location: Location) Error!i32 {
    if (cid < 0) return constantLimit(self, location);
    return cid;
}

fn addString(self: *Compiler, s: []const u8, location: Location) Error!i32 {
    return checkedConstant(self, try self.bytecode.addConstantString(s), location);
}

inline fn hash8(s: []const u8) u8 {
    return @truncate(BytecodeBuilder.getStringHash(s));
}

// ------------------------------------------------------------------ exports (LuauExportValueSyntax)

fn atTopLevel(self: *const Compiler) bool {
    const cf = self.current_function orelse return false;
    return cf.data.function.function_depth == 0 and self.block_depth == 0 and self.loops.items.len == 0;
}

fn checkExportedLocal(self: *Compiler, local: *ast.Local, location: Location) Error!void {
    if (local.is_exported and !atTopLevel(self)) {
        // We can catch some non top-level usages in the parser, but for others, like in loops, we also catch them here
        return raise(self, location, "'export' may only be applied to top-level statements", .{});
    }
}

fn ensureExportTable(self: *Compiler, location: Location) Error!void {
    self.exports.has_exports = true;

    if (self.locals.contains(&self.exports.table_local)) return;

    std.debug.assert(atTopLevel(self));

    const table_reg = try allocReg(self, location, 1);
    if (self.options.flags.LuauOptimizeExportTable and self.exports.table_cid != -1 and self.exports.table_cid < 32768) {
        try self.bytecode.emitAD(.DUPTABLE, table_reg, @intCast(self.exports.table_cid));
    } else {
        try self.bytecode.emitABC(.NEWTABLE, table_reg, encodeHashSize(0), 0);
        try self.bytecode.emitAux(0);
    }

    try pushLocal(self, &self.exports.table_local, table_reg, kDefaultAllocPc);
}

fn getExportTableReg(self: *Compiler, location: Location) Error!u8 {
    if (getLocalReg(self, &self.exports.table_local)) |reg| return reg;

    const upval = try getUpval(self, &self.exports.table_local);
    const r = try allocReg(self, location, 1);
    try self.bytecode.emitABC(.GETUPVAL, r, upval, 0);
    return r;
}

fn buildExportTableShape(self: *Compiler) Error!void {
    std.debug.assert(self.options.flags.LuauOptimizeExportTable);

    var shape: builder_mod.TableShape = .{};

    if (self.exports.exported.variables.items.len >= builder_mod.TableShape.kMaxLength) return;

    for (self.exports.exported.variables.items) |local| {
        const variable = self.variables.get(local) orelse
            return raise(self, local.location, "Local does not have corresponding variable", .{});

        const key_cid = try self.bytecode.addConstantString(local.name);
        // typically we throw a compiler error, but optimistically we can just not build a shape preemptively
        if (key_cid < 0) return;

        const idx = shape.length;
        shape.keys[idx] = key_cid;
        shape.constants[idx] = -1;
        if (variable.constant and !variable.written) {
            const value_cid = try getConstantIndex(self, variable.init.?);
            // If the constant pool is full we can't embed this value in the shape
            // Bail out so the table cid stays -1 and compileStatLocal falls back to SETTABLEKS
            if (value_cid < 0) return;

            shape.constants[idx] = value_cid;
            shape.has_constants = true;
        }
        shape.length += 1;
    }

    if (shape.length > 0) self.exports.table_cid = try self.bytecode.addConstantTable(shape);
}

fn compileExportTable(self: *Compiler) Error!void {
    const loc_node = self.current_function.?;
    const location = loc_node.location;
    var table_reg: u8 = undefined;

    if (!self.exports.isEmpty()) {
        // this arises when we have a module that is only exporting classes
        try ensureExportTable(self, location);

        table_reg = getLocalReg(self, &self.exports.table_local).?;

        if (self.options.flags.DebugLuauUserDefinedClasses) {
            var it = self.exports.classes.iterator();
            while (it.next()) |entry| {
                const class_local = entry.key_ptr.*;
                const class_name_cid = try addString(self, class_local.name, location);

                try self.bytecode.emitABC(.SETTABLEKS, entry.value_ptr.*, table_reg, hash8(class_local.name));
                try self.bytecode.emitAux(@bitCast(class_name_cid));
            }
        }

        if (self.options.flags.LuauOptimizeExportTable) {
            for (self.exports.exported.functions.keys()) |func_local| {
                const cid = try addString(self, func_local.name, func_local.location);

                const func_reg = getLocalReg(self, func_local).?;

                try self.bytecode.emitABC(.SETTABLEKS, func_reg, table_reg, hash8(func_local.name));
                try self.bytecode.emitAux(@bitCast(cid));
            }
        }
    } else {
        // Type-only exports with no return: emit a fresh empty table
        std.debug.assert(self.options.flags.LuauExportedTypesParticipateInScc);
        std.debug.assert(self.exports.has_type_exports);

        table_reg = try allocReg(self, location, 1);
        try self.bytecode.emitABC(.NEWTABLE, table_reg, encodeHashSize(0), 0);
        try self.bytecode.emitAux(0);
    }

    const freeze_reg = try allocReg(self, location, 2);
    const freeze_name = try self.names.getOrAdd("freeze");
    const freeze_cid = try addString(self, freeze_name, location);

    const table_name = try self.names.getOrAdd("table");
    const table_cid = try addString(self, table_name, location);

    // Note: GETIMPORT encoding is limited to 10 bits per object id component
    // otherwise we can fallback to getglobal
    var get_global_fallback = true;
    if (table_cid < 1024 and freeze_cid < 1024) {
        const iid = BytecodeBuilder.getImportId2(table_cid, freeze_cid);
        const cid = try self.bytecode.addImport(iid);
        if (cid >= 0 and cid < 32768) {
            try self.bytecode.emitAD(.GETIMPORT, freeze_reg, @intCast(cid));
            try self.bytecode.emitAux(iid);
            get_global_fallback = false;
        }
    }

    if (get_global_fallback) {
        try self.bytecode.emitABC(.GETGLOBAL, freeze_reg, 0, hash8(table_name));
        try self.bytecode.emitAux(@bitCast(table_cid));

        try self.bytecode.emitABC(.GETTABLEKS, freeze_reg, freeze_reg, hash8(freeze_name));
        try self.bytecode.emitAux(@bitCast(freeze_cid));
    }

    try self.bytecode.emitABC(.MOVE, freeze_reg + 1, table_reg, 0);
    try self.bytecode.emitABC(.CALL, freeze_reg, 2, 2);

    try closeLocals(self, 0);
    try self.bytecode.emitABC(.RETURN, freeze_reg, 2, 0);
}

// ------------------------------------------------------------------ classes (DebugLuauUserDefinedClasses)

// LuauClassMemberVisibility (Bytecode.h)
const kLCMVPublic: u8 = 0;
const kLCMVPrivate: u8 = 1;

const ClassVisitor = struct {
    self: *Compiler,
    // TODO (upstream CLI-226024): this needs to become a stack to handle nested classes properly.
    curr_class: ?*Stat = null,

    pub fn visitExpr(v: *ClassVisitor, e: *Expr) Error!bool {
        if (e.data == .function) try v.self.enclosing_classes.put(v.self.gpa, e, v.curr_class);
        return true;
    }

    pub fn visitStat(v: *ClassVisitor, s: *Stat) Error!bool {
        if (s.data != .class) return true;
        const cls = s.data.class;

        v.curr_class = s;

        var private_offsets: PrivateOffsets = .empty;
        var num_private_instance_props: u8 = 0;
        var num_private_static_props: u8 = 0;

        for (cls.members) |member| switch (member) {
            .property => |prop| {
                if (prop.access == .private) {
                    try private_offsets.put(v.self.gpa, prop.name, .{ .offset = num_private_instance_props, .is_static = false });
                    num_private_instance_props +%= 1;
                }
            },
            .method => |method| {
                if (method.access == .private) {
                    try private_offsets.put(v.self.gpa, method.name, .{ .offset = num_private_static_props, .is_static = true });
                    num_private_static_props +%= 1;
                }

                try an.walkExpr(ClassVisitor, v, method.function);
            },
        };

        if (private_offsets.count() != 0) try v.self.class_private_field_offsets.put(v.self.gpa, s, private_offsets);
        v.curr_class = null;

        return false;
    }
};

fn getCurClassUpval(self: *Compiler) Error!u8 {
    return getUpval(self, self.curr_class.?.data.class.name);
}

fn compilePrivateFieldAccess(self: *Compiler, location: Location, index: []const u8, target: u8, source: u8, namecall: bool) Error!void {
    const offsets = self.class_private_field_offsets.getPtr(self.curr_class.?) orelse
        return raise(self, location, "Attempt to access private field of a class with no private fields", .{});

    const field = offsets.get(index) orelse return raise(self, location, "Attempt to access undefined private field", .{});

    const class_upval = try getCurClassUpval(self);

    try self.bytecode.emitABC(.GETPRIVATEFIELD, target, source, class_upval);
    try self.bytecode.emitAux((@as(u32, @intFromBool(namecall)) << 9) | (@as(u32, @intFromBool(field.is_static)) << 8) | field.offset);
}

fn compileClassDeclaration(self: *Compiler, node: *Stat) Error!void {
    std.debug.assert(self.options.flags.DebugLuauUserDefinedClasses);
    const decl = node.data.class;

    // We allocate one register to store the class object.
    const dest = try allocReg(self, node.location, 1);

    try pushLocal(self, decl.name, dest, kDefaultAllocPc);
    if (self.options.flags.LuauExportValueSyntax and decl.exported) {
        // we want to eagerly insert into the exported classes map, as the class may be referenced by one of its methods
        try ensureExportTable(self, node.location);
        try self.exports.classes.put(self.gpa, decl.name, dest);
    }

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    // The bottom bit of the C slot indicates whether the class is open.
    const instr_c_val: u8 = @intFromBool(decl.open);

    if (decl.super) |super| {
        // If the superclass is already in a local register, we can reference it directly
        const super_reg = getExprLocalReg(self, super);
        const super_dest = try allocReg(self, node.location, if (super_reg == null) 1 else 0);

        if (super_reg) |reg| {
            try self.bytecode.emitABC(.NEWCLASS, dest, reg, instr_c_val);
        } else {
            try compileExpr(self, super, super_dest, false);
            try self.bytecode.emitABC(.NEWCLASS, dest, super_dest, instr_c_val);
        }
    } else {
        // The range of valid registers is 0-254, so we use 0xFF (255) to indicate the absence of a superclass.
        try self.bytecode.emitABC(.NEWCLASS, dest, kInvalidReg, instr_c_val);
    }

    // The class constant is patched into this AUX slot once the shape has been added to the constant table
    const aux_offset = self.bytecode.emitLabel();
    try self.bytecode.emitAux(0xDEADBEEF);

    var property_names: std.ArrayList(i32) = .empty;
    var method_names: std.ArrayList(i32) = .empty;
    var shape: builder_mod.ClassShape = .{ .class_name = try addString(self, decl.name.name, decl.name.location) };

    // temporary storage for the closures of the class methods
    const temp = try allocReg(self, node.location, 1);

    var has_explicit_constructor = false;

    for (decl.members) |member| switch (member) {
        .property => |prop| switch (prop.access) {
            .public => try property_names.append(self.gpa, try addString(self, prop.name, prop.name_location)),
            .private => {
                shape.num_private_instance_props +%= 1;
            },
        },
        .method => |method| {
            // methods are compiled as free floating functions and added to the class with NEWCLASSMEMBER
            try compileExprFunction(self, method.function, temp);
            switch (method.access) {
                .public => {
                    const method_name_cid = try addString(self, method.name, method.function.location);
                    try method_names.append(self.gpa, method_name_cid);
                    try self.bytecode.emitABC(.NEWCLASSMEMBER, dest, kLCMVPublic, temp);
                    try self.bytecode.emitAux(@bitCast(method_name_cid));
                },
                .private => {
                    try self.bytecode.emitABC(.NEWCLASSMEMBER, dest, kLCMVPrivate, temp);
                    try self.bytecode.emitAux(shape.num_private_static_props);
                    shape.num_private_static_props +%= 1;
                },
            }

            if (std.mem.eql(u8, method.name, "__init")) {
                // Private constructors are not yet supported
                std.debug.assert(method.access == .public);
                has_explicit_constructor = true;
            }
        },
    };

    // All classes have `__init` methods.
    if (!has_explicit_constructor) {
        try method_names.append(self.gpa, try addString(self, try self.names.getOrAdd("__init"), node.location));
    }

    shape.property_names = property_names.items;
    shape.method_names = method_names.items;

    // Finally, we create the class constant and patch the AUX slot from before.
    const class_const = try checkedConstant(self, try self.bytecode.addClassShape(shape), node.location);
    self.bytecode.patchAux(aux_offset, @intCast(class_const));
}

/// Register of `local` while it is allocated in the current function, or null.
fn getLocalReg(self: *Compiler, local: *ast.Local) ?u8 {
    const l = self.locals.get(local) orelse return null;
    return if (l.allocated) l.reg else null;
}

fn localInfo(self: *Compiler, local: *ast.Local) Error!*LocalInfo {
    const gop = try self.locals.getOrPut(self.gpa, local);
    if (!gop.found_existing) gop.value_ptr.* = .{};
    return gop.value_ptr;
}

fn getUpval(self: *Compiler, local: *ast.Local) Error!u8 {
    for (self.upvals.items, 0..) |uv, uid| {
        if (uv == local) return @intCast(uid);
    }

    if (self.upvals.items.len >= kMaxUpvalueCount)
        return raise(self, local.location, "Out of upvalue registers when trying to allocate {s}: exceeded limit {d}", .{ local.name, kMaxUpvalueCount });

    // mark local as captured so that closeLocals emits LOP_CLOSEUPVALS accordingly
    if (self.variables.get(local)) |v| {
        if (v.written) {
            const l = try localInfo(self, local);
            l.captured = true;

            if (self.options.flags.LuauCompileMoveElision and l.allocated) self.reg_captured[l.reg] = true;
        }
    }

    try self.upvals.append(self.gpa, local);
    return @intCast(self.upvals.items.len - 1);
}

fn alwaysTerminates(self: *Compiler, node: *Stat) bool {
    return an.alwaysTerminates(&self.constants, node);
}

fn emitLoadK(self: *Compiler, target: u8, cid: i32) Error!void {
    std.debug.assert(cid >= 0);
    if (cid < 32768) {
        try self.bytecode.emitAD(.LOADK, target, @intCast(cid));
    } else {
        try self.bytecode.emitAD(.LOADKX, target, 0);
        try self.bytecode.emitAux(@intCast(cid));
    }
}

fn allocReg(self: *Compiler, location: Location, count: u32) Error!u8 {
    const top = self.reg_top;
    if (top + count > kMaxRegisterCount)
        return raise(self, location, "Out of registers when trying to allocate {d} registers: exceeded limit {d}", .{ count, kMaxRegisterCount });

    self.reg_top += count;
    self.stack_size = @max(self.stack_size, self.reg_top);

    return @intCast(top);
}

fn setDebugLine(self: *Compiler, location: Location) void {
    if (self.options.debug_level >= 1) self.bytecode.setDebugLine(@intCast(location.begin.line + 1));
}

fn setDebugLineEnd(self: *Compiler, location: Location) void {
    if (self.options.debug_level >= 1) self.bytecode.setDebugLine(@intCast(location.end.line + 1));
}

fn needsCoverageStat(node: *Stat) bool {
    return node.data != .block and node.data != .type_alias;
}

fn patchJump(self: *Compiler, location: Location, label: usize, target: usize) Error!void {
    if (!try self.bytecode.patchJumpD(label, target))
        return raise(self, location, "Exceeded jump distance limit; simplify the code to compile", .{});
}

fn patchJumps(self: *Compiler, location: Location, labels: []const usize, target: usize) Error!void {
    for (labels) |l| try patchJump(self, location, l, target);
}

fn patchSkip(self: *Compiler, location: Location, label: usize, target: usize) Error!void {
    if (!self.bytecode.patchSkipC(label, target))
        return raise(self, location, "Exceeded jump distance limit; simplify the code to compile", .{});
}

fn patchLoopJumps(self: *Compiler, location: Location, old_jumps: usize, end_label: usize, cont_label: usize) Error!void {
    std.debug.assert(old_jumps <= self.loop_jumps.items.len);
    for (self.loop_jumps.items[old_jumps..]) |lj| {
        switch (lj.type) {
            .brk => try patchJump(self, location, lj.label, end_label),
            .cont => try patchJump(self, location, lj.label, cont_label),
        }
    }
}

fn isConstant(self: *Compiler, node: *Expr) bool {
    const cv = self.constants.get(node) orelse return false;
    return cv.known();
}

fn isConstantTrue(self: *Compiler, node: *Expr) bool {
    return an.isConstantTrue(&self.constants, node);
}

fn isConstantFalse(self: *Compiler, node: *Expr) bool {
    return an.isConstantFalse(&self.constants, node);
}

fn getConstant(self: *Compiler, node: *Expr) Constant {
    return self.constants.get(node) orelse .unknown;
}

fn getConstantNumber(self: *Compiler, node: *Expr) Error!i32 {
    if (self.constants.get(node)) |c| {
        if (c == .number) return checkedConstant(self, try self.bytecode.addConstantNumber(c.number), node.location);
    }
    return -1;
}

fn getConstantIndex(self: *Compiler, node: *Expr) Error!i32 {
    const c = self.constants.get(node) orelse return -1;

    const cid: i32 = switch (c) {
        .unknown, .table => return -1,
        .nil => try self.bytecode.addConstantNil(),
        .boolean => |b| try self.bytecode.addConstantBoolean(b),
        .number => |n| try self.bytecode.addConstantNumber(n),
        .integer => |n| try self.bytecode.addConstantInteger(n),
        .vectorf => |v| try self.bytecode.addConstantVectorf(v[0], v[1], v[2], v[3]),
        .vectord => |v| try self.bytecode.addConstantVectord(v[0], v[1], v[2], v[3]),
        .string => |s| try self.bytecode.addConstantString(s),
    };

    return checkedConstant(self, cid, node.location);
}

fn getExprLocal(node: *Expr) ?*ast.Local {
    const e = an.unwrapExpr(node);
    return if (e.data == .local) e.data.local.local else null;
}

fn getExprLocalReg(self: *Compiler, node: *Expr) ?u8 {
    // note: this can't check expr->upvalue because upvalues may be upgraded to locals during inlining
    const local = getExprLocal(node) orelse return null;
    return getLocalReg(self, local);
}

fn isExprLocalReg(self: *Compiler, node: *Expr, reg: usize) bool {
    const local_reg = getExprLocalReg(self, node) orelse return false;
    return local_reg == reg;
}

fn encodeHashSize(hash_size: u32) u8 {
    var hash_size_log2: u5 = 0;
    while ((@as(u32, 1) << hash_size_log2) < hash_size) hash_size_log2 += 1;
    return if (hash_size == 0) 0 else @as(u8, hash_size_log2) + 1;
}

// ------------------------------------------------------------------ locals

fn pushLocal(self: *Compiler, local: *ast.Local, reg: u8, allocpc: u32) Error!void {
    const live_locals = if (self.options.flags.LuauCompileReuseLowersLimit) self.active_locals else self.local_stack.items.len;
    if (live_locals >= kMaxLocalCount)
        return raise(self, local.location, "Out of local registers when trying to allocate {s}: exceeded limit {d}", .{ local.name, kMaxLocalCount });

    try self.local_stack.append(self.gpa, local);

    if (self.options.flags.LuauCompileReuseLowersLimit) self.active_locals += 1;

    const l = try localInfo(self, local);
    std.debug.assert(!l.allocated);

    l.reg = reg;
    l.allocated = true;
    l.debugpc = self.bytecode.getDebugPC();
    l.allocpc = if (allocpc == kDefaultAllocPc) l.debugpc else allocpc;

    if (self.options.flags.LuauCompileMoveElision) self.reg_captured[reg] = l.captured;

    if (self.options.flags.LuauCompileReuseLocalRegs) self.local_uses[reg] += 1;
}

fn areLocalsCaptured(self: *Compiler, start: usize) bool {
    for (self.local_stack.items[start..]) |local| {
        if (self.locals.get(local).?.captured) return true;
    }
    return false;
}

fn closeLocals(self: *Compiler, start: usize) Error!void {
    var captured = false;
    var capture_reg: u8 = 255;

    for (self.local_stack.items[start..]) |local| {
        const l = self.locals.get(local).?;
        if (l.captured) {
            captured = true;
            capture_reg = @min(capture_reg, l.reg);
        }
    }

    if (captured) try self.bytecode.emitABC(.CLOSEUPVALS, capture_reg, 0, 0);
}

fn recordLocalDebugInfo(self: *Compiler, ast_local: *ast.Local, l: *const LocalInfo, is_argument: bool, endpc: u32) Error!void {
    if (self.options.debug_level >= 2) try self.bytecode.pushDebugLocal(ast_local.name, l.reg, l.debugpc, endpc);

    if (self.options.type_info_level >= 1 and !is_argument) {
        const ty = self.local_types.get(ast_local) orelse bc.LBC_TYPE_ANY;
        try self.bytecode.pushLocalTypeInfo(ty, l.reg, l.allocpc, endpc);
    }
}

/// Returns true if the local release has also freed the last use of the register assigned to it
fn releaseLocal(self: *Compiler, l: *LocalInfo) bool {
    std.debug.assert(self.options.flags.LuauCompileReuseLocalRegs);

    l.allocated = false;

    if (self.options.flags.LuauCompileReuseLowersLimit) {
        std.debug.assert(self.active_locals != 0);
        self.active_locals -= 1;
    }

    if (self.options.flags.LuauCompileMoveElision) self.reg_captured[l.reg] = false;

    std.debug.assert(self.local_uses[l.reg] != 0);
    self.local_uses[l.reg] -= 1;
    return self.local_uses[l.reg] == 0;
}

/// index of the first free local register entry with reg >= `reg` (free_local_regs is sorted by register)
fn freeLocalLowerBound(self: *const Compiler, reg: u8) usize {
    var lo: usize = 0;
    var hi: usize = self.free_local_regs.items.len;
    while (lo < hi) {
        const mid = lo + (hi - lo) / 2;
        if (self.free_local_regs.items[mid].reg < reg) lo = mid + 1 else hi = mid;
    }
    return lo;
}

fn popLocals(self: *Compiler, start: usize) Error!void {
    for (self.local_stack.items[start..], start..) |local, i| {
        const l = self.locals.getPtr(local).?;

        if (self.options.flags.LuauCompileReuseLocalRegs) {
            if (l.reused) {
                // Local going out of scope completely, its register can no longer be reused
                const it = freeLocalLowerBound(self, l.reg);
                if (it < self.free_local_regs.items.len and self.free_local_regs.items[it].reg == l.reg) {
                    const free = self.free_local_regs.items[it];
                    try recordLocalDebugInfo(self, free.ast_local, &free.local, free.is_argument, self.bytecode.getDebugPC());
                    _ = self.free_local_regs.orderedRemove(it);
                }

                l.allocated = false;
                l.reused = false;
            } else if (l.allocated) {
                _ = releaseLocal(self, l);
                try recordLocalDebugInfo(self, local, l, i < self.arg_count, self.bytecode.getDebugPC());
            }
        } else {
            std.debug.assert(l.allocated);
            l.allocated = false;

            if (self.options.flags.LuauCompileReuseLowersLimit) {
                std.debug.assert(self.active_locals != 0);
                self.active_locals -= 1;
            }

            if (self.options.flags.LuauCompileMoveElision) self.reg_captured[l.reg] = false;

            if (self.options.debug_level >= 2) {
                const debugpc = self.bytecode.getDebugPC();
                try self.bytecode.pushDebugLocal(local.name, l.reg, l.debugpc, debugpc);
            }

            if (self.options.type_info_level >= 1 and i >= self.arg_count) {
                const debugpc = self.bytecode.getDebugPC();
                const ty = self.local_types.get(local) orelse bc.LBC_TYPE_ANY;
                try self.bytecode.pushLocalTypeInfo(ty, l.reg, l.allocpc, debugpc);
            }
        }
    }

    self.local_stack.shrinkRetainingCapacity(start);
}

/// LuauCompileReuseLocalRegs: releases registers of locals in `owner` whose last lexical use precedes `before_stat`
fn popStatLocals(self: *Compiler, owner: *Stat, before_stat: *Stat, start: usize) Error!void {
    if (self.options.optimization_level == 0) return;

    std.debug.assert(self.options.flags.LuauCompileReuseLocalRegs);
    std.debug.assert(start <= self.local_stack.items.len);

    for (self.local_stack.items[start..], start..) |local, i| {
        const l = self.locals.getPtr(local).?;

        if (!l.allocated or l.reused) continue;

        if (l.captured) continue;

        const v = self.variables.get(local) orelse continue;

        if (v.owner != owner or v.non_lexical_use) continue;

        const stat_start = before_stat.location.begin;

        const dead = if (v.last_used) |lu| blk: {
            const local_end = lu.location.end;
            break :blk stat_start.line > local_end.line or (stat_start.line == local_end.line and stat_start.column > local_end.column);
        } else true;

        if (dead) {
            if (releaseLocal(self, l)) {
                const free: FreeLocal = .{ .reg = l.reg, .local = l.*, .ast_local = local, .is_argument = i < self.arg_count };

                // Free registers are ordered from low to high as it is better to give out registers closer to regTop
                try self.free_local_regs.insert(self.gpa, freeLocalLowerBound(self, l.reg), free);

                l.reused = true;
            } else {
                try recordLocalDebugInfo(self, local, l, i < self.arg_count, self.bytecode.getDebugPC());
            }
        }
    }
}

/// LuauCompileReuseLocalRegs: picks a register of a dead local for `local`, returns null when none can be reused
fn tryReuseLocalReg(self: *Compiler, local: *ast.Local) Error!?u8 {
    std.debug.assert(self.options.flags.LuauCompileReuseLocalRegs);

    if (local.is_exported or self.free_local_regs.items.len == 0) return null;

    if (!self.variables.contains(local)) return null;

    if (self.locals.get(local)) |l| {
        if (l.captured) return null;
    }

    const free = self.free_local_regs.pop().?;

    try recordLocalDebugInfo(self, free.ast_local, &free.local, free.is_argument, self.bytecode.getDebugPC());

    std.debug.assert(free.reg < self.reg_top);
    return free.reg;
}

fn hintTemporaryRegType(self: *Compiler, expr: *Expr, reg: u8, expected_type: u8, inst_length: u32) Error!void {
    // If we know the type of a temporary and it's not the type that would be expected by codegen, provide a hint
    if (self.expr_types.get(expr)) |ty| {
        if (ty != expected_type) try self.bytecode.pushLocalTypeInfo(ty, reg, self.bytecode.getDebugPC() - inst_length, self.bytecode.getDebugPC());
    }
}

fn hintTemporaryExprRegType(self: *Compiler, expr: *Expr, reg: u8, expected_type: u8, inst_length: u32) Error!void {
    // If we allocated a temporary register for the operation argument, try hinting its type
    if (getExprLocal(expr) == null) try hintTemporaryRegType(self, expr, reg, expected_type, inst_length);
}

fn getConstantTableValue(self: *Compiler, node: *Expr) ?*Expr {
    // tryIndexConstantTable: if we are referring to a local
    const index = node.data.index_name;
    const table_local_expr = an.unwrapExpr(index.expr);
    if (table_local_expr.data != .local) return null;
    const table_local = table_local_expr.data.local.local;

    // And it's not mutable and has an initializer
    const lv = self.variables.get(table_local) orelse return null;
    if (lv.written or lv.init == null) return null;

    // And the local is a constant table
    if (!self.table_constants.contains(table_local)) return null;

    const table = an.unwrapExpr(lv.init.?);
    if (table.data != .table) return null;

    // Look into the initializer to find the function value
    var match: ?*Expr = null;

    for (table.data.table) |item| {
        if (item.kind == .record or item.kind == .general) {
            const key_constant = self.constants.get(item.key.?);
            if (key_constant == null) {
                // Dynamic key can alias any of the constant keys
                match = null;
            } else if (key_constant.? == .string and key_constant.?.string.len != 0) {
                // No break as last match determines the lookup result if there are duplicates
                if (std.mem.eql(u8, key_constant.?.string, index.index)) match = item.value;
            }
        }
    }

    return match;
}

fn getFunctionExpr(self: *Compiler, node: *Expr) ?*Expr {
    switch (node.data) {
        .local => |l| {
            const lv = self.variables.get(l.local) orelse return null;
            if (lv.written or lv.init == null) return null;
            return getFunctionExpr(self, lv.init.?);
        },
        .index_name => |i| {
            if (self.options.flags.DebugLuauUserDefinedClasses and i.private_access) return null;
            if (getConstantTableValue(self, node)) |value| return getFunctionExpr(self, value);
            return null;
        },
        .group => |g| return getFunctionExpr(self, g),
        .type_assertion => |t| return getFunctionExpr(self, t.expr),
        .instantiate => |i| return getFunctionExpr(self, i.expr),
        .function => return node,
        else => return null,
    }
}

// ------------------------------------------------------------------ expressions: calls

// returns true if node can return multiple values; may conservatively return true even if expr is known to return just a single value
fn isExprMultRet(self: *Compiler, node: *Expr) bool {
    if (node.data != .call) return node.data == .varargs;

    // conservative version, optimized for compilation throughput
    if (self.options.optimization_level <= 1) return true;

    // handles builtin calls that can be constant-folded
    // without this we may omit some optimizations eg compiling fast calls without use of FASTCALL2K
    if (isConstant(self, node)) return false;

    // handles builtin calls that can't be constant-folded but are known to return one value
    // note: optimizationLevel check is technically redundant but it's important that we never optimize based on builtins in O1
    if (self.options.optimization_level >= 2) {
        if (self.builtins.get(node)) |bfid| {
            if (bfid != .NONE) return builtin_info.getBuiltinInfo(bfid).results != 1;
        }
    }

    // handles local function calls where we know only one argument is returned
    if (getFunctionExpr(self, node.data.call.func)) |func| {
        if (self.functions.get(func)) |fi| {
            if (fi.returns_one) return false;
        }
    }

    // unrecognized call, so we conservatively assume multret
    return true;
}

// note: this doesn't just clobber target (assuming it's temp), but also clobbers *all* allocated registers >= target!
// this is important to be able to support "multret" semantics due to Lua call frame structure
fn compileExprTempMultRet(self: *Compiler, node: *Expr, target: u8) Error!bool {
    // Optimization: convert multret calls that always return one value to fixedret calls; this facilitates inlining/constant folding
    if (node.data == .call and self.options.optimization_level >= 2 and !isExprMultRet(self, node)) {
        try compileExprTemp(self, node, target);
        return false;
    }

    switch (node.data) {
        .call => {
            // We temporarily swap out regTop to have targetTop work correctly...
            const old_top = self.reg_top;
            defer self.reg_top = old_top;
            std.debug.assert(target <= self.reg_top);
            self.reg_top = target;
            try compileExprCall(self, node, target, 0, true, true);
            return true;
        },
        .varargs => {
            const old_top = self.reg_top;
            defer self.reg_top = old_top;
            std.debug.assert(target <= self.reg_top);
            self.reg_top = target;
            try compileExprVarargs(self, node, target, 0, true);
            return true;
        },
        else => {
            try compileExprTemp(self, node, target);
            return false;
        },
    }
}

// note: this doesn't just clobber target (assuming it's temp), but also clobbers *all* allocated registers >= target!
// this is important to be able to emit code that takes fewer registers and runs faster
fn compileExprTempTop(self: *Compiler, node: *Expr, target: u8) Error!void {
    // We temporarily swap out regTop to have targetTop work correctly...
    const old_top = self.reg_top;
    defer self.reg_top = old_top;
    std.debug.assert(@as(u32, target) + 1 <= self.reg_top);
    self.reg_top = @as(u32, target) + 1;
    try compileExprTemp(self, node, target);
}

fn compileExprVarargs(self: *Compiler, node: *Expr, target: u8, target_count: u8, mult_ret: bool) Error!void {
    std.debug.assert(target_count < 255);
    std.debug.assert(!mult_ret or @as(u32, target) + target_count == self.reg_top);

    setDebugLine(self, node.location); // normally compileExpr sets up line info, but compileExprVarargs can be called directly

    try self.bytecode.emitABC(.GETVARARGS, target, if (mult_ret) 0 else target_count + 1, 0);
}

fn compileExprSelectVararg(self: *Compiler, node: *Expr, target: u8, target_count: u8, target_top: bool, mult_ret: bool, regs: u8) Error!void {
    std.debug.assert(target_count == 1);
    const expr = node.data.call;
    std.debug.assert(!expr.self);
    std.debug.assert(expr.args.len == 2 and expr.args[1].data == .varargs);

    const arg = expr.args[0];

    var argreg: u8 = undefined;

    if (getExprLocalReg(self, arg)) |reg| {
        argreg = reg;
    } else {
        argreg = regs + 1;
        try compileExprTempTop(self, arg, argreg);
    }

    const fastcall_label = self.bytecode.emitLabel();

    try self.bytecode.emitABC(.FASTCALL1, @intFromEnum(bc.Builtin.SELECT_VARARG), argreg, 0);

    // note, these instructions are normally not executed and are used as a fallback for FASTCALL
    // we can't use TempTop variant here because we need to make sure the arguments we already computed aren't overwritten
    try compileExprTemp(self, expr.func, regs);

    if (argreg != regs + 1) try self.bytecode.emitABC(.MOVE, regs + 1, argreg, 0);

    try self.bytecode.emitABC(.GETVARARGS, regs + 2, 0, 0);

    const call_label = self.bytecode.emitLabel();
    try patchSkip(self, expr.func.location, fastcall_label, call_label);

    // note, this is always multCall (last argument is variadic)
    try self.bytecode.emitABC(.CALL, regs, 0, if (mult_ret) 0 else target_count + 1);

    // if we didn't output results directly to target, we need to move them
    if (!target_top) {
        for (0..target_count) |i| try self.bytecode.emitABC(.MOVE, target + @as(u8, @intCast(i)), regs + @as(u8, @intCast(i)), 0);
    }
}

fn compileExprFastcallN(self: *Compiler, node: *Expr, target: u8, target_count: u8, target_top: bool, mult_ret: bool, regs: u8, bfid: bc.Builtin, bf_k: i32) Error!void {
    const expr = node.data.call;
    std.debug.assert(!expr.self);
    std.debug.assert(expr.args.len >= 1 and expr.args.len <= 3);
    std.debug.assert(if (bfid == .BIT32_EXTRACTK) bf_k >= 0 else bf_k < 0);
    std.debug.assert(target_count < 255);

    var opc: Op = .NOP;

    if (expr.args.len == 1) {
        opc = .FASTCALL1;
    } else if (bf_k >= 0 or (expr.args.len == 2 and isConstant(self, expr.args[1]))) {
        opc = .FASTCALL2K;
    } else if (expr.args.len == 2) {
        opc = .FASTCALL2;
    } else {
        opc = .FASTCALL3;
    }

    var args = [3]u32{ 0, 0, 0 };

    for (expr.args, 0..) |arg, i| {
        if (i > 0 and opc == .FASTCALL2K) {
            const cid = try getConstantIndex(self, arg);
            if (cid < 0) return constantLimit(self, node.location);
            args[i] = @intCast(cid);
        } else {
            if (getExprLocalReg(self, arg)) |reg| {
                args[i] = reg;
            } else {
                args[i] = @as(u32, regs) + 1 + @as(u32, @intCast(i));

                if (self.options.flags.LuauCompileMoveElision) {
                    // We temporarily swap out regTop to have targetTop work correctly here, see compileExprTempTop for info
                    const arg_old_top = self.reg_top;
                    defer self.reg_top = arg_old_top;
                    self.reg_top = args[i] + 1;

                    args[i] = try compileExprAutoTemp(self, arg, @intCast(args[i]), kInvalidReg);
                } else {
                    try compileExprTempTop(self, arg, @intCast(args[i]));
                }
            }
        }
    }

    const fastcall_label = self.bytecode.emitLabel();

    try self.bytecode.emitABC(opc, @intFromEnum(bfid), @truncate(args[0]), 0);

    if (opc == .FASTCALL3) {
        std.debug.assert(bf_k < 0);
        try self.bytecode.emitAux(args[1] | (args[2] << 8));
    } else if (opc != .FASTCALL1) {
        try self.bytecode.emitAux(if (bf_k >= 0) @intCast(bf_k) else args[1]);
    }

    // Set up a traditional Lua stack for the subsequent LOP_CALL.
    // Note, as with other instructions that immediately follow FASTCALL, these are normally not executed and are used as a fallback for
    // these FASTCALL variants.
    for (0..expr.args.len) |i| {
        const slot: u8 = regs + 1 + @as(u8, @intCast(i));
        if (i > 0 and opc == .FASTCALL2K) {
            try emitLoadK(self, slot, @intCast(args[i]));
        } else if (args[i] != slot) {
            try self.bytecode.emitABC(.MOVE, slot, @intCast(args[i]), 0);
        }
    }

    // note, these instructions are normally not executed and are used as a fallback for FASTCALL
    // we can't use TempTop variant here because we need to make sure the arguments we already computed aren't overwritten
    try compileExprTemp(self, expr.func, regs);

    const call_label = self.bytecode.emitLabel();

    // FASTCALL will skip over the instructions needed to compute function and jump over CALL which must immediately follow the instruction
    // sequence after FASTCALL
    try patchSkip(self, expr.func.location, fastcall_label, call_label);

    try self.bytecode.emitABC(.CALL, regs, @truncate(expr.args.len + 1), if (mult_ret) 0 else target_count + 1);

    // if we didn't output results directly to target, we need to move them
    if (!target_top) {
        for (0..target_count) |i| try self.bytecode.emitABC(.MOVE, target + @as(u8, @intCast(i)), regs + @as(u8, @intCast(i)), 0);
    }
}

fn compileExprCall(self: *Compiler, node: *Expr, target: u8, target_count: u8, target_top: bool, mult_ret: bool) Error!void {
    const expr = node.data.call;
    std.debug.assert(target_count < 255);
    std.debug.assert(!target_top or @as(u32, target) + target_count == self.reg_top);

    setDebugLine(self, node.location); // normally compileExpr sets up line info, but compileExprCall can be called directly

    // try inlining the function
    if (self.options.optimization_level >= 2 and !expr.self) {
        if (getFunctionExpr(self, expr.func)) |func| {
            if (self.functions.get(func)) |fi| {
                if (fi.can_inline and try tryCompileInlinedCall(self, node, func, target, target_count, mult_ret, kInlineThreshold, kInlineThresholdMaxBoost, kInlineDepth))
                    return;
            }
        }
    }

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    const self_count: u32 = @intFromBool(expr.self);
    const reg_count: u32 = @max(1 + self_count + @as(u32, @intCast(expr.args.len)), target_count);

    // Optimization: if target points to the top of the stack, we can start the call at oldTop - 1 and won't need MOVE at the end
    const regs: u8 = if (target_top) (try allocReg(self, node.location, reg_count - target_count)) - target_count else try allocReg(self, node.location, reg_count);

    var selfreg: u8 = 0;

    var bfid: ?bc.Builtin = null;

    if (self.options.optimization_level >= 1 and !expr.self) {
        if (self.builtins.get(node)) |id| {
            if (id != .NONE) bfid = id;
        }
    }

    if (bfid == .SELECT_VARARG) {
        // Optimization: compile select(_, ...) as FASTCALL1; the builtin will read variadic arguments directly
        // note: for now we restrict this to single-return expressions since our runtime code doesn't deal with general cases
        if (mult_ret == false and target_count == 1) {
            return compileExprSelectVararg(self, node, target, target_count, target_top, mult_ret, regs);
        }
        bfid = null;
    }

    // Optimization: for bit32.extract with constant in-range f/w we compile using FASTCALL2K and a special builtin
    if (bfid == .BIT32_EXTRACT and expr.args.len == 3 and isConstant(self, expr.args[1]) and isConstant(self, expr.args[2])) {
        const fc = getConstant(self, expr.args[1]);
        const wc = getConstant(self, expr.args[2]);

        const fi: i64 = if (fc == .number) cInt(fc.number) else -1;
        const wi: i64 = if (wc == .number) cInt(wc.number) else -1;

        if (fi >= 0 and wi > 0 and fi + wi <= 32) {
            const fwp: i64 = fi | ((wi - 1) << 5);
            const cid = try self.bytecode.addConstantNumber(@floatFromInt(fwp));
            if (cid < 0) return constantLimit(self, node.location);

            return compileExprFastcallN(self, node, target, target_count, target_top, mult_ret, regs, .BIT32_EXTRACTK, cid);
        }
    }

    var max_fastcall_args: usize = 2;

    // Fastcall with 3 arguments is only used if it can help save one or more move instructions
    if (bfid != null and expr.args.len == 3) {
        for (expr.args) |arg| {
            if (getExprLocalReg(self, arg) != null) {
                max_fastcall_args = 3;
                break;
            }
        }
    }

    // Optimization: for 1/2/3 argument fast calls use specialized opcodes
    if (bfid != null and expr.args.len >= 1 and expr.args.len <= max_fastcall_args) {
        if (!isExprMultRet(self, expr.args[expr.args.len - 1])) {
            return compileExprFastcallN(self, node, target, target_count, target_top, mult_ret, regs, bfid.?, -1);
        } else if (self.options.optimization_level >= 2) {
            // when a builtin is none-safe with matching arity, even if the last expression returns 0 or >1 arguments,
            // we can rely on the behavior of the function being the same (none-safe means nil and none are interchangeable)
            const info = builtin_info.getBuiltinInfo(bfid.?);
            if (@as(i64, @intCast(expr.args.len)) == info.params and info.none_safe)
                return compileExprFastcallN(self, node, target, target_count, target_top, mult_ret, regs, bfid.?, -1);
        }
    }

    // Optimization: pcall/xpcall functions have a special fastcall instruction
    var fast_pcall_id: ?u8 = null;
    if (self.options.flags.LuauCompileFastpcall and self.options.optimization_level >= 1 and !expr.self) {
        if (expr.func.data == .global) {
            const g = expr.func.data.global;
            if (canImport(self, g)) {
                if (std.mem.eql(u8, g, "pcall") and expr.args.len >= 1)
                    fast_pcall_id = 0
                else if (std.mem.eql(u8, g, "xpcall") and expr.args.len >= 2)
                    fast_pcall_id = 1;
            }
        }
    }

    if (expr.self) {
        const fi = expr.func.data.index_name;

        if (getExprLocalReg(self, fi.expr)) |reg| {
            selfreg = reg;
        } else {
            // Note: to be able to compile very deeply nested self call chains (obj:method1():method2():...), we need to be able to do this in
            // finite stack space NAMECALL will happily move object from regs to regs+1 but we need to compute it into regs so that
            // compileExprTempTop doesn't increase stack usage for every recursive call
            selfreg = regs;

            try compileExprTempTop(self, fi.expr, selfreg);
        }
    } else if (bfid == null and fast_pcall_id == null) {
        try compileExprTempTop(self, expr.func, regs);
    }

    var mult_call = false;
    var construct_call = false;

    if (self.options.flags.DebugLuauUserDefinedClasses and !expr.self and expr.table_call and bfid == null and fast_pcall_id == null) {
        std.debug.assert(expr.args.len == 1 and expr.args[0].data == .table);

        const arg = regs + 1;

        const construct_old_top = self.reg_top;
        defer self.reg_top = construct_old_top;
        self.reg_top = arg + 1;

        try compileExprTable(self, expr.args[0], arg, true, regs);
        construct_call = true;
    } else {
        for (expr.args, 0..) |arg, i| {
            const slot: u8 = @intCast(@as(u32, regs) + 1 + self_count + @as(u32, @intCast(i)));
            if (i + 1 == expr.args.len)
                mult_call = try compileExprTempMultRet(self, arg, slot)
            else
                try compileExprTempTop(self, arg, slot);
        }
    }

    setDebugLineEnd(self, expr.func.location);

    if (expr.self) {
        const fi = expr.func.data.index_name;

        setDebugLine(self, fi.index_location);

        if (self.options.flags.DebugLuauUserDefinedClasses and fi.private_access) {
            // the parser only sets private_access inside a class body, so curr_class is set here
            std.debug.assert(self.curr_class != null);

            try compilePrivateFieldAccess(self, expr.func.location, fi.index, regs, selfreg, true);
        } else {
            const cid = try addString(self, fi.index, expr.func.location);

            try self.bytecode.emitABC(.NAMECALL, regs, selfreg, hash8(fi.index));
            try self.bytecode.emitAux(@intCast(cid));

            try hintTemporaryExprRegType(self, fi.expr, selfreg, bc.LBC_TYPE_TABLE, 2);
        }
    } else if (bfid != null or fast_pcall_id != null) {
        const fastcall_label = self.bytecode.emitLabel();

        if (fast_pcall_id) |pid| {
            const explicit_args: u8 = @intCast(expr.args.len - @intFromBool(mult_call));
            try self.bytecode.emitABC(.FASTPCALL, pid, explicit_args, 0);
        } else {
            try self.bytecode.emitABC(.FASTCALL, @intFromEnum(bfid.?), 0, 0);
        }

        // note, these instructions are normally not executed and are used as a fallback for FASTCALL
        // we can't use TempTop variant here because we need to make sure the arguments we already computed aren't overwritten
        try compileExprTemp(self, expr.func, regs);

        const call_label = self.bytecode.emitLabel();

        // FASTCALL will skip over the instructions needed to compute function and jump over CALL which must immediately follow the instruction
        // sequence after FASTCALL
        try patchSkip(self, expr.func.location, fastcall_label, call_label);
    }

    const construct_jump_label = self.bytecode.emitLabel();

    if (construct_call) {
        std.debug.assert(!expr.self);
        try self.bytecode.emitABC(.FINCONSTRUCT, regs + 1, 0, 0);
    }

    const construct_call_label = self.bytecode.emitLabel();

    // Without deoptimization we cannot break VARARG sequences.
    // So VARARG producer or consumer cannot be inlined, because it creates a diamond(with slow path).
    const can_inline = self.current_function.?.data.function.function_depth != 0 and !mult_call and !mult_ret;
    if (self.options.flags.LuauEmitCallFeedback and bfid == null and fast_pcall_id == null and can_inline and !construct_call) {
        const fb_slot = try self.bytecode.addCallTargetSlot(@intCast(self.bytecode.getInstructionCount()));
        try self.bytecode.emitABC(.CALLFB, regs, if (mult_call) 0 else @truncate(self_count + expr.args.len + 1), if (mult_ret) 0 else target_count + 1);
        try self.bytecode.emitAux(fb_slot);
    } else {
        try self.bytecode.emitABC(.CALL, regs, if (mult_call) 0 else @truncate(self_count + expr.args.len + 1), if (mult_ret) 0 else target_count + 1);
    }

    if (construct_call) try patchSkip(self, node.location, construct_jump_label, construct_call_label);

    // if we didn't output results directly to target, we need to move them
    if (!target_top) {
        for (0..target_count) |i| try self.bytecode.emitABC(.MOVE, target + @as(u8, @intCast(i)), regs + @as(u8, @intCast(i)), 0);
    }
}

/// C++ int(double) conversion for values that are known to be in a sane range; out-of-range values map to -1
/// (callers only use the result for small non-negative ranges).
fn cInt(d: f64) i64 {
    if (!(d > -2147483649.0 and d < 2147483648.0)) return -1;
    return @intFromFloat(@trunc(d));
}

fn shouldShareClosure(self: *Compiler, func: *Expr) bool {
    const f = self.functions.get(func) orelse return false;

    for (f.upvals) |uv| {
        const ul = self.variables.get(uv) orelse return false;

        if (ul.written) return false;

        // it's technically safe to share closures whenever all upvalues are immutable
        // this is because of a runtime equality check in DUPCLOSURE.
        // however, this results in frequent de-optimization and increases the set of reachable objects, making some temporary objects permanent
        // instead we apply a heuristic: we share closures if they refer to top-level upvalues, or closures that refer to top-level upvalues
        // this will only de-optimize (outside of fenv changes) if top level code is executed twice with different results.
        if (uv.function_depth != 0 or uv.loop_depth != 0) {
            const uf = ul.init orelse return false;
            if (uf.data != .function) return false;

            if (uf != func and !shouldShareClosure(self, uf)) return false;
        }
    }

    return true;
}

fn compileExprFunction(self: *Compiler, node: *Expr, target: u8) Error!void {
    const expr = &node.data.function;
    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    const f = self.functions.get(node).?;

    // when the closure has upvalues we'll use this to create the closure at runtime
    // when the closure has no upvalues, we use constant closures that technically don't rely on the child function list
    // however, it's still important to add the child function because debugger relies on the function hierarchy when setting breakpoints
    const pid = try self.bytecode.addChildFunction(f.id);
    if (pid < 0) return raise(self, node.location, "Exceeded closure limit; simplify the code to compile", .{});

    // we use a scratch vector to reduce allocations; this is safe since compileExprFunction is not reentrant
    self.captures.clearRetainingCapacity();
    try self.captures.ensureTotalCapacity(self.gpa, f.upvals.len);

    for (f.upvals) |uv| {
        std.debug.assert(uv.function_depth < expr.function_depth);

        if (getLocalReg(self, uv)) |reg| {
            // note: we can't check if uv is an upvalue in the current frame because inlining can migrate from upvalues to locals
            const ul = self.variables.get(uv);
            const immutable = ul == null or !ul.?.written;

            try self.captures.append(self.gpa, .{ .type = if (immutable) bc.LCT_VAL else bc.LCT_REF, .data = reg });
        } else if (self.locstants.get(uv) != null and self.locstants.get(uv).?.known()) {
            const uc = self.locstants.get(uv).?;
            // inlining can result in an upvalue capture of a constant, in which case we can't capture without a temporary register
            const creg = try allocReg(self, node.location, 1);
            try compileExprConstant(self, node, uc, creg);

            try self.captures.append(self.gpa, .{ .type = bc.LCT_VAL, .data = creg });
        } else {
            std.debug.assert(uv.function_depth < expr.function_depth - 1);

            // get upvalue from parent frame
            // note: this will add uv to the current upvalue list if necessary
            const uid = try getUpval(self, uv);

            try self.captures.append(self.gpa, .{ .type = bc.LCT_UPVAL, .data = uid });
        }
    }

    // Optimization: when closure has no upvalues, or upvalues are safe to share, instead of allocating it every time we can share closure
    // objects (this breaks assumptions about function identity which can lead to setfenv not working as expected, so we disable this when it
    // is used)
    var shared: i32 = -1;

    if (self.options.optimization_level >= 1 and shouldShareClosure(self, node) and !self.setfenv_used) {
        const cid = try self.bytecode.addConstantClosure(f.id);
        if (cid >= 0 and cid < 32768) shared = cid;
    }

    if (shared >= 0)
        try self.bytecode.emitAD(.DUPCLOSURE, target, @intCast(shared))
    else
        try self.bytecode.emitAD(.NEWCLOSURE, target, pid);

    for (self.captures.items) |c| try self.bytecode.emitABC(.CAPTURE, c.type, c.data, 0);
}

// ------------------------------------------------------------------ expressions: operators

fn getUnaryOp(op: ast.UnaryOp) Op {
    return switch (op) {
        .not => .NOT,
        .minus => .MINUS,
        .len => .LENGTH,
    };
}

fn getBinaryOpArith(op: ast.BinaryOp, k: bool) Op {
    return switch (op) {
        .add => if (k) .ADDK else .ADD,
        .sub => if (k) .SUBK else .SUB,
        .mul => if (k) .MULK else .MUL,
        .div => if (k) .DIVK else .DIV,
        .floor_div => if (k) .IDIVK else .IDIV,
        .mod => if (k) .MODK else .MOD,
        .pow => if (k) .POWK else .POW,
        else => unreachable,
    };
}

fn getJumpOpCompare(op: ast.BinaryOp, not_: bool) Op {
    return switch (op) {
        .compare_ne => if (not_) .JUMPIFEQ else .JUMPIFNOTEQ,
        .compare_eq => if (not_) .JUMPIFNOTEQ else .JUMPIFEQ,
        .compare_lt, .compare_gt => if (not_) .JUMPIFNOTLT else .JUMPIFLT,
        .compare_le, .compare_ge => if (not_) .JUMPIFNOTLE else .JUMPIFLE,
        else => unreachable,
    };
}

fn compileCompareJump(self: *Compiler, node: *Expr, not_: bool) Error!usize {
    const expr = node.data.binary;
    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    const is_eq = (expr.op == .compare_eq or expr.op == .compare_ne);
    var left = expr.left;
    var right = expr.right;

    var operand_is_constant = isConstant(self, right);
    if (is_eq and !operand_is_constant) {
        operand_is_constant = isConstant(self, left);
        if (operand_is_constant) std.mem.swap(*Expr, &left, &right);
    }

    // disable fast path for vectors because supporting it would require a new opcode
    if (operand_is_constant) {
        const rc = getConstant(self, right);
        if (rc == .vectorf or rc == .vectord or (self.options.flags.LuauIntegerType2 and rc == .integer)) operand_is_constant = false;
    }

    const rl = try compileExprAuto(self, left);

    if (is_eq and operand_is_constant) {
        const cv = self.constants.get(right).?;

        var opc: Op = .NOP;
        var cid: i32 = -1;
        const flip: u32 = if ((expr.op == .compare_eq) == not_) 0x80000000 else 0;

        switch (cv) {
            .nil => {
                opc = .JUMPXEQKNIL;
                cid = 0;
            },
            .boolean => |b| {
                opc = .JUMPXEQKB;
                cid = @intFromBool(b);
            },
            .number => {
                opc = .JUMPXEQKN;
                cid = try getConstantIndex(self, right);
            },
            .string => {
                opc = .JUMPXEQKS;
                cid = try getConstantIndex(self, right);
            },
            // integer constants have no JUMPXEQK form; upstream (LuauIntegerType2 off) reports the constant limit error
            else => {},
        }

        if (cid < 0) return constantLimit(self, node.location);

        const jump_label = self.bytecode.emitLabel();

        try self.bytecode.emitAD(opc, rl, 0);
        try self.bytecode.emitAux(@as(u32, @intCast(cid)) | flip);

        return jump_label;
    } else {
        const opc = getJumpOpCompare(expr.op, not_);

        const rr = try compileExprAuto(self, right);

        const jump_label = self.bytecode.emitLabel();

        if (expr.op == .compare_gt or expr.op == .compare_ge) {
            try self.bytecode.emitAD(opc, rr, 0);
            try self.bytecode.emitAux(rl);
        } else {
            try self.bytecode.emitAD(opc, rl, 0);
            try self.bytecode.emitAux(rr);
        }

        return jump_label;
    }
}

// compile expr to target temp register
// if the expr (or not expr if onlyTruth is false) is truthy, jump via skipJump
// if the expr (or not expr if onlyTruth is false) is falsy, fall through (target isn't guaranteed to be updated in this case)
// if target is omitted, then the jump behavior is the same - skipJump or fallthrough depending on the truthiness of the expression
fn compileConditionValue(self: *Compiler, node: *Expr, target: ?u8, skip_jump: *std.ArrayList(usize), only_truth: bool) Error!void {
    // Optimization: we don't need to compute constant values
    if (self.constants.get(node)) |cv| {
        if (cv.known()) {
            // note that we only need to compute the value if it's truthy; otherwise we cal fall through
            if (cv.isTruthful() == only_truth) {
                if (target) |t| try compileExprTemp(self, node, t);

                try skip_jump.append(self.gpa, self.bytecode.emitLabel());
                try self.bytecode.emitAD(.JUMP, 0, 0);
            }
            return;
        }
    }

    switch (node.data) {
        .binary => |expr| switch (expr.op) {
            .@"and", .@"or" => {
                // disambiguation: there's 4 cases (we only need truthy or falsy results based on onlyTruth)
                // onlyTruth = 1: a and b transforms to a ? b : dontcare
                // onlyTruth = 1: a or b transforms to a ? a : b
                // onlyTruth = 0: a and b transforms to !a ? a : b
                // onlyTruth = 0: a or b transforms to !a ? b : dontcare
                if (only_truth == (expr.op == .@"and")) {
                    // we need to compile the left hand side, and skip to "dontcare" (aka fallthrough of the entire statement) if it's not the same as
                    // onlyTruth if it's the same then the result of the expression is the right hand side because of this, we *never* care about the
                    // result of the left hand side
                    var else_jump: std.ArrayList(usize) = .empty;
                    try compileConditionValue(self, expr.left, null, &else_jump, !only_truth);

                    // fallthrough indicates that we need to compute & return the right hand side
                    // we use compileConditionValue again to process any extra and/or statements directly
                    try compileConditionValue(self, expr.right, target, skip_jump, only_truth);

                    const else_label = self.bytecode.emitLabel();

                    try patchJumps(self, node.location, else_jump.items, else_label);
                } else {
                    // we need to compute the left hand side first; note that we will jump to skipJump if we know the answer
                    try compileConditionValue(self, expr.left, target, skip_jump, only_truth);

                    // we will fall through if computing the left hand didn't give us an "interesting" result
                    // we still use compileConditionValue to recursively optimize any and/or/compare statements
                    try compileConditionValue(self, expr.right, target, skip_jump, only_truth);
                }
                return;
            },
            .compare_ne, .compare_eq, .compare_lt, .compare_le, .compare_gt, .compare_ge => {
                if (target) |t| {
                    // since target is a temp register, we'll initialize it to 1, and then jump if the comparison is true
                    // if the comparison is false, we'll fallthrough and target will still be 1 but target has unspecified value for falsy results
                    // when we only care about falsy values instead of truthy values, the process is the same but with flipped conditionals
                    try self.bytecode.emitABC(.LOADB, t, @intFromBool(only_truth), 0);
                }

                const jump_label = try compileCompareJump(self, node, !only_truth);

                try skip_jump.append(self.gpa, jump_label);
                return;
            },
            // fall-through to default path below
            else => {},
        },
        .unary => |expr| {
            // if we *do* need to compute the target, we'd have to inject "not" ops on every return path
            // this is possible but cumbersome; so for now we only optimize not expression when we *don't* need the value
            if (target == null and expr.op == .not) {
                try compileConditionValue(self, expr.expr, target, skip_jump, !only_truth);
                return;
            }
        },
        .group => |g| {
            try compileConditionValue(self, g, target, skip_jump, only_truth);
            return;
        },
        else => {},
    }

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    var reg: u8 = undefined;

    if (target) |t| {
        reg = t;
        try compileExprTemp(self, node, reg);
    } else {
        reg = try compileExprAuto(self, node);
    }

    try skip_jump.append(self.gpa, self.bytecode.emitLabel());
    try self.bytecode.emitAD(if (only_truth) .JUMPIF else .JUMPIFNOT, reg, 0);
}

// checks if compiling the expression as a condition value generates code that's faster than using compileExpr
fn isConditionFast(self: *Compiler, node: *Expr) bool {
    if (self.constants.get(node)) |cv| {
        if (cv.known()) return true;
    }

    switch (node.data) {
        .binary => |expr| return switch (expr.op) {
            .@"and", .@"or", .compare_ne, .compare_eq, .compare_lt, .compare_le, .compare_gt, .compare_ge => true,
            else => false,
        },
        .group => |g| return isConditionFast(self, g),
        else => return false,
    }
}

fn compileExprAndOr(self: *Compiler, node: *Expr, target: u8, target_temp: bool) Error!void {
    const expr = node.data.binary;
    const and_ = (expr.op == .@"and");

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    // Optimization: when left hand side is a constant, we can emit left hand side or right hand side
    if (self.constants.get(expr.left)) |cl| {
        if (cl.known()) {
            try compileExpr(self, if (and_ == cl.isTruthful()) expr.right else expr.left, target, target_temp);
            return;
        }
    }

    // Note: two optimizations below can lead to inefficient codegen when the left hand side is a condition
    if (!isConditionFast(self, expr.left)) {
        // Optimization: when right hand side is a local variable, we can use AND/OR
        if (getExprLocalReg(self, expr.right)) |rr| {
            const lr = try compileExprAuto(self, expr.left);

            try self.bytecode.emitABC(if (and_) .AND else .OR, target, lr, rr);
            return;
        }

        // Optimization: when right hand side is a constant, we can use ANDK/ORK
        const cid = try getConstantIndex(self, expr.right);

        if (cid >= 0 and cid <= 255) {
            const lr = try compileExprAuto(self, expr.left);

            try self.bytecode.emitABC(if (and_) .ANDK else .ORK, target, lr, @intCast(cid));
            return;
        }
    }

    // Optimization: if target is a temp register, we can clobber it which allows us to compute the result directly into it
    // If it's not a temp register, then something like `a = a > 1 or a + 2` may clobber `a` while evaluating left hand side, and `a+2` will break
    const reg: u8 = if (target_temp) target else try allocReg(self, node.location, 1);

    var skip_jump: std.ArrayList(usize) = .empty;
    try compileConditionValue(self, expr.left, reg, &skip_jump, !and_);

    try compileExprTemp(self, expr.right, reg);

    const move_label = self.bytecode.emitLabel();

    try patchJumps(self, node.location, skip_jump.items, move_label);

    if (target != reg) try self.bytecode.emitABC(.MOVE, target, reg, 0);
}

fn compileExprUnary(self: *Compiler, node: *Expr, target: u8) Error!void {
    const expr = node.data.unary;
    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    // Special case for integer constants, like -1000000000i
    if (self.options.flags.LuauIntegerType2 and expr.op == .minus and expr.expr.data == .constant_integer) {
        const cid = try checkedConstant(self, try self.bytecode.addConstantInteger(@bitCast(~@as(u64, @bitCast(expr.expr.data.constant_integer)) +% 1)), node.location);
        try emitLoadK(self, target, cid);
        return;
    }

    const re = try compileExprAuto(self, expr.expr);

    try self.bytecode.emitABC(getUnaryOp(expr.op), target, re, 0);
}

fn compileExprBinary(self: *Compiler, node: *Expr, target: u8, target_temp: bool) Error!void {
    const expr = node.data.binary;
    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    switch (expr.op) {
        .add, .sub, .mul, .div, .floor_div, .mod, .pow => {
            const rc = try getConstantNumber(self, expr.right);

            if (rc >= 0 and rc <= 255) {
                const rl = try compileExprAuto(self, expr.left);

                try self.bytecode.emitABC(getBinaryOpArith(expr.op, true), target, rl, @intCast(rc));

                try hintTemporaryExprRegType(self, expr.left, rl, bc.LBC_TYPE_NUMBER, 1);
            } else {
                if (expr.op == .sub or expr.op == .div) {
                    const lc = try getConstantNumber(self, expr.left);

                    if (lc >= 0 and lc <= 255) {
                        const rr = try compileExprAuto(self, expr.right);
                        const op: Op = if (expr.op == .sub) .SUBRK else .DIVRK;

                        try self.bytecode.emitABC(op, target, @intCast(lc), rr);

                        try hintTemporaryExprRegType(self, expr.right, rr, bc.LBC_TYPE_NUMBER, 1);
                        return;
                    }
                } else if (self.options.optimization_level >= 2 and (expr.op == .add or expr.op == .mul)) {
                    // Optimization: replace k*r with r*k when r is known to be a number (otherwise metamethods may be called)
                    if (self.expr_types.get(node)) |ty| {
                        // Note: for vectors, it only makes sense to do for a multiplication as number+vector is an error
                        if (ty == bc.LBC_TYPE_NUMBER or (ty == bc.LBC_TYPE_VECTOR and expr.op == .mul)) {
                            const lc = try getConstantNumber(self, expr.left);

                            if (lc >= 0 and lc <= 255) {
                                const rr = try compileExprAuto(self, expr.right);

                                try self.bytecode.emitABC(getBinaryOpArith(expr.op, true), target, rr, @intCast(lc));

                                try hintTemporaryExprRegType(self, expr.right, rr, bc.LBC_TYPE_NUMBER, 1);
                                return;
                            }
                        }
                    }
                }

                const rl = try compileExprAuto(self, expr.left);
                const rr = try compileExprAuto(self, expr.right);

                try self.bytecode.emitABC(getBinaryOpArith(expr.op, false), target, rl, rr);

                try hintTemporaryExprRegType(self, expr.left, rl, bc.LBC_TYPE_NUMBER, 1);
                try hintTemporaryExprRegType(self, expr.right, rr, bc.LBC_TYPE_NUMBER, 1);
            }
        },
        .concat => {
            var args: std.ArrayList(*Expr) = .empty;
            try args.append(self.gpa, expr.left);
            try args.append(self.gpa, expr.right);

            // unroll the tree of concats down the right hand side to be able to do multiple ops
            while (true) {
                const last = args.items[args.items.len - 1];
                if (last.data != .binary or last.data.binary.op != .concat) break;
                args.items[args.items.len - 1] = last.data.binary.left;
                try args.append(self.gpa, last.data.binary.right);
            }

            const regs = try allocReg(self, node.location, @intCast(args.items.len));

            for (args.items, 0..) |arg, i| {
                if (self.options.flags.LuauCompileConcatTargetTop)
                    try compileExprTempTop(self, arg, regs + @as(u8, @intCast(i)))
                else
                    try compileExprTemp(self, arg, regs + @as(u8, @intCast(i)));
            }

            try self.bytecode.emitABC(.CONCAT, target, regs, regs + @as(u8, @intCast(args.items.len - 1)));
        },
        .compare_ne, .compare_eq, .compare_lt, .compare_le, .compare_gt, .compare_ge => {
            const jump_label = try compileCompareJump(self, node, false);

            // note: this skips over the next LOADB instruction because of "1" in the C slot
            try self.bytecode.emitABC(.LOADB, target, 0, 1);

            const then_label = self.bytecode.emitLabel();

            try self.bytecode.emitABC(.LOADB, target, 1, 0);

            try patchJump(self, node.location, jump_label, then_label);
        },
        .@"and", .@"or" => try compileExprAndOr(self, node, target, target_temp),
    }
}

fn compileExprIfElseAndOr(self: *Compiler, and_: bool, creg: u8, other: *Expr, target: u8) Error!void {
    const cid = try getConstantIndex(self, other);

    if (cid >= 0 and cid <= 255) {
        try self.bytecode.emitABC(if (and_) .ANDK else .ORK, target, creg, @intCast(cid));
    } else {
        const old_top = self.reg_top;
        defer self.reg_top = old_top;
        const oreg = try compileExprAuto(self, other);

        try self.bytecode.emitABC(if (and_) .AND else .OR, target, creg, oreg);
    }
}

fn compileExprIfElseLocal(self: *Compiler, node: *Expr, target: u8, target_temp: bool) Error!void {
    const expr = node.data.if_else;
    std.debug.assert(expr.condition_local != null);

    const skip_else = isConstantTrue(self, expr.condition);
    const old_locals = self.local_stack.items.len;
    var else_jump: usize = 0;

    {
        const old_top = self.reg_top;
        defer self.reg_top = old_top;

        const reg = try allocReg(self, node.location, 1);
        const allocpc = self.bytecode.getDebugPC();

        try compileExprTemp(self, expr.condition, reg);
        try pushLocal(self, expr.condition_local.?, reg, allocpc);

        if (!skip_else) {
            else_jump = self.bytecode.emitLabel();
            try self.bytecode.emitAD(.JUMPIFNOT, reg, 0);
        }

        try compileExpr(self, expr.true_expr, target, target_temp);

        try closeLocals(self, old_locals);
        try popLocals(self, old_locals);
    }

    if (skip_else) return;

    // Jump over else expression evaluation
    const then_label = self.bytecode.emitLabel();
    try self.bytecode.emitAD(.JUMP, 0, 0);

    const else_label = self.bytecode.emitLabel();
    try compileExpr(self, expr.false_expr, target, target_temp);
    const end_label = self.bytecode.emitLabel();

    try patchJump(self, node.location, else_jump, else_label);
    try patchJump(self, node.location, then_label, end_label);
}

fn compileExprIfElse(self: *Compiler, node: *Expr, target: u8, target_temp: bool) Error!void {
    const expr = node.data.if_else;
    if (self.options.flags.LuauExperimentalIfLocalSyntax and expr.condition_local != null) return compileExprIfElseLocal(self, node, target, target_temp);

    if (isConstant(self, expr.condition)) {
        if (isConstantTrue(self, expr.condition)) {
            try compileExpr(self, expr.true_expr, target, target_temp);
        } else {
            try compileExpr(self, expr.false_expr, target, target_temp);
        }
    } else {
        // Optimization: convert some if..then..else expressions into and/or when the other side has no side effects and is very cheap to compute
        // if v then v else e => v or e
        // if v then e else v => v and e
        if (getExprLocalReg(self, expr.condition)) |creg| {
            if (isExprLocalReg(self, expr.true_expr, creg) and (getExprLocalReg(self, expr.false_expr) != null or isConstant(self, expr.false_expr)))
                return compileExprIfElseAndOr(self, false, creg, expr.false_expr, target)
            else if (isExprLocalReg(self, expr.false_expr, creg) and (getExprLocalReg(self, expr.true_expr) != null or isConstant(self, expr.true_expr)))
                return compileExprIfElseAndOr(self, true, creg, expr.true_expr, target);
        }

        var else_jump: std.ArrayList(usize) = .empty;
        try compileConditionValue(self, expr.condition, null, &else_jump, false);
        try compileExpr(self, expr.true_expr, target, target_temp);

        // Jump over else expression evaluation
        const then_label = self.bytecode.emitLabel();
        try self.bytecode.emitAD(.JUMP, 0, 0);

        const else_label = self.bytecode.emitLabel();
        try compileExpr(self, expr.false_expr, target, target_temp);
        const end_label = self.bytecode.emitLabel();

        try patchJumps(self, node.location, else_jump.items, else_label);
        try patchJump(self, node.location, then_label, end_label);
    }
}

fn escapeAndAppend(gpa: Allocator, buffer: *std.ArrayList(u8), str: []const u8) Error!void {
    for (str) |character| {
        try buffer.append(gpa, character);
        if (character == '%') try buffer.append(gpa, '%');
    }
}

fn compileExprInterpString(self: *Compiler, node: *Expr, target: u8, target_temp: bool) Error!void {
    const expr = node.data.interp_string;

    var skipped_sub_expr: usize = 0;
    var format_string: std.ArrayList(u8) = .empty;

    std.debug.assert(expr.strings.len == expr.expressions.len + 1);
    for (expr.strings, 0..) |string, idx| {
        try escapeAndAppend(self.gpa, &format_string, string);

        if (idx < expr.expressions.len) {
            const c = self.constants.get(expr.expressions[idx]);
            if (c != null and c.? == .string) {
                try escapeAndAppend(self.gpa, &format_string, c.?.string);
                skipped_sub_expr += 1;
            } else {
                try format_string.appendSlice(self.gpa, "%*");
            }
        }
    }

    const format_string_index: i32 = if (format_string.items.len == 0)
        try self.bytecode.addConstantString("")
    else
        try self.bytecode.addConstantString(try self.names.getOrAdd(format_string.items));

    if (format_string_index < 0) return constantLimit(self, node.location);

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    const reg_count: u32 = @intCast(2 + expr.expressions.len - skipped_sub_expr);

    // Optimization: have the format call place the result directly into the target to avoid an extra MOVE
    const target_top = target_temp and @as(u32, target) == self.reg_top -% 1;
    const base_reg: u8 = if (target_top) (try allocReg(self, node.location, reg_count - 1)) - 1 else try allocReg(self, node.location, reg_count);

    try emitLoadK(self, base_reg, format_string_index);

    var skipped: usize = 0;
    for (expr.expressions, 0..) |sub_expr, index| {
        const c = self.constants.get(sub_expr);
        if (c == null or c.? != .string)
            try compileExprTempTop(self, sub_expr, @intCast(base_reg + 2 + index - skipped))
        else
            skipped += 1;
    }

    const format_method = "format";
    const format_method_index = try self.bytecode.addConstantString(format_method);
    if (format_method_index < 0) return constantLimit(self, node.location);

    try self.bytecode.emitABC(.NAMECALL, base_reg, base_reg, hash8(format_method));
    try self.bytecode.emitAux(@intCast(format_method_index));
    try self.bytecode.emitABC(.CALL, base_reg, @truncate(expr.expressions.len + 2 - skipped_sub_expr), 2);
    if (target != base_reg) try self.bytecode.emitABC(.MOVE, target, base_reg, 0);
}

fn compileExprTable(self: *Compiler, node: *Expr, target: u8, target_temp: bool, construct_reg: u8) Error!void {
    const items = node.data.table;

    // Optimization: if the table is empty, we can compute it directly into the target
    if (items.len == 0) {
        const gop = try self.table_shapes.getOrPut(self.gpa, node);
        if (!gop.found_existing) gop.value_ptr.* = .{};
        const shape = gop.value_ptr.*;

        try self.bytecode.emitABC(.NEWTABLE, target, encodeHashSize(shape.hash_size), 0);
        try self.bytecode.emitAux(shape.array_size);
        return;
    }

    var array_size: u32 = 0;
    var hash_size: u32 = 0;
    var record_size: u32 = 0;
    var index_size: u32 = 0;

    for (items) |item| {
        array_size += @intFromBool(item.kind == .list);
        hash_size += @intFromBool(item.kind != .list);
        record_size += @intFromBool(item.kind == .record);
    }

    // Optimization: allocate sequential explicitly specified numeric indices ([1]) as arrays
    if (array_size == 0 and hash_size > 0) {
        for (items) |item| {
            const ckey = self.constants.get(item.key.?);
            index_size += @intFromBool(ckey != null and ckey.? == .number and ckey.?.number == @as(f64, @floatFromInt(index_size + 1)));
        }

        // we only perform the optimization if we don't have any other []-keys
        // technically it's "safe" to do this even if we have other keys, but doing so changes iteration order and may break existing code
        if (hash_size == record_size + index_size)
            hash_size = record_size
        else
            index_size = 0;
    }

    const encoded_hash_size = encodeHashSize(hash_size);

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    // Optimization: if target is a temp register, we can clobber it which allows us to compute the result directly into it
    const reg: u8 = if (target_temp) target else try allocReg(self, node.location, 1);

    // flattening operation where we only load the last element
    // this optimizes for tables like: { data = 43, data = "true", data = 9 }
    // this does not optimize for tables such as: { data = 43, data = function() end, data = 9}
    // in this case, we know that data = 9 should be the element, so we can just skip the rest
    const KeyVal = struct { key: i32, val: i32 };
    var last_key_val: std.ArrayList(KeyVal) = .empty; // insertion ordered
    const findKey = struct {
        fn f(list: []KeyVal, key: i32) ?*KeyVal {
            for (list) |*kv| if (kv.key == key) return kv;
            return null;
        }
    }.f;

    // Optimization: when all items are record fields, use template tables to compile expression
    if (array_size == 0 and index_size == 0 and hash_size == record_size and record_size >= 1 and record_size <= builder_mod.TableShape.kMaxLength) {
        var shape: builder_mod.TableShape = .{};

        for (items) |item| {
            std.debug.assert(item.kind == .record);
            const ckey = item.key.?;

            const key_cid = try addString(self, ckey.data.constant_string, ckey.location);

            const value_cid = try getConstantIndex(self, item.value);
            if (findKey(last_key_val.items, key_cid)) |existing| {
                if (existing.val == -1) continue;
                existing.val = value_cid;
            } else {
                try last_key_val.append(self.gpa, .{ .key = key_cid, .val = value_cid });
            }
        }

        for (last_key_val.items) |kv| {
            std.debug.assert(shape.length < builder_mod.TableShape.kMaxLength);

            const idx = shape.length;
            shape.keys[idx] = kv.key;

            shape.constants[idx] = kv.val;
            if (kv.val >= 0) shape.has_constants = true;

            shape.length += 1;
        }

        const tid = try self.bytecode.addConstantTable(shape);
        if (tid < 0) return constantLimit(self, node.location);

        if (self.options.flags.DebugLuauUserDefinedClasses and construct_reg != kInvalidReg) {
            const fb_slot = try self.bytecode.addConstructSlot(@intCast(tid));
            try self.bytecode.emitABC(.CONSTRUCT, reg, construct_reg, 0);
            try self.bytecode.emitAux(fb_slot);
        } else if (tid < 32768) {
            try self.bytecode.emitAD(.DUPTABLE, reg, @intCast(tid));
        } else {
            // must disable duptable constant optimization here, as we're defaulting back to new table
            last_key_val.clearRetainingCapacity();

            try self.bytecode.emitABC(.NEWTABLE, reg, encoded_hash_size, 0);
            try self.bytecode.emitAux(0);
        }
    } else {
        // Optimization: instead of allocating one extra element when the last element of the table literal is ..., let SETLIST allocate the
        // correct amount of storage
        const last = items[items.len - 1];

        const trailing_varargs = last.kind == .list and last.value.data == .varargs;
        std.debug.assert(!trailing_varargs or array_size > 0);

        const array_allocation = array_size - @intFromBool(trailing_varargs) + index_size;

        try self.bytecode.emitABC(.NEWTABLE, reg, encoded_hash_size, 0);
        try self.bytecode.emitAux(array_allocation);
    }

    const array_chunk_size = @min(16, array_size);
    const array_chunk_reg = try allocReg(self, node.location, array_chunk_size);
    var array_chunk_current: u32 = 0;

    var array_index: u32 = 1;
    var mult_ret = false;

    for (items, 0..) |item, i| {
        const key = item.key;
        const value = item.value;

        if (last_key_val.items.len > 0 and key != null and key.?.data == .constant_string) {
            const key_cid = try self.bytecode.addConstantString(key.?.data.constant_string);
            if (findKey(last_key_val.items, key_cid)) |kv| {
                // do not generate assignments for constants
                if (kv.val >= 0) continue;
            }
        }

        // some key/value pairs don't require us to compile the expressions, so we need to setup the line info here
        setDebugLine(self, value.location);

        if (self.options.coverage_level >= 2) try self.bytecode.emitABC(.COVERAGE, 0, 0, 0);

        // flush array chunk on overflow or before hash keys to maintain insertion order
        if (array_chunk_current > 0 and (key != null or array_chunk_current == array_chunk_size)) {
            try self.bytecode.emitABC(.SETLIST, reg, array_chunk_reg, @intCast(array_chunk_current + 1));
            try self.bytecode.emitAux(array_index);
            array_index += array_chunk_current;
            array_chunk_current = 0;
        }

        // items with a key are set one by one via SETTABLE/SETTABLEKS/SETTABLEN
        if (key) |k| {
            const inner_top = self.reg_top;
            defer self.reg_top = inner_top;

            const lv = try compileLValueIndex(self, reg, k);
            const rv = try compileExprAuto(self, value);

            try compileAssign(self, lv, rv, null);
        }
        // items without a key are set using SETLIST so that we can initialize large arrays quickly
        else {
            const temp: u8 = @intCast(array_chunk_reg + array_chunk_current);

            if (i + 1 == items.len)
                mult_ret = try compileExprTempMultRet(self, value, temp)
            else
                try compileExprTempTop(self, value, temp);

            array_chunk_current += 1;
        }
    }

    // flush last array chunk; note that this needs multret handling if the last expression was multret
    if (array_chunk_current != 0) {
        try self.bytecode.emitABC(.SETLIST, reg, array_chunk_reg, if (mult_ret) 0 else @intCast(array_chunk_current + 1));
        try self.bytecode.emitAux(array_index);
    }

    if (target != reg) try self.bytecode.emitABC(.MOVE, target, reg, 0);
}

fn canImport(self: *Compiler, name: []const u8) bool {
    return self.options.optimization_level >= 1 and an.getGlobalState(&self.globals, name) != .written;
}

fn canImportChain(self: *Compiler, name: []const u8) bool {
    return self.options.optimization_level >= 1 and an.getGlobalState(&self.globals, name) == .default;
}

fn compileExprIndexName(self: *Compiler, node: *Expr, target: u8, target_temp: bool) Error!void {
    const expr = node.data.index_name;
    setDebugLine(self, node.location); // normally compileExpr sets up line info, but compileExprIndexName can be called directly

    // Optimization: index chains that start from global variables can be compiled into GETIMPORT statement
    var import_root: ?[]const u8 = null;
    var import1: *const ast.IndexName = undefined;
    var import2: ?*const ast.IndexName = null;

    if (expr.expr.data == .index_name) {
        const index = &expr.expr.data.index_name;
        if (index.expr.data == .global) import_root = index.expr.data.global;
        import1 = index;
        import2 = &node.data.index_name;
    } else {
        if (expr.expr.data == .global) import_root = expr.expr.data.global;
        import1 = &node.data.index_name;
    }

    if (import_root) |root| {
        if (canImportChain(self, root)) {
            if (self.options.flags.DebugLuauUserDefinedClasses and (import1.private_access or (import2 != null and import2.?.private_access)))
                return raise(self, node.location, "Attempt to access private field of a global", .{});

            const id0 = try self.bytecode.addConstantString(root);
            const id1 = try self.bytecode.addConstantString(import1.index);
            const id2: i32 = if (import2) |im2| try self.bytecode.addConstantString(im2.index) else -1;

            if (id0 < 0 or id1 < 0 or (import2 != null and id2 < 0)) return constantLimit(self, node.location);

            // Note: GETIMPORT encoding is limited to 10 bits per object id component
            if (id0 < 1024 and id1 < 1024 and id2 < 1024) {
                const iid = if (import2 != null) BytecodeBuilder.getImportId3(id0, id1, id2) else BytecodeBuilder.getImportId2(id0, id1);
                const cid = try self.bytecode.addImport(iid);

                if (cid >= 0 and cid < 32768) {
                    try self.bytecode.emitAD(.GETIMPORT, target, @intCast(cid));
                    try self.bytecode.emitAux(iid);
                    return;
                }
            }
        }
    }

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    var reg = target;

    if (getExprLocalReg(self, expr.expr)) |local_reg| {
        // Locals can be indexed directly
        reg = local_reg;
    } else if (target_temp and self.options.flags.LuauCompileMoveElision) {
        // If target is a temp register, we can compute the result directly into it
        reg = try compileExprAutoTemp(self, expr.expr, target, kInvalidReg);
    } else if (target_temp) {
        try compileExprTemp(self, expr.expr, target);
    } else {
        reg = try compileExprAuto(self, expr.expr);
    }

    setDebugLine(self, expr.index_location);

    if (self.options.flags.DebugLuauUserDefinedClasses and expr.private_access) {
        // the parser only sets private_access inside a class body, so curr_class is set here
        std.debug.assert(self.curr_class != null);

        return compilePrivateFieldAccess(self, node.location, expr.index, target, reg, false);
    }

    const cid = try addString(self, expr.index, node.location);

    try self.bytecode.emitABC(.GETTABLEKS, target, reg, hash8(expr.index));
    try self.bytecode.emitAux(@intCast(cid));

    try hintTemporaryExprRegType(self, expr.expr, reg, bc.LBC_TYPE_TABLE, 2);
}

fn isSmallIndexConstant(cv: Constant) bool {
    if (cv != .number) return false;
    const v = cv.number;
    return v >= 1 and v <= 256 and @as(f64, @floatFromInt(@as(i32, @intFromFloat(v)))) == v;
}

fn compileExprIndexExpr(self: *Compiler, node: *Expr, target: u8) Error!void {
    const expr = node.data.index_expr;
    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    const cv = getConstant(self, expr.index);

    if (isSmallIndexConstant(cv)) {
        const i: u8 = @intCast(@as(i32, @intFromFloat(cv.number)) - 1);

        const rt = try compileExprAuto(self, expr.expr);

        setDebugLine(self, expr.index.location);

        try self.bytecode.emitABC(.GETTABLEN, target, rt, i);

        try hintTemporaryExprRegType(self, expr.expr, rt, bc.LBC_TYPE_TABLE, 1);
    } else if (cv == .string) {
        const iname = cv.string;
        const cid = try addString(self, iname, node.location);

        const rt = try compileExprAuto(self, expr.expr);

        setDebugLine(self, expr.index.location);

        try self.bytecode.emitABC(.GETTABLEKS, target, rt, hash8(iname));
        try self.bytecode.emitAux(@intCast(cid));

        try hintTemporaryExprRegType(self, expr.expr, rt, bc.LBC_TYPE_TABLE, 2);
    } else {
        const rt = try compileExprAuto(self, expr.expr);
        const ri = try compileExprAuto(self, expr.index);

        try self.bytecode.emitABC(.GETTABLE, target, rt, ri);

        try hintTemporaryExprRegType(self, expr.expr, rt, bc.LBC_TYPE_TABLE, 1);
        try hintTemporaryExprRegType(self, expr.index, ri, bc.LBC_TYPE_NUMBER, 1);
    }
}

fn compileExprGlobal(self: *Compiler, node: *Expr, target: u8) Error!void {
    const name = node.data.global;
    // Optimization: builtin globals can be retrieved using GETIMPORT
    if (canImport(self, name)) {
        const id0 = try addString(self, name, node.location);

        // Note: GETIMPORT encoding is limited to 10 bits per object id component
        if (id0 < 1024) {
            const iid = BytecodeBuilder.getImportId1(id0);
            const cid = try self.bytecode.addImport(iid);

            if (cid >= 0 and cid < 32768) {
                try self.bytecode.emitAD(.GETIMPORT, target, @intCast(cid));
                try self.bytecode.emitAux(iid);
                return;
            }
        }
    }

    const cid = try addString(self, name, node.location);

    try self.bytecode.emitABC(.GETGLOBAL, target, 0, hash8(name));
    try self.bytecode.emitAux(@intCast(cid));
}

fn compileExprConstant(self: *Compiler, node: *Expr, cv: Constant, target: u8) Error!void {
    switch (cv) {
        .nil => try self.bytecode.emitABC(.LOADNIL, target, 0, 0),
        .boolean => |b| try self.bytecode.emitABC(.LOADB, target, @intFromBool(b), 0),
        .number => |d| {
            if (d >= -32768.0 and d <= 32767.0 and @as(f64, @floatFromInt(@as(i16, @intFromFloat(d)))) == d and !(d == 0.0 and std.math.signbit(d))) {
                // short number encoding: doesn't require a table entry lookup
                try self.bytecode.emitAD(.LOADN, target, @intFromFloat(d));
            } else {
                // long number encoding: use generic constant path
                const cid = try checkedConstant(self, try self.bytecode.addConstantNumber(d), node.location);
                try emitLoadK(self, target, cid);
            }
        },
        .string => |s| {
            const cid = try addString(self, s, node.location);
            try emitLoadK(self, target, cid);
        },
        .integer => |l| {
            const cid = try checkedConstant(self, try self.bytecode.addConstantInteger(l), node.location);
            try emitLoadK(self, target, cid);
        },
        .vectorf => |v| {
            const cid = try checkedConstant(self, try self.bytecode.addConstantVectorf(v[0], v[1], v[2], v[3]), node.location);
            try emitLoadK(self, target, cid);
        },
        .vectord => |v| {
            const cid = try checkedConstant(self, try self.bytecode.addConstantVectord(v[0], v[1], v[2], v[3]), node.location);
            try emitLoadK(self, target, cid);
        },
        .unknown, .table => unreachable,
    }
}

fn compileExpr(self: *Compiler, node: *Expr, target: u8, target_temp: bool) Error!void {
    setDebugLine(self, node.location);

    if (self.options.coverage_level >= 2) try self.bytecode.emitABC(.COVERAGE, 0, 0, 0);

    // Optimization: if expression has a constant value, we can emit it directly
    if (self.constants.get(node)) |cv| {
        if (cv.known()) {
            try compileExprConstant(self, node, cv, target);
            return;
        }
    }

    switch (node.data) {
        .group => |g| try compileExpr(self, g, target, target_temp),
        .constant_nil => try self.bytecode.emitABC(.LOADNIL, target, 0, 0),
        .constant_bool => |b| try self.bytecode.emitABC(.LOADB, target, @intFromBool(b), 0),
        .constant_number => |n| {
            const cid = try checkedConstant(self, try self.bytecode.addConstantNumber(n), node.location);
            try emitLoadK(self, target, cid);
        },
        .constant_integer => |n| {
            const cid = try checkedConstant(self, try self.bytecode.addConstantInteger(n), node.location);
            try emitLoadK(self, target, cid);
        },
        .constant_string => |s| {
            const cid = try addString(self, s, node.location);
            try emitLoadK(self, target, cid);
        },
        .local => |l| {
            if (self.options.flags.LuauExportValueSyntax and l.local.is_exported and !self.exports.classes.contains(l.local) and
                (!self.options.flags.LuauOptimizeExportTable or !self.exports.exported.functions.contains(l.local)))
            {
                const name = l.local.name;
                const cid = try addString(self, name, node.location);

                if (getLocalReg(self, &self.exports.table_local)) |table_reg| {
                    try self.bytecode.emitABC(.GETTABLEKS, target, table_reg, hash8(name));
                    try self.bytecode.emitAux(@bitCast(cid));
                } else {
                    // we must reuse the target register for the export table lookup
                    const upval = try getUpval(self, &self.exports.table_local);
                    try self.bytecode.emitABC(.GETUPVAL, target, upval, 0);
                    try self.bytecode.emitABC(.GETTABLEKS, target, target, hash8(name));
                    try self.bytecode.emitAux(@bitCast(cid));
                }
                return;
            }

            // note: this can't check expr->upvalue because upvalues may be upgraded to locals during inlining
            if (getExprLocalReg(self, node)) |reg| {
                // Optimization: we don't need to move if target happens to be in the same register
                if (self.options.optimization_level == 0 or target != reg) try self.bytecode.emitABC(.MOVE, target, reg, 0);
            } else {
                std.debug.assert(l.upvalue);
                const uid = try getUpval(self, l.local);

                try self.bytecode.emitABC(.GETUPVAL, target, uid, 0);
            }
        },
        .global => try compileExprGlobal(self, node, target),
        .varargs => try compileExprVarargs(self, node, target, 1, false),
        .call => {
            // Optimization: when targeting temporary registers, we can compile call in a special mode that doesn't require extra register moves
            if (target_temp and @as(u32, target) == self.reg_top -% 1)
                try compileExprCall(self, node, target, 1, true, false)
            else
                try compileExprCall(self, node, target, 1, false, false);
        },
        .index_name => try compileExprIndexName(self, node, target, target_temp),
        .index_expr => try compileExprIndexExpr(self, node, target),
        .function => try compileExprFunction(self, node, target),
        .table => try compileExprTable(self, node, target, target_temp, kInvalidReg),
        .unary => try compileExprUnary(self, node, target),
        .binary => try compileExprBinary(self, node, target, target_temp),
        .type_assertion => |t| try compileExpr(self, t.expr, target, target_temp),
        .if_else => try compileExprIfElse(self, node, target, target_temp),
        .interp_string => try compileExprInterpString(self, node, target, target_temp),
        .instantiate => |i| try compileExpr(self, i.expr, target, target_temp),
    }
}

fn compileExprTemp(self: *Compiler, node: *Expr, target: u8) Error!void {
    return compileExpr(self, node, target, true);
}

/// Upstream compileExprAuto; the register is owned by the caller's RegScope (reg_top restore).
fn compileExprAuto(self: *Compiler, node: *Expr) Error!u8 {
    // Optimization: directly return locals instead of copying them to a temporary
    // This can affect side-effect ordering on the register if it is captured
    if (getExprLocalReg(self, node)) |local_reg| return local_reg;

    // note: the register is owned by the parent scope
    const reg = try allocReg(self, node.location, 1);

    if (self.options.flags.LuauCompileMoveElision) return compileExprAutoTemp(self, node, reg, kInvalidReg);

    try compileExprTemp(self, node, reg);

    return reg;
}

/// LuauCompileMoveElision: compiles `node` into `target`, or returns the source register when the result is a plain register move
fn compileExprAutoTemp(self: *Compiler, node: *Expr, target_in: u8, forbidden_source_reg: u8) Error!u8 {
    std.debug.assert(self.options.flags.LuauCompileMoveElision);
    var target = target_in;

    if (self.options.optimization_level == 0) {
        try compileExprTemp(self, node, target);
        return target;
    }

    const marker = self.bytecode.emitLabel();
    try compileExprTemp(self, node, target);

    // If only a single instruction was generated, check if it's a move of a register that we can elide
    if (self.bytecode.emitLabel() == marker + 1) {
        const insn = self.bytecode.lastInstruction();

        if (bc.insnOp(insn) == @intFromEnum(Op.MOVE) and bc.insnA(insn) == target) {
            const src = bc.insnB(insn);

            // Captured registers might get modified while the operation is lowered and rely on the move copy to prevent that
            if (!self.reg_captured[src] and src != forbidden_source_reg) {
                self.bytecode.undoEmit(.MOVE);
                target = src;
            }
        }
    }

    return target;
}

fn compileExprSide(self: *Compiler, node: *Expr) Error!void {
    // Optimization: some expressions never carry side effects so we don't need to emit any code
    switch (node.data) {
        .local, .global, .varargs, .function => return,
        else => {},
    }
    if (isConstant(self, node)) return;

    const old_top = self.reg_top;
    defer self.reg_top = old_top;
    _ = try compileExprAuto(self, node);
}

// initializes target..target+targetCount-1 range using expression
// if expression is a call/vararg, we assume it returns all values, otherwise we fill the rest with nil
// assumes target register range can be clobbered and is at the top of the register space if targetTop = true
fn compileExprTempN(self: *Compiler, node: *Expr, target: u8, target_count: u8, target_top: bool) Error!void {
    // we assume that target range is at the top of the register space and can be clobbered
    // this is what allows us to compile the last call expression - if it's a call - using targetTop=true
    std.debug.assert(!target_top or @as(u32, target) + target_count == self.reg_top);

    // LOP_CALL/LOP_GETVARARGS encoding uses 255 to signal a multret
    if (target_count == 255) return raise(self, node.location, "Exceeded result count limit; simplify the code to compile", .{});

    switch (node.data) {
        .call => try compileExprCall(self, node, target, target_count, target_top, false),
        .varargs => try compileExprVarargs(self, node, target, target_count, false),
        else => {
            try compileExprTemp(self, node, target);

            var i: u32 = 1;
            while (i < target_count) : (i += 1) try self.bytecode.emitABC(.LOADNIL, @intCast(target + i), 0, 0);
        },
    }
}

// initializes target..target+targetCount-1 range using expressions from the list
// if list has fewer expressions, and last expression is multret, we assume it returns the rest of the values
// if list has fewer expressions, and last expression isn't multret, we fill the rest with nil
// assumes target register range can be clobbered and is at the top of the register space if targetTop = true
fn compileExprListTemp(self: *Compiler, list: []const *Expr, target: u8, target_count: u8, target_top: bool) Error!void {
    // we assume that target range is at the top of the register space and can be clobbered
    // this is what allows us to compile the last call expression - if it's a call - using targetTop=true
    std.debug.assert(!target_top or @as(u32, target) + target_count == self.reg_top);

    if (list.len == target_count) {
        for (list, 0..) |e, i| try compileExprTemp(self, e, @intCast(target + i));
    } else if (list.len > target_count) {
        for (list[0..target_count], 0..) |e, i| try compileExprTemp(self, e, @intCast(target + i));

        // evaluate extra expressions for side effects
        for (list[target_count..]) |e| try compileExprSide(self, e);
    } else if (list.len > 0) {
        for (list[0 .. list.len - 1], 0..) |e, i| try compileExprTemp(self, e, @intCast(target + i));

        try compileExprTempN(self, list[list.len - 1], @intCast(target + list.len - 1), @intCast(target_count - (list.len - 1)), target_top);
    } else {
        var i: u32 = 0;
        while (i < target_count) : (i += 1) try self.bytecode.emitABC(.LOADNIL, @intCast(target + i), 0, 0);
    }
}

// ------------------------------------------------------------------ lvalues

fn compileLValueIndex(self: *Compiler, reg: u8, index: *Expr) Error!LValue {
    const cv = getConstant(self, index);

    if (isSmallIndexConstant(cv)) {
        return .{ .kind = .index_number, .reg = reg, .number = @intCast(@as(i32, @intFromFloat(cv.number)) - 1), .location = index.location };
    } else if (cv == .string) {
        return .{ .kind = .index_name, .reg = reg, .name = cv.string, .location = index.location };
    } else {
        return .{ .kind = .index_expr, .reg = reg, .index = try compileExprAuto(self, index), .location = index.location };
    }
}

fn compileLValue(self: *Compiler, node: *Expr) Error!LValue {
    setDebugLine(self, node.location);

    switch (node.data) {
        .local => |l| {
            if (self.options.flags.LuauExportValueSyntax and l.local.is_exported and
                (!self.options.flags.LuauOptimizeExportTable or !self.exports.exported.functions.contains(l.local)))
            {
                const table_reg = try getExportTableReg(self, node.location);
                return .{ .kind = .index_name, .reg = table_reg, .name = l.local.name, .location = node.location };
            }

            // note: this can't check expr->upvalue because upvalues may be upgraded to locals during inlining
            if (getExprLocalReg(self, node)) |reg| return .{ .kind = .local, .reg = reg, .location = node.location };

            std.debug.assert(l.upvalue);
            return .{ .kind = .upvalue, .upval = try getUpval(self, l.local), .location = node.location };
        },
        .global => |g| return .{ .kind = .global, .name = g, .location = node.location },
        .index_name => |i| {
            const private = self.options.flags.DebugLuauUserDefinedClasses and i.private_access;
            return .{ .kind = if (private) .private_field else .index_name, .reg = try compileExprAuto(self, i.expr), .name = i.index, .location = node.location };
        },
        .index_expr => |i| {
            const reg = try compileExprAuto(self, i.expr);
            return compileLValueIndex(self, reg, i.index);
        },
        else => unreachable, // the parser only produces assignable expressions here
    }
}

fn compileLValueUse(self: *Compiler, lv: LValue, reg: u8, set: bool, target_expr: ?*Expr) Error!void {
    setDebugLine(self, lv.location);

    switch (lv.kind) {
        .local => {
            if (set)
                try self.bytecode.emitABC(.MOVE, lv.reg, reg, 0)
            else
                try self.bytecode.emitABC(.MOVE, reg, lv.reg, 0);
        },
        .upvalue => try self.bytecode.emitABC(if (set) .SETUPVAL else .GETUPVAL, reg, lv.upval, 0),
        .global => {
            const cid = try addString(self, lv.name, lv.location);
            try self.bytecode.emitABC(if (set) .SETGLOBAL else .GETGLOBAL, reg, 0, hash8(lv.name));
            try self.bytecode.emitAux(@intCast(cid));
        },
        .index_name => {
            const cid = try addString(self, lv.name, lv.location);
            try self.bytecode.emitABC(if (set) .SETTABLEKS else .GETTABLEKS, reg, lv.reg, hash8(lv.name));
            try self.bytecode.emitAux(@intCast(cid));

            if (target_expr) |te| {
                if (te.data == .index_name) try hintTemporaryExprRegType(self, te.data.index_name.expr, lv.reg, bc.LBC_TYPE_TABLE, 2);
            }
        },
        .index_number => {
            try self.bytecode.emitABC(if (set) .SETTABLEN else .GETTABLEN, reg, lv.reg, lv.number);

            if (target_expr) |te| {
                if (te.data == .index_expr) try hintTemporaryExprRegType(self, te.data.index_expr.expr, lv.reg, bc.LBC_TYPE_TABLE, 1);
            }
        },
        .index_expr => {
            try self.bytecode.emitABC(if (set) .SETTABLE else .GETTABLE, reg, lv.reg, lv.index);

            if (target_expr) |te| {
                if (te.data == .index_expr) {
                    try hintTemporaryExprRegType(self, te.data.index_expr.expr, lv.reg, bc.LBC_TYPE_TABLE, 1);
                    try hintTemporaryExprRegType(self, te.data.index_expr.index, lv.index, bc.LBC_TYPE_NUMBER, 1);
                }
            }
        },
        .private_field => {
            std.debug.assert(self.options.flags.DebugLuauUserDefinedClasses and self.curr_class != null);

            const verb = if (set) "assign" else "access";

            const offsets = self.class_private_field_offsets.getPtr(self.curr_class.?) orelse
                return raise(self, lv.location, "Attempt to {s} private field of a class with no private fields", .{verb});

            const field = offsets.get(lv.name) orelse return raise(self, lv.location, "Attempt to {s} undefined private field", .{verb});

            if (field.is_static and set) return raise(self, lv.location, "Attempt to assign static field of a class", .{});

            try self.bytecode.emitABC(if (set) .SETPRIVATEFIELD else .GETPRIVATEFIELD, reg, lv.reg, try getCurClassUpval(self));
            try self.bytecode.emitAux(field.offset);
        },
    }
}

fn compileAssign(self: *Compiler, lv: LValue, source: u8, target_expr: ?*Expr) Error!void {
    try compileLValueUse(self, lv, source, true, target_expr);
}

// ------------------------------------------------------------------ statements

fn isStatBreak(node: *Stat) bool {
    switch (node.data) {
        .block => |b| return b.body.len == 1 and b.body[0].data == .@"break",
        .@"break" => return true,
        else => return false,
    }
}

fn extractStatContinue(block: *Stat) ?*Stat {
    const b = block.data.block;
    if (b.body.len == 1 and b.body[0].data == .@"continue") return b.body[0];
    return null;
}

fn currentLoop(self: *Compiler) *Loop {
    return &self.loops.items[self.loops.items.len - 1];
}

fn compileStatIfLocal(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.@"if";
    std.debug.assert(stat.condition_local != null);

    const then_terminates = alwaysTerminates(self, stat.thenbody);
    const skip_else = isConstantTrue(self, stat.condition);
    const old_locals = self.local_stack.items.len;
    var else_jump: usize = 0;

    {
        const old_top = self.reg_top;
        defer self.reg_top = old_top;

        const reg = try allocReg(self, node.location, 1);
        const allocpc = self.bytecode.getDebugPC();

        try compileExprTemp(self, stat.condition, reg);
        try pushLocal(self, stat.condition_local.?, reg, allocpc);

        if (!skip_else) {
            else_jump = self.bytecode.emitLabel();
            try self.bytecode.emitAD(.JUMPIFNOT, reg, 0);
        }

        try compileStat(self, stat.thenbody);

        if (!then_terminates) try closeLocals(self, old_locals);

        try popLocals(self, old_locals);
    }

    if (skip_else) return;

    if (stat.elsebody) |elsebody| {
        if (then_terminates) {
            const else_label = self.bytecode.emitLabel();
            try compileStat(self, elsebody);
            try patchJump(self, node.location, else_jump, else_label);
        } else {
            const then_label = self.bytecode.emitLabel();
            try self.bytecode.emitAD(.JUMP, 0, 0);

            const else_label = self.bytecode.emitLabel();
            try compileStat(self, elsebody);

            const end_label = self.bytecode.emitLabel();
            try patchJump(self, node.location, else_jump, else_label);
            try patchJump(self, node.location, then_label, end_label);
        }
    } else {
        const end_label = self.bytecode.emitLabel();
        try patchJump(self, node.location, else_jump, end_label);
    }
}
fn compileStatIf(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.@"if";

    // Optimization: condition is always false => we only need the else body
    if (isConstantFalse(self, stat.condition)) {
        if (stat.elsebody) |e| try compileStat(self, e);
        return;
    }

    // Optimization: condition is always false but isn't a constant => we only need the else body and condition's side effects
    if (stat.condition.data == .binary) {
        const cand = stat.condition.data.binary;
        if (cand.op == .@"and" and isConstantFalse(self, cand.right)) {
            try compileExprSide(self, cand.left);
            if (stat.elsebody) |e| try compileStat(self, e);
            return;
        }
    }

    if (self.options.flags.LuauExperimentalIfLocalSyntax and stat.condition_local != null) return compileStatIfLocal(self, node);

    // Optimization: body is a "break" statement with no "else" => we can directly break out of the loop in "then" case
    if (stat.elsebody == null and isStatBreak(stat.thenbody) and !areLocalsCaptured(self, currentLoop(self).local_offset)) {
        // fallthrough = continue with the loop as usual
        var else_jump: std.ArrayList(usize) = .empty;
        try compileConditionValue(self, stat.condition, null, &else_jump, true);

        for (else_jump.items) |jump| try self.loop_jumps.append(self.gpa, .{ .type = .brk, .label = jump });
        return;
    }

    const continue_statement = extractStatContinue(stat.thenbody);

    // Optimization: body is a "continue" statement with no "else" => we can directly continue in "then" case
    if (stat.elsebody == null and continue_statement != null and !areLocalsCaptured(self, currentLoop(self).local_offset_continue)) {
        // track continue statement for repeat..until validation (validateContinueUntil)
        if (currentLoop(self).continue_used == null) currentLoop(self).continue_used = continue_statement;

        // fallthrough = proceed with the loop body as usual
        var else_jump: std.ArrayList(usize) = .empty;
        try compileConditionValue(self, stat.condition, null, &else_jump, true);

        for (else_jump.items) |jump| try self.loop_jumps.append(self.gpa, .{ .type = .cont, .label = jump });
        return;
    }

    var else_jump: std.ArrayList(usize) = .empty;
    try compileConditionValue(self, stat.condition, null, &else_jump, false);

    try compileStat(self, stat.thenbody);

    if (stat.elsebody != null and else_jump.items.len > 0) {
        // we don't need to skip past "else" body if "then" ends with return/break/continue
        // this is important because, if "else" also ends with return, we may *not* have any statement to skip to!
        if (alwaysTerminates(self, stat.thenbody)) {
            const else_label = self.bytecode.emitLabel();

            try compileStat(self, stat.elsebody.?);

            try patchJumps(self, node.location, else_jump.items, else_label);
        } else {
            const then_label = self.bytecode.emitLabel();

            try self.bytecode.emitAD(.JUMP, 0, 0);

            const else_label = self.bytecode.emitLabel();

            try compileStat(self, stat.elsebody.?);

            const end_label = self.bytecode.emitLabel();

            try patchJumps(self, node.location, else_jump.items, else_label);
            try patchJump(self, node.location, then_label, end_label);
        }
    } else {
        const end_label = self.bytecode.emitLabel();

        try patchJumps(self, node.location, else_jump.items, end_label);
    }
}

fn compileStatWhile(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.@"while";

    // Optimization: condition is always false => there's no loop!
    if (isConstantFalse(self, stat.condition)) return;

    const old_jumps = self.loop_jumps.items.len;
    const old_locals = self.local_stack.items.len;

    try self.loops.append(self.gpa, .{ .local_offset = old_locals, .local_offset_continue = old_locals, .continue_used = null });
    self.has_loops = true;

    const loop_label = self.bytecode.emitLabel();

    var else_jump: std.ArrayList(usize) = .empty;
    try compileConditionValue(self, stat.condition, null, &else_jump, false);

    try compileStat(self, stat.body);

    const cont_label = self.bytecode.emitLabel();

    const back_label = self.bytecode.emitLabel();

    setDebugLine(self, stat.condition.location);

    // Note: this is using JUMPBACK, not JUMP, since JUMPBACK is interruptible and we want all loops to have at least one interruptible
    // instruction
    try self.bytecode.emitAD(.JUMPBACK, 0, 0);

    const end_label = self.bytecode.emitLabel();

    try patchJump(self, node.location, back_label, loop_label);
    try patchJumps(self, node.location, else_jump.items, end_label);

    try patchLoopJumps(self, node.location, old_jumps, end_label, cont_label);
    self.loop_jumps.shrinkRetainingCapacity(old_jumps);

    _ = self.loops.pop();
}

const UndefinedLocalVisitor = struct {
    self: *Compiler,
    undef: ?*ast.Local = null,
    locals: std.AutoHashMapUnmanaged(*ast.Local, void) = .empty,

    fn check(v: *UndefinedLocalVisitor, local: *ast.Local) void {
        if (v.undef == null and v.locals.contains(local)) v.undef = local;
    }

    pub fn visitExpr(v: *UndefinedLocalVisitor, e: *Expr) Error!bool {
        switch (e.data) {
            .local => |l| {
                if (!l.upvalue) v.check(l.local);
                return false;
            },
            .function => |f| {
                const fi = v.self.functions.get(e).?;
                for (fi.upvals) |uv| {
                    std.debug.assert(uv.function_depth < f.function_depth);
                    if (uv.function_depth == f.function_depth - 1) v.check(uv);
                }
                return false;
            },
            else => return true,
        }
    }

    pub fn visitStat(_: *UndefinedLocalVisitor, _: *Stat) Error!bool {
        return true;
    }
};

fn validateContinueUntil(self: *Compiler, cont: *Stat, condition: *Expr, body: *Stat, start: usize) Error!void {
    var visitor: UndefinedLocalVisitor = .{ .self = self };

    for (body.data.block.body[start..]) |s| {
        switch (s.data) {
            .local => |l| for (l.vars) |local| try visitor.locals.put(self.gpa, local, {}),
            .local_function => |l| try visitor.locals.put(self.gpa, l.name, {}),
            else => {},
        }
    }

    try an.walkExpr(UndefinedLocalVisitor, &visitor, condition);

    if (visitor.undef) |undef|
        return raise(self, condition.location, "Local {s} used in the repeat..until condition is undefined because continue statement on line {d} jumps over it", .{ undef.name, cont.location.begin.line + 1 });
}

fn compileStatRepeat(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.repeat;

    const old_jumps = self.loop_jumps.items.len;
    const old_locals = self.local_stack.items.len;

    try self.loops.append(self.gpa, .{ .local_offset = old_locals, .local_offset_continue = old_locals, .continue_used = null });
    self.has_loops = true;

    const loop_label = self.bytecode.emitLabel();

    // note: we "inline" compileStatBlock here so that we can close/pop locals after evaluating condition
    // this is necessary because condition can access locals declared inside the repeat..until body
    const body = stat.body;
    const body_stats = body.data.block.body;

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    var continue_validated = false;
    var condition_locals: usize = 0;

    for (body_stats, 0..) |s, i| {
        if (self.options.flags.LuauCompileReuseLocalRegs) try popStatLocals(self, body, s, old_locals);

        try compileStat(self, s);

        // continue statement inside the repeat..until loop should not close upvalues defined directly in the loop body before it
        // this is because the upvalues defined inside the loop body may be captured by a closure defined in the until
        // expression that continue will jump to.
        // but any locals defined in nested blocks or after first continue (which performs validateContinueUntil) do have to be closed
        if (currentLoop(self).continue_used == null) currentLoop(self).local_offset_continue = self.local_stack.items.len;

        // if continue was called from this statement, any local defined after this in the loop body should not be accessed by until condition
        // it is sufficient to check this condition once, as if this holds for the first continue, it must hold for all subsequent continues.
        if (currentLoop(self).continue_used != null and !continue_validated) {
            try validateContinueUntil(self, currentLoop(self).continue_used.?, stat.condition, body, i + 1);
            continue_validated = true;
            condition_locals = self.local_stack.items.len;
        }
    }

    // if continue was used, some locals might not have had their initialization completed
    // the lifetime of these locals has to end before the condition is executed
    // because referencing skipped locals is not possible from the condition, this earlier closure doesn't affect upvalues
    if (continue_validated) {
        // if continueValidated is set, it means we have visited at least one body node and size > 0
        setDebugLineEnd(self, body_stats[body_stats.len - 1].location);

        try closeLocals(self, condition_locals);

        try popLocals(self, condition_locals);
    }

    const cont_label = self.bytecode.emitLabel();

    var end_label: usize = undefined;

    setDebugLine(self, stat.condition.location);

    if (isConstantTrue(self, stat.condition)) {
        try closeLocals(self, old_locals);

        end_label = self.bytecode.emitLabel();
    } else {
        var skip_jump: std.ArrayList(usize) = .empty;
        try compileConditionValue(self, stat.condition, null, &skip_jump, true);

        // we close locals *after* we compute loop conditionals because during computation of condition it's (in theory) possible that user code
        // mutates them
        try closeLocals(self, old_locals);

        const back_label = self.bytecode.emitLabel();

        // Note: this is using JUMPBACK, not JUMP, since JUMPBACK is interruptible and we want all loops to have at least one interruptible
        // instruction
        try self.bytecode.emitAD(.JUMPBACK, 0, 0);

        const skip_label = self.bytecode.emitLabel();

        // we need to close locals *again* after the loop ends because the first closeLocals would be jumped over on the last iteration
        try closeLocals(self, old_locals);

        end_label = self.bytecode.emitLabel();

        try patchJump(self, node.location, back_label, loop_label);
        try patchJumps(self, node.location, skip_jump.items, skip_label);
    }

    try popLocals(self, old_locals);

    try patchLoopJumps(self, node.location, old_jumps, end_label, cont_label);
    self.loop_jumps.shrinkRetainingCapacity(old_jumps);

    _ = self.loops.pop();
}

fn compileStatReturn(self: *Compiler, node: *Stat) Error!void {
    const list = node.data.@"return";

    // LOP_RETURN encoding uses 255 to signal a multret
    if (list.len >= 255) return raise(self, node.location, "Exceeded return count limit; simplify the code to compile", .{});

    if (self.options.flags.LuauExportedTypesParticipateInScc and self.current_function.?.data.function.function_depth == 0 and self.inline_frames.items.len == 0)
        self.exports.has_module_return = true;

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    var temp: u8 = 0;
    var consecutive = false;
    var mult_ret = false;

    // Optimization: return locals directly instead of copying them into a temporary
    // this is very important for a single return value and occasionally effective for multiple values
    const first_reg: ?u8 = if (list.len > 0) getExprLocalReg(self, list[0]) else null;
    if (first_reg) |reg| {
        temp = reg;
        consecutive = true;

        for (list[1..], 1..) |e, i| {
            if (!isExprLocalReg(self, e, @as(usize, temp) + i)) {
                consecutive = false;
                break;
            }
        }
    }

    // Optimization: try to re-adjust the non-consecutive sequence into a consecutive prefix and extra moves
    if (self.options.flags.LuauCompileReuseLocalRegs and !consecutive and list.len > 1) {
        var all_locals = true;

        for (list) |e| {
            if (getExprLocalReg(self, e) == null) all_locals = false;
        }

        if (all_locals) {
            const base = getExprLocalReg(self, list[0]).?;
            var prefix: usize = 1;

            while (prefix < list.len and isExprLocalReg(self, list[prefix], base + prefix)) prefix += 1;

            if (base + prefix == self.reg_top) {
                const tail = try allocReg(self, node.location, @intCast(list.len - prefix));
                std.debug.assert(tail == base + prefix);

                for (prefix..list.len) |i| try self.bytecode.emitABC(.MOVE, @intCast(base + i), getExprLocalReg(self, list[i]).?, 0);

                temp = base;
                consecutive = true;
            }
        }
    }

    if (self.options.flags.LuauCompileMoveElision and !consecutive and list.len == 1 and !isExprMultRet(self, list[0])) {
        // Single expression return might be able to elide the extra move into the fresh register
        temp = try allocReg(self, node.location, 1);
        temp = try compileExprAutoTemp(self, list[0], temp, kInvalidReg);
    } else if (!consecutive and list.len > 0) {
        temp = try allocReg(self, node.location, @intCast(list.len));

        // Note: if the last element is a function call or a vararg specifier, then we need to somehow return all values that that call returned
        for (list, 0..) |e, i| {
            if (i + 1 == list.len)
                mult_ret = try compileExprTempMultRet(self, e, @intCast(temp + i))
            else
                try compileExprTempTop(self, e, @intCast(temp + i));
        }
    }

    try closeLocals(self, 0);

    try self.bytecode.emitABC(.RETURN, temp, if (mult_ret) 0 else @intCast(list.len + 1), 0);
}

fn areLocalsRedundant(self: *Compiler, stat: anytype) bool {
    // Extra expressions may have side effects
    if (stat.values.len > stat.vars.len) return false;

    for (stat.vars) |local| {
        const v = self.variables.get(local) orelse return false;
        if (!v.constant) return false;

        if (self.options.flags.LuauExportValueSyntax and local.is_exported and (!self.options.flags.LuauOptimizeExportTable or self.exports.table_cid == -1)) {
            // exported locals must be written to the export table
            return false;
        }
    }

    return true;
}

fn compileStatLocal(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.local;

    // Optimization: we don't need to allocate and assign const locals, since their uses will be constant-folded
    if (self.options.optimization_level >= 1 and (self.options.debug_level <= 1 or self.options.flags.LuauCompileNoDebugConst) and areLocalsRedundant(self, stat)) return;

    // Optimization: for 1-1 local assignments, we can reuse the register *if* neither local is mutated
    if (self.options.optimization_level >= 1 and stat.vars.len == 1 and stat.values.len == 1) {
        if (getExprLocal(stat.values[0])) |re| {
            const lv = self.variables.get(stat.vars[0]);
            const rv = self.variables.get(re);

            const reg = getLocalReg(self, re);
            if (reg != null and (lv == null or !lv.?.written) and (rv == null or !rv.?.written) and !stat.vars[0].is_exported and !re.is_exported) {
                try pushLocal(self, stat.vars[0], reg.?, kDefaultAllocPc);
                return;
            }
        }

        // Declaration of a local that becomes the inlined function result can use the return target register
        if (self.options.flags.LuauCompileMoveElision and self.inline_frames.items.len > 0 and self.inline_frames.items[self.inline_frames.items.len - 1].result_local == stat.vars[0]) {
            const frame_target = self.inline_frames.items[self.inline_frames.items.len - 1].target;
            try compileExprTemp(self, stat.values[0], frame_target);

            if (!self.options.flags.LuauCompileRemoveStatLocalLoop) {
                // evaluate extra expressions for side effects
                if (stat.values.len > stat.vars.len) {
                    for (stat.values[stat.vars.len..]) |extra| try compileExprSide(self, extra);
                }
            }

            try pushLocal(self, stat.vars[0], frame_target, kDefaultAllocPc);
            return;
        }

        if (self.options.flags.LuauCompileReuseLocalRegs) {
            if (try tryReuseLocalReg(self, stat.vars[0])) |target| {
                const allocpc = self.bytecode.getDebugPC();
                try compileExprTemp(self, stat.values[0], target);
                try pushLocal(self, stat.vars[0], target, allocpc);
                return;
            }
        }
    }

    // note: allocReg in this case allocates into parent block register - note that we don't have RegScope here
    const vars = try allocReg(self, node.location, @intCast(stat.vars.len));
    const allocpc = self.bytecode.getDebugPC();

    try compileExprListTemp(self, stat.values, vars, @intCast(stat.vars.len), true);

    for (stat.vars, 0..) |local, i| {
        const local_variable = self.variables.get(local);
        if (self.options.flags.LuauExportValueSyntax and local.is_exported and
            (!self.options.flags.LuauOptimizeExportTable or !local_variable.?.constant or self.exports.table_cid == -1))
        {
            try ensureExportTable(self, node.location);

            const cid = try addString(self, local.name, local.location);

            const table_reg = try getExportTableReg(self, node.location);
            try self.bytecode.emitABC(.SETTABLEKS, @intCast(vars + i), table_reg, hash8(local.name));
            try self.bytecode.emitAux(@bitCast(cid));
        } else {
            try pushLocal(self, local, @intCast(vars + i), allocpc);
        }
    }
}

fn compileStatFor(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.@"for";
    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    // Optimization: small loops can be unrolled when it is profitable
    if (self.options.optimization_level >= 2 and isConstant(self, stat.to) and isConstant(self, stat.from) and (stat.step == null or isConstant(self, stat.step.?))) {
        if (try tryCompileUnrolledFor(self, node, kLoopUnrollThreshold, kLoopUnrollThresholdMaxBoost)) return;
    }

    const old_locals = self.local_stack.items.len;
    const old_jumps = self.loop_jumps.items.len;

    try self.loops.append(self.gpa, .{ .local_offset = old_locals, .local_offset_continue = old_locals, .continue_used = null });
    self.has_loops = true;

    // register layout: limit, step, index
    const regs = try allocReg(self, node.location, 3);

    // if the iteration index is assigned from within the loop, we need to protect the internal index from the assignment
    // to do that, we will copy the index into an actual local variable on each iteration
    // this makes sure the code inside the loop can't interfere with the iteration process (other than modifying the table we're iterating
    // through)
    var varreg: u8 = regs + 2;
    const varregallocpc = self.bytecode.getDebugPC();

    if (self.variables.get(stat.variable)) |il| {
        if (il.written) varreg = try allocReg(self, node.location, 1);
    }

    try compileExprTemp(self, stat.from, regs + 2);
    try compileExprTemp(self, stat.to, regs + 0);

    if (stat.step) |step|
        try compileExprTemp(self, step, regs + 1)
    else
        try self.bytecode.emitABC(.LOADN, regs + 1, 1, 0);

    const for_label = self.bytecode.emitLabel();

    try self.bytecode.emitAD(.FORNPREP, regs, 0);

    const loop_label = self.bytecode.emitLabel();

    if (varreg != regs + 2) try self.bytecode.emitABC(.MOVE, varreg, regs + 2, 0);

    try pushLocal(self, stat.variable, varreg, varregallocpc);

    try compileStat(self, stat.body);

    try closeLocals(self, old_locals);
    try popLocals(self, old_locals);

    setDebugLine(self, node.location);

    const cont_label = self.bytecode.emitLabel();

    const back_label = self.bytecode.emitLabel();

    try self.bytecode.emitAD(.FORNLOOP, regs, 0);

    const end_label = self.bytecode.emitLabel();

    try patchJump(self, node.location, for_label, end_label);
    try patchJump(self, node.location, back_label, loop_label);

    try patchLoopJumps(self, node.location, old_jumps, end_label, cont_label);
    self.loop_jumps.shrinkRetainingCapacity(old_jumps);

    _ = self.loops.pop();
}

fn compileStatForIn(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.for_in;
    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    const old_locals = self.local_stack.items.len;
    const old_jumps = self.loop_jumps.items.len;

    try self.loops.append(self.gpa, .{ .local_offset = old_locals, .local_offset_continue = old_locals, .continue_used = null });
    self.has_loops = true;

    // register layout: generator, state, index, variables...
    const regs = try allocReg(self, node.location, 3);

    // this puts initial values of (generator, state, index) into the loop registers
    try compileExprListTemp(self, stat.values, regs, 3, true);

    // note that we reserve at least 2 variables; this allows our fast path to assume that we need 2 variables instead of 1 or 2
    const vars = try allocReg(self, node.location, @max(@as(u32, @intCast(stat.vars.len)), 2));
    std.debug.assert(vars == regs + 3);
    const varsallocpc = self.bytecode.getDebugPC();

    var skip_op: Op = .FORGPREP;

    // Optimization: when we iterate via pairs/ipairs, we generate special bytecode that optimizes the traversal using internal iteration index
    // These instructions dynamically check if generator is equal to next/inext and bail out
    // They assume that the generator produces 2 variables, which is why we allocate at least 2 above (see vars assignment)
    if (self.options.optimization_level >= 1 and stat.vars.len <= 2) {
        if (stat.values.len == 1 and stat.values[0].data == .call) {
            const builtin = an.getBuiltin(stat.values[0].data.call.func, &self.globals, &self.variables);

            if (isGlobalBuiltin(builtin, "ipairs")) {
                // for .. in ipairs(t)
                skip_op = .FORGPREP_INEXT;
            } else if (isGlobalBuiltin(builtin, "pairs")) {
                // for .. in pairs(t)
                skip_op = .FORGPREP_NEXT;
            }
        } else if (stat.values.len == 2 and (!self.getfenv_used and !self.setfenv_used)) {
            const builtin = an.getBuiltin(stat.values[0], &self.globals, &self.variables);

            // for .. in next,t
            if (isGlobalBuiltin(builtin, "next")) skip_op = .FORGPREP_NEXT;
        }
    }

    // first iteration jumps into FORGLOOP instruction, but for ipairs/pairs it does extra preparation that makes the cost of an extra instruction
    // worthwhile
    const skip_label = self.bytecode.emitLabel();

    try self.bytecode.emitAD(skip_op, regs, 0);

    const loop_label = self.bytecode.emitLabel();

    for (stat.vars, 0..) |v, i| try pushLocal(self, v, @intCast(vars + i), varsallocpc);

    try compileStat(self, stat.body);

    try closeLocals(self, old_locals);
    try popLocals(self, old_locals);

    setDebugLine(self, node.location);

    const cont_label = self.bytecode.emitLabel();

    const back_label = self.bytecode.emitLabel();

    // FORGLOOP uses aux to encode variable count and fast path flag for ipairs traversal in the high bit
    try self.bytecode.emitAD(.FORGLOOP, regs, 0);
    try self.bytecode.emitAux((if (skip_op == .FORGPREP_INEXT) @as(u32, 0x80000000) else 0) | @as(u32, @intCast(stat.vars.len)));

    const end_label = self.bytecode.emitLabel();

    try patchJump(self, node.location, skip_label, back_label);
    try patchJump(self, node.location, back_label, loop_label);

    try patchLoopJumps(self, node.location, old_jumps, end_label, cont_label);
    self.loop_jumps.shrinkRetainingCapacity(old_jumps);

    _ = self.loops.pop();
}

fn isGlobalBuiltin(b: an.Builtin, name: []const u8) bool {
    return b.object == null and b.method != null and std.mem.eql(u8, b.method.?, name);
}

const Assignment = struct {
    lvalue: LValue,
    conflict_reg: u8 = kInvalidReg,
    value_reg: u8 = kInvalidReg,
};

const AssignConflictVisitor = struct {
    self: *Compiler,
    conflict: std.StaticBitSet(256) = .empty,
    assigned: std.StaticBitSet(256) = .empty,

    pub fn visitExpr(v: *AssignConflictVisitor, e: *Expr) Error!bool {
        if (e.data == .local) {
            if (getLocalReg(v.self, e.data.local.local)) |reg| {
                if (v.assigned.isSet(reg)) v.conflict.set(reg);
            }
        }
        return true;
    }

    pub fn visitStat(_: *AssignConflictVisitor, _: *Stat) Error!bool {
        return true;
    }
};

// This function analyzes assignments and marks assignment conflicts: cases when a variable is assigned on lhs
// but subsequently used on the rhs, assuming assignments are performed in order. Note that it's also possible
// for a variable to conflict on the lhs, if it's used in an lvalue expression after it's assigned.
// When conflicts are found, Assignment::conflictReg is allocated and that's where assignment is performed instead,
// until the final fixup in compileStatAssign. Assignment::valueReg is allocated by compileStatAssign as well.
fn resolveAssignConflicts(self: *Compiler, location: Location, vars: []Assignment, values: []const *Expr) Error!void {
    var visitor: AssignConflictVisitor = .{ .self = self };

    // mark any registers that are used *after* assignment as conflicting

    // first we go through assignments to locals, since they are performed before assignments to other l-values
    for (vars, 0..) |v, i| {
        const li = v.lvalue;
        if (li.kind == .local) {
            if (i < values.len) try an.walkExpr(AssignConflictVisitor, &visitor, values[i]);
            visitor.assigned.set(li.reg);
        }
    }

    // and now we handle all other l-values
    for (vars, 0..) |v, i| {
        if (v.lvalue.kind != .local and i < values.len) try an.walkExpr(AssignConflictVisitor, &visitor, values[i]);
    }

    // mark any registers used in trailing expressions as conflicting as well
    if (values.len > vars.len) {
        for (values[vars.len..]) |e| try an.walkExpr(AssignConflictVisitor, &visitor, e);
    }

    // mark any registers used on left hand side that are also assigned anywhere as conflicting
    // this is order-independent because we evaluate all right hand side arguments into registers before doing table assignments
    for (vars) |v| {
        const li = v.lvalue;
        if ((li.kind == .index_name or li.kind == .index_number or li.kind == .index_expr or (self.options.flags.DebugLuauUserDefinedClasses and li.kind == .private_field)) and visitor.assigned.isSet(li.reg)) visitor.conflict.set(li.reg);
        if (li.kind == .index_expr and visitor.assigned.isSet(li.index)) visitor.conflict.set(li.index);
    }

    // for any conflicting var, we need to allocate a temporary register where the assignment is performed, so that we can move the value later
    for (vars) |*v| {
        if (v.lvalue.kind == .local and visitor.conflict.isSet(v.lvalue.reg)) v.conflict_reg = try allocReg(self, location, 1);
    }
}

fn compileStatAssign(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.assign;
    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    // Optimization: one to one assignments don't require complex conflict resolution machinery
    if (stat.vars.len == 1 and stat.values.len == 1) {
        const v = try compileLValue(self, stat.vars[0]);

        // Optimization: assign to locals directly
        if (v.kind == .local) {
            try compileExpr(self, stat.values[0], v.reg, false);
        } else {
            const reg = try compileExprAuto(self, stat.values[0]);

            setDebugLine(self, stat.vars[0].location);
            try compileAssign(self, v, reg, stat.vars[0]);
        }
        return;
    }

    // compute all l-values: note that this doesn't assign anything yet but it allocates registers and computes complex expressions on the
    // left hand side - for example, in "a[expr] = foo" expr will get evaluated here
    const vars = try self.gpa.alloc(Assignment, stat.vars.len);

    for (stat.vars, 0..) |e, i| vars[i] = .{ .lvalue = try compileLValue(self, e) };

    // perform conflict resolution: if any expression refers to a local that is assigned before evaluating it, we assign to a temporary
    // register after this, vars[i].conflictReg is set for locals that need to be assigned in the second pass
    try resolveAssignConflicts(self, node.location, vars, stat.values);

    // compute rhs into (mostly) fresh registers
    // note that when the lhs assignment is a local, we evaluate directly into that register
    // this is possible because resolveAssignConflicts renamed conflicting locals into temporaries
    // after this, vars[i].valueReg is set to a register with the value for *all* vars, but some have already been assigned
    var i: usize = 0;
    while (i < stat.vars.len and i < stat.values.len) : (i += 1) {
        const value = stat.values[i];

        if (i + 1 == stat.values.len and stat.vars.len > stat.values.len) {
            // allocate a consecutive range of regs for all remaining vars and compute everything into temps
            // note, this also handles trailing nils
            const rest: u32 = @intCast(stat.vars.len - stat.values.len + 1);
            const temp = try allocReg(self, node.location, rest);

            try compileExprTempN(self, value, temp, @intCast(rest), true);

            for (vars[i..], 0..) |*v, j| v.value_reg = @intCast(temp + j);
        } else {
            const v = &vars[i];

            // if target is a local, use compileExpr directly to target
            if (v.lvalue.kind == .local) {
                v.value_reg = if (v.conflict_reg == kInvalidReg) v.lvalue.reg else v.conflict_reg;

                try compileExpr(self, stat.values[i], v.value_reg, false);
            } else {
                v.value_reg = try compileExprAuto(self, stat.values[i]);
            }
        }
    }

    // compute expressions with side effects
    if (stat.values.len > stat.vars.len) {
        for (stat.values[stat.vars.len..]) |e| try compileExprSide(self, e);
    }

    // almost done... let's assign everything left to right, noting that locals were either written-to directly, or will be written-to in a
    // separate pass to avoid conflicts
    for (vars, 0..) |v, var_pos| {
        std.debug.assert(v.value_reg != kInvalidReg);

        if (v.lvalue.kind != .local) {
            setDebugLine(self, v.lvalue.location);
            try compileAssign(self, v.lvalue, v.value_reg, stat.vars[var_pos]);
        }
    }

    // all regular local writes are done by the prior loops by computing result directly into target, so this just handles conflicts OR
    // local copies from temporary registers in multret context, since in that case we have to allocate consecutive temporaries
    for (vars) |v| {
        if (v.lvalue.kind == .local and v.value_reg != v.lvalue.reg) try self.bytecode.emitABC(.MOVE, v.lvalue.reg, v.value_reg, 0);
    }
}

fn compileStatCompoundAssign(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.compound_assign;
    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    const v = try compileLValue(self, stat.variable);

    // Optimization: assign to locals directly
    const target: u8 = if (v.kind == .local) v.reg else try allocReg(self, node.location, 1);

    switch (stat.op) {
        .add, .sub, .mul, .div, .floor_div, .mod, .pow => {
            if (v.kind != .local) try compileLValueUse(self, v, target, false, stat.variable);

            const rc = try getConstantNumber(self, stat.value);

            if (rc >= 0 and rc <= 255) {
                try self.bytecode.emitABC(getBinaryOpArith(stat.op, true), target, target, @intCast(rc));
            } else {
                const rr = try compileExprAuto(self, stat.value);

                try self.bytecode.emitABC(getBinaryOpArith(stat.op, false), target, target, rr);

                if (v.kind != .local) try hintTemporaryRegType(self, stat.variable, target, bc.LBC_TYPE_NUMBER, 1);

                try hintTemporaryExprRegType(self, stat.value, rr, bc.LBC_TYPE_NUMBER, 1);
            }
        },
        .concat => {
            var args: std.ArrayList(*Expr) = .empty;
            try args.append(self.gpa, stat.value);

            // unroll the tree of concats down the right hand side to be able to do multiple ops
            while (true) {
                const last = args.items[args.items.len - 1];
                if (last.data != .binary or last.data.binary.op != .concat) break;
                args.items[args.items.len - 1] = last.data.binary.left;
                try args.append(self.gpa, last.data.binary.right);
            }

            const regs = try allocReg(self, node.location, @intCast(1 + args.items.len));

            try compileLValueUse(self, v, regs, false, stat.variable);

            for (args.items, 0..) |arg, i| {
                if (self.options.flags.LuauCompileConcatTargetTop)
                    try compileExprTempTop(self, arg, @intCast(regs + 1 + i))
                else
                    try compileExprTemp(self, arg, @intCast(regs + 1 + i));
            }

            try self.bytecode.emitABC(.CONCAT, target, regs, @intCast(regs + args.items.len));
        },
        else => unreachable,
    }

    if (v.kind != .local) try compileAssign(self, v, target, stat.variable);
}

fn compileStatFunction(self: *Compiler, node: *Stat) Error!void {
    const stat = node.data.function;

    // Optimization: compile value expression directly into target local register
    if (getExprLocalReg(self, stat.name)) |local_reg| {
        try compileExpr(self, stat.func, local_reg, false);
        return;
    }

    const old_top = self.reg_top;
    defer self.reg_top = old_top;
    const reg = try allocReg(self, node.location, 1);

    try compileExprTemp(self, stat.func, reg);

    const v = try compileLValue(self, stat.name);
    try compileAssign(self, v, reg, stat.name);
}

fn compileStat(self: *Compiler, node: *Stat) Error!void {
    setDebugLine(self, node.location);

    if (self.options.coverage_level >= 1 and needsCoverageStat(node)) try self.bytecode.emitABC(.COVERAGE, 0, 0, 0);

    switch (node.data) {
        .block => |b| {
            const old_top = self.reg_top;
            defer self.reg_top = old_top;

            const old_locals = self.local_stack.items.len;
            if (self.options.flags.LuauExportValueSyntax) self.block_depth += 1;

            var terminates_early = false;

            for (b.body) |body_stat| {
                if (self.options.flags.LuauCompileReuseLocalRegs) try popStatLocals(self, node, body_stat, 0);

                try compileStat(self, body_stat);

                if (alwaysTerminates(self, body_stat)) {
                    terminates_early = true;
                    break;
                }
            }

            if (self.options.flags.LuauExportValueSyntax) self.block_depth -= 1;

            if (!terminates_early) try closeLocals(self, old_locals);

            try popLocals(self, old_locals);
        },
        .@"if" => try compileStatIf(self, node),
        .@"while" => try compileStatWhile(self, node),
        .repeat => try compileStatRepeat(self, node),
        .@"break" => {
            std.debug.assert(self.loops.items.len > 0);

            // before exiting out of the loop, we need to close all local variables that were captured in closures since loop start
            // normally they are closed by the enclosing blocks, including the loop block, but we're skipping that here
            try closeLocals(self, currentLoop(self).local_offset);

            const label = self.bytecode.emitLabel();

            try self.bytecode.emitAD(.JUMP, 0, 0);

            try self.loop_jumps.append(self.gpa, .{ .type = .brk, .label = label });
        },
        .@"continue" => {
            std.debug.assert(self.loops.items.len > 0);

            // track continue statement for repeat..until validation (validateContinueUntil)
            if (currentLoop(self).continue_used == null) currentLoop(self).continue_used = node;

            // before continuing, we need to close all local variables that were captured in closures since loop start
            // normally they are closed by the enclosing blocks, including the loop block, but we're skipping that here
            try closeLocals(self, currentLoop(self).local_offset_continue);

            const label = self.bytecode.emitLabel();

            try self.bytecode.emitAD(.JUMP, 0, 0);

            try self.loop_jumps.append(self.gpa, .{ .type = .cont, .label = label });
        },
        .@"return" => {
            if (self.options.optimization_level >= 2 and self.inline_frames.items.len > 0)
                try compileInlineReturn(self, node)
            else
                try compileStatReturn(self, node);
        },
        .expr => |e| {
            // Optimization: since we don't need to read anything from the stack, we can compile the call to not return anything which saves register
            // moves
            if (e.data == .call) {
                const target: u8 = @intCast(self.reg_top);

                try compileExprCall(self, e, target, 0, false, false);
            } else {
                try compileExprSide(self, e);
            }
        },
        .local => |l| {
            if (self.options.flags.LuauExportValueSyntax) {
                for (l.vars) |local| try checkExportedLocal(self, local, node.location);
            }
            try compileStatLocal(self, node);
        },
        .@"for" => try compileStatFor(self, node),
        .for_in => try compileStatForIn(self, node),
        .assign => try compileStatAssign(self, node),
        .compound_assign => try compileStatCompoundAssign(self, node),
        .function => try compileStatFunction(self, node),
        .local_function => |lf| {
            if (self.options.flags.LuauExportValueSyntax and !self.options.flags.LuauOptimizeExportTable and lf.name.is_exported) {
                try checkExportedLocal(self, lf.name, node.location);

                try ensureExportTable(self, node.location);

                const old_top = self.reg_top;
                defer self.reg_top = old_top;

                const v = try allocReg(self, node.location, 1);
                try compileExprFunction(self, lf.func, v);

                const cid = try addString(self, lf.name.name, lf.name.location);

                const table_reg = try getExportTableReg(self, node.location);
                try self.bytecode.emitABC(.SETTABLEKS, v, table_reg, hash8(lf.name.name));
                try self.bytecode.emitAux(@bitCast(cid));
            } else {
                const reused: ?u8 = if (self.options.flags.LuauCompileReuseLocalRegs) try tryReuseLocalReg(self, lf.name) else null;
                const v = reused orelse try allocReg(self, node.location, 1);

                try pushLocal(self, lf.name, v, kDefaultAllocPc);
                try compileExprFunction(self, lf.func, v);

                // we *have* to pushLocal before we compile the function, since the function may refer to the local as an upvalue
                // however, this means the debugpc for the local is at an instruction where the local value hasn't been computed yet
                // to fix this we just move the debugpc after the local value is established
                self.locals.getPtr(lf.name).?.debugpc = self.bytecode.getDebugPC();
            }
        },
        .type_alias => |alias| {
            if (self.options.flags.LuauExportedTypesParticipateInScc and alias.exported and atTopLevel(self)) self.exports.has_type_exports = true;
        },
        .type_function => {},
        // classes only parse with DebugLuauUserDefinedClasses
        .class => try compileClassDeclaration(self, node),
    }
}

// ------------------------------------------------------------------ functions

const ConstUpvalueVisitor = struct {
    self: *Compiler,
    upvals: std.ArrayList(*ast.Local) = .empty,

    pub fn visitExpr(v: *ConstUpvalueVisitor, e: *Expr) Error!bool {
        switch (e.data) {
            .local => |l| {
                if (l.upvalue and isConstant(v.self, e)) try v.upvals.append(v.self.gpa, l.local);
                return false;
            },
            // short-circuits the traversal to make it faster
            .function => return false,
            else => return true,
        }
    }

    pub fn visitStat(_: *ConstUpvalueVisitor, _: *Stat) Error!bool {
        return true;
    }
};

fn gatherConstUpvals(self: *Compiler, func: *ast.Function) Error!void {
    var visitor: ConstUpvalueVisitor = .{ .self = self };
    try an.walkStat(ConstUpvalueVisitor, &visitor, func.body);

    for (visitor.upvals.items) |local| _ = try getUpval(self, local);
}

fn compileFunction(self: *Compiler, node: *Expr, protoflags: *u8) Error!u32 {
    const func = &node.data.function;
    std.debug.assert(!self.functions.contains(node));
    std.debug.assert(self.reg_top == 0 and self.stack_size == 0 and self.local_stack.items.len == 0 and self.upvals.items.len == 0);

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    if (self.options.flags.LuauExportValueSyntax) self.current_function = node;

    if (self.options.flags.LuauExportValueSyntax and self.options.flags.LuauOptimizeExportTable and atTopLevel(self)) try buildExportTableShape(self);

    const self_param: u32 = @intFromBool(func.self != null);
    const num_params = self_param + @as(u32, @intCast(func.args.len));
    // parameter count is bounded by the register limit enforced by allocReg below
    const fid = try self.bytecode.beginFunction(@truncate(num_params), func.vararg);

    setDebugLine(self, node.location);

    if (func.vararg) try self.bytecode.emitABC(.PREPVARARGS, @truncate(num_params), 0, 0);

    const args = try allocReg(self, node.location, num_params);

    if (func.self) |s| try pushLocal(self, s, args, kDefaultAllocPc);

    for (func.args, 0..) |arg, i| try pushLocal(self, arg, @intCast(args + self_param + i), kDefaultAllocPc);

    self.arg_count = self.local_stack.items.len;

    const stat = func.body;

    var terminates_early = false;
    self.current_function = node;

    for (stat.data.block.body) |body_stat| {
        if (self.options.flags.LuauCompileReuseLocalRegs) try popStatLocals(self, stat, body_stat, 0);

        try compileStat(self, body_stat);

        if (alwaysTerminates(self, body_stat)) {
            terminates_early = true;
            break;
        }
    }

    // valid function bytecode must always end with RETURN
    // we elide this if we're guaranteed to hit a RETURN statement regardless of the control flow
    if (self.options.flags.LuauExportValueSyntax) {
        setDebugLineEnd(self, stat.location);
        // in main
        if (!self.exports.isEmpty() and atTopLevel(self)) {
            try compileExportTable(self);
        } else if (self.options.flags.LuauExportedTypesParticipateInScc and self.exports.has_type_exports and !self.exports.has_module_return and atTopLevel(self)) {
            try compileExportTable(self);
        } else if (!terminates_early) {
            try closeLocals(self, 0);

            try self.bytecode.emitABC(.RETURN, 0, 1, 0);
        }
    } else if (!terminates_early) {
        setDebugLineEnd(self, stat.location);
        try closeLocals(self, 0);

        try self.bytecode.emitABC(.RETURN, 0, 1, 0);
    }

    // constant folding may remove some upvalue refs from bytecode, so this puts them back
    if (!self.options.flags.LuauCompileNoDebugConst and self.options.optimization_level >= 1 and self.options.debug_level >= 2) try gatherConstUpvals(self, func);

    self.bytecode.setDebugFunctionLineDefined(@intCast(node.location.begin.line + 1));

    if (self.options.debug_level >= 1) {
        if (func.debugname) |name| try self.bytecode.setDebugFunctionName(name);
    }

    if (self.options.debug_level >= 2 and self.upvals.items.len != 0) {
        for (self.upvals.items) |l| try self.bytecode.pushDebugUpval(l.name);
    }

    if (self.options.type_info_level >= 1) {
        for (self.upvals.items) |l| try self.bytecode.pushUpvalTypeInfo(self.local_types.get(l) orelse bc.LBC_TYPE_ANY);
    }

    if (self.options.optimization_level >= 1) self.bytecode.foldJumps();

    if (!try self.bytecode.expandJumps())
        return raise(self, node.location, "Exceeded jump distance limit; simplify the code to compile", .{});

    try popLocals(self, 0);

    if (self.bytecode.getInstructionCount() > kMaxInstructionCount)
        return raise(self, node.location, "Exceeded function instruction limit; split the function into parts to compile", .{});

    if (self.function_types.get(node)) |func_type| self.bytecode.setFunctionTypeInfo(func_type);

    // top-level code only executes once so it can be marked as cold if it has no loops; code with loops might be profitable to compile natively
    if (func.function_depth == 0 and !self.has_loops) protoflags.* |= bc.LPF_NATIVE_COLD;

    if (func.hasNativeAttribute()) protoflags.* |= bc.LPF_NATIVE_FUNCTION;

    if (self.options.flags.LuauExportValueSyntax and !self.exports.isEmpty() and func.function_depth == 0) protoflags.* |= bc.LPF_USES_EXPORT;

    if (self.options.flags.LuauExportedTypesParticipateInScc and self.exports.has_type_exports and !self.exports.has_module_return and func.function_depth == 0) protoflags.* |= bc.LPF_USES_EXPORT;

    const is_inlinable = !self.has_multi_ret and !self.getfenv_used and !self.setfenv_used;
    var cost: u64 = 0;
    if (self.options.flags.LuauEmitCallFeedback and is_inlinable and self.upvals.items.len == 0) {
        protoflags.* |= bc.LPF_INLINABLE;
        cost = try cost_model.modelCost(self.gpa, func.body, func.args, &self.builtins, &self.constants);
    }

    try self.bytecode.endFunction(@intCast(self.stack_size), @intCast(self.upvals.items.len), protoflags.*, cost);

    var info: FunctionInfo = .{ .id = fid, .upvals = try self.gpa.dupe(*ast.Local, self.upvals.items) };

    // record information for inlining
    if (self.options.optimization_level >= 2 and !func.vararg and func.self == null and !self.getfenv_used and !self.setfenv_used) {
        info.can_inline = !(self.options.flags.DebugLuauNoInline and func.hasAttribute(.debug_noinline));
        info.stack_size = self.stack_size;
        info.cost_model = if (cost == 0) try cost_model.modelCost(self.gpa, func.body, func.args, &self.builtins, &self.constants) else cost;

        // track functions that only ever return a single value so that we can convert multret calls to fixedret calls
        if (alwaysTerminates(self, func.body)) {
            var return_visitor: ReturnVisitor = .{ .self = self, .func = func };
            try an.walkStat(ReturnVisitor, &return_visitor, func.body);
            info.returns_one = return_visitor.returns_one;

            if (self.options.flags.LuauCompileMoveElision) {
                info.return_kind = return_visitor.return_kind;
                info.return_local = return_visitor.return_local;

                if (info.return_local) |rl| {
                    if (self.locals.get(rl)) |lc| {
                        if (lc.captured) info.return_local_captured = true;
                    }
                }
            }
        }
    }

    try self.functions.put(self.gpa, node, info);

    self.upvals.clearRetainingCapacity(); // note: instead of std::move above, we copy & clear to preserve capacity for future pushes
    self.stack_size = 0;

    self.arg_count = 0;

    self.has_loops = false;
    self.has_multi_ret = false;

    if (self.options.flags.LuauCompileReuseLocalRegs) self.free_local_regs.clearRetainingCapacity();
    self.current_function = null;

    return fid;
}

// ------------------------------------------------------------------ driver

const FenvVisitor = struct {
    getfenv_used: *bool,
    setfenv_used: *bool,

    pub fn visitExpr(v: *FenvVisitor, e: *Expr) Error!bool {
        if (e.data == .global) {
            const name = e.data.global;
            if (std.mem.eql(u8, name, "getfenv")) v.getfenv_used.* = true;
            if (std.mem.eql(u8, name, "setfenv")) v.setfenv_used.* = true;
            return false;
        }
        return true;
    }

    pub fn visitStat(_: *FenvVisitor, _: *Stat) Error!bool {
        return true;
    }
};

const FunctionVisitor = struct {
    gpa: Allocator,
    functions: std.ArrayList(*Expr) = .empty,
    has_native_function: bool = false,

    pub fn visitExpr(v: *FunctionVisitor, e: *Expr) Error!bool {
        if (e.data != .function) return true;
        try an.walkStat(FunctionVisitor, v, e.data.function.body);

        // this makes sure all functions that are used when compiling this one have been already added to the vector
        try v.functions.append(v.gpa, e);

        if (!v.has_native_function and e.data.function.hasNativeAttribute()) v.has_native_function = true;

        return false;
    }

    pub fn visitStat(_: *FunctionVisitor, s: *Stat) Error!bool {
        return s.data != .type_function;
    }
};

/// Upstream compileOrThrow(BytecodeBuilder&, const ParseResult&, AstNameTable&, const CompileOptions&).
pub fn compileOrThrow(gpa: Allocator, bytecode: *BytecodeBuilder, root: *Stat, hotcomments: []const ast.HotComment, names: *lex.NameTable, input_options: Options, diag: *Diagnostic) Error!void {
    var options = input_options;
    var main_flags: u8 = 0;

    for (hotcomments) |hc| {
        if (hc.header and std.mem.startsWith(u8, hc.content, "optimize ")) options.optimization_level = @intCast(std.math.clamp(atoi(hc.content[9..]), 0, 2));

        if (hc.header and std.mem.eql(u8, hc.content, "native")) {
            main_flags |= bc.LPF_NATIVE_MODULE;
            options.optimization_level = 2; // note: this might be removed in the future in favor of --!optimize
            options.type_info_level = 1;
        }
    }

    // gathers all functions with the invariant that all function references are to functions earlier in the list
    // for example, function foo() return function() end end will result in two vector entries, [0] = anonymous and [1] = foo
    var function_visitor: FunctionVisitor = .{ .gpa = gpa };
    try an.walkStat(FunctionVisitor, &function_visitor, root);

    if (function_visitor.has_native_function) {
        options.optimization_level = 2;
        options.type_info_level = 1;
    }

    var compiler: Compiler = .{
        .gpa = gpa,
        .bytecode = bytecode,
        .options = options,
        .names = names,
        .diag = diag,
        .exports = .{ .table_local = .{
            .name = try names.getOrAdd("__EXP"),
            .location = .{},
            .shadow = null,
            .function_depth = 0,
            .loop_depth = 0,
            .is_const = true,
            .annotation = null,
        } },
    };

    // feature flags that affect bytecode serialization
    bytecode.flags = options.flags;

    // since access to some global objects may result in values that change over time, we block imports from non-readonly tables
    try an.assignMutable(gpa, &compiler.globals, names, options.mutable_globals);

    // this pass analyzes mutability of locals/globals and associates locals with their initial values
    try an.trackValues(gpa, &compiler.globals, &compiler.variables, if (options.flags.LuauOptimizeExportTable) &compiler.exports.exported else null, options.flags.LuauCompileReuseLocalRegs, root);

    // this visitor tracks calls to getfenv/setfenv and disables some optimizations when they are found
    if (options.optimization_level >= 1 and (names.get("getfenv") != null or names.get("setfenv") != null)) {
        var fenv_visitor: FenvVisitor = .{ .getfenv_used = &compiler.getfenv_used, .setfenv_used = &compiler.setfenv_used };
        try an.walkStat(FenvVisitor, &fenv_visitor, root);
    }

    // builtin folding is enabled on optimization level 2 since we can't de-optimize folding at runtime
    if (options.optimization_level >= 2 and (!compiler.getfenv_used and !compiler.setfenv_used)) {
        compiler.builtins_fold = &compiler.builtins;

        if (names.get("math")) |math| {
            if (an.getGlobalState(&compiler.globals, math) == .default) compiler.builtins_fold_library_k = true;
        }
        if (!compiler.builtins_fold_library_k) {
            for (options.libraries_with_known_members) |lib| {
                if (names.get(lib)) |name| {
                    if (an.getGlobalState(&compiler.globals, name) == .default) {
                        compiler.builtins_fold_library_k = true;
                        break;
                    }
                }
            }
        }
    }

    if (options.optimization_level >= 1) {
        // this pass tracks which calls are builtins and can be compiled more efficiently
        try an.analyzeBuiltins(gpa, &compiler.builtins, &compiler.globals, &compiler.variables, builtinOptions(&compiler), options.disabled_builtins, root, names);

        // this pass determines which locals hold constant tables that are never mutated
        try an.buildTableConstantMap(gpa, &compiler.table_constants, &compiler.variables, root, options.flags);

        // this pass analyzes constantness of expressions
        try an.foldConstants(gpa, &compiler.constants, &compiler.variables, &compiler.locstants, foldOptions(&compiler), root, names, &compiler.table_constants, null, null);

        // this pass analyzes table assignments to estimate table shapes for initially empty tables
        try an.predictTableShapes(gpa, &compiler.table_shapes, root);
    }

    for (options.userdata_types) |ud| {
        // Type will only resolve to an AstName if it is actually mentioned in the source
        if (names.get(ud)) |name| try compiler.user_data_types.put(gpa, name, @intCast(try bytecode.addUserdataType(name)));
    }

    if (options.userdata_types.len > bc.LBC_TYPE_TAGGED_USERDATA_END - bc.LBC_TYPE_TAGGED_USERDATA_BASE)
        return raise(&compiler, root.location, "Exceeded userdata type limit in the compilation options", .{});

    // computes type information for all functions based on type annotations
    if (options.type_info_level >= 1 or options.optimization_level >= 2) {
        try types.buildTypeMap(gpa, &compiler.function_types, &compiler.local_types, &compiler.expr_types, root, .{
            .host_vector_type = options.vector_type,
            .userdata_types = &compiler.user_data_types,
            .builtin_types = &compiler.builtin_types,
            .builtin_calls = &compiler.builtins,
            .globals = &compiler.globals,
            .library_member_type_cb = options.library_member_type_cb,
            .flags = options.flags,
        }, bytecode);
    }

    if (options.flags.DebugLuauUserDefinedClasses) {
        var class_visitor: ClassVisitor = .{ .self = &compiler };
        try an.walkStat(ClassVisitor, &class_visitor, root);
    }

    for (function_visitor.functions.items) |expr| {
        var protoflags: u8 = 0;

        if (options.flags.DebugLuauUserDefinedClasses) {
            if (compiler.enclosing_classes.get(expr)) |cls| compiler.curr_class = cls;
        }

        _ = try compileFunction(&compiler, expr, &protoflags);

        if (options.flags.DebugLuauUserDefinedClasses) compiler.curr_class = null;

        // If a function has native attribute and the whole module is not native, we set  LPF_NATIVE_FUNCTION flag
        // This ensures that LPF_NATIVE_MODULE and LPF_NATIVE_FUNCTION are exclusive.
        if ((protoflags & bc.LPF_NATIVE_FUNCTION) != 0 and (main_flags & bc.LPF_NATIVE_MODULE) == 0) main_flags |= bc.LPF_NATIVE_FUNCTION;
    }

    const main = try gpa.create(Expr);
    main.* = .{ .location = root.location, .data = .{ .function = .{
        .attributes = &.{},
        .generics = &.{},
        .generic_packs = &.{},
        .self = null,
        .args = &.{},
        .return_annotation = null,
        .vararg = true,
        .vararg_location = .{},
        .vararg_annotation = null,
        .body = root,
        .function_depth = 0,
        .debugname = null,
    } } };
    const mainid = try compileFunction(&compiler, main, &main_flags);

    std.debug.assert(compiler.functions.get(main).?.upvals.len == 0);

    bytecode.setMainFunction(mainid);
    try bytecode.finalize();

}

fn builtinOptions(self: *Compiler) an.BuiltinOptions {
    return .{ .vector_lib = self.options.vector_lib, .vector_ctor = self.options.vector_ctor, .flags = self.options.flags };
}

fn foldOptions(self: *Compiler) an.FoldOptions {
    return .{
        .builtins = self.builtins_fold,
        .fold_library_k = self.builtins_fold_library_k,
        .vector_double_precision = self.options.vector_precision == 1,
        .library_member_constant_cb = self.options.library_member_constant_cb,
        .flags = self.options.flags,
    };
}

// ------------------------------------------------------------------ inlining (optimization level 2)

const ReturnVisitor = struct {
    self: *Compiler,
    func: *const ast.Function,
    returns_one: bool = true,
    return_kind: ReturnKind = .none,
    return_local: ?*ast.Local = null, // For Parameter/Local ReturnKind

    pub fn visitExpr(_: *ReturnVisitor, _: *Expr) Error!bool {
        return false;
    }

    pub fn visitStat(v: *ReturnVisitor, s: *Stat) Error!bool {
        if (s.data != .@"return") return true;
        const list = s.data.@"return";

        if (!v.self.options.flags.LuauCompileMoveElision) {
            v.returns_one = v.returns_one and list.len == 1 and !isExprMultRet(v.self, list[0]);
            return false;
        }

        if (list.len == 1 and !isExprMultRet(v.self, list[0])) {
            const unwrapped = an.unwrapExpr(list[0]);
            if (unwrapped.data == .local) {
                const expr_local = unwrapped.data.local;
                var kind: ReturnKind = .local;
                var local: ?*ast.Local = expr_local.local;

                for (v.func.args) |el| {
                    if (el == expr_local.local) {
                        kind = .parameter;
                        break;
                    }
                }

                if (expr_local.upvalue) {
                    kind = .expression;
                    local = null;
                }

                if (v.return_kind == .none) {
                    v.return_kind = kind;
                    v.return_local = local;
                } else if (v.return_kind != kind or v.return_local != local) {
                    v.return_kind = .unknown;
                    v.return_local = null;
                }
            } else {
                if (v.return_kind == .none) {
                    v.return_kind = .expression;
                } else if (v.return_kind != .expression) {
                    v.return_kind = .unknown;
                    v.return_local = null;
                }
            }
        } else {
            v.returns_one = false;
            v.return_kind = .unknown;
            v.return_local = null;
        }

        return false;
    }
};

fn tryCompileInlinedCall(self: *Compiler, call_node: *Expr, func_node: *Expr, target: u8, target_count: u8, mult_ret: bool, threshold_base: i32, threshold_max_boost: i32, depth_limit: usize) Error!bool {
    const expr = call_node.data.call;
    const func = &func_node.data.function;
    const fi = self.functions.get(func_node).?;

    // make sure we have enough register space
    if (self.reg_top > 128 or fi.stack_size > 32) return false;

    // we should ideally aggregate the costs during recursive inlining, but for now simply limit the depth
    if (self.inline_frames.items.len >= depth_limit) return false;

    // compiling recursive inlining is difficult because we share constant/variable state but need to bind variables to different registers
    for (self.inline_frames.items) |frame| {
        if (frame.func == func_node) return false;
    }

    // we can't inline multret functions because the caller expects L->top to be adjusted
    if (mult_ret) return false;

    // compute constant bitvector for all arguments to feed the cost model
    var varc: [8]bool = @splat(false);
    var has_constant = false;
    var i: usize = 0;
    while (i < func.args.len and i < expr.args.len and i < 8) : (i += 1) {
        if (isConstant(self, expr.args[i])) {
            varc[i] = true;
            has_constant = true;
        }
    }

    // if the last argument only returns a single value, all following arguments are nil
    if (expr.args.len != 0 and !isExprMultRet(self, expr.args[expr.args.len - 1])) {
        i = expr.args.len;
        while (i < func.args.len and i < 8) : (i += 1) {
            varc[i] = true;
            has_constant = true;
        }
    }

    // If we had constant arguments that can affect the cost model of this specific call in non-trivial ways
    var call_cost_model = fi.cost_model;

    if (has_constant) call_cost_model = try costModelInlinedCall(self, call_node, func_node);

    // we use a dynamic cost threshold that's based on the fixed limit boosted by the cost advantage we gain due to inlining
    const inlined_cost = cost_model.computeCost(call_cost_model, varc[0..@min(func.args.len, 8)]);
    const baseline_cost = cost_model.computeCost(fi.cost_model, &.{}) + 3;
    const inline_profit: i32 = if (inlined_cost == 0) threshold_max_boost else @min(threshold_max_boost, @divTrunc(100 * baseline_cost, inlined_cost));

    const threshold = @divTrunc(threshold_base * inline_profit, 100);
    const is_iife = an.unwrapExpr(expr.func).data == .function;

    if (inlined_cost > threshold and !is_iife) return false;

    try compileInlinedCall(self, call_node, func_node, target, target_count);
    return true;
}

fn costModelInlinedCall(self: *Compiler, call_node: *Expr, func_node: *Expr) Error!u64 {
    const expr = call_node.data.call;
    const func = &func_node.data.function;

    for (func.args, 0..) |v, i| {
        const arg: ?*Expr = if (i < expr.args.len) expr.args[i] else null;

        // last expression is a multret, there are no constant for it and it will fill all values
        if (i + 1 == expr.args.len and func.args.len > expr.args.len and isExprMultRet(self, arg.?)) break;

        // variable gets mutated at some point, so we do not have a constant for it
        if (self.variables.get(v)) |vv| {
            if (vv.written) continue;
        }

        if (arg == null) {
            try self.locstants.put(self.gpa, v, .nil);
        } else if (self.constants.get(arg.?)) |cv| {
            if (cv.known()) try self.locstants.put(self.gpa, v, cv);
        }
    }

    // fold constant values updated above into expressions in the function body, recording changes for undo
    self.expr_changes.clearRetainingCapacity();
    self.local_changes.clearRetainingCapacity();

    try an.foldConstants(self.gpa, &self.constants, &self.variables, &self.locstants, foldOptions(self), func.body, self.names, &self.table_constants, &self.expr_changes, &self.local_changes);

    // model the cost of the function evaluated with current constants
    const cost = try cost_model.modelCost(self.gpa, func.body, func.args, &self.builtins, &self.constants);

    // clean up constant state for future inlining attempts
    for (func.args) |v| {
        if (self.locstants.getPtr(v)) |c| c.* = .unknown;
    }

    try an.undoExprChanges(self.gpa, &self.constants, self.expr_changes.items);
    try an.undoLocalChanges(self.gpa, &self.locstants, self.local_changes.items);

    return cost;
}

const FindRegisterUseByLocal = struct {
    self: *Compiler,
    reg: u8,
    used: bool = false,

    pub fn visitExpr(v: *FindRegisterUseByLocal, e: *Expr) Error!bool {
        if (e.data == .local) {
            if (getLocalReg(v.self, e.data.local.local)) |reg| {
                if (reg == v.reg) v.used = true;
            }
        }
        return !v.used;
    }

    pub fn visitStat(v: *FindRegisterUseByLocal, _: *Stat) Error!bool {
        return !v.used;
    }
};

fn isRegisterUsedInCallArguments(self: *Compiler, expr: ast.Call, arg: ?*Expr, start_arg: usize, reg: u8) Error!bool {
    var visitor: FindRegisterUseByLocal = .{ .self = self, .reg = reg };

    if (self.options.flags.LuauCompileMoveElisionFix) {
        // Allow direct use of the target register in the current argument
        if (arg) |a| {
            if (!isExprLocalReg(self, a, reg)) try an.walkExpr(FindRegisterUseByLocal, &visitor, expr.args[start_arg]);
        }

        var i = start_arg + 1;
        while (i < expr.args.len and !visitor.used) : (i += 1) try an.walkExpr(FindRegisterUseByLocal, &visitor, expr.args[i]);
    } else {
        var i = start_arg;
        while (i < expr.args.len and !visitor.used) : (i += 1) try an.walkExpr(FindRegisterUseByLocal, &visitor, expr.args[i]);
    }

    return visitor.used;
}

fn compileInlinedCall(self: *Compiler, call_node: *Expr, func_node: *Expr, target: u8, target_count: u8) Error!void {
    const expr = call_node.data.call;
    const func = &func_node.data.function;

    const move_elision = self.options.flags.LuauCompileMoveElision;
    const fi: ?FunctionInfo = if (move_elision) self.functions.get(func_node) else null;

    // We try to write result directly to the target register when possible, but there are a few blockers
    // - function captures a reference to the local that is being returned and might close it
    // - there might not be a target register (return value is unused)
    // - target register is captured and might be read before inlined function completes
    const no_target_move_elision = move_elision and (fi.?.return_local_captured or target_count == 0 or self.reg_captured[target]);

    const old_top = self.reg_top;
    defer self.reg_top = old_top;

    const old_locals = self.local_stack.items.len;

    var args: std.ArrayList(InlineArg) = .empty;
    try args.ensureTotalCapacity(self.gpa, func.args.len);

    // evaluate all arguments; note that we don't emit code for constant arguments (relying on constant folding)
    // note that compiler state (variable registers/values) does not change here - we defer that to a separate loop below to handle nested calls
    for (func.args, 0..) |v, i| {
        const arg: ?*Expr = if (i < expr.args.len) expr.args[i] else null;
        const can_use_parameter_target = move_elision and fi.?.return_kind == .parameter and fi.?.return_local == v and !no_target_move_elision;
        const reg_use_start = if (self.options.flags.LuauCompileMoveElisionFix) i else i + 1;

        if (i + 1 == expr.args.len and func.args.len > expr.args.len and isExprMultRet(self, arg.?)) {
            // if the last argument can return multiple values, we need to compute all of them into the remaining arguments
            const tail: u32 = @intCast(func.args.len - expr.args.len + 1);
            const reg = try allocReg(self, arg.?.location, tail);
            const allocpc = self.bytecode.getDebugPC();

            switch (arg.?.data) {
                .call => try compileExprCall(self, arg.?, reg, @intCast(tail), true, false),
                .varargs => try compileExprVarargs(self, arg.?, reg, @intCast(tail), false),
                else => unreachable,
            }

            for (func.args[i..], 0..) |rest, j| try args.append(self.gpa, .{ .local = rest, .reg = @intCast(reg + j), .value = .unknown, .allocpc = allocpc });

            // all remaining function arguments have been allocated and assigned to
            break;
        } else if (self.variables.get(v) != null and self.variables.get(v).?.written) {
            if (can_use_parameter_target and !try isRegisterUsedInCallArguments(self, expr, arg, reg_use_start, target)) {
                if (arg) |a|
                    try compileExprTemp(self, a, target)
                else
                    try self.bytecode.emitABC(.LOADNIL, target, 0, 0);

                try args.append(self.gpa, .{ .local = v, .reg = target, .value = .unknown });
            } else {
                // if the argument is mutated, we need to allocate a fresh register even if it's a constant
                const reg = try allocReg(self, if (arg) |a| a.location else call_node.location, 1);
                const allocpc = self.bytecode.getDebugPC();

                if (arg) |a|
                    try compileExprTemp(self, a, reg)
                else
                    try self.bytecode.emitABC(.LOADNIL, reg, 0, 0);

                try args.append(self.gpa, .{ .local = v, .reg = reg, .value = .unknown, .allocpc = allocpc });
            }
        } else if (arg == null) {
            // since the argument is not mutated, we can simply fold the value into the expressions that need it
            try args.append(self.gpa, .{ .local = v, .reg = kInvalidReg, .value = .nil });
        } else if (self.constants.get(arg.?) != null and self.constants.get(arg.?).?.known()) {
            // since the argument is not mutated, we can simply fold the value into the expressions that need it
            try args.append(self.gpa, .{ .local = v, .reg = kInvalidReg, .value = self.constants.get(arg.?).? });
        } else {
            const le = getExprLocal(arg.?);
            const lv: ?an.Variable = if (le) |l| self.variables.get(l) else null;
            const reg: ?u8 = if (le) |l| getLocalReg(self, l) else null;

            // if the argument is a local that isn't mutated, we will simply reuse the existing register
            if (reg != null and (lv == null or !lv.?.written)) {
                try args.append(self.gpa, .{ .local = v, .reg = reg.?, .value = .unknown, .init = if (lv) |x| x.init else null });
            } else if (can_use_parameter_target and !try isRegisterUsedInCallArguments(self, expr, arg, reg_use_start, target)) {
                try compileExprTemp(self, arg.?, target);

                try args.append(self.gpa, .{ .local = v, .reg = target, .value = .unknown });
            } else {
                var temp = try allocReg(self, arg.?.location, 1);
                const allocpc = self.bytecode.getDebugPC();

                if (move_elision)
                    temp = try compileExprAutoTemp(self, arg.?, temp, target)
                else
                    try compileExprTemp(self, arg.?, temp);

                try args.append(self.gpa, .{ .local = v, .reg = temp, .value = .unknown, .allocpc = allocpc, .init = arg });
            }
        }
    }

    // evaluate extra expressions for side effects
    if (expr.args.len > func.args.len) {
        for (expr.args[func.args.len..]) |e| try compileExprSide(self, e);
    }

    // apply all evaluated arguments to the compiler state
    // note: locals use current startpc for debug info, although some of them have been computed earlier; this is similar to compileStatLocal
    for (args.items) |a| {
        if (a.value == .unknown) {
            try pushLocal(self, a.local, a.reg, a.allocpc);

            if (a.init) |init_expr| {
                if (self.variables.getPtr(a.local)) |lv| lv.init = init_expr;
            }
        } else {
            try self.locstants.put(self.gpa, a.local, a.value);
        }
    }

    const result_local: ?*ast.Local = if (move_elision and fi.?.return_kind == .local and !no_target_move_elision) fi.?.return_local else null;

    // the inline frame will be used to compile return statements as well as to reject recursive inlining attempts
    try self.inline_frames.append(self.gpa, .{ .func = func_node, .local_offset = old_locals, .target = target, .target_count = target_count, .result_local = result_local });

    // this pass tracks which calls are builtins and can be compiled more efficiently
    try an.analyzeBuiltins(self.gpa, &self.inline_builtins, &self.globals, &self.variables, builtinOptions(self), self.options.disabled_builtins, func.body, self.names);

    // If we found new builtins, apply them, but record which expressions we changed so we can undo later
    if (self.inline_builtins.count() != 0) {
        var it = self.inline_builtins.iterator();
        while (it.next()) |entry| {
            const gop = try self.builtins.getOrPut(self.gpa, entry.key_ptr.*);
            if (!gop.found_existing) gop.value_ptr.* = .NONE; // If there was no builtin previously, we will get LBF_NONE

            if (entry.value_ptr.* != gop.value_ptr.*) {
                try self.inline_builtins_backup.put(self.gpa, entry.key_ptr.*, gop.value_ptr.*);
                gop.value_ptr.* = entry.value_ptr.*;
            }
        }

        self.inline_builtins.clearRetainingCapacity();
    }

    // fold constant values updated above into expressions in the function body, recording changes for undo
    self.expr_changes.clearRetainingCapacity();
    self.local_changes.clearRetainingCapacity();

    try an.foldConstants(self.gpa, &self.constants, &self.variables, &self.locstants, foldOptions(self), func.body, self.names, &self.table_constants, &self.expr_changes, &self.local_changes);

    var terminates_early = false;

    for (func.body.data.block.body) |stat| {
        if (self.options.flags.LuauCompileReuseLocalRegs) try popStatLocals(self, func.body, stat, old_locals);

        try compileStat(self, stat);

        if (alwaysTerminates(self, stat)) {
            terminates_early = true;

            // Remove the last jump which jumps directly to the next instruction
            const curr_frame = &self.inline_frames.items[self.inline_frames.items.len - 1];
            const jumps = &curr_frame.return_jumps;
            if (jumps.items.len > 0 and jumps.items[jumps.items.len - 1] == self.bytecode.emitLabel() - 1) {
                self.bytecode.undoEmit(.JUMP);
                _ = jumps.pop();
            }
            break;
        }
    }

    // for the fallthrough path we need to ensure we clear out target registers
    if (!terminates_early) {
        var t: u32 = 0;
        while (t < target_count) : (t += 1) try self.bytecode.emitABC(.LOADNIL, @intCast(target + t), 0, 0);

        try closeLocals(self, old_locals);
    }

    try popLocals(self, old_locals);

    const return_label = self.bytecode.emitLabel();
    try patchJumps(self, call_node.location, self.inline_frames.items[self.inline_frames.items.len - 1].return_jumps.items, return_label);

    _ = self.inline_frames.pop();

    // clean up constant state for future inlining attempts
    for (func.args) |local| {
        if (self.locstants.getPtr(local)) |c| c.* = .unknown;
        if (self.variables.getPtr(local)) |lv| lv.init = null;
    }

    if (self.inline_builtins_backup.count() != 0) {
        var it = self.inline_builtins_backup.iterator();
        while (it.next()) |entry| try self.builtins.put(self.gpa, entry.key_ptr.*, entry.value_ptr.*);
        self.inline_builtins_backup.clearRetainingCapacity();
    }

    try an.undoExprChanges(self.gpa, &self.constants, self.expr_changes.items);
    try an.undoLocalChanges(self.gpa, &self.locstants, self.local_changes.items);
}

fn compileInlineReturn(self: *Compiler, node: *Stat) Error!void {
    setDebugLine(self, node.location); // normally compileStat sets up line info, but compileInlineReturn can be called directly

    const frame = self.inline_frames.items[self.inline_frames.items.len - 1];

    try compileExprListTemp(self, node.data.@"return", frame.target, frame.target_count, false);

    try closeLocals(self, frame.local_offset);

    const jump_label = self.bytecode.emitLabel();
    try self.bytecode.emitAD(.JUMP, 0, 0);

    try self.inline_frames.items[self.inline_frames.items.len - 1].return_jumps.append(self.gpa, jump_label);
}

// ------------------------------------------------------------------ loop unrolling (optimization level 2)

fn tryCompileUnrolledFor(self: *Compiler, node: *Stat, threshold_base: i32, threshold_max_boost: i32) Error!bool {
    const stat = node.data.@"for";
    const one: Constant = .{ .number = 1.0 };

    const fromc = getConstant(self, stat.from);
    const toc = getConstant(self, stat.to);
    const stepc = if (stat.step) |s| getConstant(self, s) else one;

    const trip_count: i32 = if (fromc == .number and toc == .number and stepc == .number) cost_model.getTripCount(fromc.number, toc.number, stepc.number) else -1;

    if (trip_count < 0) return false;

    // loop unroll succeeded: empty loop
    if (trip_count == 0) return true;

    if (trip_count > threshold_base) return false;

    if (self.variables.get(stat.variable)) |lv| {
        if (lv.written) return false;
    }

    const var_list = [_]*ast.Local{stat.variable};
    const model = try cost_model.modelCost(self.gpa, stat.body, &var_list, &self.builtins, &self.constants);

    // we use a dynamic cost threshold that's based on the fixed limit boosted by the cost advantage we gain due to unrolling
    const varc = [_]bool{true};
    const unrolled_cost = cost_model.computeCost(model, &varc) * trip_count;
    const baseline_cost = (cost_model.computeCost(model, &.{}) + 1) * trip_count;
    const unroll_profit: i32 = if (unrolled_cost == 0) threshold_max_boost else @min(threshold_max_boost, @divTrunc(100 * baseline_cost, unrolled_cost));

    const threshold = @divTrunc(threshold_base * unroll_profit, 100);

    if (unrolled_cost > threshold) return false;

    try compileUnrolledFor(self, node, trip_count, fromc.number, stepc.number);
    return true;
}

fn compileUnrolledFor(self: *Compiler, node: *Stat, trip_count: i32, from: f64, step: f64) Error!void {
    const stat = node.data.@"for";
    const v = stat.variable;

    const old_locals = self.local_stack.items.len;
    const old_jumps = self.loop_jumps.items.len;

    try self.loops.append(self.gpa, .{ .local_offset = old_locals, .local_offset_continue = old_locals, .continue_used = null });

    // record changes on the first iteration to capture the pre-loop state
    self.expr_changes.clearRetainingCapacity();
    self.local_changes.clearRetainingCapacity();

    var iv: i32 = 0;
    while (iv < trip_count) : (iv += 1) {
        // we need to re-fold constants in the loop body with the new value; this reuses computed constant values elsewhere in the tree
        try self.locstants.put(self.gpa, v, .{ .number = from + @as(f64, @floatFromInt(iv)) * step });

        if (iv == 0)
            try an.foldConstants(self.gpa, &self.constants, &self.variables, &self.locstants, foldOptions(self), node, self.names, &self.table_constants, &self.expr_changes, &self.local_changes)
        else
            try an.foldConstants(self.gpa, &self.constants, &self.variables, &self.locstants, foldOptions(self), node, self.names, &self.table_constants, null, null);

        const iter_jumps = self.loop_jumps.items.len;

        try compileStat(self, stat.body);

        // all continue jumps need to go to the next iteration
        const cont_label = self.bytecode.emitLabel();

        for (self.loop_jumps.items[iter_jumps..]) |lj| {
            if (lj.type == .cont) try patchJump(self, node.location, lj.label, cont_label);
        }
    }

    // all break jumps need to go past the loop
    const end_label = self.bytecode.emitLabel();

    for (self.loop_jumps.items[old_jumps..]) |lj| {
        if (lj.type == .brk) try patchJump(self, node.location, lj.label, end_label);
    }

    self.loop_jumps.shrinkRetainingCapacity(old_jumps);

    _ = self.loops.pop();

    // clean up fold state in case we need to recompile - normally we compile the loop body once, but due to inlining we may need to do it again
    try self.locstants.put(self.gpa, v, .unknown);

    try an.undoExprChanges(self.gpa, &self.constants, self.expr_changes.items);
    try an.undoLocalChanges(self.gpa, &self.locstants, self.local_changes.items);
}

/// C atoi: optional leading whitespace and sign, then decimal digits; saturates instead of overflowing.
fn atoi(s: []const u8) i64 {
    var i: usize = 0;
    while (i < s.len and lex.isSpace(s[i])) i += 1;
    var neg = false;
    if (i < s.len and (s[i] == '+' or s[i] == '-')) {
        neg = s[i] == '-';
        i += 1;
    }
    var v: i64 = 0;
    while (i < s.len and lex.isDigit(s[i])) : (i += 1) {
        v = std.math.mul(i64, v, 10) catch std.math.maxInt(i64);
        v = std.math.add(i64, v, s[i] - '0') catch std.math.maxInt(i64);
    }
    return if (neg) -v else v;
}
