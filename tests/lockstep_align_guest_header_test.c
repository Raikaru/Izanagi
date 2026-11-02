/* A mirror TU already owns the retail CRT declarations. Pulling libc headers
 * through this guest seam would redeclare these names with incompatible types. */
extern void func_0043f810(void);
extern void func_0043f9c8(void);
extern void func_00440b68(void);
extern void func_00442088(void);
#define memcpy func_0043f810
#define memset func_0043f9c8
#define printf func_00440b68
#define sprintf func_00442088
typedef int i;
#include "p4pc/lockstep_align_guest.h"
