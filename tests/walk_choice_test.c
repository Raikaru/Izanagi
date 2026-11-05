#include "p4pc/walk_script.h"
#include <stdio.h>
#include <string.h>

/* Sparse guest message, dungeon-step, stair-goal and battle fields. No host
 * pointers are exposed through the guest-memory callback. */
typedef struct {
    unsigned active, state, cursor, count, flags;
    unsigned row_state;
    unsigned extra_text;
    unsigned evt_phase, evt_active, evt_wait, evt_message, evt_handle;
    unsigned window_handle, row_index;
    unsigned previous, downs, ups, confirms;
    int wrong_confirm;
    unsigned step_present, step;
    unsigned battle_ready, action_state;
    unsigned battle_flags, other_actor;
    unsigned battle_character;
    unsigned goal_fixture, goal_area;
    unsigned field_ids, player_x, player_z, goal_control_off;
    unsigned task_fixture, task_present;
    unsigned shuffle_fixture, persona_popup;
    unsigned enemy_fixture, enemy_id, enemy_hp, other_enemy_hp;
    unsigned enemy_buffs, other_enemy_buffs;
    unsigned target_fixture, target_count, target_selected, target_mode;
    unsigned target_ids[3], target_hp[3], target_bad[3], target_friendly;
    unsigned field_event;
} Menu;

static uint32_t read32(void *user, uint32_t addr) {
    Menu *m = user;
    if (addr == 0x00764604u) return m->field_event;
    if (addr == 0x008817ECu) return m->active ? 0x01000000u : 0;
    if (addr == 0x01000008u) return m->window_handle;
    if (addr == 0x0100000Cu) return 0x01001000u;
    if (addr == 0x01000004u) return m->extra_text ? 0x01000020u : 0;
    if (addr == 0x0100002Cu) return 0x01001100u;
    if (addr == 0x01001100u) return 0x204u;
    if (addr == 0x01001124u) return 1;
    if (addr == 0x01001148u) return 0xFFFFu;
    if (addr == 0x01001000u) return m->flags;
    if (addr == 0x0100101Cu) return 0x01500000u; /* Native text-record pointer, not row.state. */
    if (addr == 0x01001024u) return m->row_state;
    if (addr == 0x0100102Cu) return m->row_index | (3u << 16);
    if (addr == 0x01001048u) return m->state | (m->cursor << 16);
    if (addr == 0x0100104Cu) return m->count << 16;
    if (addr == 0x00764334u) return m->step_present ? 0x01002000u : 0;
    if (addr == 0x01002038u) return 0x01003000u;
    if (addr == 0x01003004u) return m->step;
    if (m->shuffle_fixture) {
        static const char names[3][24] = {"battle", "battle_shuffle", "shdPersonaStatusDraw"};
        if (addr == 0x00764ADCu) return m->battle_ready ? 0x01600000u : 0;
        for (unsigned n = 0; n < 3; n++) {
            uint32_t node = 0x01600000u + n * 0x100u;
            if (addr == node + 0x3Cu)
                return n == 0 || (n == 1 && m->persona_popup) ? node + 0x100u : 0;
            if (addr >= node && addr < node + sizeof names[n]) {
                uint32_t value;
                memcpy(&value, names[n] + (addr - node), sizeof value);
                return value;
            }
        }
    }
    if (m->task_fixture) {
        switch (addr) {
        case 0x00764ADCu: return m->task_present ? 0x01300000u : 0;
        case 0x01300000u: return 0x4D706D63u; /* "cmpM" */
        case 0x01300004u: return 0x006E6961u; /* "ain" */
        default: return 0;
        }
    }
    if (m->evt_phase) {
        switch (addr) {
        case 0x00764ADCu: return 0x01400000u;
        case 0x01400000u: return 0x45747665u; /* "evtEvent" */
        case 0x01400004u: return 0x746E6576u;
        case 0x01400038u: return 0x01401000u;
        case 0x01401000u: return m->evt_phase;
        case 0x0140100Cu: return 0x01402000u;
        case 0x01402038u: return 0x01403000u;
        case 0x01403678u: return m->evt_active;
        case 0x01403680u: return m->evt_handle;
        case 0x01403684u: return m->evt_wait;
        case 0x01403688u: return m->evt_message;
        default: break;
        }
    }
    if (m->goal_fixture) {
        switch (addr) {
        case 0x00764ADCu: return m->goal_control_off ? 0 : 0x01200000u;
        case 0x01200000u: return 0x61746F72u; /* "rotate pc" */
        case 0x01200004u: return 0x70206574u;
        case 0x01200008u: return 0x00000063u;
        case 0x01200038u: return 0x01201000u;
        case 0x01201048u: return 0x01202000u;
        case 0x01202030u: return m->player_x;
        case 0x01202038u: return m->player_z;
        case 0x00762EA0u: return 0x01203000u;
        case 0x01203008u: return 0x01204000u;
        case 0x007D2510u: return 0x01207000u;
        case 0x01207038u: return 0x01208000u;
        case 0x01208018u: return m->field_ids;
        case 0x01204034u: return m->goal_area ? 0x01205000u : 0;
        case 0x01204038u: return 0x01206000u;
        case 0x01205000u: return m->goal_area;
        case 0x0120515Cu: case 0x01205180u:
            return m->goal_area == 0x37FFu ? 0x42C80000u : 0; /* x = 100 */
        case 0x01205164u: case 0x01205168u: case 0x01205170u: case 0x01205174u:
            return m->goal_area == 0x37FFu ? 0x43960000u : 0; /* 300 */
        case 0x0120517Cu: case 0x01205188u:
            return m->goal_area == 0x37FFu ? 0x43FA0000u : 0; /* z = 500 */
        case 0x01206140u:
            return m->goal_area == 0x37FFu ? 0x447A0000u : 0x43480000u;
        case 0x01206148u:
            return m->goal_area == 0x37FFu ? 0x447A0000u : 0x43C80000u;
        default: break;
        }
    }
    if (m->battle_ready) {
        if (addr == 0x00764ADCu) return 0x01100000u;
        if (addr == 0x01100000u) return 0x74746162u; /* "batt" */
        if (addr == 0x01100004u) return 0x0000656Cu; /* "le" */
        if (addr == 0x0076449Cu) return 0x01101000u;
        if (addr == 0x011012FCu) return 8;
        if (addr == 0x0110100Cu) return m->battle_flags;
        if (addr == 0x01101104u) return m->other_actor ? 0x01104000u : 0x01102000u;
        if (addr == 0x0110200Cu || addr == 0x0110400Cu) return m->action_state;
        if (addr == 0x01102030u || addr == 0x01101178u) return 0x01103000u;
        if (addr == 0x011030A4u) return m->battle_character ? m->battle_character : 1u;
        if (addr == 0x01103000u + 0xA6Cu || addr == 0x01104030u) return 0x01105000u;
        if (m->target_fixture) {
            if (addr == 0x011020D0u) return m->target_count | (m->target_selected << 16);
            if (addr == 0x007644A8u) return 0x01830000u;
            if (addr == 0x01830008u) return m->target_mode;
            for (unsigned n = 0; n < 3; n++) {
                if (addr == 0x01102098u + n * 4) return 0x01800000u + n * 0x100u;
                if (addr == 0x01800030u + n * 0x100u) return 0x01810000u + n * 0x100u;
                if (addr == 0x01810A64u + n * 0x100u) return 0x01820000u + n * 0x100u;
                if (addr == 0x01820000u + n * 0x100u)
                    return (m->target_friendly ? 1u : 4u) | (m->target_ids[n] << 16);
                if (addr == 0x01820008u + n * 0x100u) return m->target_hp[n];
                if (addr == 0x0182000Cu + n * 0x100u) return m->target_bad[n];
            }
        }
        if (m->enemy_fixture) {
            if (addr == 0x01101174u) return m->enemy_fixture == 2 ? 0x01102000u : 0x01106000u;
            if (addr == 0x01106030u) return 0x01107000u;
            if (addr == 0x01106450u) return 0x01108000u;
            if (addr == 0x01108030u) return 0x01109000u;
            if (addr == 0x01107A64u) return 0x0110A000u;
            if (addr == 0x01109A64u) return 0x0110B000u;
            if (addr == 0x0110A000u || addr == 0x0110B000u) return 0x85u | (m->enemy_id << 16);
            if (addr == 0x0110A008u) return m->enemy_hp;
            if (addr == 0x0110B008u) return m->other_enemy_hp;
            if (addr == 0x0110A014u) return m->enemy_buffs;
            if (addr == 0x0110B014u) return m->other_enemy_buffs;
        }
    }
    return 0;
}

static P4Walk *parse(const char *script) {
    char error[160];
    P4Walk *w = p4walk_parse(script, error, sizeof error);
    if (!w) fprintf(stderr, "parse: %s\n", error);
    return w;
}

static int select_answer(unsigned initial, unsigned answer) {
    Menu menu = {0};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("choose:2\nwait:1\n");
    if (!w) return 1;
    menu.cursor = initial;
    menu.count = 3;
    for (unsigned frame = 0; frame < 100; ++frame) {
        if (frame == 25) { menu.active = 1; menu.state = 1; menu.flags = 0x18; }
        if (frame == 35) { menu.state = 2; menu.flags = 0x20; }
        unsigned held = p4walk_frame(w, &host, frame);
        unsigned edge = held & ~menu.previous;
        menu.previous = held;
        if (frame >= 25 && frame < 35 && held) {
            fprintf(stderr, "input during menu opening\n");
            p4walk_free(w);
            return 1;
        }
        if (menu.active && menu.state == 2) {
            if (edge & (1u << P4W_DOWN)) { ++menu.cursor; ++menu.downs; }
            if (edge & (1u << P4W_UP)) { --menu.cursor; ++menu.ups; }
            if (edge & (1u << P4W_CROSS)) {
                ++menu.confirms;
                menu.wrong_confirm = menu.cursor != answer;
                menu.active = 0;
            }
        }
        if (p4walk_status(w) != P4WALK_RUNNING) break;
    }
    int failed = p4walk_status(w) != P4WALK_DONE || menu.wrong_confirm || menu.confirms != 1 ||
        (initial < answer ? menu.downs != answer - initial : menu.ups != initial - answer);
    p4walk_free(w);
    return failed;
}

static int wait_for_question(void) {
    Menu menu = {.state = 0xFFFFu, .cursor = 2, .count = 3, .flags = 0x204u};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("waitchoice\n");
    if (!w) return 1;
    int failed = p4walk_frame(w, &host, 0) != 0 ||
        p4walk_status(w) != P4WALK_RUNNING;
    menu.active = 1;
    failed |= p4walk_frame(w, &host, 1) != 0; /* A text-record pointer alone is not a live row. */
    menu.row_state = 1;
    failed |= p4walk_frame(w, &host, 2) != (1u << P4W_CROSS);
    menu.state = 1;
    menu.flags = 0x18;
    menu.extra_text = 1; /* An unrelated live caption must not answer the opening question. */
    for (unsigned frame = 3; frame <= 24; ++frame)
        failed |= p4walk_frame(w, &host, frame) != 0 ||
            p4walk_status(w) != P4WALK_RUNNING;
    menu.state = 2;
    failed |= p4walk_frame(w, &host, 25) != 0 ||
        p4walk_status(w) != P4WALK_RUNNING;
    menu.flags = 0x20;
    failed |= p4walk_frame(w, &host, 26) != 0 ||
        p4walk_status(w) != P4WALK_DONE || menu.cursor != 2;
    if (failed) fprintf(stderr, "waitchoice advanced nontext, answered a question, or missed native readiness\n");
    p4walk_free(w);
    return failed;
}

static int wait_for_field_text(unsigned flags, unsigned selection) {
    Menu menu = {.active = 1, .state = selection, .cursor = 0xFFFFu,
                 .flags = flags, .row_state = 0x101u};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("waitchoice\n");
    if (!w) return 1;
    int failed = p4walk_frame(w, &host, 0) != 0; /* No field event owns this row. */
    menu.field_event = 1;
    failed |= p4walk_frame(w, &host, 1) != (1u << P4W_CROSS);
    menu.battle_ready = 1;
    failed |= p4walk_frame(w, &host, 2) != 0; /* Idle battle panes are not field BF text. */
    failed |= p4walk_frame(w, &host, 24) != 0 ||
        p4walk_status(w) != P4WALK_RUNNING;
    menu.battle_ready = 0;
    failed |= p4walk_frame(w, &host, 25) != (1u << P4W_CROSS);
    menu.row_state = 0;
    failed |= p4walk_frame(w, &host, 26) != 0;
    menu.row_state = 0x101u;
    menu.state = 1;
    menu.flags = 0x18;
    menu.cursor = 0;
    menu.count = 3;
    failed |= p4walk_frame(w, &host, 27) != 0;
    menu.state = 2;
    menu.flags = 0x20;
    failed |= p4walk_frame(w, &host, 28) != 0 ||
        p4walk_status(w) != P4WALK_DONE;
    if (failed) fprintf(stderr, "field BF question wait lost ownership, battle, row-state or ready-choice guards\n");
    p4walk_free(w);
    return failed;
}

/* The player is at (200,400). The advance quad is centred there, while its
 * compass marker is at (1000,1000). Without an advance quad the placed marker
 * is at the player, and a downstairs quad at the origin must be ignored. */
static int goal_target(unsigned area) {
    Menu menu = {.goal_fixture = 1, .goal_area = area, .field_ids = 40,
                 .player_x = 0x43480000u, .player_z = 0x43C80000u};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("walk:goal,10\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 3; ++frame) p4walk_frame(w, &host, frame);
    int failed = p4walk_status(w) != P4WALK_DONE;
    if (failed) fprintf(stderr, "floor goal did not resolve for area %x\n", area);
    p4walk_free(w);
    return failed;
}

static int entry_goal(void) {
    Menu menu = {.goal_fixture = 1, .goal_area = 0x37FFu, .field_ids = 40};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("walk:entry,10\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    int failed = p4walk_status(w) != P4WALK_RUNNING;
    menu.goal_area = 0x37FEu; /* Downstairs quad at the origin, not the placed upstairs marker. */
    for (unsigned frame = 1; frame < 4; ++frame) p4walk_frame(w, &host, frame);
    failed |= p4walk_status(w) != P4WALK_DONE;
    p4walk_free(w);

    menu.player_x = menu.player_z = 0x42C80000u; /* Start away from the downstairs quad. */
    w = parse("walk:entry,10\npress:triangle\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    menu.goal_control_off = 1;
    menu.field_ids = 0x00010014u;
    failed |= !(p4walk_frame(w, &host, 1) & (1u << P4W_TRIANGLE));
    for (unsigned frame = 2; frame < 10; ++frame) p4walk_frame(w, &host, frame);
    failed |= p4walk_status(w) != P4WALK_DONE;
    p4walk_free(w);
    if (failed) fprintf(stderr, "downstairs goal used another marker or followed a field transition\n");
    return failed;
}

/* A battle can remove control without changing the floor. A stair can change
 * only the dungeon step, or the field/room, before control returns. */
static int goal_transition(unsigned have_step, unsigned next_ids, unsigned next_step) {
    Menu menu = {.goal_fixture = 1, .goal_area = 0x37FFu, .field_ids = 40,
                 .step_present = have_step, .step = 11};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("battleskip:on\nwalk:goal,10\npress:triangle\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    menu.goal_control_off = 1;
    for (unsigned frame = 1; frame < 4; ++frame) {
        unsigned held = p4walk_frame(w, &host, frame);
        if (p4walk_status(w) != P4WALK_RUNNING || (held & (1u << P4W_TRIANGLE))) {
            fprintf(stderr, "goal retired during an unchanged-floor battle\n");
            p4walk_free(w);
            return 1;
        }
    }
    menu.field_ids = next_ids;
    menu.step = next_step;
    unsigned held = p4walk_frame(w, &host, 4);
    int failed = !(held & (1u << P4W_TRIANGLE));
    for (unsigned frame = 5; frame < 12; ++frame) p4walk_frame(w, &host, frame);
    failed |= p4walk_status(w) != P4WALK_DONE;
    if (failed) fprintf(stderr, "goal followed a stair into another field/floor\n");
    p4walk_free(w);
    return failed;
}

static int goal_scopes_are_fresh(void) {
    Menu menu = {.goal_fixture = 1, .goal_area = 0x37FFu, .field_ids = 40,
                 .step_present = 1, .step = 11};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("walk:goal,10\nwalk:goal,10\npress:triangle\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    menu.step = 12;
    unsigned held = p4walk_frame(w, &host, 1);
    int failed = (held & (1u << P4W_TRIANGLE)) != 0 || p4walk_status(w) != P4WALK_RUNNING;
    menu.step = 13;
    held = p4walk_frame(w, &host, 2);
    failed |= !(held & (1u << P4W_TRIANGLE));
    for (unsigned frame = 3; frame < 10; ++frame) p4walk_frame(w, &host, frame);
    failed |= p4walk_status(w) != P4WALK_DONE;
    if (failed) fprintf(stderr, "successive goals reused the previous floor scope\n");
    p4walk_free(w);
    return failed;
}

static int long_battle_plan(unsigned character) {
    static const unsigned media[] = {
        P4W_CIRCLE, P4W_DOWN, P4W_CROSS, P4W_DOWN, P4W_DOWN, P4W_CROSS, P4W_CROSS,
        P4W_CIRCLE, P4W_CIRCLE, P4W_CIRCLE, P4W_CROSS, P4W_CROSS
    };
    static const unsigned wind[] = {
        P4W_CIRCLE, P4W_DOWN, P4W_CROSS, P4W_CROSS, P4W_CROSS
    };
    static const unsigned guard[] = {
        P4W_CIRCLE, P4W_CIRCLE, P4W_CIRCLE, P4W_UP, P4W_CROSS
    };
    const unsigned *expected = media;
    size_t count = sizeof media / sizeof media[0];
    if (character == 2) {
        expected = wind;
        count = sizeof wind / sizeof wind[0];
    } else if (character == 3) {
        expected = guard;
        count = sizeof guard / sizeof guard[0];
    }
    Menu menu = {.battle_ready = 1, .action_state = 5, .battle_character = character};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse(
        "battle:first1=circle,down,cross,down,down,cross,cross,circle,circle,circle,cross,cross;"
        "first2=circle,down,cross,cross,cross;first3=circle,circle,circle,up,cross;"
        "plan1=circle,down,cross,cross,cross,circle,circle,circle,up,cross;"
        "plan2=circle,down,cross,cross,cross;plan3=circle,circle,circle,up,cross;"
        "plan=cross,cross,cross,circle,circle,circle,cross,cross;skip=0;delay=80\n"
        "wait:1000\n");
    if (!w) return 1;
    unsigned previous = 0;
    size_t seen = 0;
    int failed = 0;
    for (unsigned frame = 0; frame < 300 && seen < count; ++frame) {
        uint32_t mask = p4walk_frame(w, &host, frame);
        if (mask && !previous) {
            failed |= mask != (1u << expected[seen]) || frame != 80u + (unsigned)seen * 14u;
            ++seen;
        }
        previous = mask;
    }
    failed |= seen != count;
    if (failed) fprintf(stderr, "full Media/Wind/Guard battle payload lost for character %u\n", character);
    p4walk_free(w);
    return failed;
}

static int selected_battle_menu(unsigned character) {
    Menu menu = {.battle_ready = 1, .action_state = 5,
                 .battle_character = character == 1 ? 2 : 1};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    char script[64];
    snprintf(script, sizeof script, "waitbtl:menu:%u\npress:triangle\n", character);
    P4Walk *w = parse(script);
    if (!w) return 1;
    int failed = 0;
    for (unsigned frame = 0; frame < 60; ++frame)
        failed |= p4walk_frame(w, &host, frame) != 0 || p4walk_status(w) != P4WALK_RUNNING;
    /* Character identity, not the first slot in the party chain, selects the menu. */
    menu.battle_character = character;
    for (unsigned frame = 60; frame < 80; ++frame)
        failed |= p4walk_frame(w, &host, frame) != 0;
    /* Even the selected actor's TARGET interrupts the fresh 40 COMMAND frames. */
    menu.action_state = 6;
    for (unsigned frame = 80; frame < 90; ++frame)
        failed |= p4walk_frame(w, &host, frame) != 0;
    menu.action_state = 5;
    for (unsigned frame = 90; frame < 129; ++frame)
        failed |= p4walk_frame(w, &host, frame) != 0 || p4walk_status(w) != P4WALK_RUNNING;
    failed |= !(p4walk_frame(w, &host, 129) & (1u << P4W_TRIANGLE));
    for (unsigned frame = 130; frame < 140; ++frame) p4walk_frame(w, &host, frame);
    failed |= p4walk_status(w) != P4WALK_DONE;
    if (failed) fprintf(stderr, "character %u wait accepted another actor or stale readiness\n", character);
    p4walk_free(w);
    return failed;
}

static int wait_battle_help(void) {
    Menu menu = {.active = 1, .state = 0xFFFF, .cursor = 0xFFFF,
                 .flags = 0x200203, .row_state = 0x00000101,
                 .battle_ready = 1, .action_state = 1};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("waitbtl:menu\n");
    if (!w) return 1;
    unsigned presses[2] = {0, 0};
    int failed = 0;
    for (unsigned frame = 0; frame < 220; ++frame) {
        if (frame == 24) { menu.flags = 0x200204; menu.action_state = 8; }
        if (frame == 48) menu.flags = 0x200103; /* Opening is not active text. */
        if (frame == 72) { menu.flags = 0x200204; menu.action_state = 5; }
        if (frame == 132) menu.action_state = 6;
        if (frame == 156) { menu.state = 2; menu.count = 3; menu.flags |= 0x20; menu.action_state = 1; } /* A live choice, not text-only. */
        if (frame == 180) { menu.active = 0; menu.action_state = 5; }
        uint32_t held = p4walk_frame(w, &host, frame);
        if (frame < 48) {
            presses[frame / 24] += held != 0;
            failed |= (held & ~(1u << P4W_CROSS)) != 0;
        } else {
            failed |= held != 0; /* Never confirm COMMAND, TARGET or a choice pane. */
        }
        if (frame < 219) failed |= p4walk_status(w) != P4WALK_RUNNING;
    }
    failed |= !presses[0] || !presses[1] || p4walk_status(w) != P4WALK_DONE;
    if (failed) fprintf(stderr, "battle Help skip did not preserve the real fresh command turn\n");
    p4walk_free(w);
    return failed;
}

static int legacy_idle_battle_text(void) {
    Menu menu = {.active = 1, .state = 0, .cursor = 0xFFFF, .count = 0,
                 .flags = 0x200204, .row_state = 0x00000101,
                 .battle_ready = 1, .action_state = 22};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("waitbtl:menu\n");
    if (!w) return 1;
    int failed = 0;
    for (unsigned frame = 0; frame < 88; frame++) {
        if (frame == 48) menu.action_state = 5;
        failed |= p4walk_frame(w, &host, frame) != 0;
        failed |= p4walk_status(w) != (frame < 87 ? P4WALK_RUNNING : P4WALK_DONE);
    }
    p4walk_free(w);
    if (failed) fprintf(stderr, "ordinary battle wait changed idle-selection input timing\n");
    return failed;
}


static int skip_evt_message(void) {
    Menu menu = {.active = 1, .state = 0xFFFF, .cursor = 0xFFFF,
                 .flags = 0x200203, .row_state = 0x00000101,
                 .evt_phase = 3, .evt_active = 1, .evt_wait = 2,
                 .evt_message = 15, .evt_handle = 7, .window_handle = 7};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("skipevtmsg:16:0\nwait:600\n");
    if (!w) return 1;
    unsigned presses[2] = {0, 0};
    int failed = 0;
    for (unsigned frame = 0; frame < 745; ++frame) {
        if (frame == 24) menu.evt_phase = 4;
        if (frame == 48) menu.window_handle = 8; /* Another live window is not this EVT's text. */
        if (frame == 72) { menu.window_handle = 7; menu.evt_message = 16; menu.row_index = 1; }
        if (frame == 96) { menu.state = 2; menu.count = 3; menu.flags |= 0x20; } /* Never pick a choice's default answer. */
        if (frame == 120) { menu.state = 0xFFFF; menu.count = 0; menu.flags = 0x200103; }
        if (frame == 144) { menu.flags = 0x200203; menu.row_index = 0; } /* Stop while the target is typing. */
        if (frame == 200) { menu.evt_message = 18; menu.row_index = 7; }
        uint32_t held = p4walk_frame(w, &host, frame);
        if ((frame >= 24 && frame < 48) || (frame >= 72 && frame < 96)) {
            presses[frame >= 72] += held != 0;
            failed |= (held & ~(1u << P4W_CROSS)) != 0;
        } else {
            failed |= held != 0;
        }
        if (frame < 744) failed |= p4walk_status(w) != P4WALK_RUNNING;
    }
    failed |= !presses[0] || !presses[1] || p4walk_status(w) != P4WALK_DONE;
    p4walk_free(w);
    /* The exact target is ready except selection.state zero still prepares a choice. */
    menu.evt_message = 16;
    menu.row_index = 0;
    menu.flags = 0x200204;
    menu.state = 0;
    w = parse("timeout:4\nskipevtmsg:16:0\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 8; ++frame) failed |= p4walk_frame(w, &host, frame) != 0;
    failed |= p4walk_status(w) != P4WALK_FAILED;
    p4walk_free(w);
    menu.state = 0xFFFF;
    menu.evt_message = 18;
    menu.row_index = 7;
    menu.evt_phase = 0;
    w = parse("timeout:4\nskipevtmsg:16:0\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 8; ++frame) failed |= p4walk_frame(w, &host, frame) != 0;
    failed |= p4walk_status(w) != P4WALK_FAILED;
    p4walk_free(w);
    if (failed) fprintf(stderr, "semantic EVT message fence advanced a choice, wrong handle or target row\n");
    return failed;
}

static int state_width(const char *script, uint32_t first, uint32_t second) {
    Menu menu = {.step = first};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse(script);
    if (!w) return 1;
    int failed = p4walk_frame(w, &host, 0) != 0 || p4walk_status(w) != P4WALK_RUNNING;
    menu.step = second;
    failed |= p4walk_frame(w, &host, 1) != 0 || p4walk_status(w) != P4WALK_DONE;
    p4walk_free(w);
    if (failed) fprintf(stderr, "state fence changed width, comparison or input ownership\n");
    return failed;
}

static int task_button(const char *script, unsigned button, unsigned initially_present) {
    Menu menu = {.task_fixture = 1, .task_present = initially_present};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse(script);
    if (!w) return 1;
    int failed = 0;
    for (unsigned frame = 0; frame < 50; frame++) {
        if (frame == 30) menu.task_present = !initially_present;
        unsigned held = p4walk_frame(w, &host, frame);
        unsigned expected = frame < 30 && frame % 24 < 3 ? button : 0;
        failed |= held != expected;
        failed |= p4walk_status(w) != (frame < 30 ? P4WALK_RUNNING : P4WALK_DONE);
    }
    p4walk_free(w);
    if (failed) fprintf(stderr, "task fence lost its button, completed early or pressed through the target\n");
    return failed;
}

static int shuffle_result(unsigned popup, unsigned modal) {
    Menu menu = {.battle_ready = 1, .action_state = 13, .shuffle_fixture = 1, .persona_popup = popup};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse(modal ?
        "battle:plan=cross;skip=0;delay=80;persona=modal\nwaittask:battle:off\n" :
        "battle:plan=cross;skip=0;delay=80\nwaittask:battle:off\n");
    if (!w) return 1;
    unsigned confirms = 0, moves = 0;
    int failed = 0;
    for (unsigned frame = 0; frame < 100; frame++) {
        if (frame == 96) menu.battle_ready = 0;
        unsigned held = p4walk_frame(w, &host, frame);
        if (frame < 96) {
            confirms += (held & (1u << P4W_CROSS)) != 0;
            moves += (held & ~(1u << P4W_CROSS)) != 0;
            if (popup && modal) failed |= (held & ~(1u << P4W_CROSS)) != 0;
            failed |= p4walk_status(w) != P4WALK_RUNNING;
        } else {
            failed |= held != 0 || p4walk_status(w) != P4WALK_DONE;
        }
    }
    failed |= !confirms || ((!popup || !modal) && !moves);
    p4walk_free(w);
    if (failed) fprintf(stderr, "shuffle modal lost Cross priority or made a Persona reward mandatory\n");
    return failed;
}

static int wait_enemy_defeated(void) {
    Menu menu = {.enemy_fixture = 1, .enemy_id = 272, .enemy_hp = 100, .other_enemy_hp = 50};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("waitbtl:dead:0x110\npress:triangle\n");
    if (!w) return 1;
    int failed = p4walk_frame(w, &host, 0) != 0 || p4walk_status(w) != P4WALK_RUNNING;
    menu.battle_ready = 1;
    failed |= p4walk_frame(w, &host, 1) != 0 || p4walk_status(w) != P4WALK_RUNNING;
    menu.enemy_fixture = 2; /* Defeat cleanup can leave a live battle task and only party actions. */
    failed |= p4walk_frame(w, &host, 2) != 0 || p4walk_status(w) != P4WALK_RUNNING;
    menu.enemy_fixture = 1;
    menu.enemy_id = 273;
    menu.enemy_hp = menu.other_enemy_hp = 0;
    failed |= p4walk_frame(w, &host, 3) != 0 || p4walk_status(w) != P4WALK_RUNNING;
    menu.enemy_id = 272;
    menu.enemy_hp = 0x12340000u; /* Nonzero SP must not make zero u16 HP look alive. */
    menu.other_enemy_hp = 50;
    failed |= p4walk_frame(w, &host, 4) != 0 || p4walk_status(w) != P4WALK_RUNNING;
    menu.other_enemy_hp = 0;
    failed |= !(p4walk_frame(w, &host, 5) & (1u << P4W_TRIANGLE));
    for (unsigned frame = 6; frame < 14; frame++) p4walk_frame(w, &host, frame);
    failed |= p4walk_status(w) != P4WALK_DONE;
    p4walk_free(w);
    if (failed) fprintf(stderr, "enemy-defeat wait accepted absent/live enemies or sent input before observing every match defeated\n");
    return failed;
}

static int charged_guard(void) {
    static const struct {
        unsigned fixture, id, hp, buffs, other_hp, other_buffs, guard;
    } cases[] = {
        {1, 259, 100, 0x40000, 0, 0, 1},
        {1, 259, 100, 0x80000, 0, 0, 1},
        {1, 259, 100, 0x1F, 0, 0, 0},
        {1, 259, 0x12340000, 0x40000, 0, 0, 0},
        {1, 273, 100, 0x40000, 0, 0, 0},
        {2, 259, 100, 0x40000, 0, 0, 0},
        {1, 259, 0, 0, 100, 0x40000, 1}
    };
    int failed = 0;
    for (unsigned n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        Menu menu = {.battle_ready = 1, .action_state = 5, .battle_character = 3,
                     .enemy_fixture = cases[n].fixture, .enemy_id = cases[n].id,
                     .enemy_hp = cases[n].hp, .enemy_buffs = cases[n].buffs,
                     .other_enemy_hp = cases[n].other_hp, .other_enemy_buffs = cases[n].other_buffs};
        P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
        P4Walk *w = parse("battle:first3=triangle;plan=square;guard=0x103;skip=0;delay=0\nwaittask:battle:off\n");
        if (!w) return 1;
        failed |= p4walk_frame(w, &host, 0) != (1u << (cases[n].guard ? P4W_CIRCLE : P4W_TRIANGLE));
        p4walk_free(w);
    }
    for (unsigned pc = 1; pc <= 8; pc++) {
        Menu menu = {.battle_ready = 1, .action_state = 5, .battle_character = pc,
                     .enemy_fixture = 1, .enemy_id = 259, .enemy_hp = 100, .enemy_buffs = 0x40000};
        P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
        P4Walk *w = parse("battle:first=triangle;plan=square;guard=259:2,3,4;skip=0;delay=0\nwaittask:battle:off\n");
        if (!w) return 1;
        failed |= p4walk_frame(w, &host, 0) != (1u << (pc >= 2 && pc <= 4 ? P4W_CIRCLE : P4W_TRIANGLE));
        p4walk_free(w);
    }
    {
        Menu menu = {.battle_ready = 1, .action_state = 5, .battle_character = 8,
                     .enemy_fixture = 1, .enemy_id = 259, .enemy_hp = 100, .enemy_buffs = 0x80000};
        P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
        P4Walk *w = parse("battle:first=triangle;plan=square;guard=0x103:8;skip=0;delay=0\nwaittask:battle:off\n");
        if (!w) return 1;
        failed |= p4walk_frame(w, &host, 0) != (1u << P4W_CIRCLE);
        p4walk_free(w);
    }
    Menu menu = {.battle_ready = 1, .action_state = 5, .battle_character = 3,
                 .enemy_fixture = 1, .enemy_id = 259, .enemy_hp = 100, .enemy_buffs = 0x40000};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("battle:first3=triangle;plan=square;guard=259;skip=0;delay=0\nwaittask:battle:off\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 160; frame++) {
        if (frame == 10) menu.enemy_buffs = 0; /* Never interrupt an armed input plan. */
        if (frame == 40 || frame == 70 || frame == 100) menu.action_state = 1;
        if (frame == 50 || frame == 80 || frame == 110) menu.action_state = 5;
        if (frame == 110) menu.enemy_buffs = 0x80000;
        if (frame == 150) menu.battle_ready = 0;
        unsigned held = p4walk_frame(w, &host, frame);
        if (menu.action_state != 5) continue;
        unsigned expected = 0;
        if (frame < 5 || (frame >= 110 && frame < 115)) expected = 1u << P4W_CIRCLE;
        if ((frame >= 14 && frame < 19) || (frame >= 124 && frame < 129)) expected = 1u << P4W_UP;
        if ((frame >= 28 && frame < 33) || (frame >= 138 && frame < 143)) expected = 1u << P4W_CROSS;
        if (frame >= 50 && frame < 55) expected = 1u << P4W_TRIANGLE;
        if (frame >= 80 && frame < 85) expected = 1u << P4W_SQUARE;
        failed |= held != expected;
        failed |= p4walk_status(w) != (frame < 150 ? P4WALK_RUNNING : P4WALK_DONE);
    }
    p4walk_free(w);
    if (failed) fprintf(stderr, "charge Guard consumed a first plan, changed an armed macro, or accepted unrelated/dead/absent enemies\n");
    return failed;
}

static int character_repeat_plan(unsigned character) {
    Menu menu = {.battle_ready = 1, .action_state = 5, .battle_character = character,
                 .enemy_fixture = 1, .enemy_id = 259, .enemy_hp = 100};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("battle:first=triangle;first3=l1;first8=r1;plan=square;plan3=left,cross;plan8=right,cross;guard=259;delay=0\nwaittask:battle:off\n");
    int failed = 0;
    unsigned first = character == 3 ? P4W_L1 : character == 8 ? P4W_R1 : P4W_TRIANGLE;
    unsigned repeat = character == 3 ? P4W_LEFT : character == 8 ? P4W_RIGHT : P4W_SQUARE;
    if (!w) return 1;
    for (unsigned frame = 0; frame < 160; frame++) {
        if (frame == 40 || frame == 70 || frame == 120) menu.action_state = 1;
        if (frame == 50 || frame == 80 || frame == 130) menu.action_state = 5;
        if ((frame == 51 || frame == 131) && character != 1) menu.action_state = 6;
        if (frame == 80) menu.enemy_buffs = 0x40000;
        if (frame == 130) menu.enemy_buffs = 0;
        if (frame == 150) menu.battle_ready = 0;
        unsigned held = p4walk_frame(w, &host, frame);
        if (menu.action_state != 5 && menu.action_state != 6) continue;
        unsigned expected = 0;
        if (frame < 5) expected = 1u << first;
        if ((frame >= 50 && frame < 55) || (frame >= 130 && frame < 135)) expected = 1u << repeat;
        if (character != 1 && ((frame >= 64 && frame < 69) || (frame >= 144 && frame < 149)))
            expected = 1u << P4W_CROSS;
        if (frame >= 80 && frame < 85) expected = 1u << P4W_CIRCLE;
        if (frame >= 94 && frame < 99) expected = 1u << P4W_UP;
        if (frame >= 108 && frame < 113) expected = 1u << P4W_CROSS;
        if (frame < 150) failed |= held != expected;
        failed |= p4walk_status(w) != (frame < 150 ? P4WALK_RUNNING : P4WALK_DONE);
    }
    p4walk_free(w);
    if (failed) fprintf(stderr, "character %u repeat changed first/common input, lost TARGET continuation, or failed to resume after Guard\n", character);
    return failed;
}

static unsigned g_turn_asked = 0xFFFFFFFFu;

static int host_turn(void *user, unsigned n, char *line, size_t size) {
    (void)user;
    g_turn_asked = n;
    if (n != 2) return 0;
    snprintf(line, size, "enemy:259=r1");
    return 1;
}

/* battleturn: lines supply the fresh commands in order ahead of a charge Guard, a retry keeps its line,
 * then the host's callback (when set) and finally the driver's own input continue. */
static int battle_turns(int with_host) {
    Menu menu = {.battle_ready = 1, .action_state = 5, .battle_character = 3,
                 .enemy_fixture = 1, .enemy_id = 259, .enemy_hp = 100, .enemy_buffs = 0x40000};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL, with_host ? host_turn : NULL};
    P4Walk *w = parse("battle:plan=square;guard=259;skip=0;delay=0\nbattleturn:triangle\nbattleturn:enemy:259=l1\nwaittask:battle:off\n");
    if (!w) return 1;
    unsigned seen[8], count = 0, last = 0;
    int failed = 0;
    g_turn_asked = 0xFFFFFFFFu;
    for (unsigned frame = 0; frame < 1000; frame++) {
        if (frame == 700 || frame == 760) menu.action_state = 1;  /* line 1 at 0 and again at 602 (it did not take), line 2 at 710, then 770 */
        if (frame == 710 || frame == 770) menu.action_state = 5;
        if (frame == 990) menu.battle_ready = 0;
        unsigned held = p4walk_frame(w, &host, frame);
        if (menu.action_state != 5) held = 0;   /* the driver's Cross through enemy turns */
        if (held && held != last && count < 8) seen[count++] = held;
        last = held;
    }
    const unsigned want[] = {1u << P4W_TRIANGLE, 1u << P4W_TRIANGLE, 1u << P4W_L1,
                             with_host ? 1u << P4W_R1 : 1u << P4W_CIRCLE};
    failed |= count < 4 || p4walk_status(w) != P4WALK_DONE;
    for (unsigned n = 0; n < 4 && n < count; n++) failed |= seen[n] != want[n];
    failed |= with_host ? g_turn_asked != 2 : g_turn_asked != 0xFFFFFFFFu;
    p4walk_free(w);
    if (failed) fprintf(stderr, "battleturn lost its order, priority over Guard, retry line or host continuation (host %d)\n", with_host);
    return failed;
}

static int bound_target(void) {
    /* 0: selected, 1: two spatial moves, 2/3: all-target classes, 4..8:
     * absent/dead/KO/ally/stuck, 9: unbound actor, 10: live duplicate ID,
     * 11: reordered live list, 12: malformed cursor, 13: highest party ID. */
    int failed = 0;
    for (unsigned test = 0; test < 14; test++) {
        Menu menu = {.battle_ready = 1, .action_state = 5, .battle_character = test == 9 ? 2u : test == 13 ? 8u : 1u,
                     .target_fixture = 1, .target_count = 3, .target_selected = 0,
                     .target_ids = {259, 272, 273}, .target_hp = {100, 100, 100}};
        if (test == 0) menu.target_selected = 1;
        if (test == 2 || test == 3) menu.target_mode = test - 1;
        if (test == 4) menu.target_ids[1] = 274;
        if (test == 5 || test == 10) menu.target_hp[1] = 0x12340000u;
        if (test == 6) menu.target_bad[1] = 0x80000;
        if (test == 7) menu.target_friendly = 1;
        if (test == 10) menu.target_ids[2] = 272;
        if (test == 11) { menu.target_ids[1] = 273; menu.target_ids[2] = 272; }
        if (test == 12) menu.target_selected = 3;
        P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
        P4Walk *w = parse("battle:first=down,cross;plan=square;target1=0x110;target8=272;delay=0\nwait:1000\n");
        if (!w) return 1;
        unsigned previous = 0, rights = 0, confirms = 0;
        int expect_failure = (test >= 4 && test <= 8) || test == 12;
        for (unsigned frame = 0; frame < 220; frame++) {
            if (frame == 6) menu.action_state = 6;
            unsigned held = p4walk_frame(w, &host, frame), edge = held & ~previous;
            previous = held;
            if (edge & (1u << P4W_RIGHT)) {
                rights++;
                /* Screen order deliberately differs from the candidate array. */
                static const unsigned next[] = {2, 0, 1};
                if (test != 8) menu.target_selected = next[menu.target_selected];
            }
            if (edge & (1u << P4W_CROSS)) {
                confirms++;
                if (test != 9 && !menu.target_mode)
                    failed |= menu.target_ids[menu.target_selected] != 272;
                menu.action_state = 15;
                break;
            }
            if (p4walk_status(w) == P4WALK_FAILED) break;
        }
        failed |= expect_failure ? p4walk_status(w) != P4WALK_FAILED || confirms != 0 :
                                   p4walk_status(w) != P4WALK_RUNNING || confirms != 1;
        if (test == 0 || test == 2 || test == 3 || test == 9) failed |= rights != 0;
        if (test == 1 || test == 13) failed |= rights != 2;
        if (test == 10 || test == 11) failed |= rights != 1;
        p4walk_free(w);
    }
    static const char *bad[] = {"target0=272", "target9=272", "target1=", "target1=0",
                               "target1=65536", "target1=-1", "target1=272junk", "target1=272,273"};
    for (unsigned n = 0; n < sizeof bad / sizeof bad[0]; n++) {
        char script[128], error[160];
        snprintf(script, sizeof script, "battle:plan=cross;%s\n", bad[n]);
        P4Walk *w = p4walk_parse(script, error, sizeof error);
        if (w) { failed = 1; p4walk_free(w); }
    }
    if (failed) fprintf(stderr, "native enemy binding confirmed the wrong/dead target, failed to steer, or changed an unbound actor\n");
    return failed;
}

int main(void) {
    if (select_answer(0, 1) || select_answer(2, 1) || select_answer(1, 1)) return 1;
    if (wait_for_question()) return 1;
    if (wait_for_field_text(0x200102u, 0xFFFFu) ||
        wait_for_field_text(0x200203u, 0) ||
        wait_for_field_text(0x200204u, 0)) return 1;
    if (goal_target(0x37FFu) || goal_target(0x37FEu) || goal_target(0)) return 1;
    if (entry_goal()) return 1;
    if (wait_enemy_defeated()) return 1;
    if (charged_guard()) return 1;
    if (bound_target()) return 1;
    if (character_repeat_plan(1) || character_repeat_plan(3) || character_repeat_plan(8)) return 1;
    if (battle_turns(0) || battle_turns(1)) return 1;
    if (goal_transition(1, 40, 12) || goal_transition(1, 0x00030017u, 11) ||
        goal_transition(0, 0x00030017u, 11) || goal_scopes_are_fresh()) return 1;
    if (long_battle_plan(1) || long_battle_plan(2) || long_battle_plan(3)) return 1;
    if (wait_battle_help()) return 1;
    if (legacy_idle_battle_text()) return 1;
    if (skip_evt_message()) return 1;
    if (selected_battle_menu(1) || selected_battle_menu(3) || selected_battle_menu(8)) return 1;
    if (task_button("skiptask:cmpMain\n", 1u << P4W_CROSS, 0) ||
        task_button("skiptask:cmpMain:triangle\n", 1u << P4W_TRIANGLE, 0) ||
        task_button("skiptask:cmpMain:off:circle\n", 1u << P4W_CIRCLE, 1) ||
        task_button("waittask:cmpMain\n", 0, 0) ||
        task_button("waittask:cmpMain:off\n", 0, 1)) return 1;
    if (state_width("waitstate16:0x01003004==211\n", 0x007E00D2u, 0x008200D3u) ||
        state_width("waitstate16:0x01003004!=211\n", 0x007E00D3u, 0x008200D2u) ||
        state_width("waitstate16:0x01003006==211\n", 0x00D2007Eu, 0x00D30082u) ||
        state_width("waitstate16:0x01003006!=211\n", 0x00D3007Eu, 0x00D20082u) ||
        state_width("waitstate:0x01003004==211\n", 0x007E00D3u, 211) ||
        state_width("waitstate:0x01003004!=211\n", 211, 0x007E00D3u)) return 1;
    if (shuffle_result(0, 0) || shuffle_result(1, 0) ||
        shuffle_result(0, 1) || shuffle_result(1, 1)) return 1;
    const char *bad[] = {"choose:0", "choose:33", "choose:no", "choose:2junk", "waitstep:", "waitstep:x",
                         "skipevtmsg:", "skipevtmsg:16", "skipevtmsg:16:", "skipevtmsg:x:0", "skipevtmsg:16:0junk",
                         "waitstate16:", "waitstate16:0x01003004=211", "waitstate16:0x01003004==65536",
                         "waitstate16:0x01003005==211",
                         "walk:entry:foo", "walk:entry,0", "walk:entry,10junk",
                         "waitbtl:dead:", "waitbtl:dead:0", "waitbtl:dead:65536", "waitbtl:dead:272junk",
                         "battle:plan=cross;plan0=circle", "battle:plan=cross;plan9=circle",
                         "battle:plan=cross;plan3=", "battle:plan=cross;plan3=bogus",
                         "battle:plan3=circle",
                         "battle:plan=cross;guard=", "battle:plan=cross;guard=0", "battle:plan=cross;guard=65536",
                         "battle:plan=cross;guard=259junk",
                         "battle:plan=cross;guard=259:", "battle:plan=cross;guard=259:0", "battle:plan=cross;guard=259:9",
                         "battle:plan=cross;guard=259:2,", "battle:plan=cross;guard=259:2junk", "battle:plan=cross;guard=259::2",
                         "waitbtl:menu:intro",
                         "waitbtl:menu:", "waitbtl:menu:0", "waitbtl:menu:9", "waitbtl:menu:1junk", "waitbtl:menu:1:2"};
    const char *bad_task[] = {"skiptask:", "skiptask::triangle", "skiptask:cmpMain:", "skiptask:cmpMain:off:",
                              "skiptask:cmpMain:invalid", "skiptask:cmpMain:triangle:cross", "waittask:cmpMain:triangle"};
    for (unsigned i = 0; i < sizeof bad_task / sizeof bad_task[0]; ++i) {
        char error[160];
        P4Walk *w = p4walk_parse(bad_task[i], error, sizeof error);
        if (w) { p4walk_free(w); return 1; }
    }
    for (unsigned i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        char error[160];
        P4Walk *w = p4walk_parse(bad[i], error, sizeof error);
        if (w) { p4walk_free(w); return 1; }
    }
    Menu menu = {.active = 1, .state = 2, .count = 2, .flags = 0x20};
    P4WalkHost host = {read32, NULL, &menu, NULL, NULL};
    P4Walk *w = parse("choose:3\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    int failed = p4walk_status(w) != P4WALK_FAILED;
    p4walk_free(w);
    if (failed) return 1;
    memset(&menu, 0, sizeof menu);
    w = parse("timeout:4\nchoose:1\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 8; ++frame) p4walk_frame(w, &host, frame);
    failed = p4walk_status(w) != P4WALK_FAILED;
    p4walk_free(w);
    if (failed) return 1;
    memset(&menu, 0, sizeof menu);
    w = parse("timeout:8\nwaitstep:8\nwait:1\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 5; ++frame) p4walk_frame(w, &host, frame);
    if (p4walk_status(w) != P4WALK_RUNNING) { p4walk_free(w); return 1; }
    menu.step_present = 1;
    menu.step = 7;
    p4walk_frame(w, &host, 5);
    p4walk_frame(w, &host, 6);
    if (p4walk_status(w) != P4WALK_RUNNING) { p4walk_free(w); return 1; }
    menu.step = 8;
    p4walk_frame(w, &host, 7);
    p4walk_frame(w, &host, 8);
    failed = p4walk_status(w) != P4WALK_DONE;
    p4walk_free(w);
    if (failed) return 1;
    memset(&menu, 0, sizeof menu);
    menu.battle_ready = 1;
    menu.action_state = 5;
    w = parse("waitbtl:menu\nwait:1\nwaitbtl:menu\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 50; ++frame) p4walk_frame(w, &host, frame);
    if (p4walk_status(w) != P4WALK_RUNNING) { p4walk_free(w); return 1; }
    menu.battle_ready = 0;
    for (unsigned frame = 50; frame < 60; ++frame) p4walk_frame(w, &host, frame);
    menu.battle_ready = 1;
    for (unsigned frame = 60; frame < 99; ++frame) p4walk_frame(w, &host, frame);
    if (p4walk_status(w) != P4WALK_RUNNING) { p4walk_free(w); return 1; }
    p4walk_frame(w, &host, 99);
    failed = p4walk_status(w) != P4WALK_DONE;
    p4walk_free(w);
    if (failed) return 1;
    memset(&menu, 0, sizeof menu);
    menu.battle_ready = 1;
    menu.action_state = 1; /* AI actor standing by while the battle context is already state 8. */
    w = parse("waitbtl:menu\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 60; ++frame)
        if (p4walk_frame(w, &host, frame)) { p4walk_free(w); return 1; }
    if (p4walk_status(w) != P4WALK_RUNNING) { p4walk_free(w); return 1; }
    menu.action_state = 8; /* AI action is not a player command menu either. */
    for (unsigned frame = 60; frame < 120; ++frame)
        if (p4walk_frame(w, &host, frame)) { p4walk_free(w); return 1; }
    if (p4walk_status(w) != P4WALK_RUNNING) { p4walk_free(w); return 1; }
    menu.action_state = 5;
    for (unsigned frame = 120; frame < 159; ++frame) p4walk_frame(w, &host, frame);
    if (p4walk_status(w) != P4WALK_RUNNING) { p4walk_free(w); return 1; }
    p4walk_frame(w, &host, 159);
    failed = p4walk_status(w) != P4WALK_DONE;
    p4walk_free(w);
    if (failed) return 1;
    menu.action_state = 30; /* Decline All-Out so a weak encounter survives until COMMAND. */
    w = parse("waitbtl:menu\n");
    if (!w) return 1;
    int declined = 0;
    for (unsigned frame = 0; frame < 48; ++frame) {
        uint32_t mask = p4walk_frame(w, &host, frame);
        if (mask & (1u << P4W_CROSS)) { p4walk_free(w); return 1; }
        declined |= (mask & (1u << P4W_CIRCLE)) != 0;
    }
    if (!declined || p4walk_status(w) != P4WALK_RUNNING) { p4walk_free(w); return 1; }
    menu.action_state = 5;
    for (unsigned frame = 48; frame < 87; ++frame) p4walk_frame(w, &host, frame);
    if (p4walk_status(w) != P4WALK_RUNNING) { p4walk_free(w); return 1; }
    p4walk_frame(w, &host, 87);
    failed = p4walk_status(w) != P4WALK_DONE;
    p4walk_free(w);
    if (failed) return 1;
    menu.action_state = 1;
    w = parse("battle:first=down;plan=cross;delay=1\nwait:500\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 60; ++frame)
        if (p4walk_frame(w, &host, frame) & (1u << P4W_DOWN)) { p4walk_free(w); return 1; }
    menu.action_state = 8;
    for (unsigned frame = 60; frame < 120; ++frame)
        if (p4walk_frame(w, &host, frame) & (1u << P4W_DOWN)) { p4walk_free(w); return 1; }
    menu.action_state = 5;
    p4walk_frame(w, &host, 120);
    failed = !(p4walk_frame(w, &host, 121) & (1u << P4W_DOWN));
    p4walk_free(w);
    if (failed) return 1;
    /* A first plan crosses COMMAND -> TARGET without restarting as the common plan. */
    memset(&menu, 0, sizeof menu);
    menu.battle_ready = 1;
    menu.action_state = 5;
    w = parse("battle:first=down,right,cross;plan=left;delay=1\nwait:500\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    if (!(p4walk_frame(w, &host, 1) & (1u << P4W_DOWN))) { p4walk_free(w); return 1; }
    menu.action_state = 6;
    for (unsigned frame = 2; frame < 15; ++frame)
        if (p4walk_frame(w, &host, frame) & ((1u << P4W_RIGHT) | (1u << P4W_CROSS))) { p4walk_free(w); return 1; }
    if (!(p4walk_frame(w, &host, 15) & (1u << P4W_RIGHT))) { p4walk_free(w); return 1; }
    for (unsigned frame = 16; frame < 29; ++frame)
        if (p4walk_frame(w, &host, frame) & (1u << P4W_CROSS)) { p4walk_free(w); return 1; }
    failed = !(p4walk_frame(w, &host, 29) & (1u << P4W_CROSS));
    p4walk_free(w);
    if (failed) return 1;

    /* Confirming TARGET advances to action 15; no remaining direction may leak into it. */
    memset(&menu, 0, sizeof menu);
    menu.battle_ready = 1;
    menu.action_state = 5;
    w = parse("battle:first=down,right;plan=left;delay=1\nwait:500\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    p4walk_frame(w, &host, 1);
    menu.action_state = 15;
    for (unsigned frame = 2; frame < 31; ++frame)
        if (p4walk_frame(w, &host, frame) & (1u << P4W_RIGHT)) { p4walk_free(w); return 1; }
    menu.action_state = 5;
    p4walk_frame(w, &host, 31);
    failed = !(p4walk_frame(w, &host, 32) & (1u << P4W_LEFT));
    p4walk_free(w);
    if (failed) return 1;

    /* Another actor's TARGET cannot inherit the previous actor's armed first plan. */
    memset(&menu, 0, sizeof menu);
    menu.battle_ready = 1;
    menu.action_state = 5;
    w = parse("battle:first=down,right;plan=left;delay=1\nwait:500\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    p4walk_frame(w, &host, 1);
    menu.other_actor = 1;
    menu.action_state = 6;
    for (unsigned frame = 2; frame < 31; ++frame)
        if (p4walk_frame(w, &host, frame) & (1u << P4W_RIGHT)) { p4walk_free(w); return 1; }
    p4walk_free(w);

    /* A direct handoff to another COMMAND must arm that actor's own first plan. */
    memset(&menu, 0, sizeof menu);
    menu.battle_ready = 1;
    menu.action_state = 5;
    w = parse("battle:first=down,right;plan=left;delay=1\nwait:500\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    p4walk_frame(w, &host, 1);
    menu.other_actor = 1;
    p4walk_frame(w, &host, 2);
    failed = !(p4walk_frame(w, &host, 3) & (1u << P4W_DOWN));
    p4walk_free(w);
    if (failed) return 1;

    /* A new driver still requires COMMAND, not a pre-existing TARGET pane. */
    memset(&menu, 0, sizeof menu);
    menu.battle_ready = 1;
    menu.action_state = 6;
    w = parse("battle:first=right;plan=left;delay=1\nwait:500\n");
    if (!w) return 1;
    for (unsigned frame = 0; frame < 60; ++frame)
        if (p4walk_frame(w, &host, frame) & ((1u << P4W_RIGHT) | (1u << P4W_LEFT))) { p4walk_free(w); return 1; }
    menu.action_state = 5;
    p4walk_frame(w, &host, 60);
    failed = !(p4walk_frame(w, &host, 61) & (1u << P4W_RIGHT));
    p4walk_free(w);
    if (failed) return 1;

    /* ACTION_RUNNING is not a target-input exemption. */
    memset(&menu, 0, sizeof menu);
    menu.battle_ready = 1;
    menu.action_state = 5;
    w = parse("battle:first=down,right;plan=left;delay=1\nwait:500\n");
    if (!w) return 1;
    p4walk_frame(w, &host, 0);
    p4walk_frame(w, &host, 1);
    menu.action_state = 6;
    menu.battle_flags = 0x400000u;
    for (unsigned frame = 2; frame < 31; ++frame)
        if (p4walk_frame(w, &host, frame) & (1u << P4W_RIGHT)) { p4walk_free(w); return 1; }
    p4walk_free(w);
    puts("walk_choice: selection, cursor, confirmation, range, timeout, dungeon step, floor-scoped live stair goals, full battle payloads, character-selected fresh readiness, AI exclusion and same-actor TARGET continuation PASS");
    return 0;
}
