#include "p4pc/game_elements_ui.h"
#include "p4pc/game_elements.h"
#include "p4pc/game_table_boot.h"
#include "p4pc/mod_setting_gate.h"
#include "p4pc/guest.h"
#include "p4pc/package_boot.h"
#include "p4pc/packages.h"
#include "p4pc/platform.h"
#include <stdio.h>
#include <string.h>

/* One selected provider may own the layout: [element-ui] with schema, revision
 * and order, plus [element-column KEY] for every ordered key. Any selected
 * provider may add [element-header KEY] images for leaves without a header
 * icon of their own and [element-prompt NAME] images (owner, key, revision of
 * its own [resource-image]).
 * Parsed from the core's immutable manifest fields; keys resolve against the
 * exact frozen Tables product. An expanded cohort with no authored layout uses
 * the retail seven-column row. Retail sessions stay inactive and allocate
 * nothing. */
typedef struct UiColumnInput {
    const char *leaf, *component, *channel, *marker_component, *marker_channel;
    const char *legacy_primary, *legacy_marker;
} UiColumnInput;
static struct ElementUiBoot {
    const P4PackageProvider *provider; /* NULL for the retail default layout. */
    unsigned line;
    uint32_t count, revision;
    char order[P4_ELEMENT_UI_MAX_COLUMNS][P4_TABLE_KEY_SIZE];
    UiColumnInput input[P4_ELEMENT_UI_MAX_COLUMNS];
    const P4GameTables *tables;
    P4GameElementUi *ui;
    int exposed;
} boot;

static void *guest_allocate(void *user, uint32_t size, uint32_t *guest)
{
    (void)user;
    void *host = p4pc_heap_malloc(size);
    if (host) *guest = P4_H2G(host);
    return host;
}
static void guest_release(void *user, uint32_t guest, void *host, uint32_t size)
{
    (void)user; (void)guest; (void)size;
    p4pc_heap_free(host);
}
static int refuse(const P4PackageProvider *provider, unsigned line, const char *reason)
{
    char message[256];
    if (line) snprintf(message, sizeof message, "element-ui line %u: %s", line, reason);
    else snprintf(message, sizeof message, "element-ui: %s", reason);
    p4_package_provider_refuse(provider, message);
    return -1; /* Return immediately even when policy scheduled a fresh candidate. */
}
static int decimal(const char *text, uint32_t *out)
{
    if (!text || !*text) return 0;
    uint32_t value = 0;
    do {
        unsigned char c = (unsigned char)*text++;
        if (c < '0' || c > '9' || value > (UINT32_MAX - (c - '0')) / 10u) return 0;
        value = value * 10u + c - '0';
    } while (*text);
    if (!value) return 0;
    *out = value;
    return 1;
}
static int space(char c) { return c == ' ' || c == '\t'; }
static int parse_order(const char *text)
{
    uint32_t count = 0;
    for (const char *p = text;;) {
        while (space(*p)) ++p;
        const char *start = p;
        while (*p && *p != ',') ++p;
        const char *end = p;
        while (end > start && space(end[-1])) --end;
        const size_t size = (size_t)(end - start);
        if (!size || size >= P4_TABLE_KEY_SIZE || count == P4_ELEMENT_UI_MAX_COLUMNS) return 0;
        memcpy(boot.order[count], start, size);
        boot.order[count][size] = 0;
        for (uint32_t i = 0; i < count; ++i)
            if (!strcmp(boot.order[i], boot.order[count])) return 0;
        ++count;
        if (!*p++) break; /* A trailing comma yields an empty key and refuses. */
    }
    boot.count = count;
    return 1;
}
static int component(const char *name, uint32_t *out)
{
    if (!strcmp(name, "damage")) *out = P4_ACTION_COMPONENT_DAMAGE;
    else if (!strcmp(name, "instant-ko")) *out = P4_ACTION_COMPONENT_INSTANT_KO;
    else if (!strcmp(name, "status")) *out = P4_ACTION_COMPONENT_STATUS;
    else return 0;
    return 1;
}
static int legacy(const char *name, uint32_t *selector)
{
    static const char *const names[7] = {"phys", "fire", "ice", "elec", "wind", "light-ko", "dark-ko"};
    for (uint32_t i = 0; i < 7u; ++i)
        if (!strcmp(name, names[i])) { *selector = i; return 1; }
    return 0;
}
/* Fields arrive sorted by (section, argument, key). */
static int parse_layout(const P4PackageProvider *provider, const P4PackageField *fields, size_t count)
{
    const char *schema = NULL, *revision = NULL, *order = NULL;
    for (size_t i = 0; i < count; ++i) {
        if (strcmp(fields[i].section, "element-ui")) continue;
        boot.line = fields[i].line;
        if (fields[i].argument[0]) return refuse(provider, boot.line, "[element-ui] takes no argument");
        if (!strcmp(fields[i].key, "schema")) schema = fields[i].value;
        else if (!strcmp(fields[i].key, "revision")) revision = fields[i].value;
        else if (!strcmp(fields[i].key, "order")) order = fields[i].value;
        else if (!strcmp(fields[i].key, P4_MOD_SETTING_GATE_FIELD) || !strcmp(fields[i].key, P4_MOD_COMPANION_GATE_FIELD) ||
                 !strcmp(fields[i].key, P4_MOD_COMPANION_SETTING_FIELD)) continue; /* evaluated by prepare() */
        else return refuse(provider, boot.line, "unknown [element-ui] field");
    }
    uint32_t schema_number;
    if (!decimal(schema, &schema_number) || schema_number != 1 || !decimal(revision, &boot.revision))
        return refuse(provider, boot.line, "requires schema=1 and a nonzero decimal revision");
    if (!order || !parse_order(order))
        return refuse(provider, boot.line, "order needs 1..9 distinct comma-separated element keys");
    for (size_t i = 0; i < count; ++i) {
        const P4PackageField *field = &fields[i];
        if (strcmp(field->section, "element-column")) continue;
        uint32_t index = 0;
        while (index < boot.count && strcmp(boot.order[index], field->argument)) ++index;
        if (index == boot.count) return refuse(provider, field->line, "[element-column] key is not in the order");
        UiColumnInput *input = &boot.input[index];
        input->leaf = boot.order[index];
        if (!strcmp(field->key, "component")) input->component = field->value;
        else if (!strcmp(field->key, "channel")) input->channel = field->value;
        else if (!strcmp(field->key, "marker-component")) input->marker_component = field->value;
        else if (!strcmp(field->key, "marker-channel")) input->marker_channel = field->value;
        else if (!strcmp(field->key, "legacy-primary")) input->legacy_primary = field->value;
        else if (!strcmp(field->key, "legacy-marker")) input->legacy_marker = field->value;
        else return refuse(provider, field->line, "unknown [element-column] field");
    }
    return 0;
}
static int compile_columns(P4GameElementUiColumn *columns)
{
    for (uint32_t i = 0; i < boot.count; ++i) {
        const UiColumnInput *input = &boot.input[i];
        P4GameElementUiColumn *column = &columns[i];
        memset(column, 0, sizeof *column);
        if (!input->leaf || !input->component || !input->channel)
            return refuse(boot.provider, boot.line, "every ordered key needs [element-column] component and channel");
        const P4ElementDesc *element = p4_game_tables_element_key(boot.tables, input->leaf);
        const P4AffinityChannelDesc *channel = p4_game_tables_channel_key(boot.tables, input->channel);
        if (!element || !channel) return refuse(boot.provider, boot.line, "element or channel key is not registered");
        if (!component(input->component, &column->component))
            return refuse(boot.provider, boot.line, "component must be damage, instant-ko or status");
        column->element_id = element->id;
        column->channel_id = channel->id;
        if (!input->marker_component != !input->marker_channel)
            return refuse(boot.provider, boot.line, "a KO marker needs both marker-component and marker-channel");
        if (input->marker_component) {
            const P4AffinityChannelDesc *marker = p4_game_tables_channel_key(boot.tables, input->marker_channel);
            if (!marker) return refuse(boot.provider, boot.line, "marker channel key is not registered");
            if (!component(input->marker_component, &column->marker_component))
                return refuse(boot.provider, boot.line, "marker-component must be instant-ko");
            column->marker_valid = 1;
            column->marker_channel_id = marker->id;
        }
        if (input->legacy_primary) {
            if (!legacy(input->legacy_primary, &column->legacy_primary_selector))
                return refuse(boot.provider, boot.line, "unknown legacy-primary selector");
            column->legacy_primary_valid = 1;
        }
        if (input->legacy_marker) {
            if (!legacy(input->legacy_marker, &column->legacy_marker_selector))
                return refuse(boot.provider, boot.line, "unknown legacy-marker selector");
            column->legacy_marker_valid = 1;
        }
    }
    return 0;
}
static int plan_failed(const char *stage, int result)
{
    char reason[192];
    snprintf(reason, sizeof reason, "%s: %s", stage, p4_game_tables_error(result));
    if (boot.provider) return refuse(boot.provider, boot.line, reason);
    fprintf(stderr, "p4pc: retail element-ui layout refused for the expanded cohort: %s\n", reason);
    return -1;
}
/* Every selected provider's [element-header KEY] and [element-prompt NAME]
 * images, each section gated by its own `when`; fields arrive sorted by
 * (section, argument, key). */
static int prompt_index(const char *name, uint32_t *prompt)
{
    if (!strcmp(name, "analyze-ailments")) *prompt = P4_ELEMENT_UI_PROMPT_ANALYZE_AILMENTS;
    else if (!strcmp(name, "analyze-affinities")) *prompt = P4_ELEMENT_UI_PROMPT_ANALYZE_AFFINITIES;
    else return 0;
    return 1;
}
static int element_images(int expanded)
{
    for (size_t package = 0; package < p4_packages_count(); ++package) {
        const P4PackageProvider *provider = p4_packages_provider(package);
        size_t count = 0;
        const P4PackageField *fields = p4_package_provider_fields(provider, &count);
        for (size_t first = 0; first < count;) {
            const char *section = fields[first].section;
            const int header = !strcmp(section, "element-header");
            if (!header && strcmp(section, "element-prompt")) { ++first; continue; }
            size_t last = first;
            while (last < count && !strcmp(fields[last].section, section) &&
                !strcmp(fields[last].argument, fields[first].argument)) ++last;
            const unsigned line = fields[first].line;
            char reason[192];
            const int gate = p4_mod_setting_section_gate(provider, fields + first, last - first, reason, sizeof reason);
            if (gate < 0) return refuse(provider, line, reason);
            if (gate) {
                if (!expanded) return refuse(provider, line, "element-header/element-prompt require an expanded element table-source cohort");
                const P4PackageInfo *info = p4_package_provider_info(provider);
                const char *owner = NULL, *key = NULL, *revision = NULL;
                for (size_t i = first; i < last; ++i) {
                    if (!strcmp(fields[i].key, "owner")) owner = fields[i].value;
                    else if (!strcmp(fields[i].key, "key")) key = fields[i].value;
                    else if (!strcmp(fields[i].key, "revision")) revision = fields[i].value;
                    else if (strcmp(fields[i].key, P4_MOD_SETTING_GATE_FIELD))
                        return refuse(provider, fields[i].line, "unknown element-header/element-prompt field");
                }
                P4ResourceRef image = {{0}, {0}, 0};
                if (!info || !owner || !key || strcmp(owner, info->id) || strlen(key) >= sizeof image.key ||
                    !decimal(revision, &image.revision))
                    return refuse(provider, line, "an image needs owner (this package), key and a nonzero revision");
                strcpy(image.owner, owner);
                strcpy(image.key, key);
                int result;
                if (header) {
                    const P4ElementDesc *element = p4_game_tables_element_key(boot.tables, fields[first].argument);
                    if (!element) return refuse(provider, line, "[element-header] key is not a registered element");
                    result = p4_game_elements_ui_element_header(boot.ui, element->id, &image);
                } else {
                    uint32_t prompt;
                    if (!prompt_index(fields[first].argument, &prompt))
                        return refuse(provider, line, "[element-prompt] is analyze-ailments or analyze-affinities");
                    result = p4_game_elements_ui_prompt(boot.ui, prompt, &image);
                }
                if (result == P4_TABLE_CONFLICT) return refuse(provider, line, "another package already declares this image");
                if (result) return refuse(provider, line, p4_game_tables_error(result));
                if (p4_package_provider_claim(provider, P4_PACKAGE_CLAIM_COSMETIC))
                    return refuse(provider, line, "cannot claim the selected element image");
            }
            first = last;
        }
    }
    return 0;
}

static int prepare(void *unused)
{
    (void)unused;
    if (boot.ui || boot.exposed) return -1;
    const void *tables_product = NULL, *images_product = NULL;
    if (p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &tables_product) ||
        p4_package_boot_product(P4_RESOURCES_IMAGES_HANDLER, &images_product)) return -1;
    const P4GameTablesBootProduct *root = tables_product;
    for (size_t package = 0; package < p4_packages_count(); ++package) {
        const P4PackageProvider *provider = p4_packages_provider(package);
        size_t count = 0;
        const P4PackageField *fields = p4_package_provider_fields(provider, &count);
        size_t i = 0;
        while (i < count && strcmp(fields[i].section, "element-ui") && strcmp(fields[i].section, "element-column")) ++i;
        if (i == count) continue;
        /* The layout's `when` and `companion` gate the whole layout, its columns included. */
        size_t first = 0;
        while (first < count && strcmp(fields[first].section, "element-ui")) ++first;
        size_t last = first;
        while (last < count && !strcmp(fields[last].section, "element-ui")) ++last;
        char reason[192];
        const int gate = p4_mod_setting_section_gate(provider, fields + first, last - first, reason, sizeof reason);
        if (gate < 0) return refuse(provider, first < count ? fields[first].line : fields[i].line, reason);
        if (!gate) continue;
        if (boot.provider) return refuse(provider, fields[i].line, "another provider already owns the element layout");
        boot.provider = provider;
        if (parse_layout(provider, fields, count)) return -1;
    }
    const int expanded = root && p4_game_elements_requested(root->tables);
    if (!expanded) {
        if (boot.provider) return refuse(boot.provider, boot.line, "a layout requires an expanded element table-source cohort");
        if (element_images(0)) return -1;
        return p4_package_boot_provide(NULL);
    }
    boot.tables = root->tables;
    P4GameElementUiColumn columns[P4_ELEMENT_UI_MAX_COLUMNS];
    const P4GameElementUiColumn *layout = p4_game_elements_ui_retail_columns;
    const char *owner = "p4";
    if (boot.provider) {
        if (compile_columns(columns)) return -1;
        const P4PackageInfo *info = p4_package_provider_info(boot.provider);
        if (!info) return refuse(boot.provider, boot.line, "layout provider is unavailable");
        layout = columns;
        owner = info->id;
    } else {
        boot.count = 7;
        boot.revision = 1;
    }
    const P4GameTableAllocator allocator = {NULL, guest_allocate, guest_release};
    boot.ui = p4_game_elements_ui_create(&allocator);
    if (!boot.ui) return plan_failed("layout", P4_TABLE_NOMEM);
    int result = p4_game_elements_ui_layout(boot.ui, owner, layout, boot.count, boot.revision);
    if (result) return plan_failed("layout", result);
    if (element_images(1)) return -1;
    result = p4_game_elements_ui_prepare(boot.ui, boot.tables);
    if (result) return plan_failed("element columns", result);
    result = p4_game_elements_ui_check_images(boot.ui, images_product);
    if (result) return plan_failed("authored element art", result);
    if (boot.provider && p4_package_provider_claim(boot.provider, P4_PACKAGE_CLAIM_COSMETIC))
        return refuse(boot.provider, boot.line, "cannot claim the selected layout");
    return p4_package_boot_provide(NULL); /* No early consumer; tokens do not exist yet. */
}
static void publish(void *unused)
{
    (void)unused;
    boot.exposed = 1;
}
static void rollback(void *unused)
{
    (void)unused;
    if (boot.exposed) p4_package_boot_runtime_failed("Element UI rollback attempted after native raw exposure.");
    p4_game_elements_ui_destroy(boot.ui); /* Early plan only: no pins or guest memory yet. */
    memset(&boot, 0, sizeof boot);
}
static int runtime_prepare(void *unused)
{
    (void)unused;
    const void *tables_product = NULL, *resources_product = NULL;
    if (p4_package_boot_product(P4_GAME_TABLES_BOOT_PROVIDER, &tables_product) ||
        p4_package_boot_product(P4_RESOURCES_IMAGES_HANDLER, &resources_product))
        return p4_package_boot_fail(NULL, "Element UI runtime dependencies are unavailable.");
    const P4GameTablesBootProduct *root = tables_product;
    if (!boot.ui) {
        if (root && p4_game_elements_requested(root->tables))
            return p4_package_boot_fail(NULL, "Expanded element cohort has no prepared element UI.");
        return p4_package_boot_provide(NULL);
    }
    if (!root || root->tables != boot.tables)
        return p4_package_boot_fail(NULL, "Element UI runtime tables differ from its CPU plan.");
    int result = p4_game_elements_ui_freeze(boot.ui, resources_product);
    if (result) return p4_package_boot_fail(boot.provider, "Element UI could not resolve its exact authored art.");
    result = p4_game_elements_ui_native_stage(boot.ui);
    if (result) return p4_package_boot_fail(NULL, p4_game_tables_error(result));
    return p4_package_boot_provide(boot.ui);
}
int p4_game_elements_ui_boot_session_end(void)
{
    /* Caller already proved task/queue/graphics quiescence and ended the
     * native snapshot; this only proves our exact model is no longer borrowed. */
    if (!p4_game_elements_ui_native_detached(boot.ui)) return P4_TABLE_CONFLICT;
    p4_game_elements_ui_destroy(boot.ui);
    memset(&boot, 0, sizeof boot);
    return P4_TABLE_OK;
}
static const char *const sections[] = {"element-ui", "element-column", "element-header", "element-prompt", NULL};
static const char *const dependencies[] = {P4_GAME_TABLES_BOOT_PROVIDER, P4_RESOURCES_IMAGES_HANDLER,
    P4_MOD_SETTINGS_HANDLER, NULL};
const P4PackageHandler p4_game_elements_ui_boot_provider = {
    .name = P4_GAME_ELEMENTS_UI_BOOT_PROVIDER, .sections = sections,
    .prepare = prepare, .publish = publish, .rollback = rollback,
    .boot_phase = P4_PACKAGE_BOOT_UI, .dependencies = dependencies,
    .runtime_prepare = runtime_prepare
};
