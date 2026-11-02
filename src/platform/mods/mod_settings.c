#include "p4pc/mod_settings.h"
#include "p4pc/settings.h"
#include "p4pc/native_glyphs.h"
#include "p4pc/mods.h"
#include "p4pc/packs.h"
#include "p4pc/packages.h"
#include "p4pc/os.h"
#include "p4pc/crt.h"
#include "../config/p4cfg.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* All state is C-owned and game-thread-only. No catalog work occurs at startup
 * or from the Config root formatter; discovery belongs to opened(). */
typedef struct HubLink {
    struct HubLink *next;
    const void *owner;
    P4ModSettingsTarget kind;
    char id[64], page[128], label[192], key[32];
} HubLink;
typedef struct HubInstallation {
    P4PackageInfo info;
    int valid, active;
    char *diagnostic;
} HubInstallation;
typedef struct HubIds {
    size_t count;
    char ids[P4_PACKAGE_MAX][64];
} HubIds;
typedef struct HubCatalog {
    HubInstallation *items;
    size_t count, capacity;
    HubIds enabled, order, packs_enabled;
    P4PackageKind collecting_kind;
    uint64_t serial;
} HubCatalog;
typedef struct HubSelection {
    uint64_t catalog;
    size_t index;
} HubSelection;

static HubLink *links;
static unsigned linkCount;
static uint64_t revision = 1, nextLink = 1, nextCatalog = 1;
static int initialized;
static int restartManual;
static HubCatalog *catalogs[P4_SETTINGS_PAGE_DEPTH_MAX];
static unsigned catalogDepth, selectionDepth, detailDepth;
static HubSelection selections[P4_SETTINGS_PAGE_DEPTH_MAX], pending;
static char *details[P4_SETTINGS_PAGE_DEPTH_MAX];
static char rowKey[32], rowLabel[P4_SETTINGS_PAGE_LABEL_MAX], rowValue[P4_SETTINGS_PAGE_VALUE_MAX];

static int error(const char *text)
{
    p4_settings_message(text);
    return -1;
}
static int copy_text(char *out, size_t capacity, const char *text)
{
    if (!text) return -1;
    size_t size = 0;
    while (size < capacity && text[size]) ++size;
    if (size == capacity) return -1;
    memcpy(out, text, size + 1);
    return 0;
}
static int ids_find(const HubIds *list, const char *id)
{
    for (size_t i = 0; i < list->count; ++i)
        if (!strcmp(list->ids[i], id)) return (int)i;
    return -1;
}
static int ids_parse(HubIds *list, const char *text)
{
    list->count = 0;
    if (!text) return 0;
    while (*text) {
        while (isspace((unsigned char)*text)) ++text;
        const char *end = strchr(text, ',');
        if (!end) end = text + strlen(text);
        const char *last = end;
        while (last > text && isspace((unsigned char)last[-1])) --last;
        size_t n = (size_t)(last - text);
        if (!n || n >= sizeof list->ids[0] || list->count == P4_PACKAGE_MAX ||
            text[0] == '.' || text[n - 1] == '.') return -1;
        for (size_t i = 0; i < n; ++i)
            if (!((text[i] >= 'a' && text[i] <= 'z') || (text[i] >= '0' && text[i] <= '9') ||
                  text[i] == '-' || text[i] == '_' || text[i] == '.')) return -1;
        char *id = list->ids[list->count];
        memcpy(id, text, n); id[n] = 0;
        if (ids_find(list, id) >= 0) return -1;
        ++list->count;
        if (!*end) break;
        text = end + 1;
        if (!*text) return -1;
    }
    return 0;
}
static void ids_remove(HubIds *list, size_t index)
{
    for (size_t i = index + 1; i < list->count; ++i)
        memcpy(list->ids[i - 1], list->ids[i], sizeof list->ids[0]);
    --list->count;
}
static int ids_text(const HubIds *list, char out[600])
{
    size_t length = 0;
    for (size_t i = 0; i < list->count; ++i) {
        size_t n = strlen(list->ids[i]);
        if (length + (i != 0) + n >= 600) return -1;
        if (i) out[length++] = ',';
        memcpy(out + length, list->ids[i], n); length += n;
    }
    out[length] = 0;
    return 0;
}
static int save_selection(HubCatalog *catalog, const HubIds *enabled, const HubIds *order)
{
    char enabled_text[600], order_text[600];
    if (ids_text(enabled, enabled_text) || ids_text(order, order_text))
        return error("Mod selection exceeds settings capacity.");
    const P4CfgChange changes[] = {{"mods.enabled", enabled_text}, {"mods.load_order", order_text}};
    if (p4cfg_set_batch(changes, 2)) return error("Cannot save mod selection.");
    catalog->enabled = *enabled;
    catalog->order = *order;
    ++revision;
    p4_settings_message("Mod changes take effect after restart.");
    return 0;
}
static int save_packs_selection(HubCatalog *catalog, const HubIds *enabled)
{
    char enabled_text[600];
    if (ids_text(enabled, enabled_text)) return error("Pack selection exceeds settings capacity.");
    const P4CfgChange change = {"packs.enabled", enabled_text};
    if (p4cfg_set_batch(&change, 1)) return error("Cannot save pack selection.");
    catalog->packs_enabled = *enabled;
    ++revision;
    p4_settings_message("Pack changes take effect after restart.");
    return 0;
}
static const P4PackageInfo *unique_pack_info(const HubCatalog *catalog, const char *id)
{
    const P4PackageInfo *found = NULL;
    for (size_t i = 0; i < catalog->count; ++i) {
        const HubInstallation *item = &catalog->items[i];
        if (!item->valid || item->info.kind != P4_PACKAGE_ENGINE_PACK || strcmp(item->info.id, id)) continue;
        if (found) return NULL;
        found = &item->info;
    }
    return found;
}
static const P4PackageInfo *unique_info(const HubCatalog *catalog, const char *id)
{
    const P4PackageInfo *found = NULL;
    for (size_t i = 0; i < catalog->count; ++i) {
        const HubInstallation *item = &catalog->items[i];
        if (!item->valid || item->info.kind != P4_PACKAGE_MOD || strcmp(item->info.id, id)) continue;
        if (found) return NULL;
        found = &item->info;
    }
    return found;
}
static int default_before(const HubCatalog *catalog, const char *left, const char *right)
{
    const P4PackageInfo *a = unique_info(catalog, left), *b = unique_info(catalog, right);
    if (a && b) {
        if (a->priority != b->priority) return a->priority < b->priority;
        if (a->order != b->order) return a->order < b->order;
    } else if (a != NULL || b != NULL) return a != NULL;
    return strcmp(left, right) < 0;
}
static int load_selection(HubCatalog *catalog)
{
    HubIds configured;
    if (ids_parse(&catalog->enabled, p4cfg_get("mods.enabled")) ||
        ids_parse(&configured, p4cfg_get("mods.load_order")))
        return error("Invalid mod selection in settings.");
    if (ids_parse(&catalog->packs_enabled, p4cfg_get("packs.enabled")))
        return error("Invalid pack selection in settings.");
    catalog->order.count = 0;
    for (size_t i = 0; i < configured.count; ++i)
        if (ids_find(&catalog->enabled, configured.ids[i]) >= 0)
            memcpy(catalog->order.ids[catalog->order.count++], configured.ids[i], sizeof configured.ids[0]);
    size_t explicit_count = catalog->order.count;
    for (size_t i = 0; i < catalog->enabled.count; ++i) {
        const char *id = catalog->enabled.ids[i];
        if (ids_find(&catalog->order, id) >= 0) continue;
        size_t position = catalog->order.count++;
        while (position > explicit_count && default_before(catalog, id, catalog->order.ids[position - 1])) {
            memcpy(catalog->order.ids[position], catalog->order.ids[position - 1], sizeof catalog->order.ids[0]);
            --position;
        }
        memcpy(catalog->order.ids[position], id, sizeof catalog->order.ids[0]);
    }
    return 0;
}
static void catalog_free(HubCatalog *catalog)
{
    if (!catalog) return;
    for (size_t i = 0; i < catalog->count; ++i) free(catalog->items[i].diagnostic);
    free(catalog->items);
    free(catalog);
}
static HubCatalog *catalog_find(uint64_t serial)
{
    for (unsigned i = 0; i < catalogDepth; ++i)
        if (catalogs[i]->serial == serial) return catalogs[i];
    return NULL;
}
static HubInstallation *selected(HubSelection selection)
{
    HubCatalog *catalog = catalog_find(selection.catalog);
    return catalog && selection.index < catalog->count ? &catalog->items[selection.index] : NULL;
}
static int installation_active(const P4PackageInfo *info)
{
    for (size_t i = 0; i < p4_packages_count(); ++i) {
        const P4PackageInfo *active = p4_packages_info(i);
        if (active && active->kind == info->kind && !strcmp(active->id, info->id) &&
            !strcmp(active->directory, info->directory)) return 1;
    }
    return 0;
}
static int collect(const P4PackageInstallation *source, void *userdata)
{
    HubCatalog *catalog = userdata;
    if (catalog->count == P4_SETTINGS_PAGE_ROWS_MAX)
        return error("Too many installed packages.");
    if (catalog->count == catalog->capacity) {
        size_t capacity = catalog->capacity ? catalog->capacity * 2 : 16;
        if (capacity > P4_SETTINGS_PAGE_ROWS_MAX) capacity = P4_SETTINGS_PAGE_ROWS_MAX;
        if (capacity > SIZE_MAX / sizeof *catalog->items) return error("Package catalog is too large.");
        HubInstallation *items = realloc(catalog->items, capacity * sizeof *items);
        if (!items) return error("Package catalog allocation failed.");
        catalog->items = items;
        catalog->capacity = capacity;
    }
    HubInstallation item;
    memset(&item, 0, sizeof item);
    if (source->info) {
        item.info = *source->info;
        item.valid = 1;
        item.active = installation_active(source->info);
    } else if (copy_text(item.info.directory, sizeof item.info.directory, source->directory)) {
        return error("Package directory exceeds catalog bounds.");
    }
    item.info.kind = catalog->collecting_kind;
    if (source->diagnostic && source->diagnostic[0]) {
        size_t size = strlen(source->diagnostic);
        if (size >= P4_SETTINGS_PAGE_DETAIL_MAX / 4)
            return error("Package diagnostic exceeds native page bounds.");
        item.diagnostic = malloc(size + 1);
        if (!item.diagnostic) return error("Package diagnostic allocation failed.");
        memcpy(item.diagnostic, source->diagnostic, size + 1);
    }
    catalog->items[catalog->count++] = item;
    return 0;
}
static int hub_opened(void *unused)
{
    (void)unused;
    if (catalogDepth == P4_SETTINGS_PAGE_DEPTH_MAX) return error("MODS history is full.");
    HubCatalog *catalog = calloc(1, sizeof *catalog);
    if (!catalog) return error("Package catalog allocation failed.");
    catalog->collecting_kind = P4_PACKAGE_MOD;
    if (p4_mods_configured_installations_scan(collect, catalog)) {
        catalog_free(catalog);
        return error("Cannot read configured mod catalog.");
    }
    catalog->collecting_kind = P4_PACKAGE_ENGINE_PACK;
    if (p4_packs_configured_installations_scan(collect, catalog)) {
        catalog_free(catalog);
        return error("Cannot read configured pack catalog.");
    }
    if (load_selection(catalog)) { catalog_free(catalog); return -1; }
    catalog->serial = nextCatalog++;
    catalogs[catalogDepth++] = catalog;
    ++revision;
    return 0;
}
static void hub_closed(void *unused)
{
    (void)unused;
    if (catalogDepth) catalog_free(catalogs[--catalogDepth]);
    ++revision;
}
static int hub_count(void *unused, uint64_t *version)
{
    (void)unused;
    if (!catalogDepth) return -1;
    size_t count = catalogs[catalogDepth - 1]->count;
    for (HubLink *link = links; link; link = link->next)
        if (link->kind == P4_MOD_SETTINGS_SERVICE) ++count;
    if (count > P4_SETTINGS_PAGE_ROWS_MAX) return error("MODS catalog exceeds native row bounds.");
    *version = revision;
    return (int)count;
}
static HubLink *service_at(size_t index)
{
    for (HubLink *link = links; link; link = link->next)
        if (link->kind == P4_MOD_SETTINGS_SERVICE && index-- == 0) return link;
    return NULL;
}
static void link_row(HubLink *link, P4SettingsPageRow *out)
{
    out->key = link->key;
    out->label = link->label;
    out->value = "OPEN";
    out->help = "Open native settings.";
    out->kind = P4_SETTINGS_PAGE_LINK;
    out->target = link->page;
}
static int hub_row(void *unused, int index, P4SettingsPageRow *out)
{
    (void)unused;
    if (!catalogDepth || index < 0) return -1;
    HubCatalog *catalog = catalogs[catalogDepth - 1];
    if ((size_t)index >= catalog->count) {
        HubLink *link = service_at((size_t)index - catalog->count);
        if (!link) return -1;
        link_row(link, out);
        return 0;
    }
    HubInstallation *item = &catalog->items[index];
    snprintf(rowKey, sizeof rowKey, "i:%llx:%x", (unsigned long long)catalog->serial, (unsigned)index);
    const int pack = item->info.kind == P4_PACKAGE_ENGINE_PACK;
    if (item->valid)
        snprintf(rowLabel, sizeof rowLabel, "%s %s", pack ? "PACK" : "MOD", item->info.id);
    else
        snprintf(rowLabel, sizeof rowLabel, "INVALID %s %u", pack ? "PACK" : "MOD", (unsigned)index + 1);
    out->key = rowKey;
    out->label = rowLabel;
    out->value = !item->valid ? "INVALID" :
        ids_find(pack ? &catalog->packs_enabled : &catalog->enabled, item->info.id) >= 0 ? "ENABLED" : "DISABLED";
    out->help = "Open package information and settings.";
    out->kind = P4_SETTINGS_PAGE_ACTION;
    return 0;
}
static int hub_find(void *unused, const char *key)
{
    (void)unused;
    if (!catalogDepth) return -1;
    HubCatalog *catalog = catalogs[catalogDepth - 1];
    unsigned long long serial;
    unsigned index;
    int consumed = 0;
    if (sscanf(key, "i:%llx:%x%n", &serial, &index, &consumed) == 2 && !key[consumed] &&
        serial == catalog->serial && index < catalog->count) return (int)index;
    size_t position = catalog->count;
    for (HubLink *link = links; link; link = link->next) {
        if (link->kind != P4_MOD_SETTINGS_SERVICE) continue;
        if (!strcmp(link->key, key)) return position <= INT32_MAX ? (int)position : -1;
        ++position;
    }
    return -1;
}
static int hub_adjust(void *unused, const char *key, int direction)
{
    (void)unused;
    if (direction) return 0;
    int index = hub_find(NULL, key);
    if (index < 0 || !catalogDepth || (size_t)index >= catalogs[catalogDepth - 1]->count) return -1;
    pending.catalog = catalogs[catalogDepth - 1]->serial;
    pending.index = (size_t)index;
    return p4_settings_page_open("mods/installation");
}
static int installation_opened(void *unused)
{
    (void)unused;
    if (selectionDepth == P4_SETTINGS_PAGE_DEPTH_MAX || !selected(pending))
        return error("Selected package is no longer available.");
    selections[selectionDepth++] = pending;
    ++revision;
    return 0;
}
static void installation_closed(void *unused)
{
    (void)unused;
    if (selectionDepth) --selectionDepth;
    ++revision;
}
static HubInstallation *current_installation(void)
{
    return selectionDepth ? selected(selections[selectionDepth - 1]) : NULL;
}
static int owns_installation(HubLink *link, const HubInstallation *item)
{
    return item->valid && item->active && item->info.kind == P4_PACKAGE_MOD &&
        link->kind == P4_MOD_SETTINGS_MOD && !strcmp(link->id, item->info.id);
}
static int installation_count(void *unused, uint64_t *version)
{
    (void)unused;
    HubInstallation *item = current_installation();
    if (!item) return -1;
    int count = item->info.kind == P4_PACKAGE_ENGINE_PACK ? 4 : 5;
    for (HubLink *link = links; link; link = link->next)
        if (owns_installation(link, item)) ++count;
    *version = revision;
    return count;
}
static int installation_row(void *unused, int index, P4SettingsPageRow *out)
{
    (void)unused;
    HubInstallation *item = current_installation();
    if (!item || index < 0) return -1;
    if (item->info.kind == P4_PACKAGE_ENGINE_PACK) {
        static const char *keys[] = {"identity", "status", "enabled", "details"};
        HubCatalog *catalog = catalog_find(selections[selectionDepth - 1].catalog);
        if (!catalog || index >= 4) return -1;
        int usable = item->valid && unique_pack_info(catalog, item->info.id) == &item->info;
        out->key = keys[index];
        out->help = "Changes take effect after restart.";
        if (index == 0) {
            snprintf(rowLabel, sizeof rowLabel, "PACK %s", item->valid ? item->info.id : "INVALID METADATA");
            out->label = rowLabel;
            out->kind = P4_SETTINGS_PAGE_INFO;
        } else if (index == 1) {
            out->label = !item->valid ? "INVALID MANIFEST" : item->active ? "ACTIVE PACKAGE" : "NOT ACTIVE";
            out->kind = P4_SETTINGS_PAGE_INFO;
        } else if (index == 2) {
            out->label = "ENABLED AFTER RESTART";
            out->kind = P4_SETTINGS_PAGE_TOGGLE;
            out->checked = item->valid && ids_find(&catalog->packs_enabled, item->info.id) >= 0;
            out->flags = P4_SETTINGS_PAGE_RESTART | (usable ? 0 : P4_SETTINGS_PAGE_DISABLED);
            if (!usable) out->help = "Invalid or duplicate pack ID cannot be selected.";
        } else {
            out->label = "PACKAGE DETAILS";
            out->value = "VIEW";
            out->kind = P4_SETTINGS_PAGE_ACTION;
        }
        return 0;
    }
    if (index < 5) {
        static const char *keys[] = {"identity", "status", "enabled", "order", "details"};
        HubCatalog *catalog = catalog_find(selections[selectionDepth - 1].catalog);
        if (!catalog) return -1;
        int enabled = item->valid && ids_find(&catalog->enabled, item->info.id) >= 0;
        int position = item->valid ? ids_find(&catalog->order, item->info.id) : -1;
        int usable = item->valid && unique_info(catalog, item->info.id) == &item->info;
        out->key = keys[index];
        out->help = "Changes take effect after restart.";
        if (index == 0) {
            snprintf(rowLabel, sizeof rowLabel, "MOD %s",
                item->valid ? item->info.id : "INVALID METADATA");
            out->label = rowLabel;
            out->kind = P4_SETTINGS_PAGE_INFO;
        } else if (index == 1) {
            out->label = !item->valid ? "INVALID MANIFEST" : item->active ? "ACTIVE PACKAGE" : "NOT ACTIVE";
            out->kind = P4_SETTINGS_PAGE_INFO;
        } else if (index == 2) {
            out->label = "ENABLED AFTER RESTART";
            out->kind = P4_SETTINGS_PAGE_TOGGLE;
            out->checked = enabled;
            out->flags = P4_SETTINGS_PAGE_RESTART | (usable ? 0 : P4_SETTINGS_PAGE_DISABLED);
            if (!usable) out->help = "Invalid or duplicate mod ID cannot be selected.";
        } else if (index == 3) {
            out->label = "LOAD ORDER";
            if (position >= 0) snprintf(rowValue, sizeof rowValue, "%d / %u", position + 1, (unsigned)catalog->order.count);
            else snprintf(rowValue, sizeof rowValue, "DISABLED");
            out->value = rowValue;
            out->kind = P4_SETTINGS_PAGE_VALUE;
            out->flags = P4_SETTINGS_PAGE_RESTART | (usable && enabled ? 0 : P4_SETTINGS_PAGE_DISABLED);
            out->help = "Left or right moves this enabled mod. Later mods win.";
        } else {
            out->label = "PACKAGE DETAILS";
            out->value = "VIEW";
            out->kind = P4_SETTINGS_PAGE_ACTION;
        }
        return 0;
    }
    int position = 5;
    for (HubLink *link = links; link; link = link->next)
        if (owns_installation(link, item) && position++ == index) {
            link_row(link, out);
            return 0;
        }
    return -1;
}
static int installation_find(void *unused, const char *key)
{
    (void)unused;
    HubInstallation *item = current_installation();
    if (!item) return -1;
    if (item->info.kind == P4_PACKAGE_ENGINE_PACK) {
        if (!strcmp(key, "identity")) return 0;
        if (!strcmp(key, "status")) return 1;
        if (!strcmp(key, "enabled")) return 2;
        return !strcmp(key, "details") ? 3 : -1;
    }
    if (!strcmp(key, "identity")) return 0;
    if (!strcmp(key, "status")) return 1;
    if (!strcmp(key, "enabled")) return 2;
    if (!strcmp(key, "order")) return 3;
    if (!strcmp(key, "details")) return 4;
    int index = 5;
    for (HubLink *link = links; link; link = link->next)
        if (owns_installation(link, item)) {
            if (!strcmp(link->key, key)) return index;
            ++index;
        }
    return -1;
}
/* Metadata is byte text, not a license to send UTF-8/control bytes to the RAW
 * game font. Escape non-ASCII bytes losslessly; never truncate or switch fonts. */
static size_t escaped_size(const char *text)
{
    size_t size = 0;
    for (; *text; ++text) size += (unsigned char)*text >= 32 && (unsigned char)*text <= 126 ? 1 : 4;
    return size;
}
static char *append_escaped(char *out, const char *text)
{
    static const char hex[] = "0123456789ABCDEF";
    for (; *text; ++text) {
        unsigned char c = (unsigned char)*text;
        if (c >= 32 && c <= 126) *out++ = (char)c;
        else { *out++ = '\\'; *out++ = 'x'; *out++ = hex[c >> 4]; *out++ = hex[c & 15]; }
    }
    return out;
}
static int detail_opened(void *unused)
{
    (void)unused;
    HubInstallation *item = current_installation();
    if (!item || detailDepth == P4_SETTINGS_PAGE_DEPTH_MAX) return error("Package detail is unavailable.");
    const char *values[] = {
        item->info.kind == P4_PACKAGE_ENGINE_PACK ? "Engine pack" : "Player mod",
        item->valid ? item->info.id : "Invalid metadata",
        item->valid ? item->info.name : "Unavailable",
        item->valid ? item->info.version : "Unavailable",
        item->active ? "Active" : "Not active",
        item->info.directory,
        item->diagnostic ? item->diagnostic : "None"
    };
    static const char *labels[] = {"Kind: ", "ID: ", "Name: ", "Version: ", "State: ", "Directory: ", "Diagnostic: "};
    static const char suffix[] = "\nMetadata discovery does not activate a package or qualify its payload. Non-ASCII bytes are shown as \\xNN.";
    size_t size = sizeof suffix;
    for (unsigned i = 0; i < sizeof values / sizeof values[0]; ++i)
        size += strlen(labels[i]) + escaped_size(values[i]) + 1;
    if (size > P4_SETTINGS_PAGE_DETAIL_MAX) return error("Package detail exceeds native page bounds.");
    char *body = malloc(size);
    if (!body) return error("Package detail allocation failed.");
    char *out = body;
    for (unsigned i = 0; i < sizeof values / sizeof values[0]; ++i) {
        size_t labelSize = strlen(labels[i]);
        memcpy(out, labels[i], labelSize);
        out = append_escaped(out + labelSize, values[i]);
        *out++ = '\n';
    }
    memcpy(out, suffix, sizeof suffix);
    details[detailDepth++] = body;
    ++revision;
    return 0;
}
static void detail_closed(void *unused)
{
    (void)unused;
    if (detailDepth) free(details[--detailDepth]);
    ++revision;
}
static const char *detail_body(void *unused, uint64_t *version)
{
    (void)unused;
    *version = revision;
    return detailDepth ? details[detailDepth - 1] : NULL;
}
static int installation_adjust(void *unused, const char *key, int direction)
{
    (void)unused;
    HubInstallation *item = current_installation();
    if (!item || !selectionDepth) return -1;
    HubCatalog *catalog = catalog_find(selections[selectionDepth - 1].catalog);
    if (!catalog) return -1;
    if (!strcmp(key, "details")) return direction ? 0 : p4_settings_page_open("mods/details");
    if (item->info.kind == P4_PACKAGE_ENGINE_PACK) {
        if (!item->valid || unique_pack_info(catalog, item->info.id) != &item->info) return -1;
        if (strcmp(key, "enabled")) return 0;
        HubIds enabled = catalog->packs_enabled;
        int found = ids_find(&enabled, item->info.id);
        if (found >= 0) ids_remove(&enabled, (size_t)found);
        else {
            if (enabled.count == P4_PACKAGE_MAX) return error("Too many enabled packs.");
            strcpy(enabled.ids[enabled.count++], item->info.id);
        }
        return save_packs_selection(catalog, &enabled);
    }
    if (!item->valid || unique_info(catalog, item->info.id) != &item->info) return -1;
    HubIds enabled = catalog->enabled, order = catalog->order;
    int found = ids_find(&enabled, item->info.id);
    if (!strcmp(key, "enabled")) {
        if (found >= 0) {
            ids_remove(&enabled, (size_t)found);
            int position = ids_find(&order, item->info.id);
            if (position >= 0) ids_remove(&order, (size_t)position);
        } else {
            if (enabled.count == P4_PACKAGE_MAX || order.count == P4_PACKAGE_MAX)
                return error("Too many enabled mods.");
            strcpy(enabled.ids[enabled.count++], item->info.id);
            strcpy(order.ids[order.count++], item->info.id);
        }
        return save_selection(catalog, &enabled, &order);
    }
    if (!strcmp(key, "order") && direction && found >= 0) {
        int position = ids_find(&order, item->info.id);
        int other = position + (direction < 0 ? -1 : 1);
        if (position < 0 || other < 0 || (size_t)other >= order.count) return 0;
        char temp[64];
        memcpy(temp, order.ids[position], sizeof temp);
        memcpy(order.ids[position], order.ids[other], sizeof temp);
        memcpy(order.ids[other], temp, sizeof temp);
        return save_selection(catalog, &enabled, &order);
    }
    return 0;
}
static int restart_opened(void *unused)
{
    (void)unused;
    restartManual = 0;
    ++revision;
    return 0;
}
static int restart_count(void *unused, uint64_t *version)
{
    (void)unused;
    *version = revision;
    return 3;
}
static const char *const restartKeys[] = {"notice", "restart", "later"};
static int restart_find(void *unused, const char *key)
{
    (void)unused;
    for (int i = 0; i < 3; ++i)
        if (!strcmp(key, restartKeys[i])) return i;
    return -1;
}
static int restart_row(void *unused, int index, P4SettingsPageRow *out)
{
    (void)unused;
    if (index < 0 || index >= 3) return -1;
    out->key = restartKeys[index];
    out->flags = P4_SETTINGS_PAGE_RESTART;
    if (index == 0) {
        out->kind = P4_SETTINGS_PAGE_INFO;
        out->label = restartManual ? "RESTART MANUALLY" : "APPLIES AFTER RESTART";
        out->help = "Current packages stay loaded until this process exits.";
    } else {
        out->kind = P4_SETTINGS_PAGE_ACTION;
        out->value = "OK";
        out->label = index == 1 ? "RESTART NOW" : "LATER";
        out->help = index == 1 ? "Close safely and start a fresh process." :
            "Keep playing with the current packages.";
        if (index == 1 && restartManual) out->flags |= P4_SETTINGS_PAGE_DISABLED;
    }
    if (restartManual) out->help = "Automatic restart is unavailable. Settings are saved; quit and start the game again.";
    return 0;
}
static int restart_adjust(void *unused, const char *key, int direction)
{
    (void)unused;
    if (direction) return 0;
    if (!strcmp(key, "later")) return p4_settings_page_back();
    if (strcmp(key, "restart")) return 0;
    if (!p4_os_request_restart("The enabled package selection or load order changed."))
        p4pc_exit(EXIT_SUCCESS); /* Registered OS tail runs after ordinary teardown. */
    restartManual = 1;
    ++revision;
    p4_settings_message("Settings saved. Quit and start the game again.");
    return 0;
}
static void root_format(char *value, size_t capacity, void *unused)
{
    (void)unused;
    if (capacity) snprintf(value, capacity, "OPEN");
}
static void root_adjust(int direction, void *unused)
{
    (void)unused;
    if (!direction && p4_settings_page_open("mods")) p4_settings_message("Cannot open MODS.");
}
int p4_mod_settings_init(void)
{
    if (initialized) return 0;
    const P4SettingsPage pages[] = {
        {.key="mods", .title="MODS", .count=hub_count, .row=hub_row, .find=hub_find,
         .adjust=hub_adjust, .opened=hub_opened, .closed=hub_closed},
        {.key="mods/installation", .title="PACKAGE SETTINGS", .count=installation_count,
         .row=installation_row, .find=installation_find, .adjust=installation_adjust,
         .opened=installation_opened, .closed=installation_closed},
        {.key="mods/details", .title="PACKAGE INFORMATION", .detail=detail_body,
         .opened=detail_opened, .closed=detail_closed},
        {.key="mods/restart", .title="RESTART REQUIRED", .count=restart_count,
         .row=restart_row, .find=restart_find, .adjust=restart_adjust, .opened=restart_opened}
    };
    size_t registered = 0;
    for (; registered < sizeof pages / sizeof pages[0]; ++registered)
        if (p4_settings_page_register(&pages[registered])) break;
    if (registered != sizeof pages / sizeof pages[0]) {
        while (registered) p4_settings_page_unregister(pages[--registered].key);
        return error("Cannot register native MODS pages.");
    }
    const P4SettingsEntry entry = {.label="MODS", .format=root_format, .adjust=root_adjust,
        .flags=P4_SETTINGS_ACTION, .description="Installed mods and settings."};
    if (p4_settings_register(&entry)) {
        while (registered) p4_settings_page_unregister(pages[--registered].key);
        return error("Cannot register MODS in Config.");
    }
    initialized = 1;
    return 0;
}
int p4_mod_settings_attach_page(const void *owner, P4ModSettingsTarget kind,
                               const char *id, const char *page_key, const char *raw_label)
{
    HubLink candidate;
    memset(&candidate, 0, sizeof candidate);
    if (!owner || (kind != P4_MOD_SETTINGS_MOD && kind != P4_MOD_SETTINGS_SERVICE) ||
        !id || !id[0] || copy_text(candidate.id, sizeof candidate.id, id) ||
        !page_key || !page_key[0] || copy_text(candidate.page, sizeof candidate.page, page_key) ||
        !raw_label || !raw_label[0] || copy_text(candidate.label, sizeof candidate.label, raw_label) ||
        p4_native_glyphs_validate((const uint8_t *)candidate.label, strlen(candidate.label) + 1, P4_GLYPHS_RAW, 0) ||
        (kind == P4_MOD_SETTINGS_SERVICE && !strncmp(page_key, "lua/", 4))) return -1;
    for (HubLink *link = links; link; link = link->next)
        if (!strcmp(link->page, page_key)) return -1;
    if (linkCount == P4_SETTINGS_PAGE_MAX || p4_mod_settings_init()) return -1;
    HubLink *link = malloc(sizeof *link);
    if (!link) return -1;
    *link = candidate;
    link->owner = owner;
    link->kind = kind;
    snprintf(link->key, sizeof link->key, "s:%llx", (unsigned long long)nextLink++);
    HubLink **tail = &links;
    while (*tail) tail = &(*tail)->next;
    *tail = link;
    ++linkCount;
    ++revision;
    return 0;
}
int p4_mod_settings_detach_page(const void *owner, const char *page_key)
{
    if (!owner || !page_key) return -1;
    for (HubLink **slot = &links; *slot; slot = &(*slot)->next)
        if ((*slot)->owner == owner && !strcmp((*slot)->page, page_key)) {
            HubLink *link = *slot;
            *slot = link->next;
            free(link);
            --linkCount;
            ++revision;
            return 0;
        }
    return -1;
}
int p4_mod_settings_detach_owner(const void *owner)
{
    if (!owner) return -1;
    int count = 0;
    for (HubLink **slot = &links; *slot;) {
        HubLink *link = *slot;
        if (link->owner != owner) { slot = &link->next; continue; }
        *slot = link->next;
        free(link);
        --linkCount;
        ++count;
    }
    if (count) ++revision;
    return count;
}
