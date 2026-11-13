/* Real page registry + MODS attachment seam; no fake settings backend or VM. */
#include "p4pc/mod_settings.h"
#include "p4pc/settings.h"
#include "p4pc/os.h"
#include "p4pc/settings_native.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static int failures, queries;
#define CHECK(value) do { if (!(value)) { \
    fprintf(stderr, "mod_settings: FAIL line%d: %s\n", __LINE__, #value); \
    ++failures; \
} } while (0)
static const char *body(void *user, uint64_t *revision)
{
    (void)user;
    ++queries;
    *revision = 1;
    return "Registered native provider.";
}
static int root_count(void)
{
    int found = 0;
    for (int i = 0; i < p4_settings_entry_count(); ++i) {
        char label[192], value[128];
        if (p4_settings_entry(i, label, sizeof label, value, sizeof value) < 0) continue;
        if (!strcmp(label, "MODS")) {
            CHECK(!strcmp(value, "OPEN"));
            CHECK(p4_settings_entry_flags(i) == P4_SETTINGS_ACTION);
            ++found;
        }
    }
    return found;
}
int main(void)
{
    p4_os_setenv("P4PC_INPUT", "script", 1);
    p4_os_setenv("SDL_AUDIODRIVER", "dummy", 1);
    p4_os_setenv("P4PC_CONFIG", "none", 1);
    int before = p4_settings_entry_count();
    CHECK(p4_mod_settings_init() == 0);
    CHECK(p4_mod_settings_init() == 0);
    CHECK(p4_settings_entry_count() == before + 1);
    CHECK(root_count() == 1);
    static int ownerA, ownerB;
    P4SettingsPage page = {.key="service/unit-one", .title="NATIVE SERVICE", .detail=body};
    CHECK(p4_settings_page_register(&page) == 0);
    char key[128] = "service/unit-one", label[192] = "COPIED NATIVE CAPTION";
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_SERVICE, "unit", key, label) == 0);
    memset(key, 'x', sizeof key);
    memset(label, 'x', sizeof label);
    CHECK(p4_mod_settings_attach_page(&ownerB, P4_MOD_SETTINGS_SERVICE, "other", page.key, "DUPLICATE") == -1);
    CHECK(p4_mod_settings_detach_page(&ownerB, page.key) == -1);
    CHECK(p4_mod_settings_detach_page(&ownerA, page.key) == 0);
    CHECK(p4_settings_page_register(&page) == -1); /* unlink is NOT unregister */
    /* Hosting validates before any guest task access, and never opens a view. */
    CHECK(p4_settings_native_host(0, page.key) == -1);
    CHECK(p4_settings_native_host(0x00100000u, "service/not-registered") == -1);
    CHECK(p4_settings_native_active() == 0 && p4_settings_page_active() == NULL);
    CHECK(p4_settings_page_unregister(page.key) == 0);

    page.key = "lua/witness/one";
    CHECK(p4_settings_page_register(&page) == 0);
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_SERVICE, "native", page.key, "WRONG NAMESPACE") == -1);
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_MOD, "witness", page.key, "PLAYER PAGE") == 0);
    CHECK(p4_mod_settings_attach_page(NULL, P4_MOD_SETTINGS_MOD, "witness", "unowned", "NULL OWNER") == -1);
    CHECK(p4_mod_settings_attach_page(&ownerA, (P4ModSettingsTarget)99, "witness", "bad-kind", "BAD KIND") == -1);
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_MOD, "", "empty-id", "EMPTY ID") == -1);
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_MOD, "witness", "raw-style", "\xc0\x80") == -1);
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_MOD, "witness", "raw-pair", "\x83") == -1);
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_MOD, "witness", "raw-line", "BAD\nLINE") == -1);
    page.key = "service/unit-two";
    CHECK(p4_settings_page_register(&page) == 0);
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_SERVICE, "native", page.key, "NATIVE\x83\xf6SPACE") == 0);
    CHECK(p4_mod_settings_detach_owner(&ownerB) == 0);
    CHECK(p4_mod_settings_detach_owner(NULL) == -1);
    CHECK(p4_mod_settings_detach_owner(&ownerA) == 2);
    CHECK(p4_mod_settings_detach_owner(&ownerA) == 0);
    CHECK(p4_settings_page_unregister("lua/witness/one") == 0);
    CHECK(p4_settings_page_unregister("service/unit-two") == 0);

    char maximumId[64], maximumKey[128], maximumLabel[192];
    memset(maximumId, 'a', sizeof maximumId - 1); maximumId[63] = 0;
    memset(maximumKey, 'k', sizeof maximumKey - 1); maximumKey[127] = 0;
    memset(maximumLabel, 'L', sizeof maximumLabel - 1); maximumLabel[191] = 0;
    page.key = maximumKey;
    CHECK(p4_settings_page_register(&page) == 0);
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_MOD, maximumId, maximumKey, maximumLabel) == 0);
    CHECK(p4_mod_settings_detach_owner(&ownerA) == 1);
    CHECK(p4_settings_page_unregister(page.key) == 0);
    maximumId[63] = 'a';
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_MOD, maximumId, "bounds", "ID TOO LONG") == -1);
    maximumId[63] = 0;
    maximumKey[127] = 'k';
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_MOD, maximumId, maximumKey, "KEY TOO LONG") == -1);
    maximumKey[127] = 0;
    maximumLabel[191] = 'L';
    CHECK(p4_mod_settings_attach_page(&ownerA, P4_MOD_SETTINGS_MOD, maximumId, maximumKey, maximumLabel) == -1);
    CHECK(queries == 0); /* no provider entered by registration/link/unlink */
    CHECK(p4_settings_page_active() == NULL);
    CHECK(root_count() == 1);
    fprintf(stderr, "mod_settings: %s (one C root, owned copies, MOD/SERVICE namespaces, unlink before unregister)\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
