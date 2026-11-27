--!capability authorize.item_use@1
--!covers operation:item_use
-- Source 0: field item application (effect is its native skill/effect ID).
-- Source 1: battle item-list confirmation; source 2: party revival item.
-- Source 3: custom field-category callback; its effects are unspecified.
-- Source 4: party AI script item setter; denial selects native wait.
-- Source 5: party battle action at READY, including direct and queued writers.
export type P4ItemUseContext = { name: "item_use", source: number, item: number, effect: number }
--!overload p4.authorize.on
(operation: "item_use", callback: (context: P4ItemUseContext) -> (boolean, ...string)) -> P4Subscription
--!example
p4.authorize.on("item_use", function(context)
    return context.source ~= 2, "revival item refused"
end)
--!refuse
p4.authorize.on("item_use", function(context) local item: string = context.item; return true end)
