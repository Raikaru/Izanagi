#ifndef P4PC_PACKAGE_BOOT_H
#define P4PC_PACKAGE_BOOT_H
#ifdef __cplusplus
extern "C" {
#endif
/* Native startup only. No host records leak into the guest ABI census. */
int p4_package_boot_install(void);
/* Generated installer only: call after every descriptor registered successfully.
 * Other entrypoints (native workers/viewer) leave optional boot disabled. */
void p4_package_boot_installed(void);
void p4_package_boot_disable(void);
/* Sole0010d480 entry:1 means THIS invocation already committed native completion
 * inside the first late transaction,0 means run the ordinary native completion
 * (later real reload, or optional startup disabled). Failure owns native error /
 * persisted recovery / normal drained exit; it never returns fake success. */
int p4_package_boot_materialize(void);
/* Post-exposure source rejection may happen BEFORE0010d480 (e.g. ITEMTBL).
 * Pass actual selected pinned owner, NULL for a real native source. reason
 * identifies that frozen source; never reconstruct ownership from strings.
 * This owns native recovery/drained exit, not an early refusal or rollback. */
struct P4PackageProvider;
#ifdef __cplusplus
[[noreturn]]
#else
_Noreturn
#endif
void p4_package_boot_source_failed(const struct P4PackageProvider *actual_provider,const char *reason);
/* No source/owner has been attested. Never pretend this is known-native.
 * Offers explicit all-selector safe mode or Quit, not guessed ownership. */
#ifdef __cplusplus
[[noreturn]]
#else
_Noreturn
#endif
void p4_package_boot_runtime_failed(const char *reason);
/* Native startup health only: observe actual interactive title state9, not
 * a timer/preview. Normal exits and an empty admitted batch also clear custody. */
extern int p4_package_boot_waiting_stable;
void p4_package_boot_stable(void);
void p4_package_boot_clean_exit(void);
void p4_package_boot_admission_complete(void);
#ifdef __cplusplus
}
#endif
#endif
