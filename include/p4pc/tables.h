/* Interface to the tables emitted by tools/gen_port.py tables. */
#ifndef P4PC_TABLES_H
#define P4PC_TABLES_H

#include <stddef.h>
#include <stdint.h>

/* A data word at PS2 address `addr` that holds a function address is set to
 * `target` (the native function or generated trap) when the image is loaded. */
typedef struct {
    uint32_t addr;
    void *target;
    const char *sig;   /* register signature of target (include/p4pc/guest.h), or NULL when unknown; 64-bit tables only */
} p4pc_patch_t;

extern const p4pc_patch_t p4pc_patch_table[];
extern const size_t p4pc_patch_count;

/* 64-bit tables only: a thunk that stands for the retail function at `addr` (an m2c name such as func_003bcbc0 that jumps to the
 * compiled function), so that its own address resolves to that function (p4_fn_alias). */
typedef struct {
    uint32_t addr;
    void *fn;
} p4pc_alias_t;

extern const p4pc_alias_t p4pc_alias_table[];
extern const size_t p4pc_alias_count;

/* A compiled function at a retail address (generated: every function the build defines for a retail
 * entry point), used by tools/mipsdiff to call the native where the retail asm calls the function. */
typedef struct {
    uint32_t addr;
    void *fn;
    const char *name;
    const char *sig;   /* as p4pc_patch_t */
} p4pc_native_t;

extern const p4pc_native_t p4pc_native_table[];
extern const size_t p4pc_native_count;

/* A function of the build that has no retail address (a RenderWare routine the decomp names, a port function) with its register signature (64-bit hosts,
 * guest.h): registered at start so that a function word stored for it (a callback kept in a struct) is adapted to the classes of the call site that
 * reads it. Empty on i386. */
typedef struct {
    void *fn;
    const char *sig;
} p4pc_hostfn_t;

extern const p4pc_hostfn_t p4pc_hostfn_table[];
extern const size_t p4pc_hostfn_count;

/* Logs the trap and aborts (P4PC_TRAP_CONTINUE=1: logs the first call and returns 0). Called by generated
 * trap stubs with the first four argument slots. A harness sets p4pc_trap_hook to take the call over: the
 * hook's result is the stub's return value. */
extern int (*p4pc_trap_hook)(const char *name, uint32_t ps2_addr, const uint32_t args[4]);
int p4pc_trap_args(const char *name, uint32_t ps2_addr, void *caller, const uint32_t args[4]);
void p4pc_trap(const char *name, uint32_t ps2_addr, void *caller);

#endif /* P4PC_TABLES_H */
