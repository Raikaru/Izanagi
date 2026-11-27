/* The boot/derived/edit CPU cohorts never request Golden image publication.
 * Reaching these hooks is a fixture failure, not a successful no-op. The image
 * fixture separately links and exercises the real binder and sealed owner. */
#include "../src/platform/golden/golden_mode_image_bind.h"
#include <stdlib.h>

int p4_golden_mode_image_bind(P4GameMode mode, P4GameTablesDefinitionBuilder *b, char *reason, size_t capacity)
{ (void)mode; (void)b; (void)reason; (void)capacity; abort(); }
void p4_golden_mode_image_release(P4GameTablesDefinitionBuilder *b)
{ (void)b; abort(); }
int p4_golden_mode_image_ready(const P4GameTablesDefinitionBuilder *b)
{ (void)b; abort(); }
