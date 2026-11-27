#ifndef P4_GOLDEN_CPK_VFS_H
#define P4_GOLDEN_CPK_VFS_H
/* Active source is Golden-only. Dormant registration is for PS2 witnesses. */
int p4_golden_cpk_vfs_start(void);
int p4_golden_cpk_vfs_register(const char *directory, int hybrid);
int p4_golden_cpk_vfs_register_dormant(const char *directory);
int p4_golden_cpk_vfs_dormant_requested(void);
int p4_golden_cpk_vfs_start_dormant(void);
#endif
