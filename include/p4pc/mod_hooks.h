#ifndef P4PC_MOD_HOOKS_H
#define P4PC_MOD_HOOKS_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/mod_entry.h"

typedef union P4ModValue { uint64_t u; int64_t i; double d; } P4ModValue;
typedef struct P4ModFunction P4ModFunction;
typedef struct P4ModHook P4ModHook;
typedef struct P4ModCall {
    P4ModFunction *function;
    P4ModValue *args, *result;
    int phase, continued;
} P4ModCall;
typedef int (*P4ModHookFn)(P4ModCall *call, void *user);
/* Signature alphabet: v void result; b/B i8/u8; h/H i16/u16; i/I
 * i32/u32; q/Q i64/u64 (explicit Luau hi/lo values); f/d f32/f64; p guest
 * data address. Result first, then <= 12 parameters. No host pointer in values. */
struct P4ModFunction {
    uint32_t address;
    P4ModEntry *entry;
    const char *signature;
    void (*invoke)(P4ModValue *args, P4ModValue *result);
    P4ModHook *hooks;
    P4ModHook *observers;
};
enum { P4_MOD_BEFORE, P4_MOD_AFTER, P4_MOD_REPLACE };
extern P4ModFunction *const p4_mod_function_table[];
extern const size_t p4_mod_function_count;
P4ModFunction *p4_mod_function_find(uint32_t address, const char *signature);
/* Native observers wrap each actual body invocation, including original/next;
 * they cannot replace a body and do not represent enabled player packages. */
#define P4_MOD_NATIVE_OWNER SIZE_MAX
P4ModHook *p4_mod_hook_add(size_t owner, P4ModFunction *function, int phase, P4ModHookFn callback, void *user);
/* Flag changes/removal are callback-safe because nodes remain tombstones.
 * New registrations and bulk teardown require an inactive dispatch stack. */
int p4_mod_hook_enable(P4ModHook *hook, int enabled);
int p4_mod_hook_remove(P4ModHook *hook);
/* Callback-stack state only, not proof that game tasks/queues are quiescent.
 * Bulk removal fails closed if dispatch is active. clear also frees handles,
 * so release their owning clients/VMs before the final aggregate clear. */
int p4_mod_hooks_quiescent(void);
void p4_mod_hooks_remove_owner(size_t owner);
void p4_mod_dispatch(P4ModFunction *function, P4ModValue *args, P4ModValue *result);
void p4_mod_original(P4ModFunction *function, P4ModValue *args, P4ModValue *result);
int p4_mod_call_next(P4ModCall *call);
void p4_mod_hooks_clear(void);
#endif
