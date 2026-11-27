/* CPU table fixtures never construct a verified Golden AI cache product.
 * This boundary is not linked into the player or the AICALC qualification. */
#include "p4pc/golden_ai_boot.h"
#include "p4pc/golden_ai_native.h"
#include <assert.h>
int p4_golden_ai_adapter_register(P4GameMode mode)
{
    return mode==P4_GAME_MODE_PS2?0:-1; /* Golden AI admission is unavailable here. */
}
const struct P4GoldenAiProduct *p4_golden_ai_product(const P4GameTablesDefinitionBuilder *builder)
{
    assert(builder != NULL);
    return NULL;
}
int p4_golden_ai_product_release(const P4GameTablesDefinitionBuilder *builder)
{
    assert(builder != NULL);
    return 0; /* No product was created by these CPU fixtures. */
}
int p4_golden_ai_native_session_end(void)
{
    return 0; /* No native runtime is linked or activated in these fixtures. */
}
