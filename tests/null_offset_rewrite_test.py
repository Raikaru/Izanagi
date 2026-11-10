"""The decomp's offsetof (`MEMBER_OFFSET`: `(s32)&((T *)0)->field`) is a byte offset on the 64-bit host too, at any offset.

The rewriter used to convert it like any pointer with p4_h2g, which knows an offset from a host pointer only below 64 KB: the
pattern field of the Shuffle Time context (+0x1F2FC) faulted the 64-bit builds in Shuffle Time pattern 3 (btlShuffleSeq4Shuffle)."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from port64 import gptr_rewrite as gr

SOURCE = r"""
typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef unsigned char u8;
extern void *p4_g2h(u32);
extern u32 p4_h2g(const void *);
#define MEMBER_OFFSET(Struct, member) ((s32)&((Struct *)0)->member)
#define MEMBER(type, base, Struct, member) (*(type *)((base) + MEMBER_OFFSET(Struct, member)))
typedef struct Inner { u16 a; u16 b[4]; } Inner;
typedef struct Context {
    u8 pad[0x1F2FC];
    s32 pattern;
    u8 *task;
    Inner inner[2];
} Context;
s32 read_pattern(u8 *context) { return MEMBER(s32, context, Context, pattern); }
void write_pattern(u8 *context, s32 value) { MEMBER(s32, context, Context, pattern) = value; }
u32 nested_offset(void) { return (u32)&((Context *)0)->inner[1].b[2]; }
u32 small_offset(void) { return (u32)&((Inner *)0)->b[1]; }
"""

HARNESS = r"""
#include <stdlib.h>
#include <string.h>
typedef unsigned int u32;
void *p4_g2h(u32 address) { (void)address; abort(); return 0; }
u32 p4_h2g(const void *pointer) { (void)pointer; abort(); return 0; }
extern int read_pattern(unsigned char *);
extern void write_pattern(unsigned char *, int);
extern u32 nested_offset(void);
extern u32 small_offset(void);
int main(void) {
    unsigned char *context = calloc(1, 0x1F320);
    int stored;
    write_pattern(context, 3);
    memcpy(&stored, context + 0x1F2FC, sizeof stored);
    if (stored != 3 || read_pattern(context) != 3) return 1;
    if (nested_offset() != 0x1F314 || small_offset() != 4) return 2;
    return 0;
}
"""


@unittest.skipUnless(shutil.which("clang"), "requires clang's AST reader")
class NullOffsetTest(unittest.TestCase):
    def test_offsets_convert_as_integers_without_p4_h2g(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            source = directory / "offsets.c"
            source.write_text(SOURCE)
            rw, _, text = gr.process(str(source), ["--target=x86_64-linux-gnu"], work, work)
            self.assertEqual(rw.census["ptr2int-null-offset"], 4)
            self.assertEqual(rw.census["guest-sum"], 0)
            self.assertNotIn("p4_h2g((&", text)
            # the build compiles the rewritten text as preprocessed C (tools/port64/unit64.py): no macro may appear in it
            generated = directory / "rewritten.i"
            generated.write_text(text, encoding="latin-1")
            harness = directory / "harness.c"
            harness.write_text(HARNESS)
            executable = directory / ("offsets.exe" if sys.platform == "win32" else "offsets")
            subprocess.run(["clang", "-std=gnu11", "-Wno-everything", "-Werror=pointer-to-int-cast", "-x", "cpp-output", str(generated),
                            "-x", "c", str(harness), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
