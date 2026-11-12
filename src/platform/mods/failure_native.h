#ifndef P4PC_MOD_FAILURE_NATIVE_H
#define P4PC_MOD_FAILURE_NATIVE_H
#include "p4pc/packages.h"
int p4_mod_native_failure_init(void);
/*0=strict automation,1=player/explicit mod witness may continue without the
 * unpublished optional batch. Also disables its late materialization. */
int p4_mod_native_startup_refused(const char *reason);
int p4_package_native_refuse(P4PackageKind kind,const char *id,const char *reason);
/* Post-exposure: owns native reason/choice, persisted selector and drained exit.
 * Does not release VMs, provider pins, native handles or guest allocations. */
_Noreturn void p4_mod_native_runtime_failed(const P4PackageProvider *provider,const char *reason);
/* No attested origin. Safe-mode disabling is an explicit user recovery action,
 * never attribution of the failure to a guessed provider or native source. */
_Noreturn void p4_mod_native_unattributed_failed(const char *reason);
/* Exact attested owner only; NULL/empty IDs refuse. One atomic write preserves
 * unrelated INI data/comments and clears this handled startup's crash marker. */
int p4_package_recovery_persist(P4PackageKind kind,const char *id,int restart);
/* Explicit all-selector safe mode, next-boot cause/notice in the SAME commit. */
int p4_package_recovery_safe_mode(int startup_crash_loop);
/* Unknown-origin Quit leaves both selections untouched. A crash-loop prompt
 * preserves its threshold until safe mode is chosen or settings change. */
int p4_package_recovery_quit(int startup_crash_loop);
#endif
