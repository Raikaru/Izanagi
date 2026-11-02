/* FPU/kernel-only tests do not link the game or enable field-entry capture.
 * Fail if they unexpectedly enter that path; never fabricate a game snapshot. */
#include <stdint.h>
#include <stdlib.h>
int64_t calGetDayEntryByPhase(int32_t day, uint32_t phase)
{
    (void)day; (void)phase;
    abort();
}
