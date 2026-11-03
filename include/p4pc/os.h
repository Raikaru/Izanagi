/* Host OS shim: everything the port needs from the operating system that is not portable C.
 * Two implementations, picked by CMake: src/platform/os/os_posix.c (Linux) and
 * src/platform/os/os_win32.c (32-bit Windows). Port code includes this header instead of
 * <sys/mman.h>, <ucontext.h>, <unistd.h> or <windows.h>; see docs/platform/windows.md. */
#ifndef P4PC_OS_H
#define P4PC_OS_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/* Installs fatal-fault diagnostics. POSIX: signals, registers, module offsets and a backtrace
 * in p4pc-crash.log beside P4PC_LOG_FILE (or stderr without a log path). Windows: vectored
 * exception diagnostics to stderr. Fatal faults exit with 128 + signal (Windows: 139). */
void p4_os_init(void);

/* First statement of each executable entry, before registering other exit handlers.
 * Captures the original argument vector; P4PC_TEST_RESTART=<marker> exercises a
 * once-only fresh-image restart and proves that the old exit handlers drained. */
int p4_os_restart_init(int argc, char **argv);
/* Schedule a fresh-process/image restart. Returns 0, or -1 without leaving the
 * current native error UI. The caller persists settings and calls exit(0) on 0.
 * Never resumes/re-enters SDL or the game. Late desktop launch failures after
 * teardown are logged and terminate with failure, not unsafe continuation.
 * On Android the request returns -1 if the separate-process handoff fails. */
int p4_os_request_restart(const char *reason);
#ifdef __ANDROID__
/* Internal JNI handoff; the helper waits for this process's Binder death. */
int p4_android_prepare_restart(void);
#else
/* Internal last-exit-handler tail, implemented by the host OS shim. */
int p4_os_restart_self(int argc, char **argv);
#endif

/* Sets an environment variable for this process; an existing value is kept unless overwrite is
 * non-zero. Returns 0, or -1. */
int p4_os_setenv(const char *name, const char *value, int overwrite);

/* Environment for a run that must show no window: Linux selects SDL's offscreen video driver
 * (EGL), Windows sets P4PC_HIDDEN_WINDOW=1 (a hidden window; the offscreen driver needs an EGL
 * library Windows does not have). Existing SDL_VIDEODRIVER / P4PC_HIDDEN_WINDOW values win. */
void p4_os_headless_env(void);

/* 1 if path names a directory (a symbolic link to one does not count where links exist). */
int p4_os_is_dir_nofollow(const char *path);
/* 1 for an ordinary file, excluding links/reparse points and directories. */
int p4_os_is_file_nofollow(const char *path);
/* Stable regular-file object identity and revision of an already-open stream.
 * Tokens are opaque/native, not a portable asset hash. No file data is read.
 * Returns0 or-1 (directories/devices/pipes and unavailable identity refused). */
typedef struct P4OSFileIdentity {
    uint64_t size, tokens[7];
} P4OSFileIdentity;
int p4_os_file_identity(FILE *stream, P4OSFileIdentity *out);

/* Defines the absolute linker symbol `name` at `addr` from top-level C (the object-file symbol
 * carries the target's C prefix: none on ELF, `_` on i686 COFF). */
#define P4_ASM_STR2(x) #x
#define P4_ASM_STR(x) P4_ASM_STR2(x)
#define P4_ABSOLUTE_SYMBOL(name, addr)                                                          \
    __asm__(".globl " P4_ASM_STR(__USER_LABEL_PREFIX__) #name "\n.set " P4_ASM_STR(__USER_LABEL_PREFIX__) \
            #name ", " #addr "\n")

/* Binds a declaration to the C symbol `name` (a string literal), as `__asm__(name)` does on ELF and x86_64 COFF. Mach-O takes an asm label
 * as the symbol exactly as written and spells C names with an underscore, so there the label gets it. */
#ifdef __APPLE__
#define P4_ASM_LABEL(name) __asm__("_" name)
#else
#define P4_ASM_LABEL(name) __asm__(name)
#endif

#ifdef __APPLE__
/* macOS: the path of the library `name` that ships with the program, into out[capacity]: the app bundle's Contents/Frameworks, else the
 * executable's own directory (a build tree). 0 when the file is there, -1 otherwise. */
int p4_os_runtime_library(const char *name, char *out, size_t capacity);
#endif

/* Monotonic clock, nanoseconds. */
uint64_t p4_os_now_ns(void);
/* Current thread's real CPU consumption, not wall/virtual time. Returns 0 on
 * success, -1 when unavailable. Windows may report scheduler-tick resolution;
 * interpreter budgets also bound VM checkpoints rather than trusting precision. */
int p4_os_thread_cpu_ns(uint64_t *out);
/* CPU-clock quantization bound in nanoseconds, not nominal units. Returns
 * 0 on success, -1 when unavailable. Budget accounting must not assign a
 * scheduler tick that crossed a native-call boundary entirely to the VM. */
int p4_os_thread_cpu_resolution_ns(uint64_t *out);
/* Whether the run's time is virtual (P4PC_VIRTUAL_CLOCK=1, or P4PC_TURBO=1 or 2): the scheduler's clock advances by the game's own
 * frames and waits (kernel_thread.c), and so must everything else the game can observe the passing of time through: the mixer is
 * rendered one frame's worth of samples per flip (p4snd_virtual_tick) instead of by a real-time device or pacer. */
int p4_os_virtual_clock(void);

/* The game's float state is the EE mode (MXCSR round toward zero, flush, denormals-are-zero; x87 control word 0x037F, hostmath.h). Code that calls into
 * a driver or system library re-checks it afterwards: p4pc_fpu_check restores the mode when a call changed it (and says so, the first few times);
 * p4pc_fpu_state_log prints the state when P4PC_FPU_LOG is set. src/platform/state_dump.c. */
void p4pc_fpu_check(const char *where);
void p4pc_fpu_state_log(const char *where);
/* Sleeps at least ns nanoseconds (the thread, not busy-waiting). */
void p4_os_sleep_ns(uint64_t ns);

/* Makes [addr, addr + size) readable, writable and executable, zero-filled, at exactly addr.
 * Returns 0, or -1 with *why set to a static message when the range cannot be had.
 * Linux: mmap with MAP_FIXED_NOREPLACE. Windows: the exe image already owns the PS2 RAM range
 * (the `.ps2ram` section, src/platform/os/win32_ps2ram.S), which is verified and unprotected;
 * any other range is VirtualAlloc'd and must be free. */
int p4_os_map_fixed(uint32_t addr, size_t size, const char **why);

/* Reserves read/write, zero-filled memory of `size` bytes at an address the OS picks, or returns NULL. The 64-bit
 * build's guest block (src/platform/guest.c, include/p4pc/guest.h): guest memory has no fixed host address there. */
void *p4_os_map_anywhere(size_t size);

/* Reserves `reserve` bytes of address space at an address the OS picks and makes the first `commit` bytes readable and writable;
 * the rest stays inaccessible (a stray access faults). NULL on failure. The guest block uses it so that a wild guest address
 * (base + any 32-bit value) never lands in another mapping (include/p4pc/guest.h). */
void *p4_os_reserve_anywhere(size_t reserve, size_t commit);
/* The same, at an address that is a multiple of `align` (a power of two), or NULL when no such address range is free. */
void *p4_os_reserve_aligned(size_t reserve, size_t commit, size_t align);

/* The address range [*lo, *hi) of the running executable's image (code, read-only data, data, bss), for the 64-bit
 * guest window (include/p4pc/guest.h). Returns 0, or -1 when the OS offers no way to ask. */
int p4_os_image_range(uintptr_t *lo, uintptr_t *hi);
/* Adds guest address / stack-guard context to the native crash report. */
extern void (*p4_os_fault_note)(uintptr_t addr);

/* Coroutines: the thread model's host threads (src/platform/kernel_thread.c). One OS thread runs
 * them all, switching explicitly.
 *   p4_coro_adopt   the calling context as a coroutine (the boot thread), once per process
 *   p4_coro_create  a new coroutine that runs entry() on first switch; entry must never return.
 *                   stack/stack_size is the guest's stack memory: Linux runs on it, Windows
 *                   allocates its own stack of at least stack_size bytes.
 *   p4_coro_switch  saves the running coroutine as `from` and resumes `to`
 *   p4_coro_destroy frees a coroutine that is not running; NULL is allowed */
typedef struct p4_coro p4_coro;
p4_coro *p4_coro_adopt(void);
p4_coro *p4_coro_create(void *stack, size_t stack_size, void (*entry)(void));
void p4_coro_switch(p4_coro *from, p4_coro *to);
void p4_coro_destroy(p4_coro *c);

/* Sampling profiler (src/platform/profile.c). p4_os_profile_start arranges for cb(pc, sp, fp) to run about
 * hz times a second with the registers of the calling thread: Linux delivers SIGPROF on CPU time (the
 * callback runs in the signal handler, on that thread); Windows runs a sampler thread that suspends the
 * calling thread, reads its context and resumes it (wall-clock time, so waits show up as the wait
 * function). cb must only read memory, and only [sp, stack_end): the end of the stack memory known to be readable. Returns 0, or -1. p4_os_list_modules calls cb(base, size, name) for
 * every module mapped in the process (the exe, DLLs, shared objects), so a report can name pcs outside
 * the exe. */
int p4_os_profile_start(int hz, void (*cb)(uintptr_t pc, uintptr_t sp, uintptr_t fp, uintptr_t stack_end));
/* 1 when [addr - 8, addr] is executable memory the process can read (the profiler checks return addresses; async-signal-safe). */
int p4_os_code_readable(uintptr_t addr);
void p4_os_list_modules(void (*cb)(uintptr_t base, uintptr_t size, const char *name));

/* Makes the whole pages inside [addr, addr + size) inaccessible (access 0) or read/write again (access 1); the heap
 * checker's use-after-free trap (P4PC_HEAP_CHECK=3). Returns 0, or -1 on failure. */
int p4_os_protect_pages(void *addr, size_t size, int access);

/* Normalize a mutable host path for filesystem calls. Windows uses absolute extended-length
 * syntax (including UNC); POSIX leaves it unchanged. Returns 0, or -1 with errno set. */
int p4_os_extended_path(char *path, size_t capacity);
/* Resolve an EXISTING file/directory through aliases to its absolute native
 * path (including final links), for custody/lease ancestry. Not read authority.
 * out may equal path.0/-1; missing is ENOENT, never guessed success. */
int p4_os_resolved_path(const char *path,char *out,size_t capacity);
/* Creates one directory (mode 0777 on Linux). Returns 0, or -1 with errno set. */
int p4_os_mkdir(const char *path);
/* Nonblocking exclusive directory ownership against other processes, without
 * creating a lockfile/changing directory contents. Caller serializes logical
 * transactions (Windows ownership is per OS thread). Native token[2] starts
 * zero; same-thread unlock consumes/resets it.0/-1, EBUSY for contention. */
int p4_os_directory_lock(const char *path, uintptr_t token[2]);
void p4_os_directory_unlock(uintptr_t token[2]);
/* Removes an empty directory, without following links. Returns 0, or -1 with errno set. */
int p4_os_rmdir(const char *path);
/* Renames a file or directory. Returns 0, or -1 with errno set. Windows retries
 * transient access/sharing denials for at most 2040 ms and reports the retry. */
int p4_os_rename(const char *from, const char *to);
/* Atomically replaces a file with another file on the same filesystem.
 * Returns 0, or -1 with errno set; never removes the destination first. */
int p4_os_replace_file(const char *from, const char *to);
/* Synchronously visits raw entry names (including "." and ".." where supplied).
 * A name is valid only during its callback. Supports extended-length Windows
 * roots without MinGW dirent's MAX_PATH cap. Returns 0, or -1 with errno set. */
int p4_os_list_dir(const char *path, void (*visit)(const char *name, void *user), void *user);
/* Sleeps for roughly us microseconds. */
void p4_os_usleep(unsigned us);

/* Opens a read-only file the way the port names its inputs: a plain path, or `fd:N` for an already open descriptor (Android's document
 * picker hands the app a descriptor, not a path; the platform glue sets P4PC_ISO and P4PC_ELF to `fd:N`). The descriptor is duplicated, so
 * the caller's stays open; the FILE owns the duplicate. Returns NULL on failure with errno set. */
FILE *p4_os_fopen_rb(const char *path);

#endif
