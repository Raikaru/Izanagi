"""A call through a declaration that disagrees with the definition about `...`, on Apple arm64 (variable arguments on the stack, fixed ones
in registers): the rewriter calls through the definition's prototype there, and leaves other targets' calls alone."""
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from port64 import gptr_rewrite as gr

SOURCE = r"""
int logf_();
int fixed2(const char *, ...);
int call(void) {
    return logf_("x", 1, 2) + fixed2("y", 5);
}
"""

ABI = {
    "logf_": {"params": [{"cls": "ptr", "ct": "const char *", "fn": False}], "ret": "int", "rt": "int", "skip": False, "va": "other"},
    "fixed2": {"params": [{"cls": "ptr", "ct": "const char *", "fn": False}, {"cls": "int", "ct": "int"}], "ret": "int", "rt": "int",
               "skip": False},
}

HARNESS = r"""
#include <stdarg.h>
int logf_(const char *tag, ...) {
    va_list ap;
    va_start(ap, tag);
    int a = va_arg(ap, int), b = va_arg(ap, int);
    va_end(ap);
    return tag[0] == 'x' && a == 1 && b == 2 ? 0 : 1;
}
int fixed2(const char *tag, int value) { return tag[0] == 'y' && value == 5 ? 0 : 2; }
extern int call(void);
int main(void) { return call(); }
"""


@unittest.skipUnless(shutil.which("clang"), "requires clang's AST reader")
class VariadicCallTest(unittest.TestCase):
    def rewrite(self, work, target):
        source = Path(work) / "calls.c"
        source.write_text(SOURCE)
        rw, _, text = gr.process(str(source), ["--target=" + target], work, work, abi=ABI)
        return rw, text

    def test_apple_arm64_calls_through_the_definitions_prototype(self):
        with tempfile.TemporaryDirectory() as work:
            rw, text = self.rewrite(work, "arm64-apple-macos12.0")
            self.assertEqual(rw.census["abi-variadic-adapted"], 2)
            self.assertIn('extern int p4abi_logf__3(const char *, ...) __asm__("_logf_");', text)
            self.assertIn('extern int p4abi_fixed2_2(const char *, int) __asm__("_fixed2");', text)
            if sys.platform == "darwin" and platform.machine() == "arm64":
                # the runner is the target: the calls reach the definitions' arguments
                directory = Path(work)
                (directory / "rewritten.c").write_text(text, encoding="latin-1")
                (directory / "harness.c").write_text(HARNESS)
                subprocess.run(["clang", "-std=gnu11", "-Wno-everything", str(directory / "rewritten.c"), str(directory / "harness.c"),
                                "-o", str(directory / "calls")], check=True)
                subprocess.run([str(directory / "calls")], check=True)

    def test_other_targets_keep_the_callers_declaration(self):
        for target in ("aarch64-linux-gnu", "x86_64-linux-gnu", "x86_64-apple-macos12.0"):
            with self.subTest(target=target), tempfile.TemporaryDirectory() as work:
                rw, text = self.rewrite(work, target)
                self.assertEqual(rw.census["abi-variadic-adapted"], 0)
                self.assertNotIn("p4abi_", text)


if __name__ == "__main__":
    unittest.main()
