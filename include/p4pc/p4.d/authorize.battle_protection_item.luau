--!capability authorize.battle_protection_item@1
--!covers operation:battle_protection_item
-- Asked when a party member's held item would be considered for battle survival.
-- A denial skips that item and continues native passive and social-link checks.
export type P4BattleProtectionItemContext = {
    name: "battle_protection_item", character: number, item: number, element: number,
}
--!overload p4.authorize.on
(operation: "battle_protection_item", callback: (context: P4BattleProtectionItemContext) -> (boolean, ...string)) -> P4Subscription
--!example
p4.authorize.on("battle_protection_item", function(context)
    return context.item ~= 0x340, "item protection denied"
end)
--!refuse
p4.authorize.on("battle_protection_item", function(context) local id: string = context.item; return true end)
