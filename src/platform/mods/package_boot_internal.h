#ifndef P4PC_PACKAGE_BOOT_INTERNAL_H
#define P4PC_PACKAGE_BOOT_INTERNAL_H
#include "p4pc/packages.h"
/*0=this invocation published runtime products;1=already materialized;
 * -1=post-exposure failure, resources deliberately retained for process drain. */
int p4_package_registry_runtime_materialize(const P4PackageProvider **failed,const char **reason);
/* Native startup cannot protect/persist recovery, or an actual other startup
 * owns its lease. Block optional preparation for THIS process only. */
extern int p4_package_startup_blocked;
/* Once before preparation:0=continue,1=two unclean startups,-1=I/O,-2=busy lease. */
int p4_package_boot_health_begin(void);
const char *p4_package_boot_health_notice(void);
int p4_package_boot_health_notice_complete(int shown);
void p4_package_boot_health_recorded(void);
#endif
