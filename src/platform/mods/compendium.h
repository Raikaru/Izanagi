#ifndef P4PC_MOD_COMPENDIUM_H
#define P4PC_MOD_COMPENDIUM_H
#include "p4pc/packages.h"
#include <stddef.h>

/* 0: recognized data-only package; 1: no Compendium/disc-tree layout; -1:
 * recognized but invalid metadata/layout. Does not mutate package files.
 * files receives the actual host payload root, retaining its original casing. */
int p4_compendium_manifest(const char *directory, const char *folder,
    P4PackageInfo *info, char *files, size_t capacity);
/* Checks package-root code inputs. An optional payload already validated by the
 * immutable file index is skipped, avoiding a second walk of every data file.
 * NULL payload performs the complete standalone metadata-witness audit. */
int p4_compendium_root_safe(const char *directory, const char *validated_payload);
int p4_mod_code_name(const char *name);
int p4_mod_code_file(const char *path);
#endif
