#include "p4pc/sfd_condition_guard.h"
#include <stdio.h>

/* Invalid keys were undefined on the original. Report the first refusal only,
 * including when several threads reach the guard at once. */
void p4_sfd_invalid_condition_key(int key)
{
    static unsigned reported;
    if (!__atomic_exchange_n(&reported, 1u, __ATOMIC_RELAXED))
        fprintf(stderr, "p4pc: refusing out-of-range SFD condition key %d\n", key);
}
