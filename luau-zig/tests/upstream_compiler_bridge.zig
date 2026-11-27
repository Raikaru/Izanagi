//! Test-only C ABI bridge to the Zig compiler. Returned storage is allocated by
//! libc malloc for the C++ test host to free; no upstream compiler is linked.
const std = @import("std");
const compiler = @import("compiler");
const vm_hash = @import("vm_hash");
extern "c" fn malloc(usize) ?*anyopaque;

export fn zig_conformance_vm_hash(data: [*]const u8, len: usize) callconv(.c) u32 {
    return vm_hash.hash(data[0..len]);
}

export fn zig_conformance_compiler_hash(data: [*]const u8, len: usize) callconv(.c) u32 {
    return compiler.builder.BytecodeBuilder.getStringHash(data[0..len]);
}

export fn zig_conformance_compile(
    source: [*]const u8,
    length: usize,
    optimization: c_int,
    debug: c_int,
    type_info: c_int,
    coverage: c_int,
    case_name: [*:0]const u8,
    bytes: *?[*]u8,
    size: *usize,
) callconv(.c) c_int {
    bytes.* = null;
    size.* = 0;
    if (optimization < 0 or optimization > 2 or debug < 0 or debug > 2 or
        type_info < 0 or type_info > 1 or coverage < 0 or coverage > 2) return 77;
    const allocator = std.heap.c_allocator;
    const name = std.mem.span(case_name);
    var options: compiler.Options = .{
        .optimization_level = @intCast(optimization),
        .debug_level = @intCast(debug),
        .type_info_level = @intCast(type_info),
        .coverage_level = @intCast(coverage),
    };
    if (std.mem.startsWith(u8, name, "Fastcall:")) {
        var tokens = std.mem.splitScalar(u8, name["Fastcall:".len..], ',');
        const id_text = tokens.next() orelse return 77;
        const id = std.fmt.parseInt(u8, id_text, 10) catch return 77;
        if (id == 54) {
            options.vector_lib = "vector";
            options.vector_ctor = "create";
        }
        while (tokens.next()) |flag| {
            if (flag.len == 0) continue;
            if (std.mem.eql(u8, flag, "LuauIntegerFastcalls")) {
                options.flags.LuauIntegerFastcalls = true;
            } else if (std.mem.eql(u8, flag, "LuauIntegerBufferFastcalls")) {
                options.flags.LuauIntegerBufferFastcalls = true;
            } else if (std.mem.eql(u8, flag, "LuauCompileTableBuiltinsCreateClear")) {
                options.flags.LuauCompileTableBuiltinsCreateClear = true;
            } else if (!std.mem.eql(u8, flag, "LuauIntegerLibrary")) {
                return 77;
            }
        }
    }
    if (std.mem.eql(u8, name, "IfLocal"))
        options.flags.LuauExperimentalIfLocalSyntax = true;
    if (std.mem.eql(u8, name, "Vector"))
        options.flags.LuauCompileNoFoldVectorEqW = true;
    if (std.mem.eql(u8, name, "Integers") or std.mem.eql(u8, name, "NativeIntegerSpills") or std.mem.eql(u8, name, "Types"))
        options.flags.LuauIntegerType2 = true;
    if (std.mem.eql(u8, name, "ExportEdgeCase"))
        options.flags.LuauExportValueSyntax = true;
    if (std.mem.eql(u8, name, "NativeUserdata"))
        options.userdata_types = &.{ "vec2", "color", "mat3", "vertex" };
    if (std.mem.eql(u8, name, "FeedbackEnabled") or std.mem.eql(u8, name, "InlinerEnabled")) {
        options.flags.LuauEmitCallFeedback = true;
        options.flags.LuauCompileCallFbRemap = true;
    }
    if (std.mem.eql(u8, name, "Classes") or
        std.mem.eql(u8, name, "ClassInheritanceRepeatedCallMemberOffsetCorruption"))
    {
        options.flags.DebugLuauUserDefinedClasses = true;
        options.flags.LuauEmitCallFeedback = true;
        options.flags.LuauBytecodeCostModel = true;
    }
    var result = compiler.compile(allocator, source[0..length], options) catch return 2;
    defer result.deinit(allocator);
    const dst: [*]u8 = @ptrCast(malloc(@max(result.bytecode.len, 1)) orelse return 2);
    @memcpy(dst[0..result.bytecode.len], result.bytecode);
    bytes.* = dst;
    size.* = result.bytecode.len;
    return 0;
}
