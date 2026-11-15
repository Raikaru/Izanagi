#include "p4pc/game_tables.h"
#include "p4pc/game_table_use.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static P4ElementDesc leaf(uint32_t id, const char *key, uint32_t order)
{
    P4ElementDesc d = {0};
    d.id = id; d.family = 0; d.order = order;
    d.flags = P4_ELEMENT_PHYSICAL | P4_ELEMENT_OFFENSIVE | P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE;
    strcpy(d.key, key); strcpy(d.display_name, "Localized physical leaf"); strcpy(d.short_name, "Leaf");
    strcpy(d.icon.owner, "art"); strcpy(d.icon.key, "art:leaf"); d.icon.revision = 123; d.revision = 7;
    return d;
}
static struct RuntimeRowProbe {
    const P4GameTables *view;
    uint32_t domain, ids[5], calls;
    int result;
} row_probe;
static int runtime_row_probe(const P4GameTables *view, uint32_t domain, const uint32_t ids[5], void *context)
{
    assert(context == &row_probe && view == row_probe.view && domain == row_probe.domain);
    assert(!memcmp(ids, row_probe.ids, sizeof row_probe.ids));
    ++row_probe.calls;
    return row_probe.result; /* Canonical dispatch only, not source authority. */
}
static void runtime_row_transport(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    row_probe = (struct RuntimeRowProbe){.view = tables};
    P4ElementDesc element = leaf(UINT32_MAX, "transport:leaf", UINT32_MAX);
    const P4AffinityChannelDesc channel = {.id = UINT32_MAX, .kind = P4_CHANNEL_PERCENT, .revision = 3,
        .key = "transport:channel", .display_name = "Transport channel"};
    const P4AffinityRow affinity = {P4_ENTITY_ENEMY, 0x80000000u, UINT32_MAX,
        UINT32_MAX, UINT32_MAX, P4_AFF_RESIST | 50u, 50};
    const P4ActionElementBinding action = {P4_ACTION_ELEMENT_WEAPON, 0x80000000u,
        P4_ACTION_COMPONENT_DAMAGE, UINT32_MAX, UINT32_MAX, 1, 2, "transport:action"};
    const P4AffinityProfileAlias alias = {P4_ENTITY_ENEMY, 0x80000000u, 65535u,
        UINT32_MAX, 0x80000000u, 1, 2, "transport:alias"};
    const P4AffinityEffectBinding effect = {P4_AFFINITY_EFFECT_SKILL, UINT32_MAX,
        UINT32_MAX, UINT32_MAX, P4_AFF_RESIST, 1, 2, "transport:effect"};
    assert(p4_game_tables_validate_runtime_sources(tables) == P4_TABLE_NOT_READY);
    assert(p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_ELEMENT, &element) == P4_TABLE_NOT_READY);
    assert(!p4_game_tables_element(tables, "transport", &element));
    assert(!p4_game_tables_channel(tables, "transport", &channel));
    assert(!p4_game_tables_affinity(tables, "transport", &affinity));
    assert(!p4_game_tables_action_element(tables, "transport", &action));
    assert(!p4_game_tables_affinity_alias(tables, "transport", &alias));
    assert(!p4_game_tables_affinity_effect(tables, "transport", &effect));
    assert(!p4_game_tables_bind_runtime_use(tables, runtime_row_probe, &row_probe));
    assert(p4_game_tables_bind_runtime_use(tables, runtime_row_probe, tables) == P4_TABLE_CONFLICT);
    assert(!p4_game_tables_freeze(tables, 1) && !row_probe.calls);
    assert(!p4_game_tables_validate_runtime_sources(tables));
    const void *rows[] = {p4_game_tables_find_element(tables, UINT32_MAX),
        p4_game_tables_find_channel(tables, UINT32_MAX),
        p4_game_tables_find_affinity(tables, P4_ENTITY_ENEMY, 0x80000000u, UINT32_MAX, UINT32_MAX, UINT32_MAX),
        p4_game_tables_find_action_element(tables, P4_ACTION_ELEMENT_WEAPON, 0x80000000u, P4_ACTION_COMPONENT_DAMAGE),
        p4_game_tables_find_affinity_alias(tables, P4_ENTITY_ENEMY, 0x80000000u, 65535u),
        p4_game_tables_find_affinity_effect(tables, P4_AFFINITY_EFFECT_SKILL, UINT32_MAX, UINT32_MAX, UINT32_MAX)};
    static const uint32_t selectors[][5] = {{UINT32_MAX}, {UINT32_MAX},
        {P4_ENTITY_ENEMY, 0x80000000u, UINT32_MAX, UINT32_MAX, UINT32_MAX},
        {P4_ACTION_ELEMENT_WEAPON, 0x80000000u, P4_ACTION_COMPONENT_DAMAGE},
        {P4_ENTITY_ENEMY, 0x80000000u, 65535u},
        {P4_AFFINITY_EFFECT_SKILL, UINT32_MAX, UINT32_MAX, UINT32_MAX}};
    for (uint32_t i = 0; i < sizeof rows / sizeof rows[0]; ++i) {
        assert(rows[i]);
        row_probe.domain = i + 1; memcpy(row_probe.ids, selectors[i], sizeof row_probe.ids);
        row_probe.result = 0;
        assert(!p4_game_tables_runtime_use_row(tables, i + 1, rows[i]));
        row_probe.result = 1;
        assert(p4_game_tables_runtime_use_row(tables, i + 1, rows[i]) == P4_TABLE_INVALID);
        row_probe.result = P4_TABLE_CONFLICT;
        assert(p4_game_tables_runtime_use_row(tables, i + 1, rows[i]) == P4_TABLE_CONFLICT);
    }
    assert(row_probe.calls == 18);
    assert(!p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_ELEMENT, p4_game_tables_find_element(tables, 0)));
    assert(!p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_CHANNEL, p4_game_tables_find_channel(tables, 0)));
    element = *(const P4ElementDesc *)rows[0];
    assert(p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_ELEMENT, &element) == P4_TABLE_INVALID);
    assert(p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_ELEMENT, rows[3]) == P4_TABLE_INVALID);
    assert(p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_ELEMENT, (const uint8_t *)rows[0] + 1) == P4_TABLE_INVALID);
    assert(p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_STRING, rows[0]) == P4_TABLE_INVALID);
    assert(row_probe.calls == 18);
    p4_game_tables_destroy(tables);
}
static void stable_image_references(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    P4ElementDesc descriptor = leaf(19, "image-test:leaf", 19), invalid = descriptor;
    invalid.icon.revision = 0;
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    invalid = descriptor; invalid.icon.owner[0] = 0;
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    invalid = descriptor; strcpy(invalid.icon.key, "other:leaf");
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    invalid = descriptor; strcpy(invalid.icon.owner, ".art"); strcpy(invalid.icon.key, ".art:leaf");
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    invalid = descriptor; strcpy(invalid.icon.key, "art:.leaf");
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    invalid = descriptor; strcpy(invalid.icon.key, "art:leaf.");
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    invalid = descriptor; memset(invalid.icon.owner, 'x', sizeof invalid.icon.owner);
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    invalid = descriptor; memset(invalid.icon.key, 'x', sizeof invalid.icon.key);
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    /* The optional header icon follows the same reference rules. */
    invalid = descriptor; invalid.header_icon = descriptor.icon; invalid.header_icon.revision = 0;
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    invalid = descriptor; invalid.header_icon = descriptor.icon; strcpy(invalid.header_icon.key, "other:leaf");
    assert(p4_game_tables_element(tables, "image-test", &invalid) == P4_TABLE_INVALID);
    descriptor.header_icon = descriptor.icon; strcpy(descriptor.header_icon.key, "art:leaf-header");
    assert(p4_game_tables_element(tables, "image-test", &descriptor) == P4_TABLE_OK);
    descriptor.icon.owner[0] = descriptor.icon.key[0] = 'x';
    P4ElementDesc absent = leaf(20, "image-test:no-image", 20); absent.icon = (P4ResourceRef){0};
    assert(p4_game_tables_element(tables, "image-test", &absent) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    const P4ResourceRef *reference = &p4_game_tables_find_element(tables, 19)->icon;
    assert(!strcmp(reference->owner, "art") && !strcmp(reference->key, "art:leaf") && reference->revision == 123);
    reference = &p4_game_tables_find_element(tables, 19)->header_icon;
    assert(!strcmp(reference->owner, "art") && !strcmp(reference->key, "art:leaf-header") && reference->revision == 123);
    reference = &p4_game_tables_find_element(tables, 20)->icon;
    assert(!reference->owner[0] && !reference->key[0] && !reference->revision);
    assert(!p4_game_tables_find_element(tables, 20)->header_icon.owner[0]);
    p4_game_tables_destroy(tables);
}
static uint32_t retail_decode(uint16_t raw)
{
    /* Literal arithmetic/branch order of datCalcSkillSlotValue0023a620. */
    uint32_t result = ((uint32_t)(raw & 0xff00) << 16) | ((raw & 0xff) * 5);
    if ((result & 0xffff) == 0) {
        if (result & 0x08000000) result |= 0x7d;
        else if (result & 0x27000000) result |= 0x64;
        else if (result & 0x10000000) result |= 0x32;
    }
    return result;
}
static void retail(void)
{
    for (uint32_t value = 0; value <= 65535; ++value)
        assert(p4_affinity_decode((uint16_t)value) == retail_decode((uint16_t)value));
    assert(p4_affinity_decode(0) == 0);
    assert(p4_affinity_decode(0x0800) == (P4_AFF_WEAK | 125));
    P4GameTables *t = p4_game_tables_create(); assert(t);
    assert(p4_game_tables_revision(t) == 0);
    assert(p4_game_tables_element_count(t) == 19);
    assert(!p4_game_elements_requested(NULL) && !p4_game_elements_requested(t));
    assert(p4_game_tables_freeze(t, 1) == P4_TABLE_OK);
    assert(p4_game_tables_revision(t) == 1);
    assert(!p4_game_elements_requested(t));
    unsigned visible = 0;
    for (uint32_t id = 0; id < 19; ++id) {
        const P4ElementDesc *d = p4_game_tables_find_element(t, id); assert(d && d->id == id);
        assert(p4_game_tables_element_key(t, d->key) == d);
        assert(p4_game_tables_element_at(t, id) == d);
        visible += !!(d->flags & P4_ELEMENT_VISIBLE);
    }
    assert(visible == 7);
    assert(!p4_game_tables_find_element(t, 19));
    assert(!p4_game_tables_element_at(t, 19));
    assert(!p4_game_tables_find_affinity(t, P4_ENTITY_PERSONA, 1, 0, 0, P4_CHANNEL_DAMAGE));
    p4_game_tables_destroy(t);
}
static void expansion(void)
{
    P4GameTables *t = p4_game_tables_create(); assert(t);
    P4ElementDesc slash = leaf(19, "split:slash", 0), strike = leaf(20, "split:strike", 1), pierce = leaf(21, "split:pierce", 2);
    assert(p4_game_tables_element(t, "split", &pierce) == P4_TABLE_OK);
    assert(p4_game_tables_element(t, "split", &slash) == P4_TABLE_OK);
    assert(p4_game_tables_element(t, "split", &strike) == P4_TABLE_OK);
    assert(p4_game_tables_element(t, "other", &slash) == P4_TABLE_CONFLICT);
    P4ElementDesc same_key = slash; same_key.id = 30;
    assert(p4_game_tables_element(t, "split", &same_key) == P4_TABLE_CONFLICT);
    P4ElementDesc wrong_owner = leaf(31, "other:leaf", 3);
    assert(p4_game_tables_element(t, "split", &wrong_owner) == P4_TABLE_INVALID);
    P4ElementDesc hidden = *p4_game_tables_find_element(t, 0); hidden.flags &= ~P4_ELEMENT_VISIBLE;
    assert(p4_game_tables_element(t, "split", &hidden) == P4_TABLE_OK);
    assert(p4_game_tables_element(t, "other", &hidden) == P4_TABLE_CONFLICT);
    P4AffinityRow row = {P4_ENTITY_ENEMY, 300, 5, 19, P4_CHANNEL_DAMAGE, P4_AFF_WEAK | 125, 125};
    assert(p4_game_tables_affinity(t, "split", &row) == P4_TABLE_OK);
    assert(p4_game_tables_affinity(t, "other", &row) == P4_TABLE_CONFLICT);
    row.entity_id = 301; row.value = P4_AFF_NULL | 100;
    assert(p4_game_tables_affinity(t, "split", &row) == P4_TABLE_OK);
    row.entity_id = 300; row.profile_id = 6; row.value = P4_AFF_REPEL | 100;
    assert(p4_game_tables_affinity(t, "split", &row) == P4_TABLE_OK);
    row.profile_id = 5; row.element_id = 6; row.value = P4_AFF_WEAK | 125;
    assert(p4_game_tables_affinity(t, "lightdark", &row) == P4_TABLE_OK);
    row.channel_id = P4_CHANNEL_INSTANT_KO; row.value = P4_KO_NULL; row.scalar = 0;
    assert(p4_game_tables_affinity(t, "lightdark", &row) == P4_TABLE_OK);
    P4AffinityChannelDesc custom = {0}; custom.id = 20; custom.kind = P4_CHANNEL_PERCENT;
    strcpy(custom.key, "custom:channel"); strcpy(custom.display_name, "Custom channel");
    assert(p4_game_tables_channel(t, "custom", &custom) == P4_TABLE_OK);
    assert(p4_game_tables_channel(t, "custom", &custom) == P4_TABLE_CONFLICT);
    assert(p4_game_tables_freeze(t, 42) == P4_TABLE_OK);
    assert(p4_game_tables_element(t, "split", &slash) == P4_TABLE_FROZEN);
    assert(p4_game_tables_affinity(t, "split", &row) == P4_TABLE_FROZEN);
    assert(p4_game_tables_channel(t, "custom", &custom) == P4_TABLE_FROZEN);
    assert(p4_game_tables_freeze(t, 43) == P4_TABLE_FROZEN);
    assert(p4_game_tables_find_affinity(t, 1, 300, 5, 19, 0)->value == (P4_AFF_WEAK | 125));
    assert(p4_game_tables_find_affinity(t, 1, 301, 5, 19, 0)->value == (P4_AFF_NULL | 100));
    assert(p4_game_tables_find_affinity(t, 1, 300, 6, 19, 0)->value == (P4_AFF_REPEL | 100));
    assert(!p4_game_tables_find_affinity(t, 1, 300, 7, 19, 0));
    assert(!p4_game_tables_find_affinity(t, 0, 300, 5, 19, 0));
    assert(p4_game_tables_find_affinity(t, 1, 300, 5, 6, 0)->value == (P4_AFF_WEAK | 125));
    assert(p4_game_tables_find_affinity(t, 1, 300, 5, 6, 1)->value == P4_KO_NULL);
    assert(p4_game_tables_find_channel(t, 20) == p4_game_tables_channel_key(t, "custom:channel"));
    assert(!strcmp(p4_game_tables_find_element(t, 19)->key, "split:slash"));
    assert(p4_game_tables_find_element(t, 19)->icon.revision == 123);
    assert(!strcmp(p4_game_tables_find_element(t, 19)->icon.owner, "art") &&
        !strcmp(p4_game_tables_find_element(t, 19)->icon.key, "art:leaf"));
    assert(p4_game_tables_channel_count(t) == 4);
    assert(p4_game_tables_affinity_count(t) == 5);
    assert(p4_game_tables_channel_at(t, 3)->id == 20);
    assert(!p4_game_tables_channel_at(t, 4));
    assert(!strcmp(p4_game_tables_element_owner(t, 0), "split"));
    assert(!strcmp(p4_game_tables_element_owner(t, 19), "split"));
    assert(!p4_game_tables_element_owner(t, 1));
    assert(!strcmp(p4_game_tables_channel_owner(t, 20), "custom"));
    assert(!p4_game_tables_channel_owner(t, 0));
    for (size_t i = 0; i < p4_game_tables_affinity_count(t); ++i) {
        const P4AffinityRow *r = p4_game_tables_affinity_at(t, i);
        assert(r == p4_game_tables_find_affinity(t, r->entity_kind, r->entity_id,
            r->profile_id, r->element_id, r->channel_id));
        assert(!strcmp(p4_game_tables_affinity_owner(t, i), r->element_id == 6 ? "lightdark" : "split"));
    }
    assert(!p4_game_tables_affinity_at(t, 5));
    assert(!p4_game_tables_affinity_owner(t, 5));
    assert(strstr(p4_game_tables_error(P4_TABLE_CONFLICT), "claimed"));
    uint32_t last_order = 0, last_id = 0;
    for (size_t i = 0; i < p4_game_tables_element_count(t); ++i) {
        const P4ElementDesc *d = p4_game_tables_element_at(t, i);
        assert(!i || d->order > last_order || (d->order == last_order && d->id > last_id));
        last_order = d->order; last_id = d->id;
    }
    p4_game_tables_destroy(t);
}
static void refusal(void)
{
    P4GameTables *t = p4_game_tables_create(); assert(t);
    P4AffinityRow row = {1, 1, 0, 19, 0, 100, 100};
    assert(p4_game_tables_affinity(t, "test", &row) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(t, 2) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_revision(t) == 0);
    P4ElementDesc d = leaf(19, "test:leaf", 0);
    assert(p4_game_tables_element(t, "test", &d) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(t, 2) == P4_TABLE_OK);
    p4_game_tables_destroy(t);
    t = p4_game_tables_create(); assert(t);
    row.element_id = 6; row.channel_id = 1; row.value = P4_AFF_REPEL | 100;
    assert(p4_game_tables_affinity(t, "test", &row) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(t, 1) == P4_TABLE_INVALID);
    p4_game_tables_destroy(t);
    t = p4_game_tables_create(); assert(t);
    row.channel_id = 0; row.value = P4_AFF_REPEL | P4_AFF_DRAIN | 100;
    assert(p4_game_tables_affinity(t, "test", &row) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(t, 1) == P4_TABLE_INVALID);
    p4_game_tables_destroy(t);
    uint32_t kind, scalar;
    p4_affinity_project_ko(P4_AFF_REPEL | 100, &kind, &scalar); assert(kind == P4_KO_NULL && !scalar);
    p4_affinity_project_ko(P4_AFF_DRAIN | 100, &kind, &scalar); assert(kind == P4_KO_NULL && !scalar);
    p4_affinity_project_ko(P4_AFF_WEAK | 125, &kind, &scalar); assert(kind == P4_KO_VULNERABLE && scalar == 125);
    p4_affinity_project_ko(P4_AFF_RESIST | 50, &kind, &scalar); assert(kind == P4_KO_RESIST && scalar == 50);
    p4_affinity_project_ko(0, &kind, &scalar); assert(kind == P4_KO_NORMAL && !scalar);
}
static void action_profiles(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    P4ElementDesc slash = leaf(19, "test:slash", 19), pierce = leaf(20, "test:pierce", 20);
    assert(p4_game_tables_element(tables, "test", &slash) == P4_TABLE_OK);
    assert(p4_game_tables_element(tables, "test", &pierce) == P4_TABLE_OK);
    P4ActionElementBinding binding = {
        P4_ACTION_ELEMENT_WEAPON, UINT32_MAX, P4_ACTION_COMPONENT_DAMAGE, 20, P4_CHANNEL_DAMAGE, 1, 3, "test:weapon"
    };
    assert(p4_game_tables_action_element(tables, "test", &binding) == P4_TABLE_OK);
    assert(p4_game_tables_action_element(tables, "other", &binding) == P4_TABLE_INVALID);
    strcpy(binding.key, "other:conflict");
    assert(p4_game_tables_action_element(tables, "other", &binding) == P4_TABLE_CONFLICT);
    strcpy(binding.key, "test:skill"); binding.source_kind = P4_ACTION_ELEMENT_SKILL; binding.element_id = 19;
    assert(p4_game_tables_action_element(tables, "test", &binding) == P4_TABLE_OK);
    binding.source_id = 0; /* Same key cannot claim another source. */
    assert(p4_game_tables_action_element(tables, "test", &binding) == P4_TABLE_CONFLICT);
    binding.source_id = UINT32_MAX; binding.component = P4_ACTION_COMPONENT_INSTANT_KO; binding.channel_id = P4_CHANNEL_INSTANT_KO;
    strcpy(binding.key, "test:skill-ko");
    assert(p4_game_tables_action_element(tables, "test", &binding) == P4_TABLE_OK);
    binding.component = UINT32_MAX; binding.channel_id = P4_CHANNEL_DAMAGE;
    strcpy(binding.key, "test:skill-future-component");
    assert(p4_game_tables_action_element(tables, "test", &binding) == P4_TABLE_OK);
    strcpy(binding.key, "test:enemy"); binding.source_kind = P4_ACTION_ELEMENT_ENEMY_ATTACK;
    binding.source_id = UINT32_MAX; binding.component = P4_ACTION_COMPONENT_INSTANT_KO; binding.channel_id = P4_CHANNEL_INSTANT_KO;
    assert(p4_game_tables_action_element(tables, "test", &binding) == P4_TABLE_OK);
    P4AffinityProfileAlias alias = {
        P4_ENTITY_ENEMY, UINT32_MAX, 65536u, 0x80000000u, UINT32_MAX, 1, 3, "test:enemy-profile"
    };
    assert(p4_game_tables_affinity_alias(tables, "test", &alias) == P4_TABLE_OK);
    strcpy(alias.key, "test:duplicate-selector");
    assert(p4_game_tables_affinity_alias(tables, "test", &alias) == P4_TABLE_CONFLICT);
    alias.native_profile = 65535u; alias.profile_id = 65535u;
    strcpy(alias.key, "test:enemy-other-profile");
    assert(p4_game_tables_affinity_alias(tables, "test", &alias) == P4_TABLE_OK);
    alias.entity_kind = P4_ENTITY_PERSONA; alias.native_profile = 0; alias.profile_id = 65535u;
    strcpy(alias.key, "test:persona-profile");
    assert(p4_game_tables_affinity_alias(tables, "test", &alias) == P4_TABLE_OK);
    P4GameActionElement resolved = {7, 7, 7, 7, 7, 7};
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_SKILL, UINT32_MAX, P4_ACTION_COMPONENT_DAMAGE, NULL, &resolved) == P4_TABLE_NOT_READY);
    assert(resolved.valid == 7); /* Refusal cannot publish a partial result. */
    assert(p4_game_tables_freeze(tables, 3) == P4_TABLE_NOT_FOUND);
    P4AffinityRow row = {P4_ENTITY_ENEMY, UINT32_MAX, 0x80000000u, 19, P4_CHANNEL_DAMAGE, 125 | P4_AFF_WEAK, 125};
    assert(p4_game_tables_affinity(tables, "test", &row) == P4_TABLE_OK);
    row.profile_id = 65535u; row.value = 100; row.scalar = 100;
    assert(p4_game_tables_affinity(tables, "test", &row) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 3) == P4_TABLE_NOT_FOUND); /* Persona is not enemy. */
    row.entity_kind = P4_ENTITY_PERSONA;
    assert(p4_game_tables_affinity(tables, "test", &row) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 3) == P4_TABLE_OK);
    assert(p4_game_tables_action_element_count(tables) == 5 && p4_game_tables_affinity_alias_count(tables) == 3);
    static const uint32_t kinds[] = {0, 0, 0, 1, 2}, components[] = {0, 1, UINT32_MAX, 0, 1};
    for (size_t i = 0; i < 5; ++i) {
        const P4ActionElementBinding *action = p4_game_tables_action_element_at(tables, i);
        assert(action && action->source_kind == kinds[i] && action->source_id == UINT32_MAX && action->component == components[i]);
        assert(action == p4_game_tables_find_action_element(tables, kinds[i], UINT32_MAX, components[i]));
        assert(!strcmp(p4_game_tables_action_element_owner(tables, i), "test"));
    }
    for (size_t i = 0; i < 3; ++i)
        assert(!strcmp(p4_game_tables_affinity_alias_owner(tables, i), "test"));
    assert(!p4_game_tables_action_element_at(tables, 5) && !p4_game_tables_affinity_alias_at(tables, 3));
    const P4AffinityProfileAlias *profile = p4_game_tables_find_affinity_alias(tables, P4_ENTITY_ENEMY, UINT32_MAX, 65536u);
    assert(profile && profile->profile_id == 0x80000000u && profile->knowledge_id == UINT32_MAX);
    assert(!p4_game_tables_find_affinity_alias(tables, P4_ENTITY_ENEMY, UINT32_MAX, 0));
    assert(p4_game_tables_find_affinity_alias(tables, P4_ENTITY_PERSONA, UINT32_MAX, 0)->profile_id == 65535u);
    P4GameActionElement declared = {0, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX};
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_SKILL, UINT32_MAX, P4_ACTION_COMPONENT_DAMAGE, &declared, &resolved) == P4_TABLE_OK);
    assert(resolved.valid && resolved.element_id == 19 && resolved.channel_id == P4_CHANNEL_DAMAGE);
    assert(p4_game_action_element_is_physical(&resolved) && p4_game_action_element_is_offensive(&resolved));
    assert(p4_game_action_element_has_affinity(&resolved) && resolved.family == 0);
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_WEAPON, UINT32_MAX, P4_ACTION_COMPONENT_DAMAGE, &declared, &resolved) == P4_TABLE_OK);
    assert(resolved.element_id == 20);
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_ENEMY_ATTACK, UINT32_MAX, P4_ACTION_COMPONENT_INSTANT_KO, &declared, &resolved) == P4_TABLE_OK);
    assert(resolved.channel_id == P4_CHANNEL_INSTANT_KO);
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_SKILL, UINT32_MAX, P4_ACTION_COMPONENT_INSTANT_KO, &declared, &resolved) == P4_TABLE_OK);
    assert(resolved.component == P4_ACTION_COMPONENT_INSTANT_KO && resolved.channel_id == P4_CHANNEL_INSTANT_KO && resolved.element_id == 19);
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_SKILL, UINT32_MAX, UINT32_MAX, &declared, &resolved) == P4_TABLE_OK);
    assert(resolved.component == UINT32_MAX && resolved.channel_id == P4_CHANNEL_DAMAGE);
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_SKILL, 0, P4_ACTION_COMPONENT_DAMAGE, &declared, &resolved) == P4_TABLE_OK);
    assert(!resolved.valid && !resolved.element_id && !resolved.channel_id && !resolved.flags && !resolved.family);
    declared = (P4GameActionElement){1, 6, P4_CHANNEL_DAMAGE, UINT32_MAX, UINT32_MAX, P4_ACTION_COMPONENT_DAMAGE};
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_SKILL, 0, P4_ACTION_COMPONENT_DAMAGE, &declared, &declared) == P4_TABLE_OK);
    assert(declared.valid && declared.element_id == 6 && declared.family == 6 && !(declared.flags & P4_ELEMENT_PHYSICAL));
    declared.valid = 2;
    assert(p4_game_tables_resolve_action_element(tables, P4_ACTION_ELEMENT_SKILL, 0, P4_ACTION_COMPONENT_DAMAGE, &declared, &resolved) == P4_TABLE_INVALID);
    assert(p4_game_tables_action_element(tables, "test", &binding) == P4_TABLE_FROZEN);
    assert(p4_game_tables_affinity_alias(tables, "test", &alias) == P4_TABLE_FROZEN);
    p4_game_tables_destroy(tables);
}
/* Damage cover rows: one-leaf final-damage reduction by an owned source. */
static void damage_covers(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    P4ElementDesc slash = leaf(19, "test:slash", 19);
    assert(p4_game_tables_element(tables, "test", &slash) == P4_TABLE_OK);
    P4DamageCoverBinding cover = {P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 400, 19, P4_CHANNEL_DAMAGE, 20, 1, 1,
        "test:lining", "test:lining-slash"};
    assert(p4_game_tables_damage_cover(tables, "test", &cover) == P4_TABLE_OK);
    assert(p4_game_tables_damage_cover(tables, "test", &cover) == P4_TABLE_CONFLICT);
    strcpy(cover.key, "test:lining-slash-again");
    assert(p4_game_tables_damage_cover(tables, "test", &cover) == P4_TABLE_CONFLICT); /* A source covers a leaf once. */
    cover.element_id = 6; strcpy(cover.family, "other:lining"); strcpy(cover.key, "other:lining-light");
    assert(p4_game_tables_damage_cover(tables, "other", &cover) == P4_TABLE_CONFLICT); /* One owner per source. */
    strcpy(cover.family, "test:lining"); strcpy(cover.key, "test:lining-light");
    assert(p4_game_tables_damage_cover(tables, "test", &cover) == P4_TABLE_OK);
    P4DamageCoverBinding bad = cover; bad.source_id = 401; strcpy(bad.key, "test:bad");
    bad.percent = 0; assert(p4_game_tables_damage_cover(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.percent = 100; assert(p4_game_tables_damage_cover(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.percent = 10; bad.channel_id = P4_CHANNEL_INSTANT_KO;
    assert(p4_game_tables_damage_cover(tables, "test", &bad) == P4_TABLE_INVALID); /* Damage channel only. */
    bad.channel_id = P4_CHANNEL_DAMAGE; bad.source_kind = 2;
    assert(p4_game_tables_damage_cover(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.source_kind = P4_AFFINITY_EFFECT_SKILL; strcpy(bad.family, "other:guard");
    assert(p4_game_tables_damage_cover(tables, "test", &bad) == P4_TABLE_INVALID); /* Families are the owner's. */
    strcpy(bad.family, "test:guard"); strcpy(bad.key, "other:guard-light");
    assert(p4_game_tables_damage_cover(tables, "test", &bad) == P4_TABLE_INVALID);
    strcpy(bad.key, "test:guard-light"); bad.revision = 0;
    assert(p4_game_tables_damage_cover(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.revision = 1;
    assert(p4_game_tables_damage_cover(tables, "p4", &bad) == P4_TABLE_INVALID);
    assert(p4_game_tables_damage_cover(tables, "test", &bad) == P4_TABLE_OK);
    assert(p4_game_tables_damage_cover_count(tables) == 3);
    assert(!p4_game_tables_find_damage_cover(tables, P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 400, 19, P4_CHANNEL_DAMAGE)); /* Frozen only. */
    assert(p4_game_elements_requested(tables));
    assert(p4_game_tables_freeze(tables, 2) == P4_TABLE_OK);
    for (size_t i = 0; i < 3; ++i) {
        const P4DamageCoverBinding *row = p4_game_tables_damage_cover_at(tables, i); assert(row);
        assert(row == p4_game_tables_find_damage_cover(tables, row->source_kind, row->source_id, row->element_id, row->channel_id));
        assert(!strcmp(p4_game_tables_damage_cover_owner(tables, i), "test"));
        assert(!strcmp(p4_game_tables_damage_cover_row_owner(tables, row), "test"));
    }
    const P4DamageCoverBinding *row = p4_game_tables_find_damage_cover(tables, P4_AFFINITY_EFFECT_EQUIPPED_ITEM, 400, 19, P4_CHANNEL_DAMAGE);
    assert(row && row->percent == 20 && !strcmp(row->family, "test:lining") && !strcmp(row->key, "test:lining-slash"));
    assert(!p4_game_tables_find_damage_cover(tables, P4_AFFINITY_EFFECT_SKILL, 400, 19, P4_CHANNEL_DAMAGE));
    assert(!p4_game_tables_damage_cover_row_owner(tables, &cover));
    assert(!p4_game_tables_damage_cover_at(tables, 3) && !p4_game_tables_damage_cover_owner(tables, 3));
    assert(p4_game_tables_damage_cover(tables, "test", &cover) == P4_TABLE_FROZEN);
    p4_game_tables_destroy(tables);
    /* Every covered leaf exists and carries affinity at freeze. */
    tables = p4_game_tables_create(); assert(tables);
    cover.element_id = 30; strcpy(cover.key, "test:lining-missing");
    assert(p4_game_tables_damage_cover(tables, "test", &cover) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_NOT_FOUND);
    p4_game_tables_destroy(tables);
    tables = p4_game_tables_create(); assert(tables);
    cover.element_id = 16; strcpy(cover.key, "test:lining-special");
    assert(p4_game_tables_damage_cover(tables, "test", &cover) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_INVALID); /* Retail leaf 16 has no affinity. */
    p4_game_tables_destroy(tables);
}
static void affinity_effects(void)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    P4AffinityEffectBinding effect = {
        P4_AFFINITY_EFFECT_SKILL, UINT32_MAX, 6, P4_CHANNEL_DAMAGE, P4_AFF_RESIST, 1, 4, "test:light-damage-resist"
    };
    assert(p4_game_tables_affinity_effect(tables, "test", &effect) == P4_TABLE_OK);
    assert(p4_game_tables_affinity_effect(tables, "test", &effect) == P4_TABLE_CONFLICT);
    strcpy(effect.key, "other:dark-damage"); effect.element_id = 7;
    assert(p4_game_tables_affinity_effect(tables, "other", &effect) == P4_TABLE_CONFLICT); /* Source coverage has one owner. */
    strcpy(effect.key, "test:dark-damage"); effect.flags = P4_AFF_DRAIN;
    assert(p4_game_tables_affinity_effect(tables, "test", &effect) == P4_TABLE_OK);
    effect.source_id = 65536u; effect.channel_id = P4_CHANNEL_INSTANT_KO; effect.flags = P4_AFF_RESIST;
    strcpy(effect.key, "test:dark-ko");
    assert(p4_game_tables_affinity_effect(tables, "test", &effect) == P4_TABLE_OK);
    effect.source_kind = P4_AFFINITY_EFFECT_EQUIPPED_ITEM; effect.source_id = UINT32_MAX;
    effect.element_id = 6; effect.flags = P4_AFF_NULL; strcpy(effect.key, "gear:light-ko");
    assert(p4_game_tables_affinity_effect(tables, "gear", &effect) == P4_TABLE_OK);
    assert(p4_game_tables_affinity_effect_count(tables) == 4);
    assert(p4_game_tables_freeze(tables, 4) == P4_TABLE_OK);
    for (size_t i = 0; i < 4; ++i) {
        const P4AffinityEffectBinding *row = p4_game_tables_affinity_effect_at(tables, i); assert(row);
        assert(row == p4_game_tables_find_affinity_effect(tables, row->source_kind, row->source_id, row->element_id, row->channel_id));
        assert(!strcmp(p4_game_tables_affinity_effect_owner(tables, i), row->source_kind == P4_AFFINITY_EFFECT_SKILL ? "test" : "gear"));
    }
    const P4AffinityEffectBinding *row = p4_game_tables_find_affinity_effect(tables, P4_AFFINITY_EFFECT_SKILL, UINT32_MAX, 6, P4_CHANNEL_DAMAGE);
    assert(row && row->flags == P4_AFF_RESIST && !strcmp(row->key, "test:light-damage-resist"));
    assert(!p4_game_tables_find_affinity_effect(tables, P4_AFFINITY_EFFECT_SKILL, UINT32_MAX, 6, P4_CHANNEL_INSTANT_KO));
    assert(!p4_game_tables_find_affinity_effect(tables, P4_AFFINITY_EFFECT_EQUIPPED_ITEM, UINT32_MAX, 6, P4_CHANNEL_DAMAGE));
    assert(!p4_game_tables_find_affinity_effect(tables, P4_AFFINITY_EFFECT_SKILL, 0, 7, P4_CHANNEL_INSTANT_KO));
    assert(!p4_game_tables_affinity_effect_at(tables, 4) && !p4_game_tables_affinity_effect_owner(tables, 4));
    assert(p4_game_tables_affinity_effect(tables, "gear", &effect) == P4_TABLE_FROZEN);
    p4_game_tables_destroy(tables);

    tables = p4_game_tables_create(); assert(tables);
    effect.source_kind = P4_AFFINITY_EFFECT_SKILL; effect.source_id = 1; strcpy(effect.key, "test:invalid");
    effect.flags = P4_AFF_WEAK;
    assert(p4_game_tables_affinity_effect(tables, "test", &effect) == P4_TABLE_INVALID);
    effect.flags = P4_AFF_RESIST | P4_AFF_NULL;
    assert(p4_game_tables_affinity_effect(tables, "test", &effect) == P4_TABLE_INVALID);
    effect.flags = P4_AFF_REPEL;
    assert(p4_game_tables_affinity_effect(tables, "test", &effect) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 4) == P4_TABLE_INVALID); /* KO has no reflect/heal state. */
    assert(!p4_game_tables_revision(tables));
    p4_game_tables_destroy(tables);

    tables = p4_game_tables_create(); assert(tables);
    effect.element_id = 19; effect.channel_id = P4_CHANNEL_DAMAGE;
    assert(p4_game_tables_affinity_effect(tables, "test", &effect) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 4) == P4_TABLE_NOT_FOUND);
    P4ElementDesc element = leaf(19, "test:new", 19);
    assert(p4_game_tables_element(tables, "test", &element) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 4) == P4_TABLE_OK);
    p4_game_tables_destroy(tables);
}
static void semantic_id_width(void)
{
    static const uint32_t ids[] = {65535u, 65536u, 0x80000000u, UINT32_MAX};
    static const char *const element_keys[] = {"wide:edge", "wide:above", "wide:sign", "wide:last"};
    static const char *const channel_keys[] = {"wide:edge_channel", "wide:above_channel", "wide:sign_channel", "wide:last_channel"};
    P4GameTables *t = p4_game_tables_create(); assert(t);
    for (size_t n = sizeof ids / sizeof ids[0]; n; --n) {
        size_t i = n - 1;
        P4ElementDesc d = leaf(ids[i], element_keys[i], UINT32_MAX);
        d.family = UINT32_MAX;
        assert(p4_game_tables_element(t, "wide", &d) == P4_TABLE_OK);
        P4AffinityChannelDesc channel = {0};
        channel.id = ids[i]; channel.kind = P4_CHANNEL_PERCENT;
        strcpy(channel.key, channel_keys[i]); strcpy(channel.display_name, "Wide channel");
        assert(p4_game_tables_channel(t, "wide", &channel) == P4_TABLE_OK);
        P4AffinityRow row = {P4_ENTITY_ENEMY, UINT32_MAX, 0x80000000u, ids[i], ids[i], P4_AFF_RESIST | 50, 65535u};
        assert(p4_game_tables_affinity(t, "wide", &row) == P4_TABLE_OK);
        assert(p4_game_tables_affinity(t, "other", &row) == P4_TABLE_CONFLICT);
        row.profile_id++;
        row.scalar = 65536u;
        assert(p4_game_tables_affinity(t, "wide", &row) == P4_TABLE_INVALID);
    }
    assert(p4_game_tables_freeze(t, 7) == P4_TABLE_OK);
    for (size_t i = 0; i < sizeof ids / sizeof ids[0]; ++i) {
        const P4ElementDesc *d = p4_game_tables_find_element(t, ids[i]);
        const P4AffinityChannelDesc *channel = p4_game_tables_find_channel(t, ids[i]);
        const P4AffinityRow *row = p4_game_tables_find_affinity(t, P4_ENTITY_ENEMY,
            UINT32_MAX, 0x80000000u, ids[i], ids[i]);
        assert(d && d->id == ids[i] && d->family == UINT32_MAX);
        assert(channel && channel->id == ids[i]);
        assert(row && row->element_id == ids[i] && row->channel_id == ids[i] && row->scalar == 65535u);
        assert(p4_game_tables_element_at(t, P4_ELEMENT_RETAIL_COUNT + i)->id == ids[i]);
        assert(p4_game_tables_channel_at(t, 3 + i)->id == ids[i]);
        assert(p4_game_tables_affinity_at(t, i) == row);
        assert(!strcmp(p4_game_tables_element_owner(t, ids[i]), "wide"));
        assert(!strcmp(p4_game_tables_channel_owner(t, ids[i]), "wide"));
    }
    p4_game_tables_destroy(t);

    t = p4_game_tables_create(); assert(t);
    P4ElementDesc d = leaf(UINT32_MAX, "wide:unresolved", 0);
    d.family = 65536u;
    assert(p4_game_tables_element(t, "wide", &d) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(t, 1) == P4_TABLE_NOT_FOUND);
    assert(!p4_game_tables_revision(t));
    P4ElementDesc family = leaf(65536u, "wide:family", 0);
    assert(p4_game_tables_element(t, "wide", &family) == P4_TABLE_OK);
    P4AffinityRow row = {P4_ENTITY_PERSONA, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, 100, 100};
    assert(p4_game_tables_affinity(t, "wide", &row) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(t, 1) == P4_TABLE_NOT_FOUND);
    p4_game_tables_destroy(t);

    t = p4_game_tables_create(); assert(t);
    row = (P4AffinityRow){P4_ENTITY_PERSONA, 1, 0, 0, P4_CHANNEL_DAMAGE, 65536u, 0};
    assert(p4_game_tables_affinity(t, "wide", &row) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(t, 1) == P4_TABLE_INVALID);
    p4_game_tables_destroy(t);
    t = p4_game_tables_create(); assert(t);
    row.channel_id = P4_CHANNEL_INSTANT_KO; row.value = P4_KO_NULL + 1u;
    assert(p4_game_tables_affinity(t, "wide", &row) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(t, 1) == P4_TABLE_INVALID);
    p4_game_tables_destroy(t);
}
/* Retail-derived rows: the value is the entity's own decoded retail entry
 * from the actual banks at seal; nothing authored or guessed before that. */
static void retail_derived(void)
{
    uint8_t enemies[3 * 32] = {0}, personas[2 * 32] = {0};
    enemies[2 * 32 + 0] = 0x00; enemies[2 * 32 + 1] = 0x08;      /* enemy row 2, Phys: weak, no percent */
    enemies[2 * 32 + 12] = 0x0A; enemies[2 * 32 + 13] = 0x10;    /* column 6: resist 50% */
    personas[1 * 32 + 0] = 0x14; personas[1 * 32 + 1] = 0x00;    /* persona row 1, Phys: normal 100 */
    personas[1 * 32 + 14] = 0x00; personas[1 * 32 + 15] = 0x01;  /* column 7: null */
    P4GameTables *t = p4_game_tables_create(); assert(t);
    P4ElementDesc slash = leaf(19, "split:slash", 0);
    assert(p4_game_tables_element(t, "split", &slash) == P4_TABLE_OK);
    P4AffinityRow enemy = {P4_ENTITY_ENEMY, 2, 2, 19, P4_CHANNEL_DAMAGE, 999999u, 999999u};
    assert(p4_game_tables_affinity_retail(t, "split", &enemy, 2, 0) == P4_TABLE_OK);
    P4AffinityRow authored = enemy; authored.value = authored.scalar = 0;
    assert(p4_game_tables_affinity(t, "split", &authored) == P4_TABLE_CONFLICT); /* Same key, authored or not. */
    assert(p4_game_tables_affinity_retail(t, "other", &enemy, 2, 0) == P4_TABLE_CONFLICT);
    P4AffinityRow persona = {P4_ENTITY_PERSONA, 1, 0x1001, 19, P4_CHANNEL_DAMAGE, 0, 0};
    assert(p4_game_tables_affinity_retail(t, "split", &persona, 1, 0) == P4_TABLE_OK);
    P4AffinityRow ko = {P4_ENTITY_PERSONA, 1, 0x1001, 7, P4_CHANNEL_INSTANT_KO, 0, 0};
    assert(p4_game_tables_affinity_retail(t, "split", &ko, 1, 7) == P4_TABLE_OK);
    P4AffinityRow resist = {P4_ENTITY_ENEMY, 2, 2, 6, P4_CHANNEL_DAMAGE, 0, 0};
    assert(p4_game_tables_affinity_retail(t, "split", &resist, 2, 6) == P4_TABLE_OK);
    P4AffinityRow bad_column = {P4_ENTITY_ENEMY, 2, 2, 20, P4_CHANNEL_DAMAGE, 0, 0};
    assert(p4_game_tables_affinity_retail(t, "split", &bad_column, 2, 16) == P4_TABLE_INVALID);
    assert(p4_game_tables_affinity_retail_pending(t) == 4);
    assert(p4_game_tables_freeze(t, 5) == P4_TABLE_NOT_READY); /* Never publish an unread value. */
    /* A row outside the actual bank refuses before resolving anything. */
    assert(p4_game_tables_affinity_resolve_retail(t, enemies, 2 * 32, personas, sizeof personas) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_affinity_retail_pending(t) == 4);
    assert(p4_game_tables_affinity_resolve_retail(t, enemies, sizeof enemies - 1, personas, sizeof personas) == P4_TABLE_NOT_FOUND);
    assert(p4_game_tables_affinity_resolve_retail(t, enemies, sizeof enemies, NULL, 0) == P4_TABLE_NOT_FOUND);
    /* A word with no form for its channel refuses here, naming its row and
     * element: two flags (weak|null), retail's ailment no-effect flag 0x20 on a
     * percentage row, and the same flag on a KO row (never projected as KO). */
    uint8_t bad[sizeof enemies], bad_personas[sizeof personas];
    memcpy(bad, enemies, sizeof bad);
    bad[2 * 32 + 1] = 0x09;
    assert(p4_game_tables_affinity_resolve_retail(t, bad, sizeof bad, personas, sizeof personas) == P4_TABLE_INVALID);
    bad[2 * 32 + 1] = 0x20;
    assert(p4_game_tables_affinity_resolve_retail(t, bad, sizeof bad, personas, sizeof personas) == P4_TABLE_INVALID);
    memcpy(bad_personas, personas, sizeof bad_personas);
    bad_personas[1 * 32 + 15] = 0x20;
    assert(p4_game_tables_affinity_resolve_retail(t, enemies, sizeof enemies, bad_personas, sizeof bad_personas) == P4_TABLE_INVALID);
    assert(p4_game_tables_affinity_retail_pending(t) == 4);
    assert(p4_game_tables_affinity_resolve_retail(t, enemies, sizeof enemies, personas, sizeof personas) == P4_TABLE_OK);
    assert(!p4_game_tables_affinity_retail_pending(t));
    assert(p4_game_tables_freeze(t, 5) == P4_TABLE_OK);
    const P4AffinityRow *row = p4_game_tables_find_affinity(t, P4_ENTITY_ENEMY, 2, 2, 19, P4_CHANNEL_DAMAGE);
    assert(row && row->value == (P4_AFF_WEAK | 125) && row->scalar == 125);
    row = p4_game_tables_find_affinity(t, P4_ENTITY_ENEMY, 2, 2, 6, P4_CHANNEL_DAMAGE);
    assert(row && row->value == (P4_AFF_RESIST | 50) && row->scalar == 50);
    row = p4_game_tables_find_affinity(t, P4_ENTITY_PERSONA, 1, 0x1001, 19, P4_CHANNEL_DAMAGE);
    assert(row && row->value == 100 && row->scalar == 100);
    row = p4_game_tables_find_affinity(t, P4_ENTITY_PERSONA, 1, 0x1001, 7, P4_CHANNEL_INSTANT_KO);
    assert(row && row->value == P4_KO_NULL); /* The retail KO projection, not a raw word. */
    assert(p4_game_tables_affinity_resolve_retail(t, enemies, sizeof enemies, personas, sizeof personas) == P4_TABLE_FROZEN);
    assert(p4_game_tables_affinity_retail(t, "split", &bad_column, 2, 0) == P4_TABLE_FROZEN);
    p4_game_tables_destroy(t);
}
static P4GameTables *side_tables(const char *leaf_owner, uint32_t leaf_flags)
{
    P4GameTables *tables = p4_game_tables_create(); assert(tables);
    char key[32];
    snprintf(key, sizeof key, "%s:freeze", leaf_owner);
    P4ElementDesc status = leaf(22, key, 22);
    status.flags = leaf_flags;
    assert(p4_game_tables_element(tables, leaf_owner, &status) == P4_TABLE_OK);
    return tables;
}
static void side_ailments(void)
{
    const uint32_t status_flags = P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE;
    P4GameTables *tables = side_tables("test", status_flags);
    const P4SideAilmentDef side = {22, P4_SIDE_FLAGS, 0x16, 2, 100, 0, 0, 1, 1, "test:freeze-ailment"};
    assert(p4_game_tables_side_ailment(tables, "test", &side) == P4_TABLE_OK);
    assert(p4_game_tables_side_ailment(tables, "test", &side) == P4_TABLE_CONFLICT);
    P4SideAilmentDef bad = side;
    strcpy(bad.key, "test:freeze-again");
    assert(p4_game_tables_side_ailment(tables, "test", &bad) == P4_TABLE_CONFLICT); /* One row per leaf. */
    bad.leaf = 23; bad.flags = 16; assert(p4_game_tables_side_ailment(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.flags = P4_SIDE_LOSE_TURN; bad.transfer = 101; assert(p4_game_tables_side_ailment(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.transfer = 0;
    bad.flags = P4_SIDE_LOSE_TURN; bad.enemy_scale = 101; assert(p4_game_tables_side_ailment(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.enemy_scale = 0;
    bad.flags = P4_SIDE_LOSE_TURN; bad.cure_mask = 0;
    assert(p4_game_tables_side_ailment(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.cure_mask = 0xFF; bad.crit_bonus = 101; assert(p4_game_tables_side_ailment(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.crit_bonus = 0; bad.revision = 0; assert(p4_game_tables_side_ailment(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.revision = 1; strcpy(bad.key, "other:shock"); assert(p4_game_tables_side_ailment(tables, "test", &bad) == P4_TABLE_INVALID);
    strcpy(bad.key, "p4:shock"); assert(p4_game_tables_side_ailment(tables, "p4", &bad) == P4_TABLE_INVALID);
    assert(p4_game_tables_side_ailment_count(tables) == 1 && p4_game_elements_requested(tables));
    assert(!p4_game_tables_find_side_ailment(tables, 22)); /* Frozen only. */
    row_probe = (struct RuntimeRowProbe){.view = tables, .domain = P4_TABLE_SOURCE_SIDE_AILMENT, .ids = {22}};
    assert(!p4_game_tables_bind_runtime_use(tables, runtime_row_probe, &row_probe));
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    const P4SideAilmentDef *row = p4_game_tables_find_side_ailment(tables, 22);
    assert(row && row == p4_game_tables_side_ailment_at(tables, 0) && !p4_game_tables_side_ailment_at(tables, 1));
    assert(row->flags == P4_SIDE_FLAGS && row->cure_mask == 0x16 && row->default_from == 2 && row->crit_bonus == 100 &&
        !row->transfer);
    assert(!strcmp(p4_game_tables_side_ailment_owner(tables, 0), "test") && !p4_game_tables_side_ailment_owner(tables, 1));
    assert(!p4_game_tables_find_side_ailment(tables, 23));
    assert(!p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_SIDE_AILMENT, row) && row_probe.calls == 1);
    assert(p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_SIDE_AILMENT, &side) == P4_TABLE_INVALID);
    assert(p4_game_tables_side_ailment(tables, "test", &side) == P4_TABLE_FROZEN);
    p4_game_tables_destroy(tables);
    /* Freeze: the owner's own non-offensive status leaf, a damage default. */
    static const struct { const char *leaf_owner; uint32_t leaf_flags, leaf, default_from; int result; } cases[] = {
        {"test", P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, 30, 2, P4_TABLE_NOT_FOUND},
        {"test", P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, 12, 2, P4_TABLE_INVALID}, /* A retail leaf. */
        {"other", P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, 22, 2, P4_TABLE_INVALID}, /* Not its owner's. */
        {"test", P4_ELEMENT_AFFINITY | P4_ELEMENT_OFFENSIVE, 22, 2, P4_TABLE_INVALID},
        {"test", P4_ELEMENT_VISIBLE, 22, 2, P4_TABLE_INVALID},
        {"test", P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, 22, 40, P4_TABLE_NOT_FOUND},
        {"test", P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, 22, 8, P4_TABLE_INVALID}, /* Panic is no damage leaf. */
        {"test", P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE, 22, P4_SIDE_DEFAULT_NONE, P4_TABLE_OK},
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        tables = side_tables(cases[i].leaf_owner, cases[i].leaf_flags);
        P4SideAilmentDef value = side;
        value.leaf = cases[i].leaf; value.default_from = cases[i].default_from;
        assert(p4_game_tables_side_ailment(tables, "test", &value) == P4_TABLE_OK);
        assert(p4_game_tables_freeze(tables, 1) == cases[i].result);
        p4_game_tables_destroy(tables);
    }
    tables = p4_game_tables_create(); assert(tables);
    for (uint32_t i = 0; i <= P4_SIDE_AILMENT_LIMIT; ++i) {
        P4SideAilmentDef value = side;
        value.leaf = 100 + i;
        snprintf(value.key, sizeof value.key, "test:side-%u", (unsigned)i);
        assert(p4_game_tables_side_ailment(tables, "test", &value) ==
            (i < P4_SIDE_AILMENT_LIMIT ? P4_TABLE_OK : P4_TABLE_INVALID));
    }
    p4_game_tables_destroy(tables);
}
static void side_riders(void)
{
    const uint32_t status_flags = P4_ELEMENT_AFFINITY | P4_ELEMENT_VISIBLE;
    const P4SideAilmentDef side = {22, P4_SIDE_FLAGS, 0x16, 2, 100, 0, 0, 1, 1, "test:freeze-ailment"};
    P4GameTables *tables = side_tables("test", status_flags);
    assert(p4_game_tables_side_ailment(tables, "test", &side) == P4_TABLE_OK);
    const P4SideRiderDesc mabufu = {0x1D, 22, 6, 1, 1, "test:mabufu-freeze"};
    const P4SideRiderDesc bufu = {0x1A, 22, 10, 1, 1, "test:bufu-freeze"};
    assert(p4_game_tables_side_rider(tables, "test", &mabufu) == P4_TABLE_OK);
    assert(p4_game_tables_side_rider(tables, "test", &bufu) == P4_TABLE_OK);
    P4SideRiderDesc bad = bufu;
    assert(p4_game_tables_side_rider(tables, "test", &bad) == P4_TABLE_CONFLICT);
    strcpy(bad.key, "test:bufu-again");
    assert(p4_game_tables_side_rider(tables, "test", &bad) == P4_TABLE_CONFLICT); /* One row per (skill, leaf). */
    bad.skill_id = 0x1B; bad.rate = 0; assert(p4_game_tables_side_rider(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.rate = 101; assert(p4_game_tables_side_rider(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.rate = 12; bad.revision = 0; assert(p4_game_tables_side_rider(tables, "test", &bad) == P4_TABLE_INVALID);
    bad.revision = 1; strcpy(bad.key, "other:bufula"); assert(p4_game_tables_side_rider(tables, "test", &bad) == P4_TABLE_INVALID);
    strcpy(bad.key, "p4:bufula"); assert(p4_game_tables_side_rider(tables, "p4", &bad) == P4_TABLE_INVALID);
    size_t first = 9, count = 9;
    assert(p4_game_tables_side_rider_count(tables) == 2 && p4_game_elements_requested(tables));
    assert(!p4_game_tables_find_side_riders(tables, 0x1A, &first, &count) && !first && !count); /* Frozen only. */
    row_probe = (struct RuntimeRowProbe){.view = tables, .domain = P4_TABLE_SOURCE_SIDE_RIDER, .ids = {0x1A, 22}};
    assert(!p4_game_tables_bind_runtime_use(tables, runtime_row_probe, &row_probe));
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_OK);
    assert(p4_game_tables_find_side_riders(tables, 0x1A, &first, &count) && first == 0 && count == 1);
    const P4SideRiderDesc *row = p4_game_tables_side_rider_at(tables, 0);
    assert(row && row->skill_id == 0x1A && row->rate == 10);
    assert(p4_game_tables_find_side_riders(tables, 0x1D, &first, &count) && first == 1 && count == 1);
    assert(!p4_game_tables_find_side_riders(tables, 0x1B, &first, &count) && !count && !p4_game_tables_side_rider_at(tables, 2));
    assert(!strcmp(p4_game_tables_side_rider_owner(tables, 1), "test") && !p4_game_tables_side_rider_owner(tables, 2));
    assert(!p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_SIDE_RIDER, row) && row_probe.calls == 1);
    assert(p4_game_tables_runtime_use_row(tables, P4_TABLE_SOURCE_SIDE_RIDER, &bufu) == P4_TABLE_INVALID);
    assert(p4_game_tables_side_rider(tables, "test", &bufu) == P4_TABLE_FROZEN);
    p4_game_tables_destroy(tables);
    /* Freeze: a rider carries its own owner's side ailment. */
    tables = side_tables("test", status_flags);
    P4SideRiderDesc orphan = bufu; orphan.leaf = 23;
    assert(p4_game_tables_side_ailment(tables, "test", &side) == P4_TABLE_OK);
    assert(p4_game_tables_side_rider(tables, "test", &orphan) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_NOT_FOUND);
    p4_game_tables_destroy(tables);
    tables = side_tables("test", status_flags);
    const P4SideRiderDesc foreign = {0x1A, 22, 10, 1, 1, "other:bufu-freeze"};
    assert(p4_game_tables_side_ailment(tables, "test", &side) == P4_TABLE_OK);
    assert(p4_game_tables_side_rider(tables, "other", &foreign) == P4_TABLE_OK);
    assert(p4_game_tables_freeze(tables, 1) == P4_TABLE_INVALID);
    p4_game_tables_destroy(tables);
    tables = p4_game_tables_create(); assert(tables);
    for (uint32_t i = 0; i <= P4_SIDE_RIDER_LIMIT; ++i) {
        P4SideRiderDesc value = bufu;
        value.skill_id = i;
        snprintf(value.key, sizeof value.key, "test:rider-%u", (unsigned)i);
        assert(p4_game_tables_side_rider(tables, "test", &value) == (i < P4_SIDE_RIDER_LIMIT ? P4_TABLE_OK : P4_TABLE_INVALID));
    }
    p4_game_tables_destroy(tables);
}
int main(void)
{
    retail(); expansion(); refusal(); action_profiles(); affinity_effects(); stable_image_references(); semantic_id_width(); runtime_row_transport(); damage_covers(); side_ailments(); side_riders();
    retail_derived();
    puts("game_tables: retail decode, owned expansion, profile/channel isolation and atomic freeze passed");
    return 0;
}
