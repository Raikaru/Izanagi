#ifndef P4_GOLDEN_ICONS_H
#define P4_GOLDEN_ICONS_H
#include "p4pc/game_mode.h"
#include "p4pc/content_source.h"
#include "p4pc/content_digest.h"
#include "p4pc/resource_ref.h"
#include "p4pc/golden_cache.h"
#define P4_GOLDEN_ICON_COUNT 27u
typedef struct P4GoldenIconImage {
    char key[96];
    P4ContentSource source;
    uint8_t *object; /* malloc-owned cache object; rgba begins at +44 */
    size_t size;
    uint32_t width,height;
    P4ContentFingerprint identity;
} P4GoldenIconImage;
int p4_golden_icons_register(P4GameMode mode);
/* Worker conversion only: writes a complete M5 object set, but grants no
 * runtime admission. The production adapter independently pins its source. */
int p4_golden_icons_cache_build(P4GameMode mode, const P4GoldenCachePaths *paths,
    const P4GoldenCacheIdentity *identity, const uint8_t *bytes, size_t size,
    char key[65], char *reason, size_t capacity);
/* Load only previously admitted, M5-hash-verified cache objects. Golden-only.
 * Empty/unprepared cache state is a refusal, never a fabricated resource. */
int p4_golden_icons_images(P4GameMode mode, P4GoldenIconImage out[P4_GOLDEN_ICON_COUNT], char *reason,size_t capacity);
int p4_golden_icons_item_ref(uint32_t flags, P4ResourceRef *out);
int p4_golden_icons_arcana_ref(unsigned arcana,int special_state,P4ResourceRef *out);
#endif
