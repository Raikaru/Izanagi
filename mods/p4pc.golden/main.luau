--!strict
-- p4pc.golden: Persona 4 Golden's battle mechanics (docs/modding/golden.md).
-- The table groups are applied natively at startup by the p4pc:golden derived
-- producer (mod.ini [table-derived golden]); this script carries the groups
-- that are behaviour rather than table rows.

--------------------------------------------------------------------------
-- SOCIAL LINK PERK ORDER
--
-- PS2 and Golden have the same five battle perks per party Social Link, in a
-- different rank order (decomp src/Battle/btlTarget_inferred_001eec60.c and
-- datCalc.c). The Golden order is a published fact (Megami Tensei Wiki,
-- megamitensei.fandom.com/wiki/Social_Link/<member>, Persona 4 Golden tab);
-- see docs/modding/golden.md, "Published facts: the one exception":
--
--   perk                         PS2 rank   Golden rank   PS2 test
--   take a mortal blow for Yu        1           9        0x001EFD50 rank > 0
--   follow-up attack                 3           3        0x001EFF50 rank >= 3
--   help a downed ally up            5           1        0x001EF9C0 rank >= 5
--   cure severe ailments             7           5        0x001EF9C0 rank >= 7
--   endure a mortal blow             9           7        0x00242990 rank >= 9
--
-- While one of those functions runs, the rank it reads through
-- cmmCommunityRecordValue is replaced by the PS2 rank that grants the same
-- perk at the player's real rank, so each PS2 test passes exactly when the
-- Golden rank is reached. The rank-up announcement (0x00255ED0, which maps a
-- rank to the perk message) is remapped the same way. Rise's navigator ranks
-- are untouched. Nothing is hooked while the setting is off.
--------------------------------------------------------------------------

local A = {
    cmmCommunityRecordValue = 0x00107AC0,
    btlActionPickStatusTarget = 0x001EF9C0,
    btlActionPickRandomCandidate = 0x001EFD50,
    datCalcResolveSkillId = 0x00242990,
    cmmRankUpSelectDisplayIndex = 0x00255ED0,
}

local function seam(address: number): string
    local info = p4.describe(address)
    if not info then
        error(string.format("p4pc.golden: this build cannot hook %08x", address))
    end
    return info.signature
end

-- The PS2 rank that grants, at Golden rank `rank`, the perk(s) a PS2 test reads.
type Remap = (rank: number) -> number
local remapStatus: Remap = function(rank) -- help up at 1, cure at 5
    if rank >= 5 then return 7 end
    if rank >= 1 then return 5 end
    return 0
end
local remapProtect: Remap = function(rank) -- take the blow at 9
    return if rank >= 9 then 1 else 0
end
local remapEndure: Remap = function(rank) -- endure at 7
    return if rank >= 7 then 9 else 0
end
-- Golden announcement rank -> the PS2 rank whose message announces that perk.
local ANNOUNCE: {[number]: number} = {[1] = 5, [3] = 3, [5] = 7, [7] = 9, [9] = 1}
-- Rise's navigator messages keep their own ranks.
local RISE_MESSAGES: {[number]: boolean} = {[7] = true, [8] = true, [9] = true}

-- Subscriptions are made once at load; the rank hook is only switched on
-- while one of the perk tests runs.
local hooks: {P4Subscription} = {}

local function installPerkOrder()
    local active: Remap? = nil
    local rankHook = p4.hook(A.cmmCommunityRecordValue, seam(A.cmmCommunityRecordValue), "replace", function(ctx)
        ctx.next()
        local remap = active
        if remap then
            ctx.result = remap(ctx.result :: number)
        end
    end)
    rankHook:disable()
    table.insert(hooks, rankHook)

    local function scope(address: number, remap: Remap)
        local signature = seam(address)
        table.insert(hooks, p4.hook(address, signature, "before", function(_ctx)
            active = remap
            rankHook:enable()
        end))
        table.insert(hooks, p4.hook(address, signature, "after", function(_ctx)
            active = nil
            rankHook:disable()
        end))
    end
    scope(A.btlActionPickStatusTarget, remapStatus)
    scope(A.btlActionPickRandomCandidate, remapProtect)
    scope(A.datCalcResolveSkillId, remapEndure)

    local announce = seam(A.cmmRankUpSelectDisplayIndex)
    table.insert(hooks, p4.hook(A.cmmRankUpSelectDisplayIndex, announce, "replace", function(ctx)
        ctx.next()
        if RISE_MESSAGES[ctx.result :: number] then return end
        local ps2 = ANNOUNCE[ctx.args[2] :: number]
        ctx.result = if ps2 then p4.call(A.cmmRankUpSelectDisplayIndex, announce, ctx.args[1], ps2) else 0
    end))
    p4.log("Golden Social Link perk order installed")
end

--------------------------------------------------------------------------
-- SOCIAL LINK PERSONA SKILLS
--
-- In Golden each party Social Link also teaches its member's Persona skills
-- at set ranks, listed in Golden's cmmRankUpSkillTable.dat (data_e.cpk
-- init.bin, event/cmm.bin; rows u16 member, u16 rank, u32 skill with the
-- skill it replaces in the high half). The rows below are that table as PS2
-- skill ids: Teddie's first row is rank 0 (taught once he has his Persona),
-- Rise's are her navigator skills (rank 3 replaces Analysis; her rank 8
-- Stamina Song is Golden-only and left out), and the rank 11 third-awakening
-- rows are out of scope. They are carried here until the script reads that
-- table from the player's install (docs/modding/golden.md).
--
-- Grants happen when the player next has control of an idle field after the
-- rank-up (so after a MAX rank's awakening), and once after loading a save
-- that already has the ranks. A skill the Persona knows is only marked done.
-- With a free slot the skill is learned and announced; with eight skills it
-- waits in MODS > p4pc.golden > PERSONA SKILLS, where the player picks the
-- skill to replace or declines it. A save records what was taught and what
-- waits, so a skill the player later replaces is not taught again.
--------------------------------------------------------------------------

local PARTY_PERSONA = 0x00796D94 -- + member id * 0x88: id +2, eight skills +0x0C (read only)
local LIVE_SKILL_NAMES = 0x00764538 -- MSG.TBL skill names, 0x13 bytes each
local cmmCommunityForArcana = 0x00247CB0 -- member id -> its Social Link, 0 without one
local cmmScriptRankUp = 0x0024B380 -- CMM_RANKUP: 1 once the rank-up sequence ends
local datPersonaGetRecord = 0x0010A900 -- member id -> its Persona record
local datPersonaAddSkill = 0x0010CC20 -- first empty slot; 0 when all eight are taken
local datPersonaReplaceSkill = 0x0010CD70 -- old skill -> new skill; -1 when old is absent
local RANKS = {0, 2, 3, 4, 5, 6, 8, 10}
local MEMBERS: {{id: number, name: string, skills: {[number]: number}, replaces: {[number]: number}?}} = {
    {id = 2, name = "YOSUKE", skills = {[2] = 244, [4] = 224, [6] = 516, [8] = 193, [10] = 485}},
    {id = 3, name = "CHIE", skills = {[2] = 234, [4] = 489, [6] = 235, [8] = 27, [10] = 479}},
    {id = 4, name = "YUKIKO", skills = {[2] = 72, [4] = 526, [6] = 208, [8] = 73, [10] = 481}},
    {id = 5, name = "RISE", skills = {[2] = 0x10D, [3] = 0x10B, [4] = 0x10C, [5] = 0x10E},
        replaces = {[3] = 0x110}},
    {id = 6, name = "KANJI", skills = {[2] = 540, [4] = 214, [6] = 227, [8] = 507, [10] = 483}},
    {id = 7, name = "NAOTO", skills = {[2] = 508, [4] = 228, [6] = 509, [8] = 225, [10] = 510}},
    {id = 8, name = "TEDDIE", skills = {[0] = 246, [4] = 209, [6] = 216, [8] = 210, [10] = 485}},
}
local ANNOUNCE_SECONDS = 6

type Grant = {member: number, rank: number}
local taught: {[string]: boolean} = {} -- "member:rank" taught, known already, or declined
local waiting: {Grant} = {}

local function grantKey(member: number, rank: number): string
    return string.format("%d:%d", member, rank)
end
local function memberInfo(member: number)
    for _, info in MEMBERS do
        if info.id == member then return info end
    end
    error("p4pc.golden: unknown party member " .. tostring(member))
end
local function skillName(id: number): string
    local names = p4.mem.read("u32", LIVE_SKILL_NAMES)
    if names == 0 then return "?" end
    return (string.gsub(p4.mem.read_string(names + id * 0x13, 0x13), "[^ -~]", "?"))
end
local function slotAddress(member: number, slot: number): number
    return PARTY_PERSONA + member * 0x88 + 0x0C + (slot - 1) * 2
end

local function installRankSkills()
    local forArcana = seam(cmmCommunityForArcana)
    local recordValue = seam(A.cmmCommunityRecordValue)
    local getRecord = seam(datPersonaGetRecord)
    local addSkill = seam(datPersonaAddSkill)
    local replaceSkill = seam(datPersonaReplaceSkill)
    local function rankOf(member: number): number
        local link = p4.call(cmmCommunityForArcana, forArcana, p4.u64(0, member)) :: number
        if link == 0 then return 0 end
        return p4.call(A.cmmCommunityRecordValue, recordValue, link) :: number
    end
    local function record(member: number): number
        return p4.call(datPersonaGetRecord, getRecord, member) :: number
    end

    local announceUntil = 0
    local function announce(text: string)
        p4.ui.status(string.sub(text, 1, 47), "hud")
        announceUntil = p4.clock.wall() + ANNOUNCE_SECONDS
    end

    local waitingPage: P4UIPage
    local replacePages: {[number]: P4UIPage} = {}
    local function refreshPages()
        waitingPage:refresh()
        for _, page in replacePages do page:refresh() end
    end

    -- Teaches one reached rank's skill through the game's own functions: in
    -- place of the skill its row replaces when the Persona has that one
    -- (datPersonaReplaceSkill), else in a free slot (datPersonaAddSkill), else
    -- it waits for the player to pick a slot.
    local function teach(member: number, rank: number)
        local info = memberInfo(member)
        local skill = info.skills[rank]
        local old = if info.replaces then info.replaces[rank] else nil
        local hasOld = false
        for slot = 1, 8 do
            local known = p4.mem.read("u16", slotAddress(member, slot))
            if known == skill then
                taught[grantKey(member, rank)] = true
                return
            end
            if old and known == old then hasOld = true end
        end
        local learned = if old and hasOld
            then (p4.call(datPersonaReplaceSkill, replaceSkill, record(member), old, skill) :: number) >= 0
            else p4.call(datPersonaAddSkill, addSkill, record(member), skill) ~= 0
        if learned then
            taught[grantKey(member, rank)] = true
            announce(string.format("%s LEARNED %s", info.name, skillName(skill)))
            p4.log(string.format("rank skill taught member=%d rank=%d skill=%d", member, rank, skill))
        else
            table.insert(waiting, {member = member, rank = rank})
            announce(string.format("%s CAN LEARN %s: SEE MODS", info.name, skillName(skill)))
            p4.log(string.format("rank skill waiting member=%d rank=%d skill=%d", member, rank, skill))
        end
    end

    local function isWaiting(member: number, rank: number): boolean
        for _, grant in waiting do
            if grant.member == member and grant.rank == rank then return true end
        end
        return false
    end

    local function grantReached()
        for _, info in MEMBERS do
            if p4.mem.read("u16", PARTY_PERSONA + info.id * 0x88 + 2) ~= 0 then
                local rank = rankOf(info.id)
                for _, r in RANKS do
                    if info.skills[r] and rank >= r and not taught[grantKey(info.id, r)] and not isWaiting(info.id, r) then
                        teach(info.id, r)
                    end
                end
            end
        end
        refreshPages()
    end

    -- The player's choice from a page, applied at the next frame boundary.
    type Choice = {grant: Grant, slot: number?} -- slot nil: decline
    local choice: Choice? = nil
    local check = false

    local watcher: P4Subscription
    watcher = p4.events.on("frame_start", function(_e)
        local picked = choice
        if picked then
            choice = nil
            local index = table.find(waiting, picked.grant)
            if index then
                local grant = picked.grant
                table.remove(waiting, index)
                taught[grantKey(grant.member, grant.rank)] = true
                local slot = picked.slot
                if slot then
                    local info = memberInfo(grant.member)
                    local skill = info.skills[grant.rank]
                    local old = p4.mem.read("u16", slotAddress(grant.member, slot))
                    if old ~= 0 and (p4.call(datPersonaReplaceSkill, replaceSkill, record(grant.member), old, skill) :: number) >= 0 then
                        announce(string.format("%s LEARNED %s", info.name, skillName(skill)))
                        p4.log(string.format("rank skill taught member=%d rank=%d skill=%d replacing=%d",
                            grant.member, grant.rank, skill, old))
                    end
                end
                refreshPages()
            end
        end
        if check and p4.scene.state().ready then
            check = false
            grantReached()
        end
        if announceUntil ~= 0 and p4.clock.wall() >= announceUntil then
            announceUntil = 0
            p4.ui.status(nil, "hud")
        end
        if not check and announceUntil == 0 and not choice then watcher:disable() end
    end)
    watcher:disable()
    table.insert(hooks, watcher)
    local function wake()
        check = true
        watcher:enable()
    end

    table.insert(hooks, p4.hook(cmmScriptRankUp, seam(cmmScriptRankUp), "after", function(ctx)
        if ctx.result == 1 then wake() end
    end))

    -- A member's earliest waiting skill; the hub lists one per member.
    local function firstWaiting(member: number): Grant?
        for _, grant in waiting do
            if grant.member == member then return grant end
        end
        return nil
    end
    local function waitingMembers(): {number}
        local out = {}
        for _, grant in waiting do
            if not table.find(out, grant.member) then table.insert(out, grant.member) end
        end
        return out
    end

    for _, info in MEMBERS do
        local member = info.id
        replacePages[member] = p4.ui.page({
            key = "replace" .. tostring(member), title = info.name .. ": REPLACE A SKILL", listed = false,
            count = function(): number return if firstWaiting(member) then 9 else 1 end,
            row = function(index: number): P4UIRow
                local grant = firstWaiting(member)
                if not grant then return {key = "none", label = "NOTHING TO LEARN", kind = "info"} end
                local learn = skillName(info.skills[grant.rank])
                if index == 9 then
                    return {key = "decline", label = "DO NOT LEARN " .. learn, kind = "action",
                            help = "The skill is not offered again."}
                end
                local known = p4.mem.read("u16", slotAddress(member, index))
                return {key = "slot" .. tostring(index), label = skillName(known), kind = "action",
                        help = string.format("Confirm: forget it and learn %s.", learn)}
            end,
            find = function(key: string): number?
                if key == "decline" then return 9 end
                local slot = tonumber(string.match(key, "^slot(%d)$"))
                return if slot and slot >= 1 and slot <= 8 then slot else nil
            end,
            adjust = function(key: string, direction: number)
                local grant = firstWaiting(member)
                if direction ~= 0 or not grant or choice then return end
                local slot = tonumber(string.match(key, "^slot(%d)$"))
                if key ~= "decline" and not slot then return end
                choice = {grant = grant, slot = slot}
                watcher:enable()
                p4.ui.back()
            end,
        })
    end
    waitingPage = p4.ui.page({
        key = "skills", title = "PERSONA SKILLS",
        count = function(): number return math.max(#waitingMembers(), 1) end,
        row = function(index: number): P4UIRow
            local member = waitingMembers()[index]
            local grant = if member then firstWaiting(member) else nil
            if not member or not grant then
                return {key = "none", label = "NO SKILLS WAITING", kind = "info"}
            end
            local info = memberInfo(member)
            return {key = "m" .. tostring(member), kind = "link", target = replacePages[member].key,
                    label = string.format("%s: %s", info.name, skillName(info.skills[grant.rank])),
                    help = string.format("Social Link rank %d. All eight skills are taken: choose one to replace.",
                        grant.rank)}
        end,
        find = function(key: string): number?
            local member = tonumber(string.match(key, "^m(%d+)$"))
            return if member then table.find(waitingMembers(), member) else nil
        end,
    })

    p4.save.register({
        schema = 1,
        max_bytes = 2048,
        snapshot = function(): P4SaveValue
            local done, open = {}, {}
            for key in taught do table.insert(done, key) end
            for _, grant in waiting do table.insert(open, grantKey(grant.member, grant.rank)) end
            if #done == 0 and #open == 0 then return nil end
            table.sort(done)
            return {taught = done, waiting = open}
        end,
        load = function(value: P4SaveValue, origin: "load" | "reset")
            table.clear(taught)
            table.clear(waiting)
            choice = nil
            local function keys(list: P4SaveValue): {string}
                local out = {}
                if type(list) == "table" then
                    for _, key in list do
                        if type(key) == "string" then table.insert(out, key) end
                    end
                end
                return out
            end
            if type(value) == "table" then
                for _, key in keys(value.taught) do taught[key] = true end
                for _, key in keys(value.waiting) do
                    local member, rank = string.match(key, "^(%d+):(%d+)$")
                    if member and rank then
                        table.insert(waiting, {member = tonumber(member) :: number, rank = tonumber(rank) :: number})
                    end
                end
            end
            if origin == "load" then wake() end
        end,
    })
    p4.log("Golden Social Link Persona skills installed")
end

if p4.settings.get("perk_order") == true then
    installPerkOrder()
end
if p4.settings.get("rank_skills") == true then
    installRankSkills()
end
