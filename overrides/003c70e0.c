/* func_003c70e0 (3248 bytes): the triangle stripifier (RenderWare TriStripStripTris). arg0 is the array of 0x14 byte triangle records (the first three halfwords are the vertex
 * indices), arg1 the triangle count, arg2 the head pointer of the strip list that receives the strips (a strip is {+0 = 16-bit index array, +4 = length, +8 = capacity, +0xC = next})
 * and arg3 is set when every start is tried with the three edge orders (the best is then used). Returns 1.
 * func_003c67b0(arg1, pools, &edge list, arg0) makes the triangle table (entries: +0 index, +4 / +8 / +0xC edges, +0x10 / +0x14 bin list links, +0x18 used, +0x1C used2, byte +0x20
 * adjacency; an edge has the two triangles at +8 and +4) and the triangles are put into four bin lists by adjacency. Two scratch strips of 2 * arg1 + 2 words (strip list entry from the pool at
 * *(0x008872E0 + *(s32 *)0x007647E0 + 4), slot 0x008873F8, tag 0x10502, array from slot 0x008873E8) are used. While triangles are left, a triangle of the bin 0 becomes a strip of its own;
 * else the first triangle of the lowest non empty bin starts a strip along an edge picked by the number of unused neighbours (EdgeAdjCount), the strip is followed (func_003c6a40
 * marks the triangle, func_003c6c10 follows an edge and returns the triangles added) forwards and, when the first edge still has an unused neighbour (used2 while the attempt number is below 4,
 * else used), backwards into the second scratch strip; the attempts (0, 1, 2 with the cached edge order + 0 / 1 / 2 mod 3, then the best one) restore the used2 flags each time. The result
 * is copied into a new strip (tag 0x30502) put at the head of arg2's list: forwards part preceded by the reversed backwards part (the backwards strip gets its length made even by repeating
 * the entry two before the end). Finally both scratch strips are freed and func_003c66c0(arg1, pools, edge list, table) destroys the tables. */

extern s32 p4c_003c66c0_003c70e0(u32 n, u32 *p, u8 *e, u32 *a) __asm__("func_003c66c0");
extern u32 **p4c_003c67b0_003c70e0(u32 n, u32 *pools, u32 el, u16 *t) __asm__("func_003c67b0");
extern void p4c_003c6a40_003c70e0(u8 *t, u8 *b, u32 c) __asm__("func_003c6a40");
extern u32 p4c_003c6c10_003c70e0(u8 *s, u8 *e, u8 *b, u8 *t, u32 c) __asm__("func_003c6c10");
extern void p4c_0043f810_003c70e0(void *d, void *s, u32 n) __asm__("func_0043f810");

typedef u8 *(*P4PoolAllocFn_003c70e0)(u32 pool, u32 tag);
typedef u8 *(*P4AllocFn_003c70e0)(u32 size, u32 tag);
typedef void (*P4FreeFn_003c70e0)(void *p);
typedef void (*P4PoolFreeFn_003c70e0)(u32 pool, u32 obj);

static u32 adj_003c70e0(u8 *e, u32 off)
{
    u8 *a = *(u8 **)(e + 8);
    u8 *b = *(u8 **)(e + 4);
    u32 n = 0;

    if (a != 0 && *(u32 *)(a + off) == 0) {
        n++;
    }
    if (b != 0 && *(u32 *)(b + off) == 0) {
        n++;
    }
    return n;
}

s32 func_003c70e0(u8 *arg0, u32 arg1, u32 *arg2, u32 arg3)
{
    u32 pools[2];
    u32 edgelist = 0;
    u8 *head[4];
    u8 **arr;
    u8 *bs;
    u8 *rs;
    u8 *ns;
    u8 *nextEdge = 0;
    u8 *firstEdge = 0;
    u32 trisUsed = 0;
    u32 trisUsedTemp = 0;
    u32 bestSize;
    u32 bestOff = 0;
    u32 cacheOff = 0;
    u32 off = 0;
    u32 cur;
    u32 i;
    u32 j;
    u32 fl;
    u32 sz;

    pools[0] = 0;
    pools[1] = 0;
    head[0] = head[1] = head[2] = head[3] = 0;
    arr = (u8 **)p4c_003c67b0_003c70e0(arg1, pools, (u32)&edgelist, (u16 *)arg0);
    for (i = 0; i < arg1; i++) {
        u8 *e = arr[i];
        u32 a = *(u8 *)(e + 0x20);

        *(u8 **)(e + 0x10) = head[a];
        if (*(u8 **)(e + 0x10) != 0) {
            *(u8 **)(*(u8 **)(e + 0x10) + 0x14) = e;
        }
        head[a] = e;
        *(u32 *)(e + 0x14) = 0;
    }
    fl = *(u32 *)(0x008872E0 + *(s32 *)0x007647E0 + 4);
    bs = (*(P4PoolAllocFn_003c70e0 *)0x008873F8)(fl, 0x10502);
    sz = (arg1 << 1) + 2;
    *(u32 *)(bs + 8) = sz;
    *(u32 *)(bs + 4) = 0;
    *(u32 *)bs = (u32)(*(P4AllocFn_003c70e0 *)0x008873E8)(*(u32 *)(bs + 8) << 2, 0x10502);
    fl = *(u32 *)(0x008872E0 + *(s32 *)0x007647E0 + 4);
    rs = (*(P4PoolAllocFn_003c70e0 *)0x008873F8)(fl, 0x10502);
    *(u32 *)(rs + 8) = sz;
    *(u32 *)(rs + 4) = 0;
    *(u32 *)rs = (u32)(*(P4AllocFn_003c70e0 *)0x008873E8)(*(u32 *)(rs + 8) << 2, 0x10502);

    while (trisUsed < arg1) {
        bestSize = 0;
        cur = (arg3 == 0) ? 3 : 0;
        if (head[0] != 0) {
            u8 *h = head[0];
            u32 tri = *(u32 *)h;
            u16 *p;
            u8 *nx;

            fl = *(u32 *)(0x008872E0 + *(s32 *)0x007647E0 + 4);
            ns = (*(P4PoolAllocFn_003c70e0 *)0x008873F8)(fl, 0x30502);
            *(u32 *)(ns + 0xC) = *arg2;
            *arg2 = (u32)ns;
            *(u32 *)(ns + 8) = 3;
            *(u32 *)(ns + 4) = 3;
            *(u32 *)ns = (u32)(*(P4AllocFn_003c70e0 *)0x008873E8)(0xC, 0x30502);
            p = (u16 *)(arg0 + tri * 0x14);
            ((u16 *)*(u32 *)ns)[0] = p[0];
            ((u16 *)*(u32 *)ns)[1] = p[1];
            ((u16 *)*(u32 *)ns)[2] = p[2];
            *(u32 *)(head[0] + 0x18) = 1;
            *(u32 *)(head[0] + 0x1C) = 1;
            nx = *(u8 **)(head[0] + 0x10);
            head[0] = nx;
            if (nx != 0) {
                *(u32 *)(nx + 0x14) = 0;
            }
            trisUsed++;
        } else {
            u32 c0;
            u32 c1;
            u32 c2;
            u32 o0;
            u32 o1;
            u32 o2;
            u8 *h;

            i = 1;
            while (head[i] == 0) {
                i++;
            }
            h = head[i];
            if (adj_003c70e0(*(u8 **)(h + 0xC), 0x18) >= 2 && adj_003c70e0(*(u8 **)(h + 8), 0x18) >= 2) {
                bestOff = 1;
            } else if (adj_003c70e0(*(u8 **)(h + 4), 0x18) >= 2 && adj_003c70e0(*(u8 **)(h + 0xC), 0x18) >= 2) {
                bestOff = 2;
            } else if (adj_003c70e0(*(u8 **)(h + 8), 0x18) >= 2 && adj_003c70e0(*(u8 **)(h + 4), 0x18) >= 2) {
                bestOff = 0;
            } else {
                c0 = adj_003c70e0(*(u8 **)(h + 4), 0x18);
                c1 = adj_003c70e0(*(u8 **)(h + 8), 0x18);
                c2 = adj_003c70e0(*(u8 **)(h + 0xC), 0x18);
                if (c0 > c1) {
                    bestOff = (c0 > c2) ? 2 : 1;
                } else {
                    bestOff = (c1 > c2) ? 0 : 1;
                }
            }
            trisUsedTemp = trisUsed;
            cacheOff = bestOff;
            o0 = (cacheOff + 0) % 3;
            o1 = (cacheOff + 1) % 3;
            o2 = (cacheOff + 2) % 3;
          restart:
            for (j = 0; j < arg1; j++) {
                *(u32 *)(arr[j] + 0x1C) = *(u32 *)(arr[j] + 0x18);
            }
            switch (cur++) {
            case 0:
                off = o0;
                break;
            case 1:
                off = o1;
                break;
            case 2:
                off = o2;
                break;
            default:
                off = bestOff;
                break;
            }
            h = head[i];
            switch (off) {
            case 0:
                nextEdge = *(u8 **)(h + 8);
                firstEdge = *(u8 **)(h + 4);
                break;
            case 1:
                nextEdge = *(u8 **)(h + 0xC);
                firstEdge = *(u8 **)(h + 8);
                break;
            case 2:
                nextEdge = *(u8 **)(h + 4);
                firstEdge = *(u8 **)(h + 0xC);
                break;
            }
            {
                u16 *p = (u16 *)(arg0 + *(u32 *)head[i] * 0x14);
                u16 *s = *(u16 **)bs;

                s[0] = p[off % 3];
                s[1] = p[(off + 1) % 3];
                s[2] = p[(off + 2) % 3];
                *(u32 *)(bs + 4) = 3;
            }
            p4c_003c6a40_003c70e0(head[i], (u8 *)head, cur);
            trisUsed = trisUsedTemp + 1;
            trisUsed += p4c_003c6c10_003c70e0(bs, nextEdge, (u8 *)head, arg0, cur);
            {
                u32 test;

                if (cur < 4) {
                    test = adj_003c70e0(firstEdge, 0x1C) > 0;
                } else {
                    test = adj_003c70e0(firstEdge, 0x18) > 0;
                }
                if (test) {
                    u16 *bsp = *(u16 **)bs;
                    u16 *rsp = *(u16 **)rs;

                    rsp[0] = bsp[1];
                    rsp[1] = bsp[0];
                    *(u32 *)(rs + 4) = 2;
                    trisUsed += p4c_003c6c10_003c70e0(rs, firstEdge, (u8 *)head, arg0, cur);
                    if (*(u32 *)(rs + 4) & 1) {
                        u16 *r2 = *(u16 **)rs;

                        r2[*(u32 *)(rs + 4)] = r2[*(u32 *)(rs + 4) - 2];
                        *(u32 *)(rs + 4) = *(u32 *)(rs + 4) + 1;
                    }
                    if (bestSize < trisUsed) {
                        bestOff = off;
                        bestSize = trisUsed;
                    }
                    if (cur < 4) {
                        goto restart;
                    }
                    fl = *(u32 *)(0x008872E0 + *(s32 *)0x007647E0 + 4);
                    ns = (*(P4PoolAllocFn_003c70e0 *)0x008873F8)(fl, 0x30502);
                    *(u32 *)(ns + 0xC) = *arg2;
                    *arg2 = (u32)ns;
                    *(u32 *)(ns + 8) = *(u32 *)(bs + 4) + *(u32 *)(rs + 4) - 2;
                    *(u32 *)(ns + 4) = 0;
                    *(u32 *)ns = (u32)(*(P4AllocFn_003c70e0 *)0x008873E8)(*(u32 *)(ns + 8) << 2, 0x30502);
                    while (*(u32 *)(rs + 4) > 2) {
                        ((u16 *)*(u32 *)ns)[*(u32 *)(ns + 4)] = ((u16 *)*(u32 *)rs)[*(u32 *)(rs + 4) - 1];
                        *(u32 *)(ns + 4) = *(u32 *)(ns + 4) + 1;
                        *(u32 *)(rs + 4) = *(u32 *)(rs + 4) - 1;
                    }
                    p4c_0043f810_003c70e0((u16 *)*(u32 *)ns + *(u32 *)(ns + 4), (void *)*(u32 *)bs, *(u32 *)(bs + 4) << 2);
                    *(u32 *)(ns + 4) = *(u32 *)(ns + 8);
                } else {
                    if (bestSize < trisUsed) {
                        bestOff = off;
                        bestSize = trisUsed;
                    }
                    if (cur < 4) {
                        goto restart;
                    }
                    fl = *(u32 *)(0x008872E0 + *(s32 *)0x007647E0 + 4);
                    ns = (*(P4PoolAllocFn_003c70e0 *)0x008873F8)(fl, 0x30502);
                    *(u32 *)(ns + 0xC) = *arg2;
                    *arg2 = (u32)ns;
                    *(u32 *)(ns + 8) = *(u32 *)(bs + 4);
                    *(u32 *)(ns + 4) = *(u32 *)(bs + 4);
                    *(u32 *)ns = (u32)(*(P4AllocFn_003c70e0 *)0x008873E8)(*(u32 *)(bs + 4) << 2, 0x30502);
                    p4c_0043f810_003c70e0((void *)*(u32 *)ns, (void *)*(u32 *)bs, *(u32 *)(bs + 4) << 2);
                }
            }
        }
    }
    (*(P4FreeFn_003c70e0 *)0x008873EC)((void *)*(u32 *)rs);
    *(u32 *)rs = 0;
    fl = *(u32 *)(0x008872E0 + *(s32 *)0x007647E0 + 4);
    (*(P4PoolFreeFn_003c70e0 *)0x008873FC)(fl, (u32)rs);
    (*(P4FreeFn_003c70e0 *)0x008873EC)((void *)*(u32 *)bs);
    *(u32 *)bs = 0;
    fl = *(u32 *)(0x008872E0 + *(s32 *)0x007647E0 + 4);
    (*(P4PoolFreeFn_003c70e0 *)0x008873FC)(fl, (u32)bs);
    p4c_003c66c0_003c70e0(arg1, pools, (u8 *)edgelist, (u32 *)arr);
    return 1;
}
