/* The battle opening camera's curve keys, bit for bit against retail. btlAct_NOP (001ccdb0) blends its first and last poses at one half for the
 * middle pose (mdlBlendTranslationRotation 001ba790, which prepares the slerp with override 003dcc70) and keeps the first pose's height;
 * btlMainCurveSetThreePoses (001baff0) then writes the first pose, two thirds toward the middle, one third from the middle toward the last,
 * and the last pose. Production mirror and override objects run in the EE float mode, as in the game.
 *
 * The words are the retail camera keys (battle context +0x24, +4..+0x70: position x y z, quaternion x y z w) at the collision 900 first-action
 * anchor of 2026-11-01 (alignment/3/retail-before.ram of collision900-quiet-0a85b684 and collision900-first-action-027deaa3, which agree): the
 * first and last keys are the function's inputs, the middle two its results. With 003dcc70 summing the slerp dot product w, z, x, y instead of
 * retail's y, x, z, w, the third key's quaternion came out bca6385b 3f69f9ea bd124425 beceb18e. */
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

static const uint32_t retail_keys[KEY_WORDS] = {
    0x4416f230u, 0x41c80000u, 0x445824f7u, 0x3c25c951u, 0x3f7402f7u, 0x3d037114u, 0xbe99e289u,
    0x441dd04cu, 0x41c7ffffu, 0x44376950u, 0xbba70613u, 0x3f6f863au, 0xbaedb7e0u, 0xbeb4b07au,
    0x4424ae6au, 0x42ae0f11u, 0x4416adabu, 0xbca63859u, 0x3f69f9e9u, 0xbd124424u, 0xbeceb18du,
    0x442b8c88u, 0x4353169bu, 0x43ebe40bu, 0xbd10f95du, 0x3f636451u, 0xbd8e3a5au, 0xbee7c848u,
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
    memcpy(poses, retail_keys, POSE_WORDS * sizeof *poses);
    memcpy(poses + 2 * POSE_WORDS, retail_keys + 3 * POSE_WORDS, POSE_WORDS * sizeof *poses);
    mdlBlendTranslationRotation(poses + POSE_WORDS, poses, poses + 2 * POSE_WORDS, 0.5f);
    poses[POSE_WORDS + 1] = poses[1];
    memset(curve, 0, 0x100);
    btlMainCurveSetThreePoses((unsigned short *)curve, poses, poses + POSE_WORDS, poses + 2 * POSE_WORDS, 1);
    memcpy(key, curve + 4, sizeof key);
    for (i = 0; i < KEY_WORDS; i++) {
        if (key[i] != retail_keys[i]) {
            printf("FAIL key %d word %d: %08x, retail %08x\n", i / POSE_WORDS, i % POSE_WORDS, (unsigned)key[i], (unsigned)retail_keys[i]);
            failures++;
        }
    }
    if (failures != 0) {
        return 1;
    }
    puts("battle opening camera curve: four keys equal to retail (28 words)");
    return 0;
}

int main(int argc, char **argv)
{
    return p4t_on_guest_stack(test_main, argc, argv);
}
