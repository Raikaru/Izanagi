/* Shared non-VM form of 001932f0. This constructs native DatUnit groups and
 * requests the same battle sequence; initialization/AI/resolution stay native.
 * Preparation and script context belong to the caller, not this adapter. */
#include "p4pc/battle_sim.h"
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/gmem.h"
#include <stdio.h>
#include <string.h>

extern void *datUnitCreatePc(uint16_t id);
extern int16_t datGetPartyId(int32_t slot);
extern void *datUnitCreateEncounterGroup(uint16_t encounter);
extern void hFadeTaskStart(int16_t mode);
extern void sequenceRequest(int32_t sequence, uint32_t data, int32_t size, int32_t delay);

static uint32_t save_word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int p4_battle_save_valid(const uint8_t *image, uint32_t size)
{
    static const struct { uint32_t tag, bytes; } sections[] = {
#define P4LS_SAVE_SECTION(tag, address, bytes) { tag, bytes },
#include "p4pc/lockstep_save_sections.inc"
#undef P4LS_SAVE_SECTION
    };
    if (!image || size < 56u || size > 0x2a000u || save_word(image)) return 0;
    unsigned summary = 0;
    for (unsigned i = 4; i < 54; i++) summary = (summary + image[i]) & 255u;
    if (image[54] != summary || image[55]) return 0;
    uint32_t offset = 56;
    unsigned checksum = 0;
    for (unsigned row = 0; row < sizeof sections / sizeof *sections; row++) {
        if (size - offset < 8) return 0;
        const uint32_t tag = save_word(image + offset), bytes = save_word(image + offset + 4);
        /* Native compact images omit all three field sections, never a subset. */
        if (sections[row].tag == 0x25 && tag == 0x28) row += 3;
        if (tag != sections[row].tag || bytes != sections[row].bytes || bytes > size - offset - 8) return 0;
        for (uint32_t i = 0; i < bytes + 8; i++) checksum = (checksum + image[offset + i]) & 255u;
        offset += bytes + 8;
    }
    return size - offset >= 13 && save_word(image + offset) == 0x2000 &&
           save_word(image + offset + 4) == 1 && image[offset + 8] == checksum &&
           save_word(image + offset + 9) == UINT32_MAX;
}

int p4_battle_encounter_start(uint32_t id)
{
    uint32_t base, count, enemy_base, enemy_count;
    if (!p4_game_table_extent_rows(P4_GPADDR(0xb414), 0x18, &base, &count) || id >= count ||
        !p4_game_table_extent_rows(P4_GPADDR(0xb3c4), 0x3c, &enemy_base, &enemy_count)) return 0;
    /* The native unit domain (datUnitInit 00231af0): 0x150 for retail, the
     * committed expanded catalog's limit otherwise; the bank extent also bounds. */
    const uint32_t enemy_limit = p4_game_enemies_native_limit();
    const uint8_t *record = P4_G2H(base + id * 0x18u);
    int populated = 0;
    for (unsigned i = 0; i < 5; i++) {
        uint16_t enemy;
        memcpy(&enemy, record + 8 + i * 2, 2);
        if (enemy >= enemy_limit || enemy >= enemy_count) return 0;
        if (enemy) populated = 1;
    }
    if (!populated) return 0;
    uint32_t party[4] = {1, 0, 0, 0}, members = 1, seen = 1u << 1;
    for (unsigned slot = 0; slot < 4 && members < 4; slot++) {
        const uint32_t pc = (uint32_t)datGetPartyId((int32_t)slot) & 65535u;
        if (!pc) continue;
        /* Bound the native persistent record domain (datUnitGetRecord2..10)
         * and reject duplicated actors. Keep the real save's formation; a
         * declarative builder additionally needs the qualified role catalog. */
        if (pc < 2 || pc > 10 || (seen & (1u << pc))) return 0;
        seen |= 1u << pc;
        party[members++] = pc;
    }
    uint32_t packet_address = p4_gport_alloc(0x3c, 16);
    if (!packet_address) return 0;
    uint8_t *packet = P4_G2H(packet_address);
    hFadeTaskStart(1);
    for (unsigned slot = 0; slot < members; slot++)
        ((uint32_t *)(packet + 4))[slot] = P4_H2G(datUnitCreatePc((uint16_t)party[slot]));
    uint32_t group = P4_H2G(datUnitCreateEncounterGroup((uint16_t)id));
    *(uint32_t *)(packet + 0x14) = group;
    uint16_t field, variant;
    memcpy(&field, record + 0x12, 2);
    memcpy(&variant, record + 0x14, 2);
    if (!field && !variant) { field = 0xf0; variant = 1; }
    *(uint16_t *)(packet + 0x20) = field;
    *(uint16_t *)(packet + 0x22) = variant;
    fprintf(stderr, "p4pc: encounter adapter: encounter %u, %u party, enemy group %08x, formation %u:%u\n",
            id, members, group, field, variant);
    sequenceRequest(6, packet_address, 0x3c, 4);
    return 1;
}
