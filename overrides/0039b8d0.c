/* func_0039b8d0: appends the 20-byte entry `item` to the list `list`
 * (entries are keyed by word 2). A duplicate key reports error 2 and returns.
 * A full list grows through the function pointer in retail slot 0x008873F0
 * (flags 0x01040131) to twice its capacity. When list->flags is non-zero,
 * the entry's type (word 3) selects a follow-up: 2 calls func_0039b720,
 * 1 calls func_0039bbd0 and stores its result in word 4, and any other type
 * reports error 1. The unit declares this function as void (s32, s32), and
 * its callers ignore any result, so the retail list/item pointers arrive as
 * s32 and are cast here. */

typedef struct P4ListEntry {
    u32 w0;
    u32 w1;
    u32 w2;
    s32 w3;
    u32 w4;
} P4ListEntry;

typedef struct P4List {
    P4ListEntry *items;
    u32 count;      /* retail compares the loop index unsigned (sltu), so a count of 0x80000000 or more is a long list, not none */
    s32 capacity;
    s32 flags;
} P4List;

typedef struct P4RwError {
    u32 pluginID;
    s32 errorCode;
} P4RwError;

typedef void *(*P4ListAllocFn)(void *ptr, u32 size, u32 flags);

extern void *func_0039bbd0(u32 a0);
extern void func_0039b720(u32 *entry, u32 *list);

void func_0039b8d0(s32 list_arg, s32 item_arg)
{
    P4List *list = (P4List *)list_arg;
    P4ListEntry *item = (P4ListEntry *)item_arg;
    P4RwError err;
    P4ListEntry *slot;
    P4ListAllocFn alloc;
    void *res;
    u32 i;

    for (i = 0; i < list->count; i++) {
        if (list->items[i].w2 == item->w2) {
            break;
        }
    }
    if (i < list->count) {
        err.pluginID = 305;
        err.errorCode = func_003df590(2);
        func_003df4d0((void *)&err);
        return;
    }

    if (list->count == list->capacity) {
        list->capacity = list->capacity * 2;
        alloc = *(P4ListAllocFn *)0x008873F0;
        list->items = (P4ListEntry *)alloc(list->items, list->capacity * 20, 0x01040131);
    }

    i = list->count;
    list->count = i + 1;
    slot = &list->items[i];
    slot->w0 = item->w0;
    slot->w1 = item->w1;
    slot->w2 = item->w2;
    slot->w3 = item->w3;
    slot->w4 = 0;

    if (list->flags != 0) {
        if (slot->w3 == 2) {
            func_0039b720((u32 *)slot, (u32 *)list);
        } else if (slot->w3 == 1) {
            res = func_0039bbd0(slot->w0);
            slot->w4 = (u32)res;
            *(u32 *)((u8 *)res + 48) = slot->w2;
        } else {
            err.pluginID = 305;
            err.errorCode = func_003df590(1);
            func_003df4d0((void *)&err);
        }
    }
}
