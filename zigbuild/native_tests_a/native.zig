//! Native targets with source lists copied from the owning CMake fragments.
const std = @import("std");
const core = @import("../core.zig");
const gates = @import("../runtime_gates.zig");

pub const Registry = struct {
    external_test_names: []const []const u8,
    external_host_cases: []const gates.HostCase,
    external_executables: []const gates.ExternalExe,
};
pub const Retail = struct {
    iso: ?[]const u8 = null,
    elf: ?[]const u8 = null,
    ref_dir: ?[]const u8 = null,
};
/// Use the runtime gate's paired-retail validation rather than bypassing it
/// with raw ENVIRONMENT assignments.
pub fn addRetail(b: *std.Build, run: *std.Build.Step.Run, retail: Retail) void {
    if (retail.iso) |path| run.addArgs(&.{ "--retail-iso", path });
    if (retail.elf) |path| run.addArgs(&.{ "--retail-elf", path });
    if (retail.ref_dir) |path| {
        run.addArgs(&.{ "--ref-dir", path });
        if (retail.iso == null or retail.elf == null)
            run.step.dependOn(&b.addFail("A retail reference directory requires both ISO and ELF inputs").step);
    }
}
pub const Witness = struct {
    enabled: bool = false,
    wine: []const []const u8 = &.{},
    wine_ready: ?*std.Build.Step = null,
    gate_prefix: []const u8 = "gate",
};


const Test = struct {
    name: []const u8,
    exe: []const u8,
    sources: []const []const u8,
    includes: []const []const u8 = &.{},
    script: ?[]const u8 = null,
    before_exe: []const []const u8 = &.{},
    no_emulator: bool = false,
    driver_end: bool = false,
    second_target: ?[]const u8 = null,
    arg_bin: ?[]const u8 = null,
    args: []const []const u8 = &.{},
    env: []const []const u8 = &.{},
    cmd_env: []const []const u8 = &.{},
    os: bool = false,
    guest: bool = false,
    mod_input_core: bool = false,
    no_guest_fixture: bool = false,
    battle_zig: bool = false,
    decomp_include: bool = false,
    game_objects: bool = false,
    modgame: bool = false,
    ee_libm: bool = false,
    printf_abi: bool = false,
    gc_sections: bool = false,
    gc_source: ?[]const u8 = null,
    windows_ntdll: bool = false,
    defines: bool = false,
    extra_defines: []const []const u8 = &.{},
    math: bool = false,
    sdl: bool = false,
    ps2: bool = false,
    timeout: ?u32 = null,
    skip: ?u32 = null,
    host: bool = false,
    gl: bool = false,
    admission_bounds: bool = false,
};

const tests = [_]Test{
    .{ .name = "os_cpu_time", .exe = "os_cpu_time_test", .sources = &.{"tests/os_cpu_time_test.c"}, .os = true, .defines = true, .math = true, .timeout = 10 },
    .{ .name = "os_directory_lock", .exe = "os_directory_lock_test", .sources = &.{"tests/os_directory_lock_test.c"}, .os = true, .defines = true, .math = true, .script = "tests/os_directory_lock_test.py", .driver_end = true, .host = true, .timeout = 90 },
    .{ .name = "process_restart", .exe = "restart_probe", .sources = &.{ "tests/restart_probe.c", "src/platform/os/restart.c" }, .includes = &.{"include"}, .os = true, .math = true, .script = "tests/restart_test.py", .timeout = 90 },
    .{ .name = "vfs_origin", .exe = "vfs_origin_test", .sources = &.{ "tests/vfs_origin_test.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c" }, .includes = &.{"src/platform/files"}, .os = true, .defines = true, .math = true, .script = "tests/vfs_origin_test.py", .driver_end = true, .timeout = 30 },
    .{ .name = "vfs_source", .exe = "vfs_source_test", .sources = &.{ "tests/vfs_source_test.c", "tests/vfs_source_iso.c", "src/platform/files/vfs.c", "src/platform/content_digest.c" }, .includes = &.{"src/platform/files"}, .os = true, .defines = true, .math = true, .script = "tests/vfs_source_test.py", .driver_end = true, .timeout = 60 },
    .{ .name = "compendium_metadata", .exe = "compendium_metadata_test", .sources = &.{ "tests/compendium_metadata_test.c", "src/platform/mods/compendium.c", "src/platform/mods/package_memory.c", "third_party/yxml/yxml.c", "@os" }, .includes = &.{ "src/platform/mods", "third_party/yxml", "src/platform/files" }, .defines = true, .math = true, .script = "tests/compendium_metadata_test.py", .driver_end = true, .timeout = 120 },
    .{ .name = "package_registry", .exe = "package_registry_test", .sources = &.{ "tests/package_registry_test.c", "src/platform/mods/package_registry.c", "src/platform/mods/package_memory.c", "src/platform/mods/mod_setting_store.c", "src/platform/mods/compendium.c", "src/platform/mods/file_index.c", "third_party/yxml/yxml.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c", "@guest", "src/platform/state_dump.c", "tests/state_dump_test_stubs.c" }, .includes = &.{ "src/platform/files", "src/platform/mods" }, .defines = true, .extra_defines = &.{"P4_MOD_GAME_BINDINGS=1"}, .math = true, .script = "tests/package_registry_test.py", .driver_end = true, .host = true, .timeout = 300 },
    .{ .name = "package_files", .exe = "package_files_test", .sources = &.{ "tests/package_files_test.c", "src/platform/mods/compendium.c", "src/platform/mods/package_memory.c", "third_party/yxml/yxml.c", "@os", "src/platform/mods/file_index.c" }, .includes = &.{ "src/platform/mods", "third_party/yxml", "src/platform/files" }, .defines = true, .math = true, .script = "tests/package_files_test.py", .driver_end = true, .timeout = 120 },
    .{ .name = "mod_package_read", .exe = "mod_package_read_test", .sources = &.{ "tests/mod_package_read_test.c", "src/platform/mods/compendium.c", "src/platform/mods/package_memory.c", "third_party/yxml/yxml.c", "@os", "src/platform/mods/mods.c", "src/platform/mods/package_registry.c", "src/platform/mods/file_index.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c" }, .includes = &.{ "src/platform/mods", "third_party/yxml", "src/platform/files" }, .defines = true, .math = true, .script = "tests/mod_package_read_test.py", .driver_end = true, .timeout = 120 },
    .{ .name = "mod_files", .exe = "mod_files_test", .sources = &.{ "tests/mod_files_test.c", "src/platform/mods/mods.c", "src/platform/mods/package_registry.c", "src/platform/mods/package_memory.c", "src/platform/mods/mod_setting_store.c", "src/platform/mods/lifecycle.c", "src/platform/mods/mod_artifacts.c", "src/platform/mods/compendium.c", "src/platform/mods/file_index.c", "third_party/yxml/yxml.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c" }, .includes = &.{"src/platform/files"}, .os = true, .defines = true, .math = true, .script = "tests/mod_files_test.py", .driver_end = true },
    .{ .name = "mod_hooks", .exe = "mod_hooks_test", .sources = &.{ "tests/mod_hooks_test.c", "@modgame", "@guest", "src/platform/mods/hooks.c", "src/platform/mods/events.c", "src/platform/mods/failure.c", "src/platform/mods/numbers.c", "src/platform/mods/mods.c", "src/platform/mods/package_registry.c", "src/platform/mods/package_memory.c", "src/platform/mods/compendium.c", "src/platform/mods/file_index.c", "third_party/yxml/yxml.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c" }, .modgame = true, .defines = true, .math = true, .ps2 = true, .skip = 77 },
    .{ .name = "mod_hook_quiescence", .exe = "mod_hooks_test", .sources = &.{}, .script = "tests/mod_callback_quiescence_test.py", .before_exe = &.{"hook"}, .skip = 77, .host = true, .timeout = 30 },
    .{ .name = "mod_save_codec", .exe = "mod_save_test", .sources = &.{ "tests/mod_save_test.c", "@modcore", "src/platform/memcard/mod_save.c", "src/platform/memcard/mod_save_hash.c", "src/platform/memcard/mod_save_format.c", "src/platform/memcard/save_image.c" }, .includes = &.{"src/platform/files"}, .defines = true, .math = true, .script = "tests/mod_save_test.py", .driver_end = true, .host = true, .timeout = 60 },
    .{ .name = "mod_save_transport", .exe = "mod_save_transport_test", .sources = &.{ "tests/mod_save_transport_test.c", "@savecore", "src/platform/memcard/save_transaction.c", "src/platform/memcard/mod_save_custody.c", "src/platform/memcard/mod_save_transport.c", "src/platform/memcard/ps2_memcard.c" }, .includes = &.{"src/platform/files"}, .defines = true, .math = true, .script = "tests/mod_save_transport_test.py", .driver_end = true, .second_target = "save_import_test", .host = true, .timeout = 300 },
    .{ .name = "mod_configured_catalog", .exe = "mod_configured_catalog_test", .sources = &.{ "tests/mod_configured_catalog_test.c", "@modcore", "src/platform/mods/mods_config.c", "src/platform/config/p4cfg.c" }, .includes = &.{"src/platform/files"}, .defines = true, .math = true, .sdl = true, .script = "tests/mod_configured_catalog_test.py", .driver_end = true, .host = true, .timeout = 30 },
    .{ .name = "package_boot", .exe = "package_boot_test", .sources = &.{ "tests/package_boot_test.c", "@modcore" }, .includes = &.{ "src/platform/files", "src/platform/mods" }, .defines = true, .math = true, .script = "tests/package_boot_test.py", .driver_end = true, .host = true, .timeout = 30 },
    .{ .name = "mod_recovery_config", .exe = "mod_recovery_config_test", .sources = &.{ "tests/mod_recovery_config_test.c", "@modcore", "src/platform/mods/failure_config.c", "src/platform/mods/boot_health.c", "src/platform/config/p4cfg.c" }, .defines = true, .math = true, .sdl = true, .script = "tests/mod_recovery_config_test.py", .driver_end = true, .host = true, .timeout = 120 },
    .{ .name = "mod_selector", .exe = "mod_selector_test", .sources = &.{ "tests/mod_selector_test.c", "@modcore", "src/platform/mods/native_glyphs.c", "src/platform/os/restart.c", "src/platform/mods/mods_config.c", "src/platform/mods/packs.c", "src/platform/mods/packs_config.c", "src/platform/config/p4cfg.c" }, .includes = &.{"src/platform/files"}, .defines = true, .math = true, .sdl = true, .script = "tests/mod_selector_test.py", .driver_end = true, .host = true, .timeout = 30 },
    .{ .name = "pack_configured_catalog", .exe = "pack_configured_catalog_test", .sources = &.{ "tests/pack_configured_catalog_test.c", "@modcore", "src/platform/mods/mods_config.c", "src/platform/mods/packs.c", "src/platform/mods/packs_config.c", "src/platform/config/p4cfg.c" }, .includes = &.{"src/platform/files"}, .defines = true, .math = true, .sdl = true, .script = "tests/pack_configured_catalog_test.py", .driver_end = true, .host = true, .timeout = 90 },
    .{ .name = "mod_lifecycle", .exe = "mod_lifecycle_test", .sources = &.{ "tests/mod_lifecycle_test.c", "src/platform/mods/lifecycle.c" }, .os = true, .defines = true, .math = true, .arg_bin = "tests/mod-lifecycle-data", .host = true, .timeout = 30 },
    .{ .name = "mod_artifacts", .exe = "mod_artifacts_test", .sources = &.{ "tests/mod_artifacts_test.c", "src/platform/mods/mod_artifacts.c" }, .os = true, .defines = true, .math = true, .arg_bin = "tests/mod-artifacts-data", .host = true, .timeout = 60 },
    .{ .name = "game_table_inputs", .exe = "game_table_inputs_test", .sources = &.{ "tests/game_table_inputs_test.c", "tests/vfs_source_iso.c", "src/platform/mods/game_table_inputs.c", "src/platform/content_digest.c" }, .includes = &.{ "src/platform/files", "src/platform/mods" }, .mod_input_core = true, .os = true, .defines = true, .math = true, .script = "tests/game_table_inputs_test.py", .driver_end = true, .timeout = 240, .host = true },
    .{ .name = "game_table_admission", .exe = "game_table_admission_test", .sources = &.{ "tests/game_table_admission_test.c", "tests/vfs_source_iso.c", "src/platform/mods/game_table_inputs.c", "src/platform/mods/game_table_admission.c", "src/platform/mods/game_table_derived.c", "src/platform/mods/game_tables_prepare.c", "src/platform/content_digest.c" }, .includes = &.{ "src/platform/files", "src/platform/mods" }, .mod_input_core = true, .os = true, .defines = true, .math = true, .script = "tests/game_table_inputs_test.py", .driver_end = true, .host = true, .timeout = 240, .admission_bounds = true },
    .{ .name = "game_table_sources", .exe = "game_table_sources_test", .sources = &.{ "tests/game_table_sources_test.c", "@savecore", "@tablecore" }, .includes = &.{ "src/platform/files", "src/platform/memcard" }, .defines = true, .math = true, .ps2 = true, .script = "tests/game_table_sources_test.py", .driver_end = true, .host = true, .timeout = 60 },
    .{ .name = "game_table_manifest", .exe = "game_table_manifest_test", .sources = &.{ "tests/game_table_manifest_test.c", "src/platform/mods/game_table_manifest.c" }, .defines = true, .host = true, .timeout = 30 },
    .{ .name = "game_table_xml", .exe = "game_table_xml_test", .sources = &.{ "tests/game_table_xml_test.c", "@savecore", "@tablecore", "src/platform/mods/game_table_xml.c", "src/platform/mods/game_table_builder.c", "src/platform/mods/game_table_derived.c", "src/platform/mods/game_table_inputs_use.c" }, .includes = &.{ "src/platform/files", "src/platform/memcard", "src/platform/mods", "third_party/yxml" }, .defines = true, .math = true, .ps2 = true, .host = true, .timeout = 60 },
    .{ .name = "game_table_builder", .exe = "game_table_builder_test", .sources = &.{ "tests/game_table_builder_test.c", "@savecore", "@tablecore", "src/platform/mods/game_table_xml.c", "src/platform/mods/game_table_builder.c", "src/platform/mods/game_table_derived.c", "src/platform/mods/game_table_inputs_use.c" }, .includes = &.{ "src/platform/files", "src/platform/memcard", "src/platform/mods", "third_party/yxml" }, .defines = true, .math = true, .ps2 = true, .host = true, .timeout = 60, .script = "tests/game_elements_inputs_test.py", .driver_end = true },
    .{ .name = "game_table_boot", .exe = "game_table_boot_test", .sources = &.{ "@savecore", "@tablecore", "src/platform/mods/game_table_xml.c", "src/platform/mods/game_table_builder.c", "src/platform/mods/game_table_derived.c", "src/platform/mods/game_table_inputs_use.c", "tests/game_table_boot_test.c", "src/platform/mods/game_table_admission.c", "src/platform/mods/game_table_manifest.c", "src/platform/mods/game_table_boot.c", "src/platform/mods/game_table_delta.c", "src/platform/mods/mod_setting_gate.c" }, .no_guest_fixture = true, .includes = &.{ "src/platform/files", "src/platform/memcard", "src/platform/mods", "third_party/yxml" }, .defines = true, .math = true, .ps2 = true, .host = true, .timeout = 60, .script = "tests/game_table_boot_test.py", .driver_end = true },
    .{ .name = "game_table_derived", .exe = "game_table_derived_test", .sources = &.{ "@savecore", "@tablecore", "src/platform/mods/game_table_xml.c", "src/platform/mods/game_table_builder.c", "src/platform/mods/game_table_derived.c", "src/platform/mods/game_table_inputs_use.c", "src/platform/mods/game_table_admission.c", "src/platform/mods/game_table_manifest.c", "src/platform/mods/game_table_boot.c", "src/platform/mods/game_table_delta.c", "src/platform/mods/mod_setting_gate.c", "tests/game_table_derived_test.c", "src/platform/mods/game_table_derived_native.c", "src/platform/mods/game_table_extents.c" }, .no_guest_fixture = true, .includes = &.{ "src/platform/files", "src/platform/memcard", "src/platform/mods", "third_party/yxml" }, .defines = true, .math = true, .ps2 = true, .host = true, .timeout = 120, .script = "tests/game_table_derived_test.py", .driver_end = true },
    .{ .name = "game_table_input_use", .exe = "game_table_inputs_use_test", .sources = &.{ "tests/game_table_inputs_use_test.c", "src/platform/mods/game_table_inputs_use.c", "@savecore", "@tablecore" }, .includes = &.{ "src/platform/files", "src/platform/mods", "src/platform/memcard" }, .defines = true, .math = true, .ps2 = true, .script = "tests/game_table_inputs_test.py", .driver_end = true, .host = true, .timeout = 240 },
    .{ .name = "game_knowledge", .exe = "game_knowledge_test", .sources = &.{ "tests/game_knowledge_test.c", "@savecore", "src/platform/mods/game_knowledge.c", "src/platform/mods/game_knowledge_native.c", "src/platform/mods/game_knowledge_save.c", "src/platform/mods/game_knowledge_boot.c", "@tablecore" }, .includes = &.{ "src/platform/files", "src/platform/memcard", "src/platform/mods" }, .defines = true, .math = true, .ps2 = true, .script = "tests/game_knowledge_test.py", .driver_end = true, .host = true, .timeout = 60 },
    .{ .name = "game_enemies_native", .exe = "game_enemies_native_test", .sources = &.{ "tests/game_enemies_native_test.c", "@savecore", "@tablecore", "src/platform/mods/game_table_inputs_use.c", "src/platform/mods/game_skills_native.c", "src/platform/mods/game_enemies_native.c", "src/platform/mods/game_personas_native.c", "src/platform/mods/game_tables_packed.c", "src/platform/mods/game_table_admission.c", "src/platform/mods/game_table_derived.c", "src/platform/mods/game_table_extents.c" }, .includes = &.{ "src/platform/files", "src/platform/memcard", "src/platform/mods" }, .defines = true, .math = true, .ps2 = true, .script = "tests/game_enemies_test.py", .driver_end = true, .host = true, .timeout = 120, .skip = 77 },
    .{ .name = "game_skills_native", .exe = "game_skills_native_test", .sources = &.{ "tests/game_skills_native_test.c", "@savecore", "@tablecore", "src/platform/mods/game_table_inputs_use.c", "src/platform/mods/game_skills_native.c", "src/platform/mods/game_enemies_native.c", "src/platform/mods/game_personas_native.c", "src/platform/mods/game_tables_packed.c", "src/platform/mods/game_table_admission.c", "src/platform/mods/game_table_derived.c", "src/platform/mods/game_table_extents.c" }, .includes = &.{ "src/platform/files", "src/platform/memcard", "src/platform/mods" }, .defines = true, .math = true, .ps2 = true, .script = "tests/game_elements_inputs_test.py", .driver_end = true, .host = true, .timeout = 60, .skip = 77 },
    .{ .name = "game_personas_native", .exe = "game_personas_native_test", .sources = &.{ "tests/game_personas_native_test.c", "@savecore", "@tablecore", "src/platform/mods/game_table_inputs_use.c", "src/platform/mods/game_skills_native.c", "src/platform/mods/game_enemies_native.c", "src/platform/mods/game_personas_native.c", "src/platform/mods/game_personas_save.c", "src/platform/mods/game_tables_packed.c", "src/platform/mods/game_table_admission.c", "src/platform/mods/game_table_derived.c", "src/platform/mods/game_table_extents.c" }, .includes = &.{ "src/platform/files", "src/platform/memcard", "src/platform/mods" }, .defines = true, .math = true, .ps2 = true, .script = "tests/game_personas_test.py", .driver_end = true, .host = true, .timeout = 120, .skip = 77 },
    .{ .name = "game_skills_save", .exe = "game_skills_save_test", .sources = &.{ "tests/game_skills_save_test.c", "@savecore", "@tablecore", "src/platform/mods/game_skills_save.c" }, .includes = &.{ "src/platform/files", "src/platform/memcard", "src/platform/mods" }, .defines = true, .math = true, .ps2 = true, .script = "tests/game_skills_test.py", .driver_end = true, .host = true, .timeout = 120, .skip = 77 },
    .{ .name = "game_elements_affinity", .exe = "game_elements_affinity_test", .sources = &.{ "tests/game_elements_affinity_test.c", "src/platform/files/iso9660.c", "src/platform/mods/game_tables.c", "src/platform/mods/game_tables_prepare.c", "src/platform/mods/game_elements_affinity.c", "src/platform/mods/game_elements_action.c", "src/platform/mods/game_elements_native.c", "src/platform/mods/game_skills.c", "src/platform/mods/game_table_inputs.c", "src/platform/content_digest.c", "src/platform/mods/game_items.c", "src/platform/mods/game_text.c", "src/platform/mods/native_glyphs.c", "src/platform/mods/mod_resources.c", "src/platform/mods/game_damage_cover.c", "src/platform/mods/game_passive_grant.c", "src/platform/mods/game_stage.c", "src/platform/mods/game_damage_stage.c" }, .includes = &.{ "src/platform/files", "src/platform/mods" }, .mod_input_core = true, .guest = true, .defines = true, .math = true, .ps2 = true, .script = "tests/game_elements_inputs_test.py", .driver_end = true, .timeout = 120, .skip = 77, .host = true },
    .{ .name = "game_elements_action", .exe = "game_elements_action_test", .sources = &.{ "tests/game_elements_action_test.c", "src/platform/files/iso9660.c", "src/platform/mods/game_elements_action.c", "src/platform/mods/game_skills.c", "src/platform/mods/game_items.c", "src/platform/mods/game_table_inputs.c", "src/platform/content_digest.c", "src/platform/mods/game_tables.c", "src/platform/mods/game_text.c", "src/platform/mods/native_glyphs.c" }, .includes = &.{ "src/platform/files", "src/platform/mods" }, .mod_input_core = true, .guest = true, .defines = true, .math = true, .ps2 = true, .script = "tests/game_elements_inputs_test.py", .driver_end = true, .timeout = 120, .skip = 77, .host = true },
    .{ .name = "game_enemies", .exe = "game_enemies_test", .sources = &.{ "tests/game_enemies_test.c", "src/platform/files/iso9660.c", "src/platform/mods/game_enemies.c", "src/platform/mods/game_table_inputs.c", "src/platform/content_digest.c", "src/platform/mods/game_tables.c", "src/platform/mods/game_text.c", "src/platform/mods/native_glyphs.c" }, .includes = &.{ "src/platform/files", "src/platform/mods" }, .mod_input_core = true, .guest = true, .defines = true, .math = true, .ps2 = true, .script = "tests/game_enemies_test.py", .driver_end = true, .timeout = 120, .skip = 77, .host = true },
    .{ .name = "game_personas", .exe = "game_personas_test", .sources = &.{ "tests/game_personas_test.c", "src/platform/files/iso9660.c", "src/platform/mods/game_personas.c", "src/platform/mods/game_table_inputs.c", "src/platform/content_digest.c", "src/platform/mods/game_tables.c", "src/platform/mods/game_text.c", "src/platform/mods/native_glyphs.c" }, .includes = &.{ "src/platform/files", "src/platform/mods" }, .mod_input_core = true, .guest = true, .defines = true, .math = true, .ps2 = true, .script = "tests/game_personas_test.py", .driver_end = true, .timeout = 120, .skip = 77, .host = true },
    .{ .name = "game_skills", .exe = "game_skills_test", .sources = &.{ "tests/game_skills_test.c", "src/platform/files/iso9660.c", "src/platform/mods/game_skills.c", "src/platform/mods/game_table_inputs.c", "src/platform/content_digest.c", "src/platform/mods/game_tables.c", "src/platform/mods/game_text.c", "src/platform/mods/native_glyphs.c" }, .includes = &.{ "src/platform/files", "src/platform/mods" }, .mod_input_core = true, .guest = true, .defines = true, .math = true, .ps2 = true, .script = "tests/game_skills_test.py", .driver_end = true, .timeout = 120, .skip = 77, .host = true },
    .{ .name = "image_guest_ranges", .exe = "image_guest_ranges_test", .sources = &.{ "tests/image_guest_ranges_test.c", "src/image.c" }, .includes = &.{"include"}, .guest = true, .defines = true, .math = true, .ps2 = true, .script = "tests/image_guest_ranges_test.py", .driver_end = true, .timeout = 60 },
    .{ .name = "content_digest", .exe = "content_digest_test", .sources = &.{ "tests/content_digest_test.c", "src/platform/content_digest.c" }, .includes = &.{"include"}, .timeout = 30 },
    .{ .name = "game_table_extents", .exe = "game_table_extents_test", .sources = &.{ "tests/game_table_extents_test.c", "src/platform/mods/game_table_extents.c" }, .includes = &.{"include"}, .guest = true, .defines = true, .math = true, .skip = 77 },
    .{ .name = "game_table_overlay", .exe = "game_table_overlay_test", .sources = &.{ "tests/game_table_overlay_test.c", "src/platform/mods/game_table_overlay.c", "src/platform/mods/game_table_extents.c" }, .includes = &.{"include"}, .guest = true, .defines = true, .math = true, .skip = 77 },
    .{ .name = "battle_sim_table", .exe = "battle_sim_table_test", .sources = &.{ "tests/battle_sim_table_test.c", "src/platform/battle_sim/entry.zig", "src/platform/battle_sim/prepare.zig", "src/platform/battle_sim/result_prompt.zig", "src/platform/mods/game_table_extents.c" }, .includes = &.{"include"}, .battle_zig = true, .guest = true, .defines = true, .math = true, .skip = 77 },
    .{ .name = "battle_policy_ratings", .exe = "battle_policy_ratings_test", .sources = &.{ "tests/battle_policy_ratings_test.c", "src/platform/battle_sim/policy_test_adapter.zig" }, .includes = &.{"include"}, .battle_zig = true, .guest = true, .gc_sections = true, .gc_source = "src/platform/battle_sim/policy_test_adapter.zig", .windows_ntdll = true, .defines = true, .math = true, .ps2 = true, .skip = 77 },
    .{ .name = "battle_events", .exe = "battle_events_test", .sources = &.{ "tests/battle_events_test.c", "src/platform/battle_sim/events.zig" }, .includes = &.{"include"}, .battle_zig = true },
    .{ .name = "battle_strategy", .exe = "battle_strategy_test", .sources = &.{ "tests/battle_strategy_test.c", "src/platform/battle_sim/strategy.zig" }, .includes = &.{"include"}, .battle_zig = true },
    .{ .name = "battle_expert", .exe = "battle_expert_test", .sources = &.{ "tests/battle_expert_test.c", "src/platform/battle_sim/expert.zig", "src/platform/battle_sim/strategy.zig" }, .includes = &.{ "include", "src/platform" }, .battle_zig = true },
    .{ .name = "battle_expert_charge_zig", .exe = "battle_expert_charge_zig_test", .sources = &.{ "tests/battle_expert_charge_zig_test.c", "src/platform/battle_sim/policy.zig", "src/platform/battle_sim/expert.zig", "src/platform/battle_sim/strategy.zig", "src/platform/guest.c", "@os" }, .includes = &.{ "include", "src/platform" }, .battle_zig = true, .skip = 77 },
    .{ .name = "battle_observation", .exe = "battle_observation_test", .sources = &.{ "tests/battle_observation_test.c", "src/platform/battle_sim/observation.c", "src/platform/battle_sim/expert.zig", "src/platform/battle_sim/strategy.zig" }, .includes = &.{ "include", "src/platform" }, .battle_zig = true, .skip = 77 },
    .{ .name = "battle_expert_parity", .exe = "battle_expert_parity_test", .sources = &.{ "tests/battle_expert_parity_test.c", "tests/battle_expert_c_reference.c", "src/platform/battle_sim/expert.zig", "src/platform/battle_sim/strategy.zig" }, .includes = &.{ "include", "src/platform" }, .battle_zig = true },
    .{ .name = "battle_live_entry", .exe = "battle_live_entry_test", .sources = &.{ "tests/battle_live_entry_test.c", "src/platform/battle_sim/live_entry.zig" }, .includes = &.{"include"}, .battle_zig = true },
    .{ .name = "lockstep_battle_boundary", .exe = "lockstep_battle_boundary_test", .sources = &.{"tests/lockstep_battle_boundary_test.c"}, .includes = &.{"include"} },
    .{ .name = "qsort_retail_order", .exe = "qsort_test", .sources = &.{ "tests/qsort_test.c", "src/platform/ee_bytes.c" }, .includes = &.{"include"}, .guest = true, .math = true, .skip = 77 },
    .{ .name = "lockstep_restore_witness", .exe = "lockstep_restore_witness_test", .sources = &.{ "tests/lockstep_restore_witness_test.c", "tests/lockstep_align_guest_header_test.c", "src/platform/ee_bytes.c" }, .includes = &.{"include"}, .guest = true, .math = true, .skip = 77 },
    .{ .name = "matrix_override", .exe = "matrix_override_test", .sources = &.{"tests/matrix_override_test.c"}, .includes = &.{"overrides"}, .math = true },
    .{ .name = "ps2_libc", .exe = "ps2_libc_test", .sources = &.{ "tests/ps2_libc_test.c", "src/platform/ee_bytes.c", "src/platform/ps2_libc.c", "src/platform/ee_printf.c", "src/platform/ee_scanf.c", "src/platform/ee_strtod.c", "src/platform/ee_libm.c", "src/platform/ee_float.c", "overrides/00440bb8.c", "src/platform/ee_decimal.c", "src/platform/ee_dtoa.c", "src/platform/ps2_fpbit.c", "src/platform/kernel_heap.c", "src/platform/ee_heap.c", "tests/ee_heap_kernel.c", "src/platform/gport.c", "src/platform/guest.c", "@os" }, .includes = &.{ "include", "src/platform", "third_party" }, .gc_sections = true, .math = true, .skip = 77 },
    .{ .name = "ee_decimal_retail_oracle", .exe = "ee_decimal_oracle_test", .sources = &.{ "tests/ee_decimal_oracle_test.c", "@decimal", "src/platform/ee_float.c", "src/image.c", "tools/mipsdiff/r5900.c", "@stdio" }, .includes = &.{ "include", "src/platform", "third_party", "tools/mipsdiff" }, .math = true, .ps2 = true, .skip = 77, .timeout = 300 },
    .{ .name = "ee_printf_retail_oracle", .exe = "ee_printf_oracle_test", .sources = &.{ "tests/ee_printf_oracle_test.c", "@decimal", "src/platform/ee_printf.c", "src/platform/ee_float.c", "src/image.c", "tools/mipsdiff/r5900.c", "@stdio" }, .includes = &.{ "include", "src/platform", "third_party", "tools/mipsdiff" }, .math = true, .ps2 = true, .skip = 77, .timeout = 300 },
    .{ .name = "ee_printf_abi_retail_oracle", .exe = "ee_printf_abi_oracle_test", .sources = &.{ "tests/ee_printf_abi_oracle_test.c", "@decimal", "src/platform/ee_printf.c", "src/platform/ee_float.c", "src/image.c", "tools/mipsdiff/r5900.c", "@stdio" }, .includes = &.{ "include", "src/platform", "third_party", "tools/mipsdiff" }, .printf_abi = true, .math = true, .ps2 = true, .skip = 77, .timeout = 300 },
    .{ .name = "ee_libm", .exe = "ee_libm_test", .sources = &.{ "tests/ee_libm_test.c", "src/platform/ee_float.c" }, .ee_libm = true, .math = true, .skip = 77 },
    .{ .name = "ee_scanf_retail_oracle", .exe = "ee_scanf_oracle_test", .sources = &.{ "tests/ee_scanf_oracle_test.c", "@crt", "src/image.c", "tools/mipsdiff/r5900.c", "@stdio" }, .includes = &.{ "include", "src/platform", "third_party", "tools/mipsdiff" }, .math = true, .skip = 77, .timeout = 300 },
    .{ .name = "ee_dlibm_retail", .exe = "ee_dlibm_test", .sources = &.{ "tests/ee_dlibm_test.c", "src/platform/ee_dlibm.c", "src/platform/ee_dfloat.c", "src/platform/ps2_fpbit.c", "src/platform/ee_float.c", "src/platform/guest.c", "tools/mipsdiff/r5900.c", "@os" }, .includes = &.{"src/platform"}, .math = true, .ps2 = true, .skip = 77, .timeout = 180 },
    .{ .name = "vu0_helpers", .exe = "vu0_helpers_test", .sources = &.{ "tests/vu0_helpers_test.c", "src/vu0.c", "src/platform/ee_float.c" }, .includes = &.{"include"}, .math = true },
    .{ .name = "fpbit", .exe = "fpbit_test", .sources = &.{ "tests/fpbit_test.c", "src/platform/ps2_fpbit.c", "src/platform/guest.c" }, .includes = &.{ "include", "src/platform" }, .os = true, .math = true },
    .{ .name = "iso_read", .exe = "iso_read_test", .sources = &.{"tests/iso_read_test.c"}, .includes = &.{"src/platform/files"}, .timeout = 30 },
    .{ .name = "game_tables", .exe = "game_tables_test", .sources = &.{ "tests/game_tables_test.c", "src/platform/mods/game_tables.c" }, .defines = true },
    .{ .name = "game_tables_prepare", .exe = "game_tables_prepare_test", .sources = &.{ "tests/game_tables_prepare_test.c", "src/platform/mods/game_tables_prepare.c" }, .defines = true },
    .{ .name = "mod_numbers", .exe = "mod_numbers_test", .sources = &.{ "tests/mod_numbers_test.c", "src/platform/mods/numbers.c" }, .math = true },
    .{ .name = "game_elements_amount", .exe = "game_elements_amount_test", .sources = &.{ "tests/game_elements_amount_test.c", "src/platform/mods/game_stage.c", "src/platform/mods/game_damage_stage.c", "src/platform/mods/game_recovery_stage.c" }, .decomp_include = true, .defines = true, .math = true, .host = true, .timeout = 60, .skip = 77 },
    .{ .name = "game_damage_stage", .exe = "game_damage_stage_test", .sources = &.{ "tests/game_damage_stage_test.c", "src/platform/mods/game_stage.c", "src/platform/mods/game_damage_stage.c", "src/platform/mods/game_recovery_stage.c" }, .defines = true, .math = true, .host = true, .timeout = 60 },
    .{ .name = "game_amount_stages", .exe = "game_amount_stages_test", .sources = &.{ "tests/game_amount_stages_test.c", "src/platform/mods/game_stage.c", "src/platform/mods/game_damage_stage.c", "src/platform/mods/game_recovery_stage.c", "src/platform/mods/game_cost_stage.c", "src/platform/mods/game_chance_stage.c", "src/platform/mods/game_wearoff_stage.c", "src/platform/mods/game_buff_stage.c", "src/platform/mods/game_reward_stage.c", "src/platform/mods/game_unit_stage.c", "src/platform/mods/game_afflictions.c", "src/platform/battle_sim/events.zig" }, .battle_zig = true, .defines = true, .math = true, .host = true, .timeout = 60 },
    .{ .name = "game_elements_hit", .exe = "game_elements_hit_test", .sources = &.{ "tests/game_elements_hit_test.c", "src/platform/mods/game_stage.c", "src/platform/mods/game_chance_stage.c", "src/platform/mods/game_hit_stage.c" }, .defines = true, .math = true, .host = true, .timeout = 60, .skip = 77 },
    .{ .name = "game_elements_route", .exe = "game_elements_route_test", .sources = &.{ "tests/game_elements_route_test.c", "src/platform/mods/game_elements_route.c" }, .defines = true, .host = true, .timeout = 60 },
    .{ .name = "golden_source", .exe = "golden_source_test", .sources = &.{ "tests/golden_source_test.c", "src/platform/golden/golden_formats.c", "src/platform/golden/golden_source.c", "src/platform/golden/golden_tables.c", "src/platform/content_digest.c" }, .includes = &.{"include"}, .defines = true, .timeout = 30 },
    .{ .name = "vag", .exe = "vag_test", .sources = &.{ "tests/vag_test.c", "src/platform/sound/vag.c" }, .includes = &.{ "include", "src/platform/sound" } },
    .{ .name = "spu_pitch", .exe = "spu_pitch_test", .sources = &.{ "tests/spu_pitch_test.c", "src/platform/sound/spu_pitch.c", "src/platform/ee_float.c", "src/platform/guest.c", "tools/mipsdiff/r5900.c", "@os" }, .includes = &.{"src/platform/sound"}, .extra_defines = &.{"P4_MIPSDIFF_RAM_LO=0x1000u"}, .math = true, .ps2 = true },
    .{ .name = "spu_pitch_retail", .exe = "spu_pitch_test", .sources = &.{}, .args = &.{"--require-retail"}, .skip = 77, .timeout = 120 },
    .{ .name = "fpu_state", .exe = "fpu_state_test", .sources = &.{ "tests/fpu_state_test.c", "tests/state_dump_test_stubs.c", "src/platform/state_dump.c", "@os" }, .math = true, .skip = 77 },
    .{ .name = "p4iso", .exe = "p4iso_test", .sources = &.{ "tests/p4iso_test.c", "src/platform/config/p4iso.c", "src/platform/config/p4cfg.c", "src/platform/files/iso9660.c", "@os" }, .defines = true, .math = true, .sdl = true },
    .{ .name = "p4diag", .exe = "p4diag_test", .sources = &.{ "tests/p4diag_test.c", "src/platform/diag/p4diag.c", "src/platform/config/p4cfg.c", "src/platform/guest.c", "@os" }, .defines = true, .math = true, .sdl = true },
    .{ .name = "files_disc", .exe = "files_test", .sources = &.{ "tests/files_test.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c", "@os" }, .includes = &.{"src/platform/files"}, .defines = true, .skip = 77 },
    .{ .name = "native_tex", .exe = "native_tex_test", .sources = &.{ "tests/native_tex_test.c", "src/render/p4gl_nativetex.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c", "@os" }, .includes = &.{ "src/render", "src/platform/files" }, .defines = true, .skip = 77 },
    .{ .name = "sdr", .exe = "sdr_test", .sources = &.{ "tests/sdr_test.c", "src/platform/sound/ps2_sndcom.c", "src/platform/sound/sdr.c", "src/platform/sound/sdr_mem.c", "src/platform/sound/spu_pitch.c", "src/platform/sound/hd_bank.c", "src/platform/sound/vag.c", "src/platform/sound/p4snd_mixer.c", "src/platform/sound/p4snd_reverb.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c", "src/platform/gport.c", "src/platform/guest.c", "@os", "tests/state_dump_test_stubs.c", "src/platform/kernel_thread.c", "src/platform/state_dump.c", "src/platform/ps2_kernel.c" }, .includes = &.{ "include", "tests", "src/platform/sound", "src/platform/files" }, .defines = true, .math = true, .sdl = true, .env = &.{ "P4PC_VIRTUAL_CLOCK=1", "P4PC_TURBO=0" } },
    .{ .name = "texture_key", .exe = "texture_key_test", .sources = &.{ "tests/texture_key_test.c", "src/render/p4texture_key.c", "src/platform/content_digest.c" }, .includes = &.{"include"} },
    .{ .name = "content_source", .exe = "content_source_test", .sources = &.{ "tests/content_source_test.c", "src/platform/p4content_source.c", "src/platform/content_digest.c", "src/platform/ee_bytes.c" }, .includes = &.{ "include", "src/render" } },
    .{ .name = "alpha_pass_cull", .exe = "alpha_pass_cull_test", .sources = &.{"tests/alpha_pass_cull_test.c"}, .includes = &.{"src/render"}, .math = true },
    .{ .name = "content_mips", .exe = "content_mips_test", .sources = &.{ "tests/content_mips_test.c", "src/render/p4content_mips.c" }, .includes = &.{"src/render"} },
    .{ .name = "content_image", .exe = "content_image_test", .sources = &.{ "tests/content_image_test.c", "src/platform/mods/p4content_image.c", "third_party/miniz/miniz.c", "third_party/miniz/miniz_tdef.c", "third_party/miniz/miniz_tinfl.c" }, .includes = &.{"src/render"} },
    .{ .name = "png_writer", .exe = "png_writer_test", .sources = &.{ "tests/png_writer_test.c", "@png", "third_party/miniz/miniz_tinfl.c" }, .includes = &.{"src/viewer"}, .defines = true },
    .{ .name = "texture_dump", .exe = "texture_dump_test", .sources = &.{ "tests/texture_dump_test.c", "src/render/p4texture_key.c", "src/platform/content_digest.c", "src/platform/p4texture_dump.c", "@png", "@os" }, .includes = &.{ "include", "src/viewer" }, .defines = true, .sdl = true, .script = "tests/texture_dump_test.py" },
    .{ .name = "gl_cache", .exe = "gl_cache_test", .sources = &.{ "tests/gl_cache_test.c", "src/render/p4gl_gl.c", "src/render/p4gl_rt.c", "src/render/p4gl_profile.c" }, .includes = &.{"src/render"}, .math = true, .sdl = true, .gl = true, .skip = 77 },
    .{ .name = "render_thread", .exe = "render_thread_test", .sources = &.{ "tests/render_thread_test.c", "src/render/p4gl_gl.c", "src/render/p4gl_rt.c", "src/render/p4gl_profile.c" }, .includes = &.{"src/render"}, .math = true, .sdl = true },
    .{ .name = "gl_cache_rt", .exe = "gl_cache_test", .sources = &.{}, .cmd_env = &.{"P4PC_RENDER_THREAD=1"}, .gl = true, .skip = 77 },
    .{ .name = "gpu_trace", .exe = "gpu_trace_test", .sources = &.{ "tests/gpu_trace_test.c", "src/render/p4gl_profile.c", "src/render/p4gl_gl.c", "src/render/p4gl_rt.c", "@os" }, .includes = &.{"src/render"}, .extra_defines = &.{ "SDL_GL_GetProcAddress=p4gpu_test_proc", "SDL_STATIC_LIB" }, .math = true, .sdl = true },
    .{ .name = "gles_context_smoke", .exe = "gles_context_smoke", .sources = &.{ "tests/gles_context_smoke.c", "@png", "@os", "src/render/p4gl_gl.c", "src/render/p4gl_rt.c", "src/render/p4gl_profile.c" }, .includes = &.{ "src/render", "src/viewer" }, .defines = true, .math = true, .sdl = true, .arg_bin = "gles_context_smoke.png", .gl = true, .skip = 77, .timeout = 120 },
    .{ .name = "config", .exe = "config_test", .sources = &.{ "tests/config_test.c", "src/platform/config/p4cfg.c", "@os" }, .defines = true, .math = true, .sdl = true },
    .{ .name = "utf8_path", .exe = "utf8_path_test", .sources = &.{ "tests/utf8_path_test.c", "src/platform/config/p4cfg.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c", "@os" }, .includes = &.{"src/platform/files"}, .defines = true, .math = true, .sdl = true, .arg_bin = "utf8_path_scratch" },
    .{ .name = "pad_keyboard", .exe = "pad_keyboard_test", .sources = &.{ "tests/pad_keyboard_test.c", "src/platform/pad/pad_sdl.c", "src/platform/config/p4cfg.c", "src/platform/pad/walk_script.c", "src/platform/pad/ps2_pad.c", "@os", "tests/battle_sim_inactive.c" }, .defines = true, .math = true, .sdl = true },
    .{ .name = "pad_gamepad", .exe = "pad_gamepad_test", .sources = &.{ "tests/pad_gamepad_test.c", "src/platform/pad/pad_sdl.c", "src/platform/config/p4cfg.c", "src/platform/pad/walk_script.c", "src/platform/pad/ps2_pad.c", "@os", "tests/battle_sim_inactive.c" }, .defines = true, .math = true, .sdl = true, .gl = true, .skip = 77 },
    .{ .name = "pad_gamepad_frame_clock", .exe = "pad_gamepad_test", .sources = &.{}, .args = &.{"--frame-clocked"}, .gl = true, .skip = 77 },
    .{ .name = "pad_isolation_script", .exe = "pad_isolation_test", .sources = &.{ "tests/pad_isolation_test.c", "src/platform/pad/pad_sdl.c", "src/platform/config/p4cfg.c", "src/platform/pad/walk_script.c", "src/platform/pad/ps2_pad.c", "@os", "tests/battle_sim_inactive.c" }, .defines = true, .math = true, .sdl = true, .args = &.{"script"}, .gl = true, .skip = 77 },
    .{ .name = "pad_isolation_virtual", .exe = "pad_isolation_test", .sources = &.{}, .args = &.{"virtual"}, .gl = true, .skip = 77 },
    .{ .name = "memcard_roundtrip", .exe = "memcard_test", .sources = &.{ "tests/memcard_test.c", "src/platform/memcard/ps2_memcard.c", "@os", "@modcore_no_os", "src/platform/memcard/mod_save.c", "src/platform/memcard/mod_save_hash.c", "src/platform/memcard/mod_save_format.c", "src/platform/memcard/save_image.c", "src/platform/memcard/save_transaction.c", "src/platform/memcard/mod_save_custody.c", "src/platform/memcard/mod_save_transport.c" }, .includes = &.{"src/platform/files"}, .defines = true, .math = true, .arg_bin = "memcard_test_save" },
    .{ .name = "memcard_long_path", .exe = "memcard_test", .sources = &.{}, .arg_bin = "memcard_long_path_save", .args = &.{"--long-path"} },
    .{ .name = "memcard_utf8", .exe = "memcard_test", .sources = &.{}, .arg_bin = "memcard_utf8_save", .args = &.{"--utf8"} },
    .{ .name = "game_text", .exe = "game_text_test", .sources = &.{ "tests/game_text_test.c", "src/platform/mods/game_text.c", "src/platform/mods/native_glyphs.c", "src/platform/mods/native_glyphs_native.c", "src/platform/mods/game_text_native.c" }, .guest = true, .defines = true, .math = true, .ps2 = true, .skip = 77 },
    .{ .name = "game_items", .exe = "game_items_test", .sources = &.{ "tests/game_items_test.c", "src/platform/mods/game_items.c", "src/platform/mods/game_tables.c", "src/platform/mods/game_items_native.c", "src/platform/mods/game_items_ui.c", "src/platform/mods/game_items_equipment.c", "src/platform/mods/game_items_battle.c", "src/platform/mods/game_items_list.c", "src/platform/mods/game_items_shop.c", "src/platform/mods/game_tables_prepare.c", "src/platform/mods/game_text.c", "src/platform/mods/native_glyphs.c", "src/platform/mods/mod_resources.c" }, .guest = true, .defines = true, .math = true, .ps2 = true, .skip = 77 },
    .{ .name = "game_table_loader_boundary", .exe = "game_table_loader_boundary_test", .sources = &.{ "tests/game_table_loader_boundary_test.c", "tests/game_table_loader_boundary_support.c", "src/platform/mods/game_items.c", "src/platform/mods/game_tables.c", "src/platform/mods/game_items_native.c", "src/platform/mods/game_items_ui.c", "src/platform/mods/game_items_equipment.c", "src/platform/mods/game_items_battle.c", "src/platform/mods/game_items_list.c", "src/platform/mods/game_items_shop.c", "src/platform/mods/game_tables_prepare.c", "src/platform/mods/game_text.c", "src/platform/mods/native_glyphs.c", "src/platform/mods/mod_resources.c" }, .guest = true, .defines = true, .math = true, .ps2 = true, .skip = 77, .host = true, .timeout = 60 },
    .{ .name = "mod_memory_policy", .exe = "mod_memory_policy_test", .sources = &.{ "tests/mod_memory_policy_test.c", "src/platform/mods/memory_policy.c" }, .includes = &.{"include"}, .guest = true, .defines = true, .math = true, .ps2 = true, .timeout = 30 },
    .{ .name = "heap_guest_span", .exe = "heap_guest_span_test", .sources = &.{ "tests/heap_guest_span_test.c", "src/platform/kernel_heap.c", "src/platform/ee_heap.c", "tests/ee_heap_kernel.c", "src/platform/gport.c", "src/platform/guest.c" }, .includes = &.{"include"}, .os = true, .defines = true, .math = true, .ps2 = true, .timeout = 30 },
    .{ .name = "image_read", .exe = "image_read_test", .sources = &.{"tests/image_read_test.c"}, .includes = &.{"include"}, .guest = true, .defines = true, .math = true, .ps2 = true, .timeout = 30 },
    .{ .name = "native_glyphs", .exe = "native_glyphs_test", .sources = &.{ "tests/native_glyphs_test.c", "src/platform/mods/native_glyphs.c", "src/platform/mods/native_glyphs_native.c" }, .guest = true, .defines = true, .math = true, .ps2 = true, .skip = 77 },
    .{ .name = "game_knowledge_ai", .exe = "game_knowledge_ai_test", .sources = &.{"tests/game_knowledge_ai_test.c"}, .defines = true },
    .{ .name = "game_elements_ai_roles", .exe = "game_elements_ai_roles_test", .sources = &.{ "tests/game_elements_ai_roles_test.c", "src/platform/mods/game_elements_ai_roles.c" }, .defines = true },
    .{ .name = "game_elements_ai_decision", .exe = "game_elements_ai_decision_test", .sources = &.{ "tests/game_elements_ai_decision_test.c", "src/platform/mods/game_elements_ai_decision.c" }, .guest = true, .defines = true, .ps2 = true },
    .{ .name = "game_elements_ui", .exe = "game_elements_ui_test", .sources = &.{ "tests/game_elements_ui_test.c", "src/platform/mods/game_elements_ui.c", "src/platform/mods/game_tables.c", "src/platform/mods/mod_resources.c" }, .guest = true, .defines = true, .math = true, .ps2 = true, .skip = 77, .host = true },
    .{ .name = "mod_resources", .exe = "mod_resources_test", .sources = &.{ "tests/mod_resources_test.c", "src/platform/mods/mod_resources.c" }, .guest = true, .defines = true, .math = true, .ps2 = true, .skip = 77 },
    .{ .name = "mod_resources_render", .exe = "mod_resources_render", .sources = &.{"tests/mod_resources_render_test.c"}, .includes = &.{ "include", "src/render" }, .game_objects = true, .defines = true, .ps2 = true, .gl = true, .skip = 77, .timeout = 120 },
    .{ .name = "mod_resources_boot", .exe = "mod_resources_boot", .sources = &.{"tests/mod_resources_boot_test.c"}, .includes = &.{ "include", "src/render", "src/platform/mods" }, .game_objects = true, .defines = true, .ps2 = true, .script = "tests/mod_resources_boot_test.py", .driver_end = true, .gl = true, .timeout = 180 },
    .{ .name = "mod_resources_boot_cpu", .exe = "mod_resources_boot", .sources = &.{}, .script = "tests/mod_resources_boot_test.py", .driver_end = true, .host = true, .timeout = 180, .args = &.{ "ResourceBoot.test_early_decode_without_graphics_context", "ResourceBoot.test_early_ownership_conflict_without_graphics_context", "ResourceBoot.test_late_failure_keeps_real_selected_provider", "ResourceBoot.test_invalid_or_changed_source_refused_before_exposure", "ResourceBoot.test_item_ui_missing_or_wrong_revision_refused_early" } },
    .{ .name = "mod_settings", .exe = "mod_settings_test", .sources = &.{"tests/mod_settings_test.c"}, .includes = &.{"include"}, .game_objects = true, .defines = true, .ps2 = true, .timeout = 30 },
    .{ .name = "settings_render", .exe = "settings_render", .sources = &.{"tests/settings_render_test.c"}, .includes = &.{ "include", "src/render" }, .game_objects = true, .defines = true, .ps2 = true, .arg_bin = "settings_render", .gl = true, .skip = 77, .timeout = 120 },
    .{ .name = "cri_sfxz_crt", .exe = "cri_sfxz_crt_test", .sources = &.{"tests/cri_sfxz_crt_test.c"}, .includes = &.{"include"}, .game_objects = true, .defines = true, .ps2 = true, .skip = 77, .timeout = 60 },
    .{ .name = "ee_heap_retail", .exe = "ee_heap_retail_test", .sources = &.{ "tests/ee_heap_retail_test.c", "tools/mipsdiff/r5900.c" }, .includes = &.{ "include", "src/platform", "tools/mipsdiff" }, .game_objects = true, .defines = true, .ps2 = true, .skip = 77, .timeout = 300 },
    .{ .name = "sky_vector_vu0", .exe = "sky_vector_vu0_test", .sources = &.{ "tests/sky_vector_vu0_test.c", "tools/mipsdiff/r5900.c" }, .includes = &.{ "include", "tools/mipsdiff" }, .game_objects = true, .defines = true, .ps2 = true, .skip = 77, .timeout = 120 },
    .{ .name = "texture_capture", .exe = "texture_capture_test", .sources = &.{"tests/texture_capture_test.c"}, .includes = &.{ "include", "src/render" }, .game_objects = true, .defines = true, .ps2 = true, .arg_bin = "texture_capture_dump", .gl = true, .skip = 77, .timeout = 120, .env = &.{ "P4PC_TURBO=0", "P4PC_TEXTURE_DUMP=" } },
    .{ .name = "render_smoke", .exe = "render_smoke", .sources = &.{"tests/render_smoke.c"}, .includes = &.{ "include", "src/render", "src/viewer", "src/platform/files" }, .game_objects = true, .defines = true, .ps2 = true, .arg_bin = "render_smoke.png", .gl = true, .skip = 77 },
    .{ .name = "render_smoke_gl33", .exe = "render_smoke", .sources = &.{}, .arg_bin = "render_smoke_gl33.png", .cmd_env = &.{"P4PC_GL_API=gl"}, .gl = true, .skip = 77 },
    .{ .name = "render_smoke_es30", .exe = "render_smoke", .sources = &.{}, .arg_bin = "render_smoke_es30.png", .cmd_env = &.{ "P4PC_GL_API=gles", "MESA_GLES_VERSION_OVERRIDE=3.0", "P4PC_GLES_BORDER=shader" }, .gl = true, .skip = 77 },
    .{ .name = "render_smoke_rt", .exe = "render_smoke", .sources = &.{}, .arg_bin = "render_smoke_rt.png", .cmd_env = &.{"P4PC_RENDER_THREAD=1"}, .gl = true, .skip = 77 },
    .{ .name = "renderer_reference_scope", .exe = "render_smoke", .sources = &.{}, .script = "tests/renderer_scope_test.py", .no_emulator = true, .arg_bin = "renderer_reference_scope.png", .gl = true, .skip = 77, .timeout = 90 },
};

fn append(comptime T: type, b: *std.Build, list: *std.ArrayList(T), item: T) void {
    list.append(b.allocator, item) catch @panic("OOM");
}
pub fn selected(selection: gates.Selection, name: []const u8, labels: []const []const u8) bool {
    for (selection.exclude_names) |item| if (std.mem.eql(u8, item, name)) return false;
    for (selection.exclude_labels) |excluded| for (labels) |label| {
        if (std.mem.eql(u8, excluded, label)) return false;
    };
    if (selection.include_labels) |required| for (required) |item| {
        var found = false;
        for (labels) |label| if (std.mem.eql(u8, item, label)) {
            found = true;
            break;
        };
        if (!found) return false;
    };
    return true;
}
fn inheritsTablePs2Ram(name: []const u8) bool {
    for ([_][]const u8{
        "game_table_xml", "game_table_builder", "game_table_input_use",
        "game_enemies_native", "game_personas_native", "game_skills_native",
        "game_skills_save", "game_table_boot", "game_table_derived",
    }) |item| if (std.mem.eql(u8, item, name)) return true;
    return false;
}





pub fn register(b: *std.Build, graph: *core.Graph, nonserial: *std.Build.Step, default_timeout: u32, software_gl: bool, retail: Retail, witness: Witness, selection: gates.Selection, x64: *std.Build.Step, x64w: *std.Build.Step) Registry {
    var names: std.ArrayList([]const u8) = .empty;
    var cases: std.ArrayList(gates.HostCase) = .empty;
    var exes: std.ArrayList(gates.ExternalExe) = .empty;
    var built_targets: std.StringArrayHashMapUnmanaged(?core.Artifact) = .empty;
    const vars = core.vars(graph, .i386);
    core.requireTwinLibrary(graph, "printf_abi_calls");
    const printf_calls = core.twinBaseLibrary(graph, .{
        .name = "printf_abi_calls",
        .sources = &.{"tests/data/ee_printf_abi_calls.c"},
        .launcher = .{ .printf = .{ .root = b.pathJoin(&.{ graph.root, "tests/data" }), .modes = "none" } },
    });
    const ee_libm_obj = if (vars.ee_float_ops) |modes| core.twinBaseLibrary(graph, .{
        .name = "ee_libm_obj", .sources = &.{"src/platform/ee_libm.c"}, .launcher = .{ .ee_float = modes },
    }) else null;
    const host_label = b.step("native-a-native-label-host", "Native host-labeled 000-030 fixtures");
    const gl_label = b.step("native-a-native-label-gl", "Native GL-labeled 000-030 fixtures");
    for (tests) |t| {
        if (t.ee_libm and vars.ee_float_ops == null) continue;
        const fixture: ?core.GeneratedFiles = if (t.modgame) core.modTestGame(graph) else null;
        var src: std.ArrayList([]const u8) = .empty;
        for (t.sources) |source| {
            if (std.mem.eql(u8, source, "@guest")) {
                src.appendSlice(b.allocator, &.{ "tests/guest_test.c", "src/platform/gport.c", "src/platform/guest.c", vars.os_src }) catch @panic("OOM");
            } else if (std.mem.eql(u8, source, "@modcore")) {
                src.appendSlice(b.allocator, &.{ "src/platform/mods/mods.c", "src/platform/mods/package_registry.c", "src/platform/mods/package_memory.c", "src/platform/mods/mod_setting_store.c", "src/platform/mods/lifecycle.c", "src/platform/mods/mod_artifacts.c", "src/platform/mods/compendium.c", "src/platform/mods/file_index.c", "third_party/yxml/yxml.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c", vars.os_src }) catch @panic("OOM");
            } else if (std.mem.eql(u8, source, "@modcore_no_os")) {
                src.appendSlice(b.allocator, &.{ "src/platform/mods/mods.c", "src/platform/mods/package_registry.c", "src/platform/mods/package_memory.c", "src/platform/mods/mod_setting_store.c", "src/platform/mods/lifecycle.c", "src/platform/mods/mod_artifacts.c", "src/platform/mods/compendium.c", "src/platform/mods/file_index.c", "third_party/yxml/yxml.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c" }) catch @panic("OOM");
            } else if (std.mem.eql(u8, source, "@savecore")) {
                src.appendSlice(b.allocator, &.{ "src/platform/mods/mods.c", "src/platform/mods/package_registry.c", "src/platform/mods/package_memory.c", "src/platform/mods/mod_setting_store.c", "src/platform/mods/lifecycle.c", "src/platform/mods/mod_artifacts.c", "src/platform/mods/compendium.c", "src/platform/mods/file_index.c", "third_party/yxml/yxml.c", "src/platform/files/vfs.c", "src/platform/files/iso9660.c", vars.os_src, "src/platform/memcard/mod_save.c", "src/platform/memcard/mod_save_hash.c", "src/platform/memcard/mod_save_format.c", "src/platform/memcard/save_image.c" }) catch @panic("OOM");
            } else if (std.mem.eql(u8, source, "@tablecore")) {
                // The source cohort was deduplicated after adding P4_GUEST_TEST_SRC:
                // the OS source already appeared in @savecore.
                src.appendSlice(b.allocator, &.{ "src/platform/mods/game_table_sources.c", "src/platform/mods/game_table_refs.c", "src/platform/mods/game_tables.c", "src/platform/mods/game_tables_prepare.c", "src/platform/mods/game_table_inputs.c", "src/platform/content_digest.c", "src/platform/mods/game_skills.c", "src/platform/mods/game_enemies.c", "src/platform/mods/game_personas.c", "src/platform/mods/game_items.c", "src/platform/mods/game_items_save.c", "src/platform/mods/game_text.c", "src/platform/mods/native_glyphs.c" }) catch @panic("OOM");
                if (!t.no_guest_fixture) append([]const u8, b, &src, "tests/guest_test.c");
                src.appendSlice(b.allocator, &.{ "src/platform/gport.c", "src/platform/guest.c" }) catch @panic("OOM");
                if (inheritsTablePs2Ram(t.name)) if (vars.ps2ram_src) |path| append([]const u8, b, &src, path);
            } else if (std.mem.eql(u8, source, "@decimal")) {
                src.appendSlice(b.allocator, &.{ "src/platform/ee_decimal.c", "src/platform/ee_dtoa.c", "src/platform/ps2_fpbit.c", "src/platform/kernel_heap.c", "src/platform/ee_heap.c", "tests/ee_heap_kernel.c" }) catch @panic("OOM");
            } else if (std.mem.eql(u8, source, "@crt")) {
                src.appendSlice(b.allocator, &.{ "src/platform/ee_bytes.c", "src/platform/ps2_libc.c", "src/platform/ee_printf.c", "src/platform/ee_scanf.c", "src/platform/ee_strtod.c", "src/platform/ee_libm.c", "src/platform/ee_float.c", "overrides/00440bb8.c", "src/platform/ee_decimal.c", "src/platform/ee_dtoa.c", "src/platform/ps2_fpbit.c", "src/platform/kernel_heap.c", "src/platform/ee_heap.c", "tests/ee_heap_kernel.c" }) catch @panic("OOM");
            } else if (std.mem.eql(u8, source, "@stdio")) {
                src.appendSlice(b.allocator, &.{ "src/platform/gport.c", "src/platform/guest.c", vars.os_src }) catch @panic("OOM");
            } else if (std.mem.eql(u8, source, "@png")) {
                src.appendSlice(b.allocator, vars.png_sources) catch @panic("OOM");
            } else if (std.mem.eql(u8, source, "@modgame")) {
                src.appendSlice(b.allocator, (fixture orelse @panic("missing mod hook fixture")).paths) catch @panic("OOM");
            } else {
                append([]const u8, b, &src, if (std.mem.eql(u8, source, "@os")) vars.os_src else source);
            }
        }
        if (t.mod_input_core) src.appendSlice(b.allocator, &.{ "src/platform/mods/mods.c", "src/platform/mods/package_registry.c", "src/platform/mods/package_memory.c", "src/platform/mods/mod_setting_store.c", "src/platform/mods/lifecycle.c", "src/platform/mods/mod_artifacts.c", "src/platform/mods/compendium.c", "src/platform/mods/file_index.c", "third_party/yxml/yxml.c", "src/platform/files/vfs.c" }) catch @panic("OOM");
        if (t.guest) src.appendSlice(b.allocator, &.{ "tests/guest_test.c", "src/platform/gport.c", "src/platform/guest.c" }) catch @panic("OOM");
        if (t.os or t.guest) append([]const u8, b, &src, vars.os_src);
        var definitions: std.ArrayList([]const u8) = .empty;
        if (t.defines) definitions.appendSlice(b.allocator, vars.port_defines) catch @panic("OOM");
        definitions.appendSlice(b.allocator, t.extra_defines) catch @panic("OOM");
        var includes: std.ArrayList([]const u8) = .empty;
        for (t.includes) |path| append([]const u8, b, &includes, b.pathJoin(&.{ graph.root, path }));
        if (t.decomp_include) append([]const u8, b, &includes, b.pathJoin(&.{ vars.decomp, "include" }));
        var props: std.ArrayList(core.SourceProps) = .empty;
        if (t.battle_zig) for (t.sources) |source| {
            if (std.mem.endsWith(u8, source, ".zig")) append(core.SourceProps, b, &props, .{
                .path = source,
                .includes = b.allocator.dupe([]const u8, &.{b.pathJoin(&.{ graph.root, "src/platform" })}) catch @panic("OOM"),
                .compile_options = if (t.gc_sections and t.gc_source != null and std.mem.eql(u8, source, t.gc_source.?)) &.{ "-ffunction-sections", "-fdata-sections" } else &.{},
            });
        };
        const ordered = t.game_objects or t.printf_abi;
        var link_order: std.ArrayList(core.LinkInput) = .empty;
        if (ordered) {
            append(core.LinkInput, b, &link_order, .{ .source = src.items[0] });
            if (t.game_objects) append(core.LinkInput, b, &link_order, .{ .objects = .tables });
            if (t.printf_abi) append(core.LinkInput, b, &link_order, .{ .objects = .{ .library = printf_calls } });
            for (src.items[1..]) |source| append(core.LinkInput, b, &link_order, .{ .source = source });
            if (t.game_objects) {
                append(core.LinkInput, b, &link_order, .{ .objects = .p4decomp });
                append(core.LinkInput, b, &link_order, .{ .objects = .p4port });
                append(core.LinkInput, b, &link_order, .{ .objects = .p4portee });
            }
        }
        const spec = core.ExecutableSpec{
            .name = t.exe,
            .dir = "tests/",
            .sources = if (ordered) &.{} else src.items,
            .generated = if (fixture) |f| b.allocator.dupe(core.GeneratedFiles, &.{f}) catch @panic("OOM") else &.{},
            .source_props = props.items,
            .includes = includes.items,
            .link_order = link_order.items,
            .objects = if (t.ee_libm and ee_libm_obj != null) b.allocator.dupe(core.ObjectsRef, &.{.{ .library = ee_libm_obj.? }}) catch @panic("OOM") else &.{},
            .compile_options = if (t.game_objects) &.{"-fno-builtin"} else if (t.gc_sections and t.gc_source == null) &.{ "-ffunction-sections", "-fdata-sections" } else &.{},
            .link_options = if (t.gc_sections) &.{"-Wl,--gc-sections"} else &.{},
            .defines = definitions.items,
            .libs = if (t.game_objects) &.{ .m, .sdl3, .ffmpeg } else if (t.math and t.sdl) &.{ .m, .sdl3 } else if (t.math) &.{.m} else if (t.sdl) &.{.sdl3} else &.{},
            .windows_libs = if (t.windows_ntdll) &.{"-lntdll"} else &.{},
            .ps2_exe = t.ps2,
        };
        const artifact: ?core.Artifact = if (built_targets.getEntry(t.exe)) |entry| entry.value_ptr.* else first: {
            const built = core.addExecutable(graph, .i386, spec);
            const custom_override = std.mem.eql(u8, t.exe, "qsort_test") or std.mem.eql(u8, t.exe, "lockstep_restore_witness_test") or std.mem.eql(u8, t.exe, "matrix_override_test");
            if (graph.link64 and graph.port64 and !custom_override and !(t.ee_libm and ee_libm_obj == null)) {
                if (!(graph.port64_arch == .aarch64 and std.mem.eql(u8, t.name, "fpu_state"))) {
                    if (core.hostTwin(graph, spec, .m64)) |twin| x64.dependOn(twin.step);
                }
                if (graph.link64w) {
                    if (core.hostTwin(graph, spec, .m64w)) |twin| x64w.dependOn(twin.step);
                }
            }
            built_targets.put(b.allocator, t.exe, built) catch @panic("OOM");
            append(gates.ExternalExe, b, &exes, .{ .name = t.exe });
            break :first built;
        };
        const timeout = if (t.timeout) |seconds| if (graph.port64_arch == .aarch64) seconds * 4 else seconds else default_timeout;
        const named = b.step(b.fmt("native-a-test-{s}", .{t.name}), b.fmt("Run native fixture {s}", .{t.name}));
        const labels: []const []const u8 = if (t.host and t.gl) &.{ "host", "gl" } else if (t.host) &.{"host"} else if (t.gl) &.{"gl"} else &.{};
        if (selected(selection, t.name, labels)) nonserial.dependOn(named);
        if (t.host) host_label.dependOn(named);
        if (t.gl) gl_label.dependOn(named);
        const second = if (t.second_target) |name| graph.artifacts.get(name) else null;
        if (artifact != null and (t.second_target == null or second != null)) {
        const built = artifact.?;
        const save = b.pathJoin(&.{ graph.build_dir, "save", t.name });
        const cwd = b.pathJoin(&.{ graph.build_dir, "tests" });
        const lockdir = b.pathJoin(&.{ graph.build_dir, "locks" });
        const result = b.pathJoin(&.{ graph.build_dir, "gates", "native-a", b.fmt("{s}.json", .{t.name}) });
        const run = b.addSystemCommand(&.{ graph.python, b.pathJoin(&.{ graph.root, "zigbuild/native_tests_a/prepare.py" }), save, cwd, graph.python, b.pathJoin(&.{ graph.root, "tools/zig_runtime_gates.py" }), "run", "--name", t.name, "--timeout", b.fmt("{d}", .{timeout}), "--cwd", cwd, "--lock-dir", lockdir, "--result", result, "--env", b.fmt("P4PC_SAVE_DIR={s}", .{save}), "--env", "P4PC_CONFIG=none", "--env", "P4PC_INPUT=script", "--env", "P4PC_MODS=0", "--env", "P4PC_PACKS=0", "--env", "P4PC_BUNDLED_MOD_DIR=" });
        addRetail(b, run, retail);
        if (t.timeout != null and graph.port64_arch == .aarch64) run.addArgs(&.{ "--env", b.fmt("P4PC_GATE_TIMEOUT={d}", .{timeout}) });
        if (t.skip) |code| run.addArgs(&.{ "--skip-code", b.fmt("{d}", .{code}) });
        if (t.admission_bounds) run.addArgs(&.{ "--env", "P4PC_TABLE_ADMISSION_BOUNDS_CASES=1" });
        for (t.env) |item| run.addArgs(&.{ "--env", item });
        for (t.cmd_env) |item| run.addArgs(&.{ "--cmd-env", item });
        if (t.gl) {
            run.addArgs(&.{ "--label", "gl" });
            if (software_gl) run.addArg("--software-gl");
        }
        run.addArg("--");
        if (t.script) |script| {
            run.addArg(graph.python);
            run.addFileArg2(b.path(script), .{ .make_absolute = true });
            run.addArgs(t.before_exe);
        }
        if (!t.no_emulator and t.cmd_env.len == 0) run.addArgs(core.emulator(graph, .i386));
        run.addArg(built.path);
        if (t.driver_end) run.addArg("--driver-end");
        if (t.arg_bin) |path| run.addArg(b.pathJoin(&.{ graph.build_dir, path }));
        run.addArgs(t.args);
        if (second) |other| {
            run.addArgs(core.emulator(graph, .i386));
            run.addArg(other.path);
            run.addArg("--import-end");
            run.addFileInput(other.lazy);
            run.step.dependOn(other.step);
        }
        run.addFileInput(built.lazy);
        run.step.dependOn(built.step);
        run.addFileInput(b.path("zigbuild/native_tests_a/prepare.py"));
        run.addFileInput(b.path("tools/zig_runtime_gates.py"));
        run.has_side_effects = true;
        named.dependOn(&run.step);
        } else {
            const unavailable = b.addFail(b.fmt("{s}: required native target is unavailable (base: {s}; secondary: {s})", .{ t.name, core.available(graph, .i386) orelse "available", t.second_target orelse "none" }));
            named.dependOn(&unavailable.step);
        }
        var argv: std.ArrayList(gates.Arg) = .empty;
        if (t.script) |script| {
            append(gates.Arg, b, &argv, .python);
            append(gates.Arg, b, &argv, .{ .src = .{ .path = script } });
            for (t.before_exe) |word| append(gates.Arg, b, &argv, .{ .lit = word });
            if (!t.no_emulator) for (core.emulator(graph, .i386)) |word| append(gates.Arg, b, &argv, .{ .lit = word });
            append(gates.Arg, b, &argv, .{ .target = t.exe });
            if (t.driver_end) append(gates.Arg, b, &argv, .{ .lit = "--driver-end" });
        } else if (t.cmd_env.len != 0) {
            append(gates.Arg, b, &argv, .cmake);
            append(gates.Arg, b, &argv, .{ .lit = "-E" });
            append(gates.Arg, b, &argv, .{ .lit = "env" });
            for (t.cmd_env) |item| append(gates.Arg, b, &argv, .{ .lit = item });
            append(gates.Arg, b, &argv, .{ .target = t.exe });
        } else {
            append(gates.Arg, b, &argv, .{ .bare = t.exe });
        }
        if (t.arg_bin) |path| append(gates.Arg, b, &argv, .{ .bin = path });
        if (t.second_target) |other| {
            for (core.emulator(graph, .i386)) |word| append(gates.Arg, b, &argv, .{ .lit = word });
            append(gates.Arg, b, &argv, .{ .target = other });
            append(gates.Arg, b, &argv, .{ .lit = "--import-end" });
        }
        for (t.args) |arg| append(gates.Arg, b, &argv, .{ .lit = arg });
        var env: std.ArrayList(gates.Env) = .empty;
        append(gates.Env, b, &env, .{ .key = "P4PC_SAVE_DIR", .value = .{ .bin = b.fmt("save/{s}", .{t.name}) } });
        append(gates.Env, b, &env, .{ .key = "P4PC_CONFIG", .value = .{ .lit = "none" } });
        append(gates.Env, b, &env, .{ .key = "P4PC_INPUT", .value = .{ .lit = "script" } });
        append(gates.Env, b, &env, .{ .key = "P4PC_MODS", .value = .{ .lit = "0" } });
        append(gates.Env, b, &env, .{ .key = "P4PC_PACKS", .value = .{ .lit = "0" } });
        append(gates.Env, b, &env, .{ .key = "P4PC_BUNDLED_MOD_DIR", .value = .{ .lit = "" } });
        for (t.env) |item| {
            const eq = std.mem.indexOfScalar(u8, item, '=') orelse @panic("invalid fixture environment");
            append(gates.Env, b, &env, .{ .key = item[0..eq], .value = .{ .lit = item[eq + 1 ..] } });
        }
        if (t.admission_bounds) append(gates.Env, b, &env, .{ .key = "P4PC_TABLE_ADMISSION_BOUNDS_CASES", .value = .{ .lit = "1" } });
        if (t.timeout != null and graph.port64_arch == .aarch64) append(gates.Env, b, &env, .{ .key = "P4PC_GATE_TIMEOUT", .value = .{ .lit = b.fmt("{d}", .{timeout}) } });
        append(gates.HostCase, b, &cases, .{ .name = t.name, .argv = argv.items, .env = env.items, .labels = labels, .timeout = if (t.timeout != null) timeout else null, .skip = t.skip });
        append([]const u8, b, &names, t.name);
    }
    const game_cases = [_]struct { name: []const u8, script: []const u8, timeout: u32 }{
        .{ .name = "settings_native", .script = "tests/settings_native_gate.sh", .timeout = 600 },
        .{ .name = "settings_pages", .script = "tests/settings_pages_gate.sh", .timeout = 600 },
        .{ .name = "settings_input", .script = "tests/settings_input_gate.sh", .timeout = 1200 },
    };
    for (game_cases) |g| {
        for ([_]bool{ false, true }) |is64| {
            const target_name: []const u8 = if (is64) "p4pc64" else "p4pc";
            const target = graph.artifacts.get(target_name);
            // 000-registration clones only if TARGET p4pc64 exists.
            if (is64 and target == null) continue;
            const name = if (is64) b.fmt("{s}64", .{g.name}) else g.name;
            const named = b.step(b.fmt("native-a-test-{s}", .{name}), b.fmt("Run {s}", .{name}));
            if (selected(selection, name, if (is64) &.{ "gl", "port64", "scenes64" } else &.{ "gl", "scenes" })) nonserial.dependOn(named);
            gl_label.dependOn(named);
            append([]const u8, b, &names, name);
            if (target) |game| {
                // 240 copies the original's ENVIRONMENT onto its auto clone,
                // including the original's save path.
                const save = b.pathJoin(&.{ graph.build_dir, "save", g.name });
                const cwd = b.pathJoin(&.{ graph.build_dir, "tests" });
                const result = b.pathJoin(&.{ graph.build_dir, "gates", "native-a", b.fmt("{s}.json", .{name}) });
                const timeout = if (graph.port64_arch == .aarch64) g.timeout * 4 else g.timeout;
                const run = b.addSystemCommand(&.{ graph.python, b.pathJoin(&.{ graph.root, "zigbuild/native_tests_a/prepare.py" }), save, cwd, graph.python, b.pathJoin(&.{ graph.root, "tools/zig_runtime_gates.py" }), "run", "--name", name, "--timeout", b.fmt("{d}", .{timeout}), "--cwd", cwd, "--lock-dir", b.pathJoin(&.{ graph.build_dir, "locks" }), "--result", result, "--skip-code", "77", "--label", "gl", "--label", if (is64) "scenes64" else "scenes", "--env", b.fmt("P4PC_SAVE_DIR={s}", .{save}), "--env", "P4PC_CONFIG=none", "--env", "P4PC_INPUT=script", "--env", "P4PC_MODS=0", "--env", "P4PC_PACKS=0", "--env", "P4PC_BUNDLED_MOD_DIR=", "--env", "P4PC_VIRTUAL_CLOCK=1" });
                if (graph.port64_arch == .aarch64) run.addArgs(&.{ "--env", b.fmt("P4PC_GATE_TIMEOUT={d}", .{timeout}) });
                addRetail(b, run, retail);
                if (software_gl) run.addArg("--software-gl");
                run.addArgs(&.{ "--", "sh" });
                run.addFileArg2(b.path(g.script), .{ .make_absolute = true });
                run.addArg(game.path);
                run.addFileInput(game.lazy);
                run.step.dependOn(game.step);
                run.has_side_effects = true;
                named.dependOn(&run.step);
            } else {
                named.dependOn(&b.addFail(b.fmt("{s}: target {s} is unavailable", .{ name, target_name })).step);
            }
        }
    }
    if (witness.enabled) {
        // The graph supplies the real tools/mods fixture with 014's
        // recovery sources and definitions attached before it is built.
        for ([_]struct { model: core.Model, suffix: []const u8 }{
            .{ .model = .i386, .suffix = "" },
            .{ .model = .m64, .suffix = "64" },
            .{ .model = .m64w, .suffix = "64w" },
        }) |profile| {
            const fixture = core.modLuauTests(graph, profile.model, true) orelse {
                if (profile.model == .i386) {
                    const missing = b.step("native-a-test-mod_loader_recovery", "Run the isolated VM recovery fixture");
                    missing.dependOn(&b.addFail("MOD_WITNESS requires the desktop Luau game binding and native VM fixture").step);
                    nonserial.dependOn(missing);
                    append([]const u8, b, &names, "mod_loader_recovery");
                }
                continue;
            };
            const name = b.fmt("mod_loader_recovery{s}", .{profile.suffix});
            const named = b.step(b.fmt("native-a-test-{s}", .{name}), b.fmt("Run {s}", .{name}));
            const labels: []const []const u8 = switch (profile.model) {
                .i386 => &.{"mod-witness"},
                .m64 => &.{ "mod-witness", "port64" },
                .m64w => &.{ "mod-witness", "port64win" },
            };
            if (selected(selection, name, labels)) nonserial.dependOn(named);
            append([]const u8, b, &names, name);
            const save = b.pathJoin(&.{ graph.build_dir, "save", name });
            const cwd = b.pathJoin(&.{ graph.build_dir, "tests" });
            const result = b.pathJoin(&.{ graph.build_dir, "gates", "native-a", b.fmt("{s}.json", .{name}) });
            const timeout: u32 = if (graph.port64_arch == .aarch64) 720 else 180;
            const run = b.addSystemCommand(&.{ graph.python, b.pathJoin(&.{ graph.root, "zigbuild/native_tests_a/prepare.py" }), save, cwd, graph.python, b.pathJoin(&.{ graph.root, "tools/zig_runtime_gates.py" }), "run", "--name", name, "--timeout", b.fmt("{d}", .{timeout}), "--cwd", cwd, "--lock-dir", b.pathJoin(&.{ graph.build_dir, "locks" }), "--result", result, "--label", "mod-witness", "--env", b.fmt("P4PC_SAVE_DIR={s}", .{save}), "--env", "P4PC_CONFIG=none", "--env", "P4PC_INPUT=script", "--env", "P4PC_MODS=0", "--env", "P4PC_PACKS=0", "--env", "P4PC_BUNDLED_MOD_DIR=", "--env", "SDL_AUDIODRIVER=dummy" });
            if (graph.port64_arch == .aarch64) run.addArgs(&.{ "--env", b.fmt("P4PC_GATE_TIMEOUT={d}", .{timeout}) });
            if (profile.model == .m64w) {
                run.addArgs(&.{ "--label", "port64win", "--env", "WINEDEBUG=-all", "--env", b.fmt("WINEPREFIX={s}", .{b.pathJoin(&.{ graph.build_dir, "tests/wine64prefix" })}) });
                run.step.dependOn(witness.wine_ready orelse gates.fixture(b, witness.gate_prefix, "wine64_ready"));
                if (witness.wine.len == 0) {
                    const missing_wine = b.addFail("P4_MOD_WINE: wine64/wine is required for the 64w Luau mod tests");
                    nonserial.dependOn(&missing_wine.step);
                    run.step.dependOn(&missing_wine.step);
                }
            } else if (profile.model == .m64) run.addArgs(&.{ "--label", "port64" });
            addRetail(b, run, retail);
            if (software_gl) run.addArg("--software-gl");
            run.addArgs(&.{ "--", graph.python });
            run.addFileArg2(b.path("tests/mod_loader_recovery_test.py"), .{ .make_absolute = true });
            run.addArgs(if (profile.model == .m64w) witness.wine else core.emulator(graph, profile.model));
            run.addArgs(&.{ fixture.luau_test.path, "--driver-end" });
            run.addFileInput(fixture.luau_test.lazy);
            run.step.dependOn(fixture.luau_test.step);
            run.has_side_effects = true;
            named.dependOn(&run.step);
        }
        const specs = [_]struct { stem: []const u8, script: []const u8, out: []const u8, timeout: u32, practice: bool }{
            .{ .stem = "mod_example_game", .script = "tests/mod_example_game_test.py", .out = "mod-example-witness", .timeout = 4800, .practice = false },
            .{ .stem = "mod_practice_game", .script = "tests/mod_practice_game_test.py", .out = "mod-practice-witness", .timeout = 7200, .practice = true },
        };
        for (specs) |w| {
            for ([_]struct { target: []const u8, suffix: []const u8, model: core.Model }{
                .{ .target = "p4pc", .suffix = "", .model = .i386 },
                .{ .target = "p4pc64", .suffix = "64", .model = .m64 },
                .{ .target = "p4pc64w", .suffix = "64w", .model = .m64w },
            }) |profile| {
                if (w.practice and profile.model == .i386) continue;
                const game = graph.artifacts.get(profile.target) orelse continue;
                const name = b.fmt("{s}{s}", .{ w.stem, profile.suffix });
                const named = b.step(b.fmt("native-a-test-{s}", .{name}), b.fmt("Run {s}", .{name}));
                if (selected(selection, name, &.{"mod-witness"})) nonserial.dependOn(named);
                append([]const u8, b, &names, name);
                const save = b.pathJoin(&.{ graph.build_dir, "save", name });
                const cwd = b.pathJoin(&.{ graph.build_dir, "tests" });
                const output_dir = b.pathJoin(&.{ graph.build_dir, b.fmt("{s}{s}", .{ w.out, profile.suffix }) });
                const result = b.pathJoin(&.{ graph.build_dir, "gates", "native-a", b.fmt("{s}.json", .{name}) });
                const timeout = if (graph.port64_arch == .aarch64) w.timeout * 4 else w.timeout;
                const run = b.addSystemCommand(&.{ graph.python, b.pathJoin(&.{ graph.root, "zigbuild/native_tests_a/prepare.py" }), save, cwd, graph.python, b.pathJoin(&.{ graph.root, "tools/zig_runtime_gates.py" }), "run", "--name", name, "--timeout", b.fmt("{d}", .{timeout}), "--cwd", cwd, "--lock-dir", b.pathJoin(&.{ graph.build_dir, "locks" }), "--result", result, "--skip-code", "77", "--label", "mod-witness", "--env", b.fmt("P4PC_SAVE_DIR={s}", .{save}), "--env", "P4PC_CONFIG=none", "--env", "P4PC_INPUT=script", "--env", if (w.practice) "P4PC_MODS=0" else "P4PC_MODS=1", "--env", "P4PC_PACKS=0", "--env", "P4PC_MOD_WITNESS=1", "--env", "P4PC_BUNDLED_MOD_DIR=" });
                if (graph.port64_arch == .aarch64) run.addArgs(&.{ "--env", b.fmt("P4PC_GATE_TIMEOUT={d}", .{timeout}) });
                if (profile.model == .m64w) {
                    run.addArgs(&.{ "--env", "WINEDEBUG=-all", "--env", b.fmt("WINEPREFIX={s}", .{b.pathJoin(&.{ graph.build_dir, "tests/wine64prefix" })}) });
                    if (witness.wine.len == 0) run.step.dependOn(&b.addFail("P4_MOD_WINE: wine64/wine is required for the 64w Luau mod tests").step);
                }
                addRetail(b, run, retail);
                if (software_gl) run.addArg("--software-gl");
                run.addArgs(&.{ "--", graph.python });
                run.addFileArg2(b.path(w.script), .{ .make_absolute = true });
                run.addArgs(&.{ "--out", output_dir, "--driver" });
                run.addArgs(if (profile.model == .m64w) witness.wine else core.emulator(graph, profile.model));
                run.addArg(game.path);
                run.addFileInput(game.lazy);
                run.step.dependOn(game.step);
                run.has_side_effects = true;
                named.dependOn(&run.step);
            }
        }
    }
    return .{ .external_test_names = names.items, .external_host_cases = cases.items, .external_executables = exes.items };
}
