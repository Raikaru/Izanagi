/* Mod rows in native menus whose layouts are hard-coded per row count
 * (docs/modding/lua.md, "Native rows"). Game thread only. With no enabled row
 * every host is byte-identical to retail.
 *
 * Velvet Room fusion command cards (P4_NATIVE_ROWS_VELVET_FUSION): one mod card
 * joins the retail cards after the fusion spreads, before Fusion Forecast, Talk
 * and Back. The card list holds up to ten rows: FclCombineWork's nine
 * commandRows plus one port-owned side row, which the overrides that read rows
 * (002eb270, 002e90d0, 00317900, 00318840, 00318f30, 003190d0) reach through
 * p4_native_rows_velvet_row. fclCombineLayoutTransition's entry (transition 0),
 * which every card list passes through, adds or drops the card. Its art draws
 * from a port-owned copy of the loaded facility/velvet01.spr records with the
 * card's records appended (retail records are never touched), installed as the
 * fcl sprite pool's pack, so the native slide-in, highlight, mark and fades
 * apply unchanged. Confirming the card hosts the row's registered page in the
 * Velvet task's own Config child (p4_settings_native_host) while the root waits
 * in sub-state P4_NATIVE_ROWS_VELVET_PAGE; on close the cards slide back in
 * with the cursor on the mod card. */
#ifndef P4PC_NATIVE_ROWS_H
#define P4PC_NATIVE_ROWS_H
#include <stdint.h>

enum { P4_NATIVE_ROWS_VELVET_FUSION = 0, P4_NATIVE_ROWS_HOSTS = 1 };
/* The mod card's entry id: retail ids are 0..8. */
#define P4_NATIVE_ROWS_VELVET_ENTRY 9
/* The card's row: state0 (an owner dispatching it itself sees this f0 mode). */
#define P4_NATIVE_ROWS_VELVET_MODE 0x40
/* The root sub-state (FclCombineWork f1) while the card's page is open. */
#define P4_NATIVE_ROWS_VELVET_PAGE 0xF1
/* The card list's row capacity with the side row. */
#define P4_NATIVE_ROWS_VELVET_CAPACITY 10

/* Art of one card, as p4_ui_sprite_create handles (ui_overlay.h): the 72x77
 * symbol, the large label (at most 99x31) and the small label (at most 72x18),
 * in the Velvet card families of docs/modding/ui_art.md. */
typedef struct P4NativeRowArt {
    int symbol, label, small;
} P4NativeRowArt;

/* page is a registered p4.ui page key (the card opens it). Returns a row
 * handle >= 0, or -1: unknown host, invalid art or page, or the host's single
 * mod row is taken. The row starts enabled. */
int p4_native_rows_add(int host, uint32_t owner, const char *key, const P4NativeRowArt *art, const char *page);
/* A disabled row is not offered (its owner's authorizer denied it). Takes
 * effect the next time the host lays out its rows. */
int p4_native_rows_enable(int handle, int enabled);
int p4_native_rows_remove(int handle);
void p4_native_rows_remove_owner(uint32_t owner);
/* 1 while an enabled mod row is part of the host's current rows. */
int p4_native_rows_present(int host);
/* The row whose page is open (the host is in its page state), otherwise -1. */
int p4_native_rows_selected(int host);

/* ---- Velvet host seams (overrides only) */
/* fclCombineLayoutTransition(task, 0): the card list gets the enabled mod card
 * after the spreads, or loses a stale one, before it is laid out. */
void p4_native_rows_velvet_sync(uint8_t *work);
/* 1 when this FclCombineWork's current card list includes the mod card. */
int p4_native_rows_velvet_active(const uint8_t *work);
/* The 10-byte FclCombineRow at index 0..9 (index 9 is the side row). */
uint8_t *p4_native_rows_velvet_row(uint8_t *work, int index);
/* fcl sprite slots of an entry's card parts; retail values for entries 0..8. */
int16_t p4_native_rows_velvet_symbol(int entry);
int16_t p4_native_rows_velvet_frame(int entry);
int16_t p4_native_rows_velvet_small(int entry);
int16_t p4_native_rows_velvet_large(int entry);
/* The pack the card list's sprites come from: the extended copy while it is
 * installed for this pack, otherwise the retail pack itself. */
uint32_t p4_native_rows_velvet_pack(uint32_t retail);
/* Confirming the card: host its page under the Velvet task. 0 on success. */
int p4_native_rows_velvet_open(uint8_t *task);
/* While the root is in P4_NATIVE_ROWS_VELVET_PAGE: 1 while the page is open;
 * 0 once it closed and was destroyed (the next layout restores the cursor to
 * the mod card). */
int p4_native_rows_velvet_step(void);
#endif
