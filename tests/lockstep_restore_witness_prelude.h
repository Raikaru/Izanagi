/* Guest declarations consumed by unit64.py for the restored override. */
extern u32 D_0079B67C[];
extern u32 iGpffffb19c;
extern char D_005E45C0[], DAT_00796de0[], DAT_00796e00[], DAT_00796e20[];
extern char DAT_007973a0[], DAT_007973b2[], iGpffff9bb0;
int func_00440b68(const char *, ...);
/* the override's sprintf (include/p4pc/libc_rename.h), as include/p4pc/ps2_libc.h declares it: a call through an implicit declaration passes the
 * variable arguments where Apple arm64 does not read them */
int func_00442088(char *, const char *, ...);
void datSaveRestoreSection(s32, u32, u32, u8 *);
