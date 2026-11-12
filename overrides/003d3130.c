/* PS2AllMat node configuration (retail 0x003d3130..0x003d3780). The INCLUDE_ASM at this
 * marker covers three functions; all three are defined here. A PS2AllMat node
 * (func_003d3a80, "PS2AllMat.csl") keeps its pipeline configuration in the node's init data
 * (RxPipelineNodeGetInitData, node + 0x20), a 64-byte block:
 *   words 0..9   RxClusterDefinition pointers, one per cluster type
 *   word 12, 13  two sizes (default 276 = 69 * 4)
 *   word 14      per-vertex quad-word count (default 4)
 *   word 15      kind flags: 3 triangles, 12 lines, 16 points; 0 when not yet set
 * func_004125d0 is RxPipelineNodeGetInitData, func_00412520 is RxPipelineNodeCreateInitData.
 * Retail code compares through the RenderWare strcmp slot at 0x008873C8.
 *
 * Direct transliteration of the retail code. */

typedef s32 (*P4StrcmpFn)(const char *a, const char *b);

extern u32 *func_004125d0(u8 *node);
extern u32 *func_00412520(u8 *node, u32 size);

/* The attribute-set name every PS2 cluster definition carries (retail string at 0x00763C08). */
#define P4_PS2_ATTRSET_NAME ((const char *)0x00763C08)
#define P4_STRCMP (*(P4StrcmpFn *)0x008873C8)

/* New init data for a node: ten empty cluster slots and the default sizes. NULL on failure;
 * otherwise the init data as read back from the node. */
static u32 *p4_ps2allmat_new_initdata(u8 *node)
{
    u32 *d = func_00412520(node, 64);
    u32 i;

    if (d == NULL)
        return NULL;
    for (i = 0; i < 10; i++)
        d[i] = 0;
    d[15] = 0;
    d[14] = 4;
    d[13] = d[14] * 69;
    d[12] = d[14] * 69;
    return func_004125d0(node);
}

/* Stores cluster definition `cluster` as cluster type `type` (0..9) of the node's init data.
 * Types 0..5 additionally require the definition's attribute bits to include `mask` (type 4
 * also needs bit 0x4000); a mismatch returns NULL. A cluster definition from another
 * attribute set is rejected for types 0..5 (NULL) and silently ignored for 6..9 (node). */
u8 *func_003d3130(u8 *node, u32 *cluster, u32 type)
{
    u32 *d;

    if (node == NULL || cluster == NULL || type >= 10)
        return NULL;

    if (type < 6) {
        u32 attrs = cluster[2];

        switch (type) {
        case 0:
            if ((attrs & 0x68000000) == 0)
                return NULL;
            break;
        case 1:
        case 3:
            if ((attrs & 0x6c000000) == 0)
                return NULL;
            break;
        case 2:
            if ((attrs & 0x64000000) == 0)
                return NULL;
            break;
        case 4:
            if ((attrs & 0x6e000000) == 0 || (attrs & 0x4000) == 0)
                return NULL;
            break;
        case 5:
            if ((attrs & 0x6e000000) == 0)
                return NULL;
            break;
        }
        if (P4_STRCMP((const char *)cluster[3], P4_PS2_ATTRSET_NAME) != 0)
            return NULL;
    }
    if (P4_STRCMP((const char *)cluster[3], P4_PS2_ATTRSET_NAME) != 0)
        return node;

    d = func_004125d0(node);
    if (d == NULL) {
        d = p4_ps2allmat_new_initdata(node);
        if (d == NULL)
            return NULL;
    }
    d[type] = (u32)cluster;
    return node;
}

/* Sets the vertex layout of a triangle PS2AllMat node: `a` quad-words per vertex, `b` (>= 3)
 * vertices per primitive, `c` primitives per batch. Fails (NULL) when the node is NULL, any
 * argument is out of range, or init data already carries a kind in 0x1c. Kind flag 3. */
u8 *func_003d3460(u8 *node, s32 a, s32 b, s32 c)
{
    u32 *d;

    if (node == NULL || a <= 0 || b < 3 || c <= 0)
        return NULL;
    d = func_004125d0(node);
    if (d != NULL && (d[15] & 0x1c) != 0)
        return NULL;
    if (d == NULL) {
        d = p4_ps2allmat_new_initdata(node);
        if (d == NULL)
            return NULL;
    }
    d[13] = a * (c * 3);
    d[12] = b * a;
    d[14] = a;
    d[15] = 3;
    return node;
}

/* Same for a line PS2AllMat node: `b` (>= 2) vertices per primitive. Kind flag 12; fails when
 * init data already carries a kind in 0x13. */
u8 *func_003d35f0(u8 *node, s32 a, s32 b, s32 c)
{
    u32 *d;

    if (node == NULL || a <= 0 || b < 2 || c <= 0)
        return NULL;
    d = func_004125d0(node);
    if (d != NULL && (d[15] & 0x13) != 0)
        return NULL;
    if (d == NULL) {
        d = p4_ps2allmat_new_initdata(node);
        if (d == NULL)
            return NULL;
    }
    d[13] = a * (c * 2);
    d[12] = b * a;
    d[14] = a;
    d[15] = 12;
    return node;
}

/* Sets the vertex layout of a point PS2AllMat node: `a` quad-words per vertex, `b` vertices per
 * batch. Kind flag 16; fails when init data already carries a kind in 0xf. The retail code
 * leaves word 12 unchanged. func_003d3780 sits at its own INCLUDE_ASM marker in the decomp;
 * it is defined here beside the other generators. */
u8 *func_003d3780(u8 *node, s32 a, s32 b)
{
    u32 *d;

    if (node == NULL || a <= 0 || b <= 0)
        return NULL;
    d = func_004125d0(node);
    if (d != NULL && (d[15] & 0xf) != 0)
        return NULL;
    if (d == NULL) {
        d = p4_ps2allmat_new_initdata(node);
        if (d == NULL)
            return NULL;
    }
    d[13] = b * a;
    d[14] = a;
    d[15] = 16;
    return node;
}
