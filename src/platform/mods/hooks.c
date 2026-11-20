#include "p4pc/mod_hooks.h"
#include "p4pc/mods.h"
#include "p4pc/mod_failure.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct P4ModHook {
    P4ModFunction *function;
    size_t owner;
    int phase, enabled, removed;
    P4ModHookFn callback;
    void *user;
    P4ModHook *next, *previous;
};
static unsigned dispatch_depth;
static unsigned reward_depth;

unsigned p4_mod_reward_begin(void)
{
    unsigned previous = reward_depth;
    reward_depth = dispatch_depth + 1;
    return previous;
}
void p4_mod_reward_end(unsigned previous) { reward_depth = previous; }
int p4_mod_reward_active(void) { return reward_depth != 0 && reward_depth == dispatch_depth; }

int p4_mod_hooks_quiescent(void) { return dispatch_depth == 0; }
static void require_quiescent(void)
{
    if (!dispatch_depth) return;
    fprintf(stderr, "p4pc: mods: bulk hook teardown requires quiescent dispatch\n");
    fflush(stderr);
    _Exit(EXIT_FAILURE);
}


P4ModFunction *p4_mod_function_find(uint32_t address, const char *signature)
{
    size_t low = 0, high = p4_mod_function_count;
    while (low < high) {
        size_t mid = low + (high - low) / 2;
        P4ModFunction *function = p4_mod_function_table[mid];
        if (function->address < address) low = mid + 1;
        else if (function->address > address) high = mid;
        else return !signature || !strcmp(function->signature, signature) ? function : NULL;
    }
    return NULL;
}

static void update(P4ModFunction *function)
{
    function->entry->enabled = P4_MOD_INACTIVE;
    P4ModHook *lists[] = {function->hooks, function->observers};
    for (unsigned i = 0; i < 2; i++)
        for (P4ModHook *h = lists[i]; h; h = h->next)
            if (h->enabled && !h->removed) { function->entry->enabled = P4_MOD_ACTIVE; return; }
}

static const char *owner_name(size_t owner)
{
    const P4PackageInfo *info = p4_packages_info(owner);
    return info ? info->id : "native";
}

static int replacement_available(P4ModHook *hook)
{
    if (hook->phase != P4_MOD_REPLACE) return 1;
    for (P4ModHook *h = hook->function->hooks; h; h = h->next) {
        if (h != hook && h->phase == P4_MOD_REPLACE && h->enabled && !h->removed) {
            fprintf(stderr, "p4pc: mods: replacement conflict at %08x: %s[%zu] and %s[%zu]\n",
                    hook->function->address, owner_name(h->owner), h->owner, owner_name(hook->owner), hook->owner);
            return 0;
        }
    }
    return 1;
}

P4ModHook *p4_mod_hook_add(size_t owner, P4ModFunction *function, int phase, P4ModHookFn callback, void *user)
{
    if (!function || !callback || phase < P4_MOD_BEFORE || phase > P4_MOD_REPLACE || dispatch_depth) return NULL;
    if (owner == P4_MOD_NATIVE_OWNER && phase == P4_MOD_REPLACE) return NULL;
    P4ModHook candidate = {.function = function, .owner = owner, .phase = phase};
    if (!replacement_available(&candidate)) return NULL;
    P4ModHook *hook = calloc(1, sizeof *hook);
    if (!hook) return NULL;
    hook->function = function;
    hook->owner = owner;
    hook->phase = phase;
    hook->callback = callback;
    hook->user = user;
    hook->enabled = 1;
    for (P4ModHook *h = function->hooks; h; h = h->next)
        if (h->enabled && !h->removed && h->owner != owner && owner != P4_MOD_NATIVE_OWNER)
            fprintf(stderr, "p4pc: mods: shared hook %08x: %s[%zu] and %s[%zu] compose in load order\n",
                    function->address, owner_name(h->owner), h->owner, owner_name(owner), owner);
    P4ModHook **link = owner == P4_MOD_NATIVE_OWNER ? &function->observers : &function->hooks;
    P4ModHook *previous = NULL;
    while (*link && (*link)->owner <= owner) { previous = *link; link = &(*link)->next; }
    hook->next = *link;
    hook->previous = previous;
    if (*link) (*link)->previous = hook;
    *link = hook;
    update(function);
    return hook;
}

int p4_mod_hook_enable(P4ModHook *hook, int enabled)
{
    if (!hook || hook->removed) return -1;
    if (enabled && !replacement_available(hook)) return -1;
    hook->enabled = !!enabled;
    update(hook->function);
    return 0;
}

int p4_mod_hook_remove(P4ModHook *hook)
{
    if (!hook) return -1;
    if (hook->removed) return 0;
    /* Subscriptions may still be held by a Luau userdata: retain this tombstone
     * until shutdown rather than turning a removed handle into a dangling ptr. */
    hook->enabled = 0;
    hook->removed = 1;
    update(hook->function);
    return 0;
}

void p4_mod_hooks_remove_owner(size_t owner)
{
    require_quiescent();
    for (size_t i = 0; i < p4_mod_function_count; i++) {
        P4ModFunction *function = p4_mod_function_table[i];
        P4ModHook *head = owner == P4_MOD_NATIVE_OWNER ? function->observers : function->hooks;
        for (P4ModHook *h = head; h; h = h->next)
            if (h->owner == owner) p4_mod_hook_remove(h);
    }
}

static void callback(P4ModHook *hook, P4ModCall *call);
static void enter_dispatch(P4ModFunction *function)
{
    if (++dispatch_depth > 64) {
        char reason[128];
        snprintf(reason, sizeof reason, "Recursive native hook depth exceeded at %08x", function->address);
        p4_mod_failure(P4_MOD_NATIVE_OWNER, reason);
    }
}
static void original_body(P4ModFunction *function, P4ModValue *args, P4ModValue *result)
{
    P4ModCall call = {function, args, result, P4_MOD_BEFORE, 0};
    P4ModHook *last = NULL;
    for (P4ModHook *h = function->observers; h; h = h->next) {
        last = h;
        if (h->enabled && !h->removed && h->phase == P4_MOD_BEFORE) callback(h, &call);
    }
    function->entry->restore = function->entry->enabled;
    function->entry->enabled = P4_MOD_BYPASS;
    function->invoke(args, result);
    for (P4ModHook *h = last; h; h = h->previous)
        if (h->enabled && !h->removed && h->phase == P4_MOD_AFTER) callback(h, &call);
}
void p4_mod_original(P4ModFunction *function, P4ModValue *args, P4ModValue *result)
{
    enter_dispatch(function);
    original_body(function, args, result);
    dispatch_depth--;
}

int p4_mod_call_next(P4ModCall *call)
{
    if (!call || call->phase != P4_MOD_REPLACE || call->continued) return -1;
    call->continued = 1;
    original_body(call->function, call->args, call->result);
    return 0;
}

static void callback(P4ModHook *hook, P4ModCall *call)
{
    call->phase = hook->phase;
    if (hook->callback(call, hook->user)) {
        char reason[160];
        snprintf(reason, sizeof reason, "Native hook failed at %08x (phase %d)", call->function->address, hook->phase);
        p4_mod_failure(hook->owner, reason);
    }
}

void p4_mod_dispatch(P4ModFunction *function, P4ModValue *args, P4ModValue *result)
{
    P4ModCall call = {function, args, result, P4_MOD_BEFORE, 0};
    P4ModHook *replacement = NULL, *last = NULL;
    enter_dispatch(function);
    for (P4ModHook *h = function->hooks; h; h = h->next) {
        last = h;
        if (!h->enabled || h->removed) continue;
        if (h->phase == P4_MOD_BEFORE) callback(h, &call);
    }
    /* Before callbacks may toggle existing subscriptions. Select the actual
     * replacement afterwards; structural insertion remains forbidden. */
    for (P4ModHook *h = function->hooks; h; h = h->next)
        if (h->enabled && !h->removed && h->phase == P4_MOD_REPLACE) { replacement = h; break; }
    if (replacement) callback(replacement, &call);
    else original_body(function, args, result);
    for (P4ModHook *h = last; h; h = h->previous)
        if (h->enabled && !h->removed && h->phase == P4_MOD_AFTER) callback(h, &call);
    dispatch_depth--;
}

void p4_mod_hooks_clear(void)
{
    require_quiescent();
    for (size_t i = 0; i < p4_mod_function_count; i++) {
        P4ModFunction *f = p4_mod_function_table[i];
        P4ModHook *lists[] = {f->hooks, f->observers};
        for (unsigned list = 0; list < 2; list++) {
            P4ModHook *hook = lists[list];
            while (hook) { P4ModHook *next = hook->next; free(hook); hook = next; }
        }
        f->hooks = f->observers = NULL;
        f->entry->enabled = P4_MOD_INACTIVE;
        f->entry->restore = P4_MOD_INACTIVE;
    }
}
