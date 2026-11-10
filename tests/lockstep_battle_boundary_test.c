#include "p4pc/lockstep_battle_boundary.h"
#undef NDEBUG
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

typedef struct { uint32_t address, value; } Word;
typedef struct { Word words[6]; } Memory;
static int read_word(void *user, uint32_t address, uint32_t *value)
{
    Memory *memory = user;
    for (size_t i = 0; i < sizeof memory->words / sizeof memory->words[0]; i++)
        if (memory->words[i].address == address) { *value = memory->words[i].value; return 1; }
    return 0;
}
int main(void)
{
    Memory memory = {{{0x10010c, 0}, {0x100044, 0x101000}, {0x100064, 0x102000},
                      {0x101048, 0x102000}, {0x102008, 0xbeef1234}, {0x103038, 0x100000}}};
    P4BattleBoundary boundary = {0};
    P4BattleFieldRequest field = {0};
    assert(p4battle_field_request(read_word, &memory, 0x103000, 0xbeef1234, 0x55, &field) == 1);
    assert(field.task == 0x103000 && field.work == 0x100000 && field.encounter == 0x1234 && field.flags == 0x55);
    memory.words[0].value = 1;
    assert(p4battle_field_request(read_word, &memory, 0x103000, 20, 0, &field) == 0);
    memory.words[0].value = 0;
    assert(p4battle_field_request(read_word, &memory, 0x104000, 20, 0, &field) == -1);
    assert(p4battle_collision(read_word, &memory, 0x173d48, 0x100000, 0x100050, &boundary) == 1);
    assert(boundary.work == 0x100000 && boundary.setup == 0x100050);
    assert(boundary.actor == 0x101000 && boundary.group == 0x102000 && boundary.encounter == 0x1234);
    assert(p4battle_collision(read_word, &memory, 0x173d44, 0x100000, 0x100050, &boundary) == 0);
    assert(p4battle_collision(read_word, &memory, 0x173d48, 0x100000, 0x100054, &boundary) == -1);
    memory.words[0].value = 1;
    assert(p4battle_collision(read_word, &memory, 0x173d48, 0x100000, 0x100050, &boundary) == 0);
    memory.words[0].value = 0;
    memory.words[3].value++;
    assert(p4battle_collision(read_word, &memory, 0x173d48, 0x100000, 0x100050, &boundary) == -1);
    memory.words[3].value--;
    memory.words[4].address++;
    assert(p4battle_collision(read_word, &memory, 0x173d48, 0x100000, 0x100050, &boundary) == -1);
    assert(p4battle_collision(read_word, &memory, 0x173d48, 0xfffffff0, 0x40, &boundary) == -1);
    return 0;
}
