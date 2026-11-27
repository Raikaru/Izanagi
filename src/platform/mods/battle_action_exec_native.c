#include "p4pc/battle_action_exec_native.h"
#include "p4pc/gmem.h"

static uint64_t uid_at(uint32_t address)
{
    return (uint64_t)P4_G32(address) | (uint64_t)P4_G32(address + 4) << 32;
}
void p4_action_exec_native_admit(uint32_t action)
{
    const uint32_t unit = P4_G32(action + 0x30);
    p4_action_exec_admit(action, uid_at(action), P4_G32(unit + 0xa8),
        P4_G16(unit + 0xa4), P4_G8(unit + 0xa2), P4_G16(action + 0x18),
        P4_G8(action + 0x28), P4_G8(action + 0x29), P4_G32(unit + 0xa64));
}
void p4_action_exec_native_command(uint32_t action)
{
    p4_action_exec_command(action, P4_G16(action + 0x6c), P4_G16(action + 0x6e));
}
void p4_action_exec_native_end(uint32_t action)
{
    p4_action_exec_end_seen(action, (P4_G16(action + 0x18) & 0x8000) != 0);
}
void p4_action_exec_native_checkpoint(uint32_t action)
{
    /* ENDHOME queues its final rotation before returning to STANDBY (1).
     * END's restart=1 also returns to 1, but has not marked end_processed. */
    if (P4_G16(action + 0xc) == 1) p4_action_exec_standby(action);
}
void p4_action_exec_native_packet_add(uint32_t packet)
{
    p4_action_exec_packet_add(packet, uid_at(packet + 0x60), P4_G32(packet + 0x40));
}
int p4_action_exec_native_unit(uint32_t data, P4ActionExecUnit *out)
{
    if (!data || !out || !p4_action_exec_battle_active) return 0;
    const uint32_t battle = P4_GPDATA(uint32_t, 0xb3ac);
    if (!battle) return 0;
    uint32_t action = P4_G32(battle + 0x174);
    P4ActionExecUnit found = {0};
    for (unsigned i = 0; action && i < P4_EXEC_RECORDS; ++i) {
        const uint32_t unit = P4_G32(action + 0x30);
        if (unit && P4_G32(unit + 0xa64) == data) {
            if (found.action) return 0;
            found = (P4ActionExecUnit){action, data, P4_G32(unit + 0xa8),
                P4_G16(unit + 0xa4), P4_G8(unit + 0xa2)};
        }
        action = P4_G32(action + 0x450);
    }
    if (action || !found.action) return 0;
    *out = found;
    return 1;
}
