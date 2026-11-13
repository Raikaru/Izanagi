/* Synthetic guest state exercises the production player-position reader. */
#include "p4pc/gmem.h"
#include "p4pc/mod_hooks.h"
#include "p4pc/mod_services.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint8_t *p4_guest_base;
void *p4_g2h_slow(uint32_t a) { (void)a; return NULL; }
int p4_settings_native_active(void) { return 0; }
int p4_mod_hook_enable(P4ModHook *hook, int enabled) { (void)hook; (void)enabled; return 0; }
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "position test line %d: %s\n", __LINE__, #c); return 1; } } while (0)
static void put(uint32_t a, uint32_t v) { memcpy(p4_guest_base + a, &v, 4); }
static void real(uint32_t a, float v) { memcpy(p4_guest_base + a, &v, 4); }
int main(void)
{
    p4_guest_base = calloc(1, P4_GUEST_SIZE);
    CHECK(p4_guest_base != NULL);
    /* Field 8:2, sequence 9, root ready, rotate pc in the port heap. */
    put(P4_GPADDR(0xb1a8), 0x1200000); put(0x1200038, 0x1201000);
    put(0x1201004, 9);
    put(0x007d2510, 0x1202000); put(0x1202038, 0x1203000);
    put(0x1203000, 13); put(0x1203018, 8 | (2u << 16));
    put(0x00762ea0, 0x1204000); put(0x1204000, 8); put(0x1204004, 2);
    put(0x00764adc, 0x2100000); memcpy(p4_guest_base + 0x2100000, "rotate pc", 10);
    put(0x2100038, 0x2101000); put(0x2101048, 0x2102000);
    real(0x2102030, -594.5f); real(0x2102038, -813.25f);
    float x = 0, z = 0;
    CHECK(p4_mod_scene_player_position(&x, &z));
    CHECK(x == -594.5f && z == -813.25f);
    put(0x1203000, 12); CHECK(!p4_mod_scene_player_position(&x, &z)); put(0x1203000, 13);
    put(0x00764604, 1); CHECK(!p4_mod_scene_player_position(&x, &z)); put(0x00764604, 0);
    put(0x2101048, P4_GUEST_SIZE - 4); CHECK(!p4_mod_scene_player_position(&x, &z));
    put(0x2101048, 0x2102000); real(0x2102030, NAN); CHECK(!p4_mod_scene_player_position(&x, &z));
    put(0x00764adc, 0); CHECK(!p4_mod_scene_player_position(&x, &z));
    free(p4_guest_base);
    puts("scene position: PASS");
    return 0;
}
