//! Bytecode definitions shared by the Zig Luau compiler.
//!
//! This is a direct transcription of the pinned upstream contract in
//! third_party/luau/Common/include/Luau/Bytecode.h and BytecodeUtils.h
//! (Luau, MIT License; see third_party/luau/LICENSE.txt). All enums are
//! order-sensitive: their numeric values are baked into serialized bytecode.

pub const Opcode = enum(u8) {
    NOP,
    BREAK,
    LOADNIL,
    LOADB,
    LOADN,
    LOADK,
    MOVE,
    GETGLOBAL,
    SETGLOBAL,
    GETUPVAL,
    SETUPVAL,
    CLOSEUPVALS,
    GETIMPORT,
    GETTABLE,
    SETTABLE,
    GETTABLEKS,
    SETTABLEKS,
    GETTABLEN,
    SETTABLEN,
    NEWCLOSURE,
    NAMECALL,
    CALL,
    RETURN,
    JUMP,
    JUMPBACK,
    JUMPIF,
    JUMPIFNOT,
    JUMPIFEQ,
    JUMPIFLE,
    JUMPIFLT,
    JUMPIFNOTEQ,
    JUMPIFNOTLE,
    JUMPIFNOTLT,
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    POW,
    ADDK,
    SUBK,
    MULK,
    DIVK,
    MODK,
    POWK,
    AND,
    OR,
    ANDK,
    ORK,
    CONCAT,
    NOT,
    MINUS,
    LENGTH,
    NEWTABLE,
    DUPTABLE,
    SETLIST,
    FORNPREP,
    FORNLOOP,
    FORGLOOP,
    FORGPREP_INEXT,
    FASTCALL3,
    FORGPREP_NEXT,
    NATIVECALL,
    GETVARARGS,
    DUPCLOSURE,
    PREPVARARGS,
    LOADKX,
    JUMPX,
    FASTCALL,
    COVERAGE,
    CAPTURE,
    SUBRK,
    DIVRK,
    FASTCALL1,
    FASTCALL2,
    FASTCALL2K,
    FORGPREP,
    JUMPXEQKNIL,
    JUMPXEQKB,
    JUMPXEQKN,
    JUMPXEQKS,
    IDIV,
    IDIVK,
    GETUDATAKS,
    SETUDATAKS,
    NAMECALLUDATA,
    NEWCLASSMEMBER,
    CALLFB,
    CMPPROTO,
    FASTPCALL,
    NEWCLASS,
    CONSTRUCT,
    FINCONSTRUCT,
    GETPRIVATEFIELD,
    SETPRIVATEFIELD,
    _COUNT,
};

// Bytecode version; runtime supports [MIN, MAX], compiler emits TARGET by default.
pub const LBC_VERSION_MIN: u8 = 3;
pub const LBC_VERSION_MAX: u8 = 14;
pub const LBC_VERSION_TARGET: u8 = 13;
pub const LBC_VERSION_CLASSES: u8 = 100;
// Type encoding version
pub const LBC_TYPE_VERSION_MIN: u8 = 1;
pub const LBC_TYPE_VERSION_MAX: u8 = 3;
pub const LBC_TYPE_VERSION_TARGET: u8 = 3;

// Types of constant table entries
pub const LBC_CONSTANT_NIL: u8 = 0;
pub const LBC_CONSTANT_BOOLEAN: u8 = 1;
pub const LBC_CONSTANT_NUMBER: u8 = 2;
pub const LBC_CONSTANT_STRING: u8 = 3;
pub const LBC_CONSTANT_IMPORT: u8 = 4;
pub const LBC_CONSTANT_TABLE: u8 = 5;
pub const LBC_CONSTANT_CLOSURE: u8 = 6;
pub const LBC_CONSTANT_VECTOR: u8 = 7;
pub const LBC_CONSTANT_TABLE_WITH_CONSTANTS: u8 = 8;
pub const LBC_CONSTANT_INTEGER: u8 = 9;
pub const LBC_CONSTANT_CLASS_SHAPE: u8 = 10;
pub const LBC_CONSTANT_VECTORD: u8 = 11;

// Type table tags
pub const LBC_TYPE_NIL: u8 = 0;
pub const LBC_TYPE_BOOLEAN: u8 = 1;
pub const LBC_TYPE_NUMBER: u8 = 2;
pub const LBC_TYPE_STRING: u8 = 3;
pub const LBC_TYPE_TABLE: u8 = 4;
pub const LBC_TYPE_FUNCTION: u8 = 5;
pub const LBC_TYPE_THREAD: u8 = 6;
pub const LBC_TYPE_USERDATA: u8 = 7;
pub const LBC_TYPE_VECTOR: u8 = 8;
pub const LBC_TYPE_BUFFER: u8 = 9;
pub const LBC_TYPE_INTEGER: u8 = 10;
pub const LBC_TYPE_CLASS: u8 = 11;
pub const LBC_TYPE_OBJECT: u8 = 12;
pub const LBC_TYPE_ANY: u8 = 15;
pub const LBC_TYPE_TAGGED_USERDATA_BASE: u8 = 64;
pub const LBC_TYPE_TAGGED_USERDATA_END: u8 = 64 + 32;
pub const LBC_TYPE_OPTIONAL_BIT: u8 = 1 << 7;
pub const LBC_TYPE_INVALID: u16 = 256;

/// Builtin function ids, used in LOP_FASTCALL
pub const Builtin = enum(u8) {
    NONE = 0,
    ASSERT,
    MATH_ABS,
    MATH_ACOS,
    MATH_ASIN,
    MATH_ATAN2,
    MATH_ATAN,
    MATH_CEIL,
    MATH_COSH,
    MATH_COS,
    MATH_DEG,
    MATH_EXP,
    MATH_FLOOR,
    MATH_FMOD,
    MATH_FREXP,
    MATH_LDEXP,
    MATH_LOG10,
    MATH_LOG,
    MATH_MAX,
    MATH_MIN,
    MATH_MODF,
    MATH_POW,
    MATH_RAD,
    MATH_SINH,
    MATH_SIN,
    MATH_SQRT,
    MATH_TANH,
    MATH_TAN,
    BIT32_ARSHIFT,
    BIT32_BAND,
    BIT32_BNOT,
    BIT32_BOR,
    BIT32_BXOR,
    BIT32_BTEST,
    BIT32_EXTRACT,
    BIT32_LROTATE,
    BIT32_LSHIFT,
    BIT32_REPLACE,
    BIT32_RROTATE,
    BIT32_RSHIFT,
    TYPE,
    STRING_BYTE,
    STRING_CHAR,
    STRING_LEN,
    TYPEOF,
    STRING_SUB,
    MATH_CLAMP,
    MATH_SIGN,
    MATH_ROUND,
    RAWSET,
    RAWGET,
    RAWEQUAL,
    TABLE_INSERT,
    TABLE_UNPACK,
    VECTOR,
    BIT32_COUNTLZ,
    BIT32_COUNTRZ,
    SELECT_VARARG,
    RAWLEN,
    BIT32_EXTRACTK,
    GETMETATABLE,
    SETMETATABLE,
    TONUMBER,
    TOSTRING,
    BIT32_BYTESWAP,
    BUFFER_READI8,
    BUFFER_READU8,
    BUFFER_WRITEU8,
    BUFFER_READI16,
    BUFFER_READU16,
    BUFFER_WRITEU16,
    BUFFER_READI32,
    BUFFER_READU32,
    BUFFER_WRITEU32,
    BUFFER_READF32,
    BUFFER_WRITEF32,
    BUFFER_READF64,
    BUFFER_WRITEF64,
    VECTOR_MAGNITUDE,
    VECTOR_NORMALIZE,
    VECTOR_CROSS,
    VECTOR_DOT,
    VECTOR_FLOOR,
    VECTOR_CEIL,
    VECTOR_ABS,
    VECTOR_SIGN,
    VECTOR_CLAMP,
    VECTOR_MIN,
    VECTOR_MAX,
    MATH_LERP,
    VECTOR_LERP,
    MATH_ISNAN,
    MATH_ISINF,
    MATH_ISFINITE,
    INTEGER_CREATE,
    INTEGER_TONUMBER,
    INTEGER_NEG,
    INTEGER_ADD,
    INTEGER_SUB,
    INTEGER_MUL,
    INTEGER_DIV,
    INTEGER_MIN,
    INTEGER_MAX,
    INTEGER_REM,
    INTEGER_IDIV,
    INTEGER_UDIV,
    INTEGER_UREM,
    INTEGER_MOD,
    INTEGER_CLAMP,
    INTEGER_BAND,
    INTEGER_BOR,
    INTEGER_BNOT,
    INTEGER_BXOR,
    INTEGER_LT,
    INTEGER_LE,
    INTEGER_ULT,
    INTEGER_ULE,
    INTEGER_GT,
    INTEGER_GE,
    INTEGER_UGT,
    INTEGER_UGE,
    INTEGER_LSHIFT,
    INTEGER_RSHIFT,
    INTEGER_ARSHIFT,
    INTEGER_LROTATE,
    INTEGER_RROTATE,
    INTEGER_EXTRACT,
    INTEGER_BTEST,
    INTEGER_COUNTRZ,
    INTEGER_COUNTLZ,
    INTEGER_BSWAP,
    BUFFER_READINTEGER,
    BUFFER_WRITEINTEGER,
    TABLE_CREATE,
    TABLE_CLEAR,
};

// Capture type, used in LOP_CAPTURE
pub const LCT_VAL: u8 = 0;
pub const LCT_REF: u8 = 1;
pub const LCT_UPVAL: u8 = 2;

// Proto flag bitmask, stored in Proto::flags
pub const LPF_NATIVE_MODULE: u8 = 1 << 0;
pub const LPF_NATIVE_COLD: u8 = 1 << 1;
pub const LPF_NATIVE_FUNCTION: u8 = 1 << 2;
pub const LPF_INLINABLE: u8 = 1 << 3;
pub const LPF_USES_EXPORT: u8 = 1 << 4;

pub inline fn insnOp(insn: u32) u8 {
    return @truncate(insn);
}
pub inline fn insnA(insn: u32) u8 {
    return @truncate(insn >> 8);
}
pub inline fn insnB(insn: u32) u8 {
    return @truncate(insn >> 16);
}
pub inline fn insnC(insn: u32) u8 {
    return @truncate(insn >> 24);
}
pub inline fn insnD(insn: u32) i32 {
    return @as(i32, @bitCast(insn)) >> 16;
}
pub inline fn insnE(insn: u32) i32 {
    return @as(i32, @bitCast(insn)) >> 8;
}

pub fn getOpLength(op: Opcode) u32 {
    return switch (op) {
        .GETGLOBAL, .SETGLOBAL, .GETIMPORT, .GETTABLEKS, .SETTABLEKS, .NAMECALL, .JUMPIFEQ, .JUMPIFLE, .JUMPIFLT, .JUMPIFNOTEQ, .JUMPIFNOTLE, .JUMPIFNOTLT, .NEWTABLE, .SETLIST, .FORGLOOP, .LOADKX, .FASTCALL2, .FASTCALL2K, .FASTCALL3, .JUMPXEQKNIL, .JUMPXEQKB, .JUMPXEQKN, .JUMPXEQKS, .GETUDATAKS, .SETUDATAKS, .NAMECALLUDATA, .NEWCLASSMEMBER, .CALLFB, .CMPPROTO, .NEWCLASS, .CONSTRUCT, .GETPRIVATEFIELD, .SETPRIVATEFIELD => 2,
        else => 1,
    };
}

pub fn isFastCall(op: Opcode) bool {
    return switch (op) {
        .FASTCALL, .FASTCALL1, .FASTCALL2, .FASTCALL2K, .FASTCALL3, .FASTPCALL, .FINCONSTRUCT => true,
        else => false,
    };
}

pub fn isJumpD(op: Opcode) bool {
    return switch (op) {
        .JUMP, .JUMPIF, .JUMPIFNOT, .JUMPIFEQ, .JUMPIFLE, .JUMPIFLT, .JUMPIFNOTEQ, .JUMPIFNOTLE, .JUMPIFNOTLT, .FORNPREP, .FORNLOOP, .FORGPREP, .FORGLOOP, .FORGPREP_INEXT, .FORGPREP_NEXT, .JUMPBACK, .JUMPXEQKNIL, .JUMPXEQKB, .JUMPXEQKN, .JUMPXEQKS, .CMPPROTO => true,
        else => false,
    };
}

pub fn isSkipC(op: Opcode) bool {
    return op == .LOADB;
}
