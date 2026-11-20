"""The capability registry: fragments parse, compose into p4.d.luau and agree with the build's table."""
from pathlib import Path
import re
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import p4_capabilities as caps  # noqa: E402

BASE = """export type T = number
declare p4: {
    id: string,
    mem: {
        read: (address: number) -> number,
    },
    clock: { wall: () -> number },
    events: {
        on: ((name: "a", callback: () -> ()) -> number)
            & ((name: "b", callback: () -> ()) -> number),
    },
    battle: {
        formation: (callback: () -> ()) -> number,
    },
}
"""


class Fragments(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="p4-capabilities-")
        self.addCleanup(self.tmp.cleanup)
        self.dir = Path(self.tmp.name)

    def fragment(self, key, body, revision=1, name=None):
        path = self.dir / (name or key + ".luau")
        path.write_bytes(f"--!capability {key}@{revision}\n{body}".encode("utf-8"))
        return path

    def compose(self):
        return caps.compose(BASE, caps.load(self.dir))

    def refused(self, needle):
        with self.assertRaises(caps.CapabilityError) as raised:
            self.compose()
        self.assertIn(needle, str(raised.exception))

    def test_repository_fragments_compose(self):
        fragments = caps.load()
        self.assertEqual([f.key for f in fragments], sorted(f.key for f in fragments))
        composed = caps.compose(caps.BASE.read_text(encoding="utf-8"), fragments)
        for fragment in fragments:
            for lines in fragment.fields.values():
                for line in lines:
                    if line.strip():
                        self.assertIn(line.strip(), composed, fragment.name)
        self.assertIn("capabilities@1", [f.name for f in fragments])
        self.assertIn("core@1", [f.name for f in fragments])

    def test_cmake_and_python_accept_the_same_headers(self):
        cmake = (ROOT / "cmake/sources/052-capabilities.cmake").read_text(encoding="utf-8")
        pattern = re.search(r'MATCHES "(\^--!capability [^"]+)"', cmake).group(1).replace("\\\\", "\\")
        for header in ("--!capability a@1", "--!capability a.b_c-d@12", "--!capability a@0", "--!capability A@1",
                       "--!capability a.@1", "--!capability .a@1", "--!capability a@", "--!capability a@01",
                       "--!capability a.1b@1", "--!capability a b@1"):
            self.assertEqual(bool(re.fullmatch(pattern.strip("^$"), header)), bool(caps.HEADER.fullmatch(header)), header)

    def test_fields_namespaces_types_and_overloads(self):
        self.fragment("mem.extra", "--!in p4.mem\nsize: () -> number,\n")
        self.fragment("tables.source", "export type Src = buffer\n--!in p4.tables\nsource: (name: string) -> Src?,\n")
        self.fragment("tables.deep", "--!in p4.tables.deep.er\nvalue: number,\n")
        self.fragment("events.c", '--!overload p4.events.on\n(name: "c", callback: (e: T) -> ()) -> number\n')
        self.fragment("top", "--!in p4\nhas: (key: string) -> boolean,\n")
        out = self.compose()
        self.assertIn("        read: (address: number) -> number,\n        size: () -> number,\n    },", out)
        self.assertIn("-- tables.source@1\nexport type Src = buffer\n\ndeclare p4: {", out)
        self.assertIn("    tables: {\n        source: (name: string) -> Src?,\n        deep: {\n"
                      "            er: {\n                value: number,\n            },\n        },\n    },\n", out)
        self.assertIn('            & ((name: "b", callback: () -> ()) -> number)\n'
                      '            & ((name: "c", callback: (e: T) -> ()) -> number),\n', out)
        self.assertIn("    },\n    has: (key: string) -> boolean,\n    tables: {\n", out)
        self.assertTrue(out.endswith("        },\n    },\n}\n"), out)
        self.assertEqual(caps.compose(BASE, []), BASE)

    def test_header_must_name_the_file(self):
        self.fragment("events.ailment", "", name="events.ailmnet.luau")
        with self.assertRaisesRegex(caps.CapabilityError, "rename the file to events.ailment.luau"):
            caps.load(self.dir)
        (self.dir / "events.ailmnet.luau").write_text("-- no header\n", encoding="utf-8")
        with self.assertRaisesRegex(caps.CapabilityError, "first line must be"):
            caps.load(self.dir)

    def test_collisions_and_bad_targets_refused(self):
        self.fragment("one", "--!in p4.mem\nread: number,\n")
        self.refused("p4.mem.read is already declared")
        (self.dir / "one.luau").unlink()
        self.fragment("two", "--!in p4.clock\nextra: number,\n")
        self.refused("p4.clock is not a multi-line namespace table")
        (self.dir / "two.luau").unlink()
        self.fragment("three", "--!overload p4.battle.formation\n() -> ()\n")
        self.refused("is not a parenthesized intersection")
        (self.dir / "three.luau").unlink()
        self.fragment("four", "--!overload p4.events.missing\n() -> ()\n")
        self.refused("no such field")
        (self.dir / "four.luau").unlink()
        self.fragment("five", "--!in p4.x\na: number,\n")
        self.fragment("six", "--!in p4.x\na: string,\n")
        self.refused("p4.x.a is already declared")

    def test_unknown_directive_and_examples(self):
        self.fragment("bad", "--!inn p4.mem\n")
        with self.assertRaisesRegex(caps.CapabilityError, "unknown directive"):
            caps.load(self.dir)
        self.fragment("bad", "--!example\nlocal n: number = 1\n--!refuse\nlocal s: string = 1\n\nlocal b: boolean = 1\n")
        fragment = caps.load(self.dir)[0]
        self.assertEqual(fragment.examples, ["local n: number = 1"])
        self.assertEqual(fragment.refusals, ["local s: string = 1", "local b: boolean = 1"])

    def test_requires_keys(self):
        self.assertEqual(caps.parse_key("events.ailment"), ("events.ailment", 1))
        self.assertEqual(caps.parse_key(" manifest.resource-model@3 "), ("manifest.resource-model", 3))
        for bad in ("", "Events", "a@0", "a@x", "a..b", "a@1@2"):
            with self.assertRaises(caps.CapabilityError):
                caps.parse_key(bad)

    def test_surfaces_index_what_a_mod_can_use(self):
        self.fragment("audio", "--!covers p4.audio section:audio-cue section:table-source.status file:data/cues.xml\n"
                               "-- volumes\n")
        self.fragment("events.c", '--!overload p4.events.on\n(name: "c", callback: (e: T) -> ()) -> number\n')
        self.fragment("tables.source", "--!in p4.tables\nsource: (name: string) -> buffer?,\nlimit: number,\n")
        fragments = caps.load(self.dir)
        self.assertEqual(fragments[0].summary, "volumes")
        self.assertEqual(caps.index(fragments), "audio 1 p4.audio section:audio-cue section:table-source.status "
                                                "file:data/cues.xml\n"
                                                "events.c 1 event:c\n"
                                                "tables.source 1 p4.tables.source p4.tables.limit\n")
        for bad in ("p4", "file:/data/cues.xml", "file:data/../cues.xml"):
            self.fragment("bad", f"--!covers {bad} something\n")
            with self.assertRaisesRegex(caps.CapabilityError, f"--!covers item '{re.escape(bad)}'"):
                caps.load(self.dir)
        repository = {f.key: caps.surfaces(f) for f in caps.load(caps.FRAGMENTS)}
        self.assertIn("p4.hook", repository["advanced"])
        self.assertIn("event:battle_end", repository["events.battle_end"])
        self.assertEqual(repository["capabilities"], ["p4.has", "p4.capabilities"])
        self.assertFalse([key for key, found in repository.items() if not found], "every key names what it covers")


if __name__ == "__main__":
    unittest.main()
