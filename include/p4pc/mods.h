/* Opt-in player-code/file packages. Native engine packs use packages.h directly. */
#ifndef P4PC_MODS_H
#define P4PC_MODS_H
#include "p4pc/packages.h"
/* Stage independently selected player mods before the common global publish.
 * Empty/NULL enables nothing and performs no directory reads. */
int p4_mods_stage(const char *root, const char *enabled);
/* Also finds enabled ids among the mods shipped with the game (bundled_root,
 * read-only; a player installation of the same id wins). */
int p4_mods_stage_bundled(const char *root, const char *bundled_root, const char *enabled);
/* The bundled mod root: P4PC_BUNDLED_MOD_DIR when set (empty disables), else
 * the program's own `mods` directory. NULL when none. */
const char *p4_mods_bundled_root(char path[1200]);
/* Apply a persisted preference after staging, before common publication. */
int p4_mods_set_load_order(const char *ids);
/* Stages player configuration with the overriding P4PC_MODS=0 switch.
 * The startup owner stages engine packs separately, then publishes once. */
int p4_mods_init(void);
/* Before staging only.0 acknowledges a native refusal message and continues
 * with that mod disabled; no handler means strict atomic admission. */
typedef int (*P4ModRefusalFn)(const char *id, const char *reason, void *user);
int p4_mods_set_refusal_handler(P4ModRefusalFn callback, void *user);
size_t p4_mods_count(void);
const P4PackageInfo *p4_mods_info(size_t index);
int p4_mods_installations_scan(const char *root, P4PackageInstallationVisitor visitor, void *user);
int p4_mods_installations_scan_bundled(const char *root, const char *bundled_root,
                                       P4PackageInstallationVisitor visitor, void *user);
/* Native Config discovery using the SAME env/INI/beside-INI root resolution
 * as init. Metadata only, independent of enabled list and kill switch; no VM,
 * payload inventory or activation. Implemented by the configuration frontend. */
int p4_mods_configured_installations_scan(P4PackageInstallationVisitor visitor, void *user);
size_t p4_mods_file_conflicts(void);
/* Player selector only. Fidelity harnesses use the common both-selector assert. */
void p4_mods_assert_disabled(void);
#endif
