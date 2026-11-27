#ifndef P4PC_C03_GUARD_H
#define P4PC_C03_GUARD_H
#include <stddef.h>
#include <stdint.h>
/* Install as the Golden guest arena factory before p4_guest_init, then arm only
 * after the retail image is loaded. Unsupported platforms fail closed. */
void *p4_c03_guard_reserve(size_t reserve, size_t commit);
void p4_c03_guard_shutdown(void);
int p4_c03_guard_alias_released(void);
int p4_c03_guard_install_cleanup(void);
int p4_c03_guard_install_report(void);
int p4_c03_guard_start(void);
int p4_c03_guard_active(void);
/* Offset is relative to SFLIB_libwork at guest 0x007458c0. */
void p4_c03_guard_store(size_t offset, const void *data, size_t size);
int p4_c03_guard_tainted(void);
uintptr_t p4_c03_guard_origin(void);
uintptr_t p4_c03_guard_address(void);
#endif
