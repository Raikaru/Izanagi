/* Loader control flow with synthetic dependencies; never starts the game. */
#define main p4pc_loader_test_entry
#include "../src/loader.c"
#undef main
#define CHECK(x) do { if (!(x)) { fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); return 1; } } while (0)

static P4GameMode mode;
static int fail_init, fail_publish, ready, started, fallback, retired, loaded, golden_requested;
int p4_os_restart_init(int argc, char **argv) { (void)argc; (void)argv; return 0; }
int p4save_import_cli(int argc, char **argv) { (void)argc; (void)argv; return -1; }
int p4_battle_sim_cli(int argc, char **argv, const char **path) { (void)argc; (void)argv; (void)path; return 0; }
int p4_battle_sim_mod_cohort(void) { return 0; }
P4GameMode p4_game_mode(void) { return mode; }
const char *p4_game_mode_reason(void) { return "invalid"; }
int p4_golden_mode_sources_register(void) { return 0; }
void p4iso_player_log(void) { }
int p4iso_prepare(char *out, size_t cap) { (void)out; (void)cap; return -1; }
int p4_mod_native_failure_init(void) { return fail_init; }
int p4_mod_witness_validate(void) { return 0; }
int p4_mod_witness_enabled(void) { return 0; }
int p4_mod_native_startup_refused(const char *why) { (void)why; ++fallback; return 1; }
void p4pc_trap_configure(void) { }
uint32_t p4pc_load_image(const char *path) { (void)path; ++loaded; return 0; }
size_t p4pc_apply_patches(void) { return 0; }
void p4_mod_save_native_init(void) { }
int p4_mod_game_bindings_init(void) { return 0; }
int p4_golden_producer_register(void) { return 0; }
int p4_package_boot_install(void) { return 0; }
int p4_mods_init(void) { return 0; }
int p4_packs_init(void) { return 0; }
int p4_package_registry_publish(void) { return fail_publish; }
void p4_package_registry_retire(void) { ++retired; }
int p4_game_tables_boot_golden_ready(void) { return ready; }
void p4_game_tables_boot_require_golden(void) { ++golden_requested; }
void p4_package_boot_admission_complete(void) { }
int p4_content_init(void) { return 0; }
int p4_content_finish(void) { return 0; }
void p4save_import_settings_init(void) { }
void p4pc_start(int argc, char **argv) { (void)argc; (void)argv; ++started; }
static int run(void) { char *args[] = {"test", "synthetic.elf", NULL}; return p4pc_loader_test_entry(2, args); }
static void reset(void) { fail_init = fail_publish = ready = started = fallback = retired = loaded = golden_requested = 0; }
int main(void)
{
    reset(); mode = P4_GAME_MODE_INVALID;
    CHECK(run() == 1 && !loaded && !started);
    reset(); mode = P4_GAME_MODE_GOLDEN; fail_init = 1;
    CHECK(run() == 1 && golden_requested == 1 && !fallback && !started);
    reset(); mode = P4_GAME_MODE_GOLDEN; fail_publish = 1;
    CHECK(run() == 1 && retired == 1 && !fallback && !started);
    reset(); mode = P4_GAME_MODE_GOLDEN;
    CHECK(run() == 1 && retired == 1 && !fallback && !started);
    reset(); mode = P4_GAME_MODE_GOLDEN; ready = 1;
    CHECK(run() == 0 && golden_requested == 1 && started == 1 && !fallback);
    reset(); mode = P4_GAME_MODE_PS2; fail_init = 1;
    CHECK(run() == 0 && !golden_requested && fallback == 1 && started == 1);
    reset(); mode = P4_GAME_MODE_PS2; fail_publish = 1;
    CHECK(run() == 0 && retired == 1 && fallback == 1 && started == 1);
    puts("golden_loader_policy: ok"); return 0;
}
