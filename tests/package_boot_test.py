"""Real C registry/file lifecycle tests, not native renderer/gameplay evidence."""
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

separator = sys.argv.index("--driver-end")
DRIVER = sys.argv[1:separator]
sys.argv = [sys.argv[0], *sys.argv[separator + 1:]]


class PackageBoot(unittest.TestCase):
    def run_mode(self, mode):
        with tempfile.TemporaryDirectory(prefix="p4 boot lifecycle ") as temporary:
            root = pathlib.Path(temporary)
            for ident in ("boot.a", "boot.b"):
                package = root / ident
                (package / "files/DATA").mkdir(parents=True)
                (package / "mod.ini").write_text(
                    f"[mod]\nid={ident}\nname=Boot fixture\nversion=1.0\napi=1.1\npriority=0\norder=0\n",
                    encoding="utf-8",
                )
                (package / "files/DATA/TEST.TXT").write_bytes(ident.encode("ascii"))
                (package / "image.bin").write_bytes(
                    b"BADBOOT\n" if mode == "renew" and ident == "boot.a" else b"P4BOOT1\n"
                )
            enabled = "" if mode == "empty" else "boot.a,boot.b" if mode == "renew" else "boot.a"
            env = dict(os.environ, P4PC_MODS="0", P4PC_PACKS="0", P4PC_INPUT="script",
                       P4PC_CONFIG="none", SDL_AUDIODRIVER="dummy")
            env.pop("P4PC_MOD_WITNESS", None)
            result = subprocess.run([*DRIVER, mode, str(root), enabled], env=env,
                                    capture_output=True, text=True, check=False)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            return result.stdout, result.stderr

    def test_declared_dependency_order_early_before_raw_and_late_before_publish(self):
        out, _ = self.run_mode("success")
        self.assertIn("boot-success", out)
        self.assertIn(
            "cpu.schema|cpu.tables|cpu.resources|cpu.ui|cpu.finalize|pub.tables|pub.resources|pub.ui|"
            "rt.tables|rt.resources|rt.ui|rt.finalize|rtpub.tables|rtpub.resources|rtpub.ui|", out)

    def test_inactive_no_input_products_are_explicit_null_without_file_reads(self):
        out, _ = self.run_mode("empty")
        self.assertIn("boot-success", out)

    def test_missing_cycle_and_backwards_edges_refuse_before_callbacks(self):
        for mode in ("missing", "cycle", "backwards"):
            with self.subTest(mode=mode):
                out, _ = self.run_mode(mode)
                self.assertIn("early-refused trace=\n", out)

    def test_partial_prepare_and_finalizer_reverse_rollback_before_exposure(self):
        for mode in ("early-fail", "early-finalize-fail"):
            with self.subTest(mode=mode):
                out, _ = self.run_mode(mode)
                self.assertIn("undo.ui|undo.resources|undo.tables|undo.schema|", out)
                self.assertNotIn("pub.", out)

    def test_ignored_duplicate_provide_is_sticky_not_success(self):
        out, _ = self.run_mode("sticky")
        self.assertIn("undo.tables|undo.schema|", out)
        self.assertNotIn("cpu.resources|", out)

    def test_post_exposure_failure_preserves_selected_owner_and_cpu_resources(self):
        for mode in ("late-fail", "late-finalize-fail"):
            with self.subTest(mode=mode):
                out, _ = self.run_mode(mode)
                self.assertIn("late-retained", out)
                self.assertNotIn("undo.", out)
                self.assertNotIn("rtpub.", out)

    def test_runtime_never_falls_back_to_early_product(self):
        out, _ = self.run_mode("cross-pass")
        self.assertIn("late-retained", out)
        self.assertNotIn("rt.resources|", out)

    def test_cpu_refusal_renews_authority_not_immutable_old_identity(self):
        out, _ = self.run_mode("renew")
        self.assertIn("boot-success", out)
        self.assertIn("undo.resources|undo.tables|undo.schema|", out)


if __name__ == "__main__":
    unittest.main()
