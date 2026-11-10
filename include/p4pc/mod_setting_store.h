/* Persisted per-mod settings (docs/modding/files.md "Settings", lua.md "p4.settings").
 *
 * A mod declares Config rows as `[setting <key>]` manifest sections (api 1.2).
 * Values live in `<settings directory>/mod-settings/<mod id>.ini`, one file per
 * mod, written by atomic replacement; the shared p4pc.ini is never touched.
 * Declared keys hold a choice name ("on"/"off" for toggles); undeclared data
 * keys hold a scalar the owning mod encodes. Game thread only. */
#ifndef P4PC_MOD_SETTING_STORE_H
#define P4PC_MOD_SETTING_STORE_H
#include <stddef.h>
#include "p4pc/packages.h"

#define P4_MOD_SETTING_KEY_MAX 49      /* [a-z0-9][a-z0-9_.-]{0,47}, including NUL */
#define P4_MOD_SETTING_VALUE_MAX 256   /* printable ASCII, including NUL */
#define P4_MOD_SETTING_CHOICE_MAX 32
#define P4_MOD_SETTING_CHOICES 8
#define P4_MOD_SETTING_LABEL_MAX 64
#define P4_MOD_SETTING_HELP_MAX 512
#define P4_MOD_SETTINGS_PER_MOD 256    /* declared settings, and separately stored keys */

typedef enum P4ModSettingKind { P4_MOD_SETTING_TOGGLE = 1, P4_MOD_SETTING_CHOICE = 2 } P4ModSettingKind;
typedef struct P4ModSettingDecl {
    char key[P4_MOD_SETTING_KEY_MAX];
    P4ModSettingKind kind;
    unsigned choice_count, default_choice; /* toggles have the two choices off, on */
    char choices[P4_MOD_SETTING_CHOICES][P4_MOD_SETTING_CHOICE_MAX];
    char label[P4_MOD_SETTING_LABEL_MAX], help[P4_MOD_SETTING_HELP_MAX], group[P4_MOD_SETTING_LABEL_MAX];
    int restart; /* apply=restart: the effective value is frozen at admission */
    unsigned line;  /* first manifest line, the display order */
} P4ModSettingDecl;

int p4_mod_setting_key_valid(const char *key);
/* 1..255 printable ASCII bytes without a leading or trailing space. */
int p4_mod_setting_value_valid(const char *value);
/* Parses every [setting] section of one manifest into *out (malloc'd, sorted by
 * manifest line; NULL when none). 0, or -1 with a bounded reason. */
int p4_mod_setting_declarations(const P4PackageField *fields, size_t count,
                                P4ModSettingDecl **out, size_t *found, char *reason, size_t capacity);
const P4ModSettingDecl *p4_mod_setting_find(const P4ModSettingDecl *decls, size_t count, const char *key);
/* Index of a stored choice name, or the declaration's default when value is NULL/invalid. */
unsigned p4_mod_setting_choice(const P4ModSettingDecl *decl, const char *value);

/* The settings directory (the folder of p4pc.ini). NULL keeps values in memory
 * only, and every write is refused. Changing the root forgets cached files. */
int p4_mod_setting_store_root(const char *directory);
/* 1 and a copy of the stored text, 0 when absent, -1 for an invalid id/key or
 * an unreadable file (reason set). The first query of a mod reads its file. */
int p4_mod_setting_stored(const char *mod_id, const char *key, char *out, size_t capacity,
                          char *reason, size_t reason_capacity);
/* Writes the whole file to <file>.tmp and atomically replaces it before
 * returning. value NULL removes the key. On failure nothing in memory changes. */
int p4_mod_setting_store(const char *mod_id, const char *key, const char *value,
                         char *reason, size_t reason_capacity);

/* The admission-time ("boot") choice of a selected provider's declared key:
 * restart-bound settings keep it until the next start. 1 and the choice name,
 * 0 for an undeclared key or a provider without settings, -1 for bad arguments. */
int p4_mod_setting_boot(const P4PackageProvider *provider, const char *key, char *out, size_t capacity);
/* Registered through the native boot-provider list; owns the [setting] section. */
extern const P4PackageHandler p4_mod_settings_boot_provider;
#endif
