/* Retail is a no-op. 001939e0 reaches this only after all eight packed-table
 * descriptors have completed; the native service is inert without staging. */
#include "p4pc/game_tables_native.h"
#include "p4pc/package_boot.h"
void datBattleUnitLoadCompleteNoop(void)
{
    if (p4_package_boot_materialize() == 1) return;
    s32 result = p4_game_tables_battle_complete();
    if (result) p4_game_tables_battle_refuse(result);
}
