#!/usr/bin/env python3
"""Synthetic seeding and draw vectors against the production RNG overrides."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
PROCESS_FLAGS = subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0


def step_rp(state):
    words = list(state[:31])
    front, back = state[31] // 4, state[32] // 4
    words[front] = (words[front] + words[back]) & 0xffffffff
    return tuple(words) + (((front + 1) % 31) * 4, ((back + 1) % 31) * 4)


class SyntheticRngTest(unittest.TestCase):
    def test_rng_seed_and_draws_against_synthetic_reference(self):
        compiler = shutil.which("cc") or shutil.which("clang") or shutil.which("gcc")
        if not compiler:
            self.skipTest("C compiler unavailable")
        rp = (ROOT / "overrides/003b7060.c").read_text()
        seed = (ROOT / "overrides/003b6f00.c").read_text()
        # the overrides call the Golden native hooks; the synthetic reference runs the retail generator, so those hooks are declined here
        rp = rp.replace('#include "p4pc/golden_ai_native.h"\n', "")
        seed = seed.replace('#include "p4pc/golden_ai_native.h"\n', "")
        for offset in (12, 8, 4):
            for slot in ("blk", "iGpffffb618"):
                rp = rp.replace(f"{slot} + {offset})", f"{slot} + {offset // 4} * sizeof(void *))")
                seed = seed.replace(f"{slot} + {offset})", f"{slot} + {offset // 4} * sizeof(void *))")
        body = r'''#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint32_t u32; typedef int32_t s32; typedef uint8_t u8;
static _Alignas(void *) u8 D_008872E0[4 * sizeof(void *)];
static unsigned iGpffffb618;
u32 p4pc_rng_draws;
int p4pc_align_rng_calls_enabled;
void p4pc_rng_value(u32 draw, u32 value) {(void)draw; (void)value;}
void p4pc_rng_note(s32 value) {(void)value;}
void p4pc_align_rng_call(u32 draw, const void *caller) {(void)draw; (void)caller;}
int p4_golden_ai_native_random(u32 *value) {(void)value; return 0;}
int p4_golden_ai_native_seed(u32 seed) {(void)seed; return 0;}
''' + rp + "\n" + seed + r'''
int main(void) {
    u32 words[31] = {0};
    u32 *slots[4] = {words, words, words, words + 31};
    memcpy(D_008872E0, slots, sizeof slots);
    RpRandomSeed(0x12345678, 0);
    for (unsigned n = 0; n < 620; n++) {
        u32 value = RpRandom();
        memcpy(slots, D_008872E0, sizeof slots);
        printf("%08x ", value);
        for (unsigned i = 0; i < 31; i++) printf("%08x ", words[i]);
        printf("%08x %08x\n", (unsigned)(slots[1] - words) * 4, (unsigned)(slots[2] - words) * 4);
    }
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "synthetic_seed.c"
            binary = str(Path(directory) / ("synthetic_seed.exe" if os.name == "nt" else "synthetic_seed"))
            source.write_text(body)
            built = subprocess.run([compiler, "-std=c11", "-O0", str(source), "-o", binary],
                                   capture_output=True, text=True, creationflags=PROCESS_FLAGS)
            self.assertEqual(built.returncode, 0, built.stdout + built.stderr)
            output = subprocess.run([binary], capture_output=True, text=True, check=True, creationflags=PROCESS_FLAGS).stdout
        words = [0x12345678]
        for _ in range(30):
            words.append((words[-1] * 0x41c64e6d + 12345) & 0xffffffff)
        state = tuple(words) + (12, 0)
        for _ in range(310):
            state = step_rp(state)
        lines = output.splitlines()
        self.assertEqual(len(lines), 620)
        for line in lines:
            front = state[-2] // 4
            state = step_rp(state)
            values = tuple(int(word, 16) for word in line.split())
            self.assertEqual(values[0], state[front] >> 1)
            self.assertEqual(values[1:], state)


if __name__ == "__main__":
    unittest.main()
