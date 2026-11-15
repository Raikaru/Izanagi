--!capability authorize.retry@1
-- p4.authorize.on("retry"): allow or refuse a battle retry once per lost field battle, before the p4.battle.defeat handler is asked.
-- source: 1 for a field script's encounter, 0 for a shadow touched on the field.
export type P4RetryContext = { name: "retry", encounter: number, source: number }
--!overload p4.authorize.on
(operation: "retry", callback: (context: P4RetryContext) -> (boolean, ...string)) -> P4Subscription
--!example
local rule: P4Subscription = p4.authorize.on("retry", function(context: P4RetryContext): (boolean, ...string)
    return false, "IRONMAN: a lost battle ends this run"
end)
--!refuse
p4.authorize.on("retry", function(context: P4RetryContext): number return context.encounter end)
