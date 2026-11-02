/* Explicit native element presentation: one owner's ordered layout of semantic
 * columns over one exact frozen registry. No family/visibility inference. */
#ifndef P4PC_GAME_ELEMENTS_UI_H
#define P4PC_GAME_ELEMENTS_UI_H
#include <stddef.h>
#include <stdint.h>
#include "p4pc/game_tables.h"
#include "p4pc/mod_resources_boot.h"
#include "p4pc/game_elements_ui_native.h"

#define P4_ELEMENT_UI_MAX_COLUMNS 9u
/* Presentation-only marker after the four P4KoAffinity values. */
#define P4_ELEMENT_UI_KO_UNKNOWN 4u

/* One column: a registered leaf with its actual effect component and affinity
 * channel, plus an optional independent same-leaf instant-KO marker with its
 * own channel and knowledge. Validity flags are separate from ID bits;
 * UINT32_MAX is a legal ID. A legacy projection lets the static persona panel
 * read its actual seven-word native snapshot for ONLY the exact native pair of
 * that selector (0..6 = Phys, Fire, Ice, Elec, Wind damage; Light, Dark KO),
 * at persona native profile 0 (any semantic profile ID, as battle's natural
 * route). An authored side row always wins. */
typedef struct P4GameElementUiColumn {
    uint32_t element_id, component, channel_id;
    uint32_t marker_valid, marker_component, marker_channel_id;
    uint32_t legacy_primary_valid, legacy_primary_selector;
    uint32_t legacy_marker_valid, legacy_marker_selector;
} P4GameElementUiColumn;
typedef enum P4GameElementUiArt {
    P4_ELEMENT_UI_RETAIL_ART = 0, P4_ELEMENT_UI_AUTHORED_ART = 1
} P4GameElementUiArt;
typedef struct P4GameElementUiPreparedColumn {
    P4GameActionElement primary, marker;
    uint32_t primary_kind, art_kind, native_affinity_sprite;
    uint32_t icon_token, image_width, image_height;
    /* The layout authorized the persona snapshot word for this pair. */
    uint32_t primary_legacy, marker_legacy;
} P4GameElementUiPreparedColumn;
/* The retail seven-column affinity row as an explicit layout, used when an
 * expanded cohort authors no layout of its own. */
extern const P4GameElementUiColumn p4_game_elements_ui_retail_columns[7];

typedef struct P4GameElementUi P4GameElementUi;
/* No allocation besides the host object; allocator is the core's guest-backed
 * allocator, never a script pointer. Game/context thread only. */
P4GameElementUi *p4_game_elements_ui_create(const P4GameTableAllocator *allocator);
/* Single-owner layout claim. Equal re-registration is idempotent; a different
 * owner or layout conflicts rather than merging. Staging only. */
int p4_game_elements_ui_layout(P4GameElementUi *ui, const char *owner,
    const P4GameElementUiColumn *columns, uint32_t count, uint32_t revision);
/* Early CPU plan over the exact frozen registry: descriptors, pairs, every
 * declared alias's coverage of every displayed pair, and the art set (each
 * visible descriptor with an icon reference; a visible new leaf must have
 * one). No guest allocation, resource pin or readiness. Locks the layout. */
int p4_game_elements_ui_prepare(P4GameElementUi *ui, const P4GameTables *tables);
/* Early CPU check that every art-set reference is an exact declared image of
 * the CPU resources product (NULL when no images are declared). */
int p4_game_elements_ui_check_images(const P4GameElementUi *ui, const P4ResourceImagesPrepared *images);
/* Late: resolve and pin real tokens from the exact frozen runtime resources
 * product, and allocate the optional KO marker stream. Failure releases
 * everything it took and exposes no prepared column. */
int p4_game_elements_ui_freeze(P4GameElementUi *ui, const P4Resources *resources);
uint32_t p4_game_elements_ui_revision(const P4GameElementUi *ui);
uint32_t p4_game_elements_ui_count(const P4GameElementUi *ui);
const P4GameElementUiPreparedColumn *p4_game_elements_ui_column(const P4GameElementUi *ui, uint32_t index);
/* Authored art for a leaf: index into the art set, or NOT_FOUND when the leaf
 * keeps native (retail) art or has none. art_draw authorizes the descriptor
 * row, then draws the pinned image at the given on-screen size. */
int p4_game_elements_ui_art_find(const P4GameElementUi *ui, uint32_t leaf, uint32_t *index);
int p4_game_elements_ui_art_draw(const P4GameElementUi *ui, uint32_t index,
    float x, float y, float depth, uint32_t rgba, float width, float height);
const P4GameTables *p4_game_elements_ui_tables(const P4GameElementUi *ui);
/* Actual visible consumption only: authorize the exact issued element (and
 * channel) rows through the Root source transport before using them. Plan,
 * check, freeze and getters never record a use. Icon art uses no channel. */
int p4_game_elements_ui_runtime_element(const P4GameElementUi *ui, uint32_t index);
int p4_game_elements_ui_runtime_pair(const P4GameElementUi *ui, uint32_t index, uint32_t marker);
/* Static persona display value for one column: exact side row (alias and row
 * used first) or, if authorized, the actual native snapshot word. KO channels
 * yield native flag words. Output untouched on failure. */
int p4_game_elements_ui_persona_snapshot_use(const P4GameElementUi *ui, uint32_t persona_id,
    const uint32_t native_words[7], uint32_t column, uint32_t marker, uint32_t *value);
/* Guest address of the command-free ASCII marker for a P4KoAffinity value or
 * P4_ELEMENT_UI_KO_UNKNOWN; zero when no column declares a marker. */
uint32_t p4_game_elements_ui_ko_text(const P4GameElementUi *ui, uint32_t kind);
/* After the native snapshot detached and native users quiesced: releases pins
 * and the marker stream. Pins survive graphics-only context loss. */
void p4_game_elements_ui_destroy(P4GameElementUi *ui);

/* Native snapshot (game_elements_ui_native.c). stage borrows a frozen model and
 * registers the "element-ui" completion participant; commit publishes it only
 * after the element owner committed the same registry. */
int p4_game_elements_ui_native_stage(const P4GameElementUi *ui);
int p4_game_elements_ui_native_detached(const P4GameElementUi *ui);
/* Core-only, after task/queue/graphics quiescence and before boot session end. */
void p4_game_elements_ui_native_session_end(void);

/* UI300 package provider: [element-ui] / [element-column KEY] manifest layout,
 * depending on the table-source and resource products. */
#define P4_GAME_ELEMENTS_UI_BOOT_PROVIDER "game-elements.ui"
extern const P4PackageHandler p4_game_elements_ui_boot_provider;
/* After native session end; refuses while the native snapshot still borrows
 * the model. Then the core may end resource and table sessions. */
int p4_game_elements_ui_boot_session_end(void);
#endif
