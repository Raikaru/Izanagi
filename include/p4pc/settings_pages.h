/* Virtual pages presented by Persona 4's native Config task. No UI toolkit,
 * package, scripting or host-pointer representation crosses into guest state. */
#ifndef P4PC_SETTINGS_PAGES_H
#define P4PC_SETTINGS_PAGES_H
#include <stddef.h>
#include <stdint.h>

#define P4_SETTINGS_EXTENSION_MAX 256
#define P4_SETTINGS_PAGE_MAX 1024
#define P4_SETTINGS_PAGE_ROWS_MAX 1048576
#define P4_SETTINGS_PAGE_KEY_MAX 128
#define P4_SETTINGS_PAGE_LABEL_MAX 192
#define P4_SETTINGS_PAGE_VALUE_MAX 128
#define P4_SETTINGS_PAGE_HELP_MAX 2048
#define P4_SETTINGS_PAGE_DETAIL_MAX 32768
#define P4_SETTINGS_PAGE_DEPTH_MAX 16

typedef enum P4SettingsPageRowKind {
    P4_SETTINGS_PAGE_INFO,
    P4_SETTINGS_PAGE_TOGGLE,
    P4_SETTINGS_PAGE_VALUE,
    P4_SETTINGS_PAGE_ACTION,
    P4_SETTINGS_PAGE_LINK
} P4SettingsPageRowKind;
enum P4SettingsPageRowFlags {
    P4_SETTINGS_PAGE_DISABLED = 1u,
    P4_SETTINGS_PAGE_RESTART = 2u,
    P4_SETTINGS_PAGE_CURRENT = 4u
};
typedef struct P4SettingsPageRow {
    const char *key; /* nonempty stable identity within the page */
    const char *label, *value, *help;
    P4SettingsPageRowKind kind;
    unsigned flags;
    int checked; /* toggle only; CURRENT marks the selected choice ACTION */
    const char *target; /* registered page key for LINK */
} P4SettingsPageRow;

typedef struct P4SettingsPage {
    const char *key, *title; /* immutable, borrowed until successful unregister */
    void *userdata;
    /* Count and immutable snapshot revision. Negative: report the reason with
     * p4_settings_message, then the native view is refused/closed. */
    int (*count)(void *userdata, uint64_t *revision);
    int (*row)(void *userdata, int index, P4SettingsPageRow *out);
    int (*find)(void *userdata, const char *row_key); /* index, -1 if removed */
    /* -1/+1 changes, 0 activates. Stable key, never a stale row index.
     * Return 0/-1; errors use p4_settings_message and never claim success. */
    int (*adjust)(void *userdata, const char *row_key, int direction);
    /* Detail pages have detail instead of count/row/find/adjust. The native
     * font measures and wraps the complete bounded body, then pages it. */
    const char *(*detail)(void *userdata, uint64_t *revision);
    int (*opened)(void *userdata); /* optional 0/-1 before the first query */
    void (*closed)(void *userdata); /* exactly once per successful opened view */
} P4SettingsPage;

/* All calls/callbacks are game-thread only. Copies the descriptor, not its
 * strings/userdata. Row/detail strings are valid until the next provider call:
 * native code copies them before another callback. Bounds include NUL;
 * oversized/malformed fields fail visibly rather than being truncated.
 * Display text is native_glyphs.h RAW encoding, not UTF-8 or MSG/BMD commands.
 * Captions are single-line; help/detail may use LF page-layer separators.
 * Each line is validated separately before passing it to the raw font creator. */
int p4_settings_page_register(const P4SettingsPage *page);
/* Refused inside callbacks. Otherwise closes affected views/descendants and
 * cancels pending references synchronously; userdata may then be released.
 * Deferred native fades draw owned snapshots, never provider pointers. */
int p4_settings_page_unregister(const char *key);
/* Accepted request is not visibility. Uses the existing asynchronous native
 * menu gates; no forced field/title entry. Missing/busy/invalid: -1. */
int p4_settings_page_open(const char *key);
int p4_settings_page_back(void);
int p4_settings_page_close(void);
const char *p4_settings_page_active(void); /* NULL outside a registered page */
#endif
