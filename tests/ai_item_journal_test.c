#include "p4pc/mod_entry.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
static void set16(unsigned char *u, unsigned at, uint16_t v) { memcpy(u + at, &v, 2); }
static uint16_t get16(unsigned char *u, unsigned at) { uint16_t v; memcpy(&v, u + at, 2); return v; }
static void set32(unsigned char *u, unsigned at, uint32_t v) { memcpy(u + at, &v, 4); }
static uint32_t get32(unsigned char *u, unsigned at) { uint32_t v; memcpy(&v, u + at, 4); return v; }
int main(void)
{
    unsigned char u[48] = {0}; int a, b;
    assert(p4_ai_item_before(1, &a, 0x2091)); /* outside an operation */
    set16(u, 4, 0x1234); u[0x20] = 0xa5;
    p4_ai_item_begin(1, &a, u, 1);
    assert(p4_ai_item_before(1, &a, 0x2091));
    set16(u, 4, 0x9876); u[0x20] = 0; p4_ai_item_after(1, &a);
    assert(p4_ai_item_before(1, &a, 0x2091));
    set16(u, 4, 0x1111); u[0x20] = 0x22; p4_ai_item_after(1, &a);
    p4_ai_item_deny(1, &a); assert(p4_ai_item_finish(1, &a));
    assert(get16(u, 4) == 0x1234 && u[0x20] == 0xa5);
    p4_ai_item_begin(1, &a, u, 1);
    assert(p4_ai_item_before(1, &a, 0x2091));
    set16(u, 4, 0x9876); p4_ai_item_after(1, &a); set16(u, 4, 0x5678);
    unsigned conflicts = p4_ai_item_conflicts;
    p4_ai_item_deny(1, &a); assert(p4_ai_item_finish(1, &a));
    assert(get16(u, 4) == 0x5678 && p4_ai_item_conflicts == conflicts + 1);
    u[0] = 0x18; p4_ai_item_begin(1, &a, u, 1);
    assert(p4_ai_item_before(1, &a, 0x2095)); u[0] = 0;
    p4_ai_item_after(1, &a); u[0] = 8; conflicts = p4_ai_item_conflicts;
    p4_ai_item_deny(1, &a); assert(p4_ai_item_finish(1, &a));
    assert(u[0] == 0x18 && p4_ai_item_conflicts == conflicts + 1);
    p4_ai_item_begin(1, &a, u, 1); p4_ai_item_begin(1, &b, u, 1);
    assert(!p4_ai_item_before(1, &a, 0x2091));
    assert(!p4_ai_item_before(1, &b, 0x2091));
    assert(p4_ai_item_finish(1, &a)); assert(p4_ai_item_finish(1, &b));
    p4_ai_item_begin(1, &a, u, 1); /* disabled after opening: discard on allowance */
    assert(p4_ai_item_before(1, &a, 0x2096)); u[0] ^= 0x10;
    p4_ai_item_after(1, &a); assert(!p4_ai_item_finish(1, &a));
    unsigned char after = u[0];
    p4_ai_item_begin(1, &a, u, 0); /* enabled after opening: no rollback */
    assert(p4_ai_item_before(1, &a, 0x2096)); u[0] ^= 0x10;
    p4_ai_item_after(1, &a); p4_ai_item_deny(1, &a);
    assert(p4_ai_item_finish(1, &a) && u[0] == (unsigned char)(after ^ 0x10));
    p4_ai_item_begin(1, &a, u, 1); conflicts = p4_ai_item_conflicts;
    assert(!p4_ai_item_before(1, &a, 0xffff));
    assert(p4_ai_item_denied(1, &a) && p4_ai_item_finish(1, &a));
    assert(p4_ai_item_conflicts == conflicts + 1);
    after = u[0]; p4_ai_item_begin(1, &a, u, 1);
    assert(p4_ai_item_before(1, &a, 0x2095)); u[0] ^= 8;
    p4_ai_item_after(1, &a); p4_ai_item_forget(&a);
    assert(u[0] == after && !p4_ai_item_finish(1, &a));
    unsigned char target[48] = {0}, other[48] = {0};
    set32(u, 0x18, 0x50); set32(target, 0x18, 0x10003);
    assert(p4_ai_item_prediction_before(1, &a, target, 0x10001)); /* no scope */
    p4_ai_item_begin(1, &a, u, 1);
    assert(p4_ai_item_prediction_before(1, &a, target, 0x10001));
    set32(u, 0x18, 0); set32(target, 0x18, 0);
    p4_ai_item_prediction_after(1, &a);
    p4_ai_item_deny(1, &a); assert(p4_ai_item_finish(1, &a));
    assert(get32(u, 0x18) == 0x50 && get32(target, 0x18) == 0x10003);
    p4_ai_item_begin(1, &a, u, 1);
    assert(p4_ai_item_prediction_before(1, &a, target, 0x10002));
    set32(target, 0x18, 0); p4_ai_item_prediction_after(1, &a);
    set32(target, 0x18, 1); conflicts = p4_ai_item_conflicts;
    p4_ai_item_deny(1, &a); assert(p4_ai_item_finish(1, &a));
    assert(get32(target, 0x18) == 0x10003 && p4_ai_item_conflicts == conflicts + 1);
#ifdef P4_AI_ITEM_TEST_SMALL
    p4_ai_item_begin(1, &a, u, 1);
    assert(p4_ai_item_prediction_before(1, &a, target, 0x10001));
    set32(target, 0x18, 0); p4_ai_item_prediction_after(1, &a);
    conflicts = p4_ai_item_conflicts;
    assert(!p4_ai_item_prediction_before(1, &a, other, 0x10003));
    assert(p4_ai_item_denied(1, &a) && p4_ai_item_finish(1, &a));
    assert(get32(target, 0x18) == 0x10003 && get32(other, 0x18) == 0);
    assert(p4_ai_item_conflicts == conflicts + 1);
#else
    (void)other;
#endif
#ifdef P4_AI_ITEM_TEST_SMALL
    conflicts = p4_ai_item_conflicts;
    p4_ai_item_begin(2, &a, u, 1);
    assert(!p4_ai_item_before(2, &a, 0x2091));
    assert(p4_ai_item_finish(2, &a));
    assert(p4_ai_item_conflicts == conflicts + 1);
#endif
    puts("ai_item_journal: ok");
    return 0;
}
