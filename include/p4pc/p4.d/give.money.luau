--!capability give.money@1
-- p4.give.money and p4.take.money: change the player's yen (0..9,999,999); returns the change, the requested change and "queued ..." during a battle, or 0 and a refusal.
--!in p4.give
money: (amount: number) -> (number, string?),
--!in p4.take
money: (amount: number) -> (number, string?),
--!example
local paid: number, refused: string? = p4.give.money(5000)
local spent: number = p4.take.money(100)
--!refuse
p4.give.money("lots")
