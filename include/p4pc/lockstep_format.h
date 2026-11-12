/* Shared on-disk/wire format for p4pc and the retail hook. All words are LE;
 * never fwrite a native struct. A record is 40 bytes: kind, game frame, 32 bytes.
 * PAD records are every successful port-0 scePadRead, including repeated reads.
 * FRAME records are at RASTERSHOWRASTER entry, after drawing, before present.
 * Their order is significant: a missing/extra read is an error, not a held input. */
#ifndef P4PC_LOCKSTEP_FORMAT_H
#define P4PC_LOCKSTEP_FORMAT_H
#include <stdint.h>
#include <string.h>
#include "p4pc/walk_script.h"
#define P4LS_MAGIC "P4LOCK1\n"
#define P4LS_RECORD_SIZE 40
#define P4LS_PAD 1u
#define P4LS_FRAME 2u
#define P4LS_MARK 0x100u
#define P4LS_EFFECT_RNG 0x200u /* anchored profile: effect RNG and meaningful matrix lanes */
#define P4LS_SAVED_STATE 0x400u /* aligned profile: serialized state + real playtime */
#define P4LS_BATTLE_QUALIFICATION 0x800u /* battle-only profile excludes elapsed playtime */
#define P4LS_GAME_FRAME 0x00764290u
#define P4LS_STATE_WORDS 6
static const uint32_t p4ls_state_addresses[P4LS_STATE_WORDS] = {
    0x00764290u, 0x00764298u, 0x008c024cu, 0x008c02ecu, 0x00797b78u, 0x00797b7cu
};
static const char *const p4ls_state_names[P4LS_STATE_WORDS] = {
    "game_frame", "sequence_task", "pad_edge", "pad_edge_raw", "calendar_packed", "time_slot_packed"
};
static inline uint32_t p4ls_get(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static inline void p4ls_put(uint8_t *p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static inline uint32_t p4ls_hash_word(uint32_t h, uint32_t v) {
    for (int b=0; b<4; b++) h=(h^((v>>(b*8))&255u))*16777619u;
    return h;
}
static inline uint32_t p4ls_hash_range(const P4WalkHost *h, uint32_t hash, uint32_t address, uint32_t size) {
    while (size) {
        const uint32_t offset=address&3u, value=h->read32(h->user,address&~3u);
        const uint32_t count=size<4u-offset ? size : 4u-offset;
        for (uint32_t i=0;i<count;i++) hash=(hash^((value>>((offset+i)*8))&255u))*16777619u;
        address+=count; size-=count;
    }
    return hash;
}
static inline uint32_t p4ls_hash_saved(const P4WalkHost *h, uint32_t hash, uint32_t flags) {
    static const struct { uint32_t tag,address,size; } sections[]={
#define P4LS_SAVE_SECTION(tag,address,size) {tag,address,size},
#include "p4pc/lockstep_save_sections.inc"
#undef P4LS_SAVE_SECTION
    };
    for (size_t i=0;i<sizeof sections/sizeof *sections;i++) {
        hash=p4ls_hash_word(hash,sections[i].tag);
        hash=p4ls_hash_range(h,hash,sections[i].address,sections[i].size);
    }
    if (!(flags&P4LS_BATTLE_QUALIFICATION))
        hash=p4ls_hash_word(hash,h->read32(h->user,0x0076428cu));
    hash=p4ls_hash_word(hash,h->read32(h->user,0x0079b67cu));
    hash=p4ls_hash_word(hash,h->read32(h->user,0x00764638u));
    hash=p4ls_hash_range(h,hash,0x00796de0u,18);
    hash=p4ls_hash_range(h,hash,0x00796e00u,18);
    return p4ls_hash_range(h,hash,0x00796e20u,36);
}
static inline int p4ls_equal(const uint8_t *a, const uint8_t *b) {
    return !memcmp(a,b,8) && !memcmp(a+16,b+16,16);
}

/* Pointer-free battle unit/action fingerprint. Failure is not equality:
 * malformed or unbounded lists stop the qualification on both participants. */
static inline int p4ls_battle_address(uint32_t address, uint32_t size) {
    return address>=0x00100000u && size<=0x01f00000u && address<=0x02000000u-size;
}
static inline int p4ls_battle_hash(const P4WalkHost *h, uint32_t *hash) {
    static const uint32_t heads[]={0x00764adcu,0x00764af4u,0x00764ae8u};
    static const uint32_t unit_offsets[]={0x00,0x04,0x08,0x0c,0x10,0x14,0x18,0x1c,
        0x20,0x24,0x28,0x2c,0x30,0x34,0x38,0x3c,0x40,0x80,0x84,0x88,0x8c,
        0x98,0x9c,0xa0,0xa4,0xa8,0xb0,0xb4,0xb8,0xbc,0xc0,0xc4,0xc8,
        0x9d8,0x9dc,0x9e0,0x9ec,0x9f0,0x9fc};
    static const uint32_t action_offsets[]={0,4,8,12,16,20,24,28,36};
    unsigned battle=0;
    for (unsigned l=0;l<3;l++) {
        uint32_t task=h->read32(h->user,heads[l]);
        unsigned count=0;
        for (;task && count<256;count++,task=h->read32(h->user,task+0x3cu)) {
            if (!p4ls_battle_address(task,0x40u)) return 0;
            char name[25];
            for (unsigned i=0;i<24;i+=4) p4ls_put((uint8_t *)name+i,h->read32(h->user,task+i));
            name[24]=0;
            if (!strcmp(name,"battle")) battle++;
        }
        if (task) return 0;
    }
    *hash=p4ls_hash_word(*hash,battle);
    if (!battle) return 1;
    if (battle!=1) return 0;
    const uint32_t context=h->read32(h->user,0x0076449cu);
    if (!p4ls_battle_address(context,0x300u)) return 0;
    *hash=p4ls_hash_word(*hash,h->read32(h->user,context+0x0cu));
    *hash=p4ls_hash_word(*hash,h->read32(h->user,context+0x2fcu));
    const uint32_t camera=context+0x24u;
    *hash=p4ls_hash_word(*hash,h->read32(h->user,camera)&0xffffu);
    *hash=p4ls_hash_range(h,*hash,camera+4u,0x70u);
    *hash=p4ls_hash_word(*hash,h->read32(h->user,camera+0x74u));
    *hash=p4ls_hash_word(*hash,h->read32(h->user,camera+0x78u)&0xffffu);
    *hash=p4ls_hash_range(h,*hash,camera+0x7cu,0x10u);
    *hash=p4ls_hash_word(*hash,h->read32(h->user,camera+0x94u));
    *hash=p4ls_hash_range(h,*hash,camera+0x9cu,0x20u);
    *hash=p4ls_hash_word(*hash,h->read32(h->user,camera+0xd0u)&0xffffu);
    *hash=p4ls_hash_range(h,*hash,camera+0xd4u,0x0cu);
    *hash=p4ls_hash_range(h,*hash,camera+0xe4u,8u);
    *hash=p4ls_hash_range(h,*hash,camera+0xf4u,8u);
    const uint32_t view=h->read32(h->user,0x00764b70u);
    *hash=p4ls_hash_word(*hash,!!view);
    if (view) {
        if (!p4ls_battle_address(view,0x88u)) return 0;
        const uint32_t frame=h->read32(h->user,view+4u);
        *hash=p4ls_hash_word(*hash,!!frame);
        if (frame) {
            if (!p4ls_battle_address(frame,0x90u)) return 0;
            for (unsigned lane=0;lane<16;lane++)
                if (lane!=7 && lane!=11 && lane!=15)
                    *hash=p4ls_hash_word(*hash,h->read32(h->user,frame+80u+lane*4u));
        }
        *hash=p4ls_hash_range(h,*hash,view+104u,8u);
        *hash=p4ls_hash_range(h,*hash,view+128u,8u);
    }
    const uint32_t current=h->read32(h->user,context+0x104u);
    for (unsigned group=0;group<3;group++) {
        uint32_t unit=h->read32(h->user,context+0x178u+group*4u);
        unsigned count=0;
        for (;unit && count<128;count++,unit=h->read32(h->user,unit+0xa6cu)) {
            if (!p4ls_battle_address(unit,0xa70u)) return 0;
            *hash=p4ls_hash_word(*hash,group);
            for (unsigned i=0;i<sizeof unit_offsets/sizeof *unit_offsets;i++)
                *hash=p4ls_hash_word(*hash,h->read32(h->user,unit+unit_offsets[i]));
            const uint32_t stats=h->read32(h->user,unit+0xa64u);
            *hash=p4ls_hash_word(*hash,!!stats);
            if (stats) {
                if (!p4ls_battle_address(stats,0x30u)) return 0;
                *hash=p4ls_hash_range(h,*hash,stats,0x30u);
            }
            const uint32_t model=h->read32(h->user,unit+0xa00u);
            *hash=p4ls_hash_word(*hash,!!model);
            if (model) {
                if (!p4ls_battle_address(model,0x234u)) return 0;
                *hash=p4ls_hash_word(*hash,h->read32(h->user,model+0xd8u));
                for (unsigned layer=0;layer<2;layer++) {
                    const uint32_t base=model+0xecu+layer*0xa4u;
                    *hash=p4ls_hash_range(h,*hash,base,0x20u);
                }
            }
        }
        if (unit) return 0;
        *hash=p4ls_hash_word(*hash,count);
    }
    uint32_t action=h->read32(h->user,context+0x174u);
    unsigned count=0;
    for (;action && count<256;count++,action=h->read32(h->user,action+0x450u)) {
        if (!p4ls_battle_address(action,0x454u)) return 0;
        *hash=p4ls_hash_word(*hash,action==current);
        for (unsigned i=0;i<sizeof action_offsets/sizeof *action_offsets;i++)
            *hash=p4ls_hash_word(*hash,h->read32(h->user,action+action_offsets[i]));
        const uint32_t owner=h->read32(h->user,action+0x30u);
        *hash=p4ls_hash_word(*hash,!!owner);
        if (owner) {
            if (!p4ls_battle_address(owner,0xacu)) return 0;
            *hash=p4ls_hash_word(*hash,h->read32(h->user,owner+0xa0u));
            *hash=p4ls_hash_word(*hash,h->read32(h->user,owner+0xa4u));
            *hash=p4ls_hash_word(*hash,h->read32(h->user,owner+0xa8u));
        }
    }
    if (action) return 0;
    *hash=p4ls_hash_word(*hash,count);
    return 1;
}
/* The walk digest's matrix and MT array, plus RpRandom's lagged-Fibonacci
 * state and cursor offsets. Heap addresses themselves are not comparable. */
/* Optional snapshot: MT[624], cursor, RP-valid, RP[31], front/back offsets,
 * effect[4]. Read once for both the ordinary hash and load-delta evidence. */
static inline int p4ls_sample(const P4WalkHost *host, uint8_t out[32], uint32_t flags, uint32_t *snapshot) {
    uint32_t matrix=p4walk_player_matrix(host), walk=2166136261u, rng=2166136261u;
    walk=p4ls_hash_word(walk, matrix != 0);
    if (matrix) {
        if (flags&P4LS_EFFECT_RNG) {
            /* RwMatrix.flags (lane 3) is state; lanes 7/11/15 are padding. */
            for (uint32_t i=0; i<16; i++)
                if (i!=7 && i!=11 && i!=15)
                    walk=p4ls_hash_word(walk,host->read32(host->user,matrix+i*4));
        } else {
            /* Preserve the legacy recording/nightly fingerprint byte for byte. */
            for (uint32_t i=0; i<16; i++) walk=p4ls_hash_word(walk,host->read32(host->user,matrix+i*4));
        }
    }
    if (flags&P4LS_SAVED_STATE) walk=p4ls_hash_saved(host,walk,flags);
    if ((flags&P4LS_BATTLE_QUALIFICATION) && !p4ls_battle_hash(host,&walk)) return 0;
    for (uint32_t i=0;i<624;i++) {
        const uint32_t value=host->read32(host->user,0x00885a90u+i*4);
        rng=p4ls_hash_word(rng,value);
        if (snapshot) snapshot[i]=value;
    }
    {
        const uint32_t cursor=host->read32(host->user,0x00763b6cu);
        rng=p4ls_hash_word(rng,cursor);
        if (snapshot) snapshot[624]=cursor;
    }
    {
        uint32_t offset=host->read32(host->user,0x00764708u);
        uint32_t slot=offset<0x10000u ? 0x008872e0u+offset : 0;
        uint32_t base=slot ? host->read32(host->user,slot) : 0;
        int valid=base>=0x00100000u && base<=0x03ffff84u; /* includes the native guest arena */
        rng=p4ls_hash_word(rng,(uint32_t)valid);
        if (snapshot) snapshot[625]=(uint32_t)valid;
        if (valid) {
            for (uint32_t i=0;i<31;i++) {
                const uint32_t value=host->read32(host->user,base+i*4);
                rng=p4ls_hash_word(rng,value);
                if (snapshot) snapshot[626+i]=value;
            }
            const uint32_t front=host->read32(host->user,slot+4)-base;
            const uint32_t back=host->read32(host->user,slot+8)-base;
            rng=p4ls_hash_word(rng,front);
            rng=p4ls_hash_word(rng,back);
            if (snapshot) { snapshot[657]=front; snapshot[658]=back; }
        } else if (snapshot) {
            memset(snapshot+626,0,33*sizeof *snapshot);
        }
    }
    if ((flags&P4LS_EFFECT_RNG) || snapshot)
        for (uint32_t i=0;i<4;i++) {
            const uint32_t value=host->read32(host->user,0x00922de0u+i*4);
            if (flags&P4LS_EFFECT_RNG) rng=p4ls_hash_word(rng,value);
            if (snapshot) snapshot[659+i]=value;
        }
    p4ls_put(out,walk); p4ls_put(out+4,rng);
    for (unsigned i=0;i<P4LS_STATE_WORDS;i++) p4ls_put(out+8+i*4,host->read32(host->user,p4ls_state_addresses[i]));
    return 1;
}
#endif
