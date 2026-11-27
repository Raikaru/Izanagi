//! Non-local error unwinding for the Luau VM, written in Zig (naked functions with
//! inline assembly) instead of C setjmp/longjmp.
//!
//! `protect(buf, f, ud)` saves the callee-saved register state of its caller in
//! `buf`, calls `f(ud)` and returns 0. `jump(buf)` restores that state and makes
//! the pending `protect` call return 1. Because `protect` is an ordinary (opaque)
//! call from the optimizer's point of view and every callee-saved register is
//! restored to its value at entry, no "returns twice" semantics are required.
//!
//! Frames skipped by `jump` are discarded without running cleanup code, exactly like
//! upstream Luau built with LUA_USE_LONGJMP=1. Callers must not hold resources that
//! need cleanup (Zig `defer`, C++ destructors) across a call that can raise.
//!
//! Control-flow integrity:
//! - x86_64 (Linux SHSTK and Windows hardware-enforced stack protection): `protect`
//!   records the shadow stack pointer (RDSSPQ) and `jump` unwinds the shadow stack
//!   with INCSSPQ before transferring control, like glibc longjmp. On CPUs or
//!   processes without shadow stacks those instructions execute as NOPs.
//! - aarch64: entry points start with a BTI landing pad. Guarded Control Stacks
//!   (Linux GCS) cannot be unwound here; `shadowStackUnsupported` reports an active
//!   GCS, or a status query failure other than "feature not implemented", so state
//!   creation fails closed instead of crashing on the first error.
//! - x86_64 requires the LLVM backend (vm.zig rejects the self-hosted backend at
//!   compile time); the runtime fail-closed path for builds without CET unwinding
//!   remains as a defensive check.
//! - Windows x64: the naked frames have no unwind tables, so SEH/C++ exceptions must
//!   not be thrown through VM frames (they are not by the VM itself).
//! - x86 (32-bit): restores the SEH chain head (fs:0) on Windows; 32-bit shadow
//!   stacks are not used by any supported OS.

const std = @import("std");
const builtin = @import("builtin");

pub const JmpBuf = extern struct {
    regs: [32]u64 align(16) = undefined,
};

pub const Body = *const fn (?*anyopaque) callconv(.c) void;

const ProtectFn = *const fn (*JmpBuf, Body, ?*anyopaque) callconv(.c) c_int;
const JumpFn = *const fn (*JmpBuf) callconv(.c) noreturn;

const arch = builtin.cpu.arch;
const is_windows = builtin.os.tag == .windows;

/// INVARIANT: the naked bodies must never be inlined, have their arguments removed,
/// or have attributes inferred from them; they are real C-ABI functions whose asm
/// reads the argument registers directly. They are therefore `noinline` and reached
/// only through function pointers loaded with volatile reads, which keeps them
/// address-taken and opaque to LLVM's inliner, dead-argument elimination and
/// attribute inference (all of which previously broke optimized builds: the asm was
/// inlined into the caller with the wrong registers, and code after the call was
/// treated as unreachable).
var protect_entry: ProtectFn = @ptrCast(&protectNaked);
var jump_entry: JumpFn = @ptrCast(&jumpNaked);

/// Runs `body(ud)`; returns 0 if it returned normally, 1 if `jump` was used.
pub inline fn protect(buf: *JmpBuf, body: Body, ud: ?*anyopaque) c_int {
    const f = @as(*volatile const ProtectFn, &protect_entry).*;
    return f(buf, body, ud);
}

/// Transfers control to the `protect` call that filled `buf`, which returns 1.
pub inline fn jump(buf: *JmpBuf) noreturn {
    const f = @as(*volatile const JumpFn, &jump_entry).*;
    f(buf);
}

// ---------------------------------------------------------------------------
// x86_64 shadow-stack instructions
// ---------------------------------------------------------------------------
//
// With LLVM, RDSSPQ/INCSSPQ are emitted as raw bytes (LLVM's inline assembler
// rejects the mnemonics unless the whole target enables shstk). Zig's self-hosted
// x86_64 backend has neither a `.byte` directive nor encodings for these
// instructions, so builds with that backend contain no shadow-stack unwinding and
// `shadowStackUnsupported` makes state creation fail closed whenever the process
// actually runs with a shadow stack. CET is never silently disabled.
// RDSSPQ leaves rax unchanged (0) when shadow stacks are disabled, which skips the
// unwinding loop; the SSP recorded in protect points at protect's own return entry
// and jump pops up to and including it, since control returns with `jmp`.

pub const x86_64_cet_unwinding = builtin.zig_backend == .stage2_llvm;
const RDSSPQ_RAX = if (x86_64_cet_unwinding) ".byte 0xf3, 0x48, 0x0f, 0x1e, 0xc8\n" else "";
const INCSSPQ_RDX = ".byte 0xf3, 0x48, 0x0f, 0xae, 0xea\n";

/// Pops shadow-stack entries down to the SSP saved at `off(%reg)` plus protect's
/// own return entry. Clobbers rax, rdx, r11.
fn unwindShadowStack(comptime reg: []const u8, comptime off: []const u8) []const u8 {
    if (!x86_64_cet_unwinding) return "";
    return "xorl %%eax, %%eax\n" ++ RDSSPQ_RAX ++
        \\testq %%rax, %%rax
        \\jz 2f
        \\movq 
    ++ off ++ "(%%" ++ reg ++ "), %%r11\n" ++
        \\subq %%rax, %%r11
        \\shrq $3, %%r11
        \\addq $1, %%r11
        \\1:
        \\testq %%r11, %%r11
        \\jz 2f
        \\movq %%r11, %%rdx
        \\cmpq $255, %%rdx
        \\jbe 3f
        \\movl $255, %%edx
        \\3:
        \\
    ++ INCSSPQ_RDX ++
        \\subq %%rdx, %%r11
        \\jmp 1b
        \\2:
        \\
    ;
}

/// True when state creation must fail closed: the process runs with a shadow stack
/// that this build cannot unwind (aarch64 Linux Guarded Control Stack, or x86_64
/// built without CET unwinding), or that status could not be determined.
///
/// A failed status query counts as "shadow stack possibly active" except for the
/// one documented signal that the OS does not implement the feature at all:
/// - Linux: `EINVAL` from the status request, which kernels without user shadow
///   stack (x86_64) or GCS (aarch64) support return for the unknown option code.
///   `EPERM`, `ENOSYS` (e.g. seccomp filters) and anything else fail closed. A
///   seccomp filter that fabricates `EINVAL` would be indistinguishable.
/// - Windows: `ERROR_INVALID_PARAMETER` from `GetProcessMitigationPolicy`, which
///   systems that predate the user shadow stack policy return for the unknown
///   policy class. Any other failure fails closed.
/// - macOS (x86_64) has no user-mode shadow stacks; any other OS fails closed.
pub fn shadowStackUnsupported() bool {
    if (arch == .x86_64 and !x86_64_cet_unwinding) return x86ShadowStackPossiblyActive();
    if (arch == .aarch64 and builtin.os.tag == .linux) {
        const PR_GET_SHADOW_STACK_STATUS = 74;
        const PR_SHADOW_STACK_ENABLE: c_ulong = 1;
        var status: c_ulong = 0;
        const rc = std.os.linux.syscall5(.prctl, PR_GET_SHADOW_STACK_STATUS, @intFromPtr(&status), 0, 0, 0);
        return linuxStatusPossiblyActive(rc, status & PR_SHADOW_STACK_ENABLE != 0);
    }
    return false;
}

const EINVAL = 22;

fn linuxStatusPossiblyActive(rc: usize, enabled: bool) bool {
    const signed: isize = @bitCast(rc);
    if (signed == -EINVAL) return false; // the kernel does not implement the feature
    if (signed < 0) return true; // unknown failure: fail closed
    return enabled;
}

extern "kernel32" fn GetCurrentProcess() callconv(.winapi) ?*anyopaque;
extern "kernel32" fn GetProcessMitigationPolicy(process: ?*anyopaque, policy: c_int, buffer: *u32, length: usize) callconv(.winapi) c_int;
extern "kernel32" fn GetLastError() callconv(.winapi) u32;

/// Whether this x86_64 process may run with a user-mode shadow stack.
fn x86ShadowStackPossiblyActive() bool {
    if (builtin.os.tag == .linux) {
        const ARCH_SHSTK_STATUS = 0x5005;
        const ARCH_SHSTK_SHSTK: u64 = 1;
        var features: u64 = 0;
        const rc = std.os.linux.syscall2(.arch_prctl, ARCH_SHSTK_STATUS, @intFromPtr(&features));
        return linuxStatusPossiblyActive(rc, features & ARCH_SHSTK_SHSTK != 0);
    } else if (builtin.os.tag == .windows) {
        const ProcessUserShadowStackPolicy = 16;
        const ERROR_INVALID_PARAMETER = 87;
        var flags: u32 = 0;
        if (GetProcessMitigationPolicy(GetCurrentProcess(), ProcessUserShadowStackPolicy, &flags, @sizeOf(u32)) == 0)
            return GetLastError() != ERROR_INVALID_PARAMETER;
        return flags & 1 != 0; // EnableUserShadowStack
    }
    // macOS does not implement user-mode shadow stacks on x86_64; other operating
    // systems have no supported query, so fail closed
    if (builtin.os.tag.isDarwin()) return false;
    return true;
}

/// Declared with the exact C signature and a non-noreturn type: this function
/// returns (0 normally, 1 via `jump`). Declaring it `noreturn` lets LLVM treat code
/// after the call as unreachable, which corrupted control flow after a normal return.
noinline fn protectNaked(_: *JmpBuf, _: Body, _: ?*anyopaque) callconv(.naked) c_int {
    switch (arch) {
        .x86_64 => if (is_windows) {
            asm volatile (
                \\movq %%rbx, 0(%%rcx)
                \\movq %%rbp, 8(%%rcx)
                \\movq %%rdi, 16(%%rcx)
                \\movq %%rsi, 24(%%rcx)
                \\movq %%r12, 32(%%rcx)
                \\movq %%r13, 40(%%rcx)
                \\movq %%r14, 48(%%rcx)
                \\movq %%r15, 56(%%rcx)
                \\leaq 8(%%rsp), %%rax
                \\movq %%rax, 64(%%rcx)
                \\movq (%%rsp), %%rax
                \\movq %%rax, 72(%%rcx)
                \\movdqu %%xmm6, 80(%%rcx)
                \\movdqu %%xmm7, 96(%%rcx)
                \\movdqu %%xmm8, 112(%%rcx)
                \\movdqu %%xmm9, 128(%%rcx)
                \\movdqu %%xmm10, 144(%%rcx)
                \\movdqu %%xmm11, 160(%%rcx)
                \\movdqu %%xmm12, 176(%%rcx)
                \\movdqu %%xmm13, 192(%%rcx)
                \\movdqu %%xmm14, 208(%%rcx)
                \\movdqu %%xmm15, 224(%%rcx)
                \\xorl %%eax, %%eax
                \\
            ++ RDSSPQ_RAX ++
                \\movq %%rax, 240(%%rcx)
                \\subq $40, %%rsp
                \\movq %%rdx, %%rax
                \\movq %%r8, %%rcx
                \\callq *%%rax
                \\addq $40, %%rsp
                \\xorl %%eax, %%eax
                \\retq
            );
        } else {
            asm volatile (
                \\movq %%rbx, 0(%%rdi)
                \\movq %%rbp, 8(%%rdi)
                \\movq %%r12, 16(%%rdi)
                \\movq %%r13, 24(%%rdi)
                \\movq %%r14, 32(%%rdi)
                \\movq %%r15, 40(%%rdi)
                \\leaq 8(%%rsp), %%rax
                \\movq %%rax, 48(%%rdi)
                \\movq (%%rsp), %%rax
                \\movq %%rax, 56(%%rdi)
                \\xorl %%eax, %%eax
                \\
            ++ RDSSPQ_RAX ++
                \\movq %%rax, 64(%%rdi)
                \\subq $8, %%rsp
                \\movq %%rsi, %%rax
                \\movq %%rdx, %%rdi
                \\callq *%%rax
                \\addq $8, %%rsp
                \\xorl %%eax, %%eax
                \\retq
            );
        },
        .aarch64 => asm volatile (
            \\hint #34
            \\stp x19, x20, [x0, #0]
            \\stp x21, x22, [x0, #16]
            \\stp x23, x24, [x0, #32]
            \\stp x25, x26, [x0, #48]
            \\stp x27, x28, [x0, #64]
            \\stp x29, x30, [x0, #80]
            \\mov x3, sp
            \\str x3, [x0, #96]
            \\stp d8, d9, [x0, #104]
            \\stp d10, d11, [x0, #120]
            \\stp d12, d13, [x0, #136]
            \\stp d14, d15, [x0, #152]
            \\stp x29, x30, [sp, #-16]!
            \\mov x29, sp
            \\mov x0, x2
            \\blr x1
            \\ldp x29, x30, [sp], #16
            \\mov w0, #0
            \\ret
        ),
        .x86 => if (is_windows) {
            asm volatile (
                \\movl 4(%%esp), %%eax
                \\movl %%ebx, 0(%%eax)
                \\movl %%esi, 4(%%eax)
                \\movl %%edi, 8(%%eax)
                \\movl %%ebp, 12(%%eax)
                \\leal 4(%%esp), %%ecx
                \\movl %%ecx, 16(%%eax)
                \\movl (%%esp), %%ecx
                \\movl %%ecx, 20(%%eax)
                \\movl %%fs:0, %%ecx
                \\movl %%ecx, 24(%%eax)
                \\movl 8(%%esp), %%ecx
                \\movl 12(%%esp), %%edx
                \\subl $8, %%esp
                \\pushl %%edx
                \\calll *%%ecx
                \\addl $12, %%esp
                \\xorl %%eax, %%eax
                \\retl
            );
        } else {
            asm volatile (
                \\movl 4(%%esp), %%eax
                \\movl %%ebx, 0(%%eax)
                \\movl %%esi, 4(%%eax)
                \\movl %%edi, 8(%%eax)
                \\movl %%ebp, 12(%%eax)
                \\leal 4(%%esp), %%ecx
                \\movl %%ecx, 16(%%eax)
                \\movl (%%esp), %%ecx
                \\movl %%ecx, 20(%%eax)
                \\movl 8(%%esp), %%ecx
                \\movl 12(%%esp), %%edx
                \\subl $8, %%esp
                \\pushl %%edx
                \\calll *%%ecx
                \\addl $12, %%esp
                \\xorl %%eax, %%eax
                \\retl
            );
        },
        else => @compileError("luau-zig VM: non-local unwinding is not implemented for this architecture"),
    }
}

noinline fn jumpNaked(_: *JmpBuf) callconv(.naked) noreturn {
    switch (arch) {
        .x86_64 => if (is_windows) {
            asm volatile (unwindShadowStack("rcx", "240") ++
                \\movq 0(%%rcx), %%rbx
                \\movq 8(%%rcx), %%rbp
                \\movq 16(%%rcx), %%rdi
                \\movq 24(%%rcx), %%rsi
                \\movq 32(%%rcx), %%r12
                \\movq 40(%%rcx), %%r13
                \\movq 48(%%rcx), %%r14
                \\movq 56(%%rcx), %%r15
                \\movdqu 80(%%rcx), %%xmm6
                \\movdqu 96(%%rcx), %%xmm7
                \\movdqu 112(%%rcx), %%xmm8
                \\movdqu 128(%%rcx), %%xmm9
                \\movdqu 144(%%rcx), %%xmm10
                \\movdqu 160(%%rcx), %%xmm11
                \\movdqu 176(%%rcx), %%xmm12
                \\movdqu 192(%%rcx), %%xmm13
                \\movdqu 208(%%rcx), %%xmm14
                \\movdqu 224(%%rcx), %%xmm15
                \\movq 64(%%rcx), %%rsp
                \\movl $1, %%eax
                \\jmpq *72(%%rcx)
            );
        } else {
            asm volatile (unwindShadowStack("rdi", "64") ++
                \\movq 0(%%rdi), %%rbx
                \\movq 8(%%rdi), %%rbp
                \\movq 16(%%rdi), %%r12
                \\movq 24(%%rdi), %%r13
                \\movq 32(%%rdi), %%r14
                \\movq 40(%%rdi), %%r15
                \\movq 48(%%rdi), %%rsp
                \\movl $1, %%eax
                \\jmpq *56(%%rdi)
            );
        },
        .aarch64 => asm volatile (
            \\hint #34
            \\ldp x19, x20, [x0, #0]
            \\ldp x21, x22, [x0, #16]
            \\ldp x23, x24, [x0, #32]
            \\ldp x25, x26, [x0, #48]
            \\ldp x27, x28, [x0, #64]
            \\ldp x29, x30, [x0, #80]
            \\ldr x3, [x0, #96]
            \\mov sp, x3
            \\ldp d8, d9, [x0, #104]
            \\ldp d10, d11, [x0, #120]
            \\ldp d12, d13, [x0, #136]
            \\ldp d14, d15, [x0, #152]
            \\mov w0, #1
            \\ret
        ),
        .x86 => if (is_windows) {
            asm volatile (
                \\movl 4(%%esp), %%edx
                \\movl 0(%%edx), %%ebx
                \\movl 4(%%edx), %%esi
                \\movl 8(%%edx), %%edi
                \\movl 12(%%edx), %%ebp
                \\movl 24(%%edx), %%ecx
                \\movl %%ecx, %%fs:0
                \\movl 16(%%edx), %%esp
                \\movl $1, %%eax
                \\jmpl *20(%%edx)
            );
        } else {
            asm volatile (
                \\movl 4(%%esp), %%edx
                \\movl 0(%%edx), %%ebx
                \\movl 4(%%edx), %%esi
                \\movl 8(%%edx), %%edi
                \\movl 12(%%edx), %%ebp
                \\movl 16(%%edx), %%esp
                \\movl $1, %%eax
                \\jmpl *20(%%edx)
            );
        },
        else => @compileError("luau-zig VM: non-local unwinding is not implemented for this architecture"),
    }
}
