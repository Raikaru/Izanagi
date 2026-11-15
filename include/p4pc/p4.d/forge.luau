--!capability forge@1
-- p4.forge: the weapon fusion transaction, applied at the next frame safe point outside battle after the weapon_fusion commit stage.
-- A weapon fusion: consume a carried non-lead Persona, one unequipped vessel
-- weapon and fee yen; add one item (a weapon the character can equip).
export type P4ForgeRequest = {
    character: number, persona: number, vessel: number, item: number, fee: number,
    signature: boolean?,
}
export type P4ForgeResult = {
    outcome: "forged" | "refused", reason: string?,
    character: number, persona: number, vessel: number, item: number, fee: number,
    signature: boolean,
}
--!in p4.forge
-- Weapon fusion transaction: commit validates and queues; the next frame
-- safe point outside battle forges or refuses; result takes the outcome
-- once. commit is refused in synchronous callbacks and snapshots.
commit: (request: P4ForgeRequest) -> (boolean, string?),
result: () -> P4ForgeResult?,
pending: () -> boolean,
--!example
local queued: boolean, why: string? = p4.forge.commit({character = 1, persona = 7, vessel = 2, item = 1600, fee = 1200})
p4.forge.commit({character = 4, persona = 100, vessel = 88, item = 1601, fee = 0, signature = true})
local done = p4.forge.result()
if done and done.outcome == "refused" then local text: string? = done.reason end
local waiting: boolean = p4.forge.pending()
--!refuse
p4.forge.commit({character = 1, persona = 7, vessel = 2, fee = 0})
p4.forge.commit({character = "yu", persona = 7, vessel = 2, item = 3, fee = 0})
local o: "made" = p4.forge.result().outcome
