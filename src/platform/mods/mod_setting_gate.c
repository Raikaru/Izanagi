#include "p4pc/mod_setting_gate.h"
#include "p4pc/mod_setting_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int refuse(char *reason, size_t capacity, const char *text, const char *key)
{
    if (reason && capacity) snprintf(reason, capacity, "%s%s%s", text, key ? ": " : "", key ? key : "");
    return -1;
}
int p4_mod_setting_section_gate(const P4PackageProvider *provider, const P4PackageField *fields,
                                size_t count, char *reason, size_t capacity)
{
    if (reason && capacity) *reason = 0;
    const char *key = NULL;
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(fields[i].key, P4_MOD_SETTING_GATE_FIELD)) continue;
        if (key) return refuse(reason, capacity, "a section takes at most one when", NULL);
        key = fields[i].value;
    }
    if (!key) return 1;
    if (!*key) return refuse(reason, capacity, "when needs a declared toggle key", NULL);
    size_t all = 0, found = 0;
    const P4PackageField *manifest = p4_package_provider_fields(provider, &all);
    P4ModSettingDecl *decls = NULL;
    char error[256];
    if (!manifest || p4_mod_setting_declarations(manifest, all, &decls, &found, error, sizeof error))
        return refuse(reason, capacity, "when cannot read the mod's [setting] declarations", NULL);
    const P4ModSettingDecl *decl = p4_mod_setting_find(decls, found, key);
    const int toggle = decl && decl->kind == P4_MOD_SETTING_TOGGLE, restart = decl && decl->restart;
    free(decls);
    if (!decl) return refuse(reason, capacity, "when names an undeclared setting", key);
    if (!toggle) return refuse(reason, capacity, "when needs a toggle setting", key);
    /* Boot sections are sealed for the whole session: a live value would
     * disagree with them after the first change. */
    if (!restart) return refuse(reason, capacity, "when needs a setting with apply = restart", key);
    char value[P4_MOD_SETTING_CHOICE_MAX];
    if (p4_mod_setting_boot(provider, key, value, sizeof value) != 1)
        return refuse(reason, capacity, "when has no admitted value for", key);
    return !strcmp(value, "on");
}
