/* Entry point for p4pc. Loads the retail SLUS_217.82 image (src/image.c),
 * patches it, and enters the game through p4pc_start (src/crt.c). */
#include <stdint.h>
#include <stdio.h>

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
#include "p4pc/mod_bindings.h"
#include "platform/mods/failure_native.h"
#include "p4pc/content.h"
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
            "p4pc.ini, p4pc.log and the saves are in %s.\n"
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
    if (argc > 1 && is_help(argv[1])) return usage(argv[0]);
    if (p4_os_restart_init(argc, argv) != 0) return 1;
    int import_status = p4save_import_cli(argc, argv);
    if (import_status >= 0) return import_status;
    const char *path = argc > 1 ? argv[1] : "SLUS_217.82";
    int sim = p4_battle_sim_cli(argc, argv, &path);
    if (sim < 0) return 2;
#ifdef P4PC_PLAYER_DISC
    /* Started with no arguments, as a player does: the disc is the player's own (asked for once), and the executable is read from it. */
    char elf[1100];

    if (argc <= 1) {
        p4iso_player_log();
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
        if (p4_mod_witness_validate() ||
            !p4_mod_native_startup_refused("Native package recovery could not be initialized. Starting without optional packages."))
            return 1;
        optional = 0;
    }
    p4pc_trap_configure();

    uint32_t entry = p4pc_load_image(path);
    size_t patched = p4pc_apply_patches();
    fprintf(stderr, "p4pc: patched %zu function pointers\n", patched);
    fprintf(stderr, "p4pc: entering game (PS2 entry 0x%08x)\n", (unsigned)entry);

    p4_mod_save_native_init();
    if (optional && (p4_mod_game_bindings_init() != 0 || p4_golden_producer_register() != 0 || p4_package_boot_install() != 0 ||
                     p4_mods_init() != 0 || p4_packs_init() != 0 ||
                     p4_package_registry_publish() != 0)) {
        /* The first selector may already be staged when the second fails. */
        p4_package_registry_retire();
        if (p4_mod_witness_validate() ||
            !p4_mod_native_startup_refused("Optional packages could not be prepared. No replacement files/native work or mod code were exposed. Starting without optional packages."))
            return 1;
    }
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
