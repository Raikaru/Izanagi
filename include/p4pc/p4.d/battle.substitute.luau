--!capability battle.substitute@1
-- p4.battle.substitute: replace an enemy's chosen command inside a before-hook on btlAICommandSetActionCode (0x001DE640).
--!in p4.battle
-- Only inside a before-hook on 0x001de640 for its own action;
-- false plus the reason when the skill is inactive or not self/all-party.
substitute: (action: number, skill: number) -> (boolean, string?),
--!example
local tactic: P4Subscription = p4.hook(0x1de640, "vppH", "before", function(context)
    local ok: boolean, why: string? = p4.battle.substitute(context.args[1] :: number, 227)
end)
--!refuse
p4.battle.substitute(0x1000, "Power Charge")
local s: string = p4.battle.substitute(0x1000, 227)
