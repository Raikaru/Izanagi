#ifndef P4PC_GAME_TABLE_OVERLAY_H
#define P4PC_GAME_TABLE_OVERLAY_H
/* Per-run session overlays of the loaded BTL.CVM banks (docs/modding/tables.md
 * "Session bank overlays"), owned by the table service. A bank key is the file
 * label of the actual 0x005f66c0 descriptor plus its block index ("PERSONA1");
 * the static FUSION0..3 banks are the arcana fusion charts in ELF data, and the
 * static ITEMTBL bank is the retail item table (API 1.11): loaded only while the
 * item pointer names that built-in table, and field-restricted to each row's
 * unlock-gate words (every other byte must stay stock). Every
 * loaded bank's bytes and extent come from the extent registry; stock copies are
 * captured from the live bank when its first overlay is applied. Overlays are
 * refused while a package table source is published (the catalog identity is
 * not retail), during a battle, for banks outside the overlay allowlist and for
 * a bank another owner overlays. Every load and New Game reset ends the session:
 * the save-commit restore puts every bank's stock bytes back before any mod's
 * load callback runs. With no overlay applied nothing is registered and no game
 * byte changes. Game thread only. */
#include <stddef.h>
#include <stdint.h>

#define P4_TABLE_OVERLAY_BANKS 41u
#define P4_TABLE_OVERLAY_REASON 192u

typedef struct P4TableOverlayInfo {
    const char *key;  /* "PERSONA1" */
    const char *file; /* "battle/PERSONA.TBL", or the ELF data label of a static bank */
    unsigned block;
    uint32_t slot;    /* GP pointer slot; 0 for a static bank */
    int overlayable;  /* in the overlay allowlist */
    uint32_t address, size; /* static banks: the fixed ELF data span */
    unsigned byte_limit;    /* static banks: every overlaid byte must be <= this */
    uint32_t pointer;       /* static banks: loaded only while this word names address (0: always) */
    unsigned row_stride, field_offset, field_size; /* row_stride: only this window of each row may differ */
} P4TableOverlayInfo;
typedef struct P4TableOverlayBank {
    const char *key;
    const void *bytes;
    size_t size;
} P4TableOverlayBank;

/* The descriptor's banks in file/block order; NULL past the end. */
const P4TableOverlayInfo *p4_game_tables_overlay_info(unsigned index);
const P4TableOverlayInfo *p4_game_tables_overlay_find(const char *key);
/* 1 with the bank's current base/extent, 0 while it is not loaded/published. */
int p4_game_tables_overlay_extent(const P4TableOverlayInfo *bank, uint32_t *base, uint32_t *bytes);
/* Copies the bank's stock bytes (the live bank when no overlay is active):
 * 0, or -1 when the bank is not loaded or capacity differs from its extent. */
int p4_game_tables_overlay_stock(const P4TableOverlayInfo *bank, void *out, size_t capacity);
/* Replaces owner's whole layer, all or none: banks left out of the new layer
 * get their stock back. 0, or -1 with reason and nothing changed. */
int p4_game_tables_overlay_apply(size_t owner, const char *owner_name,
    const P4TableOverlayBank *banks, size_t count, char *reason, size_t capacity);
/* Number of banks owner overlays; owner (size_t)-1 counts every owner. */
size_t p4_game_tables_overlay_count(size_t owner);
/* Name of the owner that overlays the bank, or NULL. */
const char *p4_game_tables_overlay_owner(const P4TableOverlayInfo *bank);
/* Restores owner's banks (VM teardown). Infallible and allocation free. */
void p4_game_tables_overlay_clear_owner(size_t owner);
/* Session end: restores every overlaid bank (load/New Game reset commit). */
void p4_game_tables_overlay_reset(void);
/* A static bank's span was copied again and re-published (the item table's
 * init copy): re-captures each overlaid bank there as stock and re-applies its
 * layer, or drops it with a diagnostic when the layer no longer fits. */
void p4_game_tables_overlay_static_published(uint32_t address, uint32_t bytes);
/* Registers the session-end restore as an optional native save provider
 * (no block is ever written). Idempotent; 0 or -1. */
int p4_game_tables_overlay_session_register(void);
#endif
