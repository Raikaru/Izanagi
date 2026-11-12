"""Compiler-AST regressions for uniform masked EE GP words, without retail data."""
from pathlib import Path
import os
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from port64 import gptr_rewrite as gr
from port64 import printf_rewrite as pr


@unittest.skipUnless(shutil.which("clang"), "requires clang's AST reader")
class PrintfWordsTest(unittest.TestCase):
    def fixture(self, narrow):
        source = ROOT / "tests/data/ee_printf_abi_calls.c"
        with tempfile.TemporaryDirectory() as work:
            flags = ["--target=i386-linux-gnu"] if narrow else ["--target=x86_64-linux-gnu"]
            if narrow:
                rw, text = pr.process(str(source), flags, work, str(source.parent))
            else:
                rw, _, text = gr.process(str(source), flags, work, str(source.parent), abi=pr.printf_abi())
            self.assertEqual(rw.census["printf-masked"], 3)
            self.assertEqual(rw.census["printf-gp-word"], 18)
            for mask in ("0xffu,", "0x737u,", "0x3u,"):
                self.assertIn(mask, text)
            self.assertIn("((unsigned long long)(s))", text)
            self.assertIn("((unsigned long long)(u))", text)
            self.assertNotIn("((unsigned long long)(native))", text)
            self.assertNotIn("((unsigned long long)(real))", text)
            self.assertEqual(text.count('__asm__("p4pf_sprintf")'), 3)
            return text

    def test_full_x64_uses_words_and_native_classes(self):
        self.fixture(False)

    def test_narrow_i386_uses_the_same_words_and_classes(self):
        self.fixture(True)

    def test_narrow_is_idempotent(self):
        text = self.fixture(True)
        with tempfile.TemporaryDirectory() as work:
            source = Path(work) / "again.c"
            source.write_text(text, encoding="latin-1")
            rw, text = pr.process(str(source), ["--target=i386-linux-gnu"], work, str(ROOT / "tests/data"))
            self.assertEqual(rw.census.get("printf-masked", 0), 0)
            self.assertEqual(text.count("0x737u,"), 1)

    def test_integer_fixed_addresses_and_all_families(self):
        with tempfile.TemporaryDirectory() as work:
            source = Path(work) / "calls.c"
            source.write_text("""
extern int sprintf(int, int, ...);
extern int snprintf(int, unsigned int, int, ...);
extern int printf(int, ...);
extern int fprintf(int, int, ...);
void calls(int buffer, int format, int stream, int value) {
    sprintf(buffer, format, value);
    snprintf(buffer, 64u, format, value);
    printf(format, value);
    fprintf(stream, format, value);
}
""")
            rw, text = pr.process(str(source), ["--target=i386-linux-gnu"], work, work)
            self.assertEqual(rw.census["printf-masked"], 4)
            self.assertEqual(rw.census["printf-gp-word"], 4)
            self.assertIn("((void *)(unsigned long)(format))", text)
            for family in pr.FIXED:
                self.assertIn(f'__asm__("p4pf_{family}")', text)

    def test_i386_coff_symbol_prefix(self):
        source = ROOT / "tests/data/ee_printf_abi_calls.c"
        with tempfile.TemporaryDirectory() as work:
            rw, text = pr.process(str(source), ["--target=i686-w64-windows-gnu"], work,
                                  str(source.parent), symbol_prefix="_")
            self.assertEqual(rw.census["printf-masked"], 3)
            self.assertEqual(text.count('__asm__("_p4pf_sprintf")'), 3)

    def test_local_definition_is_not_a_crt_call(self):
        with tempfile.TemporaryDirectory() as work:
            source = Path(work) / "local.c"
            source.write_text("""
static int printf(const char *format, ...) { return *format; }
int caller(void) { return printf("%ld", -7); }
""")
            rw, text = pr.process(str(source), ["--target=i386-linux-gnu"], work, work)
            self.assertEqual(rw.census.get("printf-masked", 0), 0)
            self.assertNotIn("p4pf_", text)

    def test_launcher_composes_abi_arithmetic_and_original_dependencies(self):
        with tempfile.TemporaryDirectory() as work:
            directory = Path(work)
            source = directory / "calls.c"
            header = directory / "operand.h"
            harness = directory / "harness.c"
            header.write_text("#define OPERAND 17\n")
            source.write_text("""
#include "operand.h"
static_assert(sizeof(int) == 4);
extern int sprintf(char *, const char *, ...);
int caller(char *out, float a, float b) {
    return sprintf(out, "%ld", -OPERAND) + (int)(a + b);
}
""")
            harness.write_text("""
/* A compiler-ABI witness needs no host C headers or library calls. */
extern int caller(char *, float, float);
int p4pf_sprintf(char *out, const char *format, ...) {
    __builtin_va_list ap;
    __builtin_va_start(ap, format);
    unsigned mask = __builtin_va_arg(ap, unsigned);
    unsigned long long word = __builtin_va_arg(ap, unsigned long long);
    __builtin_va_end(ap);
    if (mask != 1 || word != (unsigned long long)-17LL ||
        format[0] != '%' || format[1] != 'l' || format[2] != 'd' ||
        format[3] != 0) __builtin_trap();
    out[0] = 0;
    return 5;
}
/* A distinct pure result witnesses the launcher phase, not EE math accuracy
 * (which has its own original-instruction differential corpus). */
float p4pc_ee_add(float a, float b) { return a + b + 100.0f; }
int main(void) {
    char out[1];
    return caller(out, 1.0f, 2.0f) != EXPECTED;
}
""")
            environment = os.environ.copy()
            environment.pop("P4_EE_FLOAT_REPORT", None)
            clang = shutil.which("clang")
            for mode, expected in (("none", 8), ("add", 108)):
                with self.subTest(mode=mode):
                    obj = directory / f"{mode}.o"
                    dep = directory / f"{mode}.d"
                    exe = directory / f"{mode}.exe"
                    subprocess.run([
                        sys.executable, str(ROOT / "tools/ee_float_cc.py"),
                        f"--modes={mode}", f"--printf-root={work}",
                        # the host compiler's C symbol prefix, as CMake passes it for Win32: Mach-O names C symbols `_name`
                        *(["--printf-symbol-prefix=_"] if sys.platform == "darwin" else []), clang,
                        "-std=c2x", "-O1", "-Xclang", "-fno-spell-checking",
                        "-MMD", "-MF", str(dep), "-MT", obj.name,
                        "-c", str(source), "-o", str(obj),
                    ], check=True, env=environment)
                    subprocess.run([
                        clang, str(harness), str(obj), f"-DEXPECTED={expected}",
                        "-o", str(exe),
                    ], check=True)
                    subprocess.run([str(exe)], check=True)
                    dependencies = dep.read_text()
                    self.assertIn("calls.c", dependencies)
                    self.assertIn("operand.h", dependencies)
                    self.assertNotIn(".printf.i", dependencies)
                    self.assertNotIn(".ll", dependencies)
                    self.assertFalse(Path(str(obj) + ".printf.i").exists())
                    self.assertEqual(list(directory.glob("*.ll")), [])


if __name__ == "__main__":
    unittest.main()
