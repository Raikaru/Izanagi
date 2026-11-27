/* Cache-verified full rows; legacy banks remain PS2-shaped. */
#include "golden_battle_rows.h"
#include "p4pc/golden_battle_rows_native.h"
#include "p4pc/game_mode.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/gmem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ResultMarker {
    struct ResultMarker *next;
    uint64_t uid;
    uint32_t action;
} ResultMarker;
typedef struct PacketDelay {
    struct PacketDelay *next;
    uint32_t packet, remaining;
} PacketDelay;
static struct {
    uint8_t skill[472u * 44u], effect[472u * 12u];
    P4GameTableAllocator allocator;
    void *effect_host;
    uint32_t effect_guest;
    int staged, active;
    ResultMarker *markers;
    PacketDelay *delays;
    P4GoldenCommonResourceOwner common_owner;
} rows;
static uint32_t golden_id(uint32_t port)
{
    port &= 0xFFFFu;
    if (port < 440u) return port;
    if (port < 576u) return port + 32u;
    if (port < 608u) return port - 136u;
    return port;
}
static uint32_t word(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24; }
static void ready(void)
{
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN || !rows.active) {
        fputs("p4pc: Golden battle row read before verified native publication\n", stderr);
        abort();
    }
}
int p4_golden_common_resource_owner_install(const P4GoldenCommonResourceOwner *owner)
{
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN || !owner || !owner->interception ||
        rows.common_owner.interception) return -1;
    rows.common_owner = *owner;
    return 0;
}
int p4_golden_common_resource_owner_remove(const P4GoldenCommonResourceOwner *owner)
{
    if (!owner || !rows.common_owner.interception ||
        rows.common_owner.interception != owner->interception ||
        rows.common_owner.context != owner->context) return -1;
    memset(&rows.common_owner, 0, sizeof rows.common_owner);
    return 0;
}
uint32_t p4_golden_common_interception_resource(uint32_t battle)
{
    ready();
    uint32_t handle = battle && rows.common_owner.interception ?
        rows.common_owner.interception(rows.common_owner.context, battle) : 0;
    if (!handle) {
        fputs("p4pc: M8f-res:battle-common-14A8-1490: ready BED400 common resource owner is unavailable\n", stderr);
        abort();
    }
    return handle;
}
const uint8_t *p4_golden_skill_raw(uint32_t port)
{
    ready();
    uint32_t id = golden_id(port);
    return id < 472u ? rows.skill + id * 44u : NULL;
}
const uint8_t *p4_golden_effect_raw(uint32_t port)
{
    ready();
    uint32_t id = golden_id(port);
    return id < 472u ? P4_GPTR(const uint8_t, rows.effect_guest + id * 12u) : NULL;
}
uint32_t p4_golden_skill_flags(uint32_t port)
{
    const uint8_t *row = p4_golden_skill_raw(port);
    if (!row) { fputs("p4pc: passive skill has no Golden SKILL1 row\n", stderr); abort(); }
    return word(row);
}
uint32_t p4_golden_skill_selector(uint32_t port)
{ return (p4_golden_skill_flags(port) >> 9) & 15u; }
uint32_t p4_golden_effect_flags(uint32_t port)
{
    ready();
    /* EFFECT's original unchecked readers use the actual native guest backing,
     * including surrounding memory for an out-of-payload address. No zero row
     * or asset-derived value is substituted there. */
    return P4_G32(rows.effect_guest + golden_id(port) * 12u + 4u);
}
uint32_t p4_golden_effect_interval(uint32_t port)
{
    ready();
    uint32_t value = P4_G8(rows.effect_guest + golden_id(port) * 12u);
    return value ? value : 1u;
}
int32_t p4_golden_effect_timing(uint32_t port)
{
    ready();
    return (int16_t)P4_G16(rows.effect_guest + golden_id(port) * 12u + 8u);
}
void p4_golden_result_clear(uint32_t action)
{
    ResultMarker **link = &rows.markers;
    while (*link) {
        ResultMarker *entry = *link;
        if (entry->action == action) { *link = entry->next; free(entry); return; }
        link = &entry->next;
    }
}
int p4_golden_result_intercepted(uint32_t action)
{
    ready();
    for (ResultMarker *entry = rows.markers; entry; entry = entry->next)
        if (entry->action == action) return entry->uid == P4_GDATA(uint64_t, action);
    return 0;
}
void p4_golden_result_intercept(uint32_t action)
{
    ready();
    for (ResultMarker *entry = rows.markers; entry; entry = entry->next)
        if (entry->action == action) { entry->uid = P4_GDATA(uint64_t, action); return; }
    ResultMarker *entry = malloc(sizeof *entry);
    if (!entry) { fputs("p4pc: cannot retain Golden interception result\n", stderr); abort(); }
    *entry = (ResultMarker){rows.markers, P4_GDATA(uint64_t, action), action};
    rows.markers = entry;
}
void p4_golden_result_copy(uint32_t destination, uint32_t source)
{
    if (destination == source) return;
    int marked = p4_golden_result_intercepted(source);
    p4_golden_result_clear(destination);
    if (marked) p4_golden_result_intercept(destination);
}
void p4_golden_packet_delay_clear(uint32_t packet)
{
    PacketDelay **link = &rows.delays;
    while (*link) {
        PacketDelay *entry = *link;
        if (entry->packet == packet) { *link = entry->next; free(entry); return; }
        link = &entry->next;
    }
}
void p4_golden_packet_delay_set(uint32_t packet, uint32_t frames)
{
    ready();
    if (frames <= 0x7fffu) {
        p4_golden_packet_delay_clear(packet);
        P4_G16(packet + 0x48u) = (uint16_t)frames;
        return;
    }
    for (PacketDelay *entry = rows.delays; entry; entry = entry->next)
        if (entry->packet == packet) {
            entry->remaining = frames;
            P4_G16(packet + 0x48u) = (uint16_t)frames;
            return;
        }
    PacketDelay *entry = malloc(sizeof *entry);
    if (!entry) { fputs("p4pc: cannot retain Golden packet delay\n", stderr); abort(); }
    *entry = (PacketDelay){rows.delays, packet, frames};
    rows.delays = entry;
    P4_G16(packet + 0x48u) = (uint16_t)frames;
}
int p4_golden_packet_delay_step(uint32_t packet)
{
    for (PacketDelay *entry = rows.delays; entry; entry = entry->next)
        if (entry->packet == packet) {
            if (!entry->remaining) return 0;
            --entry->remaining;
            P4_G16(packet + 0x48u) = (uint16_t)entry->remaining;
            return 1;
        }
    return -1;
}
static int prepare_complete(void *unused)
{
    (void)unused;
    uint32_t metadata, count, effect, effects;
    if (!rows.staged || rows.active) return P4_TABLE_NOT_READY;
    if (!p4_game_table_extent_rows(0x007644A8u, 40u, &metadata, &count) || count < 440u ||
        !p4_game_table_extent_rows(0x007644ACu, 4u, &effect, &effects) || effects < 440u)
        return P4_TABLE_NOT_READY;
    for (uint32_t i = 0; i < 440u; ++i) {
        const uint8_t *native = P4_GPTR(const uint8_t, metadata + i * 40u);
        const uint8_t *raw = rows.skill + i * 44u;
        if (native[0] != raw[0] || memcmp(native + 1, raw + 4, 3) || memcmp(native + 4, raw + 8, 36))
            return P4_TABLE_CONFLICT;
        const uint8_t *presentation = P4_GPTR(const uint8_t, effect + i * 4u);
        const uint8_t *full = rows.effect + i * 12u;
        if (memcmp(presentation, full, 2) || memcmp(presentation + 2, full + 4, 2))
            return P4_TABLE_CONFLICT;
    }
    if (!rows.effect_host) {
        uint32_t guest = 0;
        void *host = rows.allocator.allocate(rows.allocator.user, sizeof rows.effect, &guest);
        if (!host) return P4_TABLE_NOMEM;
        if (!guest || (guest & 15u) || guest > UINT32_MAX - sizeof rows.effect) {
            rows.allocator.release(rows.allocator.user, guest, host, sizeof rows.effect);
            return P4_TABLE_INVALID;
        }
        memcpy(host, rows.effect, sizeof rows.effect);
        rows.effect_host = host;
        rows.effect_guest = guest;
    }
    return P4_TABLE_OK;
}
static void commit_complete(void *unused) { (void)unused; rows.active = 1; }
int p4_golden_battle_rows_capture(void *context, P4GameTablesDefinitionBuilder *builder,
    const P4GoldenCacheTable *extra, size_t count, char *reason, size_t capacity)
{
    (void)context;
    const P4GoldenCacheTable *skill = NULL, *effect = NULL;
    if (p4_game_mode() != P4_GAME_MODE_GOLDEN || rows.staged || !builder ||
        !builder->allocator.allocate || !builder->allocator.release || !extra) goto invalid;
    for (size_t i = 0; i < count; ++i) {
        if (!extra[i].name) goto invalid;
        if (!strcmp(extra[i].name, "rows/skill1")) { if (skill) goto invalid; skill = &extra[i]; }
        if (!strcmp(extra[i].name, "rows/effect0")) { if (effect) goto invalid; effect = &extra[i]; }
    }
    if (!skill || !skill->data || skill->size != sizeof rows.skill ||
        !effect || !effect->data || effect->size != sizeof rows.effect) goto invalid;
    if (p4_game_tables_battle_complete_register("golden-rows", prepare_complete, commit_complete, NULL)) goto invalid;
    memcpy(rows.skill, skill->data, sizeof rows.skill);
    memcpy(rows.effect, effect->data, sizeof rows.effect);
    rows.allocator = builder->allocator;
    rows.staged = 1;
    return 0;
invalid:
    if (reason && capacity) snprintf(reason, capacity, "verified Golden widened battle rows are missing or cannot stage");
    return -1;
}
void p4_golden_battle_rows_reset(void)
{
    if (rows.staged && !rows.active && !rows.effect_host &&
        p4_game_tables_battle_complete_unregister("golden-rows", prepare_complete, commit_complete, NULL)) abort();
    if (rows.effect_host)
        rows.allocator.release(rows.allocator.user, rows.effect_guest, rows.effect_host, sizeof rows.effect);
    while (rows.markers) p4_golden_result_clear(rows.markers->action);
    while (rows.delays) p4_golden_packet_delay_clear(rows.delays->packet);
    memset(&rows, 0, sizeof rows);
}
