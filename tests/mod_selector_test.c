/* Exercise actual hub callbacks, configured discovery and atomic INI writes. */
#include "../src/platform/mods/mod_settings.c"

static const char *notice;
void p4_settings_message(const char *text) { notice = text; }
int p4_settings_page_open(const char *key) { return strcmp(key, "mods/installation"); }
int p4_settings_page_back(void) { return 0; }
int p4_settings_page_register(const P4SettingsPage *page) { (void)page; return 0; }
int p4_settings_page_unregister(const char *key) { (void)key; return 0; }
int p4_settings_register(const P4SettingsEntry *entry) { (void)entry; return 0; }

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "selector line %d: %s\n", __LINE__, #x); return 1; } } while (0)
static int select_id(const char *id)
{
    HubCatalog *catalog = catalogs[catalogDepth - 1];
    for (size_t i = 0; i < catalog->count; ++i) {
        if (catalog->items[i].valid && !strcmp(catalog->items[i].info.id, id)) {
            pending.catalog = catalog->serial; pending.index = i;
            return installation_opened(NULL);
        }
    }
    return -1;
}
int main(void)
{
    CHECK(hub_opened(NULL) == 0);
    HubCatalog *catalog = catalogs[catalogDepth - 1];
    CHECK(catalog->enabled.count == 2 && catalog->order.count == 2);
    CHECK(!strcmp(catalog->order.ids[0], "mod.low") && !strcmp(catalog->order.ids[1], "mod.high"));
    CHECK(select_id("mod.high") == 0);
    P4SettingsPageRow row = {0};
    CHECK(installation_row(NULL, 2, &row) == 0 && row.checked && !(row.flags & P4_SETTINGS_PAGE_DISABLED));
    CHECK(installation_adjust(NULL, "order", -1) == 0);
    CHECK(!strcmp(p4cfg_get("mods.load_order"), "mod.high,mod.low"));
    CHECK(notice && strstr(notice, "restart"));
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low"));
    CHECK(!strcmp(p4cfg_get("mods.load_order"), "mod.low"));
    CHECK(installation_row(NULL, 3, &row) == 0 && (row.flags & P4_SETTINGS_PAGE_DISABLED));
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high"));
    CHECK(!strcmp(p4cfg_get("mods.load_order"), "mod.low,mod.high"));
    installation_closed(NULL);
    CHECK(select_id("mod.duplicate") == 0);
    CHECK(installation_row(NULL, 2, &row) == 0 && (row.flags & P4_SETTINGS_PAGE_DISABLED));
    CHECK(installation_adjust(NULL, "enabled", 0) == -1);
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high"));
    installation_closed(NULL);
    CHECK(select_id("pack.single") == 0);
    CHECK(current_installation()->info.kind == P4_PACKAGE_ENGINE_PACK);
    CHECK(hub_row(NULL, (int)pending.index, &row) == 0 && !strcmp(row.label, "PACK pack.single"));
    CHECK(!strcmp(row.value, "DISABLED"));
    CHECK(installation_count(NULL, &(uint64_t){0}) == 4);
    CHECK(installation_row(NULL, 2, &row) == 0 && !row.checked &&
          !(row.flags & P4_SETTINGS_PAGE_DISABLED));
    CHECK(installation_find(NULL, "details") == 3 && installation_find(NULL, "order") == -1);
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    CHECK(!strcmp(p4cfg_get("packs.enabled"), "pack.single"));
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high"));
    CHECK(!strcmp(p4cfg_get("mods.load_order"), "mod.low,mod.high"));
    CHECK(installation_row(NULL, 2, &row) == 0 && row.checked);
    CHECK(notice && strstr(notice, "restart"));
    installation_closed(NULL);
    CHECK(select_id("pack.duplicate") == 0);
    CHECK(installation_row(NULL, 2, &row) == 0 && (row.flags & P4_SETTINGS_PAGE_DISABLED));
    CHECK(installation_adjust(NULL, "enabled", 0) == -1);
    CHECK(!strcmp(p4cfg_get("packs.enabled"), "pack.single"));
    installation_closed(NULL);
    /* Requirements: enabling a dependent enables its requirement in front of it,
     * writes the required setting and locks both rows; disabling only unlocks. */
    CHECK(select_id("mod.dep") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high,mod.base,mod.dep"));
    CHECK(!strcmp(p4cfg_get("mods.load_order"), "mod.low,mod.high,mod.base,mod.dep"));
    char value[32], reason[128];
    CHECK(p4_mod_setting_stored("mod.base", "split", value, sizeof value, reason, sizeof reason) == 1 &&
          !strcmp(value, "on"));
    CHECK(installation_adjust(NULL, "order", -1) == -1 && notice && strstr(notice, "must load first"));
    CHECK(!strcmp(p4cfg_get("mods.load_order"), "mod.low,mod.high,mod.base,mod.dep"));
    installation_closed(NULL);
    CHECK(select_id("mod.base") == 0);
    CHECK(installation_row(NULL, 2, &row) == 0 && row.checked && (row.flags & P4_SETTINGS_PAGE_DISABLED) &&
          !strcmp(row.label, "REQUIRED BY DEPENDENT"));
    CHECK(installation_adjust(NULL, "enabled", 0) == -1 && strstr(notice, "Required by Dependent"));
    int split = installation_find(NULL, "set:split");
    CHECK(split == 5 && installation_row(NULL, split, &row) == 0 && row.checked &&
          (row.flags & P4_SETTINGS_PAGE_DISABLED));
    CHECK(installation_adjust(NULL, "set:split", 1) == -1);
    CHECK(installation_adjust(NULL, "order", 1) == -1);
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high,mod.base,mod.dep"));
    installation_closed(NULL);
    CHECK(select_id("mod.dep") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high,mod.base"));
    installation_closed(NULL);
    CHECK(select_id("mod.base") == 0);
    CHECK(installation_row(NULL, 2, &row) == 0 && row.checked && !(row.flags & P4_SETTINGS_PAGE_DISABLED));
    CHECK(installation_row(NULL, 5, &row) == 0 && !(row.flags & P4_SETTINGS_PAGE_DISABLED));
    CHECK(installation_adjust(NULL, "set:split", 1) == 0);
    installation_closed(NULL);
    CHECK(select_id("mod.orphan") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == -1 && strstr(notice, "which is not installed"));
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high,mod.base"));
    installation_closed(NULL);
    /* Companions: never enabled or locked by the hub, but a newly enabled one
     * goes in front of its user and an incompatible one is refused. */
    CHECK(select_id("mod.user") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high,mod.base,mod.user"));
    installation_closed(NULL);
    CHECK(select_id("mod.comp") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high,mod.base,mod.user,mod.comp"));
    CHECK(!strcmp(p4cfg_get("mods.load_order"), "mod.low,mod.high,mod.base,mod.comp,mod.user"));
    CHECK(installation_row(NULL, 2, &row) == 0 && row.checked && !(row.flags & P4_SETTINGS_PAGE_DISABLED));
    CHECK(installation_adjust(NULL, "order", 1) == -1 && strstr(notice, "User loads after its companion Comp"));
    installation_closed(NULL);
    CHECK(select_id("mod.olduser") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == -1 &&
          strstr(notice, "Olduser needs companion Comp 3.0+; version 2.0 is installed"));
    installation_closed(NULL);
    CHECK(select_id("mod.comp") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    installation_closed(NULL);
    CHECK(select_id("mod.olduser") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    installation_closed(NULL);
    CHECK(select_id("mod.comp") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == -1 &&
          strstr(notice, "Olduser needs companion Comp 3.0+; version 2.0 is installed"));
    installation_closed(NULL);
    CHECK(select_id("mod.olduser") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    installation_closed(NULL);
    CHECK(select_id("mod.user") == 0);
    CHECK(installation_adjust(NULL, "enabled", 0) == 0);
    installation_closed(NULL);
    CHECK(!strcmp(p4cfg_get("mods.enabled"), "mod.low,mod.high,mod.base"));
    hub_closed(NULL);
    puts("mod selector callbacks and atomic INI: PASS");
    return 0;
}
