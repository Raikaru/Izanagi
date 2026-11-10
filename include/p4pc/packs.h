/* Independent engine-pack selector using the common package registry. */
#ifndef P4PC_PACKS_H
#define P4PC_PACKS_H
#include "p4pc/packages.h"

/* Stage before the common registry publish. Empty selection does no discovery. */
int p4_packs_stage(const char *root, const char *enabled);
int p4_packs_init(void);
typedef int (*P4PackRefusalFn)(const char *id, const char *reason, void *user);
int p4_packs_set_refusal_handler(P4PackRefusalFn callback, void *user);
size_t p4_packs_count(void);
const P4PackageInfo *p4_packs_info(size_t index);
size_t p4_packs_file_conflicts(void);
/* Metadata only; discovery neither activates nor validates payloads. */
int p4_packs_installations_scan(const char *root, P4PackageInstallationVisitor visitor, void *user);
int p4_packs_configured_installations_scan(P4PackageInstallationVisitor visitor, void *user);
#endif
