--!capability reward@1
-- p4.reward.on: change the EXP each recipient and the money the team earns from a won battle (docs/modding/tables.md "Reward stage").
-- One award, frozen. reward "exp" names its recipient: the hero (id 1), a party member (character id) or one of
-- the protagonist's Personas (persona id; equipped is true for the one he carries, false for a stock Persona earning
-- through Growth); level is the recipient's. reward "money" is the team's yen and has no recipient, id or level.
-- stock is the game's award; 0 pays nothing.
export type P4RewardQuery = {
    reward: "exp" | "money",
    recipient: ("hero" | "party" | "persona")?,
    id: number?,
    level: number?,
    equipped: boolean?,
    stock: number,
}
-- fn returns a factor (multiply) or an addend (add), or nil to leave it alone; the result is truncated and clamped
-- to 0..999999. Money is asked every frame the results screen shows it, then for the payment: answer the same each time.
export type P4RewardSpec = { kind: "multiply" | "add", priority: number?, fn: (query: P4RewardQuery) -> number? }
--!in p4
reward: { on: (specification: P4RewardSpec) -> P4Subscription },
--!example
local cap: P4Subscription = p4.reward.on({
    kind = "multiply",
    fn = function(query: P4RewardQuery): number?
        if query.reward == "exp" and (query.level or 0) >= 30 then return 0 end -- level cap 30
        if query.reward == "money" then return 1.5 end
        return nil
    end,
})
--!refuse
p4.reward.on({ kind = "set", fn = function(query) return 0 end })
p4.reward.on({ kind = "add", fn = function(query) local r: "item" = query.reward; return nil end })
