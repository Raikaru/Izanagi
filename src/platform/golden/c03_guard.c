/* Golden C03: keep the retail guest address and protect its entire guest RAM
 * page. The game and admitted writers use distinct views of shared backing.
 * An unknown page writer taints before the canonical view is opened. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include "p4pc/c03_guard.h"
#include "p4pc/os.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <signal.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#ifdef __linux__
#include <ucontext.h>
#endif
#include <unistd.h>
#endif

#if __SIZEOF_POINTER__ == 8 && (defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__)))
#define C03_GUEST_PAGE 0x00745000u
#define C03_ANCHOR_IN_PAGE 0x8c0u
#define C03_ANCHOR_SIZE 0x228u
#define C03_PAGE_SIZE 4096u
static unsigned char *game_view;
static unsigned char *write_view;
static unsigned char *arena_base, *alias_base;
static int alias_released;
static size_t arena_reserve, arena_commit;
#ifdef _WIN32
static void *exception_handler;
#else
static int signal_installed;
#endif
static _Atomic int tainted;
static _Atomic uintptr_t origin, fault_address;
static atomic_flag fault_lock = ATOMIC_FLAG_INIT;
static int armed, game_open;
_Static_assert(ATOMIC_INT_LOCK_FREE == 2 && ATOMIC_POINTER_LOCK_FREE == 2,
               "C03 fault state must be lock-free");

#ifdef _WIN32
#ifndef MEM_RESERVE_PLACEHOLDER
#define MEM_RESERVE_PLACEHOLDER 0x00040000u
#endif
#ifndef MEM_REPLACE_PLACEHOLDER
#define MEM_REPLACE_PLACEHOLDER 0x00004000u
#endif
#ifndef MEM_PRESERVE_PLACEHOLDER
#define MEM_PRESERVE_PLACEHOLDER 0x00000002u
#endif
typedef PVOID (WINAPI *Alloc2Fn)(HANDLE, PVOID, SIZE_T, ULONG, ULONG, void *, ULONG);
typedef PVOID (WINAPI *Map3Fn)(HANDLE, HANDLE, PVOID, ULONG64, SIZE_T, ULONG, ULONG, void *, ULONG);

void *p4_c03_guard_reserve(size_t reserve, size_t commit)
{
    SYSTEM_INFO si; GetSystemInfo(&si);
    if (game_view || si.dwPageSize != C03_PAGE_SIZE || !si.dwAllocationGranularity ||
        C03_GUEST_PAGE % si.dwAllocationGranularity + C03_PAGE_SIZE > si.dwAllocationGranularity ||
        commit <= C03_GUEST_PAGE + C03_PAGE_SIZE || reserve <= commit || commit > UINT32_MAX)
        return NULL;
    HMODULE kernel = GetModuleHandleW(L"kernelbase.dll");
    HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll");
    Alloc2Fn alloc2 = kernel ? (Alloc2Fn)(void *)GetProcAddress(kernel, "VirtualAlloc2") : NULL;
    Map3Fn map3 = kernel ? (Map3Fn)(void *)GetProcAddress(kernel, "MapViewOfFile3") : NULL;
    if (!alloc2 && kernel32) alloc2 = (Alloc2Fn)(void *)GetProcAddress(kernel32, "VirtualAlloc2");
    if (!map3 && kernel32) map3 = (Map3Fn)(void *)GetProcAddress(kernel32, "MapViewOfFile3");
    if (!alloc2 || !map3) return NULL;
    /* Match the ordinary guest allocator's preferred 4-GiB alignment so guest
     * allocator words retain the same low bits when the address space permits. */
    unsigned char *base = NULL;
    const uintptr_t alignment = UINT64_C(0x100000000);
    if (reserve <= SIZE_MAX - alignment) {
        for (int attempt = 0; attempt < 16 && !base; ++attempt) {
            void *probe = alloc2(NULL, NULL, reserve + alignment,
                                 MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
            if (!probe) break;
            uintptr_t address = ((uintptr_t)probe + alignment - 1) & ~(alignment - 1);
            if (!VirtualFree(probe, 0, MEM_RELEASE)) return NULL;
            base = alloc2(NULL, (void *)address, reserve, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER,
                          PAGE_NOACCESS, NULL, 0);
        }
    }
    if (!base)
        base = alloc2(NULL, NULL, reserve, MEM_RESERVE | MEM_RESERVE_PLACEHOLDER, PAGE_NOACCESS, NULL, 0);
    if (!base) return NULL;
    int split = 0;
    HANDLE section = NULL;
    unsigned char *primary = NULL, *alias = NULL;
    if (!VirtualFree(base, commit, MEM_RELEASE | MEM_PRESERVE_PLACEHOLDER)) goto fail;
    split = 1;
    section = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, (DWORD)commit, NULL);
    if (!section) goto fail;
    primary = map3(section, NULL, base, 0, commit, MEM_REPLACE_PLACEHOLDER, PAGE_READWRITE, NULL, 0);
    if (primary != base) goto fail;
    DWORD alias_offset = C03_GUEST_PAGE - C03_GUEST_PAGE % si.dwAllocationGranularity;
    alias = MapViewOfFile(section, FILE_MAP_READ | FILE_MAP_WRITE, 0, alias_offset, si.dwAllocationGranularity);
    if (!alias) goto fail;
    CloseHandle(section);
    game_view = primary + C03_GUEST_PAGE;
    write_view = alias + (C03_GUEST_PAGE - alias_offset);
    arena_base = base; arena_reserve = reserve; arena_commit = commit; alias_base = alias;
    return base;
fail:
    if (alias) UnmapViewOfFile(alias);
    if (primary) UnmapViewOfFile(primary);
    if (primary != base) VirtualFree(base, 0, MEM_RELEASE);
    if (section) CloseHandle(section);
    if (split) VirtualFree(base + commit, 0, MEM_RELEASE);
    return NULL;
}
#else
void *p4_c03_guard_reserve(size_t reserve, size_t commit)
{
    long page = sysconf(_SC_PAGESIZE);
    if (game_view || page != C03_PAGE_SIZE || commit <= C03_GUEST_PAGE + C03_PAGE_SIZE || reserve <= commit)
        return NULL;
    unsigned char *base = p4_os_reserve_aligned(reserve, commit, 0x100000000ull);
    if (!base) base = p4_os_reserve_anywhere(reserve, commit);
    if (!base) return NULL;
    int fd = (int)syscall(SYS_memfd_create, "p4pc-c03", 1u);
    if (fd < 0 || ftruncate(fd, C03_PAGE_SIZE)) {
        if (fd >= 0) close(fd);
        munmap(base, reserve); return NULL;
    }
    unsigned char *alias = mmap(NULL, C03_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (alias == MAP_FAILED) { close(fd); munmap(base, reserve); return NULL; }
    unsigned char *primary = mmap(base + C03_GUEST_PAGE, C03_PAGE_SIZE, PROT_READ | PROT_WRITE,
                                  MAP_SHARED | MAP_FIXED, fd, 0);
    close(fd);
    if (primary != base + C03_GUEST_PAGE) {
        munmap(alias, C03_PAGE_SIZE); munmap(base, reserve); return NULL;
    }
    game_view = primary; write_view = alias; alias_base = alias;
    arena_base = base; arena_reserve = reserve; arena_commit = commit;
    return base;
}
#endif

static void fatal_fault(void)
{
#ifdef _WIN32
    TerminateProcess(GetCurrentProcess(), 120);
#else
    _exit(120);
#endif
}
static void handle_guard_fault(uintptr_t address, uintptr_t pc)
{
    atomic_store_explicit(&tainted, 1, memory_order_seq_cst);
    while (atomic_flag_test_and_set_explicit(&fault_lock, memory_order_acquire)) { }
    if (!atomic_load_explicit(&origin, memory_order_relaxed)) {
        atomic_store_explicit(&fault_address, address, memory_order_relaxed);
        atomic_store_explicit(&origin, pc, memory_order_release);
    }
    if (!game_open) {
#ifdef _WIN32
        DWORD old;
        if (!VirtualProtect(game_view, C03_PAGE_SIZE, PAGE_READWRITE, &old)) fatal_fault();
#else
        if (syscall(SYS_mprotect, game_view, C03_PAGE_SIZE, PROT_READ | PROT_WRITE)) fatal_fault();
#endif
        game_open = 1;
    }
    atomic_flag_clear_explicit(&fault_lock, memory_order_release);
}
#ifdef _WIN32
static LONG WINAPI guard_exception(EXCEPTION_POINTERS *ep)
{
    EXCEPTION_RECORD *er = ep->ExceptionRecord;
    if (er->ExceptionCode != EXCEPTION_ACCESS_VIOLATION || er->NumberParameters < 2 ||
        er->ExceptionInformation[0] != 1 ||
        (uintptr_t)er->ExceptionInformation[1] - (uintptr_t)game_view >= C03_PAGE_SIZE)
        return EXCEPTION_CONTINUE_SEARCH;
    handle_guard_fault((uintptr_t)er->ExceptionInformation[1], (uintptr_t)er->ExceptionAddress);
    return EXCEPTION_CONTINUE_EXECUTION;
}
#else
static struct sigaction previous_segv;
static void guard_signal(int sig, siginfo_t *info, void *context)
{
    ucontext_t *uc = context;
    uintptr_t address = (uintptr_t)info->si_addr, pc;
    int write;
#if defined(__x86_64__)
    _Static_assert(sizeof uc->uc_mcontext.gregs / sizeof uc->uc_mcontext.gregs[0] == 23,
                   "Linux x86-64 ucontext register ABI");
    write = (uc->uc_mcontext.gregs[19] & 2) != 0;
    pc = (uintptr_t)uc->uc_mcontext.gregs[16];
#elif defined(__aarch64__)
    /* The game view is mapped read-only, so only a store faults; the AArch64 siginfo carries no separate write bit. */
    write = 1;
    pc = (uintptr_t)uc->uc_mcontext.pc;
#else
#error "c03_guard.c: unsupported Linux CPU for the guard signal handler"
#endif
    if (write && address - (uintptr_t)game_view < C03_PAGE_SIZE) {
        handle_guard_fault(address, pc);
        return;
    }
    if (previous_segv.sa_flags & SA_SIGINFO) {
        if (previous_segv.sa_sigaction) previous_segv.sa_sigaction(sig, info, context);
    } else if (previous_segv.sa_handler == SIG_IGN) return;
    else if (previous_segv.sa_handler != SIG_DFL && previous_segv.sa_handler)
        previous_segv.sa_handler(sig);
    else sigaction(sig, &previous_segv, NULL);
}
#endif
int p4_c03_guard_start(void)
{
    if (!game_view || !write_view || armed) return -1;
#ifdef _WIN32
    DWORD old;
    exception_handler = AddVectoredExceptionHandler(1, guard_exception);
    if (!exception_handler || !VirtualProtect(game_view, C03_PAGE_SIZE, PAGE_READONLY, &old)) return -1;
#else
    struct sigaction action = {0};
    sigemptyset(&action.sa_mask); action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    action.sa_sigaction = guard_signal;
    if (sigaction(SIGSEGV, &action, &previous_segv)) return -1;
    signal_installed = 1;
    if (mprotect(game_view, C03_PAGE_SIZE, PROT_READ)) return -1;
#endif
    armed = 1;
    return 0;
}
int p4_c03_guard_active(void) { return armed; }
void p4_c03_guard_store(size_t offset, const void *data, size_t size)
{
    if (!armed || !data || offset > C03_ANCHOR_SIZE || size > C03_ANCHOR_SIZE - offset) abort();
    memcpy(write_view + C03_ANCHOR_IN_PAGE + offset, data, size);
}
int p4_c03_guard_tainted(void) { return atomic_load_explicit(&tainted, memory_order_seq_cst); }
uintptr_t p4_c03_guard_origin(void) { return atomic_load_explicit(&origin, memory_order_acquire); }
uintptr_t p4_c03_guard_address(void) { return atomic_load_explicit(&fault_address, memory_order_relaxed); }
void p4_c03_guard_shutdown(void)
{
    unsigned char stack_marker;
    int on_guest_stack = arena_base &&
        (uintptr_t)&stack_marker - (uintptr_t)arena_base < arena_reserve;
#ifdef _WIN32
    if (exception_handler) RemoveVectoredExceptionHandler(exception_handler);
    if (alias_base) alias_released = UnmapViewOfFile(alias_base) != 0;
    if (arena_base && !on_guest_stack) {
        UnmapViewOfFile(arena_base);
        VirtualFree(arena_base + arena_commit, 0, MEM_RELEASE);
        arena_base = NULL;
    }
    exception_handler = NULL;
#else
    if (signal_installed) sigaction(SIGSEGV, &previous_segv, NULL);
    if (alias_base) alias_released = munmap(alias_base, C03_PAGE_SIZE) == 0;
    if (arena_base && !on_guest_stack) {
        munmap(arena_base, arena_reserve);
        arena_base = NULL;
    }
    signal_installed = 0;
#endif
    game_view = write_view = alias_base = NULL;
    armed = game_open = 0;
}
int p4_c03_guard_alias_released(void) { return alias_released; }
#else
void *p4_c03_guard_reserve(size_t reserve, size_t commit) { (void)reserve; (void)commit; return NULL; }
int p4_c03_guard_start(void) { return -1; }
int p4_c03_guard_active(void) { return 0; }
void p4_c03_guard_store(size_t offset, const void *data, size_t size) { (void)offset; (void)data; (void)size; abort(); }
int p4_c03_guard_tainted(void) { return 1; }
uintptr_t p4_c03_guard_origin(void) { return 0; }
uintptr_t p4_c03_guard_address(void) { return 0; }
void p4_c03_guard_shutdown(void) {}
int p4_c03_guard_alias_released(void) { return 1; }
#endif

static void report_c03_session(void)
{
    uintptr_t pc = p4_c03_guard_origin();
    if (p4_c03_guard_tainted())
        fprintf(stderr, "p4pc: Golden C03 tainted: writer pc=%p address=%p; saves refused\n",
                (void *)pc, (void *)p4_c03_guard_address());
    else
        fprintf(stderr, "p4pc: Golden C03 session clean\n");
}
int p4_c03_guard_install_cleanup(void) { return atexit(p4_c03_guard_shutdown); }
int p4_c03_guard_install_report(void) { return atexit(report_c03_session); }
