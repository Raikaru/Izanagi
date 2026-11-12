#ifndef P4PC_PS2_LIBC_H
#define P4PC_PS2_LIBC_H

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

/* Retail EE allocator entry widths and guest reent; native pointer bridges. */
uint8_t *func_0043ece8(int32_t size);
void func_0043ed08(uint8_t *pointer);
void *func_0043c710(uint32_t count, uint32_t size);
void *func_00441558(void *pointer, uint32_t size);
uint8_t *func_0043ef80(void *reent, int32_t size);
void func_0043e170(void *reent, uint8_t *pointer);

/* Retail conversions accumulate 64 bits even though guest pointers and atoi's
 * result are 32 bits. An end pointer is written to a guest-address word. */
int64_t func_00443fd0(uint32_t *reent, const char *text, uint32_t *end, int base);
uint64_t func_00444240(uint32_t *reent, const char *text, uint32_t *end, int base);
int64_t func_00444210(const char *text, uint32_t *end, int base);
uint64_t func_00444458(const char *text, uint32_t *end, int base);
int32_t func_0043c6b0(const char *text);
uint64_t func_00443068(uint32_t *reent, const char *text, uint32_t *end);
int func_00442100(const char *text, const char *format, ...);
int func_00442188(uint32_t *reent, const char *text, const char *format, ...);
int func_00446108(uint32_t *reent, uint32_t *file, const char *format, const void *args);

/* Retail returns the difference of the first differing unsigned bytes. */
int func_004426e8(const char *left, const char *right);
int func_00442c30(const char *left, const char *right, uint32_t count);
int func_0043f778(const void *left, const void *right, uint32_t count);
char *func_00443f38(char *text, const char *delimiters, uint32_t *save);
char *func_00443f18(char *text, const char *delimiters);

/* Reviewed host byte operations behind the original EE-width entry ABI.
 * In particular, no callback installed by game code takes native size_t. */
void *func_0043f810(void *destination, const void *source, uint32_t count);
void *func_0043f8c0(void *destination, const void *source, uint32_t count);
void *func_0043f9c8(void *destination, int value, uint32_t count);
char *func_00442a80(char *destination, const char *source, uint32_t count);
char *func_00442de8(char *destination, const char *source, uint32_t count);
uint32_t func_00442948(const char *text);

/* Optional native-only asset provenance observer. Called before writes (after=0)
 * and after copy/move (after=1); source is NULL for clear/invalidation. It must not mutate
 * bytes or reenter these wrappers. Registration refuses a competing owner;
 * NULL unregisters the current observer. No observer leaves retail bytes exact. */
typedef void (*P4ByteObserver)(void *destination, const void *source,
                                uint32_t count, int after);
int p4pc_bytes_observer(P4ByteObserver observer);
/* Notify before releasing/protecting native-owned bytes, without touching them.
 * Uses the same (destination,NULL,count,0) event as clear's invalidation phase. */
void p4pc_bytes_invalidate(void *destination, uint32_t count);

/* Output uses the retail formatter; varargs forwards keep their native ABI. */
int func_00442088(char *buffer, const char *format, ...);
/* Bounded native-ABI frontend for game-facing native formatter callbacks. */
int p4pc_ee_snprintf(char *buffer, size_t capacity, const char *format, ...);
int func_00440b68(const char *format, ...);
int func_004244c8(const char *format, ...);
int func_00446ed8(char *buffer, const char *format, va_list args);
int func_00446e88(const char *format, va_list args);

#endif
