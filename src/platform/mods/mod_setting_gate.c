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
int p4_mod_setting_gate_declared(const P4PackageField *fields, size_t count, const P4PackageField *manifest,
                                 size_t all, const char **gate, int *default_on, char *reason, size_t capacity)
{
    const char *key = NULL;
    *gate = NULL;
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(fields[i].key, P4_MOD_SETTING_GATE_FIELD)) continue;
        if (key) return refuse(reason, capacity, "a section takes at most one when", NULL);
        key = fields[i].value;
    }
    if (!key) return 0;
    if (!*key) return refuse(reason, capacity, "when needs a declared toggle key", NULL);
    size_t found = 0;
    P4ModSettingDecl *decls = NULL;
    char error[256];
    if (!manifest || p4_mod_setting_declarations(manifest, all, &decls, &found, error, sizeof error))
        return refuse(reason, capacity, "when cannot read the mod's [setting] declarations", NULL);
    const P4ModSettingDecl *decl = p4_mod_setting_find(decls, found, key);
    const int toggle = decl && decl->kind == P4_MOD_SETTING_TOGGLE, restart = decl && decl->restart;
    const int on = toggle && decl->default_choice < decl->choice_count &&
        !strcmp(decl->choices[decl->default_choice], "on");
    free(decls);
    if (!decl) return refuse(reason, capacity, "when names an undeclared setting", key);
    if (!toggle) return refuse(reason, capacity, "when needs a toggle setting", key);
    /* Boot sections are sealed for the whole session: a live value would
     * disagree with them after the first change. */
    if (!restart) return refuse(reason, capacity, "when needs a setting with apply = restart", key);
    *gate = key;
    if (default_on) *default_on = on;
    return 1;
}
#ifndef P4_MOD_SETTING_CHECK
/* The section's `when`: 1 absent or on, 0 off, -1 refused. */
static int when_gate(const P4PackageProvider *provider, const P4PackageField *fields, size_t count,
                     char *reason, size_t capacity)
{
    size_t all = 0;
    const P4PackageField *manifest = NULL;
    for (size_t i = 0; i < count && !manifest; ++i)
        if (!strcmp(fields[i].key, P4_MOD_SETTING_GATE_FIELD)) manifest = p4_package_provider_fields(provider, &all);
    const char *key;
    const int declared = p4_mod_setting_gate_declared(fields, count, manifest, all, &key, NULL, reason, capacity);
    if (declared <= 0) return declared < 0 ? -1 : 1;
    char value[P4_MOD_SETTING_CHOICE_MAX];
    if (p4_mod_setting_boot(provider, key, value, sizeof value) != 1)
        return refuse(reason, capacity, "when has no admitted value for", key);
    return !strcmp(value, "on");
}
/* The admitted companion package with this id, or NULL. */
static const P4PackageProvider *companion_provider(const char *id)
{
    for (size_t i = 0; i < p4_packages_count(); ++i) {
        const P4PackageInfo *info = p4_packages_info(i);
        if (info && !strcmp(info->id, id)) return p4_packages_provider(i);
    }
    return NULL;
}
/* `companion-setting = <key>=<value>`, syntax only: it runs whether or not the
 * companion is admitted, so a malformed declaration never depends on another
 * mod being present. 0, or -1 refused. */
static int companion_setting_parse(const char *id, const char *setting, char key[P4_MOD_SETTING_KEY_MAX],
                                   const char **wanted, char *reason, size_t capacity)
{
    const char *equals = strchr(setting, '=');
    size_t length = equals ? (size_t)(equals - setting) : 0;
    if (!equals || !length || length >= P4_MOD_SETTING_KEY_MAX || !equals[1]) {
        if (reason && capacity) snprintf(reason, capacity, "companion-setting needs <key>=<value> for %s", id);
        return -1;
    }
    memcpy(key, setting, length);
    key[length] = 0;
    *wanted = equals + 1;
    return 0;
}
/* The parsed setting against the admitted companion's restart setting: 1 equal,
 * 0 different, -1 refused (not a declared restart key and value). */
static int companion_setting(const char *id, const char *key, const char *wanted, char *reason, size_t capacity)
{
    const P4PackageProvider *companion = companion_provider(id);
    size_t all = 0, found = 0;
    const P4PackageField *manifest = companion ? p4_package_provider_fields(companion, &all) : NULL;
    P4ModSettingDecl *decls = NULL;
    char error[256];
    if (!manifest || p4_mod_setting_declarations(manifest, all, &decls, &found, error, sizeof error)) {
        if (reason && capacity) snprintf(reason, capacity, "companion %s: cannot read its [setting] declarations", id);
        return -1;
    }
    /* A restart setting the companion declares, and one of its declared values
     * (a toggle's choices are off and on). */
    const P4ModSettingDecl *decl = p4_mod_setting_find(decls, found, key);
    int valid = 0;
    for (unsigned i = 0; decl && decl->restart && i < decl->choice_count; ++i) valid |= !strcmp(decl->choices[i], wanted);
    free(decls);
    if (!valid) {
        if (reason && capacity)
            snprintf(reason, capacity, "companion-setting %s=%s is not a restart setting value that companion %s declares",
                     key, wanted, id);
        return -1;
    }
    char value[P4_MOD_SETTING_CHOICE_MAX];
    if (p4_mod_setting_boot(companion, key, value, sizeof value) != 1) {
        if (reason && capacity) snprintf(reason, capacity, "companion %s has no admitted value for %s", id, key);
        return -1;
    }
    return !strcmp(value, wanted);
}
/* The section's `companion` (and its optional `companion-setting`): 1 absent or
 * admitted (with the setting's value), 0 not admitted or another value, -1
 * refused. It is checked even when `when` is off, so a bad declaration never
 * depends on a toggle's value; the companion's setting is read only once the
 * companion is admitted. */
static int companion_gate(const P4PackageProvider *provider, const P4PackageField *fields, size_t count,
                          char *reason, size_t capacity)
{
    const char *id = NULL, *setting = NULL;
    for (size_t i = 0; i < count; ++i) {
        if (!strcmp(fields[i].key, P4_MOD_COMPANION_SETTING_FIELD)) {
            if (setting) return refuse(reason, capacity, "a section takes at most one companion-setting", NULL);
            setting = fields[i].value;
        }
        if (strcmp(fields[i].key, P4_MOD_COMPANION_GATE_FIELD)) continue;
        if (id) return refuse(reason, capacity, "a section takes at most one companion", NULL);
        id = fields[i].value;
    }
    if (!id && setting) return refuse(reason, capacity, "companion-setting needs a companion in the same section", NULL);
    if (!id) return 1;
    if (!*id) return refuse(reason, capacity, "companion needs a declared [companion] id", NULL);
    char key[P4_MOD_SETTING_KEY_MAX];
    const char *wanted = NULL;
    if (setting && companion_setting_parse(id, setting, key, &wanted, reason, capacity)) return -1;
    const int admitted = p4_packages_companion_admitted(provider, id);
    if (admitted < 0) {
        if (reason && capacity) snprintf(reason, capacity, "companion = %s is not a declared [companion]", id);
        return -1;
    }
    if (!admitted || !setting) return admitted;
    return companion_setting(id, key, wanted, reason, capacity);
}
int p4_mod_setting_section_gate(const P4PackageProvider *provider, const P4PackageField *fields,
                                size_t count, char *reason, size_t capacity)
{
    if (reason && capacity) *reason = 0;
    const int when = when_gate(provider, fields, count, reason, capacity);
    if (when < 0) return -1;
    const int companion = companion_gate(provider, fields, count, reason, capacity);
    if (companion < 0) return -1;
    return when && companion;
}
#endif
