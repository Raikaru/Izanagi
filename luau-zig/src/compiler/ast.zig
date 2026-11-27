//! Luau syntax tree.
//!
//! Mirrors the node set of third_party/luau/Ast/include/Luau/Ast.h (Luau, MIT
//! License; see third_party/luau/LICENSE.txt) for the syntax accepted by the
//! pinned parser with default flags. Concrete-syntax (CST) data is not kept.
//! Nodes are arena-allocated and referenced by pointer; node identity is used
//! by the compiler analyses the same way upstream uses AstNode pointers.

const lexer = @import("lexer.zig");
pub const Location = lexer.Location;
pub const Position = lexer.Position;

/// Interned name (pointer-stable for the lifetime of the NameTable).
pub const Name = []const u8;

pub fn nameEql(a: Name, b: Name) bool {
    if (a.ptr == b.ptr and a.len == b.len) return true;
    return @import("std").mem.eql(u8, a, b);
}

pub const Local = struct {
    name: Name,
    location: Location,
    shadow: ?*Local,
    function_depth: u32,
    loop_depth: u32,
    is_const: bool,
    annotation: ?*Type,
    /// LuauExportValueSyntax: binding is stored in the module export table
    is_exported: bool = false,
};

pub const AttrType = enum { checked, native, deprecated, debug_noinline, unknown };

pub const Attr = struct {
    location: Location,
    type: AttrType,
    args: []const *Expr,
    name: Name,
};

pub const UnaryOp = enum { not, minus, len };

pub const BinaryOp = enum(u8) {
    add,
    sub,
    mul,
    div,
    floor_div,
    mod,
    pow,
    concat,
    compare_ne,
    compare_eq,
    compare_lt,
    compare_le,
    compare_gt,
    compare_ge,
    @"and",
    @"or",
};

pub const TableItemKind = enum { list, record, general };

pub const TableItem = struct {
    kind: TableItemKind,
    key: ?*Expr,
    value: *Expr,
};

pub const Function = struct {
    attributes: []const *Attr,
    generics: []const *GenericType,
    generic_packs: []const *GenericTypePack,
    self: ?*Local,
    args: []const *Local,
    return_annotation: ?*TypePack,
    vararg: bool,
    vararg_location: Location,
    vararg_annotation: ?*TypePack,
    body: *Stat,
    function_depth: u32,
    debugname: ?Name,

    pub fn hasAttribute(self: *const Function, t: AttrType) bool {
        for (self.attributes) |a| if (a.type == t) return true;
        return false;
    }
    pub fn hasNativeAttribute(self: *const Function) bool {
        return self.hasAttribute(.native);
    }
};

pub const Call = struct {
    func: *Expr,
    args: []const *Expr,
    self: bool,
    table_call: bool,
    type_arguments: []const TypeOrPack,
    arg_location: Location,
};

pub const IndexName = struct {
    expr: *Expr,
    index: Name,
    index_location: Location,
    op_position: Position,
    op: u8,
    /// DebugLuauUserDefinedClasses: `_name` access inside a class method body
    private_access: bool = false,
};

pub const IfElse = struct {
    condition: *Expr,
    has_then: bool,
    true_expr: *Expr,
    has_else: bool,
    false_expr: *Expr,
    /// Active for 'if local' and 'if const' expressions (LuauExperimentalIfLocalSyntax); bound to `condition`, in scope for true_expr only
    condition_local: ?*Local = null,
    condition_is_const: bool = false,
};

pub const Expr = struct {
    location: Location,
    data: Data,

    pub const Data = union(enum) {
        group: *Expr,
        constant_nil,
        constant_bool: bool,
        constant_number: f64,
        constant_string: []const u8,
        /// LuauIntegerType2 integer literal (`123i`)
        constant_integer: i64,
        local: struct { local: *Local, upvalue: bool },
        global: Name,
        varargs,
        call: Call,
        index_name: IndexName,
        index_expr: struct { expr: *Expr, index: *Expr },
        function: Function,
        table: []const TableItem,
        unary: struct { op: UnaryOp, expr: *Expr },
        binary: struct { op: BinaryOp, left: *Expr, right: *Expr },
        type_assertion: struct { expr: *Expr, annotation: *Type },
        if_else: IfElse,
        interp_string: struct { strings: []const []const u8, expressions: []const *Expr },
        instantiate: struct { expr: *Expr, type_arguments: []const TypeOrPack },
    };
};

pub const Stat = struct {
    location: Location,
    has_semicolon: bool = false,
    data: Data,

    pub const Data = union(enum) {
        block: Block,
        @"if": If,
        @"while": struct { condition: *Expr, body: *Stat },
        repeat: struct { condition: *Expr, body: *Stat },
        @"break",
        @"continue",
        @"return": []const *Expr,
        expr: *Expr,
        local: struct { vars: []const *Local, values: []const *Expr, is_const: bool },
        @"for": struct { variable: *Local, from: *Expr, to: *Expr, step: ?*Expr, body: *Stat },
        for_in: struct { vars: []const *Local, values: []const *Expr, body: *Stat },
        assign: struct { vars: []const *Expr, values: []const *Expr },
        compound_assign: struct { op: BinaryOp, variable: *Expr, value: *Expr },
        function: struct { name: *Expr, func: *Expr },
        local_function: struct { name: *Local, func: *Expr, is_const: bool },
        type_alias: struct { name: Name, generics: []const *GenericType, generic_packs: []const *GenericTypePack, type: *Type, exported: bool },
        type_function: struct { name: Name, body: *Expr, exported: bool },
        /// DebugLuauUserDefinedClasses
        class: Class,
    };

    pub const Block = struct {
        body: []const *Stat,
        has_end: bool,
    };

    pub const ClassAccess = enum { public, private };

    pub const ClassMember = union(enum) {
        property: struct { name: Name, name_location: Location, type: ?*Type, access: ClassAccess },
        method: struct { name: Name, name_location: Location, function: *Expr, access: ClassAccess },
    };

    pub const Class = struct {
        name: *Local,
        super: ?*Expr,
        members: []const ClassMember,
        exported: bool,
        open: bool,
    };

    pub const If = struct {
        condition: *Expr,
        thenbody: *Stat,
        elsebody: ?*Stat,
        /// Active for 'if local' and 'if const' statements (LuauExperimentalIfLocalSyntax)
        condition_local: ?*Local = null,
        condition_is_const: bool = false,
    };
};

pub const GenericType = struct {
    location: Location,
    name: Name,
    default_value: ?*Type,
};

pub const GenericTypePack = struct {
    location: Location,
    name: Name,
    default_value: ?*TypePack,
};

pub const TypeOrPack = struct {
    type: ?*Type = null,
    type_pack: ?*TypePack = null,
};

pub const TableAccess = enum { read, write, read_write };

pub const TableProp = struct {
    name: Name,
    location: Location,
    type: *Type,
    access: TableAccess,
};

pub const TableIndexer = struct {
    index_type: *Type,
    result_type: *Type,
    location: Location,
    access: TableAccess,
};

pub const ArgumentName = struct { name: Name, location: Location };

pub const TypeList = struct {
    types: []const *Type,
    tail_type: ?*TypePack,
};

pub const Type = struct {
    location: Location,
    data: Data,

    pub const Data = union(enum) {
        /// `synthetic` marks compiler-created builtin types (upstream BuiltinAstTypes), whose AstName never matches
        /// a parsed alias or userdata name because upstream compares AstNames by pointer.
        reference: struct { prefix: ?Name, name: Name, has_parameters: bool, parameters: []const TypeOrPack, synthetic: bool = false },
        table: struct { props: []const TableProp, indexer: ?*TableIndexer },
        function: struct {
            attributes: []const *Attr,
            generics: []const *GenericType,
            generic_packs: []const *GenericTypePack,
            arg_types: TypeList,
            arg_names: []const ?ArgumentName,
            return_types: *TypePack,
        },
        typeof: *Expr,
        optional,
        @"union": []const *Type,
        intersection: []const *Type,
        singleton_bool: bool,
        singleton_string: []const u8,
        group: *Type,
    };
};

pub const TypePack = struct {
    location: Location,
    data: Data,

    pub const Data = union(enum) {
        explicit: TypeList,
        variadic: *Type,
        generic: Name,
    };
};

pub const HotComment = struct {
    header: bool,
    location: Location,
    content: []const u8,
};
