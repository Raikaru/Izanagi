/* Entry point for p4pc. Loads the retail SLUS_217.82 image (src/image.c),
 * patches it, and enters the game through p4pc_start (src/crt.c). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "p4pc/crt.h"
#include "p4pc/image.h"
#include "p4pc/save_import.h"
#include "p4pc/mod_save_native.h"
#include "p4pc/mods.h"
#include "p4pc/packs.h"
#include "p4pc/battle_sim.h"
#include "p4pc/os.h"
#include "p4pc/package_boot.h"
#include "p4pc/golden_producer.h"
#include "p4pc/game_mode.h"
#include "p4pc/game_table_boot.h"
#include "platform/golden/golden_mode_sources.h"
#include "platform/diag/p4diag.h"
#include "platform/golden/golden_cpk_vfs.h"
#include "p4pc/mod_bindings.h"
#include "platform/mods/failure_native.h"
#include "p4pc/content.h"
#include "p4pc/guest.h"
#include "p4pc/c03_guard.h"
#include "p4pc/build_id.h"
#include "platform/memcard/mod_save_transport.h"

#if defined(_WIN32) || (defined(__linux__) && !defined(__ANDROID__)) || defined(__APPLE__)
#include "platform/config/p4iso.h"
#define P4PC_PLAYER_DISC 1
#endif

#ifdef __ANDROID__
/* SDL's activity starts SDL_main (src/platform/android/android_main.c), which runs the loader on a thread with a large stack. */
#define main p4pc_main
#endif

/* `--help` (or -h, /?): what a player can pass; developer switches (battle simulator, test variables) are not listed. */
#ifdef _WIN32
#define P4PC_HELP_FOLDER "%APPDATA%\\p4pc\\Persona4\\"
#elif defined(__APPLE__)
#define P4PC_HELP_FOLDER "~/Library/Application Support/p4pc/Persona4/"
#else
#define P4PC_HELP_FOLDER "~/.local/share/p4pc/Persona4/ ($XDG_DATA_HOME/p4pc/Persona4/)"
#endif
static int usage(const char *argv0)
{
    static const char *const rows[][2] = {
        {"", "play; the first run asks for your disc image"},
        {"<SLUS_217.82>", "start from an extracted game executable instead"},
        {"--import-save FILE_OR_FOLDER [--slot N]", "copy a PS2 save into the game's memory card"},
        {"--help", "this text"},
    };
    const char *program = argv0 != NULL && *argv0 ? argv0 : "p4pc";

    for (const char *p = program; *p; p++) {
        if (*p == '/' || *p == '\\') program = p + 1;
    }
    fprintf(stdout, "%s: Persona 4 (USA) on PC, played from your own disc image.\n\nusage:\n", "Persona4-PC");
    for (size_t i = 0; i < sizeof rows / sizeof rows[0]; i++) {
        fprintf(stdout, "  %s %-40s %s\n", program, rows[i][0], rows[i][1]);
    }
    fprintf(stdout, "\nThe disc image is `iso` under [game] in p4pc.ini (or the P4PC_ISO environment variable).\n"
            "p4pc.ini, the logs (p4pc.log, and p4pc.prev.log of the run before) and the saves are in %s.\n"
            "Mods: choose them in Config > MODS. To make one, see p4mod --help.\n", P4PC_HELP_FOLDER);
    return 0;
}

/* argv[1] is a help spelling. A byte loop: the game's own strcmp is the retail one, not for port startup. */
static int is_help(const char *arg)
{
    static const char *const spellings[] = {"--help", "-h", "/?"};

    for (size_t i = 0; i < sizeof spellings / sizeof spellings[0]; i++) {
        const char *a = arg, *b = spellings[i];
        while (*a != '\0' && *a == *b) {
            a++;
            b++;
        }
        if (*a == *b) return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
#ifdef P4_GOLDEN_AI_QUALIFICATION
    fprintf(stderr, "p4pc: test-only Golden AI qualification build refuses normal boot; use golden_ai_native_install64[w]\n");
    return 2;
#endif
    if (argc > 1 && is_help(argv[1])) return usage(argv[0]);
    if (p4_os_restart_init(argc, argv) != 0) return 1;
    p4_os_crash_build_set(p4_build_id()); /* the native crash report names the build */
#ifdef __ANDROID__
    fprintf(stderr, "p4pc: build %s\n", p4_build_id()); /* the log is already redirected */
#endif
    int import_status = p4save_import_cli(argc, argv);
    if (import_status >= 0) return import_status;
    const char *path = argc > 1 ? argv[1] : "SLUS_217.82";
    int sim = p4_battle_sim_cli(argc, argv, &path);
    if (sim < 0) return 2;
    const P4GameMode mode = p4_game_mode();
    if (mode == P4_GAME_MODE_INVALID) {
        fprintf(stderr, "p4pc: invalid game mode: %s\n", p4_game_mode_reason());
        return 1;
    }
    if (mode == P4_GAME_MODE_GOLDEN) {
#ifdef P4_GUEST64
        if (p4_c03_guard_install_cleanup() || p4_guest_arena_factory(p4_c03_guard_reserve)) {
            fprintf(stderr, "p4pc: Golden C03 arena could not be reserved; refusing to start\n");
            return 1;
        }
#else
        fprintf(stderr, "p4pc: Golden C03 protection requires a supported 64-bit runtime\n");
        return 1;
#endif
        if (p4_golden_mode_sources_register()) {
            fprintf(stderr, "p4pc: could not register Golden table sources\n");
            return 1;
        }
        p4_game_tables_boot_require_golden();
    }
#if defined(P4PC_GOLDEN_VFS_WITNESS) && !defined(NDEBUG)
    if (mode == P4_GAME_MODE_PS2 && p4_golden_cpk_vfs_dormant_requested() &&
        p4_golden_cpk_vfs_start_dormant()) {
        fprintf(stderr, "p4pc: dormant Golden VFS witness setup failed\n");
        return 1;
    }
#endif
#ifdef P4PC_PLAYER_DISC
    /* Started with no arguments, as a player does: the disc is the player's own (asked for once), and the executable is read from it. */
    char elf[1100];

    if (argc <= 1) {
        p4iso_player_log();
        fprintf(stderr, "p4pc: build %s\n", p4_build_id()); /* first line of p4pc.log */
        if (p4iso_prepare(elf, sizeof elf) != 0) {
            return 1;
        }
        path = elf;
    }
#endif
    /* Simulator runs are retail; only a declared mod cohort admits its packages,
     * through the same optional-package path as an ordinary launch. */
    const int cohort = sim && p4_battle_sim_mod_cohort();
    int optional = !sim || cohort;
    if (optional && p4_mod_native_failure_init() != 0) {
        if (mode == P4_GAME_MODE_GOLDEN) {
            fprintf(stderr, "p4pc: Golden mode cannot start after native package recovery initialization failed\n");
            return 1;
        }
        if (p4_mod_witness_validate() ||
            !p4_mod_native_startup_refused("Native package recovery could not be initialized. Starting without optional packages."))
            return 1;
        optional = 0;
    }
    p4diag_startup(path); /* the log's header: build, system, settings, mods, the disc check (player runs only) */
    p4pc_trap_configure();

    uint32_t entry = p4pc_load_image(path);
    if (mode == P4_GAME_MODE_GOLDEN) {
#ifdef P4_GUEST64
        if (p4_c03_guard_start() ||
            p4_mod_save_transport_fence(p4_c03_guard_tainted) || p4_c03_guard_install_report()) {
            fprintf(stderr, "p4pc: Golden C03 protection could not be established; refusing to start\n");
            return 1;
        }
#else
        fprintf(stderr, "p4pc: Golden C03 protection requires a supported 64-bit runtime\n");
        return 1;
#endif
    }
    size_t patched = p4pc_apply_patches();
    fprintf(stderr, "p4pc: patched %zu function pointers\n", patched);
    if (mode != P4_GAME_MODE_GOLDEN)
        fprintf(stderr, "p4pc: entering game (PS2 entry 0x%08x)\n", (unsigned)entry);

    p4_mod_save_native_init();
    if (optional && (p4_mod_game_bindings_init() != 0 || p4_golden_producer_register() != 0 || p4_package_boot_install() != 0 ||
                     p4_mods_init() != 0 || p4_packs_init() != 0 ||
                     p4_package_registry_publish() != 0)) {
        /* The first selector may already be staged when the second fails. */
        p4_package_registry_retire();
        if (mode == P4_GAME_MODE_GOLDEN) {
            fprintf(stderr, "p4pc: Golden table or package preparation failed; refusing PS2 fallback\n");
            return 1;
        }
        if (p4_mod_witness_validate() ||
            !p4_mod_native_startup_refused("Optional packages could not be prepared. No replacement files/native work or mod code were exposed. Starting without optional packages."))
            return 1;
    }
    if (mode == P4_GAME_MODE_GOLDEN && !p4_game_tables_boot_golden_ready()) {
        if (optional) p4_package_registry_retire();
        fprintf(stderr, "p4pc: Golden mode requires all tables from one published verified Golden cache; refusing PS2 fallback\n");
        return 1;
    }
    if (mode == P4_GAME_MODE_GOLDEN && p4_golden_cpk_vfs_start()) {
        fprintf(stderr, "p4pc: cannot mount the verified Golden archives\n");
        return 1;
    }
    if (mode == P4_GAME_MODE_GOLDEN)
        fprintf(stderr, "p4pc: entering game (PS2 entry 0x%08x)\n", (unsigned)entry);
    if (!sim || cohort) p4_package_boot_admission_complete();
    if (!sim && p4_content_init() != 0) return 1;
    if (p4_mod_witness_enabled())
        fprintf(stderr, "p4pc: run_mode=mod-witness mod_witness=true reference_eligible=false; NOT a retail reference\n");

    p4save_import_settings_init();
    char *game_argv[] = { (char *)path, NULL };
    p4pc_start(1, game_argv);
    if (sim) return 0;
    return p4_content_finish() != 0 ? 1 : 0;
}
