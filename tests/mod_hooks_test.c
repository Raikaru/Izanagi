#include "p4pc/mod_hooks.h"
#include "p4pc/gmem.h"
#include "p4pc/os.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint32_t mod_test_unsigned(uint32_t);
int16_t mod_test_narrow(int8_t, uint16_t);
float mod_test_mixed(int, float, unsigned char *, double);
uint32_t *mod_test_pointer(uint32_t *);
int mod_test_recursive(int);
void mod_test_void(uint32_t *);
static int failures, recursive_calls, native_before, native_after;
static int teardown_mode;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_hooks: FAIL line %d: %s\n", __LINE__, #x); failures++; } } while (0)

static int first(P4ModCall *call, void *user)
{
    (void)user;
    CHECK(!p4_mod_hooks_quiescent());
    if (teardown_mode == 1) p4_mod_hooks_clear();
    if (teardown_mode == 2) p4_mod_hooks_remove_owner(0);
    if (call->phase == P4_MOD_BEFORE) call->args[0].i++;
    else call->result->d++;
    return 0;
}
static int second(P4ModCall *call, void *user)
{
    (void)user;
    if (call->phase == P4_MOD_BEFORE) call->args[0].i *= 2;
    else call->result->d *= 2;
    return 0;
}
static int replacement(P4ModCall *call, void *user)
{
    (void)user;
    CHECK(p4_mod_call_next(call) == 0);
    CHECK(p4_mod_call_next(call) == -1);
    call->result->d += 5;
    return 0;
}
static int recursion(P4ModCall *call, void *user)
{
    (void)call; (void)user;
    recursive_calls++;
    return 0;
}

static int observer(P4ModCall *call, void *user)
{
    (void)user;
    if (call->phase == P4_MOD_BEFORE) native_before++;
    else native_after++;
    return 0;
}
static int skip_original(P4ModCall *call, void *user)
{
    (void)user;
    call->result->d = 10;
    return 0;
}

typedef struct Toggle { P4ModHook *replacement, *self; } Toggle;
static int toggle_existing(P4ModCall *call, void *user)
{
    (void)call;
    Toggle *toggle = user;
    CHECK(p4_mod_hook_enable(toggle->replacement, 1) == 0);
    CHECK(p4_mod_hook_remove(toggle->self) == 0);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc == 2) {
        if (!strcmp(argv[1], "--active-clear")) teardown_mode = 1;
        else if (!strcmp(argv[1], "--active-owner-remove")) teardown_mode = 2;
        else return 2;
    } else if (argc != 1) return 2;
    CHECK(p4_mod_hooks_quiescent());
    p4_os_init();
    uint32_t storage = p4_gport_alloc(16, 16);
    if (!storage) return 77;
    uint32_t *word = P4_G2H(storage);
    unsigned char *byte = (unsigned char *)word;
    *byte = 3;
    P4ModFunction *mixed = p4_mod_function_find(0x00100140, "fifpd");
    CHECK(mixed && !mixed->entry->enabled);
    CHECK(!p4_mod_function_find(0x00100140, "ii"));
    CHECK(!p4_mod_function_find(0x00100141, "fifpd"));
    CHECK(mod_test_unsigned(UINT32_MAX - 1) == UINT32_MAX);
    CHECK(mod_test_narrow(-7, 12) == 5);
    CHECK(mod_test_pointer(word) == word);
    CHECK(mod_test_mixed(2, 1.5f, byte, 2.25) == 8.75f);
    mod_test_void(word);
    CHECK(*word == 4);
    *byte = 3;
    CHECK(p4_mod_hook_add(P4_MOD_NATIVE_OWNER, mixed, P4_MOD_BEFORE, observer, NULL));
    CHECK(p4_mod_hook_add(P4_MOD_NATIVE_OWNER, mixed, P4_MOD_AFTER, observer, NULL));
    CHECK(!p4_mod_hook_add(P4_MOD_NATIVE_OWNER, mixed, P4_MOD_REPLACE, replacement, NULL));
    P4ModHook *b = p4_mod_hook_add(1, mixed, P4_MOD_BEFORE, second, NULL);
    P4ModHook *a = p4_mod_hook_add(0, mixed, P4_MOD_BEFORE, first, NULL);
    P4ModHook *aa = p4_mod_hook_add(0, mixed, P4_MOD_AFTER, first, NULL);
    P4ModHook *bb = p4_mod_hook_add(1, mixed, P4_MOD_AFTER, second, NULL);
    P4ModHook *r = p4_mod_hook_add(2, mixed, P4_MOD_REPLACE, replacement, NULL);
    CHECK(a && b && aa && bb && r && mixed->entry->enabled == P4_MOD_ACTIVE);
    CHECK(mod_test_mixed(2, 1.5f, byte, 2.25) == 36.5f);
    CHECK(native_before == 1 && native_after == 1);
    CHECK(!p4_mod_hook_add(3, mixed, P4_MOD_REPLACE, replacement, NULL));
    CHECK(p4_mod_hook_enable(r, 0) == 0);
    CHECK(mod_test_mixed(2, 1.5f, byte, 2.25) == 26.5f);
    CHECK(native_before == 2 && native_after == 2);
    CHECK(p4_mod_hook_enable(r, 1) == 0);
    P4ModValue args[4] = {{0}}, result = {0};
    args[0].i = 2; args[1].d = 1.5; args[2].u = storage; args[3].d = 2.25;
    p4_mod_original(mixed, args, &result);
    CHECK(result.d == 8.75 && mixed->entry->enabled == P4_MOD_ACTIVE);
    CHECK(native_before == 3 && native_after == 3);
    CHECK(p4_mod_hook_enable(r, 0) == 0);
    P4ModHook *skip = p4_mod_hook_add(3, mixed, P4_MOD_REPLACE, skip_original, NULL);
    CHECK(skip && mod_test_mixed(2, 1.5f, byte, 2.25) == 21.0f);
    CHECK(native_before == 3 && native_after == 3);
    CHECK(p4_mod_hook_remove(skip) == 0);
    p4_mod_hooks_remove_owner(P4_MOD_NATIVE_OWNER);
    Toggle toggle = {r, NULL};
    toggle.self = p4_mod_hook_add(4, mixed, P4_MOD_BEFORE, toggle_existing, &toggle);
    CHECK(toggle.self && mod_test_mixed(2, 1.5f, byte, 2.25) == 36.5f);
    CHECK(native_before == 3 && native_after == 3);
    P4ModFunction *rec = p4_mod_function_find(0x00100180, "ii");
    CHECK(p4_mod_hook_add(0, rec, P4_MOD_BEFORE, recursion, NULL));
    CHECK(mod_test_recursive(4) == 4 && recursive_calls == 5);
    CHECK(p4_mod_hook_remove(a) == 0);
    CHECK(p4_mod_hook_enable(a, 1) == -1);
    p4_mod_hooks_clear();
    CHECK(p4_mod_hooks_quiescent());
    CHECK(!mixed->entry->enabled && !rec->entry->enabled);
    CHECK(mod_test_recursive(4) == 4 && recursive_calls == 5);
    printf("mod_hooks: %s (mixed float/double, narrow, unsigned, guest pointers, ordering, original/next observers, recursion, conflicts)\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
