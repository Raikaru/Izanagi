const std = @import("std");

const license =
    \\MIT License
    \\
    \\Copyright (c) 2019-2026 Roblox Corporation
    \\Copyright (c) 1994-2019 Lua.org, PUC-Rio.
    \\
    \\Permission is hereby granted, free of charge, to any person obtaining a copy of
    \\this software and associated documentation files (the "Software"), to deal in
    \\the Software without restriction, including without limitation the rights to
    \\use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    \\of the Software, and to permit persons to whom the Software is furnished to do
    \\so, subject to the following conditions:
    \\
    \\The above copyright notice and this permission notice shall be included in all
    \\copies or substantial portions of the Software.
    \\
    \\THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    \\IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    \\FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    \\AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    \\LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    \\OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    \\SOFTWARE.
    \\
;

pub fn build(b: *std.Build) void {
    // Host-independent by default: the host's OS and ABI, but the baseline CPU model of its architecture,
    // never the detected native CPU. Zig's native detection can name a CPU its own LLVM then rejects
    // ("unknown target CPU 'skylake_avx512'" on Cascade Lake), and a receipt from one host must mean the
    // same build as on any other. -Dcpu=... or -Dtarget=... still override this.
    const target = b.standardTargetOptions(.{ .default_target = .{ .cpu_model = .baseline } });
    const optimize = b.standardOptimizeOption(.{});
    // The pinned nightly's self-hosted x86_64 backend cannot encode VM SIMD code.
    // This changes the Zig code generator, not Luau execution semantics.
    const use_llvm: ?bool = if (target.result.os.tag == .linux or target.result.cpu.arch == .x86_64) true else null;
    const license_file = b.addWriteFiles().add("LICENSE.txt", license);
    const install_license = b.addInstallFile(license_file, "share/licenses/luau/LICENSE.txt");
    b.getInstallStep().dependOn(&install_license.step);

    // Neither reusable module links libc. The C bridge is a separate opt-in artifact.
    const luau = b.addModule("luau", .{
        .root_source_file = b.path("src/root.zig"),
        .target = target,
        .optimize = optimize,
    });
    const compiler = b.addModule("compiler", .{
        .root_source_file = b.path("src/compiler.zig"),
        .target = target,
        .optimize = optimize,
    });
    const vm_hash = b.createModule(.{
        .root_source_file = b.path("src/vm/string_hash.zig"),
        .target = target,
        .optimize = optimize,
    });
    const exe = b.addExecutable(.{
        .name = "luau",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
            .imports = &.{.{ .name = "luau", .module = luau }},
        }),
    });
    b.installArtifact(exe);
    const run = b.addRunArtifact(exe);
    run.addPassthruArgs();
    b.step("run", "Run the Luau interpreter").dependOn(&run.step);

    const tests = b.addTest(.{ .root_module = luau, .use_llvm = use_llvm });
    const run_tests = b.addRunArtifact(tests);
    b.step("test", "Run the Zig module tests").dependOn(&run_tests.step);
    const fastcall_tests = b.step("fastcall-unit-tests", "Run FASTCALL family unit tests");
    inline for (.{
        .{ "src/fastmath_test.zig", false },
        .{ "src/fastvector_test.zig", false },
        .{ "src/fastmisc_test.zig", true },
    }) |family| {
        const unit = b.addTest(.{
            .use_llvm = use_llvm,
            .root_module = b.createModule(.{
                .root_source_file = b.path(family[0]),
                .target = target,
                .optimize = optimize,
                .link_libc = family[1],
            }),
        });
        fastcall_tests.dependOn(&b.addRunArtifact(unit).step);
    }

    // Script-visible NaN output boundaries (ruling LZ-NaN), hermetic like the FASTCALL unit tests.
    const nan_boundary = b.addTest(.{
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/nan_boundary_test.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });
    b.step("nan-boundary-tests", "Run the script-visible NaN output boundary tests").dependOn(&b.addRunArtifact(nan_boundary).step);

    const abi = b.addLibrary(.{
        .name = "luau-vm",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/root.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    const install_abi = b.addInstallArtifact(abi, .{});

    // This C-only witness deliberately uses an optimized VM irrespective of the
    // default build mode; its headers are the repository's pinned Luau headers.
    const protected_abi = b.addLibrary(.{
        .name = "luau-vm-protected-regression",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/root.zig"),
            .target = target,
            .optimize = .ReleaseSafe,
            .link_libc = true,
        }),
    });
    const protected_test = b.addExecutable(.{
        .name = "protected-c-call-regression",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = .ReleaseSafe,
            .link_libc = true,
        }),
    });
    protected_test.root_module.addCSourceFile(.{
        .file = b.path("tests/protected_c_call_regression.c"),
        .flags = &.{"-std=c11"},
    });
    protected_test.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    protected_test.root_module.linkLibrary(protected_abi);
    const run_protected_test = b.addRunArtifact(protected_test);
    b.step("protected-c-regression", "Run the optimized Zig VM protected-call C ABI regression").dependOn(&run_protected_test.step);

    const gc_recovery_test = b.addExecutable(.{
        .name = "gc_interrupt_recovery",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    gc_recovery_test.root_module.addCSourceFile(.{
        .file = b.path("tests/gc_interrupt_recovery.c"),
        .flags = &.{"-std=c11"},
    });
    gc_recovery_test.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    gc_recovery_test.root_module.linkLibrary(abi);
    const run_gc_recovery_test = b.addRunArtifact(gc_recovery_test);
    b.step("gc-interrupt-regression", "Run GC interrupt and allocator recovery against Zig").dependOn(&run_gc_recovery_test.step);

    const compiler_bridge = b.addObject(.{
        .name = "luau-conformance-compiler",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .root_source_file = b.path("tests/upstream_compiler_bridge.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .imports = &.{ .{ .name = "compiler", .module = compiler }, .{ .name = "vm_hash", .module = vm_hash } },
        }),
    });

    const debugstep_test = b.addExecutable(.{
        .name = "udata_debugstep_fallback",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    debugstep_test.root_module.addCSourceFile(.{
        .file = b.path("tests/udata_debugstep_fallback.c"),
        .flags = &.{"-std=c11"},
    });
    debugstep_test.root_module.addCMacro("LUAU_ZIG_CANDIDATE", "1");
    debugstep_test.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    debugstep_test.root_module.addObject(compiler_bridge);
    debugstep_test.root_module.linkLibrary(abi);
    const run_debugstep_test = b.addRunArtifact(debugstep_test);
    b.step("udata-debugstep-regression", "Run the direct-userdata fallback debugger regression").dependOn(&run_debugstep_test.step);

    const setlocal_test = b.addExecutable(.{
        .name = "debug_setlocal_regression",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    setlocal_test.root_module.addCSourceFile(.{
        .file = b.path("tests/debug_setlocal_regression.c"),
        .flags = &.{"-std=c11"},
    });
    setlocal_test.root_module.addCMacro("LUAU_ZIG_CANDIDATE", "1");
    setlocal_test.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    setlocal_test.root_module.addObject(compiler_bridge);
    setlocal_test.root_module.linkLibrary(abi);
    const run_setlocal_test = b.addRunArtifact(setlocal_test);
    b.step("debug-setlocal-regression", "Run the debugger local-assignment stack regression").dependOn(&run_setlocal_test.step);

    const cage_test = b.addExecutable(.{
        .name = "buffer_cage_regression",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
    });
    cage_test.root_module.addCSourceFile(.{
        .file = b.path("tests/buffer_cage_regression.cpp"),
        .flags = &.{"-std=c++17"},
    });
    cage_test.root_module.addCMacro("LUAU_ZIG_CANDIDATE", "1");
    cage_test.root_module.addCMacro("LUA_API", "extern \"C\"");
    cage_test.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    cage_test.root_module.linkLibrary(abi);
    const run_cage_test = b.addRunArtifact(cage_test);
    b.step("buffer-cage-regression", "Run the buffer cage host-boundary regression").dependOn(&run_cage_test.step);

    // Test adapters link the Zig VM and compiler bridge only. Upstream sources
    // are used as test inputs/headers, never as a candidate engine.
    const script_candidate = b.addExecutable(.{
        .name = "upstream_script_candidate",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
    });
    script_candidate.root_module.addCSourceFile(.{
        .file = b.path("tests/upstream_script_host.cpp"),
        .flags = &.{"-std=c++17"},
    });
    script_candidate.root_module.addCSourceFile(.{
        .file = b.path("tests/upstream-0.744/tests/BufferCage.cpp"),
        .flags = &.{"-std=c++17"},
    });
    script_candidate.root_module.addCMacro("LUAU_ZIG_CANDIDATE", "1");
    script_candidate.root_module.addCMacro("LUA_API", "extern \"C\"");
    script_candidate.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    script_candidate.root_module.addObject(compiler_bridge);
    script_candidate.root_module.linkLibrary(abi);
    const install_script = b.addInstallArtifact(script_candidate, .{});
    b.step("upstream-script-candidate", "Build and install the Zig-only upstream script adapter").dependOn(&install_script.step);

    const fastcall_candidate = b.addExecutable(.{
        .name = "fastcall_candidate",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
    });
    fastcall_candidate.root_module.addCSourceFile(.{
        .file = b.path("tests/fastcall_probe.cpp"),
        .flags = &.{"-std=c++17"},
    });
    fastcall_candidate.root_module.addCMacro("LUAU_ZIG_CANDIDATE", "1");
    fastcall_candidate.root_module.addCMacro("LUA_API", "extern \"C\"");
    fastcall_candidate.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    fastcall_candidate.root_module.addIncludePath(b.path("../third_party/luau/Compiler/include"));
    fastcall_candidate.root_module.addObject(compiler_bridge);
    fastcall_candidate.root_module.linkLibrary(abi);
    const install_fastcall = b.addInstallArtifact(fastcall_candidate, .{});
    b.step("fastcall-candidate", "Build the Zig FASTCALL differential host").dependOn(&install_fastcall.step);

    const python = b.option([]const u8, "python", "Python executable used to generate the upstream API adapter") orelse "python3";
    const generate_api = b.addSystemCommand(&.{python});
    generate_api.addFileArg(b.path("tests/generate_upstream_api.py"));
    const generated_api = generate_api.addOutputFileArg("upstream_api_cases.cpp");
    // The inventory verifies every vendored fixture and manifest, including
    // directory membership; always rerun rather than cache an incomplete set.
    generate_api.has_side_effects = true;
    const api_candidate = b.addExecutable(.{
        .name = "upstream_api_candidate",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
            .link_libcpp = true,
        }),
    });
    api_candidate.root_module.addCSourceFile(.{ .file = generated_api, .flags = &.{"-std=c++17"} });
    api_candidate.root_module.addCMacro("LUAU_ZIG_CANDIDATE", "1");
    api_candidate.root_module.addCMacro("LUA_API", "extern \"C\"");
    api_candidate.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    api_candidate.root_module.addIncludePath(b.path("../third_party/luau/Compiler/include"));
    api_candidate.root_module.addIncludePath(b.path("tests"));
    api_candidate.root_module.addIncludePath(b.path("tests/upstream-0.744/extern"));
    api_candidate.root_module.addObject(compiler_bridge);
    api_candidate.root_module.linkLibrary(abi);
    const install_api = b.addInstallArtifact(api_candidate, .{});
    b.step("upstream-api-candidate", "Generate, build and install the Zig-only upstream API adapter").dependOn(&install_api.step);
    const install_bridge = b.addInstallArtifact(compiler_bridge, .{ .dest_dir = .{ .override = .lib } });
    b.step("conformance-compiler", "Build the test-only compiler bridge object").dependOn(&install_bridge.step);
    const abi_step = b.step("vm-abi", "Build and install the libc-linked VM and standard library C exports");
    abi_step.dependOn(&install_abi.step);
    abi_step.dependOn(&install_license.step);

    const host = b.addLibrary(.{
        .name = "p4pc-luau",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/p4pc.zig"),
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    const install_host = b.addInstallArtifact(host, .{});
    const host_step = b.step("p4pc", "Build and install the libc-linked P4PC C ABI archive");
    host_step.dependOn(&install_host.step);
    host_step.dependOn(&install_license.step);

    // Run the unchanged project script host against the Zig bridge, including
    // its digest, file, heap and safepoint helpers.
    const script_runner = b.addExecutable(.{
        .name = "luau_script_runner",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    script_runner.root_module.addCSourceFiles(.{
        .files = &.{ "../tests/luau_script_runner.c", "../src/platform/content_digest.c" },
        .flags = &.{"-std=c11"},
    });
    script_runner.root_module.addIncludePath(b.path("../include"));
    script_runner.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    script_runner.root_module.linkLibrary(host);
    const install_runner = b.addInstallArtifact(script_runner, .{});
    b.step("script-runner", "Build the project script host against the Zig bridge").dependOn(&install_runner.step);

    const sandbox_host = b.addExecutable(.{
        .name = "luau_sandbox_test",
        .use_llvm = use_llvm,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });
    sandbox_host.root_module.addCSourceFile(.{
        .file = b.path("../tests/luau_sandbox_test.c"),
        .flags = &.{"-std=c11"},
    });
    sandbox_host.root_module.addIncludePath(b.path("../include"));
    sandbox_host.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
    sandbox_host.root_module.linkLibrary(host);
    const install_sandbox_host = b.addInstallArtifact(sandbox_host, .{});
    b.step("sandbox-host", "Build the unchanged project sandbox witness against the Zig bridge").dependOn(&install_sandbox_host.step);

    addModLuauTests(b, target, optimize, use_llvm, host, python, b.step(
        "mod-luau-tests",
        "Build the unchanged mod_luau_test and mod_luau_save_test against the Zig bridge",
    ));

    // These are real C witnesses linked only to the Zig p4pc bridge. The
    // aggregate requires every witness to pass: a platform SKIP (exit 77)
    // remains visible and fails the aggregate as incomplete, never as a pass.
    const witnesses = b.step("sandbox-witness", "Run all six P4PC sandbox witnesses (exit 77 means incomplete)");
    for ([_][]const u8{ "globals", "budget", "memory", "allocator", "heap", "mod" }) |name| {
        const witness = b.addExecutable(.{
            .name = b.fmt("sandbox_witness_{s}_candidate", .{name}),
            .use_llvm = use_llvm,
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
                .link_libc = true,
            }),
        });
        witness.root_module.addCSourceFile(.{
            .file = b.path(b.fmt("sandbox-witness/witness_{s}.c", .{name})),
            .flags = &.{"-std=c11"},
        });
        witness.root_module.addIncludePath(b.path("sandbox-witness"));
        witness.root_module.addIncludePath(b.path("../include"));
        witness.root_module.addIncludePath(b.path("../third_party/luau/VM/include"));
        witness.root_module.linkLibrary(host);
        if (target.result.os.tag == .linux) witness.root_module.linkSystemLibrary("m", .{});
        const run_witness = b.addRunArtifact(witness);
        run_witness.skip_foreign_checks = false;
        run_witness.failing_to_execute_foreign_is_an_error = true;
        run_witness.setEnvironmentVariable("P4PC_INPUT", "script");
        run_witness.setEnvironmentVariable("P4PC_MODS", "0");
        run_witness.setEnvironmentVariable("P4PC_PACKS", "0");
        run_witness.setEnvironmentVariable("P4PC_ISO", "");
        b.step(b.fmt("sandbox-witness-{s}", .{name}), b.fmt("Run sandbox witness: {s} (exit 77 means incomplete)", .{name})).dependOn(&run_witness.step);
        witnesses.dependOn(&run_witness.step);
    }
}

// tools/mods/CMakeLists.txt's 64-bit profiles of mod_luau_test/mod_luau_save_test
// (P4_GUEST64), with the production bindings and fixture codegen, but the Zig
// bridge in place of p4_luau. The 32-bit profile needs the fixed PS2 RAM image.
fn addModLuauTests(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
    use_llvm: ?bool,
    host: *std.Build.Step.Compile,
    python: []const u8,
    step: *std.Build.Step,
) void {
    if (target.result.ptrBitWidth() != 64) {
        step.dependOn(&b.addFail("mod-luau-tests requires a 64-bit target (the P4_GUEST64 profile)").step);
        return;
    }
    const windows = target.result.os.tag == .windows;
    const aarch64 = target.result.cpu.arch.isAarch64();
    const lib_flags: []const []const u8 = if (aarch64)
        &.{ "-fsigned-char", "-mno-outline-atomics", "-O2", "-g1", "-fno-strict-aliasing", "-ffp-contract=off", "-ffunction-sections", "-fdata-sections" }
    else
        &.{ "-O2", "-g1", "-fno-strict-aliasing", "-ffp-contract=off", "-ffunction-sections", "-fdata-sections" };
    const test_flags = lib_flags[0 .. lib_flags.len - 2];

    const fixture = b.addSystemCommand(&.{python});
    fixture.addFileArg(b.path("../tests/mod_codegen_fixture.py"));
    for ([_][]const u8{
        "../tests/mod_hooks_game.c",      "../tools/mod_codegen.py",
        "../tools/gen_port.py",           "../tools/build_cache.py",
        "../include/p4pc/libc_rename.h",
    }) |input| fixture.addFileInput(b.path(input));
    const fixture_dir = fixture.addOutputDirectoryArg("mod_hook_fixture");

    const layout = b.addSystemCommand(&.{python});
    layout.addFileArg(b.path("tests/generate_mod_luau_api.py"));
    layout.addFileArg(b.path("../src/platform/mods/layout_api.luau"));
    const layout_header = layout.addOutputFileArg("mod_luau_api.h");

    // 052-capabilities.cmake's registry (package_registry.c includes it), from
    // every include/p4pc/p4.d fragment; rerun each build, as CMake re-globs them.
    const capabilities = b.addSystemCommand(&.{python});
    capabilities.addFileArg(b.path("tests/generate_mod_capabilities.py"));
    capabilities.addDirectoryArg(b.path("../include/p4pc/p4.d"));
    capabilities.has_side_effects = true;
    const capabilities_dir = capabilities.addOutputDirectoryArg("capabilities");

    const core = b.addLibrary(.{
        .name = "p4_mod_core",
        .use_llvm = use_llvm,
        .root_module = modLuauModule(b, target, optimize, false),
    });
    core.root_module.addCSourceFiles(.{ .files = &.{
        "../src/platform/mods/hooks.c",   "../src/platform/mods/events.c",
        "../src/platform/mods/failure.c", "../src/platform/mods/numbers.c",
        "../src/platform/mods/memory_policy.c",
    }, .flags = lib_flags });

    const bindings = b.addLibrary(.{
        .name = "p4_mod_luau",
        .use_llvm = use_llvm,
        .root_module = modLuauModule(b, target, optimize, true),
    });
    bindings.root_module.addCSourceFiles(.{ .files = &.{
        "../src/platform/mods/lua_api.c",       "../src/platform/mods/lua_runtime.c",
        "../src/platform/mods/lua_memory.c",    "../src/platform/mods/lua_hooks.c",
        "../src/platform/mods/lua_events.c",    "../src/platform/mods/lua_input.c",
        "../src/platform/mods/lua_files.c",     "../src/platform/mods/lua_settings.c",
        "../src/platform/mods/lua_lifecycle.c", "../src/platform/mods/lua_artifacts.c",
        "../src/platform/mods/lua_contract.c",  "../src/platform/mods/lua_save.c",
        "../src/platform/mods/failure_script.c",
    }, .flags = lib_flags });
    bindings.root_module.addIncludePath(layout_header.dirname());

    const shared = [_][]const u8{
        "../src/platform/mods/mods.c",                 "../src/platform/mods/package_registry.c",
        "../src/platform/mods/package_memory.c",       "../src/platform/mods/mod_setting_store.c",
        "../src/platform/mods/lifecycle.c",            "../src/platform/mods/mod_artifacts.c",
        "../src/platform/mods/compendium.c",           "../src/platform/mods/file_index.c",
        "../third_party/yxml/yxml.c",                  "../src/platform/memcard/mod_save.c",
        "../src/platform/memcard/mod_save_hash.c",     "../src/platform/memcard/mod_save_format.c",
        "../src/platform/memcard/save_image.c",        "../src/platform/files/vfs.c",
        "../src/platform/files/iso9660.c",             "../tests/guest_test.c",
        "../src/platform/gport.c",                     "../src/platform/guest.c",
        "../src/platform/ee_bytes.c",                  "../src/platform/content_digest.c",
        "../src/platform/kernel_thread.c",             "../src/platform/state_dump.c",
        "../src/platform/ps2_kernel.c",                "../tests/state_dump_test_stubs.c",
        "../src/platform/mods/lua_stages.c",           "../src/platform/mods/game_stage.c",
        "../src/platform/mods/game_damage_stage.c",    "../src/platform/mods/game_recovery_stage.c",
        "../src/platform/mods/game_cost_stage.c",      "../src/platform/mods/game_chance_stage.c",
        "../src/platform/mods/game_wearoff_stage.c",   "../src/platform/mods/game_buff_stage.c",
        "../src/platform/mods/game_reward_stage.c",    "../src/platform/mods/game_unit_stage.c",
        "../src/platform/mods/game_hit_stage.c",       "../src/platform/mods/lua_passive.c",
        "../src/platform/mods/game_passive_grant.c",   "../src/platform/mods/game_data.c",
        "../tests/mod_passive_facts.c",
    };
    const os_source: []const u8 = if (windows) "../src/platform/os/os_win32.c" else "../src/platform/os/os_posix.c";
    // Only mod_luau_test supplies the synthetic services used by these bindings;
    // the snapshot witness adds the production card transaction transport.
    const variants = [_]struct { name: []const u8, extra: []const []const u8 }{
        .{ .name = "mod_luau_test", .extra = &.{ "../tests/mod_luau_test.c", "../src/platform/mods/lua_battle_substitute.c" } },
        .{ .name = "mod_luau_save_test", .extra = &.{
            "../tests/mod_luau_save_test.c",               "../src/platform/memcard/save_transaction.c",
            "../src/platform/memcard/mod_save_custody.c", "../src/platform/memcard/mod_save_transport.c",
        } },
    };
    for (variants) |variant| {
        const exe = b.addExecutable(.{
            .name = variant.name,
            .use_llvm = use_llvm,
            .root_module = modLuauModule(b, target, optimize, true),
        });
        const module = exe.root_module;
        module.addCSourceFiles(.{ .files = variant.extra, .flags = test_flags });
        module.addCSourceFiles(.{ .files = &shared, .flags = test_flags });
        module.addCSourceFiles(.{ .files = &.{os_source}, .flags = test_flags });
        module.addCSourceFiles(.{ .root = fixture_dir, .files = &.{ "game.c", "mod_bridges.c" }, .flags = test_flags });
        module.addIncludePath(b.path("../tests"));
        module.addIncludePath(b.path("../src/platform/mods"));
        module.addIncludePath(b.path("../src/platform/files"));
        module.addIncludePath(capabilities_dir);
        module.linkLibrary(bindings);
        module.linkLibrary(core);
        module.linkLibrary(host);
        step.dependOn(&b.addInstallArtifact(exe, .{}).step);
    }
}

/// Compile settings shared by the native mod targets: the 64-bit guest model
/// definitions and the public include roots, without Zig's C sanitizer (the
/// CMake profiles build these units with plain clang).
fn modLuauModule(b: *std.Build, target: std.Build.ResolvedTarget, optimize: std.builtin.OptimizeMode, luau: bool) *std.Build.Module {
    const module = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .sanitize_c = .off,
    });
    module.addCMacro("_FILE_OFFSET_BITS", "64");
    module.addCMacro("P4_GUEST64", "1");
    module.addCMacro("P4_GUEST_CHECK", "1");
    module.addIncludePath(b.path("../include"));
    if (luau) {
        module.addCMacro("P4_MOD_LUAU", "1");
        module.addIncludePath(b.path("../third_party/luau/VM/include"));
    }
    if (target.result.os.tag == .linux) module.linkSystemLibrary("m", .{});
    return module;
}
