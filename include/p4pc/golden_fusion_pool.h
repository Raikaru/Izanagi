/* Golden-only twelve-source fusion inheritance pool. */
#ifndef P4PC_GOLDEN_FUSION_POOL_H
#define P4PC_GOLDEN_FUSION_POOL_H
#include <stdint.h>
#define P4_GOLDEN_FUSION_POOL_CAPACITY 96u
typedef struct P4GoldenFusionPool {
    uint16_t ids[P4_GOLDEN_FUSION_POOL_CAPACITY];
    unsigned count;
} P4GoldenFusionPool;
/* Returns 1 if inserted, 0 if already present, -1 if full. */
static inline int p4_golden_fusion_pool_add(P4GoldenFusionPool *pool, uint16_t id)
{
    unsigned i;
    for (i = 0; i < pool->count; ++i) if (pool->ids[i] == id) return 0;
    if (pool->count >= P4_GOLDEN_FUSION_POOL_CAPACITY) return -1;
    pool->ids[pool->count++] = id;
    return 1;
}
static inline void p4_golden_fusion_pool_remove(P4GoldenFusionPool *pool, unsigned index)
{
    if (index >= pool->count) return;
    --pool->count;
    for (; index < pool->count; ++index) pool->ids[index] = pool->ids[index + 1];
}
#endif
