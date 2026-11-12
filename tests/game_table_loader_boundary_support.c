/* Reuse the native item fixture's isolated platform providers while linking
 * the real item service. Its independent test entry is not run here. */
#define main p4_items_fixture_unused_main
#include "game_items_test.c"
#undef main

int item_boundary_guest_map(void) { return p4t_guest_map(0x00764000u, 0x1000u); }
uint32_t item_boundary_guest_base(void) { return P4_G32(0x007642a0u); }
