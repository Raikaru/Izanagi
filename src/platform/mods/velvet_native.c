/* Velvet Room command (include/p4pc/mod_velvet.h). The dispatcher override
 * (overrides/002e7ac0.c) asks p4_mod_velvet_offer at the idle root, draws the
 * hint from p4_mod_velvet_hint_text, and on a Square edge creates the Config
 * child and hands it to p4_mod_velvet_host; the registered page then runs
 * hosted (p4_settings_native_host) until Config's step is done. The offer is
 * asked once per Velvet task (or after the command changes), so a denial is
 * logged once, not every frame. */
#include "p4pc/mod_velvet.h"
#include "p4pc/mod_authorize.h"
#include "p4pc/mod_entry.h"
#include "p4pc/native_glyphs.h"
#include "p4pc/settings_native.h"
#include "p4pc/gmem.h"
#include "p4pc/guest.h"
#include "../config/p4cfg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { LABEL_MAX = 48, HINT_MAX = 64, PAGE_KEY_MAX = 128 };
struct P4ModVelvetCommand {
    size_t owner;
    int enabled, operation;
    char page[PAGE_KEY_MAX];
    char label[LABEL_MAX];
};
static P4ModVelvetCommand *command;
static uint32_t hintAddress;
static uint32_t offerTask;
static int offerAnswer, offerAsked;
static uint32_t configTask;
uint32_t p4_mod_velvet_mask;

static void update(void)
{
    p4_mod_velvet_mask = command && command->enabled ? 1u : 0u;
    offerAsked = 0;
}

/* "SQUARE (V)  WEAPON FUSION": the pad button and the first keyboard key bound
 * to Square ("V, none" binds V), then the mod's label, copied into port-owned
 * guest storage. */
static int compose_hint(void)
{
    if (!hintAddress && !(hintAddress = p4_gport_alloc(HINT_MAX, 16))) return -1;
    const char *binding = p4cfg_str("input.kb.square", "V");
    char key[16] = "";
    size_t length = binding ? strcspn(binding, ",") : 0;
    while (length && binding[length - 1] == ' ') --length;
    if (length && length < sizeof key && !(length == 4 && !strncmp(binding, "none", 4))) memcpy(key, binding, length);
    char text[HINT_MAX];
    int size = *key ? snprintf(text, sizeof text, "SQUARE (%s)  %s", key, command->label) : -1;
    if (size <= 0 || size >= HINT_MAX ||
        p4_native_glyphs_validate((const uint8_t *)text, (size_t)size + 1, P4_GLYPHS_RAW, 0)) {
        size = snprintf(text, sizeof text, "SQUARE  %s", command->label);
        if (size <= 0 || size >= HINT_MAX) return -1;
    }
    memcpy(P4_GPTR(char, hintAddress), text, (size_t)size + 1);
    return 0;
}

P4ModVelvetCommand *p4_mod_velvet_add(size_t owner, const char *page, const char *label, int operation)
{
    if (command || configTask || !page || !label) return NULL;
    size_t pageLength = strnlen(page, PAGE_KEY_MAX), labelLength = strnlen(label, LABEL_MAX);
    if (!pageLength || pageLength >= PAGE_KEY_MAX || !labelLength || labelLength >= LABEL_MAX) return NULL;
    if (p4_native_glyphs_validate((const uint8_t *)label, labelLength + 1, P4_GLYPHS_RAW, 0)) return NULL;
    if (operation != -1 && operation != P4_MOD_AUTH_WEAPON_FUSION) return NULL;
    P4ModVelvetCommand *c = calloc(1, sizeof *c);
    if (!c) return NULL;
    *c = (P4ModVelvetCommand){owner, 1, operation, "", ""};
    memcpy(c->page, page, pageLength + 1);
    memcpy(c->label, label, labelLength + 1);
    command = c;
    if (compose_hint()) {
        free(c);
        command = NULL;
        return NULL;
    }
    update();
    return c;
}
int p4_mod_velvet_enable(P4ModVelvetCommand *c, int enabled)
{
    if (!c || c != command || configTask) return -1;
    c->enabled = !!enabled;
    update();
    return 0;
}
int p4_mod_velvet_remove(P4ModVelvetCommand *c)
{
    if (!c || c != command || configTask) return -1;
    free(c);
    command = NULL;
    update();
    return 0;
}
void p4_mod_velvet_remove_owner(size_t owner)
{
    if (!command || command->owner != owner) return;
    if (configTask) {
        fprintf(stderr, "p4pc: mods: a Velvet command's owner left while its page was open\n");
        fflush(stderr);
        _Exit(EXIT_FAILURE);
    }
    free(command);
    command = NULL;
    update();
}
int p4_mod_velvet_open(void)
{
    return configTask != 0;
}

/* Asked once per idle streak of the root: p4_mod_velvet_rearm (called while the
 * root is not waiting for input) makes the next idle frame ask again, so a
 * denial is logged once per visit to the root, not every frame. The hint is
 * composed again then, so a key rebound in Config shows. */
int p4_mod_velvet_offer(uint32_t task)
{
    if (!p4_mod_velvet_mask) return 0;
    if (offerAsked && offerTask == task) return offerAnswer;
    offerTask = task;
    offerAsked = 1;
    if (compose_hint()) return offerAnswer = 0;
    offerAnswer = command->operation != P4_MOD_AUTH_WEAPON_FUSION ||
        p4_mod_authorize_weapon_fusion(P4_MOD_WEAPON_FUSION_OFFER, 0, 0, 0, 0, NULL, 0);
    return offerAnswer;
}
void p4_mod_velvet_rearm(void)
{
    offerAsked = 0;
}
uint32_t p4_mod_velvet_hint_text(void)
{
    return p4_mod_velvet_mask ? hintAddress : 0;
}
/* The Velvet task's own Config child: cmpConfigCreate 0x0035C690 (parent,
 * mode; returns the child task's guest address), cmpConfigIsStepDone
 * 0x0035C7D0, kwlnTaskDestroyWithHierarchy 0x00452080. Mode 1 (title): it
 * activates nLine, so its close completes outside Camp (mode 0's waits on an
 * inactive nLine forever), and it draws Config's own opaque backdrop. */
#define VELVET_CONFIG_MODE 1
extern int32_t cmpConfigCreate(void *parent, int32_t mode);
extern int32_t cmpConfigIsStepDone(void *task);
extern uint8_t kwlnTaskDestroyWithHierarchy(void *task);

int p4_mod_velvet_open_page(uint32_t task)
{
    if (!p4_mod_velvet_mask || configTask || !task) return -1;
    uint32_t config = (uint32_t)cmpConfigCreate(P4_G2H(task), VELVET_CONFIG_MODE);
    if (!config) return -1;
    if (p4_settings_native_host(config, command->page)) {
        kwlnTaskDestroyWithHierarchy(P4_G2H(config));
        return -1;
    }
    configTask = config;
    fprintf(stderr, "p4pc: mods: Velvet command opened page %s\n", command->page);
    return 0;
}
int p4_mod_velvet_page_step(void)
{
    if (configTask && !cmpConfigIsStepDone(P4_G2H(configTask))) return 1;
    if (configTask) kwlnTaskDestroyWithHierarchy(P4_G2H(configTask));
    configTask = 0;
    /* Ask again when the root resumes: the page may have changed what is allowed. */
    offerAsked = 0;
    fprintf(stderr, "p4pc: mods: Velvet command page closed\n");
    return 0;
}
