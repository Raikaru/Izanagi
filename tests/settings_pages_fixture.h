/* Permanent real-game fixture, included only by the existing menu harness.
 * Registered only for the explicit page or MODS-hub witness. */
#ifndef P4_SETTINGS_PAGES_FIXTURE_H
#define P4_SETTINGS_PAGES_FIXTURE_H
#include "p4pc/mod_settings.h"

enum { PF_HOME, PF_TOGGLES, PF_CATALOG, PF_CHOICES, PF_DETAIL, PF_DEEP, PF_EMPTY, PF_GONE, PF_BAD, PF_COUNT };
typedef struct PageFixtureContext { int kind; } PageFixtureContext;
static struct {
    int initialized, toggles[37], choice, shift, deadGone, deadCalls;
    int opened[PF_COUNT], closed[PF_COUNT], lastClosed[2], full, callbackUnregister, hubDetached;
    unsigned rowCalls, findCalls;
    uint64_t revision;
    PageFixtureContext context[PF_COUNT];
    char key[128], label[192], value[128], help[2048], target[128], body[8192];
} pageFixture;
static const char *const pageFixtureKeys[PF_COUNT] = {
    "test.pages", "test.toggles", "test.catalog", "test.choices", "test.detail",
    "test.deep", "test.empty", "test.gone", "test.bad"
};
static void pageFixtureScratch(void)
{
    /* Providers may reuse every returned buffer at their very next call. */
    strcpy(pageFixture.key, "provider-buffer-reused");
    strcpy(pageFixture.label, "PROVIDER BUFFER REUSED");
    strcpy(pageFixture.value, "REUSED");
    strcpy(pageFixture.help, "Provider buffer reused.");
    pageFixture.target[0] = '\0';
}
static int pageFixtureCount(void *userdata, uint64_t *revision)
{
    int kind = ((PageFixtureContext *)userdata)->kind;
    pageFixtureScratch();
    if (kind == PF_GONE && pageFixture.deadGone) ++pageFixture.deadCalls;
    *revision = pageFixture.revision;
    return kind == PF_HOME ? 8 : kind == PF_TOGGLES ? 37 :
        kind == PF_CATALOG ? P4_SETTINGS_PAGE_ROWS_MAX - pageFixture.shift :
        kind == PF_CHOICES ? 12 : kind == PF_DEEP ? 2 : kind == PF_EMPTY ? 0 : 1;
}
static int pageFixtureFind(void *userdata, const char *key)
{
    int kind = ((PageFixtureContext *)userdata)->kind;
    ++pageFixture.findCalls;
    int number = -1;
    const char *slash = strchr(key, '/');
    if (slash) number = atoi(slash + 1);
    pageFixtureScratch();
    return kind == PF_CATALOG ? number - pageFixture.shift : number;
}
static int pageFixtureRow(void *userdata, int index, P4SettingsPageRow *out)
{
    int kind = ((PageFixtureContext *)userdata)->kind;
    ++pageFixture.rowCalls;
    if (kind == PF_EMPTY) {
        p4_settings_message("An empty provider must not be queried for a row.");
        return -1;
    }
    pageFixtureScratch();
    if (kind == PF_GONE && pageFixture.deadGone) ++pageFixture.deadCalls;
    memset(out, 0, sizeof *out);
    snprintf(pageFixture.key, sizeof pageFixture.key, "row/%d", index + (kind == PF_CATALOG ? pageFixture.shift : 0));
    pageFixture.value[0] = '\0';
    strcpy(pageFixture.help, "Confirm: open. Back: previous page.");
    out->kind = P4_SETTINGS_PAGE_ACTION;
    if (kind == PF_HOME) {
        static const char *const labels[] = {"Toggle bank", "Virtual catalog", "Measured detail", "Choice editor",
            "Bounded back stack", "Empty catalog", "Unregister owner", "Provider error"};
        static const int targets[] = {PF_TOGGLES, PF_CATALOG, PF_DETAIL, PF_CHOICES, PF_DEEP, PF_EMPTY, PF_GONE, PF_BAD};
        snprintf(pageFixture.label, sizeof pageFixture.label, "%s", labels[index]);
        snprintf(pageFixture.target, sizeof pageFixture.target, "%s", pageFixtureKeys[targets[index]]);
        out->kind = P4_SETTINGS_PAGE_LINK;
    } else if (kind == PF_TOGGLES) {
        snprintf(pageFixture.label, sizeof pageFixture.label, "Toggle %02d", index + 1);
        out->kind = P4_SETTINGS_PAGE_TOGGLE;
        out->checked = pageFixture.toggles[index];
        strcpy(pageFixture.help, "A native toggle with stable identity. This help deliberately exceeds one retail help line, so its complete text must be measured and paged rather than clipped or replaced.");
        if (index == 36) out->flags = P4_SETTINGS_PAGE_RESTART;
    } else if (kind == PF_CATALOG) {
        snprintf(pageFixture.label, sizeof pageFixture.label, "Record %d", index + pageFixture.shift);
        strcpy(pageFixture.value, "VIEW");
    } else if (kind == PF_CHOICES) {
        snprintf(pageFixture.label, sizeof pageFixture.label, "Choice %d", index);
        if (index == pageFixture.choice) out->flags = P4_SETTINGS_PAGE_CURRENT;
        strcpy(pageFixture.help, "Confirm: commit and return.");
    } else if (kind == PF_DEEP) {
        strcpy(pageFixture.label, index ? "Refuse callback unregister" : "Open child");
        snprintf(pageFixture.value, sizeof pageFixture.value, "%d", pageFixture.opened[PF_DEEP] - pageFixture.closed[PF_DEEP]);
    } else if (kind == PF_GONE) {
        strcpy(pageFixture.label, "Open owned detail");
        strcpy(pageFixture.target, pageFixtureKeys[PF_DETAIL]);
        out->kind = P4_SETTINGS_PAGE_LINK;
    } else if (kind == PF_BAD) {
        if (pageFixture.opened[PF_BAD] == 1)
            memset(pageFixture.label, 'X', sizeof pageFixture.label); /* not terminated */
        else {
            strcpy(pageFixture.label, "Invalid raw help");
            strcpy(pageFixture.help, "Valid first line.\n\xc0\x80"); /* MSG style, not RAW */
        }
    }
    out->key = pageFixture.key;
    out->label = pageFixture.label;
    out->value = pageFixture.value;
    out->help = pageFixture.help;
    out->target = pageFixture.target;
    return 0;
}
static int pageFixtureAdjust(void *userdata, const char *key, int direction)
{
    int kind = ((PageFixtureContext *)userdata)->kind;
    int index = atoi(strchr(key, '/') + 1);
    pageFixtureScratch();
    if (kind == PF_TOGGLES) {
        pageFixture.toggles[index] = direction ? direction < 0 : !pageFixture.toggles[index];
        ++pageFixture.revision;
        return 0;
    }
    if (kind == PF_CHOICES) {
        pageFixture.choice = index;
        ++pageFixture.revision;
        return p4_settings_page_back();
    }
    if (kind == PF_CATALOG) return p4_settings_page_open(pageFixtureKeys[PF_DETAIL]);
    if (kind == PF_DEEP) {
        int result = index ? p4_settings_page_unregister(pageFixtureKeys[PF_DEEP]) :
            p4_settings_page_open(pageFixtureKeys[PF_DEEP]);
        if (result < 0) {
            if (index) ++pageFixture.callbackUnregister;
            else ++pageFixture.full;
        }
        return result;
    }
    return -1;
}
static const char *pageFixtureDetail(void *userdata, uint64_t *revision)
{
    (void)userdata;
    pageFixtureScratch();
    *revision = pageFixture.revision;
    return pageFixture.body;
}
static int pageFixtureOpened(void *userdata)
{
    ++pageFixture.opened[((PageFixtureContext *)userdata)->kind];
    pageFixtureScratch();
    return 0;
}
static void pageFixtureClosed(void *userdata)
{
    int kind = ((PageFixtureContext *)userdata)->kind;
    ++pageFixture.closed[kind];
    pageFixture.lastClosed[0] = pageFixture.lastClosed[1];
    pageFixture.lastClosed[1] = kind;
    pageFixtureScratch();
}
static void pageFixtureRoot(int direction, void *unused)
{
    (void)unused;
    if (!direction) p4_settings_page_open(pageFixtureKeys[PF_HOME]);
}
static void settingsPagesFixtureInit(void)
{
    int pages = getenv("P4PC_SETTINGS_PAGES_TEST") != NULL;
    int hub = getenv("P4PC_MOD_SETTINGS_TEST") != NULL;
    if (pageFixture.initialized || (!pages && !hub)) return;
    pageFixture.initialized = 1;
    pageFixture.revision = 1;
    static const char *const titles[PF_COUNT] = {"Native page substrate", "Thirty-seven native toggles",
        "Virtual catalog / one million rows", "Native choice editor", "Native measured detail",
        "Bounded native back stack", "Empty native catalog", "Unregister owning page", "Provider failure"};
    for (int i = 0; i < PF_COUNT; ++i) {
        pageFixture.context[i].kind = i;
        P4SettingsPage page = {.key = pageFixtureKeys[i], .title = titles[i], .userdata = &pageFixture.context[i],
            .opened = pageFixtureOpened, .closed = pageFixtureClosed};
        if (i == PF_DETAIL) page.detail = pageFixtureDetail;
        else {
            page.count = pageFixtureCount; page.row = pageFixtureRow;
            page.find = pageFixtureFind; page.adjust = pageFixtureAdjust;
        }
        if (p4_settings_page_register(&page)) exit(1);
    }
    static const P4SettingsEntry entry = {.label = "Page substrate", .adjust = pageFixtureRoot,
        .flags = P4_SETTINGS_ACTION, .description = "Open the native page regression fixture."};
    if (pages && p4_settings_register(&entry)) exit(1);
    size_t used = 0;
    for (int i = 0; i < 18; ++i) {
        int size = snprintf(pageFixture.body + used, sizeof pageFixture.body - used,
            "Paragraph %02d: This is the game's own font and native row layout. Long prose\x83\xf6is measured using its glyph widths and kerning, with no clipped words, borrowed pointers or alternate canvas.\n\n", i + 1);
        if (size < 0 || (size_t)size >= sizeof pageFixture.body - used) exit(1);
        used += (size_t)size;
    }
    if (hub && p4_mod_settings_attach_page(&pageFixture, P4_MOD_SETTINGS_SERVICE,
        "native-page-witness", pageFixtureKeys[PF_HOME], "C SERVICE SETTINGS")) exit(1);
}
static void settingsPagesFixtureCommand(const char *command)
{
    if (!pageFixture.initialized) exit(1);
    int result = 0;
    if (!strcmp(command, "request")) result = p4_settings_page_open(pageFixtureKeys[PF_HOME]);
    else if (!strcmp(command, "revise")) { pageFixture.shift = 1; ++pageFixture.revision; }
    else if (!strcmp(command, "unregister-deep")) result = p4_settings_page_unregister(pageFixtureKeys[PF_DEEP]);
    else if (!strcmp(command, "unregister-gone")) {
        result = p4_settings_page_unregister(pageFixtureKeys[PF_GONE]);
        pageFixture.deadGone = 1;
        memset(pageFixture.body, '!', strlen(pageFixture.body));
    } else if (!strcmp(command, "hub-detach")) {
        if (!getenv("P4PC_MOD_SETTINGS_TEST") ||
            p4_mod_settings_detach_owner(&pageFixture) != 1) exit(1);
        result = p4_settings_page_unregister(pageFixtureKeys[PF_HOME]);
        if (!result) result = p4_settings_page_unregister(pageFixtureKeys[PF_DETAIL]);
        pageFixture.hubDetached = !result;
    } else if (!strcmp(command, "close")) result = p4_settings_page_close();
    else exit(1);
    if (result) exit(1);
}
static int settingsPagesFixtureCheck(const char *name)
{
    if (!pageFixture.initialized) return 0;
    const char *active = p4_settings_page_active();
    int view, cursor = p4_settings_native_cursor(&view);
    if (!strcmp(name, "page-home")) return active && !strcmp(active, pageFixtureKeys[PF_HOME]);
    if (!strcmp(name, "page-toggle")) return active && !strcmp(active, pageFixtureKeys[PF_TOGGLES]) && pageFixture.toggles[0];
    if (!strcmp(name, "page-last-toggle")) return cursor == 36 && active && !strcmp(active, pageFixtureKeys[PF_TOGGLES]);
    if (!strcmp(name, "page-catalog-last")) return cursor == P4_SETTINGS_PAGE_ROWS_MAX - 1;
    if (!strcmp(name, "page-catalog-identity")) return cursor == P4_SETTINGS_PAGE_ROWS_MAX - 2 && pageFixture.findCalls > 0;
    if (!strcmp(name, "page-detail")) return active && !strcmp(active, pageFixtureKeys[PF_DETAIL]);
    if (!strcmp(name, "page-detail-paged")) return cursor >= 7 && active && !strcmp(active, pageFixtureKeys[PF_DETAIL]);
    if (!strcmp(name, "page-choice")) return pageFixture.choice == 8 && active && !strcmp(active, pageFixtureKeys[PF_HOME]);
    if (!strcmp(name, "page-depth")) return pageFixture.full == 1 && pageFixture.opened[PF_DEEP] == 14;
    if (!strcmp(name, "page-unregister-callback")) return pageFixture.callbackUnregister == 1;
    if (!strcmp(name, "page-unregister-deep")) return pageFixture.closed[PF_DEEP] == 14 && active && !strcmp(active, pageFixtureKeys[PF_HOME]);
    if (!strcmp(name, "page-empty")) return cursor == 0 && active && !strcmp(active, pageFixtureKeys[PF_EMPTY]);
    if (!strcmp(name, "page-unregister-gone")) return !pageFixture.deadCalls && pageFixture.lastClosed[0] == PF_DETAIL &&
        pageFixture.lastClosed[1] == PF_GONE && active && !strcmp(active, pageFixtureKeys[PF_HOME]);
    if (!strcmp(name, "page-error")) return pageFixture.opened[PF_BAD] == 1 && pageFixture.closed[PF_BAD] == 1 &&
        active && !strcmp(active, pageFixtureKeys[PF_HOME]);
    if (!strcmp(name, "page-error-glyphs")) return pageFixture.opened[PF_BAD] == 2 &&
        pageFixture.closed[PF_BAD] == 2 && active && !strcmp(active, pageFixtureKeys[PF_HOME]);
    if (!strcmp(name, "page-closed")) return !active && !p4_settings_active() &&
        pageFixture.opened[PF_HOME] == pageFixture.closed[PF_HOME] && !pageFixture.deadCalls &&
        pageFixture.rowCalls < 100000;
    if (!strcmp(name, "page-hub-detached")) return pageFixture.hubDetached &&
        pageFixture.closed[PF_HOME] == 1 && pageFixture.closed[PF_DETAIL] == 1 &&
        pageFixture.lastClosed[0] == PF_DETAIL && pageFixture.lastClosed[1] == PF_HOME &&
        active && !strcmp(active, "mods");
    return 0;
}
#endif
