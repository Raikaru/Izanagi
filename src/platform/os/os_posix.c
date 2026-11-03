/* POSIX (Linux, Android, macOS) implementation of include/p4pc/os.h. */
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#ifdef __APPLE__
#include <sys/ucontext.h> /* (<ucontext.h> there is the deprecated makecontext API, refused without _XOPEN_SOURCE) */
#else
#include <ucontext.h>
#endif
#ifdef __ANDROID__
#include <dlfcn.h>
#include <android/log.h>
#include <unwind.h>
#else
#include <execinfo.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#endif

#include "p4pc/os.h"

#ifndef MAP_FIXED_NOREPLACE
#define MAP_FIXED_NOREPLACE 0x100000 /* Linux 4.17; the NDK headers of older API levels lack it, the kernel of every supported device has it */
#endif

void (*p4_os_fault_note)(uintptr_t addr);
static int crash_fd = -1;
static volatile sig_atomic_t crashing;
static _Thread_local unsigned char crash_stack[64 * 1024];

static void crash_address(const char *kind, unsigned index, uintptr_t pc)
{
#ifdef __ANDROID__
    Dl_info module;
    if (dladdr((void *)pc, &module) && module.dli_fbase)
        dprintf(STDERR_FILENO, "p4pc:   %s %u: %p (%s+0x%llx)\n", kind, index, (void *)pc,
                module.dli_fname, (unsigned long long)(pc - (uintptr_t)module.dli_fbase));
    else
        dprintf(STDERR_FILENO, "p4pc:   %s %u: %p\n", kind, index, (void *)pc);
#else
    void *address = (void *)pc;
    dprintf(STDERR_FILENO, "p4pc:   %s %u: ", kind, index);
    backtrace_symbols_fd(&address, 1, STDERR_FILENO);
#endif
}

#ifdef __ANDROID__
typedef struct { unsigned depth; } CrashUnwind;
static _Unwind_Reason_Code crash_unwind(struct _Unwind_Context *ctx, void *arg)
{
    CrashUnwind *trace = arg;
    crash_address("frame", trace->depth++, (uintptr_t)_Unwind_GetIP(ctx));
    return trace->depth < 48 ? _URC_NO_REASON : _URC_END_OF_STACK;
}
#endif

static void crash_handler(int sig, siginfo_t *info, void *context)
{
    static const char prefix[] = "p4pc: fatal signal; writing native crash report\n";
    if (crashing) _exit(128 + sig);
    crashing = 1;
    /* Publish a minimal record before best-effort libc/unwinder diagnostics. Neither
     * backtrace nor the guest callback is guaranteed async-signal-safe after heap damage. */
    if (crash_fd >= 0) {
        ftruncate(crash_fd, 0);
        lseek(crash_fd, 0, SEEK_SET);
        write(crash_fd, prefix, sizeof prefix - 1);
        dup2(crash_fd, STDERR_FILENO);
    } else {
        write(STDERR_FILENO, prefix, sizeof prefix - 1);
    }
    dprintf(STDERR_FILENO, "p4pc: CRASH signal %d code %d, fault address %p\n", sig, info->si_code, info->si_addr);
    if (p4_os_fault_note) p4_os_fault_note((uintptr_t)info->si_addr);
    ucontext_t *uc = context;
#if defined(__linux__) && defined(__x86_64__)
    /* Linux sys/ucontext.h gregset ABI. The REG_* enum is GNU-only; do not
     * enable GNU features for unrelated port units just to name these slots. */
    static const char *const names[] = {"r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "rdi", "rsi", "rbp", "rbx", "rdx", "rax", "rcx", "rsp", "rip", "eflags", "csgsfs",
        "err", "trapno", "oldmask", "cr2"};
    _Static_assert(sizeof uc->uc_mcontext.gregs / sizeof uc->uc_mcontext.gregs[0] ==
                   sizeof names / sizeof names[0], "Linux x86_64 ucontext register ABI");
    crash_address("instruction", 0, (uintptr_t)uc->uc_mcontext.gregs[16]);
    for (unsigned i = 0; i < sizeof names / sizeof names[0]; ++i)
        dprintf(STDERR_FILENO, "p4pc:   %s=%016llx\n", names[i], (unsigned long long)uc->uc_mcontext.gregs[i]);
#elif defined(__linux__) && defined(__i386__)
    static const char *const names[] = {"gs", "fs", "es", "ds", "edi", "esi", "ebp", "esp",
        "ebx", "edx", "ecx", "eax", "trapno", "err", "eip", "cs", "eflags", "uesp", "ss"};
    _Static_assert(sizeof uc->uc_mcontext.gregs / sizeof uc->uc_mcontext.gregs[0] ==
                   sizeof names / sizeof names[0], "Linux i386 ucontext register ABI");
    crash_address("instruction", 0, (uintptr_t)uc->uc_mcontext.gregs[14]);
    for (unsigned i = 0; i < sizeof names / sizeof names[0]; ++i)
        dprintf(STDERR_FILENO, "p4pc:   %s=%08llx\n", names[i], (unsigned long long)uc->uc_mcontext.gregs[i]);
#elif defined(__linux__) && defined(__aarch64__)
    crash_address("instruction", 0, (uintptr_t)uc->uc_mcontext.pc);
    for (unsigned i = 0; i < 31; ++i)
        dprintf(STDERR_FILENO, "p4pc:   x%u=%016llx\n", i, (unsigned long long)uc->uc_mcontext.regs[i]);
    dprintf(STDERR_FILENO, "p4pc:   sp=%016llx pstate=%016llx\n",
            (unsigned long long)uc->uc_mcontext.sp, (unsigned long long)uc->uc_mcontext.pstate);
#elif defined(__APPLE__) && defined(__x86_64__)
    {
        const struct __darwin_x86_thread_state64 *ss = &uc->uc_mcontext->__ss;
        const struct { const char *name; uint64_t value; } regs[] = {
            {"rax", ss->__rax}, {"rbx", ss->__rbx}, {"rcx", ss->__rcx}, {"rdx", ss->__rdx}, {"rdi", ss->__rdi}, {"rsi", ss->__rsi},
            {"rbp", ss->__rbp}, {"rsp", ss->__rsp}, {"r8", ss->__r8}, {"r9", ss->__r9}, {"r10", ss->__r10}, {"r11", ss->__r11},
            {"r12", ss->__r12}, {"r13", ss->__r13}, {"r14", ss->__r14}, {"r15", ss->__r15}, {"rip", ss->__rip}, {"rflags", ss->__rflags}};
        crash_address("instruction", 0, (uintptr_t)ss->__rip);
        for (unsigned i = 0; i < sizeof regs / sizeof regs[0]; ++i)
            dprintf(STDERR_FILENO, "p4pc:   %s=%016llx\n", regs[i].name, (unsigned long long)regs[i].value);
    }
#elif defined(__APPLE__) && defined(__aarch64__)
    {
        const struct __darwin_arm_thread_state64 *ss = &uc->uc_mcontext->__ss;
        crash_address("instruction", 0, (uintptr_t)__darwin_arm_thread_state64_get_pc(*ss));
        for (unsigned i = 0; i < 29; ++i)
            dprintf(STDERR_FILENO, "p4pc:   x%u=%016llx\n", i, (unsigned long long)ss->__x[i]);
        dprintf(STDERR_FILENO, "p4pc:   fp=%016llx lr=%016llx sp=%016llx cpsr=%08x\n",
                (unsigned long long)__darwin_arm_thread_state64_get_fp(*ss), (unsigned long long)__darwin_arm_thread_state64_get_lr(*ss),
                (unsigned long long)__darwin_arm_thread_state64_get_sp(*ss), (unsigned)ss->__cpsr);
    }
#else
    (void)uc;
#endif
#ifdef __ANDROID__
    CrashUnwind trace = {0};
    _Unwind_Backtrace(crash_unwind, &trace);
#else
    void *frames[48];
    int count = backtrace(frames, 48);
    for (int i = 0; i < count; ++i) crash_address("frame", (unsigned)i, (uintptr_t)frames[i]);
#endif
    fflush(stderr); /* the existing guest fault note uses stderr */
    if (crash_fd >= 0) fsync(crash_fd);
#ifdef __ANDROID__
    /* Do not rely on the game's asynchronous stdout/logcat pipe draining on death. */
    if (crash_fd >= 0) {
        char line[1024];
        ssize_t n;
        lseek(crash_fd, 0, SEEK_SET);
        while ((n = read(crash_fd, line, sizeof line - 1)) > 0) {
            line[n] = 0;
            __android_log_write(ANDROID_LOG_FATAL, "p4pc", line);
        }
    } else {
        __android_log_write(ANDROID_LOG_FATAL, "p4pc", "Fatal native signal; crash log could not be opened");
    }
#endif
    _exit(128 + sig);
}

void p4_os_init(void)
{
    static const int signals[] = {SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT};
    const char *log = getenv("P4PC_LOG_FILE");
    if (crash_fd < 0 && log && *log) {
        char path[4096];
        int n = snprintf(path, sizeof path, "%s", log);
        char *slash = strrchr(path, '/');
        if (n > 0 && (size_t)n < sizeof path && slash &&
            (size_t)(slash - path) + sizeof "/p4pc-crash.log" <= sizeof path) {
            strcpy(slash + 1, "p4pc-crash.log");
            crash_fd = open(path, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
        }
    }
    stack_t stack = {.ss_sp = crash_stack, .ss_size = sizeof crash_stack};
    sigaltstack(&stack, NULL);
    struct sigaction action = {.sa_sigaction = crash_handler, .sa_flags = SA_SIGINFO | SA_ONSTACK};
    sigfillset(&action.sa_mask);
    for (unsigned i = 0; i < sizeof signals / sizeof signals[0]; ++i)
        sigaction(signals[i], &action, NULL);
#ifndef __ANDROID__
    /* Resolve the unwinder before a fault, when allocating is still safe. */
    void *frame;
    backtrace(&frame, 1);
#endif
}

#ifndef __ANDROID__
int p4_os_restart_self(int argc, char **argv)
{
    (void)argc;
#ifdef __APPLE__
    char executable[PATH_MAX];
    uint32_t size = sizeof executable;
    if (_NSGetExecutablePath(executable, &size) != 0) { errno = ENAMETOOLONG; return -1; }
    execv(executable, argv);
#else
    /* The executable inode, even if argv[0] is relative or PATH/cwd changed. */
    execv("/proc/self/exe", argv);
#endif
    return -1;
}
#endif

#ifdef __APPLE__
int p4_os_runtime_library(const char *name, char *out, size_t capacity) {
    static const char *const places[] = {"/../Frameworks/", "/"};   /* the app bundle's, then a build tree's (beside the executable) */
    char executable[PATH_MAX], dir[PATH_MAX];
    uint32_t size = sizeof executable;
    char *slash;

    if (_NSGetExecutablePath(executable, &size) != 0 || realpath(executable, dir) == NULL || (slash = strrchr(dir, '/')) == NULL) return -1;
    *slash = '\0';
    for (size_t i = 0; i < sizeof places / sizeof places[0]; i++) {
        int n = snprintf(out, capacity, "%s%s%s", dir, places[i], name);
        if (n > 0 && (size_t)n < capacity && access(out, R_OK) == 0) return 0;
    }
    return -1;
}
#endif

int p4_os_setenv(const char *name, const char *value, int overwrite) { return setenv(name, value, overwrite); }

#ifdef __APPLE__
/* macOS: SDL's offscreen driver has only EGL device displays, which ANGLE's Metal back end does not offer; a hidden Cocoa window gives the
 * context instead (as on Windows: src/platform/pad/pad_sdl.c, P4PC_HIDDEN_WINDOW). */
void p4_os_headless_env(void) { setenv("P4PC_HIDDEN_WINDOW", "1", 0); }
#else
void p4_os_headless_env(void) { setenv("SDL_VIDEODRIVER", "offscreen", 0); }
#endif

int p4_os_is_dir_nofollow(const char *path) {
    struct stat st;
    return lstat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

int p4_os_is_file_nofollow(const char *path) {
    struct stat st;
    return lstat(path, &st) == 0 && S_ISREG(st.st_mode);
}

int p4_os_file_identity(FILE *stream, P4OSFileIdentity *out) {
    struct stat st;
    if (!stream || !out || fstat(fileno(stream), &st) || !S_ISREG(st.st_mode) || st.st_size < 0) return -1;
    *out = (P4OSFileIdentity){0};
    out->size = (uint64_t)st.st_size;
    out->tokens[0] = (uint64_t)st.st_dev;
    out->tokens[1] = (uint64_t)st.st_ino;
    #ifdef __APPLE__
    out->tokens[2] = (uint64_t)st.st_mtimespec.tv_sec;
    out->tokens[3] = (uint64_t)st.st_mtimespec.tv_nsec;
    out->tokens[4] = (uint64_t)st.st_ctimespec.tv_sec;
    out->tokens[5] = (uint64_t)st.st_ctimespec.tv_nsec;
    #else
    out->tokens[2] = (uint64_t)st.st_mtim.tv_sec;
    out->tokens[3] = (uint64_t)st.st_mtim.tv_nsec;
    out->tokens[4] = (uint64_t)st.st_ctim.tv_sec;
    out->tokens[5] = (uint64_t)st.st_ctim.tv_nsec;
    #endif
    return 0;
}

uint64_t p4_os_now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

int p4_os_thread_cpu_ns(uint64_t *out) {
    struct timespec ts;
    if (!out || clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) return -1;
    *out = (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    return 0;
}

int p4_os_thread_cpu_resolution_ns(uint64_t *out) {
    struct timespec ts;
    if (!out || clock_getres(CLOCK_THREAD_CPUTIME_ID, &ts) != 0) return -1;
    uint64_t resolution = (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
    if (resolution == 0) return -1;
    *out = resolution;
    return 0;
}

int p4_os_virtual_clock(void) {
    static int v = -1;

    if (v < 0) {
        const char *e = getenv("P4PC_VIRTUAL_CLOCK");
        const char *t = getenv("P4PC_TURBO");

        v = (e != NULL && e[0] == '1') || (t != NULL && (t[0] == '1' || t[0] == '2'));
    }
    return v;
}

void p4_os_sleep_ns(uint64_t ns) {
    struct timespec ts;
    ts.tv_sec = (time_t)(ns / 1000000000ull);
    ts.tv_nsec = (long)(ns % 1000000000ull);
    nanosleep(&ts, NULL);
}

void p4_os_usleep(unsigned us) { usleep(us); }

int p4_os_map_fixed(uint32_t addr, size_t size, const char **why) {
    void *m = mmap((void *)(uintptr_t)addr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (m == MAP_FAILED) {
        *why = "mmap(MAP_FIXED_NOREPLACE) failed (address range in use?)";
        return -1;
    }
    return 0;
}

void *p4_os_map_anywhere(size_t size) {
    void *m = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    return m == MAP_FAILED ? NULL : m;
}

void *p4_os_reserve_anywhere(size_t reserve, size_t commit) {
    void *m = mmap(NULL, reserve, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (m == MAP_FAILED) return NULL;
    if (mprotect(m, commit, PROT_READ | PROT_WRITE) != 0) {
        munmap(m, reserve);
        return NULL;
    }
    return m;
}

void *p4_os_reserve_aligned(size_t reserve, size_t commit, size_t align) {
    uint8_t *m = mmap(NULL, reserve + align, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    if (m == MAP_FAILED) return NULL;
    uint8_t *a = (uint8_t *)(((uintptr_t)m + align - 1) & ~(uintptr_t)(align - 1));
    if (a > m) munmap(m, (size_t)(a - m));
    if (a + reserve < m + reserve + align) munmap(a + reserve, (size_t)((m + reserve + align) - (a + reserve)));
    if (mprotect(a, commit, PROT_READ | PROT_WRITE) != 0) {
        munmap(a, reserve);
        return NULL;
    }
    return a;
}

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#include <mach-o/loader.h>

int p4_os_image_range(uintptr_t *lo, uintptr_t *hi) {
    const struct mach_header_64 *mh = (const struct mach_header_64 *)_dyld_get_image_header(0);
    intptr_t slide = _dyld_get_image_vmaddr_slide(0);
    const struct load_command *lc = (const struct load_command *)(mh + 1);
    uintptr_t top = 0;
    for (uint32_t i = 0; i < mh->ncmds; i++, lc = (const struct load_command *)((const char *)lc + lc->cmdsize)) {
        if (lc->cmd != LC_SEGMENT_64) continue;
        const struct segment_command_64 *sc = (const struct segment_command_64 *)lc;
        if (sc->vmsize == 0 || sc->initprot == 0) continue;           /* __PAGEZERO */
        uintptr_t end = (uintptr_t)(sc->vmaddr + sc->vmsize + (uint64_t)slide);
        if (end > top) top = end;
    }
    *lo = (uintptr_t)mh;
    *hi = top;
    return top > *lo ? 0 : -1;
}
#elif __SIZEOF_POINTER__ == 8
extern char __ehdr_start[], _end[];

int p4_os_image_range(uintptr_t *lo, uintptr_t *hi) {
    *lo = (uintptr_t)__ehdr_start;
    *hi = (uintptr_t)_end;
    return 0;
}
#else
/* a 32-bit host has no guest window to give: a guest address is a host address */
int p4_os_image_range(uintptr_t *lo, uintptr_t *hi) {
    *lo = *hi = 0;
    return -1;
}
#endif

int p4_os_protect_pages(void *addr, size_t size, int access) {
    uintptr_t lo = ((uintptr_t)addr + 4095u) & ~(uintptr_t)4095u;
    uintptr_t hi = ((uintptr_t)addr + size) & ~(uintptr_t)4095u;

    if (hi <= lo) {
        return 0;
    }
    return mprotect((void *)lo, hi - lo, access ? PROT_READ | PROT_WRITE : PROT_NONE);
}

int p4_os_extended_path(char *path, size_t capacity) {
    (void)path;
    (void)capacity;
    return 0;
}
int p4_os_resolved_path(const char *path,char *out,size_t capacity) {
    if(!path||!out||!capacity){errno=EINVAL;return -1;}
    char resolved[PATH_MAX];
    if(!realpath(path,resolved))return -1;
    size_t length=strlen(resolved);
    if(length>=capacity){errno=ENAMETOOLONG;return -1;}
    memcpy(out,resolved,length+1);return 0;
}

int p4_os_mkdir(const char *path) { return mkdir(path, 0777); }
int p4_os_rmdir(const char *path) { return rmdir(path); }
int p4_os_rename(const char *from, const char *to) { return rename(from, to); }
int p4_os_replace_file(const char *from, const char *to) { return rename(from, to); }

int p4_os_directory_lock(const char *path, uintptr_t token[2]) {
    if (!path || !token || token[0] || token[1]) { errno = EINVAL; return -1; }
    int fd = open(path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) return -1;
    if (flock(fd, LOCK_EX | LOCK_NB)) {
        int saved = errno;
        close(fd);
        errno = saved == EWOULDBLOCK || saved == EAGAIN ? EBUSY : saved;
        return -1;
    }
    token[0] = (uintptr_t)fd + 1;
    return 0;
}
void p4_os_directory_unlock(uintptr_t token[2]) {
    if (!token) return;
    if (token[0]) close((int)(token[0] - 1));
    token[0] = token[1] = 0;
}

int p4_os_list_dir(const char *path, void (*visit)(const char *name, void *user), void *user) {
    DIR *directory = opendir(path);
    struct dirent *entry;
    if (directory == NULL) return -1;
    for (;;) {
        int error;
        errno = 0;
        entry = readdir(directory);
        if (entry != NULL) {
            visit(entry->d_name, user);
            continue;
        }
        error = errno;
        closedir(directory);
        if (error != 0) {
            errno = error;
            return -1;
        }
        return 0;
    }
}

FILE *p4_os_fopen_rb(const char *path) {
    if (strncmp(path, "fd:", 3) == 0) {
        const int fd = dup(atoi(path + 3));
        FILE *f;

        if (fd < 0) return NULL;
        f = fdopen(fd, "rb");
        if (f == NULL) close(fd);
        return f;
    }
    return fopen(path, "rb");
}

#if defined(__x86_64__)
/* x86_64 System V: the context switch is ours, so a coroutine runs on exactly the stack memory it is given (the guest block's, docs/port64.md
 * 2.7) and nothing depends on ucontext. A suspended coroutine is its stack pointer; the callee-saved registers sit on its stack. The EE
 * rounding mode in MXCSR and the x87 control word are process-wide settings, not per coroutine, so they are not switched. */
struct p4_coro {
    void *sp;
};

void p4_coro_swap(void **save_sp, void *load_sp);
#if defined(__APPLE__)
/* Mach-O: the C name has a leading underscore, and there is no .type/.size */
__asm__(".text\n"
        ".globl _p4_coro_swap\n"
        ".private_extern _p4_coro_swap\n"
        "_p4_coro_swap:\n"
        "\tpushq %rbp\n\tpushq %rbx\n\tpushq %r12\n\tpushq %r13\n\tpushq %r14\n\tpushq %r15\n"
        "\tmovq %rsp, (%rdi)\n"
        "\tmovq %rsi, %rsp\n"
        "\tpopq %r15\n\tpopq %r14\n\tpopq %r13\n\tpopq %r12\n\tpopq %rbx\n\tpopq %rbp\n"
        "\tretq\n");
#else
__asm__(".text\n"
        ".globl p4_coro_swap\n"
        ".hidden p4_coro_swap\n"
        ".type p4_coro_swap,@function\n"
        "p4_coro_swap:\n"
        "\tpushq %rbp\n\tpushq %rbx\n\tpushq %r12\n\tpushq %r13\n\tpushq %r14\n\tpushq %r15\n"
        "\tmovq %rsp, (%rdi)\n"
        "\tmovq %rsi, %rsp\n"
        "\tpopq %r15\n\tpopq %r14\n\tpopq %r13\n\tpopq %r12\n\tpopq %rbx\n\tpopq %rbp\n"
        "\tretq\n"
        ".size p4_coro_swap,.-p4_coro_swap\n");
#endif

p4_coro *p4_coro_adopt(void) { return calloc(1, sizeof(p4_coro)); }

p4_coro *p4_coro_create(void *stack, size_t stack_size, void (*entry)(void)) {
    p4_coro *c = calloc(1, sizeof *c);
    if (c == NULL) return NULL;
    /* First resume pops six zeroed registers and returns into entry with rsp = top - 8 (16-byte aligned plus 8, as at the first
     * instruction of a called function); the word there is a return address that is never used. */
    uintptr_t top = ((uintptr_t)stack + stack_size) & ~(uintptr_t)15;
    uint64_t *sp = (uint64_t *)(top - 8);
    *sp = 0;
    *--sp = (uint64_t)(uintptr_t)entry;
    for (int i = 0; i < 6; i++) *--sp = 0;
    c->sp = sp;
    return c;
}

void p4_coro_switch(p4_coro *from, p4_coro *to) { p4_coro_swap(&from->sp, to->sp); }

void p4_coro_destroy(p4_coro *c) { free(c); }
#elif defined(__aarch64__)
/* AArch64 (Linux, Android, macOS, iOS): the same scheme. A suspended coroutine is its stack pointer; x19..x28, x29, x30 and d8..d15 (the
 * callee-saved set; x18 is the platform register and is never touched) sit in a 176-byte frame at it. */
struct p4_coro {
    void *sp;
};

void p4_coro_swap(void **save_sp, void *load_sp);
#if defined(__APPLE__)
#define P4_SYM(n) "_" #n
#define P4_FUNC_BEGIN(n) ".globl " P4_SYM(n) "\n.private_extern " P4_SYM(n) "\n.p2align 2\n" P4_SYM(n) ":\n"
#define P4_FUNC_END(n) ""
#else
#define P4_SYM(n) #n
#define P4_FUNC_BEGIN(n) ".globl " #n "\n.hidden " #n "\n.type " #n ",%function\n.p2align 2\n" #n ":\n"
#define P4_FUNC_END(n) ".size " #n ",.-" #n "\n"
#endif
__asm__(".text\n"
        P4_FUNC_BEGIN(p4_coro_swap)
        "\tsub sp, sp, #176\n"
        "\tstp x19, x20, [sp, #0]\n\tstp x21, x22, [sp, #16]\n\tstp x23, x24, [sp, #32]\n\tstp x25, x26, [sp, #48]\n"
        "\tstp x27, x28, [sp, #64]\n\tstp x29, x30, [sp, #80]\n"
        "\tstp d8, d9, [sp, #96]\n\tstp d10, d11, [sp, #112]\n\tstp d12, d13, [sp, #128]\n\tstp d14, d15, [sp, #144]\n"
        "\tmov x9, sp\n\tstr x9, [x0]\n"
        "\tmov sp, x1\n"
        "\tldp x19, x20, [sp, #0]\n\tldp x21, x22, [sp, #16]\n\tldp x23, x24, [sp, #32]\n\tldp x25, x26, [sp, #48]\n"
        "\tldp x27, x28, [sp, #64]\n\tldp x29, x30, [sp, #80]\n"
        "\tldp d8, d9, [sp, #96]\n\tldp d10, d11, [sp, #112]\n\tldp d12, d13, [sp, #128]\n\tldp d14, d15, [sp, #144]\n"
        "\tadd sp, sp, #176\n"
        "\tret\n"
        P4_FUNC_END(p4_coro_swap));

p4_coro *p4_coro_adopt(void) { return calloc(1, sizeof(p4_coro)); }

p4_coro *p4_coro_create(void *stack, size_t stack_size, void (*entry)(void)) {
    p4_coro *c = calloc(1, sizeof *c);
    if (c == NULL) return NULL;
    /* the first resume loads a zeroed frame whose x30 is entry, and returns into it with sp = top (16-byte aligned) */
    uintptr_t top = ((uintptr_t)stack + stack_size) & ~(uintptr_t)15;
    uint64_t *frame = (uint64_t *)(top - 176);
    memset(frame, 0, 176);
    frame[11] = (uint64_t)(uintptr_t)entry;       /* x30 at [sp, #88] */
    c->sp = frame;
    return c;
}

void p4_coro_switch(p4_coro *from, p4_coro *to) { p4_coro_swap(&from->sp, to->sp); }

void p4_coro_destroy(p4_coro *c) { free(c); }
#elif defined(__ANDROID__)
/* p4_coro_*: src/platform/os/coro_asm.c, the i386 switch (bionic has no ucontext; the x86_64 and aarch64 ones are above). */
#else
struct p4_coro {
    ucontext_t ctx;
};

p4_coro *p4_coro_adopt(void) { return calloc(1, sizeof(p4_coro)); }

p4_coro *p4_coro_create(void *stack, size_t stack_size, void (*entry)(void)) {
    p4_coro *c = calloc(1, sizeof *c);
    if (c == NULL) return NULL;
    getcontext(&c->ctx);
    c->ctx.uc_stack.ss_sp = stack;
    c->ctx.uc_stack.ss_size = stack_size;
    c->ctx.uc_link = NULL;
    makecontext(&c->ctx, entry, 0);
    return c;
}

void p4_coro_switch(p4_coro *from, p4_coro *to) { swapcontext(&from->ctx, &to->ctx); }

void p4_coro_destroy(p4_coro *c) { free(c); }
#endif

/* ---- sampling profiler ---------------------------------------------------------------------- */

static void (*sProfCb)(uintptr_t pc, uintptr_t sp, uintptr_t fp, uintptr_t stack_end);

static void onProf(int sig, siginfo_t *info, void *uctx) {
    const ucontext_t *uc = uctx;
    (void)sig;
    (void)info;
#if defined(__i386__)
    /* i386 gregs: 6 = EBP, 7 = ESP, 14 = EIP */
    sProfCb((uintptr_t)uc->uc_mcontext.gregs[14], (uintptr_t)uc->uc_mcontext.gregs[7],
            (uintptr_t)uc->uc_mcontext.gregs[6], (uintptr_t)uc->uc_mcontext.gregs[7] + (1u << 16));
#else
    (void)uc;
#endif
}

int p4_os_profile_start(int hz, void (*cb)(uintptr_t pc, uintptr_t sp, uintptr_t fp, uintptr_t stack_end)) {
#if defined(__i386__)
    struct sigaction sa;
    struct itimerval it;
    memset(&sa, 0, sizeof sa);
    sProfCb = cb;
    sa.sa_sigaction = onProf;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGPROF, &sa, NULL);
    it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = 1000000 / hz;
    it.it_value = it.it_interval;
    return setitimer(ITIMER_PROF, &it, NULL);
#else
    (void)hz;
    (void)cb;
    return -1;
#endif
}

int p4_os_code_readable(uintptr_t addr) {
    (void)addr; /* the exe's segments are contiguous; the caller has checked the module range */
    return 1;
}

#if defined(__APPLE__)
/* macOS has no /proc: the executable code of each image dyld has loaded is its __TEXT segment */
void p4_os_list_modules(void (*cb)(uintptr_t base, uintptr_t size, const char *name)) {
    for (uint32_t i = 0, n = _dyld_image_count(); i < n; i++) {
        const struct mach_header_64 *mh = (const struct mach_header_64 *)_dyld_get_image_header(i);
        const char *path = _dyld_get_image_name(i);
        if (mh == NULL || mh->magic != MH_MAGIC_64 || path == NULL) continue;
        const struct load_command *lc = (const struct load_command *)(mh + 1);
        for (uint32_t c = 0; c < mh->ncmds; c++, lc = (const struct load_command *)((const char *)lc + lc->cmdsize)) {
            const struct segment_command_64 *sc = (const struct segment_command_64 *)lc;
            if (lc->cmd == LC_SEGMENT_64 && strcmp(sc->segname, "__TEXT") == 0) {
                cb((uintptr_t)(sc->vmaddr + (uint64_t)_dyld_get_image_vmaddr_slide(i)), (uintptr_t)sc->vmsize, path);
                break;
            }
        }
    }
}
#else
void p4_os_list_modules(void (*cb)(uintptr_t base, uintptr_t size, const char *name)) {
    FILE *f = fopen("/proc/self/maps", "r");
    char line[512];
    if (f == NULL) return;
    while (fgets(line, sizeof line, f)) {
        unsigned long lo, hi;
        char perms[8], path[400];
        path[0] = 0;
        if (sscanf(line, "%lx-%lx %7s %*s %*s %*s %399[^\n]", &lo, &hi, perms, path) >= 3 && perms[2] == 'x' && path[0])
            cb((uintptr_t)lo, (uintptr_t)(hi - lo), path);
    }
    fclose(f);
}
#endif
