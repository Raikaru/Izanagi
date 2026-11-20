/* Exercise the battle camera curve with hand-written synthetic poses.
 * Position and quaternion output words are checked for all four keys. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "guest_test.h"
#include "p4pc/hostmath.h"
#include "p4pc/image.h"

void mdlBlendTranslationRotation(float *result, float *from, float *to, float blend);
void btlMainCurveSetThreePoses(unsigned short *curve, float *firstPose, float *middlePose, float *lastPose, unsigned short mode);

#define POSE_WORDS 7
#define KEY_WORDS (4 * POSE_WORDS)

static const uint32_t expected_keys[KEY_WORDS] = {
    0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u,
    0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u,
    0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u,
    0x40c00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u,
};

static int test_main(int argc, char **argv)
{
    const char *elf = getenv("P4PC_ELF");
    float *poses;
    unsigned char *curve;
    uint32_t key[KEY_WORDS];
    int i, failures = 0;

    (void)argc;
    (void)argv;
    if (elf == NULL || elf[0] == '\0') {
        fprintf(stderr, "battle_camera_curve_test: P4PC_ELF unset, skipped\n");
        return 77;
    }
    p4pc_load_image(elf);   /* the curve fractions and sine coefficients are retail data */
    p4pc_apply_patches();
    if (!p4pc_fpu_ee_mode()) {
        fprintf(stderr, "battle_camera_curve_test: P4PC_EE_ROUNDING=near, skipped\n");
        return 77;
    }
    poses = p4t_galloc(3 * POSE_WORDS * sizeof *poses, 16);
    curve = p4t_galloc(0x100, 16);
    if (poses == NULL || curve == NULL) {
        return 1;
    }
    memcpy(poses, expected_keys, POSE_WORDS * sizeof *poses);
    memcpy(poses + 2 * POSE_WORDS, expected_keys + 3 * POSE_WORDS, POSE_WORDS * sizeof *poses);
    mdlBlendTranslationRotation(poses + POSE_WORDS, poses, poses + 2 * POSE_WORDS, 0.5f);
    poses[POSE_WORDS + 1] = poses[1];
    memset(curve, 0, 0x100);
    btlMainCurveSetThreePoses((unsigned short *)curve, poses, poses + POSE_WORDS, poses + 2 * POSE_WORDS, 1);
    memcpy(key, curve + 4, sizeof key);
    for (i = 0; i < KEY_WORDS; i++) {
        if (key[i] != expected_keys[i]) {
            printf("FAIL key %d word %d: %08x, expected %08x\n", i / POSE_WORDS, i % POSE_WORDS, (unsigned)key[i], (unsigned)expected_keys[i]);
            failures++;
        }
    }
    if (failures != 0) {
        return 1;
    }
    puts("synthetic battle camera curve: four keys match (28 words)");
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
