/* Operation authorizer registry (p4.authorize). Seams test the mask; only an
 * enabled authorizer makes a seam consult this list. */
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_failure.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const names[P4_MOD_AUTH_COUNT] = {"fusion", "encounter"};
_Static_assert(P4_MOD_AUTH_FUSION_BIT == 1u << P4_MOD_AUTH_FUSION &&
    P4_MOD_AUTH_ENCOUNTER_BIT == 1u << P4_MOD_AUTH_ENCOUNTER, "authorizer seam bits");
struct P4ModAuthorizer {
    size_t owner;
    int operation, enabled, removed;
    P4ModAuthorizeFn callback;
    void *user;
    P4ModAuthorizer *next;
};
static P4ModAuthorizer *authorizers;
static struct P4ModFormation *formation;
static unsigned formation_depth;
static unsigned depth;
uint32_t p4_mod_authorize_mask;

static void update(void)
{
    uint32_t mask = 0;
    for (P4ModAuthorizer *a = authorizers; a; a = a->next)
        if (a->enabled && !a->removed) mask |= 1u << a->operation;
    p4_mod_authorize_mask = mask;
}
static void require_idle(void)
{
    if (!depth) return;
    fprintf(stderr, "p4pc: mods: authorizer teardown requires idle dispatch\n");
    fflush(stderr);
    _Exit(EXIT_FAILURE);
}

const char *p4_mod_operation_name(int operation)
{
    return operation >= 0 && operation < P4_MOD_AUTH_COUNT ? names[operation] : NULL;
}
P4ModAuthorizer *p4_mod_authorizer_add(size_t owner, const char *operation, P4ModAuthorizeFn callback, void *user)
{
    if (!operation || !callback || depth) return NULL;
    int op = 0;
    while (op < P4_MOD_AUTH_COUNT && strcmp(operation, names[op])) ++op;
    if (op == P4_MOD_AUTH_COUNT) return NULL;
    P4ModAuthorizer *a = calloc(1, sizeof *a);
    if (!a) return NULL;
    *a = (P4ModAuthorizer){owner, op, 1, 0, callback, user, NULL};
    P4ModAuthorizer **link = &authorizers;
    while (*link && (*link)->owner <= owner) link = &(*link)->next;
    a->next = *link;
    *link = a;
    update();
    return a;
}
int p4_mod_authorizer_enable(P4ModAuthorizer *a, int enabled)
{
    if (!a || a->removed || depth) return -1;
    a->enabled = !!enabled;
    update();
    return 0;
}
int p4_mod_authorizer_remove(P4ModAuthorizer *a)
{
    if (!a || depth) return -1;
    a->removed = 1;
    a->enabled = 0;
    update();
    return 0;
}
void p4_mod_authorizers_remove_owner(size_t owner)
{
    require_idle();
    for (P4ModAuthorizer *a = authorizers; a; a = a->next)
        if (a->owner == owner) p4_mod_authorizer_remove(a);
}
void p4_mod_authorizers_clear(void)
{
    require_idle();
    while (authorizers) {
        P4ModAuthorizer *next = authorizers->next;
        free(authorizers);
        authorizers = next;
    }
    p4_mod_authorize_mask = 0;
    if (formation_depth) {
        fprintf(stderr, "p4pc: mods: formation teardown requires idle dispatch\n");
        fflush(stderr);
        _Exit(EXIT_FAILURE);
    }
    free(formation);
    formation = NULL;
    p4_mod_formation_mask = 0;
}
int p4_mod_authorize(int operation, const P4ModEvent *context, char *reason, size_t capacity)
{
    char local[256];
    if (!reason || !capacity) { reason = local; capacity = sizeof local; }
    reason[0] = 0;
    if (!(p4_mod_authorize_mask & (1u << operation))) return 1;
    ++depth;
    for (P4ModAuthorizer *a = authorizers; a; a = a->next) {
        if (!a->enabled || a->removed || a->operation != operation) continue;
        reason[0] = 0;
        int result = a->callback(context, reason, capacity, a->user);
        if (result < 0) {
            char message[160];
            snprintf(message, sizeof message, "Native %s authorizer failed", names[operation]);
            --depth;
            p4_mod_failure(a->owner, message);
        }
        if (result == 0) {
            reason[capacity - 1] = 0;
            --depth;
            fprintf(stderr, "p4pc: mods: %s refused by owner %zu: %s\n", names[operation], a->owner,
                reason[0] ? reason : "(no reason)");
            return 0;
        }
    }
    --depth;
    reason[0] = 0;
    return 1;
}
int p4_mod_authorize_fusion(int kind, int materials)
{
    P4ModEvent context = {"fusion", 2, {{"kind", kind}, {"materials", materials}}};
    return p4_mod_authorize(P4_MOD_AUTH_FUSION, &context, NULL, 0);
}

/* Battle formation projector: one owner at a time, never last-writer-wins. */
struct P4ModFormation {
    size_t owner;
    int enabled, removed;
    P4ModFormationFn callback;
    void *user;
};
uint32_t p4_mod_formation_mask;

P4ModFormation *p4_mod_formation_add(size_t owner, P4ModFormationFn callback, void *user)
{
    if (!callback || formation_depth || (formation && !formation->removed)) return NULL;
    P4ModFormation *f = calloc(1, sizeof *f);
    if (!f) return NULL;
    *f = (P4ModFormation){owner, 1, 0, callback, user};
    /* A removed projector stays a tombstone: its handle may still be held. */
    formation = f;
    p4_mod_formation_mask = 1;
    return f;
}
int p4_mod_formation_enable(P4ModFormation *f, int enabled)
{
    if (!f || f != formation || f->removed || formation_depth) return -1;
    f->enabled = !!enabled;
    p4_mod_formation_mask = (uint32_t)f->enabled;
    return 0;
}
int p4_mod_formation_remove(P4ModFormation *f)
{
    if (!f || formation_depth) return -1;
    if (f == formation) p4_mod_formation_mask = 0;
    f->removed = 1;
    f->enabled = 0;
    return 0;
}
void p4_mod_formation_remove_owner(size_t owner)
{
    if (formation_depth) {
        fprintf(stderr, "p4pc: mods: formation teardown requires idle dispatch\n");
        fflush(stderr);
        _Exit(EXIT_FAILURE);
    }
    if (formation && formation->owner == owner) p4_mod_formation_remove(formation);
}
uint32_t p4_mod_formation_project(uint32_t packed, uint32_t count)
{
    P4ModFormation *f = formation;
    if (!p4_mod_formation_mask || !f || !f->enabled || f->removed || count > 4) return 0xFFFFFFFFu;
    uint8_t in[4] = {0}, out[4] = {0};
    for (uint32_t i = 0; i < count; ++i) in[i] = (uint8_t)(packed >> (i * 8));
    ++formation_depth;
    int n = f->callback(in, (int)count, out, f->user);
    --formation_depth;
    if (n < 0) p4_mod_failure(f->owner, "Native battle formation projector failed");
    if (n == 0) return 0xFFFFFFFFu;
    int valid = n <= 4;
    for (int i = 0; valid && i < n; ++i) {
        valid = out[i] >= 1 && out[i] <= 10;
        for (int j = 0; valid && j < i; ++j) valid = out[i] != out[j];
    }
    if (!valid) p4_mod_failure(f->owner, "Battle formation projector returned an invalid lineup");
    uint32_t result = (uint32_t)n << 24;
    for (int i = 0; i < n; ++i) result |= (uint32_t)out[i] << (i * 4);
    fprintf(stderr, "p4pc: mods: battle lineup projected by owner %zu: %u -> %d members\n", f->owner, count, n);
    return result;
}
