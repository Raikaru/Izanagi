const std=@import("std");
pub const panic = @import("panic.zig").panic;
const Preparation=extern struct { header:[*]const u8, source:[*]const u8, actors:[*]const u8, personas:[*]const u8, items:[*]const u8, decisions:[*]const u8, source_bytes:u32, actor_count:u32, persona_count:u32, item_count:u32, decision_count:u32 };
extern fn p4_battle_save_valid(?[*]const u8,u32) c_int;
extern fn p4_game_table_extent_rows(u32,u32,*u32,*u32) c_int;
extern var p4_guest_base:[*]u8;
extern fn p4_g2h_slow(u32) ?*anyopaque;
fn guest(a: u32) [*]u8 {
    if (@sizeOf(usize) == 4) return @ptrFromInt(a);
    if (a -% 1 < 0x3ffffff) return p4_guest_base + a;
    return @ptrCast(p4_g2h_slow(a).?);
}
fn word(p: [*]const u8) u32 { return std.mem.readInt(u32, p[0..4], .little); }
fn half(p: [*]const u8) u16 { return std.mem.readInt(u16, p[0..2], .little); }
fn zero(p: [*]const u8, n: usize) bool {
    for (p[0..n]) |b| if (b != 0) { return false; };
    return true;
}
fn skills(p: [*]const u8) bool {
    for (0..8) |i| {
        const s = half(p + i * 2);
        if (s > 4095) return false;
        if (s != 0) {
            for (0..i) |j| if (half(p + j * 2) == s) { return false; };
        }
    }
    return true;
}
fn fail(reason: ?*?[*:0]const u8, msg: [*:0]const u8) c_int {
    if (reason) |r| r.* = msg;
    return 0;
}
export fn p4_battle_prepare_decode(bytes:?[*]const u8,size:usize,view:?*Preparation,reason:?*?[*:0]const u8) c_int {
    if (reason) |r| r.* = null;
    const p = bytes orelse return fail(reason, "preparation_request_size");
    const out = view orelse return fail(reason, "preparation_request_size");
    if (size < 160 or size > 8 * 1024 * 1024) return fail(reason, "preparation_request_size");
    if (!std.mem.eql(u8, p[0..8], "P4BPRE01") or word(p + 8) != 160 or word(p + 12) != 0 or !zero(p + 116, 12))
        return fail(reason, "preparation_header_or_reserved_fields");
    const source_bytes = word(p + 16);
    const ac = word(p + 20);
    const pc = word(p + 24);
    const ic = word(p + 28);
    const dc = word(p + 32);
    if (ac == 0 or ac > 4 or pc > 21 or ic > 1536 or dc > 4096) return fail(reason, "preparation_record_count");
    const expected: u64 = 160 + @as(u64, source_bytes) + @as(u64, ac) * 64 +
        @as(u64, pc) * 112 + @as(u64, ic) * 8 + @as(u64, dc) * 24;
    if (expected != size) return fail(reason, "preparation_exact_payload_length");
    const kind = word(p + 36);
    const heal = word(p + 40);
    const hero = word(p + 84);
    const context = word(p + 88);
    if ((kind != 1 and kind != 2) or heal > 1 or word(p + 44) > 1 or hero > 2 or
        context & ~@as(u32, 31) != 0 or word(p + 112) > 1) return fail(reason, "preparation_modes");
    if (word(p + 60) == 0 or word(p + 60) > 10000000 or word(p + 64) == 0 or word(p + 64) > 10000000)
        return fail(reason, "preparation_update_bounds");
    if ((word(p + 76) != 0xffffffff and word(p + 76) > 3) or
        (word(p + 80) != 0xffffffff and word(p + 80) > 11))
        return fail(reason, "preparation_roster_mode_or_selection");
    if (if (hero == 0) word(p + 68) != 0 or word(p + 72) != 0 else
        word(p + 68) == 0 or word(p + 68) > 99 or word(p + 72) > 0x7fffffff)
        return fail(reason, "preparation_hero_level_exp");
    if (kind == 1 and hero == 1) return fail(reason, "source_attested_cannot_grant_hero_levels");
    if (word(p + 112) == 0 and ic != 0) return fail(reason, "preserved_inventory_has_declared_counts");
    for (0..5) |i| {
        if (context & (@as(u32, 1) << @intCast(i)) == 0 and word(p + 92 + i * 4) != 0)
            return fail(reason, "unused_context_assertion_value");
    }
    const source = p + 160;
    const actors = source + source_bytes;
    const personas = actors + ac * 64;
    const items = personas + pc * 112;
    const decisions = items + ic * 8;
    if (p4_battle_save_valid(source, source_bytes) == 0) return fail(reason, "preparation_source_native_image");
    for (0..ac) |i| {
        const a = actors + i * 64;
        const id = word(a);
        if (id < 1 or id > 8 or id == 5 or (i == 0 and id != 1) or (i != 0 and id == 1))
            return fail(reason, "preparation_selected_actor_identity");
        for (0..i) |j| {
            if (word(actors + j * 64) == id) return fail(reason, "preparation_duplicate_actor");
        }
        for (0..3) |j| {
            const item = word(a + 4 + j * 4);
            if (item != 0xffffffff and item > 0x7fff) return fail(reason, "preparation_equipment_id_range");
        }
        const rm = word(a + 16);
        const tactic = word(a + 28);
        if (rm > 1 or (rm == 0 and !zero(a + 20, 8)) or
            (rm != 0 and (word(a + 20) > 999 or word(a + 24) > 999)))
            return fail(reason, "preparation_actor_resource_values");
        if (heal != 0 and rm != 0) return fail(reason, "competing_resource_policies");
        if ((tactic != 0xffffffff and tactic > 4) or !zero(a + 32, 32))
            return fail(reason, "preparation_actor_tactic_or_reserved");
    }
    var cursor: u32 = 0;
    for (0..pc) |i| {
        const q = personas + i * 112;
        const owner = word(q);
        const slot = word(q + 4);
        const id = word(q + 8);
        const mode = word(q + 12);
        const source_slot = word(q + 16);
        const level = word(q + 20);
        const exp = word(q + 24);
        const origin = word(q + 28);
        const first = word(q + 80);
        const count = word(q + 84);
        const mask = word(q + 88);
        if (owner < 1 or owner > 10 or (if (owner == 1) slot > 11 else slot != 0) or mode > 2 or
            mask & ~@as(u32, 31) != 0 or q[79] != 0 or !zero(q + 92, 20))
            return fail(reason, "preparation_persona_identity_mode_or_reserved");
        if ((mode != 0 or id != 0xffffffff) and (id == 0 or id > 65535))
            return fail(reason, "preparation_persona_id_range");
        for (0..i) |j| {
            const prev = personas + j * 112;
            if (word(prev) == owner and word(prev + 4) == slot)
                return fail(reason, "preparation_duplicate_persona_output");
        }
        if (origin > 1 or (if (origin != 0) source_slot != 0xffffffff else if (owner == 1) source_slot > 11 else source_slot != 0))
            return fail(reason, "preparation_persona_source_slot");
        if ((mode == 0 and (origin != 0 or mask & 16 != 0)) or
            (mode == 1 and (origin != 1 or (mask != 7 and mask != 15))) or
            (kind == 1 and (mode == 1 or origin != 0 or mask & 16 != 0)))
            return fail(reason, "preparation_persona_mode_capability");
        if (mode != 0 or mask & 1 != 0) {
            if (level == 0 or level > 99 or exp > 0x7fffffff) return fail(reason, "preparation_persona_level_exp");
        } else if (level != 0 or exp != 0) return fail(reason, "unused_persona_level_exp");
        if (mask & 2 != 0) {
            if (!skills(q + 48)) return fail(reason, "preparation_invalid_final_skills");
        } else if (!zero(q + 48, 16)) return fail(reason, "unused_persona_final_skills");
        if (mask & 16 != 0) {
            if (mode != 2 or !skills(q + 32)) return fail(reason, "preparation_starting_skills_mode");
        } else if (!zero(q + 32, 16)) return fail(reason, "unused_persona_starting_skills");
        if (mask & 4 != 0) {
            for (q[64..69]) |b| {
                if (b == 0 or b > 99) return fail(reason, "preparation_persona_stat_range");
            }
        } else if (!zero(q + 64, 5)) return fail(reason, "unused_persona_stat_assertions");
        if ((mode == 1 or mask & 8 == 0) and !zero(q + 69, 10)) return fail(reason, "unused_or_synthetic_bonus_bytes");
        if (mode != 2 and (first != 0 or count != 0)) return fail(reason, "nongrowth_persona_has_skill_decisions");
        if ((count == 0 and first != 0) or count > dc - cursor or (count != 0 and first != cursor))
            return fail(reason, "preparation_skill_decision_partition");
        var last: u32 = 0;
        for (0..count) |j| {
            const d = decisions + (@as(usize, first) + j) * 24;
            const learned_level = word(d + 8);
            const learned = word(d + 12);
            const replace = word(d + 16);
            const removed = word(d + 20);
            if (word(d) != owner or word(d + 4) != slot or learned_level == 0 or learned_level > level or
                learned_level < last or learned == 0 or learned > 4095 or
                (replace != 0xffffffff and replace > 7) or removed > 4095 or
                (replace == 0xffffffff and removed != 0))
                return fail(reason, "preparation_skill_decision_fields");
            last = learned_level;
        }
        cursor += count;
    }
    if (cursor != dc) return fail(reason, "preparation_unowned_skill_decisions");
    var seen: [192]u8 = @splat(0);
    for (0..ic) |i| {
        const item = items + i * 8;
        const id = word(item);
        if (id >= 1536 or word(item + 4) > 99) return fail(reason, "preparation_item_id_or_quantity_range");
        const bit = @as(u8, 1) << @as(u3, @intCast(id & 7));
        if (seen[id >> 3] & bit != 0) return fail(reason, "preparation_duplicate_inventory_id");
        seen[id >> 3] |= bit;
    }
    out.* = .{ .header = p, .source = source, .actors = actors, .personas = personas,
        .items = items, .decisions = decisions, .source_bytes = source_bytes,
        .actor_count = ac, .persona_count = pc, .item_count = ic, .decision_count = dc };
    return 1;
}
export fn p4_battle_prepare_persona_catalogued(id: u32) c_int {
    var metadata: u32 = undefined;
    var rows: u32 = undefined;
    if (p4_game_table_extent_rows(0x7644c4, 14, &metadata, &rows) == 0 or id >= rows) return 0;
    const p: *volatile u8 = @ptrCast(guest(metadata +% (id *% 14) +% 3));
    const level = p.*;
    return @intFromBool(level != 0 and level <= 99);
}
