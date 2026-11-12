/* Real sprite packet, ordering-table clear and resource ownership on the
 * no-device RenderWare backend. No retail art or GPU is required. */
#include "p4pc/battle_sim.h"
#include "p4pc/mod_resources.h"
#include "p4pc/ui_overlay.h"
#include "p4pc/image.h"
#include "p4pc/os.h"
#include "p4gl.h"
#include "guest_test.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void *kwlnProbeHeapBudget(void);
extern void kwlnInitEngine(void);
extern void PrimCreatePools(int32_t, int32_t);
extern void PrimClearNodes(int32_t, int32_t);
extern void sdkOtFlushBuckets(void *);
#define OT 0x00793e80u
#define SCREEN_BUCKET (OT + 0xd7u * 0x30u)
static void clear_ot(void) { PrimClearNodes((int32_t)OT, 0xd8); }
static int add(uint32_t owner, uint32_t token, int x)
{
    int sprite = p4_ui_sprite_create(owner, token, x, 0, 2, 2);
    assert(sprite >= 0);
    P4UiOverlayItem item = {.kind=P4_UI_ITEM_SPRITE, .context=P4_UI_CONTEXT_SCREEN,
        .rgb=0xffffff, .alpha=255, .scale=1, .sprite=sprite};
    assert(p4_ui_overlay_add(owner, &item) >= 0);
    return sprite;
}
static uint32_t packet_pack(uint32_t node)
{
    assert(node);
    return P4_G32(node + 0x10u);
}
static void check_pack(uint32_t pack, uint32_t raster, int left)
{
    assert(pack && P4_G32(pack + 0x104u) == raster);
    uint32_t record = P4_G32(pack + 0x204u);
    assert(record && P4_G32(record + 0x54u) == (uint32_t)left &&
        P4_G32(record + 0x5cu) == (uint32_t)(left + 2));
}
static int test_main(int argc, char **argv)
{
    (void)argc; (void)argv;
    const char *elf = getenv("P4PC_ELF");
    if (!elf || !elf[0]) { fprintf(stderr, "ui_overlay_queue: P4PC_ELF unset\n"); return 77; }
    p4_os_headless_env();
    p4_os_setenv("P4PC_INPUT", "script", 1);
    p4_os_setenv("P4PC_MODS", "0", 1);
    p4_os_setenv("P4PC_CONFIG", "none", 1);
    p4_os_setenv("P4PC_RESOLUTION", "640x448", 1);
    p4pc_trap_configure();
    p4pc_load_image(elf);
    p4pc_apply_patches();
    p4pc_kernel_pre_init();
    kwlnProbeHeapBudget();
    p4_battle_sim_renderless = 1;
    kwlnInitEngine();
    assert(p4gl_device_ready() && !p4gl_context_is_current());
    PrimCreatePools(64, 8);
    clear_ot();
    uint8_t rgba[8 * 8 * 4]; memset(rgba, 0xff, sizeof rgba);
    P4ContentSource source = {0}; source.origin = P4_TEXTURE_OVERRIDE; source.rank = 1;
    strcpy(source.provider, "test.overlay"); strcpy(source.key, "authored/overlay.rgba");
    P4Resources *resources = p4_resources_create(2 * sizeof rgba); assert(resources);
    uint32_t token = 0;
    assert(p4_resources_image(resources, "test.overlay", "test.overlay:icon", &source,
        rgba, sizeof rgba, 8, 8, 1, &token) == P4_RESOURCE_OK);
    assert(p4_resources_freeze(resources, 1) == P4_RESOURCE_OK);
    const P4ResourceImage *image = p4_resources_query(token); assert(image);
    uint32_t raster = image->raster;

    int old = add(7, token, 1);
    p4_ui_overlay_draw(P4_UI_CONTEXT_SCREEN);
    uint32_t node = P4_G32(SCREEN_BUCKET), pack = packet_pack(node);
    p4_ui_overlay_release_owner(7);
    assert(p4_resources_retire_unpinned(resources) == P4_RESOURCE_CONFLICT);
    check_pack(pack, raster, 1);
    int replacement = add(7, token, 4); assert(replacement != old);
    p4_ui_overlay_draw(P4_UI_CONTEXT_SCREEN);
    uint32_t next = P4_G32(node), new_pack = packet_pack(next);
    assert(new_pack != pack); check_pack(pack, raster, 1); check_pack(new_pack, raster, 4);
    p4_ui_overlay_release_owner(7);
    /* Real walker invokes both deferred callbacks and native sprite draws. */
    sdkOtFlushBuckets(P4_G2H(SCREEN_BUCKET));
    assert(p4_resources_retire_unpinned(resources) == P4_RESOURCE_CONFLICT);
    clear_ot();

    /* Camera-begin failure skips the walker: only the real next OT clear runs. */
    add(7, token, 2); p4_ui_overlay_draw(P4_UI_CONTEXT_SCREEN);
    node = P4_G32(SCREEN_BUCKET); pack = packet_pack(node);
    p4_ui_overlay_release_owner(7);
    check_pack(pack, raster, 2);
    assert(p4_resources_retire_unpinned(resources) == P4_RESOURCE_CONFLICT);
    clear_ot();

    for (int i = 0; i < 100; ++i) {
        add(7, token, 0); p4_ui_overlay_draw(P4_UI_CONTEXT_SCREEN);
        p4_ui_overlay_release_owner(7);
        clear_ot(); /* No separately allocated sprite sample on a failed frame. */
        add(7, token, 0); p4_ui_overlay_draw(P4_UI_CONTEXT_SCREEN);
        p4_ui_overlay_release_owner(7);
        sdkOtFlushBuckets(P4_G2H(SCREEN_BUCKET));
        clear_ot();
    }
    assert(p4_resources_retire_unpinned(resources) == P4_RESOURCE_OK);
    assert(!p4_resources_query(token));
    p4gl_context_close();
    puts("ui_overlay_queue: real queued draws, clear without draw, reuse and pin retirement passed");
    return 0;
}
int main(int argc, char **argv) { return p4t_on_guest_stack(test_main, argc, argv); }
