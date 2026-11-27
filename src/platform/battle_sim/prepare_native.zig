const std=@import("std");
pub const panic = @import("panic.zig").panic;
const c = @import("c_bindings");
const View=extern struct { header:[*]const u8, source:[*]const u8, actors:[*]const u8, personas:[*]const u8, items:[*]const u8, decisions:[*]const u8, source_bytes:u32, actor_count:u32, persona_count:u32, item_count:u32, decision_count:u32 };
fn word(p:[*]const u8) u32 {return std.mem.readInt(u32,p[0..4],.little);}
const P=[*]u8;
const CP=[*]const u8;
const Reason=?[*:0]const u8;
extern var p4_guest_base:P;
extern fn p4_g2h_slow(u32) ?*anyopaque;
extern fn p4_h2g_slow(?*const anyopaque) u32;
extern fn p4_gport_alloc(u32,u32) u32;
extern fn p4_game_table_extent_rows(u32,u32,*u32,*u32) c_int;
extern fn p4_battle_prepare_persona_catalogued(u32) c_int;
extern fn p4_content_digest(*const anyopaque,usize,[*]u8) void;
extern fn datUnitGetRecord(i16) ?*anyopaque;
extern fn datCalcInitUnitById(i16) void;
extern fn datCalcGetLevel(?*anyopaque) i32;
extern fn datCalcGetHp(?*anyopaque) u16;
extern fn datCalcGetSp(?*anyopaque) u16;
extern fn datCalcGetBadStatus(?*anyopaque) u32;
extern fn datCalcSetBadStatus(?*anyopaque,u32) u32;
extern fn datCalcClearBadStatus(?*anyopaque,u32) u32;
extern fn datCalcClearAllBuffs(?*anyopaque) void;
extern fn datCalcIsDead(?*anyopaque,i32) u32;
extern fn datCalcMaxHpById(i16) u16;
extern fn datCalcMaxSpById(i16) u16;
extern fn datSetUnitHp(i16,i16) void;
extern fn datSetUnitSp(i16,i16) void;
extern fn datSetUnitAiTactic(i16,i16) void;
extern fn datSetHeroLevel(i16,u8) void;
extern fn datSetHeroExp(i16,u32) void;
extern fn datLevelFromExp(i32) u8;
extern fn datSetPartyId(i32,i16) void;
extern fn datBuildMemberIdListExcluding5(*anyopaque) i16;
extern fn datPersonaGetRecord(u16) ?*anyopaque;
extern fn datPersonaRosterSlot(i16) ?*anyopaque;
extern fn datPersonaRosterCapacity() i32;
extern fn datPersonaSetRosterMode(i32) void;
extern fn datPersonaClearRoster() void;
extern fn datPersonaInsertRosterRecord(*anyopaque) i32;
extern fn datPersonaSetLeadSlot(i32) i32;
extern fn datPersonaInitRecord(*anyopaque,u16) void;
extern fn datPersonaPreviewLevelUp(*anyopaque,*anyopaque,i32) void;
extern fn datPersonaApplyLevelUp(*anyopaque,*anyopaque) void;
extern fn datPersonaGetExpForLevel(*anyopaque,u16) u32;
extern fn datPersonaAddExp(*anyopaque,i32) void;
extern fn datPersonaAddSkill(*anyopaque,u16) i32;
extern fn datPersonaReplaceSkill(*anyopaque,i16,u16) i32;
extern fn datGetEquippedItemByUnit(i16,i16) i16;
extern fn datAssignEquippedItem(i16,i16,i16) void;
extern fn datItemCount(i16) i32;
extern fn datItemSetCount(i16,i32) void;
extern fn datEntryGetKind(i16) u32;
extern fn datEntryHasFlag(i16,i16) i32;
extern fn datIdBand(i16) i32;
fn g2h(a: u32) P {
    if (@sizeOf(usize) == 4) return @ptrFromInt(a);
    if (a -% 1 < 0x3ffffff) return p4_guest_base + a;
    return @ptrCast(p4_g2h_slow(a).?);
}
fn h2g(p: ?*const anyopaque) u32 {
    if (@sizeOf(usize) == 4) return @truncate(@intFromPtr(p));
    const d = @intFromPtr(p) -% @intFromPtr(p4_guest_base);
    if (d -% 1 < 0x3ffffff) return @truncate(d);
    return p4_h2g_slow(p);
}
fn g16(a: u32) u16 {
    const p: *volatile align(1) u16 = @ptrCast(g2h(a));
    return p.*;
}
fn g32(a: u32) u32 {
    const p: *volatile align(1) u32 = @ptrCast(g2h(a));
    return p.*;
}
fn g8(a: u32) u8 {
    const p: *volatile u8 = @ptrCast(g2h(a));
    return p.*;
}
fn half(p:CP) u16 {return std.mem.readInt(u16,p[0..2],.little);}
fn putHalf(p:P,v:u16) void {std.mem.writeInt(u16,p[0..2],v,.little);}
fn putWord(p:P,v:u32) void {std.mem.writeInt(u32,p[0..4],v,.little);}
fn ptr(p:P) *anyopaque {return @ptrCast(p);}
fn int16(v:u32) i16 {return @bitCast(@as(u16,@truncate(v)));}
fn assertions(q: CP, r: CP) bool {
    const m = word(q + 88);
    return (m & 1 == 0 or (r[4] == word(q + 20) and word(r + 8) == word(q + 24))) and
        (m & 2 == 0 or std.mem.eql(u8, r[12..28], q[48..64])) and
        (m & 4 == 0 or std.mem.eql(u8, r[28..33], q[64..69])) and
        (m & 8 == 0 or std.mem.eql(u8, r[33..43], q[69..79]));
}
fn grow(view: *const View, q: CP, r: P, delta: P) Reason {
    const level = word(q + 20);
    const exp = word(q + 24);
    const first = word(q + 80);
    const count = word(q + 84);
    var index: u32 = 0;
    if (r[4] > level or word(r + 8) > exp) return "native_growth_cannot_reduce_level_or_exp";
    while (r[4] < level) {
        const next = datPersonaGetExpForLevel(ptr(r), @as(u16, r[4]) + 1);
        const current = word(r + 8);
        if (next < current or next > exp or next - current > 0x7fffffff)
            return "native_growth_level_exp_inconsistent";
        const added: i32 = @intCast(next - current);
        datPersonaPreviewLevelUp(ptr(r), ptr(delta), added);
        if (delta[0] != 1) return "native_growth_did_not_reach_exact_next_level";
        var learned: [32]u16 = undefined;
        for (0..32) |i| learned[i] = half(delta + 2 + i * 2);
        @memset(delta[2..66], 0);
        datPersonaApplyLevelUp(ptr(r), ptr(delta));
        datPersonaAddExp(ptr(r), added);
        for (learned) |skill| {
            if (skill == 0) continue;
            if (index == count) return "native_growth_missing_skill_decision";
            const d = view.decisions + (@as(usize, first) + index) * 24;
            index += 1;
            const replace = word(d + 16);
            const removed = word(d + 20);
            if (word(d + 8) != r[4] or word(d + 12) != skill)
                return "native_growth_skill_decision_does_not_match_learn_event";
            if (replace == 0xffffffff) {
                if (datPersonaAddSkill(ptr(r), skill) == 0)
                    return "native_growth_requires_explicit_skill_replacement";
            } else {
                if (half(r + 12 + replace * 2) != removed) return "native_growth_stale_replacement_slot";
                if (removed == 0) {
                    var empty: u32 = 0;
                    while (empty < 8 and half(r + 12 + empty * 2) != 0) : (empty += 1) {}
                    if (empty != replace or datPersonaAddSkill(ptr(r), skill) == 0)
                        return "native_growth_empty_replacement_not_native_auto_add";
                } else if (datPersonaReplaceSkill(ptr(r), int16(removed), skill) != @as(i32, @intCast(replace))) {
                    return "native_growth_native_replacement_failed";
                }
            }
        }
    }
    if (index != count) return "native_growth_extra_skill_decisions";
    if (level < 99 and datPersonaGetExpForLevel(ptr(r), @intCast(level + 1)) <= exp)
        return "native_growth_final_exp_exceeds_declared_level";
    const remaining = exp -% word(r + 8);
    if (remaining > 0x7fffffff) return "native_growth_exp_increment_outside_native_range";
    datPersonaAddExp(ptr(r), @intCast(remaining));
    return if (assertions(q, r)) null else "native_growth_final_record_assertion";
}
const Gear=struct {actor:i16,slot:i16,old:i16,new:i16,done:bool};
export fn p4_battle_prepare_apply(view:*const View) Reason {
    const header = view.header;
    const synthetic = word(header + 36) == 2;
    var metadata: u32 = undefined;
    var metadata_rows: u32 = undefined;
    var names: u32 = undefined;
    var name_rows: u32 = undefined;
    if (p4_game_table_extent_rows(0x7644c4, 14, &metadata, &metadata_rows) == 0 or
        p4_game_table_extent_rows(0x764538, 19, &names, &name_rows) == 0)
        return "preparation_requires_observed_original_persona_and_skill_names";
    const item_base = g32(0x7642a0);
    if (item_base < 16 or item_base - 16 < 0x100000 or item_base - 16 > 0x2000000 or
        16 + 1536 * 68 > 0x2000000 - (item_base - 16) or g32(item_base - 16) != 1536)
        return "preparation_requires_original_item_rows_and_native_stock_capacity";
    const roster_mode = word(header + 76);
    const selected = word(header + 80);
    const old_capacity = datPersonaRosterCapacity();
    const capacity: u32 = if (roster_mode == 0xffffffff) @bitCast(old_capacity) else 6 + roster_mode * 2;
    const lead: u32 = if (selected == 0xffffffff) g16(0x797f88) else selected;
    if (!synthetic and capacity > @as(u32, @bitCast(old_capacity)))
        return "source_attested_cannot_grant_roster_capacity";
    if (lead >= capacity) return "prepared_selected_roster_slot_outside_capacity";
    const scratch = p4_gport_alloc(view.persona_count * 48 + 0x88 + 16, 16);
    if (scratch == 0) return "preparation_native_scratch_allocation";
    const records = g2h(scratch);
    const delta = records + view.persona_count * 48;
    const members = scratch + view.persona_count * 48 + 0x88;
    const member_count = datBuildMemberIdListExcluding5(ptr(g2h(members)));
    if (member_count < 1 or member_count > 8) return "native_unlocked_member_list_invalid";
    var stock: [1536]u8 = undefined;
    @memcpy(&stock, g2h(0x79757a)[0..1536]);
    var gear: [12]Gear = undefined;
    var gear_count: usize = 0;
    for (0..view.actor_count) |i| {
        const actor = view.actors + i * 64;
        const id = int16(word(actor));
        var unlocked = false;
        for (0..@as(usize, @intCast(member_count))) |j| {
            if (@as(i16, @bitCast(g16(members + @as(u32, @intCast(j * 2))))) == id) unlocked = true;
        }
        if (!synthetic and !unlocked) return "source_attested_actor_not_unlocked";
        for (0..3) |s| {
            const slot: i16 = @intCast(s);
            const desired = word(actor + 4 + s * 4);
            const old = datGetEquippedItemByUnit(id, slot);
            if (desired == 0xffffffff or desired == @as(u16, @bitCast(old))) continue;
            if (desired == 0 or desired >= 0x300 or datIdBand(int16(desired)) != slot or
                datEntryHasFlag(int16(desired), id) == 0)
                return "prepared_equipment_not_native_actor_eligible";
            if (old < 0 or old >= 1536) return "source_equipment_outside_native_stock";
            gear[gear_count] = .{ .actor = id, .slot = slot, .old = old, .new = int16(desired), .done = false };
            gear_count += 1;
        }
    }
    var order: [12]usize = undefined;
    var planned: usize = 0;
    while (planned < gear_count) {
        const before = planned;
        for (0..gear_count) |i| {
            const change = &gear[i];
            const ni: usize = @intCast(change.new);
            const oi: usize = @intCast(change.old);
            if (change.done or (stock[ni] == 0 and !synthetic)) continue;
            if (stock[ni] == 0) stock[ni] = 1;
            stock[ni] = @intCast(@min(@as(u32, stock[ni]) - 1, 99));
            if (change.old != 0) stock[oi] = @intCast(@min((@as(u32, stock[oi]) + 1) & 255, 99));
            change.done = true;
            order[planned] = i;
            planned += 1;
        }
        if (before == planned) return "source_attested_equipment_swaps_have_no_native_stock_feasible_order";
    }
    for (0..view.item_count) |i| {
        const item = view.items + i * 8;
        const id = int16(word(item));
        if (datEntryGetKind(id) != 3) return "declared_inventory_item_not_native_battle_consumable";
        if (!synthetic and word(item + 4) > @as(u32, @bitCast(datItemCount(id))))
            return "source_attested_cannot_grant_item_stock";
    }
    var hero_slots: u32 = 0;
    var hero_count: u32 = 0;
    for (0..view.persona_count) |i| {
        const q = view.personas + i * 112;
        const record = records + i * 48;
        const owner = word(q);
        const slot = word(q + 4);
        const mode = word(q + 12);
        const origin = word(q + 28);
        if (owner == 1) {
            if (slot >= capacity) return "prepared_hero_persona_outside_roster_capacity";
            hero_slots |= @as(u32, 1) << @intCast(slot);
            hero_count += 1;
        }
        if (origin == 0) {
            const source_slot = word(q + 16);
            if (owner == 1 and source_slot >= @as(u32, @bitCast(old_capacity)))
                return "source_persona_slot_outside_original_capacity";
            const source = if (owner == 1) datPersonaRosterSlot(int16(source_slot)) else datPersonaGetRecord(@intCast(owner));
            if (source == null) return "source_persona_record_missing";
            const src: CP = @ptrCast(source.?);
            @memcpy(record[0..48], src[0..48]);
            if (word(q + 8) != 0xffffffff and half(record + 2) != word(q + 8))
                return "source_persona_id_assertion";
        } else {
            const id = word(q + 8);
            if (p4_battle_prepare_persona_catalogued(id) == 0)
                return "prepared_persona_not_populated_native_metadata";
            @memset(record[0..48], 0);
            datPersonaInitRecord(ptr(record), @intCast(id));
            putHalf(record, 1);
        }
        if (half(record + 2) == 0 or half(record + 2) >= metadata_rows or record[4] == 0 or record[4] > 99)
            return "prepared_persona_record_not_native_valid";
        const mask = word(q + 88);
        if (mask & 16 != 0) @memcpy(record[12..28], q[32..48]);
        if (mode == 1) {
            const level = word(q + 20);
            const exp = word(q + 24);
            if (exp < datPersonaGetExpForLevel(ptr(record), @intCast(level)) or
                (level < 99 and exp >= datPersonaGetExpForLevel(ptr(record), @intCast(level + 1))))
                return "synthetic_persona_level_exp_inconsistent";
            record[4] = @intCast(level);
            putWord(record + 8, exp);
            @memcpy(record[12..28], q[48..64]);
            @memcpy(record[28..33], q[64..69]);
        } else if (mode == 2) {
            if (!synthetic and (record[4] != word(q + 20) or word(record + 8) != word(q + 24)))
                return "source_attested_cannot_grant_persona_growth";
            if (grow(view, q, record, delta)) |reason| return reason;
        }
        if (!assertions(q, record)) return "prepared_persona_record_assertion";
        for (0..8) |j| {
            const skill = half(record + 12 + j * 2);
            if (skill != 0 and (skill >= name_rows or g8(names +% (@as(u32, skill) *% 19)) == 0))
                return "prepared_skill_not_in_populated_native_name_catalog";
        }
    }
    if (hero_count != 0) {
        if (hero_slots != (@as(u32, 1) << @intCast(hero_count)) - 1)
            return "declared_hero_roster_must_be_complete_contiguous_slots";
        if (hero_slots & (@as(u32, 1) << @intCast(lead)) == 0)
            return "prepared_selected_roster_slot_missing";
    } else if (lead >= @as(u32, @bitCast(old_capacity)) or datPersonaRosterSlot(int16(lead)) == null) {
        return "preserved_selected_roster_slot_missing";
    }
    const hero_mode = word(header + 84);
    const hero_level = word(header + 68);
    const hero_exp = word(header + 72);
    if (hero_mode == 1 and datLevelFromExp(@intCast(hero_exp)) != hero_level)
        return "synthetic_hero_level_exp_inconsistent";
    if (hero_mode == 2 and (datCalcGetLevel(datUnitGetRecord(1)) != @as(i32, @intCast(hero_level)) or
        g32(0x797400) != hero_exp)) return "source_hero_level_exp_assertion";
    // Validate staged records and stock before publishing changes.
    if (roster_mode != 0xffffffff) datPersonaSetRosterMode(@intCast(roster_mode));
    if (hero_count != 0) {
        datPersonaClearRoster();
        for (0..hero_count) |slot| {
            for (0..view.persona_count) |i| {
                const q = view.personas + i * 112;
                if (word(q) != 1 or word(q + 4) != slot) continue;
                const record = records + i * 48;
                if (datPersonaInsertRosterRecord(ptr(record)) == 0) return "native_roster_insert_failed";
                const actual = datPersonaRosterSlot(@intCast(slot)) orelse return "native_inserted_roster_slot_missing";
                if (word(q + 12) == 0) {
                    const dst: P = @ptrCast(actual);
                    @memcpy(dst[0..48], record[0..48]);
                }
            }
        }
    }
    for (0..view.persona_count) |i| {
        const owner = word(view.personas + i * 112);
        if (owner != 1) {
            const dst: P = @ptrCast(datPersonaGetRecord(@intCast(owner)).?);
            @memcpy(dst[0..48], (records + i * 48)[0..48]);
        }
    }
    if (datPersonaSetLeadSlot(@intCast(lead)) == 0) return "native_lead_slot_selection_failed";
    if (hero_mode == 1) {
        datSetHeroExp(1, hero_exp);
        datSetHeroLevel(1, @intCast(hero_level));
    }
    for (0..gear_count) |i| {
        const change = gear[order[i]];
        var available = datItemCount(change.new);
        if (available == 0) {
            if (!synthetic) return "native_source_stock_plan_diverged";
            datItemSetCount(change.new, 1);
            available = 1;
        }
        datItemSetCount(change.new, available - 1);
        if (change.old != 0) datItemSetCount(change.old, datItemCount(change.old) + 1);
        datAssignEquippedItem(change.actor, change.slot, change.new);
    }
    if (word(header + 112) != 0) {
        for (0..1536) |i| {
            const id: i16 = @intCast(i);
            if (datEntryGetKind(id) == 3 and datItemCount(id) != 0) datItemSetCount(id, 0);
        }
        for (0..view.item_count) |i| {
            const item = view.items + i * 8;
            datItemSetCount(int16(word(item)), @intCast(word(item + 4)));
        }
    }
    for (0..3) |i|
        datSetPartyId(@intCast(i), if (i + 1 < view.actor_count) int16(word(view.actors + (i + 1) * 64)) else 0);
 for (0..view.actor_count) |i| {
  const actor = view.actors + i * 64;
  const id = int16(word(actor));
  const unit = datUnitGetRecord(id);
  if (synthetic and g16(h2g(unit) +% 2) != @as(u16, @bitCast(id))) datCalcInitUnitById(id);
  if (word(actor + 28) != 0xffffffff) datSetUnitAiTactic(id, int16(word(actor + 28)));
  if (word(header + 44) != 0) {
   _ = datCalcClearBadStatus(unit, 0xfff7ffff);
   datCalcClearAllBuffs(unit);
  }
  if (word(actor + 16) != 0) {
   const hp: u16 = @intCast(word(actor + 20));
   datSetUnitHp(id, @bitCast(hp));
   datSetUnitSp(id, int16(word(actor + 24)));
   if (hp != 0) {
    _ = datCalcClearBadStatus(unit, 0x80000);
   } else {
    _ = datCalcSetBadStatus(unit, 0x80000);
   }
  } else if (word(header + 40) != 0 and datCalcIsDead(unit, 0) == 0) {
   datSetUnitHp(id, @bitCast(datCalcMaxHpById(id)));
   datSetUnitSp(id, @bitCast(datCalcMaxSpById(id)));
  }
 }
 return null;
}

fn personaReport(stream: *c.FILE, record: CP) void {
    _ = c.fprintf(stream, "{\"id\":%u,\"flags\":%u,\"level\":%u,\"exp\":%u,\"skills\":[", @as(c_uint,half(record+2)), @as(c_uint,half(record)), @as(c_uint,record[4]), word(record+8));
    for (0..8) |i| _ = c.fprintf(stream, "%s%u", if(i!=0) @as([*:0]const u8,",") else "", @as(c_uint,half(record+12+i*2)));
    _ = c.fputs("],\"stats\":[",stream);
    for (0..5) |i| _ = c.fprintf(stream, "%s%u", if(i!=0) @as([*:0]const u8,",") else "", @as(c_uint,record[28+i]));
    _ = c.fputs("],\"bonus_a\":[",stream);
    for (0..5) |i| _ = c.fprintf(stream, "%s%d", if(i!=0) @as([*:0]const u8,",") else "", @as(c_int,@as(i8,@bitCast(record[33+i]))));
    _ = c.fputs("],\"bonus_b\":[",stream);
    for (0..5) |i| _ = c.fprintf(stream, "%s%d", if(i!=0) @as([*:0]const u8,",") else "", @as(c_int,@as(i8,@bitCast(record[38+i]))));
    _ = c.fputs("]}",stream);
}
export fn p4_battle_prepare_report(stream: *c.FILE, view: *const View) void {
    var hash: [65]u8 = undefined;
    p4_content_digest(@ptrCast(g2h(0x79757a)), 1536, &hash);
    _ = c.fprintf(stream, "\"native_prepared_state\":{\"hero_level\":%d,\"hero_exp\":%u,\"roster_capacity\":%d,\"selected_slot\":%u,\"stock_sha256\":\"%s\",\"actors\":[",
        datCalcGetLevel(datUnitGetRecord(1)), g32(0x797400), datPersonaRosterCapacity(),
        @as(c_uint, g16(0x797f88)), @as([*:0]const u8, @ptrCast(&hash)));
    for (0..view.actor_count) |i| {
        const id = int16(word(view.actors + i * 64));
        const unit = datUnitGetRecord(id);
        _ = c.fprintf(stream, "%s{\"id\":%d,\"level\":%d,\"hp\":%u,\"sp\":%u,\"max_hp\":%u,\"max_sp\":%u,\"status\":%u,\"equipment\":[%d,%d,%d]}",
            if (i != 0) @as([*:0]const u8, ",") else "", @as(c_int, id),
            datCalcGetLevel(unit), @as(c_uint, datCalcGetHp(unit)), @as(c_uint, datCalcGetSp(unit)),
            @as(c_uint, datCalcMaxHpById(id)), @as(c_uint, datCalcMaxSpById(id)),
            datCalcGetBadStatus(unit), @as(c_int, datGetEquippedItemByUnit(id, 0)),
            @as(c_int, datGetEquippedItemByUnit(id, 1)), @as(c_int, datGetEquippedItemByUnit(id, 2)));
    }
    _ = c.fputs("],\"hero_personas\":[", stream);
    var emitted = false;
    const capacity = datPersonaRosterCapacity();
    var slot: i16 = 0;
    while (slot < capacity) : (slot += 1) {
        const raw = datPersonaRosterSlot(slot) orelse continue;
        const record: CP = @ptrCast(raw);
        _ = c.fprintf(stream, "%s{\"slot\":%d,\"record\":", if (emitted) @as([*:0]const u8, ",") else "", @as(c_int, slot));
        emitted = true;
        personaReport(stream, record);
        _ = c.fputc('}', stream);
    }
    _ = c.fputs("],\"companion_personas\":[", stream);
    emitted = false;
    for (2..11) |owner| {
        const raw = datPersonaGetRecord(@intCast(owner)) orelse continue;
        const record: CP = @ptrCast(raw);
        if (half(record + 2) == 0) continue;
        _ = c.fprintf(stream, "%s{\"owner\":%u,\"record\":", if (emitted) @as([*:0]const u8, ",") else "", @as(c_uint, @intCast(owner)));
        emitted = true;
        personaReport(stream, record);
        _ = c.fputc('}', stream);
    }
    _ = c.fputs("],\"declared_inventory\":[", stream);
    for (0..view.item_count) |i| {
        const item = int16(word(view.items + i * 8));
        _ = c.fprintf(stream, "%s{\"id\":%d,\"count\":%d}", if (i != 0) @as([*:0]const u8, ",") else "", @as(c_int, item), datItemCount(item));
    }
    _ = c.fputs("],\"inventory\":[", stream);
    emitted = false;
    for (0..1536) |index| {
        const item: i16 = @intCast(index);
        const count = datItemCount(item);
        if (count <= 0) continue;
        _ = c.fprintf(stream, "%s{\"id\":%d,\"count\":%d}", if (emitted) @as([*:0]const u8, ",") else "", @as(c_int, item), count);
        emitted = true;
    }
    _ = c.fputs("]}", stream);
}
