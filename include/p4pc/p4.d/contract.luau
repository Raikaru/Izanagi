--!capability contract@1
-- p4.contract.publish/read: a mod publishes frozen plain values for the mods that name it in [require] or [companion].
--!in p4.contract
-- Only while main.luau runs: a frozen copy (p4.save value rules, at most
-- 64 KiB, 16 per mod) under a stable name and a positive integer version.
publish: (name: string, version: number, value: P4SaveValue) -> (),
-- A [require]/[companion] owner's contract as read-only tables and its
-- version, or nil when that owner is absent or never published it.
read: (owner: string, name: string) -> (P4SaveValue, number?),
--!example
p4.contract.publish("profile", 1, { mode = "hard", ranks = { 1, 2 } })
local profile, profile_version = p4.contract.read("p4pc.base", "profile")
local known: boolean = profile ~= nil and profile_version ~= nil
--!refuse
p4.contract.publish("profile", "one", true)
p4.contract.publish("profile", 1, buffer.create(1))
local n: number = p4.contract.read("p4pc.base", "profile")
