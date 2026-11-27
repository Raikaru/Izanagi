const std = @import("std");

// The graph-specific replacements are real build steps supplied by the graph
// owner, not substitutes for the legacy CMake test scripts. They must implement
// their full graph assertions, the named timeout, host-tooling serialization,
// test working directory and the same safe environment as the Python fixtures.
// host_adapter_graph is required exactly for a native Linux build with C++.
pub const Options = struct {
    python: []const u8,
    clang: []const u8,
    nm: []const u8,
    objdump: []const u8,
    // Existing or creatable test binary directory; use an absolute path.
    working_dir: []const u8,
    // Absolute path shared by every concurrent Zig tooling test in this build.
    lock_file: []const u8,
    // These correspond independently to the three optional private fragments.
    private_tools: bool,
    private_orchestrator: bool,
    private_platform: bool,
    native_linux_with_cxx: bool,
    cmake_sources_replacement: *std.Build.Step,
    host_adapter_graph_replacement: ?*std.Build.Step,
    // The suite depends on this separate serial phase. `all` passed to
    // register must contain every nonserial runtime and tooling gate; it must
    // not depend on this phase. The suite instance waits for `all`.
    remote_check_host_serial: *std.Build.Step,
    // CTest's default when no per-test TIMEOUT is specified.
    default_timeout_seconds: u32 = 1500,
};

const Group = enum { public, private_tools, private_orchestrator, private_platform };
const Test = struct {
    name: []const u8,
    file: []const u8,
    timeout: u32 = 0,
    tools: bool = false,
    host: bool = false,
    input_script: bool = false,
    group: Group = .public,
    // Private CMake fragments removed wholesale by the public exporter.
    fragment: ?[]const u8 = null,
    crt: bool = false,
};

const tests = [_]Test{
    .{ .name = "doc_links", .file = "../tools/check_doc_links.py" },
    .{ .name = "ref_guard", .file = "ref_guard_test.py" },
    .{ .name = "scale_diff", .file = "scale_diff_test.py" },
    .{ .name = "title_edges", .file = "title_edges_test.py" },
    .{ .name = "cutin_edges", .file = "cutin_edges_test.py" },
    .{ .name = "disc_contract", .file = "disc_contract_test.py" },
    .{ .name = "unit64_auto_init", .file = "unit64_auto_init_test.py" },
    .{ .name = "field_entry_witness", .file = "field_entry_test.py" },
    .{ .name = "event_scene_capture", .file = "event_scene_capture_test.py" },
    .{ .name = "package_runtime_provenance", .file = "package_runtime_provenance_test.py" },
    .{ .name = "windows_tool_lint", .file = "windows_tool_lint_test.py", .tools = true, .timeout = 60 },
    .{ .name = "profile_report", .file = "profile_report_test.py" },
    .{ .name = "crt_guard", .file = "crt_guard_test.py", .crt = true },
    .{ .name = "printf_rewrite", .file = "printf_rewrite_test.py" },
    .{ .name = "fn_word_rewrite", .file = "fn_word_rewrite_test.py" },
    .{ .name = "null_offset_rewrite", .file = "null_offset_rewrite_test.py" },
    .{ .name = "zig_abi", .file = "zig_abi_test.py" },
    .{ .name = "zig_tool_graph", .file = "zig_tool_graph_test.py", .timeout = 600 },
    .{ .name = "variadic_call_rewrite", .file = "variadic_call_rewrite_test.py" },
    .{ .name = "asm_worklist", .file = "asm_worklist_test.py" },
    .{ .name = "mipsdiff_sigs", .file = "mipsdiff_sigs_test.py" },
    .{ .name = "gs_audit", .file = "gs_audit_test.py", .group = .private_tools },
    .{ .name = "script_input", .file = "script_input_test.py", .group = .private_tools },
    .{ .name = "local_priority", .file = "local_priority_test.py", .group = .private_tools },
    .{ .name = "retail_lease", .file = "retail_lease_test.py", .group = .private_tools },
    .{ .name = "pcsx2_save_queue", .file = "pcsx2_save_queue_test.py", .group = .private_tools },
    .{ .name = "boot_timing", .file = "boot_timing_test.py", .group = .private_tools },
    .{ .name = "disc_timing", .file = "disc_timing_test.py", .group = .private_tools },
    .{ .name = "logic64_init", .file = "logic64_init_test.py", .group = .private_tools },
    .{ .name = "gate_artifacts", .file = "gate_artifacts_test.py", .group = .private_tools },
    .{ .name = "collection_save_staging", .file = "collection_save_test.py", .group = .private_tools },
    .{ .name = "land_game_split", .file = "land_game_split_test.py", .group = .private_tools },
    .{ .name = "land_game_rotation", .file = "land_game_rotation_test.py", .group = .private_tools },
    .{ .name = "phone_stage", .file = "phone_stage_test.py", .group = .private_tools },
    .{ .name = "phone_game_runner", .file = "phone_game_runner_test.py", .group = .private_tools },
    .{ .name = "land_host_telemetry", .file = "land_telemetry_test.py", .group = .private_tools },
    .{ .name = "land_gate_regression", .file = "land_gate_regression_test.py", .group = .private_tools },
    .{ .name = "fast_scenarios", .file = "fast_scenarios_test.py", .group = .private_tools },
    .{ .name = "remote_check_host", .file = "remote_check_test.py", .group = .private_tools },
    .{ .name = "local_work", .file = "local_work_test.py", .group = .private_tools },
    .{ .name = "workspace_usage", .file = "workspace_usage_test.py", .group = .private_tools },
    .{ .name = "land_watchdog", .file = "land_watchdog_test.py", .group = .private_tools },
    .{ .name = "land_policy", .file = "land_policy_test.py", .group = .private_tools },
    .{ .name = "land_queue_backup", .file = "land_queue_backup_test.py", .group = .private_tools },
    .{ .name = "land_rotation", .file = "land_rotation_test.py", .group = .private_tools },
    .{ .name = "local_boot_scratch", .file = "local_boot_scratch_test.py", .group = .private_tools },
    .{ .name = "local_storage", .file = "local_storage_test.py", .group = .private_tools },
    .{ .name = "ref_guard_private", .file = "ref_guard_private_test.py", .group = .private_tools },
    .{ .name = "local_scenes_manifest", .file = "local_scenes_manifest_test.py", .group = .private_tools },
    .{ .name = "land_runner_fresh", .file = "land_runner_fresh_test.py", .group = .private_tools },
    .{ .name = "cmake_flow", .file = "cmake_flow_test.py", .group = .private_tools },
    .{ .name = "polish_capture", .file = "polish_capture_test.py", .group = .private_tools },
    .{ .name = "retail_refs", .file = "retail_refs_test.py", .group = .private_tools },
    .{ .name = "capture_archive", .file = "capture_archive_test.py", .fragment = "251-captures.cmake" },
    .{ .name = "remote_output_retention", .file = "remote_work_test.py", .fragment = "251-captures.cmake" },
    .{ .name = "battle_sim_coordinator", .file = "test_battle_sim.py", .timeout = 300 },
    .{ .name = "battle_sim_compare", .file = "battle_sim_compare_test.py" },
    .{ .name = "battle_tas", .file = "battle_tas_test.py", .timeout = 60 },
    .{ .name = "land_model", .file = "land_model_test.py", .timeout = 60, .group = .private_orchestrator },
    .{ .name = "land_executor", .file = "land_executor_test.py", .timeout = 60, .group = .private_orchestrator },
    .{ .name = "land_runtime", .file = "land_runtime_test.py", .timeout = 60, .group = .private_orchestrator },
    .{ .name = "land_host", .file = "land_host_test.py", .timeout = 60, .group = .private_orchestrator },
    .{ .name = "land_bootstrap", .file = "land_bootstrap_test.py", .timeout = 60, .group = .private_orchestrator },
    .{ .name = "land_verifier", .file = "land_verifier_test.py", .timeout = 60, .group = .private_orchestrator },
    .{ .name = "land_phone", .file = "land_phone_test.py", .timeout = 60, .group = .private_orchestrator },
    .{ .name = "land_github", .file = "land_github_test.py", .timeout = 60, .group = .private_orchestrator },
    .{ .name = "ci_publish_images", .file = "ci_publish_images_test.py", .timeout = 60, .tools = true, .group = .private_orchestrator },
    .{ .name = "lockstep_native_job", .file = "lockstep_native_job_test.py", .timeout = 90, .tools = true, .group = .private_orchestrator },
    .{ .name = "land_client_wait", .file = "land_client_wait_test.py", .timeout = 300, .tools = true, .fragment = "253-client-wait.cmake" },
    .{ .name = "land_client_wait_api", .file = "land_client_wait_api_test.py", .timeout = 300, .tools = true, .fragment = "253-client-wait.cmake" },
    .{ .name = "land_client_wait_controls", .file = "land_client_wait_controls_test.py", .timeout = 300, .tools = true, .fragment = "253-client-wait.cmake" },
    .{ .name = "land_client_wait_count", .file = "land_client_wait_count_test.py", .timeout = 300, .tools = true, .fragment = "253-client-wait.cmake" },
    .{ .name = "land_precheck_runtime", .file = "land_precheck_runtime_test.py", .timeout = 120, .tools = true, .fragment = "253-client-wait.cmake" },
    .{ .name = "land_precheck_queue", .file = "land_precheck_test.py", .timeout = 120, .tools = true, .fragment = "253-client-wait.cmake" },
    .{ .name = "land_attribution", .file = "land_attribution_test.py", .timeout = 60, .tools = true, .fragment = "253-client-wait.cmake" },
    .{ .name = "preflight", .file = "preflight_test.py", .timeout = 120, .tools = true, .fragment = "253-client-wait.cmake" },
    .{ .name = "derived_rebase", .file = "derived_rebase_test.py", .timeout = 120, .tools = true, .fragment = "254-derived-artifacts.cmake" },
    .{ .name = "land_submodule", .file = "land_submodule_test.py", .timeout = 120, .tools = true, .fragment = "254-derived-artifacts.cmake" },
    .{ .name = "build_cache", .file = "build_cache_test.py", .timeout = 180, .tools = true },
    .{ .name = "land_cold", .file = "land_cold_test.py", .timeout = 180, .tools = true, .fragment = "258-cold-residency.cmake" },
    .{ .name = "nightly_cold", .file = "nightly_cold_test.py", .timeout = 180, .tools = true, .fragment = "258-cold-residency.cmake" },
    .{ .name = "c_budget", .file = "c_budget_test.py", .timeout = 180, .tools = true, .fragment = "258-cold-residency.cmake" },
    .{ .name = "wsl_compact", .file = "wsl_compact_test.py", .timeout = 120, .tools = true, .fragment = "258-cold-residency.cmake" },
    .{ .name = "retail_stage", .file = "retail_stage_test.py", .timeout = 120, .tools = true, .fragment = "258-cold-residency.cmake" },
    .{ .name = "nightly_compact_ram", .file = "nightly_compact_ram_test.py", .timeout = 180, .tools = true, .fragment = "258-cold-residency.cmake" },
    .{ .name = "nightly_gates", .file = "nightly_gates_test.py", .timeout = 180, .tools = true, .fragment = "258-cold-residency.cmake" },
    .{ .name = "boot_seams", .file = "boot_seams_test.py" },
    .{ .name = "android_restart_driver", .file = "android_restart_test.py" },
    .{ .name = "land_gs_smoke", .file = "land_gs_smoke_test.py", .group = .private_platform },
    .{ .name = "quarantine", .file = "quarantine_test.py", .group = .private_platform },
    .{ .name = "lockstep_transport", .file = "lockstep_test.py", .group = .private_platform },
    .{ .name = "remote_check_process", .file = "remote_check_process_test.py", .timeout = 60, .fragment = "261-platform-checks.cmake" },
    .{ .name = "remote_check_teardown", .file = "remote_check_teardown_test.py", .timeout = 60, .tools = true, .fragment = "261-platform-checks.cmake" },
    .{ .name = "land_result_cache", .file = "land_result_cache_test.py", .timeout = 180, .tools = true, .fragment = "262-land-result-reuse.cmake" },
    .{ .name = "land_rebase_queue", .file = "land_rebase_queue_test.py", .timeout = 180, .tools = true, .fragment = "263-land-rebase-queue.cmake" },
    .{ .name = "ps2save_export", .file = "ps2save_export_test.py", .timeout = 180, .tools = true },
    .{ .name = "public_export", .file = "public_export_test.py", .timeout = 120, .tools = true, .fragment = "265-public-export.cmake" },
    .{ .name = "mod_guard_timing_contract", .file = "mod_guard_timing_test.py", .timeout = 60, .host = true, .input_script = true, .fragment = "266-mod-guard-timing.cmake" },
};

fn addNamed(b: *std.Build, all: ?*std.Build.Step, port64: *std.Build.Step, tools: *std.Build.Step, host: *std.Build.Step, name: []const u8, step: *std.Build.Step, is_tools: bool, is_host: bool) void {
    const named = b.step(b.fmt("zig-tool-test-{s}", .{name}), b.fmt("Run tooling test {s}", .{name}));
    named.dependOn(step);
    if (all) |group| group.dependOn(named);
    if (is_host) host.dependOn(named) else port64.dependOn(named);
    if (is_tools) tools.dependOn(named);
}

fn addRun(b: *std.Build, options: Options, test_case: Test) *std.Build.Step {
    const run = b.addSystemCommand(&.{options.python});
    run.addFileArg2(b.path("tools/zig_tool_tests.py"), .{ .make_absolute = true });
    run.addArgs(&.{ "--name", test_case.name, "--cwd", options.working_dir, "--lock", options.lock_file, "--timeout" });
    run.addArg(b.fmt("{d}", .{if (test_case.timeout == 0) options.default_timeout_seconds else test_case.timeout}));
    if (test_case.input_script) run.addArg("--script-input");
    run.addArgs(&.{ "--", options.python });
    // absolute: the runner starts the fixture in options.working_dir
    run.addFileArg2(b.path(b.fmt("tests/{s}", .{test_case.file})), .{ .make_absolute = true });
    if (test_case.crt) run.addArgs(&.{ "--clang", options.clang, "--nm", options.nm, "--objdump", options.objdump });
    return &run.step;
}
// Created by the public export overlay, never by the private checkout. This is
// an explicit source-tree mode, not an authentication or security boundary.
fn isPublicExport(b: *std.Build) bool {
    const root = b.root.toString(b.allocator) catch @panic("OOM");
    const marker = b.pathJoin(&.{ root, "zigbuild/public-export-mode.txt" });
    var buffer: [128]u8 = undefined;
    const content = std.Io.Dir.cwd().readFile(b.graph.io, marker, &buffer) catch |err| switch (err) {
        error.FileNotFound => return false,
        else => std.process.fatal("cannot read export mode {s}: {s}", .{ marker, @errorName(err) }),
    };
    if (!std.mem.eql(u8, content, "izanagi-public-export-v1"))
        std.process.fatal("invalid public export mode marker: {s}", .{marker});
    const exporter = b.pathJoin(&.{ root, "tools/public_export.py" });
    std.Io.Dir.cwd().access(b.graph.io, exporter, .{}) catch |err| switch (err) {
        error.FileNotFound => return true,
        else => std.process.fatal("cannot inspect private exporter {s}: {s}", .{ exporter, @errorName(err) }),
    };
    std.process.fatal("public export marker conflicts with private exporter: {s}", .{exporter});
}

fn fragmentAvailable(b: *std.Build, fragment: []const u8, public_export: bool) bool {
    const root = b.root.toString(b.allocator) catch @panic("OOM");
    const path = b.pathJoin(&.{ root, "tests/cmake", fragment });
    std.Io.Dir.cwd().access(b.graph.io, path, .{}) catch |err| switch (err) {
        error.FileNotFound => {
            if (public_export) return false;
            std.process.fatal("missing private test fragment: {s}", .{path});
        },
        else => std.process.fatal("cannot inspect private test fragment {s}: {s}", .{ path, @errorName(err) }),
    };
    return true;
}


pub fn register(b: *std.Build, all: *std.Build.Step, options: Options) void {
    const public_export = isPublicExport(b);
    const private_tools = fragmentAvailable(b, "250-tools-private.cmake", public_export);
    const private_orchestrator = fragmentAvailable(b, "252-orchestrator-private.cmake", public_export);
    const private_platform = fragmentAvailable(b, "260-platform-checks-private.cmake", public_export);
    if (private_tools != options.private_tools or private_orchestrator != options.private_orchestrator or
        private_platform != options.private_platform)
        std.process.fatal("private test fragment detection disagrees with build configuration", .{});
    const port64 = b.step("zig-tool-label-port64", "Run port64-labeled script tooling tests");
    const tools = b.step("zig-tool-label-tools", "Run tools-labeled script tooling tests");
    const host = b.step("zig-tool-label-host", "Run host-labeled script tooling tests");
    for (tests) |test_case| {
        const enabled = switch (test_case.group) {
            .public => true,
            .private_tools => options.private_tools,
            .private_orchestrator => options.private_orchestrator,
            .private_platform => options.private_platform,
        };
        if (!enabled) continue;
        // Only an explicitly classified private fragment may be omitted, and
        // only in an exporter-marked tree. Enabled scripts remain build inputs.
        if (test_case.fragment) |fragment| {
            if (!fragmentAvailable(b, fragment, public_export)) continue;
        }
        const run = addRun(b, options, test_case);
        if (std.mem.eql(u8, test_case.name, "remote_check_host")) {
            // The named instance stays independently selectable. The separate
            // suite instance runs after all nonserial runtime/tool gates.
            addNamed(b, null, port64, tools, host, test_case.name, run, test_case.tools, test_case.host);
            const serial_run = addRun(b, options, test_case);
            serial_run.dependOn(all);
            options.remote_check_host_serial.dependOn(serial_run);
        } else {
            addNamed(b, all, port64, tools, host, test_case.name, run, test_case.tools, test_case.host);
        }
    }
    addNamed(b, all, port64, tools, host, "cmake_sources", options.cmake_sources_replacement, true, false);
    if (options.native_linux_with_cxx) {
        const graph = options.host_adapter_graph_replacement orelse @panic("native Linux C++ builds require the host_adapter_graph Zig replacement");
        addNamed(b, all, port64, tools, host, "host_adapter_graph", graph, true, false);
    }
}
