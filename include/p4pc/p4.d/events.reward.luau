--!capability events.reward@1
-- p4.events.on("reward"): a won battle's EXP, yen and items, once the result has given them.
export type P4RewardEvent = {
    name: "reward", sequence: number,
    exp: number, -- the protagonist's EXP
    money: number, -- the yen given (after an Arcana Chance and p4.reward modifiers)
    items: { { item: P4Row, count: number } },
    party: { { character: string?, exp: number } }, -- the party members who earned EXP
}
--!overload p4.events.on
<T>(name: "reward", callback: (event: P4RewardEvent) -> ()) -> P4Subscription
--!example
p4.events.on("reward", function(e: P4RewardEvent)
    local total: number = e.exp
    for _, member in e.party do total += member.exp end
    local first: string? = if #e.items > 0 then e.items[1].item.name else nil
end)
--!refuse
p4.events.on("reward", function(e) local s: string = e.money end)
