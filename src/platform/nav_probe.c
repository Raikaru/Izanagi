/* The walk script planner's view of the field's collision world (include/p4pc/walk_script.h, nav_*): the game's own tests, the ones the player
 * controller makes every step.
 *   p4pc_nav_blocked: FldFrame_CollideSphereSelected, a sphere of the player's radius against the field's collision world;
 *   p4pc_nav_ground:  FldFrame_RaycastField, a vertical line through the floor.
 * Called with host pointers to host-side structs: on the 32-bit builds a host pointer is a guest address, on the 64-bit ones the mirror functions take host
 * pointers in registers (docs/port64.md) and only read and write through them. Without the probes the walker keeps the cell grid and cannot route through a
 * castle floor (castle_3f_041864 ended in a battle field on the 64-bit builds). */
#include <stdint.h>
#include <string.h>

#include "p4pc/gmem.h"
#include "p4pc/walk_script.h"

extern void p4pc_pad_set_nav(int (*blocked)(void *, float, float, float, float), int (*ground)(void *, float, float, float, float, float *), int (*ready)(void *));

static int navReady(void *user)
{
    uint32_t scene = P4_G32(P4_GPADDR(0x9db0));

    (void)user;
    /* fieldSceneUnload clears sceneTask before freeing the collision resources;
     * fieldSceneLoadStep creates it only after those resources finish loading.
     * The old template pointers and rotate-pc task can outlive the scene. */
    return scene != 0 && P4_G32(scene + 0x1Cu) != 0;
}

#if defined(__i386__) || defined(_M_IX86) || defined(P4_GUEST64)
typedef struct { float x, y, z; } NavV3;

extern int32_t FldFrame_CollideSphereSelected(const NavV3 *point, float radius, NavV3 *normal, NavV3 *vector);
extern int32_t FldFrame_RaycastField(const NavV3 *line, NavV3 *hitPointDst);

int p4pc_nav_blocked(void *user, float x, float y, float z, float radius)
{
    NavV3 point __attribute__((aligned(16))) = { x, y, z };
    NavV3 normal __attribute__((aligned(16))) = { 0, 0, 0 }, vector __attribute__((aligned(16))) = { 0, 0, 0 };

    (void)user;
    return FldFrame_CollideSphereSelected(&point, radius, &normal, &vector) != 0;
}

int p4pc_nav_ground(void *user, float x, float z, float y_hi, float y_lo, float *y)
{
    NavV3 line[2] __attribute__((aligned(16)));
    NavV3 hit __attribute__((aligned(16))) = { 0, 0, 0 };

    (void)user;
    line[0].x = x; line[0].y = y_hi; line[0].z = z;
    line[1].x = x; line[1].y = y_lo; line[1].z = z;
    if (FldFrame_RaycastField(line, &hit) != 1) {
        return 0;
    }
    *y = hit.y;
    return 1;
}
#else
int p4pc_nav_blocked(void *user, float x, float y, float z, float radius) { (void)user; (void)x; (void)y; (void)z; (void)radius; return 0; }
int p4pc_nav_ground(void *user, float x, float z, float y_hi, float y_lo, float *y) { (void)user; (void)x; (void)z; (void)y_hi; (void)y_lo; (void)y; return 0; }
#endif

static void __attribute__((constructor)) navRegister(void)
{
    p4pc_pad_set_nav(p4pc_nav_blocked, p4pc_nav_ground, navReady);
}
