"""Byte-exact VFS consumption; optional authored packages stay outside the repo."""
import hashlib
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest

COMMAND = []
PRIVATE = os.environ.get("P4PC_PRIVATE_MOD_FIXTURES")


class PackageRead(unittest.TestCase):
    def compare(self, root, ident, files):
        result = subprocess.run([*COMMAND, str(root), ident, *(key for key, _ in files)],
                                capture_output=True, timeout=90, check=True)
        expected = hashlib.sha256()
        size = 0
        for _, source in files:
            with source.open("rb") as stream:
                for block in iter(lambda: stream.read(65536), b""):
                    expected.update(block)
                    size += len(block)
        self.assertEqual(len(result.stdout), size)
        self.assertEqual(hashlib.sha256(result.stdout).digest(), expected.digest())
        metadata = result.stderr.decode("utf-8", errors="replace")
        sources = [line for line in metadata.splitlines() if line.startswith("source=")]
        self.assertEqual(len(sources), len(files))
        for line in sources:
            self.assertIn(f"provider:{ident} rank:1 ", line)
        return sources

    def test_wrapped_binary_and_zero_length_files(self):
        with tempfile.TemporaryDirectory(prefix="p4-package-read-") as temp:
            root = pathlib.Path(temp)
            package = root / "wrapped"
            payload = package / "Data/Data"
            payload.mkdir(parents=True)
            (package / "Mod.xml").write_text(
                "<Mod><Id>00000000-0000-0000-0000-000000000007</Id><Title>Binary witness</Title><Version>1</Version></Mod>",
                encoding="utf-8")
            binary = payload / "Binary.BIN"
            empty = payload / "Empty.BIN"
            binary.write_bytes(b"DATA" + bytes(range(256)) * 513)
            empty.write_bytes(b"")
            sources = self.compare(root, "compendium.00000000-0000-0000-0000-000000000007", [
                ("cdrom0:\\data\\binary.bin;1", binary), ("DATA/EMPTY.BIN", empty)])
            self.assertIn("key:DATA/BINARY.BIN ", sources[0])
            self.assertIn("key:DATA/EMPTY.BIN ", sources[1])

    def private_package(self, folder, guid):
        root = pathlib.Path(PRIVATE)
        payload = root / folder / "Data"
        self.assertTrue(payload.is_dir(), f"private package payload missing: {payload}")
        files = []
        for path in sorted(payload.rglob("*")):
            if not path.is_file():
                continue
            relative = path.relative_to(payload)
            if relative.parts[0].upper() in ("DATA", "BTL", "BGM", "ENV", "ZZZZZ"):
                files.append((relative.as_posix().upper(), path))
        self.assertTrue(files, "private package contains no disc-tree files")
        self.compare(root, f"compendium.{guid}", files)

    @unittest.skipUnless(PRIVATE, "set P4PC_PRIVATE_MOD_FIXTURES to privately installed authored packages")
    def test_private_almighty_loop_fix(self):
        # Author source: https://gamebanana.com/sounds/63084 ; no assets committed.
        self.private_package("The Almighty Loop Fix", "35a8d87c-2cb9-4bee-af42-ce2e61ef3830")

    @unittest.skipUnless(PRIVATE, "set P4PC_PRIVATE_MOD_FIXTURES to privately installed authored packages")
    def test_private_saturated_portraits(self):
        # Author source: https://gamebanana.com/mods/424557 ; no assets committed.
        self.private_package("portraits", "5c600e28-937f-4e00-8587-e3708bf3d99e")


if __name__ == "__main__":
    split = sys.argv.index("--driver-end")
    COMMAND = sys.argv[1:split]
    if not COMMAND:
        raise SystemExit("supply native driver before --driver-end")
    unittest.main(argv=[sys.argv[0], *sys.argv[split + 1:]])
