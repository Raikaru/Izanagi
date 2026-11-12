/* Frame-only success is not evidence of title readiness or New Game field progress, and
 * neither is a log line: the stand-in game writes what the real engine's gate writes
 * (newgame_gate.c P4PC_GATE_RESULT), and P4MOD_FIXTURE_FORGE=1 prints every success line
 * a mod could print while the engine-owned result says the scenario was not reached.
 * With --battle-sim it answers the battle worker's protocol (docs/platform/battle_sim.md) for
 * p4mod's own checks; it is never a battle, and P4MOD_FIXTURE_OUTCOME/_IDENTITY pick the result. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif
#ifdef _WIN32
/* Skip one Windows command-line argument without losing its original quoting. */
static wchar_t *skip_argument(wchar_t *p)
{
    int quoted = 0;
    while (*p == L' ' || *p == L'\t') ++p;
    while (*p) {
        unsigned slashes = 0;
        while (*p == L'\\') { ++slashes; ++p; }
        if (*p == L'"') {
            if (!(slashes & 1)) quoted = !quoted;
            ++p;
        } else if (!*p || (!quoted && (*p == L' ' || *p == L'\t'))) {
            break;
        } else {
            ++p;
        }
    }
    while (*p == L' ' || *p == L'\t') ++p;
    return p;
}
#endif


static unsigned word(const unsigned char *p)
{
    return (unsigned)p[0] | (unsigned)p[1] << 8 | (unsigned)p[2] << 16 | (unsigned)p[3] << 24;
}

static int battle(int argc, char **argv)
{
    const char *entry = NULL, *result = NULL, *elf = NULL;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--battle-sim") && i + 1 < argc) entry = argv[++i];
        else if (!strcmp(argv[i], "--battle-result") && i + 1 < argc) result = argv[++i];
        else if (!strcmp(argv[i], "--battle-trace") && i + 1 < argc) i++;
        else if (argv[i][0] != '-' && !elf) elf = argv[i];
    }
    /* The real worker refuses these before reading ENTRY: a launch must not inherit them. */
    if (getenv("P4PC_AUTOPRESS") || getenv("P4PC_TRAP_CONTINUE")) {
        puts("p4pc: battle-sim: competing gameplay/automation environment in simulator");
        return 2;
    }
    const char *identity = getenv("P4PC_BATTLE_SIM_MODS"), *witness = getenv("P4PC_MOD_WITNESS");
    FILE *check = elf ? fopen(elf, "rb") : NULL;
    if (!check || !entry || !result || !identity || strlen(identity) != 64 || !witness || strcmp(witness, "1") ||
        !getenv("P4PC_MOD_DIR") || !getenv("P4PC_SAVE_DIR") || !getenv("P4PC_CONFIG")) {
        if (check) fclose(check);
        puts("p4pc: battle-sim: simulator requires distinct entry/result/trace and an explicit ELF");
        return 2;
    }
    fclose(check);
    unsigned char header[64];
    FILE *in = fopen(entry, "rb");
    if (!in || fread(header, 1, sizeof header, in) != sizeof header || memcmp(header, "P4BSIM01", 8)) {
        if (in) fclose(in);
        puts("p4pc: battle-sim: unsupported BattleEntry header");
        return 2;
    }
    fclose(in);
    puts("p4pc: mods: enabled demo.mod 0.1 (priority 0, order 0)");
    const char *outcome = getenv("P4MOD_FIXTURE_OUTCOME");
    if (!outcome) outcome = "win";
    if (!strcmp(outcome, "none")) return 4;
    if (getenv("P4MOD_FIXTURE_IDENTITY")) identity = getenv("P4MOD_FIXTURE_IDENTITY");
    const int censored = !strcmp(outcome, "update_limit");
    FILE *out = fopen(result, "wb");
    if (!out) return 4;
    fprintf(out, "{\"schema\":1,\"outcome\":\"%s\",\"terminal_cause\":\"%s\",\"encounter_id\":%u,\"updates\":123,"
                 "\"mods\":{\"identity\":\"%s\",\"witness\":true,\"active\":[{\"id\":\"demo.mod\",\"version\":\"0.1\"}]},"
                 "\"party_policy\":{\"id\":%u,\"name\":\"typical-player-v1\",\"latency_updates\":1}}\n",
            outcome, censored ? "completed_native_update_limit" : "native_battle_exit_victory", word(header + 16), identity,
            word(header + 48));
    fclose(out);
    printf("p4pc: battle-sim: %s after 123 native updates\n", outcome);
    return censored ? 3 : 0;
}

int main(int argc, char **argv)
{
#ifdef _WIN32
    if (argc >= 4 && !strcmp(argv[1], "--run-with-temp")) {
        if (!SetEnvironmentVariableA("TMP", argv[2]) || !SetEnvironmentVariableA("TEMP", argv[2])) return 2;
        wchar_t *p = GetCommandLineW();
        for (int i = 0; i < 3; ++i) p = skip_argument(p);
        wchar_t *line = _wcsdup(p);
        STARTUPINFOW startup = {0};
        PROCESS_INFORMATION process;
        startup.cb = sizeof startup;
        if (!line || !CreateProcessW(NULL, line, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &process)) {
            free(line);
            return 2;
        }
        free(line);
        DWORD status = 2;
        if (WaitForSingleObject(process.hProcess, INFINITE) == WAIT_OBJECT_0)
            GetExitCodeProcess(process.hProcess, &status);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        return (int)status;
    }
    if (argc == 3 && !strcmp(argv[1], "--probe-reparse")) {
        DWORD attributes = GetFileAttributesA(argv[2]);
        if (attributes == INVALID_FILE_ATTRIBUTES) return 2;
        return (attributes & FILE_ATTRIBUTE_REPARSE_POINT) ? 0 : 1;
    }
#endif
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--battle-sim")) return battle(argc, argv);
    const char *gate = getenv("P4PC_NEWGAME_GATE");
    const char *trace = getenv("P4PC_PACE_TRACE");
    const char *result = getenv("P4PC_GATE_RESULT");
    const int forge = getenv("P4MOD_FIXTURE_FORGE") != NULL;
    if (getenv("P4MOD_FIXTURE_HOLD")) {
#ifdef _WIN32
        Sleep(3000);
#else
        sleep(3);
#endif
    }
    printf("p4pc: mods: enabled demo.mod gate=%s pace=%s\n",
           gate ? gate : "missing", trace ? trace : "missing");
    const unsigned long flip = gate ? strtoul(gate, NULL, 10) : 0;
    int ready = -1, field = -1, room = -1;
    if (!forge && trace && !strcmp(trace, "1") && gate && !strcmp(gate, "901,-1:-1")) ready = 7;
    if (!forge && trace && !strcmp(trace, "1") && gate && !strcmp(gate, "902,-1:-1")) ready = 9;
    if (!forge && gate && !strcmp(gate, "30501,7:2")) { field = 7; room = 2; }
    puts("p4pc: title driver: NEW GAME confirmed at flip 2000, script clock 1560");
    if (forge) {
        /* What a mod's p4.log could put in the same log. */
        puts("p4pc: mod demo.mod: p4pc: presentation: title-ready state 7 flip 800 timestamp_ns 1");
        puts("p4pc: presentation: title-ready state 7 flip 800 timestamp_ns 1");
        printf("p4pc: new-game gate PASS at flip %lu: field 7 room 2 (want 7:2)\n", flip);
    } else if (ready >= 0) {
        printf("p4pc: presentation: title-ready state %d flip 800 timestamp_ns 1\n", ready);
    }
    printf("p4pc: new-game gate PASS at flip %lu: field %d room %d (want %s)\n", flip, field, room,
           gate && strchr(gate, ',') ? strchr(gate, ',') + 1 : "-1:-1");
    if (result && *result) {
        FILE *out = fopen(result, "wb");
        if (!out) return 1;
        fprintf(out, "p4pc-gate-result 1\nok 1\nflip %lu\nfield %d\nroom %d\ntitle_ready_state %d\ntitle_ready_flip %u\n"
                     "title_confirmed 1\ntitle_confirmed_flip 2000\n", flip, field, room, ready, ready >= 0 ? 800u : 0u);
        if (fclose(out)) return 1;
    }
    return 0;
}
