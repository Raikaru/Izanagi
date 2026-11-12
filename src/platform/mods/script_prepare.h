#ifndef P4PC_MOD_SCRIPT_PREPARE_H
#define P4PC_MOD_SCRIPT_PREPARE_H
#include "p4pc/packages.h"
/* Native selector callbacks, never included by translated game units. No mod
 * code or queued game work may run before this admission transaction commits. */
int p4_mod_scripts_prepare(const P4PackageRegistry *registry, void *user);
void p4_mod_scripts_discard_preflight(void *user);
#endif
