/* Velvet Room command (include/p4pc/mod_velvet.h, the 002e7ac0 seams in
 * mod_entry.h) over stubbed Config and settings services: one command at a
 * time, the hint text in guest storage, the weapon_fusion offer asked once per
 * idle streak, the Config child created, hosted, stepped and destroyed, and
 * every refusal while its page is open. */
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include "p4pc/mod_velvet.h"
#include "p4pc/native_glyphs.h"
#include "guest_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "mod_velvet: FAIL line%d: %s\n", __LINE__, #x); failures++; } } while (0)
_Noreturn void p4_mod_failure(size_t owner, const char *reason)
{
    fprintf(stderr, "mod_velvet: unexpected failure owner %zu: %s\n", owner, reason);
    exit(1);
}

/* Stubbed services: Config creation, step and destroy; the settings host; the
 * keyboard binding; RAW glyph syntax (printable ASCII here). */
static uint32_t configTask, parentSeen, destroyed;
static int modeSeen = -1, created, stepDone, hostFails;
static char hostedKey[128];
static const char *squareBinding = "V";
int32_t cmpConfigCreate(void *parent, int32_t mode)
{
    parentSeen = P4_H2G(parent);
    modeSeen = mode;
    ++created;
    stepDone = 0;
    return (int32_t)configTask;
}
int32_t cmpConfigIsStepDone(void *task) { return P4_H2G(task) == configTask && stepDone; }
uint8_t kwlnTaskDestroyWithHierarchy(void *task) { destroyed = P4_H2G(task); return 1; }
int p4_settings_native_host(uint32_t config, const char *key)
{
    if (hostFails || config != configTask) return -1;
    snprintf(hostedKey, sizeof hostedKey, "%s", key);
    return 0;
}
const char *p4cfg_str(const char *key, const char *def) { return !strcmp(key, "input.kb.square") ? squareBinding : def; }
int p4_native_glyphs_validate(const uint8_t *bytes, size_t size, P4NativeGlyphFlavor flavor, int multiline)
{
    (void)flavor; (void)multiline;
    if (!size || bytes[size - 1]) return -1;
    for (size_t i = 0; i + 1 < size; ++i)
        if (bytes[i] < 0x20 || bytes[i] > 0x7E) return -1;
    return 0;
}

static int asked, allow = 1;
static int offer_rule(const P4ModEvent *context, char *reason, size_t capacity, void *user)
{
    (void)context; (void)user;
    ++asked;
    if (!allow) snprintf(reason, capacity, "NO WEAPON FUSION challenge rule");
    return allow;
}

static const char *hint(void)
{
    uint32_t text = p4_mod_velvet_hint_text();
    return text ? (const char *)P4_G2H(text) : "";
}

int main(void)
{
    if (p4t_guest_init()) return 1;
    const uint32_t velvet = P4_H2G(p4t_galloc(0x40, 16));
    configTask = P4_H2G(p4t_galloc(0x40, 16));

    /* No command: the seams are inert. */
    CHECK(!p4_mod_velvet_mask && !p4_mod_velvet_offer(velvet) && !p4_mod_velvet_hint_text());
    CHECK(p4_mod_velvet_open_page(velvet) == -1 && created == 0);

    /* Refused registrations: no page, an empty or non-RAW label, another operation. */
    CHECK(!p4_mod_velvet_add(1, "", "WEAPON FUSION", -1));
    CHECK(!p4_mod_velvet_add(1, "fusion.forge", "", -1));
    CHECK(!p4_mod_velvet_add(1, "fusion.forge", "WEAPON\nFUSION", -1));
    CHECK(!p4_mod_velvet_add(1, "fusion.forge", "WEAPON FUSION", P4_MOD_AUTH_FUSION));

    /* One command; a second mod is refused while it is held. */
    squareBinding = "V, none";
    P4ModVelvetCommand *command = p4_mod_velvet_add(1, "fusion.forge", "WEAPON FUSION", P4_MOD_AUTH_WEAPON_FUSION);
    CHECK(command && p4_mod_velvet_mask);
    CHECK(!p4_mod_velvet_add(2, "other.page", "OTHER", -1));

    /* The offer asks weapon_fusion once per idle streak and composes the hint. */
    P4ModAuthorizer *rule = p4_mod_authorizer_add(3, "weapon_fusion", offer_rule, NULL);
    CHECK(rule && p4_mod_velvet_offer(velvet) == 1 && asked == 1);
    CHECK(!strcmp(hint(), "SQUARE (V)  WEAPON FUSION"));
    CHECK(p4_mod_velvet_offer(velvet) == 1 && asked == 1);
    p4_mod_velvet_rearm();
    allow = 0;
    CHECK(p4_mod_velvet_offer(velvet) == 0 && asked == 2);
    CHECK(p4_mod_velvet_offer(velvet) == 0 && asked == 2);
    allow = 1;
    p4_mod_velvet_rearm();
    squareBinding = "none";
    CHECK(p4_mod_velvet_offer(velvet) == 1 && asked == 3 && !strcmp(hint(), "SQUARE  WEAPON FUSION"));
    squareBinding = "V";

    /* A failed host destroys the child it created and leaves nothing open. */
    hostFails = 1;
    CHECK(p4_mod_velvet_open_page(velvet) == -1 && created == 1 && destroyed == configTask && !p4_mod_velvet_open());
    hostFails = 0;
    destroyed = 0;

    /* Square: the child of the Velvet task, mode 1, the command's page. */
    CHECK(p4_mod_velvet_open_page(velvet) == 0 && created == 2 && parentSeen == velvet && modeSeen == 1);
    CHECK(!strcmp(hostedKey, "fusion.forge") && p4_mod_velvet_open());
    CHECK(p4_mod_velvet_open_page(velvet) == -1 && created == 2);
    /* Nothing changes the command while its page is open. */
    CHECK(p4_mod_velvet_enable(command, 0) == -1 && p4_mod_velvet_remove(command) == -1);
    /* The root waits while the page runs, then the child is destroyed once. */
    CHECK(p4_mod_velvet_page_step() == 1 && p4_mod_velvet_page_step() == 1 && destroyed == 0);
    stepDone = 1;
    CHECK(p4_mod_velvet_page_step() == 0 && destroyed == configTask && !p4_mod_velvet_open());
    /* Closing asks the offer again on the next idle frame. */
    CHECK(p4_mod_velvet_offer(velvet) == 1 && asked == 4);

    /* Disabled: inert; enabled again: offered. Removal frees the slot for another mod. */
    CHECK(p4_mod_velvet_enable(command, 0) == 0 && !p4_mod_velvet_mask && !p4_mod_velvet_offer(velvet));
    CHECK(p4_mod_velvet_open_page(velvet) == -1);
    CHECK(p4_mod_velvet_enable(command, 1) == 0 && p4_mod_velvet_offer(velvet) == 1 && asked == 5);
    CHECK(p4_mod_velvet_remove(command) == 0 && !p4_mod_velvet_mask);
    P4ModVelvetCommand *other = p4_mod_velvet_add(2, "other.page", "OTHER", -1);
    CHECK(other && p4_mod_velvet_offer(velvet) == 1 && asked == 5); /* no operation: not asked */
    p4_mod_velvet_remove_owner(2);
    CHECK(!p4_mod_velvet_mask);

    if (failures) return 1;
    puts("mod_velvet: ok");
    return 0;
}
