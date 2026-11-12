--!capability stage.enemy-reward@1
-- One defeated enemy's EXP or yen result at battle reward aggregation only.
-- unit is an opaque identity token, valid for matching the same live unit only;
-- enemy is its enemy ID, amount is the current integer result (0..65535).
export type P4EnemyReward = {unit: number, enemy: number, amount: number}
--!in p4.enemy_reward
on: (kind: "exp" | "yen", fn: (reward: P4EnemyReward) -> number?) -> P4Subscription,
--!example
local reward: P4Subscription = p4.enemy_reward.on("exp", function(query: P4EnemyReward): number?
    if query.enemy == 42 then return math.min(65535, math.floor(query.amount * 1.2 + 0.5)) end
    return nil
end)
reward:disable()
--!refuse
p4.enemy_reward.on("money", function(query) return nil end)
p4.enemy_reward.on("exp", function(query) return "more" end)
