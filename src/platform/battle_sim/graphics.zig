const builtin = @import("builtin");
pub const panic = @import("panic.zig").panic;
const c = @import("c_bindings");
extern fn __acrt_iob_func(c_uint) ?*c.FILE;
extern var __stderrp: ?*c.FILE;
extern var stderr: ?*c.FILE;
fn cStderr() ?*c.FILE {
    if (builtin.os.tag == .windows) return __acrt_iob_func(2);
    if (comptime builtin.os.tag.isDarwin()) return __stderrp;
    return stderr;
}
const glcall: stdcall = if (builtin.os.tag == .windows) .winapi else .c;
const stdcall = @import("std").builtin.CallingConvention;
const U = c_uint;
const I = c_int;
const B = u8;
const F = f32;

// The device operations intentionally have no effects; CPU-side game rendering still runs.
fn sink0() callconv(glcall) void {}
fn sinkU(_: U) callconv(glcall) void {}
fn sinkUU(_: U, _: U) callconv(glcall) void {}
fn sinkUI(_: U, _: I) callconv(glcall) void {}
fn sinkB(_: B) callconv(glcall) void {}
fn sinkII(_: I, _: I) callconv(glcall) void {}
fn sinkIF(_: I, _: F) callconv(glcall) void {}
fn sinkIFF(_: I, _: F, _: F) callconv(glcall) void {}
fn sinkIFFF(_: I, _: F, _: F, _: F) callconv(glcall) void {}
fn sinkIFFFF(_: I, _: F, _: F, _: F, _: F) callconv(glcall) void {}
fn sinkFFFF(_: F, _: F, _: F, _: F) callconv(glcall) void {}
fn sinkBBBB(_: B, _: B, _: B, _: B) callconv(glcall) void {}
fn sinkUUU(_: U, _: U, _: U) callconv(glcall) void {}
fn sinkUUUU(_: U, _: U, _: U, _: U) callconv(glcall) void {}
fn sinkUUI(_: U, _: U, _: I) callconv(glcall) void {}
fn sinkUII(_: U, _: I, _: I) callconv(glcall) void {}
fn sinkUIII(_: U, _: I, _: I, _: I) callconv(glcall) void {}
fn sinkIptrU(_: I, _: [*c]const U) callconv(glcall) void {}
fn sinkIIfptr(_: I, _: I, _: [*c]const F) callconv(glcall) void {}
fn sinkIIBfptr(_: I, _: I, _: B, _: [*c]const F) callconv(glcall) void {}
fn sinkUUptr(_: U, _: I, _: U, _: ?*const anyopaque) callconv(glcall) void {}
fn sinkBuffer(_: U, _: isize, _: ?*const anyopaque, _: U) callconv(glcall) void {}
fn sinkAttrib(_: U, _: I, _: U, _: B, _: I, _: ?*const anyopaque) callconv(glcall) void {}
fn sinkSource(_: U, _: I, _: [*c]const [*c]const c.GLchar, _: [*c]const I) callconv(glcall) void {}
fn sinkImage(_: U, _: I, _: I, _: I, _: I, _: I, _: U, _: U, _: ?*const anyopaque) callconv(glcall) void {}
fn sinkSubImage(_: U, _: I, _: I, _: I, _: I, _: I, _: U, _: U, _: ?*const anyopaque) callconv(glcall) void {}
fn sinkCopyImage(_: U, _: I, _: I, _: I, _: I, _: I, _: I, _: I) callconv(glcall) void {}
fn sinkBlit(_: I, _: I, _: I, _: I, _: I, _: I, _: I, _: I, _: U, _: U) callconv(glcall) void {}
fn sinkFramebufferTexture(_: U, _: U, _: U, _: U, _: I) callconv(glcall) void {}
fn sinkRenderbufferStorage(_: U, _: U, _: I, _: I) callconv(glcall) void {}

var next_name: U = 1;
var texture: U = 0;
var depth: U = 0;
var read_framebuffer: U = 0;
var draw_framebuffer: U = 0;
var viewport: [4]I = .{ 0, 0, 640, 448 };
fn names(n: I, objects: [*c]U) callconv(glcall) void {
    var i: I = 0;
    while (i < n) : (i += 1) {
        if (next_name == 0) c.abort();
        objects[@intCast(i)] = next_name;
        next_name +%= 1;
    }
}
fn createProgram() callconv(glcall) U { var name: U = undefined; names(1, &name); return name; }
fn createShader(_: U) callconv(glcall) U { return createProgram(); }
fn bindTexture(_: U, object: U) callconv(glcall) void { texture = object; }
fn bindDepth(_: U, object: U) callconv(glcall) void { depth = object; }
fn bindFramebuffer(target: U, object: U) callconv(glcall) void {
    if (target == c.GL_READ_FRAMEBUFFER or target == c.GL_FRAMEBUFFER) read_framebuffer = object;
    if (target == c.GL_DRAW_FRAMEBUFFER or target == c.GL_FRAMEBUFFER) draw_framebuffer = object;
}
fn setViewport(x: I, y: I, w: I, h: I) callconv(glcall) void { viewport = .{ x, y, w, h }; }
fn noError() callconv(glcall) U { return c.GL_NO_ERROR; }
fn framebufferStatus(_: U) callconv(glcall) U { return c.GL_FRAMEBUFFER_COMPLETE; }
fn uniformLocation(_: U, _: [*c]const c.GLchar) callconv(glcall) I { return -1; }
fn objectStatus(_: U, name: U, value: [*c]I) callconv(glcall) void { value.* = if (name == c.GL_COMPILE_STATUS or name == c.GL_LINK_STATUS) 1 else 0; }
fn objectLog(_: U, size: I, length: [*c]I, log: [*c]c.GLchar) callconv(glcall) void {
    if (length != null) length.* = 0;
    if (size > 0 and log != null) log[0] = 0;
}
fn description(name: U) callconv(glcall) [*c]const u8 {
    return if (name == c.GL_RENDERER) "battle-sim/no-device" else if (name == c.GL_VERSION) "3.3 no-device" else "";
}
fn extension(_: U, _: U) callconv(glcall) [*c]const u8 { return ""; }
fn integer(name: U, value: [*c]I) callconv(glcall) void {
    switch (name) {
        0x0ba2 => for (viewport, 0..) |v, i| { value[i] = v; },
        0x0d33, 0x84e8 => value.* = 16384,
        0x8caa => value.* = @bitCast(read_framebuffer),
        0x8ca6 => value.* = @bitCast(draw_framebuffer),
        0x8069 => value.* = @bitCast(texture),
        0x8ca7 => value.* = @bitCast(depth),
        else => value.* = 0,
    }
}
fn forbiddenRead(_: I, _: I, _: I, _: I, _: U, _: U, _: ?*anyopaque) callconv(glcall) void {
    _ = c.fputs("p4pc: battle-sim: unexpected pixel read in no-device mode\n", cStderr());
    c.exit(4);
}

export fn p4_battle_sim_graphics_open() c_int {
    if (c.p4_battle_sim_renderless == 0) return -1;
    c.p4gl_glDeleteTextures = sinkIptrU;
    c.p4gl_glTexImage2D = sinkImage;
    c.p4gl_glCopyTexSubImage2D = sinkCopyImage;
    c.p4gl_glActiveTexture = sinkU;
    c.p4gl_glTexSubImage2D = sinkSubImage;
    c.p4gl_glTexParameteri = sinkUUI;
    c.p4gl_glPixelStorei = sinkUI;
    c.p4gl_glClearColor = sinkFFFF;
    c.p4gl_glClear = sinkU;
    c.p4gl_glFlush = sink0;
    c.p4gl_glEnable = sinkU;
    c.p4gl_glDisable = sinkU;
    c.p4gl_glBlendFunc = sinkUU;
    c.p4gl_glBindVertexArray = sinkU;
    c.p4gl_glBindBuffer = sinkUU;
    c.p4gl_glBufferData = sinkBuffer;
    c.p4gl_glEnableVertexAttribArray = sinkU;
    c.p4gl_glVertexAttribPointer = sinkAttrib;
    c.p4gl_glVertexAttribDivisor = sinkUU;
    c.p4gl_glDeleteShader = sinkU;
    c.p4gl_glShaderSource = sinkSource;
    c.p4gl_glCompileShader = sinkU;
    c.p4gl_glDeleteProgram = sinkU;
    c.p4gl_glAttachShader = sinkUU;
    c.p4gl_glLinkProgram = sinkU;
    c.p4gl_glUseProgram = sinkU;
    c.p4gl_glUniform2f = sinkIFF;
    c.p4gl_glUniform1i = sinkII;
    c.p4gl_glUniform1f = sinkIF;
    c.p4gl_glDrawArrays = sinkUII;
    c.p4gl_glDrawArraysInstanced = sinkUIII;
    c.p4gl_glDrawElements = sinkUUptr;
    c.p4gl_glDeleteBuffers = sinkIptrU;
    c.p4gl_glDepthFunc = sinkU;
    c.p4gl_glDepthMask = sinkB;
    c.p4gl_glCullFace = sinkU;
    c.p4gl_glBlitFramebuffer = sinkBlit;
    c.p4gl_glFrontFace = sinkU;
    c.p4gl_glColorMask = sinkBBBB;
    c.p4gl_glBlendEquation = sinkU;
    c.p4gl_glBlendFuncSeparate = sinkUUUU;
    c.p4gl_glBlendColor = sinkFFFF;
    c.p4gl_glUniform3f = sinkIFFF;
    c.p4gl_glUniform3fv = sinkIIfptr;
    c.p4gl_glUniform4f = sinkIFFFF;
    c.p4gl_glUniformMatrix3fv = sinkIIBfptr;
    c.p4gl_glUniformMatrix4fv = sinkIIBfptr;
    c.p4gl_glFramebufferTexture2D = sinkFramebufferTexture;
    c.p4gl_glDeleteFramebuffers = sinkIptrU;
    c.p4gl_glDeleteRenderbuffers = sinkIptrU;
    c.p4gl_glRenderbufferStorage = sinkRenderbufferStorage;
    c.p4gl_glFramebufferRenderbuffer = sinkUUUU;
    c.p4gl_glGenTextures = names;
    c.p4gl_glGenVertexArrays = names;
    c.p4gl_glGenBuffers = names;
    c.p4gl_glGenFramebuffers = names;
    c.p4gl_glGenRenderbuffers = names;
    c.p4gl_glBindTexture = bindTexture;
    c.p4gl_glBindRenderbuffer = bindDepth;
    c.p4gl_glBindFramebuffer = bindFramebuffer;
    c.p4gl_glViewport = setViewport;
    c.p4gl_glGetError = noError;
    c.p4gl_glCheckFramebufferStatus = framebufferStatus;
    c.p4gl_glCreateProgram = createProgram;
    c.p4gl_glCreateShader = createShader;
    c.p4gl_glGetUniformLocation = uniformLocation;
    c.p4gl_glGetShaderiv = objectStatus;
    c.p4gl_glGetProgramiv = objectStatus;
    c.p4gl_glGetShaderInfoLog = objectLog;
    c.p4gl_glGetString = description;
    c.p4gl_glGetStringi = extension;
    c.p4gl_glGetIntegerv = integer;
    c.p4gl_glReadPixels = forbiddenRead;
    c.p4gl_gl_set_draw(0);
    _ = c.fputs("p4pc: battle-sim: no-device graphics (no window, context, submission or readback)\n", cStderr());
    return 0;
}
