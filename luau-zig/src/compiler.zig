//! Luau source -> bytecode compiler written in Zig.
//!
//! Port of the pinned upstream Luau front end and compiler in third_party/luau
//! (Ast/, Compiler/, Bytecode/BytecodeBuilder; Luau is MIT licensed, see
//! third_party/luau/LICENSE.txt). The bytecode targets the format of
//! third_party/luau/Common/include/Luau/Bytecode.h: version 13 by default,
//! version 14 with LuauCompileFastpcall and version 100 with experimental
//! classes. Conformance coverage and unsupported variants are recorded in
//! luau-zig/tests/README.txt; compatibility is not established for every flag.
//!
//! Public interface:
//!   `compile(gpa, source, options)` returns an `Output` whose `bytecode` is
//!   either a loadable blob (first byte = bytecode version) or an upstream
//!   compatible error blob (first byte 0 followed by ":<line>: <message>"),
//!   exactly like `Luau::compile`. On failure `diagnostic` carries the
//!   structured location/message as well. All returned memory is owned by the
//!   caller and released with `Output.deinit(gpa)`; transient compiler state is
//!   arena-allocated from `gpa` and freed before returning, so an allocator with
//!   a byte budget bounds compiler memory. The only error is OutOfMemory.
//!
//! `Options` mirrors upstream CompileOptions (optimization levels 0-2 including
//! inlining/unrolling/builtin folding, debug levels, type info levels, coverage,
//! vector lib/ctor/type/precision, mutable globals, userdata types, library
//! member type/constant callbacks, disabled builtins). Hot comments
//! (`--!optimize N`, `--!native`) and `@native` behave as upstream.
//!
//! Feature flags: `Options.flags` exposes the pinned upstream parser/compiler/
//! bytecode-builder flags (all off by default). Flag availability does not by
//! itself establish that a variant matches upstream; see the suite coverage.
//!
//! Limits of the port (stated, never faked):
//!   * Where upstream iterates pointer-keyed hash containers (exported classes
//!     and, with LuauOptimizeExportTable, exported functions in the export
//!     table epilogue), this port uses declaration order; upstream's order
//!     depends on DenseHashMap bucket placement of AST node addresses, so the
//!     SETTABLEKS order of that epilogue may differ while the resulting export
//!     table is identical.
//!   * ParseOptions are the upstream defaults (no declaration syntax).
//!   * Builtin folding at O2 uses Zig std.math (no C libm); transcendental
//!     results can differ from a host libm in the last ulp.

const std = @import("std");
const Allocator = std.mem.Allocator;

pub const bytecode = @import("compiler/bytecode.zig");
pub const ast = @import("compiler/ast.zig");
pub const lexer = @import("compiler/lexer.zig");
pub const parser = @import("compiler/parser.zig");
pub const analysis = @import("compiler/analysis.zig");
pub const types = @import("compiler/types.zig");
pub const builder = @import("compiler/builder.zig");
pub const codegen = @import("compiler/codegen.zig");

pub const Options = codegen.Options;
pub const Location = lexer.Location;
pub const Position = lexer.Position;
pub const LibraryConstant = analysis.LibraryConstant;
pub const LibraryMemberConstantCallback = analysis.LibraryMemberConstantCallback;
pub const LibraryMemberTypeCallback = types.LibraryMemberTypeCallback;

pub const DiagnosticKind = enum {
    /// Upstream ParseError (lexing/parsing, including syntax limits)
    syntax,
    /// Upstream CompileError (register/constant/jump limits, repeat..until continue validation, ...)
    compile,
};

pub const Diagnostic = struct {
    kind: DiagnosticKind,
    /// 0-based positions as in upstream Luau::Location; the error blob reports `location.begin.line + 1`.
    location: Location,
    /// Caller-owned message text (identical to upstream ParseError/CompileError::what()).
    message: []u8,
};

pub const Output = struct {
    /// Caller-owned blob in upstream luau_compile format (valid bytecode or 0-prefixed error).
    bytecode: []u8,
    /// Present when `bytecode` is an error blob.
    diagnostic: ?Diagnostic = null,

    pub fn ok(self: Output) bool {
        return self.diagnostic == null;
    }

    pub fn deinit(self: *Output, gpa: Allocator) void {
        gpa.free(self.bytecode);
        if (self.diagnostic) |d| gpa.free(d.message);
        self.* = undefined;
    }
};

/// Upstream feature flags, see compiler/flags.zig.
pub const Flags = @import("compiler/flags.zig").Flags;

/// Compiles Luau `source` into an upstream-compatible bytecode blob (see module documentation).
pub fn compile(gpa: Allocator, source: []const u8, options: Options) error{OutOfMemory}!Output {
    var arena_state = std.heap.ArenaAllocator.init(gpa);
    defer arena_state.deinit();
    const arena = arena_state.allocator();

    var names = try lexer.NameTable.init(arena);

    var parse_diag: parser.Diagnostic = .{};
    const parsed = parser.Parser.parse(arena, source, &names, &parse_diag, options.flags) catch |err| switch (err) {
        error.OutOfMemory => return error.OutOfMemory,
        error.ParseError => return failure(gpa, .syntax, parse_diag.location, parse_diag.message),
    };

    var bcb = builder.BytecodeBuilder.init(arena);
    var compile_diag: codegen.Diagnostic = .{};
    codegen.compileOrThrow(arena, &bcb, parsed.root, parsed.hotcomments, &names, options, &compile_diag) catch |err| switch (err) {
        error.OutOfMemory => return error.OutOfMemory,
        error.CompileError => return failure(gpa, .compile, compile_diag.location, compile_diag.message),
    };

    return .{ .bytecode = try gpa.dupe(u8, bcb.getBytecode()) };
}

fn failure(gpa: Allocator, kind: DiagnosticKind, location: Location, message: []const u8) error{OutOfMemory}!Output {
    // Users of this function expect only a single error message: ":%d: %s"
    const text = try std.fmt.allocPrint(gpa, ":{d}: {s}", .{ location.begin.line + 1, message });
    defer gpa.free(text);

    const blob = try builder.BytecodeBuilder.getError(gpa, text);
    errdefer gpa.free(blob);

    return .{
        .bytecode = blob,
        .diagnostic = .{ .kind = kind, .location = location, .message = try gpa.dupe(u8, message) },
    };
}

