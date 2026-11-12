/* One native Config MODS hub. Catalogs and VM-owned pages share its widgets. */
#ifndef P4PC_MOD_SETTINGS_H
#define P4PC_MOD_SETTINGS_H
#ifdef __cplusplus
extern "C" {
#endif

typedef enum P4ModSettingsTarget {
    P4_MOD_SETTINGS_MOD,
    P4_MOD_SETTINGS_SERVICE
} P4ModSettingsTarget;

/* Game thread only. Idempotently registers the static root and C-owned pages;
 * never scans directories, prepares packages, starts a VM or opens a menu. */
int p4_mod_settings_init(void);

/* Register the target P4SettingsPage first. Copies id/page_key/native RAW label
 * (64/128/192 bytes INCLUDING NUL). owner is a non-NULL identity cookie, never
 * dereferenced or used as permanent root userdata. MOD id is the real player
 * mod id; SERVICE id names a C-owned service, not an installed package. Native
 * services cannot use the reserved lua/ page namespace. Duplicate page targets
 * are refused. Returns0/-1; failures never transfer ownership of the page.
 * Discovery and display do not activate a mod or qualify its payload. */
int p4_mod_settings_attach_page(const void *owner, P4ModSettingsTarget kind,
                               const char *id, const char *page_key,
                               const char *raw_label);
/* Unlinks and advances the hub revision synchronously; never unregisters a page
 * or invokes VM code. Returns0/-1 (unknown owner/page is-1). */
int p4_mod_settings_detach_page(const void *owner, const char *page_key);
/* Returns removed count, or-1 for NULL owner. Before releasing a VM: unlink its
 * pages here, then successfully unregister EVERY owned page outside provider
 * callbacks, and only then release Lua references/VM. Native page unregister
 * synchronously closes affected views and cancels pending references. */
int p4_mod_settings_detach_owner(const void *owner);

#ifdef __cplusplus
}
#endif
#endif
