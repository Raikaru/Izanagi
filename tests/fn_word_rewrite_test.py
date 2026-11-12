"""Execute guest-word callback casts at multiple declared pointer depths."""
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
extern void *p4_g2h(u32);
extern void *p4_fn_g2h(u32);
extern void *p4_fn_g2h_sig(u32, u32);
extern u32 p4_h2g(const void *);
extern u32 p4_fn_h2g(const void *);
typedef int (*Code)(void *);
extern void *level1[1];
extern void **level2[1];
extern void ***level3[1];
extern Code *function_tables[1];
extern Code direct_functions[1];
int invoke_words(void *argument) {
    return ((Code)level1[0])(argument) +
           ((Code)level2[0])(argument) +
           ((Code)level3[0])(argument) +
           ((Code)function_tables[0])(argument) +
           direct_functions[0](argument);
}
int invoke_native(void *function, void *argument) {
    return ((Code)function)(argument);
}
"""

HARNESS = r"""
typedef unsigned int u32;
extern void abort(void);
u32 level1[1] = {0x1234};
u32 level2[1] = {0x1234};
u32 level3[1] = {0x1234};
u32 function_tables[1] = {0x1234};
u32 direct_functions[1] = {0x1234};
static int marker;
static unsigned lookups;
static int callback(void *argument) {
    if (argument != &marker) abort();
    return 7;
}
void *p4_g2h(u32 address) { (void)address; abort(); return 0; }
void *p4_fn_g2h_sig(u32 address, u32 signature) {
    (void)signature;
    if (address != 0x1234) abort();
    ++lookups;
    return (void *)callback;
}
void *p4_fn_g2h(u32 address) { return p4_fn_g2h_sig(address, 0); }
u32 p4_h2g(const void *pointer) { (void)pointer; abort(); return 0; }
u32 p4_fn_h2g(const void *pointer) { (void)pointer; abort(); return 0; }
extern int invoke_words(void *);
extern int invoke_native(void *, void *);
int main(void) {
    if (invoke_words(&marker) != 35 || lookups != 5) return 1;
    if (invoke_native((void *)callback, &marker) != 7 || lookups != 5) return 2;
    return 0;
}
"""


@unittest.skipUnless(shutil.which("clang"), "requires clang's AST reader")
class FunctionWordsTest(unittest.TestCase):
    def test_nested_data_words_decode_as_callbacks_and_native_values_stay_native(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            source = directory / "words.c"
            source.write_text(SOURCE)
            rw, _, text = gr.process(str(source), ["--target=x86_64-linux-gnu"], work, work)
            self.assertEqual(rw.census["fn-cast-of-data-word"], 4)
            generated = directory / "rewritten.c"
            generated.write_text(text, encoding="latin-1")
            harness = directory / "harness.c"
            harness.write_text(HARNESS)
            executable = directory / ("words.exe" if sys.platform == "win32" else "words")
            subprocess.run(["clang", "-std=gnu11", "-Wno-everything", str(generated),
                            str(harness), "-o", str(executable)], check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
