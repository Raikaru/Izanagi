#ifndef P4PC_MOD_MODELS_H
#define P4PC_MOD_MODELS_H
/* glTF model resources ([resource-model owner:key], docs/modding/files.md and docs/modding/models.md).
 *
 * Early CPU preparation cooks each declared glTF (src/platform/models, cached by content hash) and
 * checks its target and budget; the runtime phase freezes a table of overrides keyed by the disc key
 * each one replaces. The model loader (mdlReadAuxChunks, overrides/0047b0c0.c) asks this table only
 * when it is nonempty; with no declaration every model takes the retail path untouched. */
#include <stddef.h>
#include <stdint.h>
#include "p4pc/packages.h"

#define P4_RESOURCES_MODELS_HANDLER "game-resources.models"
extern const P4PackageHandler p4_resources_models_boot_provider;

typedef struct P4ModelOverride P4ModelOverride;

/* 1 once a frozen table with at least one override exists; 0 otherwise (a single load). */
int p4_model_overrides_active(void);
/* The override for canonical disc key `key` (DATA/MODEL/WEAPON/WP001.RMD, or
 * DATA/MODEL/PACK/BC001.PAC#bc001.RMD for a pack member; case-insensitive), or NULL. */
const P4ModelOverride *p4_model_override_find(const char *key);
const uint8_t *p4_model_override_blob(const P4ModelOverride *override, size_t *size);
/* "owner:key" and the providing package id, for messages. */
const char *p4_model_override_name(const P4ModelOverride *override);
const char *p4_model_override_provider(const P4ModelOverride *override);
/* The declaration's schema (the viewer's seam: 3). Slot animations apply from schema 3. */
uint32_t p4_model_override_schema(const P4ModelOverride *override);

/* p4pc_viewer's development seams (never package boot; docs/modding/models.md "Development seam").
 * P4PC_VIEWER_MODEL=<glTF>: cooks it with the budget of the viewer's RMD `rmd_path` (files beside it)
 * and installs it as that model's override before the viewer loads it. P4PC_VIEWER_DUMP=<json>: writes
 * the loaded clump's atomics as RenderWare holds them. Each is 0 when its variable is unset, else 0 or -1
 * with the reason printed. */
int p4_model_viewer_override(const char *rmd_path);
int p4_model_viewer_dump(void *clump);
/* The canonical disc key of a game path ("model/weapon/wp001.RMD" -> DATA/MODEL/WEAPON/WP001.RMD): no
 * device or version suffix, '/', upper case, an unqualified path in the first container holding it
 * (the VFS's own rule; needs the mounted VFS). 0, or -1. */
int p4_model_disc_key(const char *game_path, char *out, size_t capacity);

/* Model loader seam (src/platform/models/p4model_build.c). `model` is the game's model object whose
 * auxiliary block has just read its clump. Returns the override for that model's file, or NULL. */
const P4ModelOverride *p4_model_override_for_model(const void *model);
/* Builds the override's textures, materials and geometries and replaces the clump's atomics with
 * atomics on the frames the cooked meshes name. 0, or -1 when nothing could be applied (the retail
 * clump is then left as it was and the reason is printed). */
int p4_model_override_apply(const P4ModelOverride *override, void *clump);
/* Model loader seam for an animation chunk (RtAnimAnimation, 0x1B) that mdlReadAuxChunks would read into
 * slot `slot` of layer `layer`, after the clump `clump`: a schema=3 override's glTF animation for that slot
 * (L<layer>S<slot>), read by RtAnimAnimationStreamRead, or NULL to read the file's own (no such animation,
 * or it cannot drive the replaced model's hierarchy: the reason is printed). */
void *p4_model_override_animation(const P4ModelOverride *override, void *clump, uint32_t layer, uint32_t slot);

/* Game/context thread only, after the core has quiesced model users. Releases the table. */
void p4_model_overrides_session_end(void);
#endif
