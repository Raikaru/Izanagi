/* Exercises canonical saves, the bounded preparation wire and encounter start
 * over the table service's extents, with no retail assets or simulated combat.
 * Any accidental native launch is an immediate test abort. */
#include "guest_test.h"
#include "p4pc/battle_sim.h"
#include "p4pc/game_table_extents.h"
#include "p4pc/battle_prepare.h"
#undef NDEBUG
#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *datUnitCreatePc(uint16_t id) { (void)id; abort(); }
/* Reaching the party query means encounter start admitted every enemy ID;
 * only admits_enemy expects it, and it unwinds before any native builder. */
static jmp_buf bounds_passed;
static int expect_party_query;
int16_t datGetPartyId(int32_t slot)
{
    (void)slot;
    if (!expect_party_query) abort();
    longjmp(bounds_passed, 1);
}
static uint32_t enemy_limit;
uint32_t p4_game_enemies_native_limit(void) { return enemy_limit; }
void *datUnitCreateEncounterGroup(uint16_t encounter) { (void)encounter; abort(); }
void hFadeTaskStart(int16_t mode) { (void)mode; abort(); }
void sequenceRequest(int32_t id, uint32_t data, int32_t size, int32_t delay)
{
    (void)id; (void)data; (void)size; (void)delay; abort();
}

static void put_word(uint8_t *out, uint32_t value)
{
    for (unsigned i = 0; i < 4; i++) out[i] = (uint8_t)(value >> (i * 8));
}

/* Synthetic serialization only: no retail data and never passed to a battle. */
static uint32_t make_save(uint8_t *image, int compact, uint32_t omit, uint32_t shorten)
{
    static const struct { uint32_t tag, bytes; } sections[] = {
#define P4LS_SAVE_SECTION(tag, address, bytes) { tag, bytes },
#include "p4pc/lockstep_save_sections.inc"
#undef P4LS_SAVE_SECTION
    };
    memset(image, 0, 0x2a000);
    uint32_t offset = 56;
    for (unsigned i = 0; i < sizeof sections / sizeof *sections; i++) {
        const uint32_t tag = sections[i].tag;
        if (tag == omit || (compact && tag >= 0x25 && tag <= 0x27)) continue;
        const uint32_t bytes = sections[i].bytes - (tag == shorten);
        put_word(image + offset, tag);
        put_word(image + offset + 4, bytes);
        offset += 8 + bytes;
    }
    unsigned checksum = 0;
    for (uint32_t i = 56; i < offset; i++) checksum = (checksum + image[i]) & 255u;
    put_word(image + offset, 0x2000);
    put_word(image + offset + 4, 1);
    image[offset + 8] = (uint8_t)checksum;
    put_word(image + offset + 9, UINT32_MAX);
    return offset + 13;
}

static void check_save_images(void)
{
    static uint8_t image[0x2a000];
    uint32_t bytes = make_save(image, 0, 0, 0);
    assert(bytes == 171112 && p4_battle_save_valid(image, bytes));
    assert(!p4_battle_save_valid(NULL, bytes));
    assert(!p4_battle_save_valid(image, 55));
    assert(!p4_battle_save_valid(image, bytes - 1));
    image[4] = 1;
    assert(!p4_battle_save_valid(image, bytes));
    image[4] = 0;
    image[200] ^= 1;
    assert(!p4_battle_save_valid(image, bytes));
    bytes = make_save(image, 1, 0, 0);
    assert(bytes == 85064 && p4_battle_save_valid(image, bytes));
    /* These malformed images have correct checksums: layout must reject them. */
    bytes = make_save(image, 0, 2, 0);
    assert(!p4_battle_save_valid(image, bytes));
    bytes = make_save(image, 0, 0x25, 0);
    assert(!p4_battle_save_valid(image, bytes));
    bytes = make_save(image, 0, 0x26, 0);
    assert(!p4_battle_save_valid(image, bytes));
    bytes = make_save(image, 0, 0, 2);
    assert(!p4_battle_save_valid(image, bytes));
}

/* Structural wire fixtures only. The zero-data save is canonically valid but
 * is never a prepared party, native field or combat/fidelity witness. */
static uint32_t make_preparation(uint8_t *request, uint32_t actors, uint32_t personas, uint32_t items, uint32_t decisions)
{
    memset(request, 0, 0x2b000);
    memcpy(request, "P4BPRE01", 8);
    put_word(request + 8, P4_PREPARE_HEADER_BYTES);
    const uint32_t source_bytes = make_save(request + P4_PREPARE_HEADER_BYTES, 0, 0, 0);
    put_word(request + 16, source_bytes); put_word(request + 20, actors); put_word(request + 24, personas);
    put_word(request + 28, items); put_word(request + 32, decisions); put_word(request + 36, 2);
    put_word(request + 60, 1000); put_word(request + 64, 2000);
    put_word(request + 76, UINT32_MAX); put_word(request + 80, UINT32_MAX);
    put_word(request + 112, items != 0);
    uint8_t *actor = request + P4_PREPARE_HEADER_BYTES + source_bytes;
    for (uint32_t i = 0; i < actors; i++, actor += P4_PREPARE_ACTOR_BYTES) {
        put_word(actor, i + 1);
        put_word(actor + 4, UINT32_MAX); put_word(actor + 8, UINT32_MAX);
        put_word(actor + 12, UINT32_MAX); put_word(actor + 28, UINT32_MAX);
    }
    uint8_t *persona = actor;
    for (uint32_t i = 0; i < personas; i++, persona += P4_PREPARE_PERSONA_BYTES) {
        put_word(persona, 1); put_word(persona + 4, i); put_word(persona + 8, 1);
        put_word(persona + 12, 1); put_word(persona + 16, UINT32_MAX);
        put_word(persona + 20, 10); put_word(persona + 24, 100); put_word(persona + 28, 1);
        persona[48] = 1; persona[50] = 2; memset(persona + 64, 10, 5); put_word(persona + 88, 7);
    }
    uint8_t *item = persona;
    for (uint32_t i = 0; i < items; i++, item += P4_PREPARE_ITEM_BYTES) {
        put_word(item, 0x301 + i); put_word(item + 4, 3);
    }
    uint8_t *decision = item;
    for (uint32_t i = 0; i < decisions; i++, decision += P4_PREPARE_DECISION_BYTES) {
        put_word(decision, 1); put_word(decision + 8, 2); put_word(decision + 12, 3);
        put_word(decision + 16, UINT32_MAX);
    }
    return (uint32_t)(decision - request);
}
static void check_preparation_wire(void)
{
    static uint8_t request[0x2b000];
    P4BattlePreparation view;
    const char *reason = NULL;
#define ACCEPT() assert(p4_battle_prepare_decode(request, bytes, &view, &reason) && !reason)
#define REJECT() assert(!p4_battle_prepare_decode(request, bytes, &view, &reason) && reason)
    uint32_t bytes = make_preparation(request, 1, 0, 0, 0);
    ACCEPT();
    assert(view.source == request + 160 && view.source_bytes == 171112 && view.actor_count == 1);
    assert(!p4_battle_prepare_decode(NULL, bytes, &view, &reason));
    assert(!p4_battle_prepare_decode(request, bytes + 1, &view, &reason));
    put_word(request + 12, 1); REJECT(); put_word(request + 12, 0);
    request[160 + 200] ^= 1; REJECT(); request[160 + 200] ^= 1; ACCEPT();
    put_word(request + 92, 2); REJECT(); put_word(request + 88, 1); ACCEPT();
    bytes = make_preparation(request, 2, 0, 0, 0);
    uint8_t *actor = request + 160 + 171112;
    put_word(actor + 64, 1); REJECT(); put_word(actor + 64, 5); REJECT();
    put_word(actor + 64, 2); ACCEPT();
    put_word(actor + 16, 1); put_word(actor + 20, 100); put_word(actor + 24, 50); ACCEPT();
    put_word(request + 40, 1); REJECT(); put_word(request + 40, 0);
    put_word(actor + 20, 1000); REJECT();
    bytes = make_preparation(request, 1, 1, 0, 0);
    uint8_t *persona = request + 160 + 171112 + 64;
    ACCEPT();
    /* The parser bound is schema-only: added persona IDs pass, up to the 16-bit
     * native record field; the catalog check happens at native apply. */
    put_word(persona + 8, 256); ACCEPT(); put_word(persona + 8, 511); ACCEPT();
    put_word(persona + 8, UINT16_MAX); ACCEPT();
    put_word(persona + 8, UINT16_MAX + 1u); REJECT();
    put_word(persona + 8, UINT32_MAX); REJECT(); /* Only preserve mode may omit the ID. */
    put_word(persona + 8, 1);
    put_word(persona + 88, 3); REJECT(); put_word(persona + 88, 7);
    put_word(persona + 16, 0); REJECT(); put_word(persona + 16, UINT32_MAX);
    persona[69] = 1; REJECT(); persona[69] = 0;
    persona[32] = 1; REJECT(); persona[32] = 0;
    persona[50] = 1; REJECT(); persona[50] = 2;
    persona[48] = 255; persona[49] = 15; ACCEPT();
    persona[48] = 0; persona[49] = 16; REJECT(); persona[48] = 1; persona[49] = 0;
    put_word(request + 36, 1); REJECT(); put_word(request + 36, 2);
    memset(persona, 0, 112); put_word(persona, 1); put_word(persona + 8, UINT32_MAX); ACCEPT();
    put_word(persona + 8, 0); REJECT(); put_word(persona + 8, UINT32_MAX);
    put_word(persona + 16, 12); REJECT();
    bytes = make_preparation(request, 1, 1, 0, 1);
    persona = request + 160 + 171112 + 64;
    put_word(persona + 12, 2); memset(persona + 32, 0, 48); put_word(persona + 88, 0); put_word(persona + 84, 1); ACCEPT();
    uint8_t *decision = persona + 112;
    put_word(decision, 2); REJECT(); put_word(decision, 1);
    put_word(decision + 20, 1); REJECT(); put_word(decision + 20, 0);
    put_word(persona + 80, 1); REJECT(); put_word(persona + 80, 0);
    put_word(persona + 84, 0); REJECT();
    bytes = make_preparation(request, 1, 0, 2, 0);
    uint8_t *item = request + 160 + 171112 + 64;
    ACCEPT();
    put_word(item + 8, 0x301); REJECT(); put_word(item + 8, 0x302);
    put_word(item, 1536); REJECT(); put_word(item, 0x301);
    put_word(item + 4, 100); REJECT(); put_word(item + 4, 3);
    put_word(request + 112, 0); REJECT();
    bytes = make_preparation(request, 1, 0, 0, 0);
    put_word(request + 32, UINT32_MAX); REJECT();
#undef REJECT
#undef ACCEPT
}

/* 1 when encounter 0, whose first enemy is `enemy`, passes the enemy-ID
 * bounds for this native limit and published enemy-bank extent. */
static int admits_enemy(uint32_t enemy, uint32_t limit, uint32_t bank_rows)
{
    const uint32_t encounter_slot = P4_GPADDR(0xb414), encounter_base = 0x01000000u;
    const uint32_t enemy_slot = P4_GPADDR(0xb3c4), enemy_base = 0x01100000u; /* Never read. */
    uint8_t *row = P4_G2H(encounter_base);
    memset(row, 0, 0x18);
    row[8] = (uint8_t)enemy;
    row[9] = (uint8_t)(enemy >> 8);
    P4_G32(encounter_slot) = encounter_base;
    P4_G32(enemy_slot) = enemy_base;
    p4_game_table_extent_note(encounter_slot, encounter_base, 0x18);
    p4_game_table_extent_note(enemy_slot, enemy_base, bank_rows * 0x3c);
    enemy_limit = limit;
    volatile int admitted = 0;
    expect_party_query = 1;
    if (setjmp(bounds_passed)) admitted = 1;
    else assert(!p4_battle_encounter_start(0));
    expect_party_query = 0;
    return admitted;
}

int main(void)
{
    check_save_images();
    check_preparation_wire();
    if (p4t_guest_map(0x00764000u, 0x2000) != 0 || p4t_guest_map(0x01000000u, 0x1000) != 0 ||
        p4t_guest_map(0x01200000u, 0x2000) != 0) return 77;
    const uint32_t slot = P4_GPADDR(0xb414), base = 0x007f17b0u;
    P4_G32(slot) = base;
    P4_GPDATA(uint32_t, 0xb438) = 8;
    p4_game_table_extent_note(slot, base, 48);
    assert(!p4_battle_encounter_start(2)); /* Before any native builder/access. */
    p4_game_table_extent_note(slot, base, 0);
    assert(!p4_battle_encounter_start(0));
    /* Retail: the native limit 0x150 matches the bank. */
    assert(admits_enemy(0x14f, 0x150, 0x150) && !admits_enemy(0x150, 0x150, 0x150));
    /* Relocated larger banks do not widen the domain past the native limit. */
    assert(!admits_enemy(0x150, 0x150, 0x160));
    /* An expanded catalog's limit admits its added IDs, still bounded by it... */
    assert(admits_enemy(0x150, 0x160, 0x160) && admits_enemy(0x15f, 0x160, 0x160));
    assert(!admits_enemy(0x160, 0x160, 0x170));
    /* ...and by the published bank extent. */
    assert(!admits_enemy(0x158, 0x160, 0x158));
    /* Persona catalog: populated metadata rows exist past every extent below,
     * so only the published extent can reject them. */
    const uint32_t persona_slot = 0x007644c4u, persona_base = 0x01200000u;
    uint8_t *metadata = P4_G2H(persona_base);
    for (uint32_t row = 0; row < 0x2000 / 14; row++) metadata[row * 14 + 3] = (uint8_t)(row ? 1 : 0);
    P4_G32(persona_slot) = persona_base;
    p4_game_table_extent_note(persona_slot, persona_base, 256 * 14); /* Retail. */
    assert(p4_battle_prepare_persona_catalogued(255));
    assert(!p4_battle_prepare_persona_catalogued(256) && !p4_battle_prepare_persona_catalogued(511));
    p4_game_table_extent_note(persona_slot, persona_base, 512 * 14); /* Expanded. */
    assert(p4_battle_prepare_persona_catalogued(256) && p4_battle_prepare_persona_catalogued(511));
    assert(!p4_battle_prepare_persona_catalogued(512)); /* Past the published extent. */
    assert(!p4_battle_prepare_persona_catalogued(0)); /* Unpopulated level. */
    metadata[300 * 14 + 3] = 100;
    assert(!p4_battle_prepare_persona_catalogued(300));
    P4_G32(persona_slot) = persona_base + 14;
    assert(!p4_battle_prepare_persona_catalogued(256)); /* Slot moved without publishing. */
    puts("battle_sim_table: canonical saves, bounded preparation wire, extent/limit-bounded encounters and persona catalog PASS");
    return 0;
}
