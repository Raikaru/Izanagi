/* Mod rows in native menus (include/p4pc/native_rows.h). Velvet Room fusion
 * command cards are the first host.
 *
 * Rows: FclCombineWork's commandRows[9] (10 bytes each at +0xC4: state0,
 * state1, id, x, y) plus one port-owned guest side row, the tenth. The sync
 * inserts the mod card after the spreads, before the three fixed cards
 * (Forecast, Talk, Back, which every row builder adds last), so display order
 * stays array order and the cursor (+0xB4) and count (+0xB5) keep their
 * meaning up to ten.
 *
 * Art: the card list draws fcl pool slots through the pool's single pack
 * (FclSpritePool.resource at +0). The loaded facility/velvet01.spr pack has 26
 * rasters and 489 records; at Velvet init slots 0..499 take their own index as
 * record id. While the mod card shows, the pool draws from a port-owned copy
 * of that pack: the same rasters plus the card's, and a copy of the 489
 * records followed by the card's records at 0x1E9 (symbol), 0x1EA (its mark:
 * the same rectangle, as retail symbol pairs are), 0x1EB (large label) and
 * 0x1EC (small label). Slot 0x1ED is the card base (record 0xDC, as the menu's
 * creation resets slots 0x20D..0x215). The retail pack and records are never
 * written.
 *
 * Page: confirming the card creates the Velvet task's Config child
 * (cmpConfigCreate 0x0035c690) and hosts the row's page in it
 * (p4_settings_native_host); the root waits until cmpConfigIsStepDone
 * (0x0035c7d0), destroys the child (kwlnTaskDestroyWithHierarchy 0x00452080)
 * and rebuilds the cards. */
#include "p4pc/native_rows.h"
#include "p4pc/ui_overlay.h"
#include "p4pc/settings_native.h"
#include "p4pc/gmem.h"
#include "p4pc/guest.h"
#include <stdio.h>
#include <string.h>

extern void fclSpriteReset(int16_t index, int16_t spriteId);
extern int32_t cmpConfigCreate(void *parent, int32_t mode);
extern int32_t cmpConfigIsStepDone(void *task);
extern uint8_t kwlnTaskDestroyWithHierarchy(void *task);

enum { ROW_BYTES = 10, RETAIL_ROWS = 9, ROWS_AT = 0xC4, CURSOR_AT = 0xB4, COUNT_AT = 0xB5,
       POOL_TASK_AT = 0x24C, TASK_WORK_AT = 0x38 };
enum { PACK_SIZE = 0x240, RECORD_SIZE = 0x80, RETAIL_RECORDS = 0x1E9, MOD_RECORDS = 4,
       RASTERS_AT = 0x104, RECORDS_AT = 0x204, RECORD_COUNT_AT = 0x216, HEADER_RASTERS_AT = 0x22C,
       HEADER_RECORDS_AT = 0x22E, MAX_RASTERS = 32 };
/* CONFIG_MODE 1: cmpConfigCreate's title mode drives its own frame (mode 0
 * waits on Camp's nLine), which p4_settings_native_host requires. */
enum { SLOT_SYMBOL = 0x1E9, SLOT_MARK = 0x1EA, SLOT_LARGE = 0x1EB, SLOT_SMALL = 0x1EC, SLOT_FRAME = 0x1ED,
       CARD_BASE_RECORD = 0xDC, FIXED_ROWS = 3, KEY_MAX = 48, PAGE_MAX = 128, CONFIG_MODE = 1 };

typedef struct Row {
    uint32_t owner;
    int used, enabled;
    char key[KEY_MAX], page[PAGE_MAX];
    P4NativeRowArt art;
} Row;

static Row velvet_row;        /* the Velvet host's single mod card */
static uint32_t side_address, pack_address, records_address;
static uint32_t active_work;  /* the FclCombineWork whose current list has the mod card */
static uint32_t retail_pack;  /* the pack the extended copy was made from */
static uint32_t config_task;  /* the hosted page's Config child */
static int return_to_card;    /* the next layout puts the cursor on the mod card */

static uint8_t *side(void)
{
    if (!side_address) side_address = p4_gport_alloc(16, 16);
    return side_address ? P4_GPTR(uint8_t, side_address) : NULL;
}

static int art_valid(const P4NativeRowArt *art)
{
    uint32_t raster;
    int rect[4];
    return !p4_ui_sprite_rect(art->symbol, &raster, rect) && rect[2] == 72 && rect[3] == 77 &&
        !p4_ui_sprite_rect(art->label, &raster, rect) && rect[2] <= 99 && rect[3] <= 31 &&
        !p4_ui_sprite_rect(art->small, &raster, rect) && rect[2] <= 72 && rect[3] <= 18;
}

int p4_native_rows_add(int host, uint32_t owner, const char *key, const P4NativeRowArt *art, const char *page)
{
    if (host != P4_NATIVE_ROWS_VELVET_FUSION || !key || !art || !page || velvet_row.used || config_task) return -1;
    size_t key_length = strnlen(key, KEY_MAX), page_length = strnlen(page, PAGE_MAX);
    if (!key_length || key_length == KEY_MAX || !page_length || page_length == PAGE_MAX || !art_valid(art)) return -1;
    velvet_row = (Row){.owner = owner, .used = 1, .enabled = 1, .art = *art};
    memcpy(velvet_row.key, key, key_length + 1);
    memcpy(velvet_row.page, page, page_length + 1);
    return 0;
}

int p4_native_rows_enable(int handle, int enabled)
{
    if (handle != 0 || !velvet_row.used) return -1;
    velvet_row.enabled = !!enabled;
    return 0;
}

int p4_native_rows_remove(int handle)
{
    if (handle != 0 || !velvet_row.used || config_task) return -1;
    velvet_row = (Row){0};
    return 0;
}

void p4_native_rows_remove_owner(uint32_t owner)
{
    if (!velvet_row.used || velvet_row.owner != owner) return;
    if (config_task) {
        /* The page's owner is going away while its page is open: close the
         * Config child first, as the root would on the page's own close. */
        kwlnTaskDestroyWithHierarchy(P4_G2H(config_task));
        config_task = 0;
    }
    velvet_row = (Row){0};
}

int p4_native_rows_present(int host)
{
    return host == P4_NATIVE_ROWS_VELVET_FUSION && active_work != 0;
}

int p4_native_rows_selected(int host)
{
    return host == P4_NATIVE_ROWS_VELVET_FUSION && config_task ? 0 : -1;
}

/* ---- Velvet host */

uint8_t *p4_native_rows_velvet_row(uint8_t *work, int index)
{
    if (index < RETAIL_ROWS) return work + ROWS_AT + index * ROW_BYTES;
    return side();
}

int p4_native_rows_velvet_active(const uint8_t *work)
{
    return active_work && active_work == P4_H2G(work);
}

int16_t p4_native_rows_velvet_symbol(int entry)
{
    if (entry == P4_NATIVE_ROWS_VELVET_ENTRY) return SLOT_SYMBOL;
    return (int16_t)(entry == 6 ? 0xEF : entry == 7 ? 0xEB : entry == 8 ? 0xED : entry * 2 + 0xDF);
}
int16_t p4_native_rows_velvet_frame(int entry) { return (int16_t)(entry == P4_NATIVE_ROWS_VELVET_ENTRY ? SLOT_FRAME : entry + 0x20D); }
int16_t p4_native_rows_velvet_small(int entry) { return (int16_t)(entry == P4_NATIVE_ROWS_VELVET_ENTRY ? SLOT_SMALL : entry + 0xF2); }
int16_t p4_native_rows_velvet_large(int entry) { return (int16_t)(entry == P4_NATIVE_ROWS_VELVET_ENTRY ? SLOT_LARGE : entry + 0xFD); }

uint32_t p4_native_rows_velvet_pack(uint32_t retail)
{
    return pack_address && retail == retail_pack ? pack_address : retail;
}

static uint32_t get32(uint32_t address) { return P4_G32(address); }
static int16_t get16(uint8_t *base, uint32_t offset) { int16_t v; memcpy(&v, base + offset, 2); return v; }
static void put32(uint8_t *base, uint32_t offset, uint32_t value) { memcpy(base + offset, &value, 4); }
static void put16(uint8_t *base, uint32_t offset, uint16_t value) { memcpy(base + offset, &value, 2); }

/* One appended record for a sprite handle's raster and rectangle; the card's
 * parts share a raster when they sit in one atlas. */
static int add_record(uint8_t *pack, uint8_t *record, int handle, int *raster_count)
{
    uint32_t raster;
    int rect[4];
    if (p4_ui_sprite_rect(handle, &raster, rect)) return -1;
    int index = 0;
    while (index < *raster_count && get32(P4_H2G(pack) + RASTERS_AT + 4u * (uint32_t)index) != raster) ++index;
    if (index == *raster_count) {
        if (index == MAX_RASTERS) return -1;
        put32(pack, RASTERS_AT + 4u * (uint32_t)index, raster);
        ++*raster_count;
    }
    memset(record, 0, RECORD_SIZE);
    put32(record, 0x14, (uint32_t)index);
    put32(record, 0x18, 4);
    put32(record, 0x54, (uint32_t)rect[0]);
    put32(record, 0x58, (uint32_t)rect[1]);
    put32(record, 0x5c, (uint32_t)(rect[0] + rect[2]));
    put32(record, 0x60, (uint32_t)(rect[1] + rect[3]));
    for (uint32_t i = 0; i < 4; ++i) put32(record, 0x64 + 4 * i, 0x80808080u);
    return 0;
}

/* The extended copy of the pool's loaded retail pack, rebuilt for each Velvet
 * visit (the facility reloads its pack per visit). */
static int install_pack(uint32_t pool)
{
    uint32_t retail = get32(pool);
    uint8_t *source = P4_GPTR(uint8_t, retail);
    if (get16(source, 0) != 5 || get16(source, RECORD_COUNT_AT) != RETAIL_RECORDS) return -1;
    if (!pack_address && !(pack_address = p4_gport_alloc(PACK_SIZE, 16))) return -1;
    if (!records_address &&
        !(records_address = p4_gport_alloc((RETAIL_RECORDS + MOD_RECORDS) * RECORD_SIZE, 16))) return -1;
    uint8_t *pack = P4_GPTR(uint8_t, pack_address), *copy = P4_GPTR(uint8_t, records_address);
    memcpy(pack, source, PACK_SIZE);
    memset(pack + 0x238, 0, 8); /* never linked into the retail pack list */
    memcpy(copy, P4_GPTR(uint8_t, get32(retail + RECORDS_AT)), RETAIL_RECORDS * RECORD_SIZE);
    int raster_count = get16(source, HEADER_RASTERS_AT);
    uint8_t *mod = copy + RETAIL_RECORDS * RECORD_SIZE;
    if (add_record(pack, mod + 0 * RECORD_SIZE, velvet_row.art.symbol, &raster_count) ||
        add_record(pack, mod + 1 * RECORD_SIZE, velvet_row.art.symbol, &raster_count) ||
        add_record(pack, mod + 2 * RECORD_SIZE, velvet_row.art.label, &raster_count) ||
        add_record(pack, mod + 3 * RECORD_SIZE, velvet_row.art.small, &raster_count))
        return -1;
    put32(pack, RECORDS_AT, records_address);
    put16(pack, RECORD_COUNT_AT, RETAIL_RECORDS + MOD_RECORDS);
    put16(pack, HEADER_RASTERS_AT, (uint16_t)raster_count);
    put16(pack, HEADER_RECORDS_AT, RETAIL_RECORDS + MOD_RECORDS);
    retail_pack = retail;
    put32(P4_GPTR(uint8_t, pool), 0, pack_address); /* FclSpritePool.resource */
    return 0;
}

static int mod_index(uint8_t *work, int count)
{
    for (int i = 0; i < count; ++i)
        if (get16(p4_native_rows_velvet_row(work, i), 4) == P4_NATIVE_ROWS_VELVET_ENTRY) return i;
    return -1;
}

void p4_native_rows_velvet_sync(uint8_t *work)
{
    uint32_t work_address = P4_H2G(work);
    int count = (int8_t)work[COUNT_AT];
    /* Nothing to do, and nothing allocated or written, without a mod row. */
    if (!velvet_row.used && active_work != work_address) return;
    int present = side() ? mod_index(work, count) : -1;
    int wanted = velvet_row.used && velvet_row.enabled;
    if (present >= 0 && !wanted) {
        /* A stale card (its row was disabled or removed): close the gap. */
        for (int i = present; i < count - 1; ++i)
            memcpy(p4_native_rows_velvet_row(work, i), p4_native_rows_velvet_row(work, i + 1), ROW_BYTES);
        work[COUNT_AT] = (uint8_t)(count - 1);
        if ((int8_t)work[CURSOR_AT] >= count - 1) work[CURSOR_AT] = (uint8_t)(count - 2);
        if (active_work == work_address) active_work = 0;
        return;
    }
    if (present < 0) {
        if (active_work == work_address) active_work = 0;
        if (!wanted || !side() || count < FIXED_ROWS || count >= P4_NATIVE_ROWS_VELVET_CAPACITY) return;
        uint32_t pool_task = get32(work_address + POOL_TASK_AT);
        uint32_t pool = pool_task ? get32(pool_task + TASK_WORK_AT) : 0; /* FclSpritePool */
        if (!pool || (get32(pool) != pack_address && install_pack(pool))) return;
        /* Forecast, Talk and Back move one place up; Back may reach the side row. */
        present = count - FIXED_ROWS;
        for (int i = count; i > present; --i)
            memcpy(p4_native_rows_velvet_row(work, i), p4_native_rows_velvet_row(work, i - 1), ROW_BYTES);
        uint8_t *row = p4_native_rows_velvet_row(work, present);
        memset(row, 0, ROW_BYTES);
        put16(row, 0, P4_NATIVE_ROWS_VELVET_MODE); /* state0 */
        put16(row, 4, P4_NATIVE_ROWS_VELVET_ENTRY); /* id */
        work[COUNT_AT] = (uint8_t)(count + 1);
        fclSpriteReset(SLOT_FRAME, CARD_BASE_RECORD);
    }
    active_work = work_address;
    if (return_to_card) {
        work[CURSOR_AT] = (uint8_t)present;
        return_to_card = 0;
    }
}

int p4_native_rows_velvet_open(uint8_t *task)
{
    if (!velvet_row.used || !velvet_row.enabled || config_task || !task) return -1;
    uint32_t config = (uint32_t)cmpConfigCreate(task, CONFIG_MODE);
    if (!config) return -1;
    /* p4_settings_native_host is provided by the Config/Settings host
     * (0035f0c0; transplant/settingshost-20261110), which lands after this
     * stack. Until then, the hosted page cannot be shown; refuse honestly. */
    if (1) {
        kwlnTaskDestroyWithHierarchy(P4_G2H(config));
        fprintf(stderr, "p4pc: native row velvet %s refused page %s\n", velvet_row.key, velvet_row.page);
        return -1;
    }
    config_task = config;
    fprintf(stderr, "p4pc: native row velvet %s opened page %s\n", velvet_row.key, velvet_row.page);
    return 0;
}

int p4_native_rows_velvet_step(void)
{
    if (config_task && !cmpConfigIsStepDone(P4_G2H(config_task))) return 1;
    if (config_task) {
        kwlnTaskDestroyWithHierarchy(P4_G2H(config_task));
        fprintf(stderr, "p4pc: native row velvet %s closed page %s\n", velvet_row.key, velvet_row.page);
    }
    config_task = 0;
    return_to_card = 1;
    return 0;
}
