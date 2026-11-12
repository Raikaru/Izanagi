/* Operation authorizers (include/p4pc/mod_authorize.h): the weapon_fusion
 * operation is its own closed entry, independent of fusion, with integer
 * context fields, first-denial-wins order and the seam mask. */
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_authorize: FAIL line%d: %s\n", __LINE__, #x); failures++; } } while (0)

_Noreturn void p4_mod_failure(size_t owner, const char *reason)
{
    fprintf(stderr, "mod_authorize: unexpected failure owner %zu: %s\n", owner, reason);
    exit(1);
}

static P4ModEvent seen;
static int calls;
static int deny_signature(const P4ModEvent *context, char *reason, size_t capacity, void *user)
{
    (void)user;
    seen = *context;
    ++calls;
    for (size_t i = 0; i < context->count; ++i)
        if (!strcmp(context->fields[i].name, "signature") && context->fields[i].value) {
            snprintf(reason, capacity, "NO SIGNATURES");
            return 0;
        }
    return 1;
}
static int deny_all(const P4ModEvent *context, char *reason, size_t capacity, void *user)
{
    (void)context; (void)user;
    ++calls;
    snprintf(reason, capacity, "NO WEAPON FUSION challenge rule");
    return 0;
}
static int deny_fusion(const P4ModEvent *context, char *reason, size_t capacity, void *user)
{
    (void)context; (void)user;
    ++calls;
    snprintf(reason, capacity, "NO FUSION");
    return 0;
}
static int64_t field(const P4ModEvent *event, const char *name)
{
    for (size_t i = 0; i < event->count; ++i)
        if (!strcmp(event->fields[i].name, name)) return event->fields[i].value;
    return -999;
}

int main(void)
{
    char reason[64];
    CHECK(!strcmp(p4_mod_operation_name(P4_MOD_AUTH_WEAPON_FUSION), "weapon_fusion"));
    CHECK(p4_mod_authorizer_add(1, "weapon-fusion", deny_all, NULL) == NULL);
    /* No authorizer: allowed, nothing called, mask clear. */
    CHECK(p4_mod_authorize_mask == 0);
    CHECK(p4_mod_authorize_weapon_fusion(P4_MOD_WEAPON_FUSION_OFFER, 0, 0, 0, 0, reason, sizeof reason) == 1);
    CHECK(calls == 0 && reason[0] == 0);

    /* A fusion authorizer never sees weapon fusion, and the reverse. */
    P4ModAuthorizer *fusion = p4_mod_authorizer_add(1, "fusion", deny_fusion, NULL);
    CHECK(fusion && p4_mod_authorize_mask == P4_MOD_AUTH_FUSION_BIT);
    CHECK(p4_mod_authorize_weapon_fusion(P4_MOD_WEAPON_FUSION_COMMIT, 1, 39, 7, 1, reason, sizeof reason) == 1);
    CHECK(calls == 0);
    CHECK(p4_mod_authorize_fusion(0, 2) == 0 && calls == 1);

    /* Integer context and the first denial wins in load order. */
    P4ModAuthorizer *sig = p4_mod_authorizer_add(2, "weapon_fusion", deny_signature, NULL);
    P4ModAuthorizer *all = p4_mod_authorizer_add(3, "weapon_fusion", deny_all, NULL);
    CHECK(sig && all && (p4_mod_authorize_mask & P4_MOD_AUTH_WEAPON_FUSION_BIT));
    calls = 0;
    CHECK(p4_mod_authorize_weapon_fusion(P4_MOD_WEAPON_FUSION_COMMIT, 4, 88, 100, 1, reason, sizeof reason) == 0);
    CHECK(calls == 1 && !strcmp(reason, "NO SIGNATURES"));
    CHECK(!strcmp(seen.name, "weapon_fusion") && seen.count == 5);
    CHECK(field(&seen, "stage") == 1 && field(&seen, "character") == 4 && field(&seen, "vessel") == 88 &&
        field(&seen, "persona") == 100 && field(&seen, "signature") == 1);
    calls = 0;
    CHECK(p4_mod_authorize_weapon_fusion(P4_MOD_WEAPON_FUSION_OFFER, 0, 0, 0, 0, reason, sizeof reason) == 0);
    CHECK(calls == 2 && !strcmp(reason, "NO WEAPON FUSION challenge rule"));

    /* Disabled and removed authorizers leave the seam mask clear again. */
    CHECK(p4_mod_authorizer_enable(all, 0) == 0);
    calls = 0;
    CHECK(p4_mod_authorize_weapon_fusion(P4_MOD_WEAPON_FUSION_OFFER, 0, 0, 0, 0, NULL, 0) == 1 && calls == 1);
    CHECK(p4_mod_authorizer_remove(sig) == 0);
    CHECK(!(p4_mod_authorize_mask & P4_MOD_AUTH_WEAPON_FUSION_BIT));
    p4_mod_authorizers_remove_owner(1);
    CHECK(p4_mod_authorize_mask == 0);
    p4_mod_authorizers_clear();
    if (failures) return 1;
    puts("mod_authorize: ok");
    return 0;
}
