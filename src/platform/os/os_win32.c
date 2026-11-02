/* 32-bit Windows implementation of include/p4pc/os.h. */
/* Declare the full-width FileIdInfo query. Downlevel NTFS/FAT use their actual
 * complete64-bit IDs; ReFS is never accepted with a truncated identifier. */
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0602
#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0602
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tlhelp32.h>

#include <direct.h>
#include <errno.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "p4pc/os.h"
#include "p4pc/profile_trace.h"

/* ---- crash log -------------------------------------------------------------------------- */

static const char *exception_name(DWORD code) {
    switch (code) {
    case EXCEPTION_ACCESS_VIOLATION: return "access violation";
    case EXCEPTION_IN_PAGE_ERROR: return "in-page error";
    case EXCEPTION_ILLEGAL_INSTRUCTION: return "illegal instruction";
    case EXCEPTION_PRIV_INSTRUCTION: return "privileged instruction";
    case EXCEPTION_INT_DIVIDE_BY_ZERO: return "integer divide by zero";
    case EXCEPTION_INT_OVERFLOW: return "integer overflow";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: return "array bounds exceeded";
    case EXCEPTION_STACK_OVERFLOW: return "stack overflow";
    case EXCEPTION_DATATYPE_MISALIGNMENT: return "datatype misalignment";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO: return "float divide by zero";
    case EXCEPTION_FLT_INVALID_OPERATION: return "float invalid operation";
    default: return NULL;
    }
}

/* module+offset for an address, so the log can be symbolized against the map or the PDB-less exe. */
static void describe(char *out, size_t cap, uintptr_t addr) {
    HMODULE m = NULL;
    char path[MAX_PATH];
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)addr, &m) &&
        GetModuleFileNameA(m, path, sizeof path)) {
        const char *base = strrchr(path, '\\');
        snprintf(out, cap, "0x%08llx (%s+0x%llx)", (unsigned long long)addr, base ? base + 1 : path,
                 (unsigned long long)(addr - (uintptr_t)m));
    } else {
        snprintf(out, cap, "0x%08llx", (unsigned long long)addr);
    }
}

static int readable(uintptr_t addr, size_t len) {
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery((void *)addr, &mbi, sizeof mbi)) return 0;
    if (mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return 0;
    return addr + len <= (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
}

void (*p4_os_fault_note)(uintptr_t addr);

static volatile LONG crashing;

static LONG WINAPI crash_handler(EXCEPTION_POINTERS *ep) {
    const EXCEPTION_RECORD *er = ep->ExceptionRecord;
    const CONTEXT *c = ep->ContextRecord;
    const char *name = exception_name(er->ExceptionCode);
    char where[160];
    uintptr_t fp, sp;
    int depth;
    if (name == NULL) {
        /* debugger traps, C++ throws, thread names are not ours; an error status we do not name is reported (and left to the system) */
        if ((er->ExceptionCode & 0xC0000000u) == 0xC0000000u) {
            char b[128];
            int n = wsprintfA(b, "p4pc: unhandled exception %08lx at %p\n", (unsigned long)er->ExceptionCode, er->ExceptionAddress);
            DWORD w;
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), b, (DWORD)n, &w, NULL);
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }
    {
        /* first: a line that needs almost no stack, before anything that could fault again (a stack overflow gives this handler a few KB) */
        char b[512];
#if __SIZEOF_POINTER__ == 8
        const void *const *sp = (const void *const *)c->Rsp;
        int n = wsprintfA(b, "p4pc: exception %08lx at %p, rsp %p, info %p %p, stack %p %p %p %p\n", (unsigned long)er->ExceptionCode,
                          er->ExceptionAddress, (void *)c->Rsp, (void *)er->ExceptionInformation[0], (void *)er->ExceptionInformation[1],
                          sp[0], sp[1], sp[2], sp[3]);
#else
        int n = wsprintfA(b, "p4pc: exception %08lx at %p\n", (unsigned long)er->ExceptionCode, er->ExceptionAddress);
#endif
        DWORD w;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), b, (DWORD)n, &w, NULL);
#if __SIZEOF_POINTER__ == 8
        {
            const NT_TIB *tib = (const NT_TIB *)NtCurrentTeb();
            n = wsprintfA(b, "p4pc:   TIB stack base %p limit %p dealloc %p\n", tib->StackBase, tib->StackLimit, *(void *const *)((const char *)tib + 0x1478));
            WriteFile(GetStdHandle(STD_ERROR_HANDLE), b, (DWORD)n, &w, NULL);
        }
        /* the call stack by the unwind tables (the exe has them, as has every system DLL), without the C library: the heap may be what failed */
        {
            extern IMAGE_DOS_HEADER __ImageBase;
            CONTEXT ctx = *c;
            for (int i = 0; i < 40 && ctx.Rip; i++) {
                DWORD64 base = 0, frame = 0;
                PVOID hd = NULL;
                RUNTIME_FUNCTION *rf = RtlLookupFunctionEntry(ctx.Rip, &base, NULL);
                char mod[MAX_PATH] = "?";
                if (base != (DWORD64)(uintptr_t)&__ImageBase) GetModuleFileNameA((HMODULE)(uintptr_t)base, mod, sizeof mod);
                else lstrcpyA(mod, "exe");
                int m = wsprintfA(b, "p4pc:   frame %d: %p %s+0x%lx\n", i, (void *)ctx.Rip, mod, (unsigned long)(ctx.Rip - base));
                WriteFile(GetStdHandle(STD_ERROR_HANDLE), b, (DWORD)m, &w, NULL);
                if (!rf) break;
                RtlVirtualUnwind(UNW_FLAG_NHANDLER, base, ctx.Rip, rf, &ctx, &hd, &frame, NULL);
            }
        }
#endif
    }
    if (InterlockedExchange(&crashing, 1)) {
        char b[96];
        int n = wsprintfA(b, "p4pc: second exception %08lx at %p inside the crash handler\n", (unsigned long)er->ExceptionCode, er->ExceptionAddress);
        DWORD w;
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), b, (DWORD)n, &w, NULL);
        TerminateProcess(GetCurrentProcess(), 139);
    }

    describe(where, sizeof where, (uintptr_t)er->ExceptionAddress);
    fprintf(stderr, "p4pc: CRASH %s (0x%08lx) at %s\n", name, (unsigned long)er->ExceptionCode, where);
    if ((er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION || er->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) &&
        er->NumberParameters >= 2) {
        const char *op = er->ExceptionInformation[0] == 0 ? "read" : er->ExceptionInformation[0] == 1 ? "write" : "execute";
        fprintf(stderr, "p4pc:   %s of address 0x%08llx\n", op, (unsigned long long)er->ExceptionInformation[1]);
        if (p4_os_fault_note) p4_os_fault_note((uintptr_t)er->ExceptionInformation[1]);
    }
#if __SIZEOF_POINTER__ == 8
    fprintf(stderr, "p4pc:   rax=%016llx rbx=%016llx rcx=%016llx rdx=%016llx rsi=%016llx rdi=%016llx rbp=%016llx rsp=%016llx\n",
            (unsigned long long)c->Rax, (unsigned long long)c->Rbx, (unsigned long long)c->Rcx, (unsigned long long)c->Rdx,
            (unsigned long long)c->Rsi, (unsigned long long)c->Rdi, (unsigned long long)c->Rbp, (unsigned long long)c->Rsp);
    fprintf(stderr, "p4pc:   r8=%016llx r9=%016llx r10=%016llx r11=%016llx r12=%016llx r13=%016llx r14=%016llx r15=%016llx\n",
            (unsigned long long)c->R8, (unsigned long long)c->R9, (unsigned long long)c->R10, (unsigned long long)c->R11,
            (unsigned long long)c->R12, (unsigned long long)c->R13, (unsigned long long)c->R14, (unsigned long long)c->R15);
    fp = (uintptr_t)c->Rbp;
    sp = (uintptr_t)c->Rsp;
#else
    fprintf(stderr, "p4pc:   eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx\n",
            (unsigned long)c->Eax, (unsigned long)c->Ebx, (unsigned long)c->Ecx, (unsigned long)c->Edx,
            (unsigned long)c->Esi, (unsigned long)c->Edi, (unsigned long)c->Ebp, (unsigned long)c->Esp);
    fp = (uintptr_t)c->Ebp;
    sp = (uintptr_t)c->Esp;
#endif
    /* Frame-pointer walk. Code built without frame pointers yields a short or empty chain; the
     * raw return-address candidates on the stack are listed too. */
    for (depth = 0; depth < 24 && fp != 0 && (fp & (sizeof(uintptr_t) - 1)) == 0 && readable(fp, 2 * sizeof(uintptr_t)); depth++) {
        uintptr_t ret = ((const uintptr_t *)fp)[1];
        uintptr_t next = ((const uintptr_t *)fp)[0];
        describe(where, sizeof where, ret);
        fprintf(stderr, "p4pc:   frame %d: ret %s\n", depth, where);
        if (next <= fp) break;
        fp = next;
    }
    {
        int shown = 0;
        for (size_t i = 0; i < 256 && shown < 12 && readable(sp + i * sizeof(uintptr_t), sizeof(uintptr_t)); i++) {
            uintptr_t v = ((const uintptr_t *)sp)[i];
            HMODULE m = NULL;
            if (v >= 0x1000 && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                                  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                                  (LPCSTR)v, &m) &&
                m == GetModuleHandleA(NULL) && readable(v, 1)) {
                describe(where, sizeof where, v);
                fprintf(stderr, "p4pc:   stack[%lu]: %s\n", (unsigned long)i, where);
                shown++;
            }
        }
    }
    fflush(stderr);
    TerminateProcess(GetCurrentProcess(), 139);
    return EXCEPTION_CONTINUE_SEARCH;
}

void p4_os_init(void) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    AddVectoredExceptionHandler(1, crash_handler);
}

int p4_os_restart_self(int argc, char **argv)
{
    DWORD capacity = MAX_PATH, length;
    WCHAR *executable = NULL, *command;
    STARTUPINFOW startup = {0};
    PROCESS_INFORMATION process;
    (void)argc;
    (void)argv;
    for (;;) {
        WCHAR *grown = realloc(executable, (size_t)capacity * sizeof *executable);
        if (!grown) { free(executable); return -1; }
        executable = grown;
        length = GetModuleFileNameW(NULL, executable, capacity);
        if (!length) goto failed;
        if (length < capacity) break;
        if (capacity >= 32768) { SetLastError(ERROR_FILENAME_EXCED_RANGE); goto failed; }
        capacity *= 2;
        if (capacity > 32768) capacity = 32768;
    }
    /* CreateProcess edits this buffer. Preserve the original Unicode command
     * line verbatim, including quoting, empty arguments and trailing slashes. */
    const WCHAR *original = GetCommandLineW();
    size_t bytes = (wcslen(original) + 1) * sizeof *command;
    command = malloc(bytes);
    if (!command) { free(executable); return -1; }
    memcpy(command, original, bytes);
    startup.cb = sizeof startup;
    BOOL created = CreateProcessW(executable, command, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &process);
    DWORD error = created ? ERROR_SUCCESS : GetLastError();
    free(command);
    free(executable);
    if (!created) {
        fprintf(stderr, "p4pc restart: CreateProcess failed: %lu\n", (unsigned long)error);
        errno = EIO;
        return -1;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return 0;
failed:
    fprintf(stderr, "p4pc restart: executable lookup failed: %lu\n", (unsigned long)GetLastError());
    free(executable);
    errno = EIO;
    return -1;
}

/* ---- time ------------------------------------------------------------------------------- */

uint64_t p4_os_now_ns(void) {
    static LARGE_INTEGER freq;
    LARGE_INTEGER c;
    uint64_t ticks;
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&c);
    ticks = (uint64_t)c.QuadPart;
    return (ticks / (uint64_t)freq.QuadPart) * 1000000000ull +
           (ticks % (uint64_t)freq.QuadPart) * 1000000000ull / (uint64_t)freq.QuadPart;
}

int p4_os_thread_cpu_ns(uint64_t *out) {
    FILETIME created, exited, kernel, user;
    if (!out || !GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user)) return -1;
    uint64_t kernel_ticks = (uint64_t)kernel.dwHighDateTime << 32 | kernel.dwLowDateTime;
    uint64_t user_ticks = (uint64_t)user.dwHighDateTime << 32 | user.dwLowDateTime;
    *out = (kernel_ticks + user_ticks) * 100u;
    return 0;
}

int p4_os_thread_cpu_resolution_ns(uint64_t *out) {
    DWORD adjustment, increment;
    BOOL disabled;
    if (!out || !GetSystemTimeAdjustment(&adjustment, &increment, &disabled) || increment == 0)
        return -1;
    *out = (uint64_t)increment * 100u;
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
    /* A high-resolution waitable timer sleeps to well under a millisecond (Windows 10 1803+); on
     * older systems the plain timer rounds to the system tick, which is still a sleep. */
    static HANDLE timer;
    LARGE_INTEGER due;
    uint64_t trace_start = p4pc_prof_span_begin();
    if (timer == NULL) {
        timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        if (timer == NULL) timer = CreateWaitableTimerW(NULL, TRUE, NULL);
    }
    if (timer == NULL) {
        Sleep((DWORD)((ns + 999999u) / 1000000u));
        p4pc_prof_span_end(trace_start, "sleep.fallback", ns, 0, 0, "p4_os_sleep_ns");
        return;
    }
    due.QuadPart = -(LONGLONG)(ns / 100u + 1u);
    if (SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE))
        WaitForSingleObject(timer, INFINITE);
    else
        Sleep((DWORD)((ns + 999999u) / 1000000u));
    p4pc_prof_span_end(trace_start, "sleep.timer", ns, 0, (uintptr_t)timer, "p4_os_sleep_ns");
}

void p4_os_usleep(unsigned us) { p4_os_sleep_ns((uint64_t)us * 1000u); }

int p4_os_protect_pages(void *addr, size_t size, int access) {
    uintptr_t lo = ((uintptr_t)addr + 4095u) & ~(uintptr_t)4095u;
    uintptr_t hi = ((uintptr_t)addr + size) & ~(uintptr_t)4095u;
    DWORD old;

    if (hi <= lo) {
        return 0;
    }
    return VirtualProtect((void *)lo, hi - lo, access ? PAGE_READWRITE : PAGE_NOACCESS, &old) ? 0 : -1;
}

int p4_os_extended_path(char *path, size_t capacity) {
    WCHAR input[4096], full[4096];
    size_t prefix, skip;
    DWORD length;
    for (char *p = path; *p; p++) if (*p == '/') *p = '\\';
    if (strncmp(path, "\\\\?\\", 4) == 0) return 0;
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, input, sizeof input / sizeof input[0])) {
        errno = EINVAL;
        return -1;
    }
    length = GetFullPathNameW(input, sizeof full / sizeof full[0], full, NULL);
    if (!length || length >= sizeof full / sizeof full[0]) {
        errno = length ? ENAMETOOLONG : EINVAL;
        return -1;
    }
    skip = full[0] == '\\' && full[1] == '\\' ? 2 : 0;
    prefix = skip ? 8 : 4;
    if (prefix >= capacity) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(path, skip ? "\\\\?\\UNC\\" : "\\\\?\\", prefix);
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, full + skip, -1, path + prefix,
                             (int)(capacity - prefix), NULL, NULL)) {
        errno = GetLastError() == ERROR_INSUFFICIENT_BUFFER ? ENAMETOOLONG : EINVAL;
        return -1;
    }
    return 0;
}
/* Older Wine GetFinalPathNameByHandleW uses a fixed MAX_PATH-sized
 * NtQueryObject buffer. Query the same handle name with the required size;
 * accept only the DOS namespace form, otherwise retain the original failure.
 * https://gitlab.winehq.org/wine/wine/-/merge_requests/8285 */
static DWORD resolved_object_path(HANDLE handle, WCHAR *out, DWORD capacity) {
    typedef struct { USHORT length, maximum; WCHAR *buffer; } ObjectName;
    typedef LONG (WINAPI *QueryObject)(HANDLE, ULONG, void *, ULONG, ULONG *);
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    QueryObject query = ntdll ? (QueryObject)GetProcAddress(ntdll, "NtQueryObject") : NULL;
    ULONG needed = 0;
    if (!query) return 0;
    query(handle, 1, NULL, 0, &needed);
    if (needed < sizeof(ObjectName) || needed > 65536) return 0;
    void *storage = malloc(needed);
    if (!storage) return 0;
    DWORD result = 0;
    if (query(handle, 1, storage, needed, &needed) >= 0) {
        ObjectName *name = storage;
        uintptr_t begin = (uintptr_t)storage, end = begin + needed;
        uintptr_t text = (uintptr_t)name->buffer;
        if (!(name->length % sizeof(WCHAR)) && name->length >= 4 * sizeof(WCHAR) &&
            text >= begin && text <= end && name->length <= end - text &&
            !wcsncmp(name->buffer, L"\\??\\", 4)) {
            size_t chars = name->length / sizeof(WCHAR) - 4;
            if (chars + 4 < capacity) {
                memcpy(out, L"\\\\?\\", 4 * sizeof(WCHAR));
                for (size_t i = 0; i < chars; ++i)
                    out[4 + i] = name->buffer[4 + i] == L'/' ? L'\\' : name->buffer[4 + i];
                out[4 + chars] = 0;
                result = (DWORD)(4 + chars);
            }
        }
    }
    free(storage);
    return result;
}
int p4_os_resolved_path(const char *path,char *out,size_t capacity) {
    if(!path||!out||!capacity||capacity>0x7fffffffU){errno=EINVAL;return -1;}
    WCHAR input[4096],resolved[4096];
    char extended[16384];
    size_t path_length=strlen(path);
    if(path_length>=sizeof extended){errno=ENAMETOOLONG;return -1;}
    memcpy(extended,path,path_length+1);
    if(p4_os_extended_path(extended,sizeof extended))return -1;
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,extended,-1,input,sizeof input/sizeof input[0])){
        errno=EINVAL;return -1;
    }
    HANDLE handle=CreateFileW(input,0,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
        NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if(handle==INVALID_HANDLE_VALUE){
        DWORD error=GetLastError();
        errno=error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND?ENOENT:
            error==ERROR_ACCESS_DENIED?EACCES:EIO;return -1;
    }
    DWORD length=GetFinalPathNameByHandleW(handle,resolved,sizeof resolved/sizeof resolved[0],
        FILE_NAME_NORMALIZED|VOLUME_NAME_DOS);
    DWORD error=length?ERROR_SUCCESS:GetLastError();
    if(!length)length=resolved_object_path(handle,resolved,sizeof resolved/sizeof resolved[0]);
    CloseHandle(handle);
    if(!length||length>=sizeof resolved/sizeof resolved[0]){
        errno=length?ENAMETOOLONG:error==ERROR_FILE_NOT_FOUND||error==ERROR_PATH_NOT_FOUND?ENOENT:EIO;return -1;
    }
    if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,resolved,-1,out,(int)capacity,NULL,NULL)){
        errno=GetLastError()==ERROR_INSUFFICIENT_BUFFER?ENAMETOOLONG:EINVAL;return -1;
    }
    return 0;
}

/* One complete object-ID policy for issued file plans and directory leases. */
static int object_identity(HANDLE handle, const BY_HANDLE_FILE_INFORMATION *facts,
                           P4OSFileIdentity *out) {
    FILE_ID_INFO object;
    if (GetFileInformationByHandleEx(handle, FileIdInfo, &object, sizeof object)) {
        out->tokens[0] = object.VolumeSerialNumber;
        memcpy(&out->tokens[1], object.FileId.Identifier, sizeof object.FileId.Identifier);
        out->tokens[6] = 1;
    } else {
        DWORD error = GetLastError();
        if (error != ERROR_INVALID_PARAMETER && error != ERROR_NOT_SUPPORTED && error != ERROR_INVALID_FUNCTION)
            return -1;
        WCHAR filesystem[32];
        if (!GetVolumeInformationByHandleW(handle, NULL, 0, NULL, NULL, NULL,
                filesystem, sizeof filesystem / sizeof filesystem[0]) ||
            (lstrcmpiW(filesystem, L"NTFS") && lstrcmpiW(filesystem, L"FAT") &&
             lstrcmpiW(filesystem, L"FAT32") && lstrcmpiW(filesystem, L"exFAT"))) return -1;
        uint64_t index = (uint64_t)facts->nFileIndexHigh << 32 | facts->nFileIndexLow;
        if (!index) return -1;
        out->tokens[0] = facts->dwVolumeSerialNumber;
        out->tokens[1] = index;
        out->tokens[6] = 2;
    }
    return 0;
}

int p4_os_mkdir(const char *path) { return _mkdir(path); }

int p4_os_directory_lock(const char *path, uintptr_t token[2]) {
    if (!path || !token || token[0] || token[1]) { errno = EINVAL; return -1; }
    WCHAR wide[4096];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, sizeof wide / sizeof wide[0])) {
        errno = EINVAL; return -1;
    }
    /* Wine can report a Unix symlink as a reparse point by path while its
     * OPEN_REPARSE_POINT directory handle reports the target's attributes. */
    DWORD attributes = GetFileAttributesW(wide);
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        errno = ENOTSUP; return -1;
    }
    HANDLE directory = CreateFileW(wide, FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (directory == INVALID_HANDLE_VALUE) { errno = EACCES; return -1; }
    BY_HANDLE_FILE_INFORMATION facts;
    P4OSFileIdentity identity = {0};
    if (!GetFileInformationByHandle(directory, &facts) ||
        !(facts.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
        (facts.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        object_identity(directory, &facts, &identity)) {
        CloseHandle(directory); errno = ENOTSUP; return -1;
    }
    char name[128];
    snprintf(name, sizeof name, "Global\\p4pc-save-%llu-%016llx-%016llx-%016llx",
        (unsigned long long)identity.tokens[6], (unsigned long long)identity.tokens[0],
        (unsigned long long)identity.tokens[1], (unsigned long long)identity.tokens[2]);
    HANDLE mutex = CreateMutexA(NULL, FALSE, name);
    if (!mutex) { CloseHandle(directory); errno = EACCES; return -1; }
    DWORD waited = WaitForSingleObject(mutex, 0);
    if (waited != WAIT_OBJECT_0 && waited != WAIT_ABANDONED) {
        CloseHandle(mutex); CloseHandle(directory);
        errno = waited == WAIT_TIMEOUT ? EBUSY : EIO;
        return -1;
    }
    token[0] = (uintptr_t)mutex; token[1] = (uintptr_t)directory;
    return 0;
}
void p4_os_directory_unlock(uintptr_t token[2]) {
    if (!token) return;
    if (token[0]) {
        if (!ReleaseMutex((HANDLE)token[0])) fprintf(stderr, "p4pc: directory lock released by wrong thread\n");
        CloseHandle((HANDLE)token[0]);
    }
    if (token[1]) CloseHandle((HANDLE)token[1]);
    token[0] = token[1] = 0;
}

/* Indexers/AV may briefly hold a newly written directory without SHARE_DELETE.
 * Retry only the two transient Windows denials, never arbitrary I/O errors. */
static int retry_path_change(const WCHAR *from, const WCHAR *to, const char *operation) {
    unsigned waited = 0, delay = 8;
    for (;;) {
        int rc = to ? _wrename(from, to) : _wrmdir(from);
        if (!rc) {
            if (waited) fprintf(stderr, "p4pc: %s succeeded after %u ms of sharing/access retries\n", operation, waited);
            return 0;
        }
        int error = errno;
        unsigned long native = 0;
        _get_doserrno(&native);
        if ((native != ERROR_ACCESS_DENIED && native != ERROR_SHARING_VIOLATION) || delay > 1024) {
            if (waited) fprintf(stderr, "p4pc: %s failed after %u ms of sharing/access retries (Windows error %lu)\n", operation, waited, native);
            errno = error;
            _set_doserrno(native);
            SetLastError(native);
            return -1;
        }
        Sleep(delay);
        waited += delay;
        delay *= 2;
    }
}

int p4_os_rename(const char *from, const char *to) {
    WCHAR wide_from[4096], wide_to[4096];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, from, -1, wide_from, sizeof wide_from / sizeof wide_from[0]) ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, to, -1, wide_to, sizeof wide_to / sizeof wide_to[0])) {
        errno = EINVAL;
        return -1;
    }
    return retry_path_change(wide_from, wide_to, "rename");
}

int p4_os_replace_file(const char *from, const char *to) {
    WCHAR wide_from[4096], wide_to[4096];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, from, -1, wide_from, sizeof wide_from / sizeof wide_from[0]) ||
        !MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, to, -1, wide_to, sizeof wide_to / sizeof wide_to[0])) {
        errno = EINVAL;
        return -1;
    }
    unsigned delay = 8;
    for (;;) {
        if (MoveFileExW(wide_from, wide_to, MOVEFILE_REPLACE_EXISTING)) return 0;
        DWORD error = GetLastError();
        if ((error != ERROR_ACCESS_DENIED && error != ERROR_SHARING_VIOLATION) || delay > 1024) {
            errno = error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? ENOENT :
                    error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION ? EACCES : EIO;
            return -1;
        }
        Sleep(delay);
        delay *= 2;
    }
}

int p4_os_rmdir(const char *path) {
    WCHAR wide[4096];
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, sizeof wide / sizeof wide[0])) {
        errno = EINVAL;
        return -1;
    }
    return retry_path_change(wide, NULL, "rmdir");
}

int p4_os_list_dir(const char *path, void (*visit)(const char *name, void *user), void *user) {
    char pattern[4096];
    struct _finddata_t entry;
    const size_t length = strlen(path);
    const char *separator;
    intptr_t handle;
    int error;
    if (length == 0) {
        errno = ENOENT;
        return -1;
    }
    separator = path[length - 1] == '/' || path[length - 1] == '\\' ? "" : "\\";
    if (snprintf(pattern, sizeof pattern, "%s%s*", path, separator) >= (int)sizeof pattern) {
        errno = ENAMETOOLONG;
        return -1;
    }
    handle = _findfirst(pattern, &entry);
    if (handle == -1) return -1;
    do {
        visit(entry.name, user);
    } while (_findnext(handle, &entry) == 0);
    error = errno;
    _findclose(handle);
    if (error != ENOENT) {
        errno = error;
        return -1;
    }
    return 0;
}

int p4_os_setenv(const char *name, const char *value, int overwrite) {
    if (!overwrite && getenv(name) != NULL) return 0;
    return _putenv_s(name, value) == 0 ? 0 : -1;
}

void p4_os_headless_env(void) { p4_os_setenv("P4PC_HIDDEN_WINDOW", "1", 0); }

int p4_os_is_dir_nofollow(const char *path) {
    WCHAR wide[4096];
    DWORD a;
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, sizeof wide / sizeof wide[0]))
        return 0;
    a = GetFileAttributesW(wide);
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) && !(a & FILE_ATTRIBUTE_REPARSE_POINT);
}

int p4_os_is_file_nofollow(const char *path) {
    WCHAR wide[4096];
    DWORD a;
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, sizeof wide / sizeof wide[0]))
        return 0;
    a = GetFileAttributesW(wide);
    return a != INVALID_FILE_ATTRIBUTES && !(a & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE));
}

int p4_os_file_identity(FILE *stream, P4OSFileIdentity *out) {
    if (!stream || !out) return -1;
    intptr_t descriptor = _get_osfhandle(_fileno(stream));
    if (descriptor == -1) return -1;
    HANDLE handle = (HANDLE)descriptor;
    BY_HANDLE_FILE_INFORMATION facts;
    if (!GetFileInformationByHandle(handle, &facts) ||
        (facts.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE))) return -1;
    *out = (P4OSFileIdentity){0};
    out->size = (uint64_t)facts.nFileSizeHigh << 32 | facts.nFileSizeLow;
    if (object_identity(handle, &facts, out)) return -1;
    out->tokens[3] = (uint64_t)facts.ftLastWriteTime.dwHighDateTime << 32 | facts.ftLastWriteTime.dwLowDateTime;
    out->tokens[4] = (uint64_t)facts.ftCreationTime.dwHighDateTime << 32 | facts.ftCreationTime.dwLowDateTime;
    return 0;
}

/* ---- fixed-address memory --------------------------------------------------------------- */

/* The exe image owns the PS2 RAM range: the link places a zero-filled `.ps2ram` section exactly
 * at 0x00100000..0x02000000 (win32_ps2ram.S, CMakeLists.txt), because Windows' own start-up
 * (heaps, stacks, locale tables) has already taken parts of that range by the time main runs,
 * so a runtime VirtualAlloc there fails. What is left to do is to make the pages executable. */
static int owned_by_image(uint32_t addr, size_t size) {
    uintptr_t at = addr, end = (uintptr_t)addr + size;
    HMODULE exe = GetModuleHandleW(NULL);
    while (at < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((void *)at, &mbi, sizeof mbi)) return 0;
        if (mbi.State != MEM_COMMIT || mbi.Type != MEM_IMAGE || mbi.AllocationBase != (void *)exe) return 0;
        at = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    return 1;
}

int p4_os_map_fixed(uint32_t addr, size_t size, const char **why) {
    if (owned_by_image(addr, size)) {
        DWORD old;
        if (!VirtualProtect((void *)(uintptr_t)addr, size, PAGE_EXECUTE_READWRITE, &old)) {
            *why = "VirtualProtect on the image's .ps2ram section failed";
            return -1;
        }
        return 0;
    }
    if (VirtualAlloc((void *)(uintptr_t)addr, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE) !=
        (void *)(uintptr_t)addr) {
        *why = "VirtualAlloc at the fixed address failed (range in use, and not provided by the exe's "
               ".ps2ram section)";
        return -1;
    }
    return 0;
}

void *p4_os_map_anywhere(size_t size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

void *p4_os_reserve_anywhere(size_t reserve, size_t commit) {
    void *m = VirtualAlloc(NULL, reserve, MEM_RESERVE, PAGE_NOACCESS);
    if (m == NULL) return NULL;
    if (VirtualAlloc(m, commit, MEM_COMMIT, PAGE_READWRITE) == NULL) {
        VirtualFree(m, 0, MEM_RELEASE);
        return NULL;
    }
    return m;
}

void *p4_os_reserve_aligned(size_t reserve, size_t commit, size_t align) {
    /* a reservation cannot be trimmed: find where an aligned one would fit, give it back and take exactly that range (another thread may take it in between: retry) */
    for (int attempt = 0; attempt < 16; attempt++) {
        void *probe = VirtualAlloc(NULL, reserve + align, MEM_RESERVE, PAGE_NOACCESS);
        if (probe == NULL) return NULL;
        uintptr_t a = ((uintptr_t)probe + align - 1) & ~(uintptr_t)(align - 1);
        VirtualFree(probe, 0, MEM_RELEASE);
        void *m = VirtualAlloc((void *)a, reserve, MEM_RESERVE, PAGE_NOACCESS);
        if (m == NULL) continue;
        if (VirtualAlloc(m, commit, MEM_COMMIT, PAGE_READWRITE) == NULL) {
            VirtualFree(m, 0, MEM_RELEASE);
            return NULL;
        }
        return m;
    }
    return NULL;
}

#if __SIZEOF_POINTER__ == 8
int p4_os_image_range(uintptr_t *lo, uintptr_t *hi) {
    extern IMAGE_DOS_HEADER __ImageBase;
    const IMAGE_NT_HEADERS *nt = (const IMAGE_NT_HEADERS *)((const char *)&__ImageBase + __ImageBase.e_lfanew);
    *lo = (uintptr_t)&__ImageBase;
    *hi = *lo + nt->OptionalHeader.SizeOfImage;
    return 0;
}
#else
int p4_os_image_range(uintptr_t *lo, uintptr_t *hi) {
    *lo = *hi = 0;
    return -1;
}
#endif

/* ---- coroutines (fibers) ----------------------------------------------------------------
 * No FIBER_FLAG_FLOAT_SWITCH: the EE's rounding (MXCSR) and the x87 control word are set once on the game thread (hostmath.h) and every coroutine runs in
 * them, as on Linux, where a coroutine continues the state of the thread that made it. With the flag every new fiber starts with the thread's default float
 * state (MXCSR 0x1F80, x87 control word 0x027F: round to nearest, no flush, 53-bit precision, measured with P4PC_FPU_LOG=1), so the game's worker threads
 * computed their floats differently from the Linux build's. */

#if __SIZEOF_POINTER__ == 8
/* Windows x64: the context switch is ours, so a coroutine runs on exactly the stack memory it is given (the guest block's stack arena,
 * docs/port64.md 2.7) and the address of a local is a guest address. A suspended coroutine is its stack pointer. The frame it holds is, from
 * the stack pointer up: xmm6..xmm15, the three NT_TIB stack fields (DeallocationStack, StackLimit, StackBase), then rbx, rbp, rdi, rsi, r12..r15
 * in pop order. The TIB fields are switched with the stack because exception dispatch and unwinding check the stack pointer against them
 * (the vectored handler that names a guest fault would otherwise never be called on a stack the TIB does not know). MXCSR and the x87 control
 * word are the EE rounding mode, a process-wide setting, and are not switched. */
struct p4_coro {
    void *sp;
};

void p4_coro_swap(void **save_sp, void *load_sp);
__asm__(".text\n"
        ".globl p4_coro_swap\n"
        ".def p4_coro_swap; .scl 2; .type 32; .endef\n"
        "p4_coro_swap:\n"
        "\tpushq %rbp\n\tpushq %rbx\n\tpushq %rdi\n\tpushq %rsi\n\tpushq %r12\n\tpushq %r13\n\tpushq %r14\n\tpushq %r15\n"
        "\tpushq %gs:0x08\n"          /* StackBase */
        "\tpushq %gs:0x10\n"          /* StackLimit */
        "\tpushq %gs:0x1478\n"        /* DeallocationStack */
        "\tsubq $160, %rsp\n"
        "\tmovups %xmm6, 0(%rsp)\n\tmovups %xmm7, 16(%rsp)\n\tmovups %xmm8, 32(%rsp)\n\tmovups %xmm9, 48(%rsp)\n\tmovups %xmm10, 64(%rsp)\n"
        "\tmovups %xmm11, 80(%rsp)\n\tmovups %xmm12, 96(%rsp)\n\tmovups %xmm13, 112(%rsp)\n\tmovups %xmm14, 128(%rsp)\n\tmovups %xmm15, 144(%rsp)\n"
        "\tmovq %rsp, (%rcx)\n"
        "\tmovq %rdx, %rsp\n"
        "\tmovups 0(%rsp), %xmm6\n\tmovups 16(%rsp), %xmm7\n\tmovups 32(%rsp), %xmm8\n\tmovups 48(%rsp), %xmm9\n\tmovups 64(%rsp), %xmm10\n"
        "\tmovups 80(%rsp), %xmm11\n\tmovups 96(%rsp), %xmm12\n\tmovups 112(%rsp), %xmm13\n\tmovups 128(%rsp), %xmm14\n\tmovups 144(%rsp), %xmm15\n"
        "\taddq $160, %rsp\n"
        "\tpopq %rax\n\tmovq %rax, %gs:0x1478\n"
        "\tpopq %rax\n\tmovq %rax, %gs:0x10\n"
        "\tpopq %rax\n\tmovq %rax, %gs:0x08\n"
        "\tpopq %r15\n\tpopq %r14\n\tpopq %r13\n\tpopq %r12\n\tpopq %rsi\n\tpopq %rdi\n\tpopq %rbx\n\tpopq %rbp\n"
        "\tretq\n");

p4_coro *p4_coro_adopt(void) { return calloc(1, sizeof(p4_coro)); }

p4_coro *p4_coro_create(void *stack, size_t stack_size, void (*entry)(void)) {
    p4_coro *c = calloc(1, sizeof *c);
    if (c == NULL) return NULL;
    /* The first resume pops this frame and returns into entry with rsp = top - 0x28: 8 modulo 16 as at the first instruction of a called
     * function, with the 32 bytes of shadow space of the Windows x64 convention above the (never used) return address. */
    uintptr_t top = ((uintptr_t)stack + stack_size) & ~(uintptr_t)15;
    uint64_t *x = (uint64_t *)(top - 0x28);
    uint64_t *sp = x;
    memset(x, 0, 0x28);
    *--sp = (uint64_t)(uintptr_t)entry;
    for (int i = 0; i < 8; i++) *--sp = 0;                 /* rbp, rbx, rdi, rsi, r12..r15 */
    *--sp = (uint64_t)top;                                  /* StackBase */
    /* the limit is a page boundary: __chkstk probes page by page from it down to the frame it is asked for, and stops only on equality with a page */
    uintptr_t limit = ((uintptr_t)stack + 4095u) & ~(uintptr_t)4095u;
    *--sp = (uint64_t)limit;                                /* StackLimit */
    *--sp = (uint64_t)limit;                                /* DeallocationStack */
    sp -= 20;                                               /* xmm6..xmm15 */
    memset(sp, 0, 160);
    c->sp = sp;
    return c;
}

void p4_coro_switch(p4_coro *from, p4_coro *to) { p4_coro_swap(&from->sp, to->sp); }

void p4_coro_destroy(p4_coro *c) { free(c); }
#else
/* i386: fibers. The guest's stack memory is not used: native code needs more stack than the PS2 code did, so the fiber gets its own, never
 * smaller than the guest asked for. */
struct p4_coro {
    void *fiber;
    int adopted;
    void (*entry)(void);
};

p4_coro *p4_coro_adopt(void) {
    p4_coro *c = calloc(1, sizeof *c);
    if (c == NULL) return NULL;
    c->adopted = 1;
    c->fiber = IsThreadAFiber() ? GetCurrentFiber() : ConvertThreadToFiberEx(NULL, 0);
    if (c->fiber == NULL) {
        free(c);
        return NULL;
    }
    return c;
}

static VOID CALLBACK fiber_main(void *param) {
    ((p4_coro *)param)->entry();
    /* entry never returns; a fiber that falls off its function would end the thread */
    ExitProcess(3);
}

p4_coro *p4_coro_create(void *stack, size_t stack_size, void (*entry)(void)) {
    size_t reserve = stack_size > 256u * 1024u ? stack_size : 256u * 1024u;
    p4_coro *c = calloc(1, sizeof *c);
    (void)stack;
    if (c == NULL) return NULL;
    c->entry = entry;
    c->fiber = CreateFiberEx(0, reserve, 0, fiber_main, c);
    if (c->fiber == NULL) {
        free(c);
        return NULL;
    }
    return c;
}

void p4_coro_switch(p4_coro *from, p4_coro *to) {
    (void)from; /* the running fiber is saved by SwitchToFiber itself */
    SwitchToFiber(to->fiber);
}

void p4_coro_destroy(p4_coro *c) {
    if (c == NULL) return;
    if (!c->adopted) DeleteFiber(c->fiber);
    free(c);
}
#endif

/* ---- sampling profiler ---------------------------------------------------------------------- */

static struct {
    HANDLE target;
    HANDLE timer; /* sampler-private: never re-arm the game pacing timer */
    int period_ms;
    void (*cb)(uintptr_t pc, uintptr_t sp, uintptr_t fp, uintptr_t stack_end);
} sProf;

static DWORD WINAPI prof_thread(LPVOID unused) {
    (void)unused;
    for (;;) {
        CONTEXT ctx;
        LARGE_INTEGER due;
        due.QuadPart = -(LONGLONG)sProf.period_ms * 10000ll;
        if (sProf.timer != NULL && SetWaitableTimer(sProf.timer, &due, 0, NULL, NULL, FALSE))
            WaitForSingleObject(sProf.timer, INFINITE);
        else
            Sleep((DWORD)sProf.period_ms);
        if (SuspendThread(sProf.target) == (DWORD)-1) continue;
        ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
        if (GetThreadContext(sProf.target, &ctx)) {
            /* the committed stack pages above the stack pointer: one VirtualQuery region (a guard page ends it) */
            MEMORY_BASIC_INFORMATION mbi;
#if __SIZEOF_POINTER__ == 8
            uintptr_t pc = (uintptr_t)ctx.Rip, sp = (uintptr_t)ctx.Rsp, fp = (uintptr_t)ctx.Rbp;
#else
            uintptr_t pc = (uintptr_t)ctx.Eip, sp = (uintptr_t)ctx.Esp, fp = (uintptr_t)ctx.Ebp;
#endif
            uintptr_t end = sp;

            if (VirtualQuery((void *)sp, &mbi, sizeof mbi) && mbi.State == MEM_COMMIT &&
                !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
                end = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            sProf.cb(pc, sp, fp, end);
        }
        ResumeThread(sProf.target);
    }
    return 0;
}

int p4_os_profile_start(int hz, void (*cb)(uintptr_t pc, uintptr_t sp, uintptr_t fp, uintptr_t stack_end)) {
    HANDLE self = NULL;
    if (!DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &self, 0, FALSE,
                         DUPLICATE_SAME_ACCESS))
        return -1;
    sProf.target = self;
    sProf.cb = cb;
    sProf.period_ms = hz > 0 && 1000 / hz > 0 ? 1000 / hz : 1;
    sProf.timer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (sProf.timer == NULL) sProf.timer = CreateWaitableTimerW(NULL, TRUE, NULL);
    HANDLE thread = CreateThread(NULL, 0, prof_thread, NULL, 0, NULL);
    if (thread == NULL) {
        if (sProf.timer != NULL) CloseHandle(sProf.timer);
        CloseHandle(self);
        return -1;
    }
    CloseHandle(thread);
    return 0;
}

int p4_os_code_readable(uintptr_t addr) {
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery((void *)(addr - 8), &mbi, sizeof mbi) || mbi.State != MEM_COMMIT) return 0;
    if (!(mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) return 0;
    return addr <= (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
}

void p4_os_list_modules(void (*cb)(uintptr_t base, uintptr_t size, const char *name)) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
    MODULEENTRY32 me;
    if (snap == INVALID_HANDLE_VALUE) return;
    me.dwSize = sizeof me;
    if (Module32First(snap, &me)) {
        do {
            cb((uintptr_t)me.modBaseAddr, (uintptr_t)me.modBaseSize, me.szModule);
        } while (Module32Next(snap, &me));
    }
    CloseHandle(snap);
}

FILE *p4_os_fopen_rb(const char *path) { return fopen(path, "rb"); }
