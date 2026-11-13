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

-- HUD status rows are this mod's as a whole (p4.ui.status replaces them):
-- features set their own line by key, the bike ride offer above skills
-- learned or waiting.
local statusLines: {[string]: string} = {}
local STATUS_ORDER = {"ride", "grant"}
local function setStatus(key: string, text: string?)
    statusLines[key] = if text then string.sub(text, 1, 47) else nil
    local rows = {}
    for _, k in STATUS_ORDER do
        local line = statusLines[k]
        if line then table.insert(rows, line) end
    end
    p4.ui.status(if #rows > 0 then rows else nil, "hud")
end

-- One save provider for the mod (schema 1, plain tables): each feature writes
-- its own fields of the block and rebuilds its state from them on load.
type SaveBlock = {[boolean | number | string]: P4SaveValue}
type SavePart = {
    snapshot: (block: SaveBlock) -> (),
    load: (value: P4SaveValue, origin: "load" | "reset") -> (),
    authorizeSave: (() -> (boolean, string))?,
}
local saveParts: {SavePart} = {}
local function registerSave()
    if #saveParts == 0 then return end
    p4.save.register({
        schema = 1,
        max_bytes = 4096,
        authorize_save = function(): (boolean, string)
            for _, part in saveParts do
                local authorize = part.authorizeSave
                if authorize then
                    local ok, reason = authorize()
                    if not ok then return false, reason end
                end
            end
            return true, ""
        end,
        snapshot = function(): P4SaveValue
            local block: SaveBlock = {}
            for _, part in saveParts do part.snapshot(block) end
            return if next(block) ~= nil then block else nil
        end,
        load = function(value: P4SaveValue, origin: "load" | "reset")
            for _, part in saveParts do part.load(value, origin) end
        end,
    })
end

--------------------------------------------------------------------------
-- GOLDEN SOURCE FILES
--
-- The p4pc:golden producer records two raw files of the player's install for
-- this script (p4.tables.source): init/cmpTable.bin (field menu tables, the
-- bike-ride skills) and event/cmm.bin (community tables, the Social Link rank
-- skills). Both are Golden containers: a u32 count, then per entry a 32-byte
-- name, a u32 size and that many bytes. Golden skill ids become PS2 ids as in
-- the native translation (golden_tables.c): actives below 440 keep their id,
-- 440..471 are Golden-only, passives are the PS2 id plus 32, and the PS2 slot
-- must have a name.
--------------------------------------------------------------------------

local LIVE_SKILL_NAMES = 0x00764538 -- MSG.TBL skill names, 0x13 bytes each
local MEMBER_NAMES: {[number]: string} = {
    [2] = "YOSUKE", [3] = "CHIE", [4] = "YUKIKO", [5] = "RISE", [6] = "KANJI", [7] = "NAOTO", [8] = "TEDDIE",
}

local function goldenSource(name: string): buffer
    local bytes = p4.tables.source("p4pc:golden", name)
    if not bytes then error("p4pc.golden: the p4pc:golden producer recorded no " .. name) end
    return bytes
end
-- The data offset and size of one container entry.
local function section(file: buffer, want: string): (number, number)
    local count, at, size = buffer.readu32(file, 0), 4, buffer.len(file)
    for _ = 1, count do
        if at + 36 > size then break end
        local length = buffer.readu32(file, at + 32)
        if length > size - at - 36 then break end
        local name = string.match(buffer.readstring(file, at, 32), "^[^%z]*")
        if name == want then return at + 36, length end
        at += 36 + length
    end
    error("p4pc.golden: Golden file has no " .. want)
end
local function skillNameRaw(id: number): string
    local names = p4.mem.read("u32", LIVE_SKILL_NAMES)
    if names == 0 then error("p4pc.golden: the skill names are not loaded") end
    return p4.mem.read_string(names + id * 0x13, 0x13)
end
-- "Blank", empty or a hex placeholder ("116"): a PS2 slot without a skill.
local function placeholder(name: string): boolean
    return name == "" or name == "Blank" or name == "blank" or string.match(name, "^[0-9A-F][0-9A-F][0-9A-F]?$") ~= nil
end
local function ps2Skill(golden: number): number?
    local id = if golden < 440 then golden elseif golden >= 472 then golden - 32 else nil
    if not id or id <= 0 or id >= 576 or placeholder(skillNameRaw(id)) then return nil end
    return id
end

--------------------------------------------------------------------------
-- SOCIAL LINK RANK SKILL TABLE
--
-- In Golden each party Social Link also teaches its member's Persona skills
-- at set ranks: cmmRankUpSkillTable.dat in event/cmm.bin, after an 8-byte
-- header rows of u16 member, u16 rank and u32 skill (the high half naming a
-- skill it replaces), ended by a zero row. Rank 11 (the third awakenings) and
-- skills PS2 lacks (Rise's Stamina Song) are left out; Teddie's first row is
-- rank 0, taught once he has his Persona.
--------------------------------------------------------------------------

type Member = {id: number, name: string, skills: {[number]: number}, replaces: {[number]: number}, ranks: {number}}
local MEMBERS: {Member} = {}

-- Fills MEMBERS from the install's table, once the skill names are loaded.
local function loadRankSkills()
    if #MEMBERS > 0 then return end
    local file = goldenSource("event/cmm.bin")
    local at, size = section(file, "cmmRankUpSkillTable.dat")
    local byId: {[number]: Member} = {}
    for row = at + 8, at + size - 8, 8 do
        local member, rank, skill = buffer.readu16(file, row), buffer.readu16(file, row + 2), buffer.readu32(file, row + 4)
        if member == 0 and rank == 0 and skill == 0 then break end
        local learn, old = ps2Skill(bit32.band(skill, 0xFFFF)), bit32.rshift(skill, 16)
        if MEMBER_NAMES[member] and rank <= 10 and learn then
            local info = byId[member]
            if not info then
                info = {id = member, name = MEMBER_NAMES[member], skills = {}, replaces = {}, ranks = {}}
                byId[member] = info
                table.insert(MEMBERS, info)
            end
            info.skills[rank] = learn
            table.insert(info.ranks, rank)
            local replaced = if old ~= 0 then ps2Skill(old) else nil
            if replaced then info.replaces[rank] = replaced end
        end
    end
    for _, info in MEMBERS do table.sort(info.ranks) end
    p4.log(string.format("Golden rank skills read: %d members", #MEMBERS))
end
local function memberInfo(member: number): Member
    loadRankSkills()
    for _, info in MEMBERS do
        if info.id == member then return info end
    end
    error("p4pc.golden: unknown party member " .. tostring(member))
end

--------------------------------------------------------------------------
-- PERSONA SKILL GRANTS (rank skills and bike rides)
--
-- Both features teach a party member's Persona through the game's own
-- functions: in place of the skill a Social Link row replaces when the
-- Persona has that one (datPersonaReplaceSkill), else in a free slot
-- (datPersonaAddSkill), else the skill waits in MODS > p4pc.golden > PERSONA
-- SKILLS, where the player picks the skill to replace or declines it. A skill
-- the Persona already knows is only marked done. The save block keeps the
-- taught and waiting rank skills and the waiting ride skills, so a skill the
-- player later replaces or declines is never taught again.
--------------------------------------------------------------------------

local PARTY_PERSONA = 0x00796D94 -- + member id * 0x88: id +2, level +4, eight skills +0x0C (read only)
local cmmCommunityForArcana = 0x00247CB0 -- member id -> its Social Link, 0 without one
local datPersonaGetRecord = 0x0010A900 -- member id -> its Persona record
local datPersonaAddSkill = 0x0010CC20 -- first empty slot; 0 when all eight are taken
local datPersonaReplaceSkill = 0x0010CD70 -- old skill -> new skill; -1 when old is absent
local ANNOUNCE_SECONDS = 6

-- A Social Link rank's skill (rank; the skill is the rank table's) or a bike
-- ride's skill (skill).
type Grant = {member: number, rank: number?, skill: number?}
local taught: {[string]: boolean} = {} -- "member:rank" taught, known already, or declined
local waiting: {Grant} = {}

local function rankKey(member: number, rank: number): string
    return string.format("%d:%d", member, rank)
end
local function grantKey(grant: Grant): string
    local rank = grant.rank
    if rank then return rankKey(grant.member, rank) end
    return string.format("ride:%d:%d", grant.member, grant.skill :: number)
end
local function grantSkill(grant: Grant): number
    local rank = grant.rank
    if rank then return memberInfo(grant.member).skills[rank] end
    return grant.skill :: number
end
local function skillName(id: number): string
    local names = p4.mem.read("u32", LIVE_SKILL_NAMES)
    if names == 0 then return "?" end
    return (string.gsub(p4.mem.read_string(names + id * 0x13, 0x13), "[^ -~]", "?"))
end
local function slotAddress(member: number, slot: number): number
    return PARTY_PERSONA + member * 0x88 + 0x0C + (slot - 1) * 2
end
local function hasPersona(member: number): boolean
    return p4.mem.read("u16", PARTY_PERSONA + member * 0x88 + 2) ~= 0
end
local function knows(member: number, skill: number): boolean
    for slot = 1, 8 do
        if p4.mem.read("u16", slotAddress(member, slot)) == skill then return true end
    end
    return false
end
local function waitingFor(member: number, skill: number): boolean
    for _, grant in waiting do
        if grant.member == member and grantSkill(grant) == skill then return true end
    end
    return false
end

-- Set by installGrants, which every teaching feature calls first.
local seams: {[string]: string} = {}
local grantWatcher: P4Subscription? = nil
local waitingPage: P4UIPage? = nil
local replacePages: {[number]: P4UIPage} = {}
local readyChecks: {() -> ()} = {} -- run once at the next ready frame
local afterLoad: {() -> ()} = {} -- queued as ready checks after a real load
local announceUntil = 0

local function refreshPages()
    if waitingPage then waitingPage:refresh() end
    for _, page in replacePages do page:refresh() end
end
local function wakeGrants()
    local watcher = grantWatcher
    if watcher then watcher:enable() end
end
local function whenReady(check: () -> ())
    if not table.find(readyChecks, check) then table.insert(readyChecks, check) end
    wakeGrants()
end
local function announce(text: string)
    setStatus("grant", text)
    announceUntil = p4.clock.wall() + ANNOUNCE_SECONDS
    wakeGrants()
end
local function personaRecord(member: number): number
    return p4.call(datPersonaGetRecord, seams.getRecord, member) :: number
end
-- The member's Social Link rank; 0 before it begins.
local function rankOf(member: number): number
    local link = p4.call(cmmCommunityForArcana, seams.forArcana, p4.u64(0, member)) :: number
    if link == 0 then return 0 end
    return p4.call(A.cmmCommunityRecordValue, seams.recordValue, link) :: number
end

-- Teaches one grant now: "known" (the Persona has it), "taught", or
-- "waiting" for the player's choice on PERSONA SKILLS.
local function teach(grant: Grant, replaces: number?): string
    local member, skill = grant.member, grantSkill(grant)
    local hasOld = false
    for slot = 1, 8 do
        local known = p4.mem.read("u16", slotAddress(member, slot))
        if known == skill then return "known" end
        if replaces and known == replaces then hasOld = true end
    end
    local learned = if replaces and hasOld
        then (p4.call(datPersonaReplaceSkill, seams.replaceSkill, personaRecord(member), replaces, skill) :: number) >= 0
        else p4.call(datPersonaAddSkill, seams.addSkill, personaRecord(member), skill) ~= 0
    if learned then return "taught" end
    table.insert(waiting, grant)
    refreshPages()
    return "waiting"
end

local function installGrants()
    if grantWatcher then return end
    seams.getRecord = seam(datPersonaGetRecord)
    seams.addSkill = seam(datPersonaAddSkill)
    seams.replaceSkill = seam(datPersonaReplaceSkill)
    seams.forArcana = seam(cmmCommunityForArcana)
    seams.recordValue = seam(A.cmmCommunityRecordValue)

    -- The player's choice from a page, applied at the next frame boundary.
    type Choice = {grant: Grant, slot: number?} -- slot nil: decline
    local choice: Choice? = nil

    local watcher: P4Subscription
    watcher = p4.events.on("frame_start", function(_e)
        local picked = choice
        if picked then
            choice = nil
            local index = table.find(waiting, picked.grant)
            if index then
                local grant = picked.grant
                local rank = grant.rank
                local slot = picked.slot
                local resolved = slot == nil
                if slot then
                    local skill = grantSkill(grant)
                    local old = p4.mem.read("u16", slotAddress(grant.member, slot))
                    if knows(grant.member, skill) then
                        resolved = true
                    elseif old == 0 then
                        resolved = p4.call(datPersonaAddSkill, seams.addSkill, personaRecord(grant.member), skill) ~= 0
                    else
                        resolved = (p4.call(datPersonaReplaceSkill, seams.replaceSkill, personaRecord(grant.member), old, skill) :: number) >= 0
                    end
                    if resolved then
                        announce(string.format("%s LEARNED %s", MEMBER_NAMES[grant.member], skillName(skill)))
                        p4.log(string.format("skill grant taught member=%d skill=%d replacing=%d", grant.member, skill, old))
                    end
                end
                if resolved then
                    table.remove(waiting, index)
                    if rank then taught[rankKey(grant.member, rank)] = true end
                    if not rank and not slot then taught[grantKey(grant)] = true end
                    refreshPages()
                else
                    announce("SKILL NOT LEARNED: SEE MODS")
                end
            end
        end
        if #readyChecks > 0 and p4.scene.state().ready then
            local checks = readyChecks
            readyChecks = {}
            for _, check in checks do check() end
        end
        if announceUntil ~= 0 and p4.clock.wall() >= announceUntil then
            announceUntil = 0
            setStatus("grant", nil)
        end
        if #readyChecks == 0 and announceUntil == 0 and not choice then watcher:disable() end
    end)
    watcher:disable()
    grantWatcher = watcher
    table.insert(hooks, watcher)

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

    for member, memberName in MEMBER_NAMES do
        replacePages[member] = p4.ui.page({
            key = "replace" .. tostring(member), title = memberName .. ": REPLACE A SKILL", listed = false,
            count = function(): number return if firstWaiting(member) then 9 else 1 end,
            row = function(index: number): P4UIRow
                local grant = firstWaiting(member)
                if not grant then return {key = "none", label = "NOTHING TO LEARN", kind = "info"} end
                local learn = skillName(grantSkill(grant))
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
                wakeGrants()
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
            local rank = grant.rank
            return {key = "m" .. tostring(member), kind = "link", target = replacePages[member].key,
                    label = string.format("%s: %s", MEMBER_NAMES[member], skillName(grantSkill(grant))),
                    help = (if rank then string.format("Social Link rank %d.", rank) else "Bike ride.")
                        .. " All eight skills are taken: choose one to replace."}
        end,
        find = function(key: string): number?
            local member = tonumber(string.match(key, "^m(%d+)$"))
            return if member then table.find(waitingMembers(), member) else nil
        end,
    })

    table.insert(saveParts, {
        snapshot = function(block: SaveBlock)
            local done: {string}, open: {string} = {}, {}
            for key in taught do table.insert(done, key) end
            for _, grant in waiting do table.insert(open, grantKey(grant)) end
            if #done > 0 then
                table.sort(done)
                block.taught = done :: any
            end
            if #open > 0 then block.waiting = open :: any end
        end,
        load = function(value: P4SaveValue, origin: "load" | "reset")
            table.clear(taught)
            table.clear(waiting)
            choice = nil
            table.clear(readyChecks)
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
                    local rider, skill = string.match(key, "^ride:(%d+):(%d+)$")
                    if member and rank then
                        table.insert(waiting, {member = tonumber(member) :: number, rank = tonumber(rank) :: number})
                    elseif rider and skill then
                        table.insert(waiting, {member = tonumber(rider) :: number, skill = tonumber(skill) :: number})
                    end
                end
            end
            if origin == "load" then
                for _, check in afterLoad do whenReady(check) end
            end
        end,
    })
end

--------------------------------------------------------------------------
-- SOCIAL LINK PERSONA SKILLS
--
-- Grants happen when the player next has control of an idle field after the
-- rank-up (so after a MAX rank's awakening), and once after loading a save
-- that already has the ranks. With a free slot the skill is learned and
-- announced; with eight skills it waits (PERSONA SKILL GRANTS above).
--------------------------------------------------------------------------

local cmmScriptRankUp = 0x0024B380 -- CMM_RANKUP: 1 once the rank-up sequence ends

local function installRankSkills()
    installGrants()

    local function isWaiting(member: number, rank: number): boolean
        for _, grant in waiting do
            if grant.member == member and grant.rank == rank then return true end
        end
        return false
    end

    local function teachRank(member: number, rank: number)
        local info = memberInfo(member)
        local skill = info.skills[rank]
        local outcome = teach({member = member, rank = rank}, info.replaces[rank])
        if outcome ~= "waiting" then taught[rankKey(member, rank)] = true end
        if outcome == "taught" then
            announce(string.format("%s LEARNED %s", info.name, skillName(skill)))
            p4.log(string.format("rank skill taught member=%d rank=%d skill=%d", member, rank, skill))
        elseif outcome == "waiting" then
            announce(string.format("%s CAN LEARN %s: SEE MODS", info.name, skillName(skill)))
            p4.log(string.format("rank skill waiting member=%d rank=%d skill=%d", member, rank, skill))
        end
    end

    local function grantReached()
        loadRankSkills()
        for _, info in MEMBERS do
            if hasPersona(info.id) then
                local rank = rankOf(info.id)
                for _, r in info.ranks do
                    if info.skills[r] and rank >= r and not taught[rankKey(info.id, r)] and not isWaiting(info.id, r) then
                        teachRank(info.id, r)
                    end
                end
            end
        end
        refreshPages()
    end
    table.insert(afterLoad, grantReached)

    table.insert(hooks, p4.hook(cmmScriptRankUp, seam(cmmScriptRankUp), "after", function(ctx)
        if ctx.result == 1 then whenReady(grantReached) end
    end))
    p4.log("Golden Social Link Persona skills installed")
end

--------------------------------------------------------------------------
-- BIKE RIDES (decision D13; docs/modding/golden.md)
--
-- Golden's scooter rides teach party Personas skills. Here, from 8/24, the
-- Moel gas station offers one ride a day during the day unless something
-- holds the time (p4.calendar.hold: the retail flags on which the station's
-- own time-spending activities refuse, such as the 12/05 investigation, or the
-- slot's scheduler procedure still running). While
-- the player stands idle near the attendant a HUD row names SELECT, which opens BIKE RIDE in
-- MODS > p4pc.golden. Each recruited member whose Social Link has started
-- (Rise has no ride skills) has Golden's two talks:
--   TALK ABOUT THE FUTURE  the member's next skill in Golden's own
--                          cmpPsSkillAddTable.ctd (init/cmpTable.bin: seven
--                          20-byte rows after a 16-byte header, five u16 skill
--                          ids each, members 2..8 in order); Rise's row is empty
--   SHARE MEMORIES         a skill the member's Persona learned by level (its
--                          learnset in the PERSONA2 party block) and no longer
--                          has, the most recent first
-- The ride ends the time slot through p4.calendar.pass, which refuses again
-- if a hold began meanwhile; only a confirmed pass grants the skill
-- (free slot, or it waits on PERSONA SKILLS). A refused or unsuccessful pass
-- costs nothing.
--------------------------------------------------------------------------

local DATE = 0x00797B7A -- s16 date index (0 = 4/1)
local SLOT = 0x00797B7C -- u8 time slot (0..5)
local RIDES_FROM = 145 -- 8/24
local FIRST_DAY_SLOT, LAST_DAY_SLOT = 3, 4 -- afternoon / after school
-- The attendant approach used by collection0320_attendant_to_tv.walk; its
-- 150-unit approach radius keeps the invitation at the station, not the district.
local GAS_STATION = {field = 8, room = 2, x = -594, z = -813, radius = 150}
local PARTY_ROW = 0x26E -- PERSONA2 party block row: +4 thirty-two learn events of 4 bytes
local PARTY_FIRST = 0xC0
local RIDERS = {2, 3, 4, 6, 7, 8} -- every party member but Rise

local function installBikeRides()
    installGrants()

    local rideSkills: {[number]: {number}} = {}
    local loaded = false
    local function loadRides()
        if loaded then return end
        local file = goldenSource("init/cmpTable.bin")
        local at, size = section(file, "cmpPsSkillAddTable.ctd")
        if size < 16 + 7 * 20 or buffer.readu32(file, at) ~= 7 * 20 or buffer.readu32(file, at + 4) ~= 7 then
            error("p4pc.golden: cmpPsSkillAddTable.ctd has an unexpected layout")
        end
        for row = 0, 6 do
            local list = {}
            for s = 0, 4 do
                local skill = ps2Skill(buffer.readu16(file, at + 16 + row * 20 + s * 2))
                if skill then table.insert(list, skill) end
            end
            rideSkills[row + 2] = list
        end
        loaded = true
        p4.log("Golden ride skills read")
    end

    local ridden = -1 -- date index of the last ride
    local futures: {[number]: number} = {} -- member -> how far its future talks went in its row
    type Talk = {member: number, future: boolean}
    type PendingRide = {talk: Talk, day: number, skill: number?, index: number}
    local talk: Talk? = nil
    local pending: PendingRide? = nil
    local earned: PendingRide? = nil -- a confirmed pass awaiting a ready field
    local promptShown = false
    local offerOpen = false -- SELECT opened the offer; page callbacks cannot call the game
    local lastX: number? = nil
    local lastZ: number? = nil
    local positionX: number? = nil
    local positionZ: number? = nil
    local stationary = false
    local offerX: number? = nil
    local offerZ: number? = nil
    table.insert(saveParts, {
        authorizeSave = function(): (boolean, string)
            if pending then return false, "A bike ride is still passing time." end
            return true, ""
        end,
        snapshot = function(block: SaveBlock)
            local counts = {}
            local any = false
            for member, count in futures do
                counts[tostring(member)] = count
                any = true
            end
            if ridden >= 0 then block.ridden = ridden end
            if any then block.futures = counts end
            local reward = earned
            if reward then
                block.earnedRide = {
                    member = reward.talk.member, future = reward.talk.future,
                    day = reward.day, skill = reward.skill, index = reward.index,
                }
            end
        end,
        load = function(value: P4SaveValue, _origin: "load" | "reset")
            ridden = -1
            table.clear(futures)
            talk = nil
            pending = nil
            earned = nil
            promptShown = false
            offerOpen = false
            lastX, lastZ, positionX, positionZ = nil, nil, nil, nil
            stationary = false
            offerX, offerZ = nil, nil
            setStatus("ride", nil)
            if type(value) ~= "table" then return end
            local day = value.ridden
            if type(day) == "number" then ridden = day end
            local counts = value.futures
            if type(counts) == "table" then
                for member, count in counts do
                    local id = tonumber(member)
                    if id and type(count) == "number" then futures[id] = count end
                end
            end
            local reward = value.earnedRide
            if type(reward) == "table" then
                local member, future, day = reward.member, reward.future, reward.day
                local skill, index = reward.skill, reward.index
                if type(member) == "number" and table.find(RIDERS, member)
                    and type(future) == "boolean" and type(day) == "number"
                    and (skill == nil or type(skill) == "number") and type(index) == "number" then
                    earned = {talk = {member = member, future = future}, day = day, skill = skill, index = index}
                end
            end
        end,
    })

    -- The member's next row skill its Persona neither knows nor has waiting,
    -- and its place in the row.
    local function nextFuture(member: number): (number?, number)
        local list = rideSkills[member] or {}
        for index = (futures[member] or 0) + 1, #list do
            local skill = list[index]
            if not taught[string.format("ride:%d:%d", member, skill)]
                and not knows(member, skill) and not waitingFor(member, skill) then return skill, index end
        end
        return nil, 0
    end
    -- A false second result means the learnset cannot be inspected; do not
    -- confuse that with a valid learnset containing no forgotten skill.
    local function memory(member: number): (number?, boolean)
        local persona = p4.mem.read("u16", PARTY_PERSONA + member * 0x88 + 2)
        local level = p4.mem.read("u8", PARTY_PERSONA + member * 0x88 + 4)
        local row = persona - PARTY_FIRST
        if row < 0 or not p4.tables.bank("PERSONA2").bytes then return nil, false end
        local party = p4.tables.stock("PERSONA2")
        if (row + 1) * PARTY_ROW > buffer.len(party) then return nil, false end
        local best, bestLevel = nil, -1
        for e = 0, 31 do
            local at = row * PARTY_ROW + 4 + e * 4
            local learnLevel, kind, skill = buffer.readu8(party, at), buffer.readi8(party, at + 1), buffer.readu16(party, at + 2)
            if kind == 1 and skill ~= 0 and learnLevel <= level and learnLevel > bestLevel
                and not taught[string.format("ride:%d:%d", member, skill)]
                and not knows(member, skill) and not waitingFor(member, skill) then
                best, bestLevel = skill, learnLevel
            end
        end
        return best, true
    end
    local function offered(): boolean
        local s = p4.scene.state()
        local today = p4.mem.read("i16", DATE)
        local slot = p4.mem.read("u8", SLOT)
        if not (s.ready and s.context == "field" and s.field == GAS_STATION.field and s.room == GAS_STATION.room
            and today >= RIDES_FROM and today ~= ridden and slot >= FIRST_DAY_SLOT and slot <= LAST_DAY_SLOT
            and p4.calendar.hold() == nil) then return false end
        local x, z = positionX, positionZ
        if not stationary or not x or not z then return false end
        local dx, dz = x - GAS_STATION.x, z - GAS_STATION.z
        return dx * dx + dz * dz <= GAS_STATION.radius * GAS_STATION.radius
    end

    local riders: {number} = {}
    local ridePage: P4UIPage

    -- Queue the pass first. A queued request can still be refused or fail, so
    -- neither the daily limit nor a grant is committed until it really passed.
    local function ride(chosen: Talk)
        local member, name = chosen.member, MEMBER_NAMES[chosen.member]
        loadRides()
        local skill: number?, index = nil, 0
        if chosen.future then
            skill, index = nextFuture(member)
        else
            local remembered, available = memory(member)
            skill = remembered
            if not available then
                announce("NO RIDE: PERSONA LEARNSET UNAVAILABLE")
                return
            end
        end
        local ok, why = p4.calendar.pass()
        p4.log(string.format("ride time pass member=%d ok=%s why=%s", member, tostring(ok), tostring(why)))
        if not ok then
            announce("NO RIDE NOW: " .. string.upper(tostring(why)))
            return
        end
        pending = {talk = chosen, day = p4.mem.read("i16", DATE), skill = skill, index = index}
    end

    local function finishRide(ride: PendingRide)
        local member, skill, index = ride.talk.member, ride.skill, ride.index
        local name = MEMBER_NAMES[member]
        ridden = ride.day
        local kind = if ride.talk.future then "future" else "memories"
        if not skill then
            announce(name .. ": THE TALK WENT ANOTHER WAY")
            p4.log(string.format("ride skill none member=%d talk=%s", member, kind))
            return
        end
        if ride.talk.future then futures[member] = index end
        local outcome = teach({member = member, skill = skill})
        if outcome == "waiting" then
            announce(string.format("%s CAN LEARN %s: SEE MODS", name, skillName(skill)))
        elseif ride.talk.future then
            announce(string.format("%s: SOMETHING NEW, %s", name, skillName(skill)))
        else
            announce(string.format("%s: IT CAME BACK, %s", name, skillName(skill)))
        end
        p4.log(string.format("ride skill %s member=%d skill=%d talk=%s", outcome, member, skill, kind))
    end

    local frames, openedAt = 0, 0
    table.insert(hooks, p4.events.on("frame_start", function(_e)
        -- Two consecutive ready-frame positions must agree. An unavailable
        -- position or a menu/transition invalidates the movement sample.
        local x, z = p4.scene.player_position()
        stationary = x ~= nil and z ~= nil and x == lastX and z == lastZ
        lastX, lastZ, positionX, positionZ = x, z, x, z
        local reward = earned
        if reward then
            if p4.scene.state().ready then
                finishRide(reward)
                earned = nil
            end
            return
        end
        local chosen = talk
        if chosen and not pending and p4.scene.state().ready then
            if positionX ~= offerX or positionZ ~= offerZ then
                talk = nil
                offerOpen = false
                promptShown = false
                setStatus("ride", nil)
                return
            end
            if not stationary then return end
            talk = nil
            offerOpen = false
            promptShown = false
            setStatus("ride", nil)
            if offered() then ride(chosen) end
            return
        end
        if pending then
            local state, detail = p4.scene.status()
            if state == "done" or state == "refused" then
                local ride = pending
                pending = nil
                p4.log("ride pass " .. state .. " " .. detail)
                if state == "done" and string.sub(detail, 1, 6) == "passed" then
                    earned = ride
                else
                    announce("NO RIDE: " .. string.upper(detail))
                end
            end
            return
        end
        -- Update while idle; keep the answer while the ride menu is open.
        -- After closing it without a talk, SELECT can open a new offer.
        frames += 1
        if offerOpen and frames - openedAt >= 60 and p4.ui.active() == nil and p4.scene.state().ready then
            offerOpen = false
        end
        if p4.scene.state().ready then
            local show = offered()
            if not show then offerOpen = false end
            if show ~= promptShown then
                promptShown = show
                setStatus("ride", if show then "MOEL: PRESS SELECT FOR A BIKE RIDE" else nil)
            end
        elseif promptShown and not offerOpen then
            promptShown = false
            setStatus("ride", nil)
        end
        if promptShown and not offerOpen then
            local _down, pressed = p4.input.action("select")
            if pressed and offered() then
                table.clear(riders)
                for _, member in RIDERS do
                    if hasPersona(member) and rankOf(member) > 0 then table.insert(riders, member) end
                end
                offerOpen, openedAt = true, frames
                offerX, offerZ = positionX, positionZ
                ridePage:refresh()
                ridePage:open()
                p4.log(string.format("ride offer opened riders=%d", #riders))
            end
        end
    end))

    local memberPages: {[number]: P4UIPage} = {}
    for _, member in RIDERS do
        memberPages[member] = p4.ui.page({
            key = "ride" .. tostring(member), title = "RIDE WITH " .. MEMBER_NAMES[member], listed = false,
            count = function(): number return 2 end,
            row = function(index: number): P4UIRow
                if index == 1 then
                    return {key = "future", label = "TALK ABOUT THE FUTURE", kind = "action",
                            help = "Their Persona may learn something new. The ride ends this time of day."}
                end
                return {key = "memories", label = "SHARE MEMORIES", kind = "action",
                        help = "Their Persona may remember a skill it once had. The ride ends this time of day."}
            end,
            find = function(key: string): number?
                return if key == "future" then 1 elseif key == "memories" then 2 else nil
            end,
            adjust = function(key: string, direction: number)
                if direction ~= 0 or talk or pending or not offerOpen or not table.find(riders, member) then return end
                talk = {member = member, future = key == "future"}
                p4.ui.close()
            end,
        })
    end
    ridePage = p4.ui.page({
        key = "ride", title = "BIKE RIDE",
        count = function(): number return if offerOpen then math.max(#riders, 1) else 1 end,
        row = function(index: number): P4UIRow
            if not offerOpen then
                return {key = "none", label = "RIDES START AT THE MOEL GAS STATION", kind = "info",
                        help = "From 8/24, once a day during the day: press SELECT near the Moel attendant."}
            end
            local member = riders[index]
            if not member then return {key = "none", label = "NO ONE TO RIDE WITH YET", kind = "info"} end
            return {key = "m" .. tostring(member), label = MEMBER_NAMES[member], kind = "link",
                    target = memberPages[member].key}
        end,
        find = function(key: string): number?
            if not offerOpen then return if key == "none" then 1 else nil end
            local member = tonumber(string.match(key, "^m(%d+)$"))
            return if member then table.find(riders, member) else nil
        end,
    })
    p4.log("Golden bike rides installed")
end

if p4.settings.get("perk_order") == true then
    installPerkOrder()
end
if p4.settings.get("rank_skills") == true then
    installRankSkills()
end
if p4.settings.get("bike_rides") == true then
    installBikeRides()
end
registerSave()

-- The groups this run actually applies, for packages that name p4pc.golden
-- as a companion or requirement (p4.contract.read("p4pc.golden", "groups"),
-- version 1): the producer applies party only with rank_skills and
-- encounters only with enemies, and so does this table.
do
    local function on(key: string): boolean return p4.settings.get(key) == true end
    p4.contract.publish("groups", 1, {
        numbers = on("numbers"), enemies = on("enemies"), party = on("party") and on("rank_skills"),
        exp_curves = on("exp_curves"), encounters = on("encounters") and on("enemies"),
        perk_order = on("perk_order"), rank_skills = on("rank_skills"), bike_rides = on("bike_rides"),
    })
end
