/* SPDX-License-Identifier: LicenseRef-reconstructed */
/* CRI stream joint "universal" (SJUNI), translated from the retail asm 0x004EEB20..0x004EF700.
 *
 * The retail joint is a chunk-chain container: four lines (id 0..3), each a singly linked list of
 * {data, len} chunks, drawn from a pool of 16-byte nodes that the caller supplies as a work
 * buffer. Every vtable entry is a lock wrapper (SJCRS_Lock = func_004ed450, unlock =
 * func_004ed468) around a checked body: a NULL handle or an unused slot reports an E20040902xx
 * error through the SJ error forwarder (func_004ed480) and does nothing else.
 *
 * The decomp's src/cri/re4/sj_uni.c models a different joint (a flat buffer; its bodies are
 * dead-stripped in the reference game), so it is excluded and this file is the only SJUNI.
 *
 * Object (0x30 bytes): +0 vtbl, +4 used, +5 mode, +8 uuid, +0xc node work, +0x10 node count,
 * +0x14 free pool, +0x18..+0x24 lines, +0x28 error callback, +0x2c error object.
 * Chain node (16 bytes): +0 next, +8 chunk data, +0xc chunk length.
 *
 * The objects and nodes are guest memory (port-owned storage, the caller's work buffer): every
 * pointer in them is a 4-byte guest address, the error callback is a guest code address. The
 * functions take and return host pointers (register ABI) and convert at the guest-memory edge.
 */
#include <stdint.h>
#include <string.h>

#include "p4pc/gmem.h"

typedef struct {
    uint32_t data;
    int32_t len;
} P4SjCk;

typedef struct P4SjCkcn {
    uint32_t next;
    int32_t unused;
    P4SjCk ck;
} P4SjCkcn;

typedef struct {
    uint32_t vtbl;
    uint8_t used;
    uint8_t mode;
    uint8_t pad[2];
    uint32_t uuid;
    uint32_t ckcnwk;
    int32_t nckcn;
    uint32_t pool;
    uint32_t lin[4];
    uint32_t errfunc;
    uint32_t errobj;
} P4SjUni;

_Static_assert(sizeof(P4SjUni) == 0x30, "SJUNI object is 0x30 bytes");
_Static_assert(offsetof(P4SjUni, uuid) == 0x08, "SJUNI uuid offset");
_Static_assert(offsetof(P4SjUni, ckcnwk) == 0x0c, "SJUNI ckcnwk offset");
_Static_assert(offsetof(P4SjUni, nckcn) == 0x10, "SJUNI nckcn offset");
_Static_assert(offsetof(P4SjUni, pool) == 0x14, "SJUNI pool offset");
_Static_assert(offsetof(P4SjUni, lin) == 0x18, "SJUNI lines offset");
_Static_assert(offsetof(P4SjUni, errfunc) == 0x28, "SJUNI errfunc offset");
_Static_assert(offsetof(P4SjUni, errobj) == 0x2c, "SJUNI errobj offset");
_Static_assert(sizeof(P4SjCk) == 8, "chunk is 8 bytes");
_Static_assert(sizeof(P4SjCkcn) == 16, "chain node is 16 bytes");
_Static_assert(offsetof(P4SjCkcn, ck) == 8, "chain node chunk offset");

#define P4_SJUNI_NUM_OBJ 64
#define P4_SJUNI_UUID 0x0075cec8u /* retail rodata */
#define P4_SJUNI_VTBL 0x00741ff0u /* SJ_IF at its retail address (config/port_symbols.txt); its slots are the wrappers below */

extern void func_004ed450(void); /* SJCRS_Lock */
extern void func_004ed468(void); /* SJCRS_Unlock */
extern void func_004ed480(uint32_t msg); /* SJ error forwarder (decomp: the message's guest address) */
extern void SJ_SplitChunk(P4SjCk *ck, int32_t nbyte, P4SjCk *head, P4SjCk *tail);

/* The 64 objects, in port-owned guest memory: the handle the game gets is the object's address. */
static P4SjUni *sjuni_objs;
static int32_t sjuni_init_cnt;

static P4SjUni *sjuni_obj(void)
{
    if (sjuni_objs == NULL) {
        sjuni_objs = (P4SjUni *)P4_G2H(p4_gport_alloc(sizeof(P4SjUni) * P4_SJUNI_NUM_OBJ, 16));
    }
    return sjuni_objs;
}

#define SJ_MSG_NULL " : NULL pointer is specified."
#define SJ_MSG_INVALID " : Specified handle is invalid."

#define NODE(a) ((P4SjCkcn *)P4_G2H(a))

/* 0x004eead8: "<code><message>" to the SJ error forwarder. */
static void sjuni_err(const char *code, const char *msg)
{
    char buf[64];

    strcpy(buf, code);
    strcat(buf, msg);
    {   /* the message is built per call: a fresh guest copy (the string pool interns literals by address) */
        size_t n = strlen(buf) + 1;
        uint32_t at = p4_gport_alloc((uint32_t)n, 1);

        memcpy(P4_G2H(at), buf, n);
        func_004ed480(at);
    }
}

typedef void (*SjUniErrFn)(void *obj, int32_t code);

#define SJUNI_ERRFN(uni) \
    do { \
        if ((uni)->errfunc != 0) { \
            ((SjUniErrFn)p4_fn_g2h_sig((uni)->errfunc, P4_SIG(2, 0, 1, 0, 0, 0, 0, 0)))(P4_G2H((uni)->errobj), -3); \
        } \
    } while (0)

/* 0x004eeab8: default error callback. */
static void sjuni_default_err(void *obj, int32_t code)
{
    (void)obj;
    (void)code;
    func_004ed480(p4_gport_str("SJUNI Error"));
}

/* ---- SJ library init level, 0x004ed3e8 / 0x004ed420 ------------------------------------------
 * Counted by every SJ module's Init/Finish wrapper. 0x0073dac8 is the level; the first level
 * clears 0x0073dac0, the last level clears it again. Nothing in the decomp reads either word. */
#define P4_GW(a) P4_G32(a)

void func_004ed3e8(void)
{
    P4_GW(0x0073dac8) = P4_GW(0x0073dac8) + 1;
    if (P4_GW(0x0073dac8) == 1) {
        P4_GW(0x0073dac0) = 0;
    }
}

void func_004ed420(void)
{
    P4_GW(0x0073dac8) = P4_GW(0x0073dac8) - 1;
    if (P4_GW(0x0073dac8) == 0) {
        P4_GW(0x0073dac0) = 0;
    }
}

/* ---- init / finish ----------------------------------------------------------------------- */

/* 0x004eeb20 (inner 0x004eeb50). */
void SJUNI_Init(void)
{
    func_004ed3e8();
    func_004ed450();
    if (sjuni_init_cnt == 0) {
        memset(sjuni_obj(), 0, sizeof(P4SjUni) * P4_SJUNI_NUM_OBJ);
    }
    sjuni_init_cnt++;
    func_004ed468();
}

/* 0x004eeba0 (inner 0x004eebd0). */
void SJUNI_Finish(void)
{
    func_004ed450();
    sjuni_init_cnt--;
    if (sjuni_init_cnt == 0) {
        memset(sjuni_obj(), 0, sizeof(P4SjUni) * P4_SJUNI_NUM_OBJ);
    }
    func_004ed468();
    func_004ed420();
}

/* ---- the SJ_IF bodies ---------------------------------------------------------------------- */

static void sjuni_reset(P4SjUni *uni)
{
    P4SjCkcn *node;
    int32_t i;

    if (uni == 0) {
        sjuni_err("E2004090267", SJ_MSG_NULL);
        return;
    }
    if (uni->used == 0) {
        sjuni_err("E2004090268", SJ_MSG_INVALID);
        return;
    }
    node = NODE(uni->ckcnwk);
    uni->pool = uni->ckcnwk;
    for (i = 0; i < uni->nckcn - 1; i++) {
        node[i].next = P4_H2G(&node[i + 1]);
        node[i].ck.data = 0;
        node[i].ck.len = 0;
    }
    node[i].next = 0;
    node[i].ck.data = 0;
    node[i].ck.len = 0;
    for (i = 0; i < 4; i++) {
        uni->lin[i] = 0;
    }
}

/* 0x004eec78 inner, 0x004eec20 wrapper: SJUNI_Create(mode, work, wksize). Returns the joint or
 * NULL when all 64 slots are taken. */
void *SJUNI_Create(int32_t mode, void *work, int32_t wksize)
{
    P4SjUni *objs = sjuni_obj();
    P4SjUni *uni;
    int32_t i;

    func_004ed450();
    for (i = 0; i < P4_SJUNI_NUM_OBJ; i++) {
        if (objs[i].used == 0) {
            break;
        }
    }
    uni = 0;
    if (i != P4_SJUNI_NUM_OBJ) {
        uni = &objs[i];
        uni->used = 1;
        uni->vtbl = P4_SJUNI_VTBL;
        uni->mode = (uint8_t)mode;
        uni->uuid = P4_SJUNI_UUID;
        uni->ckcnwk = P4_H2G(work);
        uni->nckcn = wksize / 16;
        uni->errfunc = p4_fn_h2g_sig(sjuni_default_err, "vpi");
        uni->errobj = P4_H2G(uni);
        sjuni_reset(uni);
    }
    func_004ed468();
    return uni;
}

/* 0x004eed60: SJUNI_Destroy. */
void func_004eed60(P4SjUni *uni)
{
    func_004ed450();
    if (uni == 0) {
        sjuni_err("E2004090261", SJ_MSG_NULL);
    } else if (uni->used == 0) {
        sjuni_err("E2004090262", SJ_MSG_INVALID);
    } else {
        memset(uni, 0, sizeof(*uni));
        uni->used = 0;
    }
    func_004ed468();
}

/* 0x004eee18: SJUNI_GetUuid. */
const void *func_004eee18(P4SjUni *uni)
{
    const void *ret;

    func_004ed450();
    if (uni == 0) {
        sjuni_err("E2004090263", SJ_MSG_NULL);
        ret = 0;
    } else if (uni->used == 0) {
        sjuni_err("E2004090264", SJ_MSG_INVALID);
        ret = 0;
    } else {
        ret = P4_G2H(uni->uuid);
    }
    func_004ed468();
    return ret;
}

/* 0x004eeea8: SJUNI_EntryErrFunc. */
void func_004eeea8(P4SjUni *uni, void (*func)(void *obj, int32_t code), void *obj)
{
    func_004ed450();
    if (uni == 0) {
        sjuni_err("E2004090265", SJ_MSG_NULL);
    } else if (uni->used == 0) {
        sjuni_err("E2004090266", SJ_MSG_INVALID);
    } else {
        uni->errobj = P4_H2G(obj);
        uni->errfunc = p4_fn_h2g_sig(func, "vpi");
    }
    func_004ed468();
}

/* 0x004eef58: SJUNI_Reset. */
void func_004eef58(P4SjUni *uni)
{
    func_004ed450();
    sjuni_reset(uni);
    func_004ed468();
}

/* 0x004ef060: SJUNI_GetNumData. Bytes queued on a line. */
int32_t func_004ef060(P4SjUni *uni, uint32_t id)
{
    int32_t ret = 0;

    func_004ed450();
    if (uni == 0) {
        sjuni_err("E2004090269", SJ_MSG_NULL);
    } else if (uni->used == 0) {
        sjuni_err("E2004090270", SJ_MSG_INVALID);
    } else if (id < 4) {
        P4SjCkcn *n;

        for (n = NODE(uni->lin[id]); n != 0; n = NODE(n->next)) {
            ret += n->ck.len;
        }
    } else {
        SJUNI_ERRFN(uni);
    }
    func_004ed468();
    return ret;
}

/* 0x004ef160: SJUNI_GetChunk(uni, id, nbyte, ck). Takes the head chunk when it fits in nbyte; in
 * mode 1 a longer head chunk is split and the first nbyte bytes are taken. */
void func_004ef160(P4SjUni *uni, uint32_t id, int32_t nbyte, P4SjCk *ck)
{
    func_004ed450();
    if (uni == 0) {
        sjuni_err("E2004090271", SJ_MSG_NULL);
    } else if (uni->used == 0) {
        sjuni_err("E2004090272", SJ_MSG_INVALID);
    } else {
        ck->len = 0;
        ck->data = 0;
        if (id >= 4) {
            SJUNI_ERRFN(uni);
        } else {
            P4SjCkcn *head = NODE(uni->lin[id]);

            if (head != 0) {
                P4SjCk ck0 = head->ck;

                if (ck0.len <= nbyte) {
                    *ck = ck0;
                    uni->lin[id] = head->next;
                    head->next = uni->pool;
                    uni->pool = P4_H2G(head);
                } else if (uni->mode == 1) {
                    P4SjCk ck1;

                    SJ_SplitChunk(&ck0, nbyte, &ck0, &ck1);
                    *ck = ck0;
                    head->ck = ck1;
                }
            }
        }
    }
    func_004ed468();
}

/* 0x004ef310: SJUNI_PutChunk(uni, id, ck). Appends to a line; in mode 1 a chunk that continues
 * the tail chunk extends it instead of taking a node. */
void func_004ef310(P4SjUni *uni, uint32_t id, P4SjCk *ck)
{
    func_004ed450();
    if (uni == 0) {
        sjuni_err("E2004090273", SJ_MSG_NULL);
    } else if (uni->used == 0) {
        sjuni_err("E2004090274", SJ_MSG_INVALID);
    } else if (id >= 4) {
        SJUNI_ERRFN(uni);
    } else if (ck->len >= 1 && ck->data != 0) {
        uint32_t *link = &uni->lin[id];
        P4SjCkcn *tail = 0;
        P4SjCkcn *n;

        for (n = NODE(*link); n != 0; n = NODE(n->next)) {
            tail = n;
            link = &n->next;
        }
        if (uni->mode == 1 && tail != 0 && tail->ck.data + (uint32_t)tail->ck.len == ck->data) {
            tail->ck.len += ck->len;
        } else {
            n = NODE(uni->pool);
            if (n != 0) {
                uni->pool = n->next;
                n->next = 0;
                n->ck = *ck;
                *link = P4_H2G(n);
            } else {
                SJUNI_ERRFN(uni);
            }
        }
    }
    func_004ed468();
}

/* 0x004ef488: SJUNI_UngetChunk(uni, id, ck). Pushes a chunk back on the head of a line; in mode 1
 * a chunk that precedes the head chunk extends it. */
void func_004ef488(P4SjUni *uni, uint32_t id, P4SjCk *ck)
{
    func_004ed450();
    if (uni == 0) {
        sjuni_err("E2004090275", SJ_MSG_NULL);
    } else if (uni->used == 0) {
        sjuni_err("E2004090276", SJ_MSG_INVALID);
    } else if (id >= 4) {
        SJUNI_ERRFN(uni);
    } else if (ck->len >= 1 && ck->data != 0) {
        P4SjCkcn *head = NODE(uni->lin[id]);

        if (uni->mode == 1 && head != 0 && ck->data + (uint32_t)ck->len == head->ck.data) {
            head->ck.data = ck->data;
            head->ck.len += ck->len;
        } else {
            P4SjCkcn *n = NODE(uni->pool);

            if (n != 0) {
                uni->pool = n->next;
                n->next = 0;
                n->ck = *ck;
                n->next = uni->lin[id];
                uni->lin[id] = P4_H2G(n);
            } else {
                SJUNI_ERRFN(uni);
            }
        }
    }
    func_004ed468();
}

/* 0x004ef5f8: SJUNI_IsGetChunk(uni, id, nbyte, rbyte). *rbyte is the head chunk length; mode 1
 * accepts a head chunk of at least nbyte bytes, other modes need exactly nbyte. */
int32_t func_004ef5f8(P4SjUni *uni, uint32_t id, int32_t nbyte, int32_t *rbyte)
{
    int32_t ret = 0;

    func_004ed450();
    if (uni == 0) {
        sjuni_err("E2004090277", SJ_MSG_NULL);
    } else if (uni->used == 0) {
        sjuni_err("E2004090278", SJ_MSG_INVALID);
    } else {
        *rbyte = 0;
        if (id < 4) {
            P4SjCkcn *head = NODE(uni->lin[id]);

            if (head != 0) {
                *rbyte = head->ck.len;
                if (uni->mode == 1) {
                    ret = !(head->ck.len < nbyte);
                } else {
                    ret = (head->ck.len == nbyte);
                }
            }
        } else {
            SJUNI_ERRFN(uni);
        }
    }
    func_004ed468();
    return ret;
}
