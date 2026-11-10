/* Pure shared-format witness: real walker/sampler, callback-backed sparse RAM. */
#include "p4pc/lockstep_format.h"
#include <assert.h>
#include <stdio.h>

typedef struct { uint32_t base, front, mt_cursor, effect, matrix[16], save_address, save_value; } Memory;
static uint32_t read_word(void *user, uint32_t address)
{
    Memory *m=user;
    if (address==m->save_address) return m->save_value;
    if (address==0x00764adcu) return 0x00110000u;
    if (address>=0x00110000u && address<0x00110018u) {
        static const char name[24]="rotate pc";
        uint32_t value;
        memcpy(&value,name+(address-0x00110000u),sizeof value);
        return value;
    }
    if (address==0x00110038u) return 0x00120000u;
    if (address==0x00120048u) return 0x00130000u;
    if (address>=0x00130000u && address<0x00130040u)
        return m->matrix[(address-0x00130000u)/4];
    if (address==0x00764708u) return 0x200;
    if (address==0x008874e0u) return m->base;
    if (address==0x008874e4u) return m->base+m->front;
    if (address==0x008874e8u) return m->base;
    if (address==0x00763b6cu) return m->mt_cursor;
    if (address>=m->base && address<m->base+124) return (address-m->base)/4+123;
    if (address>=0x00885a90u && address<0x00886450u) return (address-0x00885a90u)/4+456;
    if (address>=0x00922de0u && address<0x00922df0u) return m->effect+(address-0x00922de0u)/4;
    return 0;
}
int main(void)
{
    Memory memory={.base=0x01000000u,.front=12,.mt_cursor=3};
    P4WalkHost host={.read32=read_word,.user=&memory};
    uint8_t a[32], b[32], encoded[4];
    p4ls_put(encoded,0x12345678u);
    assert(encoded[0]==0x78 && encoded[3]==0x12 && p4ls_get(encoded)==0x12345678u);
    p4ls_sample(&host, a, 0, NULL);
    memory.base=0x03001000u;
    p4ls_sample(&host, b, 0, NULL);
    assert(p4ls_equal(a,b)); /* heap relocation is not an RNG difference */
    memory.front+=4;
    p4ls_sample(&host, b, 0, NULL);
    assert(p4ls_get(a+4)!=p4ls_get(b+4));
    memory.front-=4;
    memory.mt_cursor++;
    p4ls_sample(&host, b, 0, NULL);
    assert(p4ls_get(a+4)!=p4ls_get(b+4));
    memcpy(b,a,32);
    b[12]^=1; /* raw sequence task pointer is diagnostic, not equality */
    assert(p4ls_equal(a,b));
    b[16]^=1; /* pad edge is gameplay state */
    assert(!p4ls_equal(a,b));
    p4ls_sample(&host, a, 0, NULL);
    memory.effect++;
    p4ls_sample(&host, b, 0, NULL);
    assert(p4ls_equal(a,b)); /* legacy records retain their fingerprint */
    p4ls_sample(&host, a, P4LS_EFFECT_RNG, NULL);
    memory.effect++;
    p4ls_sample(&host, b, P4LS_EFFECT_RNG, NULL);
    assert(!p4ls_equal(a,b)); /* anchored comparisons include the effect RNG */
    {
        uint8_t legacy[32];
        p4ls_sample(&host, a, P4LS_EFFECT_RNG, NULL);
        p4ls_sample(&host, legacy, 0, NULL);
        memory.matrix[7]=0x8f8f8f8fu;
        memory.matrix[11]=0x14b21c39u;
        memory.matrix[15]=0x94fd247cu;
        p4ls_sample(&host, b, P4LS_EFFECT_RNG, NULL);
        assert(p4ls_equal(a,b)); /* real 1602 divergence was only matrix padding */
        p4ls_sample(&host, b, 0, NULL);
        assert(!p4ls_equal(legacy,b)); /* the legacy profile is intentionally unchanged */
        memory.matrix[3]=0x20003u;
        p4ls_sample(&host, b, P4LS_EFFECT_RNG, NULL);
        assert(!p4ls_equal(a,b)); /* meaningful matrix flags are never discarded */
    }
    {
        uint32_t snapshot[663];
        p4ls_sample(&host,a,0,NULL);
        p4ls_sample(&host,b,0,snapshot);
        assert(!memcmp(a,b,sizeof a)); /* observing does not change legacy hashes */
        for (unsigned i=0;i<624;i++) assert(snapshot[i]==456+i);
        assert(snapshot[624]==memory.mt_cursor && snapshot[625]==1);
        for (unsigned i=0;i<31;i++) assert(snapshot[626+i]==123+i);
        assert(snapshot[657]==memory.front && snapshot[658]==0);
        for (unsigned i=0;i<4;i++) assert(snapshot[659+i]==memory.effect+i);
        memory.base=0;
        p4ls_sample(&host,b,0,snapshot);
        assert(snapshot[625]==0);
        for (unsigned i=626;i<659;i++) assert(snapshot[i]==0);
    }
    {
        const uint32_t flags=P4LS_EFFECT_RNG|P4LS_SAVED_STATE;
        memory.save_address=0x0079740cu; memory.save_value=0;
        p4ls_sample(&host,a,flags,NULL);
        memory.save_value=1; p4ls_sample(&host,b,flags,NULL);
        assert(p4ls_get(a)!=p4ls_get(b)); /* roster/inventory changes are strict-frame differences */
        memory.save_address=0x00796ea0u; memory.save_value=0;
        p4ls_sample(&host,a,flags,NULL);
        memory.save_value=0xbeef0000u; p4ls_sample(&host,b,flags,NULL);
        assert(p4ls_equal(a,b)); /* party +82/+83 are not serialized */
        memory.save_value=0xbeef0001u; p4ls_sample(&host,b,flags,NULL);
        assert(!p4ls_equal(a,b)); /* the neighboring serialized bytes are never masked */
    }
    puts("lockstep format: endian, RNG relocation/cursors, matrix semantics and state equality PASS");
    return 0;
}
