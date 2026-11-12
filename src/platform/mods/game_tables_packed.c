/* The battle packed loader (0x00193840) has one observer for every family that
 * relocates its banks: the selected input is authenticated once, then each
 * staged family records the banks it saw in that load. */
#include "p4pc/game_table_admission_native.h"
#include "p4pc/game_skills_native.h"
#include "p4pc/game_enemies.h"
#include "p4pc/game_enemies_native.h"
#include "p4pc/game_personas.h"
#include "p4pc/game_personas_native.h"
#include "p4pc/game_tables_native.h"
#include <stdio.h>
#include <stdlib.h>

static uint32_t word(const uint8_t *p)
{
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
int p4_game_tables_packed_blocks(const uint8_t *descriptor, const uint8_t *bytes,
    uint32_t size, P4GameTablePackedBlock blocks[P4_GAME_TABLE_PACKED_SLOTS])
{
    if (!descriptor || !bytes || !blocks) return -1;
    uint32_t offset = 0;
    /* The actual loader's descriptor, not a path search through other files:
     * twenty eight-byte slots in each 0xA4-byte descriptor. */
    for (unsigned block = 0; block < P4_GAME_TABLE_PACKED_SLOTS; ++block) {
        const uint32_t slot = word(descriptor + block * 8u + 4u);
        if (!slot) return (int)block;
        if (offset > size || size - offset < 4) return -1;
        const uint32_t block_size = word(bytes + offset);
        if (block_size > size - offset - 4) return -1;
        blocks[block] = (P4GameTablePackedBlock){slot, block_size};
        const uint32_t span = block_size + 4;
        if (span > UINT32_MAX - 15 || offset > UINT32_MAX - ((span + 15) & ~15u)) return -1;
        offset += (span + 15) & ~15u;
    }
    return -1;
}
static _Noreturn void refuse(int result)
{
    fprintf(stderr, "p4pc: packed table observation refused (%d): %s\n", result, p4_game_tables_error(result));
    abort();
}
void p4_game_tables_observe_packed(const uint8_t *descriptor, const char *path,
    const uint8_t *bytes, uint32_t size, P4GameTableAdmissionReceipt *receipt)
{
    if (!p4_game_skills_packed_pending() && !p4_game_enemies_packed_pending() &&
        !p4_game_personas_packed_pending()) return;
    if (!descriptor || !path || !bytes) refuse(P4_TABLE_NOT_READY);
    P4GameTablePackedBlock blocks[P4_GAME_TABLE_PACKED_SLOTS];
    const int count = p4_game_tables_packed_blocks(descriptor, bytes, size, blocks);
    if (count < 0) refuse(P4_TABLE_INVALID);
    uint32_t skill_rows = 0;
    const uint32_t skills = p4_game_skills_packed_scan(blocks, (unsigned)count, &skill_rows);
    const uint32_t enemies = p4_game_enemies_packed_scan(blocks, (unsigned)count);
    const uint32_t personas = p4_game_personas_packed_scan(blocks, (unsigned)count);
    if (!skills && !enemies && !personas) return;
    int result;
    if (receipt) result = p4_game_table_admission_consume(receipt, descriptor, path, bytes, size);
    else {
        result = skills ? p4_game_tables_prepare("skill", path, skill_rows, bytes, size) : P4_TABLE_OK;
        if (!result && enemies) result = p4_game_tables_prepare("enemy", path, P4_ENEMY_RETAIL_ROWS, bytes, size);
        if (!result && personas) result = p4_game_tables_prepare("persona", path, P4_PERSONA_RETAIL_ROWS, bytes, size);
    }
    if (result) refuse(result);
    p4_game_skills_packed_accept(skills);
    p4_game_enemies_packed_accept(enemies);
    p4_game_personas_packed_accept(personas);
}
