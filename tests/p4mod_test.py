"""p4mod, the modder's CLI, end to end: help, new, check diagnostics, pack and setup.

usage: p4mod_test.py [emulator ...] p4mod-executable --game-fixture fixture-executable
The executable's folder must hold the p4.d.luau and p4mod-capabilities.txt the build writes
beside it. The fixture exercises rejection of frame-only success and, with a retail disc in
P4PC_ISO, p4mod's side of the battle worker protocol; real game runs are exercised separately
(docs/modding/p4mod.md).
"""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
import zipfile


def main():
    # The fixture follows p4_target_command (tests/cmake/000-registration.cmake):
    # on the Windows profile it is [wine64, fixture.exe]; the emulator only names
    # the runner -- p4mod (already under the same emulator) spawns the fixture.
    _dash = sys.argv.index("--game-fixture")
    fixture_command = sys.argv[_dash + 1:]
    _rest = fixture_command
    if len(_rest) >= 2 and Path(_rest[0]).name.lower() in ("wine", "wine64"):
        _rest = _rest[1:]
    assert len(_rest) == 1, sys.argv
    command = sys.argv[1:_dash]
    game_fixture = Path(_rest[0]).resolve()
    with tempfile.TemporaryDirectory(prefix="p4mod-test-") as directory:
        root = Path(directory)
        env = dict(os.environ, P4PC_CONFIG=str(root / "p4pc.ini"))
        scratch = root / "scratch"
        scratch.mkdir()
        scratch_env = str(scratch)
        if os.name != "nt" and game_fixture.suffix.lower() == ".exe":
            scratch_env = "Z:" + scratch.as_posix()  # Wine's mapping of the host root
            # Debian Wine 10's server_tmpdir frees getenv("TMPDIR") when /run/user/<uid>
            # is unavailable (Debian #1110936), before the Windows executable starts.
            # Keep the Windows temporary directory isolated via TMP/TEMP instead.
            env.pop("TMPDIR", None)
        else:
            env["TMPDIR"] = str(scratch)
        env.update(TMP=scratch_env, TEMP=scratch_env)
        if os.name != "nt" and game_fixture.suffix.lower() == ".exe":
            # Wine replaces Unix TMP/TEMP with its prefix defaults at startup.
            # Set them inside Windows before spawning p4mod so its work stays private.
            command = [*fixture_command, "--run-with-temp", scratch_env, command[-1]]

        def p4mod(expected, *args, cwd=None, environment=None):
            result = subprocess.run([*command, *map(str, args)], capture_output=True, text=True,
                                    env=env if environment is None else environment, cwd=cwd or root)
            output = result.stdout + result.stderr
            assert result.returncode == expected, (args, result.returncode, output)
            return output

        def package_symlink(link, target, *, target_is_directory=False):
            try:
                link.symlink_to(target, target_is_directory=target_is_directory)
            except (OSError, NotImplementedError):
                return False  # Windows may not grant symlink creation to this account.
            if os.name != "nt" and game_fixture.suffix.lower() == ".exe":
                # Wine 10 exposes Unix symlinks as their targets, not reparse points.
                # Probe the Windows API rather than silently accepting an unsafe pack.
                result = subprocess.run([*fixture_command, "--probe-reparse", "Z:" + link.as_posix()],
                                        capture_output=True, text=True, env=env, cwd=root)
                assert result.returncode in (0, 1), (result.returncode, result.stdout, result.stderr)
                if result.returncode == 1:
                    link.unlink()
                    print("p4mod: Wine does not expose this host symlink as a reparse point")
                    return False
            return True

        assert "usage: p4mod <command>" in p4mod(0, "--help")
        assert "usage: p4mod check" in p4mod(0, "check", "--help")
        assert "usage: p4mod new" in p4mod(0, "help", "new")
        assert 'unknown command "frobnicate"' in p4mod(2, "frobnicate")
        assert "unknown option --bogus for `check`" in p4mod(2, "check", "--bogus")
        assert "is not a mod id" in p4mod(2, "new", "Bad Id")
        # The game's id rule (package_identifier.h): never a leading or trailing '.', so never "..".
        for unsafe in ("..", ".", ".hidden", "trailing.", "a/b", "a\\b"):
            assert "is not a mod id" in p4mod(2, "new", unsafe, "--dir", root), unsafe
        assert sorted(p.name for p in root.iterdir()) == ["scratch"], list(root.iterdir())

        # A new mod checks clean, then each class of mistake is one located, explained line.
        assert "Luau template" in p4mod(0, "new", "demo.mod", "--dir", root)
        mod = root / "demo.mod"
        assert {p.name for p in mod.iterdir()} == {"mod.ini", "main.luau", "README.md"}
        assert "already exists" in p4mod(2, "new", "demo.mod", "--dir", root)
        assert {p.name for p in mod.iterdir()} == {"mod.ini", "main.luau", "README.md"}
        assert "demo.mod: 0 error(s), 0 warning(s)" in p4mod(0, "check", mod)
        script = (mod / "main.luau").read_text(encoding="utf-8")
        lines = len(script.splitlines())
        (mod / "main.luau").write_text(script + 'p4.events.on("save", function() end)\nlocal raw = p4.mem\n'
                                       'local n: number = "x"\n', encoding="utf-8")
        output = p4mod(1, "check", mod)
        assert re.search(rf"main\.luau:{lines + 1}:1: warning: uses event:save \(capability events\.save@1\), "
                         r"which requires= does not list\. .* Add events\.save@1 to requires=", output), output
        assert re.search(rf"main\.luau:{lines + 2}:13: note: uses p4\.mem \(capability advanced@1\)", output), output
        assert re.search(rf"main\.luau:{lines + 3}:\d+: error: .*(number.*string|string.*number)", output), output
        assert "1 error(s), 1 warning(s)" in output, output
        (mod / "main.luau").write_text(script, encoding="utf-8")
        # data.luau is strict-checked against the p4 it actually gets (no events, hooks or UI);
        # data/edits.xml goes through the game's decoder and field rules and maps to its keys.
        (mod / "data.luau").write_text('p4.data.edit(function(d)\n    d.item("Medicine").price = 100\nend)\n'
                                       'local m: number = "x"\np4.events.on("save", function() end)\n', encoding="utf-8")
        (mod / "data").mkdir()
        edits = mod / "data" / "edits.xml"
        edits.write_text('<p4-edits schema="1">\n  <item name="Medicine" price="100"/>\n</p4-edits>\n', encoding="utf-8")
        output = p4mod(1, "check", mod)
        assert re.search(r"data\.luau:1:1: warning: uses p4\.data\.edit \(capability data\.edit@1\), "
                         r"which requires= does not list", output), output
        assert re.search(r"data\.luau:4:\d+: error: .*(number.*string|string.*number)", output), output
        assert re.search(r"data\.luau:5:\d+: error: .*events", output), output
        assert re.search(r"data\.luau:5:1: note: p4\.events does not exist in data\.luau", output), output
        assert "events.save" not in output, output
        assert "data.luau: note: the rows and values data.luau edits are checked when the game runs it" in output, output
        assert re.search(r"data/edits\.xml: warning: uses data/edits\.xml \(capability data\.xml@1\)", output), output
        assert re.search(r"data/edits\.xml: note: \d+ rule\(s\) need your disc's tables and were not checked here", output), output
        assert re.search(r"\d+ error\(s\), 2 warning\(s\)", output), output
        # settings.get in data.luau is a toggle's boolean or a choice's string, never a number.
        (mod / "data.luau").write_text('local v = p4.settings.get("x")\nlocal n: number? = v\n', encoding="utf-8")
        assert re.search(r"data\.luau:2:\d+: error: ", p4mod(1, "check", mod))
        (mod / "data.luau").unlink()
        # The game's own refusals: unknown attribute, malformed XML, range, enum and learn level.
        for body, expected in [
                ('<item name="Medicine" bogus="1"/>', r'data/edits\.xml:2:1: error: <item> has no attribute "bogus"'),
                ('<item name="Medicine" price="100">', r"data/edits\.xml:\d+:1: error: malformed XML"),
                ('<skill name="Agilao" power="10000"/>',
                 r"data/edits\.xml:2:1: error: skill power: 10000 is not a whole number from 0 to 9999"),
                ('<enemy name="Lying Hablerie" fire="sideways"/>',
                 r'data/edits\.xml:2:1: error: enemy affinities\.fire: "sideways" is not normal, weak, resist'),
                ('<persona name="Tomoe"><learn level="120" skill="Bufula"/></persona>',
                 r"data/edits\.xml:2:1: error: persona learnset: level 120 is outside 0\.\.99"),
                # Values past uint32 are refused, never wrapped onto a small level, date or ID.
                ('<persona name="Tomoe"><learn level="4294967346" skill="Bufula"/></persona>',
                 r"data/edits\.xml:2:1: error: persona learnset: level 4294967295 is outside 0\.\.99"),
                ('<skill id="4294967297" power="10"/>', r"data/edits\.xml:2:1: error: no skill has the id 4294967297"),
                ('<skill id="-1" power="10"/>', r"data/edits\.xml:2:1: error: no skill has the id -1"),
                ('<item name="Medicine" from="4294967300/20"/>',
                 r'data/edits\.xml:2:1: error: <item> from="4294967300/20" is not a month/day such as 4/20'),
                ('<enemy name="Lying Hablerie"><skill slot="1" name="4294967297"/></enemy>',
                 r"data/edits\.xml:2:1: error: enemy skills\.1: no skill has the ID 4\.29497e\+09")]:
            edits.write_text('<p4-edits schema="1">\n  ' + body + '\n</p4-edits>\n', encoding="utf-8")
            output = p4mod(1, "check", mod)
            assert re.search(expected + r".*The game refuses the mod", output), (body, output)
        # What the records use maps to the keys it needs: renames, shop fields, dungeon rows.
        edits.write_text('<p4-edits schema="1">\n  <item name="Medicine" rename="Tonic" sold="true"/>\n'
                         '  <floor name="Yukiko\'s Castle 1F" spawn="3"/>\n</p4-edits>\n', encoding="utf-8")
        output = p4mod(0, "check", mod)
        for key in ("data.item_names@1", "data.shop@1", "data.dungeons@1"):
            assert f"(capability {key})" in output, (key, output)
        # The game's cross-field rules run on the final rows after data.luau: named, not claimed.
        edits.write_text('<p4-edits schema="1">\n  <skill name="Agilao" hits-min="3"/>\n'
                         '  <enemy name="Lying Hablerie"><drop slot="1" chance="90"/></enemy>\n</p4-edits>\n',
                         encoding="utf-8")
        output = p4mod(0, "check", mod)
        assert "not checked here: every skill's hits.min <= hits.max" in output, output
        assert "not checked here: each enemy's drop chances add up to at most 100%" in output, output
        edits.unlink()
        # The game finds package files case-insensitively: Data/Edits.xml is data/edits.xml.
        (mod / "data").rmdir()
        (mod / "Data").mkdir()
        (mod / "Data" / "Edits.xml").write_text('<p4-edits schema="1">\n  <skill name="Agilao" power="10000"/>\n'
                                                '</p4-edits>\n', encoding="utf-8")
        output = p4mod(1, "check", mod)
        assert re.search(r"Data/Edits\.xml:2:1: error: skill power: 10000", output), output
        (mod / "Data" / "Edits.xml").unlink()
        (mod / "Data").rmdir()
        (mod / "DATA.luau").write_text('p4.events.on("save", function() end)\n', encoding="utf-8")
        if not (mod / "data.luau").exists():  # a case-sensitive filesystem: two names, one key
            (mod / "data.luau").write_text('p4.log("x")\n', encoding="utf-8")
            output = p4mod(1, "check", mod)
            assert "canonical path collision" in output and "the game's package index refuses this folder" in output, output
            (mod / "data.luau").unlink()
        output = p4mod(1, "check", mod)
        assert re.search(r"DATA\.luau:1:1: note: p4\.events does not exist in data\.luau", output), output
        (mod / "DATA.luau").unlink()
        # p4.data.edit in main.luau is an error the game raises; move it to data.luau.
        (mod / "main.luau").write_text(script + "p4.data.edit(function(d) end)\n", encoding="utf-8")
        assert re.search(rf"main\.luau:{lines + 1}:1: error: p4\.data\.edit runs only in data\.luau", p4mod(1, "check", mod))
        (mod / "main.luau").write_text(script, encoding="utf-8")

        # [table-source]: the game's own metadata rules and table XML decoder, then features.
        ini = (mod / "mod.ini").read_text(encoding="utf-8")
        tables = mod / "data" / "tables"
        tables.mkdir(parents=True)
        rows = tables / "rows.xml"
        rows.write_text('<p4-tables schema="1">\n  <skill id="600" active="1" element="0" element-aux="0" '
                        'inheritance-group="0" inheritance-rate="0" key="demo:bolt" derive="1" delivery="magic"/>\n'
                        '  <status-ailment element="24" flags="12" cure-mask="0" key="demo:freeze"/>\n'
                        '  <item id="1600" key="demo:weapon" derive="4" icon-source="anchor"/>\n</p4-tables>\n',
                        encoding="utf-8")
        section = "[table-source demo:rows]\nfile=data/tables/rows.xml\nformat=p4-tables-xml\nschema=1\nrevision=1\n"
        (mod / "mod.ini").write_text(ini + section, encoding="utf-8")
        output = p4mod(0, "check", mod)
        assert re.search(r"data/tables/rows\.xml:2:1: warning: uses <skill delivery=\.\.\.> "
                         r"\(capability table-source\.skill-delivery@1\)", output), output
        assert "(capability table-source.skill-derive@1)" in output, output
        assert re.search(r"data/tables/rows\.xml:3:1: warning: uses <status-ailment> \(capability table-source\.status@3\)",
                         output), output
        assert re.search(r"data/tables/rows\.xml:4:1: warning: uses <item icon-source=\.\.\.> "
                         r"\(capability table-source\.item-icon-anchor@1\)", output), output
        assert "manifest.table-source@1" in output, output
        assert "the IDs and keys they claim and the families they join are checked when the game builds" in output, output
        for text, expected in [
                ('<p4-tables schema="1">\n  <skill id="600" key="demo:bolt" delivery="magic"/>\n</p4-tables>\n',
                 r"rows\.xml:2:1: error: Invalid skill fields \(line2\)"),
                ('<!DOCTYPE x>\n<p4-tables schema="1"/>\n', r"error: Table XML DTDs are unsupported"),
                ('<p4-tables schema="1">\n  <p4-tables schema="1"><skill/></p4-tables>\n</p4-tables>\n',
                 r"error: Nested table records are unsupported"),
                ('<tables schema="1">\n  <skill/>\n</tables>\n', r"error: Require p4-tables root with supported schema1"),
                ('<p4-tables schema="1">\0</p4-tables>\n', r"error: Embedded NUL in table source"),
                ('<p4-tables schema="1">\n</p4-tables>\n', r"error: Incomplete or empty table source")]:
            rows.write_text(text, encoding="utf-8")
            output = p4mod(1, "check", mod)
            assert re.search(expected + r".*The game refuses the mod", output), (text, output)
        rows.write_bytes(b'<p4-tables schema="1">' + b" " * (8 * 1024 * 1024) + b"</p4-tables>")
        assert "rows.xml: error: is larger than 8192 KiB" in p4mod(1, "check", mod)
        (mod / "mod.ini").write_text(ini + section.replace("format=p4-tables-xml\n", ""), encoding="utf-8")
        assert "table-source requires explicit file, supported p4-tables-xml/schema1" in p4mod(1, "check", mod)
        (mod / "mod.ini").write_text(ini + section + section, encoding="utf-8")
        assert "Duplicate table-source key across providers" in p4mod(1, "check", mod)
        (mod / "mod.ini").write_text(ini + section.replace("rows.xml", "missing.xml"), encoding="utf-8")
        assert "Table source must be an authorized contained indexed regular file within8MiB" in p4mod(1, "check", mod)
        # The game's `when` gate (mod_setting_gate.c): its rules are errors; a section off by
        # default is still checked, as warnings, for when the player turns it on.
        restart = "[setting extra]\nkind=toggle\nlabel=Extra rows\napply=restart\n"
        gated = section + "when=extra\n"
        (mod / "mod.ini").write_text(ini + section + "when=nope\n", encoding="utf-8")
        assert "when names an undeclared setting: nope" in p4mod(1, "check", mod)
        (mod / "mod.ini").write_text(ini + "[setting extra]\nkind=toggle\nlabel=Extra rows\n" + gated, encoding="utf-8")
        assert "when needs a setting with apply = restart: extra" in p4mod(1, "check", mod)
        (mod / "mod.ini").write_text(ini + section + "when=extra\nwhen=extra\n", encoding="utf-8")
        assert "a section takes at most one when" in p4mod(1, "check", mod)
        rows.write_text('<p4-tables schema="1">\n  <skill id="600" key="demo:bolt"/>\n</p4-tables>\n', encoding="utf-8")
        (mod / "mod.ini").write_text(ini + restart + gated, encoding="utf-8")
        output = p4mod(0, "check", mod)
        assert re.search(r"rows\.xml:2:1: warning: with extra on \(off by default\): Invalid skill fields", output), output
        (mod / "mod.ini").write_text(ini + restart + "default=on\n" + gated, encoding="utf-8")
        assert re.search(r"rows\.xml:2:1: error: Invalid skill fields", p4mod(1, "check", mod))
        # Companion gates (mod_setting_gate.c): the game's table-source metadata takes them, and
        # they are capability manifest.companion-setting@1.
        rows.write_text('<p4-tables schema="1">\n  <status-ailment element="24" flags="12" cure-mask="0" '
                        'key="demo:freeze"/>\n</p4-tables>\n', encoding="utf-8")
        (mod / "mod.ini").write_text(ini + "[companion demo.other]\nversion=1.0\n" + section +
                                     "companion=demo.other\ncompanion-setting=split=on\n", encoding="utf-8")
        output = p4mod(0, "check", mod)
        assert "Unknown table-source declaration field" not in output, output
        assert re.search(r"mod\.ini:\d+:1: warning: uses \[table-source\] companion= "
                         r"\(capability manifest\.companion-setting@1\)", output), output
        (mod / "mod.ini").write_text(ini, encoding="utf-8")
        rows.unlink()
        tables.rmdir()
        (mod / "data").rmdir()

        # The game's package index refuses links; check reports it rather than following one.
        outside = root / "outside.luau"
        outside.write_text('p4.log("outside")\n', encoding="utf-8")
        if package_symlink(mod / "data.luau", outside):
            output = p4mod(1, "check", mod)
            assert "symlink/reparse-point" in output and "the game's package index refuses this folder" in output, output
            assert "outside" not in output.replace(str(outside), ""), output
            (mod / "data.luau").unlink()
        outside.unlink()

        ini = (mod / "mod.ini").read_text(encoding="utf-8")
        (mod / "mod.ini").write_text(ini.replace("api = 1.", "api = 1.9") + "author = me\nrequires = nope.thing@1\n"
                                     "[frobnicate]\nx = 1\n", encoding="utf-8")
        output = p4mod(1, "check", mod)
        assert re.search(r"mod\.ini:5:1: error: api 1\.9\d+ is newer than this game's API 1\.\d+", output), output
        assert re.search(r'mod\.ini:9:1: error: unknown \[mod\] key "author"', output), output
        assert re.search(r'mod\.ini:10:1: error: "requires" appears twice', output), output
        assert re.search(r"mod\.ini:11:1: error: \[frobnicate\] is not a section this build reads", output), output
        (mod / "mod.ini").write_text(ini.replace("events.battle_start@1", "nope.thing@1, ui.status@99"), encoding="utf-8")
        output = p4mod(1, "check", mod)
        assert "requires nope.thing@1, which this build does not provide" in output, output
        assert re.search(r"requires ui\.status@99, but this build provides ui\.status@\d", output), output
        assert "uses event:battle_start (capability events.battle_start@1)" in output, output
        assert "not packed" in p4mod(1, "pack", mod)
        (mod / "mod.ini").write_text(ini, encoding="utf-8")

        # pack: one top folder named by the id, every file but hidden ones and zips.
        (mod / ".git").mkdir()
        (mod / ".git" / "HEAD").write_text("x", encoding="utf-8")
        (mod / "files" / "DATA").mkdir(parents=True)
        (mod / "files" / "DATA" / "BIG.BIN").write_bytes(b"\x07" * 5000)
        assert "(4 files)" in p4mod(0, "pack", mod, "--out", root / "out.zip")
        with zipfile.ZipFile(root / "out.zip") as archive:
            assert archive.testzip() is None
            assert sorted(archive.namelist()) == ["demo.mod/README.md", "demo.mod/files/DATA/BIG.BIN",
                                                  "demo.mod/main.luau", "demo.mod/mod.ini"], archive.namelist()
            assert archive.read("demo.mod/files/DATA/BIG.BIN") == b"\x07" * 5000
            assert archive.getinfo("demo.mod/files/DATA/BIG.BIN").compress_type == zipfile.ZIP_DEFLATED
            assert archive.read("demo.mod/mod.ini") == (mod / "mod.ini").read_bytes()
        assert "demo.mod-0.1.zip" in p4mod(0, "pack", mod)
        assert (root / "demo.mod-0.1.zip").is_file()
        default_zip = root / "demo.mod-0.1.zip"
        default_bytes = default_zip.read_bytes()
        assert "already exists" in p4mod(2, "pack", mod)
        assert default_zip.read_bytes() == default_bytes
        prior = (root / "out.zip").read_bytes()
        assert "already exists" in p4mod(2, "pack", mod, "--out", root / "out.zip")
        assert (root / "out.zip").read_bytes() == prior
        link = root / "linked.zip"
        try:
            link.symlink_to(root / "out.zip")
        except (OSError, NotImplementedError):
            pass  # Windows may not grant symlink creation to this account.
        else:
            assert "already exists" in p4mod(2, "pack", mod, "--out", link)
            assert (root / "out.zip").read_bytes() == prior
        # Package sources must not point outside the mod. No archive is created
        # for a file or directory link, and an ordinary archive contains no secret.
        secret = root / "private.txt"
        secret.write_bytes(b"private data outside the mod")
        source_link = mod / "files" / "DATA" / "SECRET.TXT"
        if package_symlink(source_link, secret):
            rejected = root / "source-link.zip"
            output = p4mod(1, "pack", mod, "--out", rejected)  # the game's index refuses the folder
            assert "symlink/reparse-point" in output, output
            assert not rejected.exists() and secret.read_bytes() == b"private data outside the mod"
            with zipfile.ZipFile(root / "out.zip") as archive:
                assert all(b"private data outside the mod" not in archive.read(name) for name in archive.namelist())
            source_link.unlink()
        directory_link = mod / "files" / "DATA" / "OUTSIDE"
        if package_symlink(directory_link, root, target_is_directory=True):
            rejected = root / "directory-link.zip"
            output = p4mod(1, "pack", mod, "--out", rejected)
            assert "symlink/reparse-point" in output, output
            assert not rejected.exists() and secret.read_bytes() == b"private data outside the mod"
            directory_link.unlink()
        root_link = root / "mod-link"
        if package_symlink(root_link, mod, target_is_directory=True):
            rejected = root / "root-link.zip"
            output = p4mod(2, "pack", str(root_link) + os.sep, "--out", rejected)
            assert "symlink/reparse-point" in output and not rejected.exists(), output
            root_link.unlink()
        if hasattr(os, "mkfifo") and game_fixture.suffix.lower() != ".exe":
            fifo = mod / "files" / "DATA" / "SPECIAL"
            os.mkfifo(fifo)
            output = p4mod(1, "pack", mod, "--out", root / "special.zip")
            assert "special" in output and not (root / "special.zip").exists(), output
            fifo.unlink()
        # pack . writes beside the canonical folder, never inside it.
        default_zip.unlink()
        assert "demo.mod-0.1.zip" in p4mod(0, "pack", ".", cwd=mod)
        assert default_zip.is_file() and not (mod / "demo.mod-0.1.zip").exists()
        default_zip.unlink()
        # A version that is not a plain file name never names the zip; --out never names a stream.
        ini = (mod / "mod.ini").read_text(encoding="utf-8")
        for version in ("../../escape", "1:stream", "a/b", ".hidden"):
            (mod / "mod.ini").write_text(ini.replace("version = 0.1", "version = " + version), encoding="utf-8")
            assert "cannot name the zip" in p4mod(2, "pack", mod), version
        (mod / "mod.ini").write_text(ini, encoding="utf-8")
        assert not (root / "escape.zip").exists() and not (root.parent / "escape.zip").exists()
        assert "not a file name for the zip" in p4mod(2, "pack", mod, "--out", str(root / "x.zip") + ":stream")
        # A bare `p4mod` run through PATH reads its data from its own folder, not the current one.
        if len(command) == 1 and (os.name == "nt" or not command[0].lower().endswith(".exe")):
            exe = Path(command[0]).resolve()
            planted = root / "planted"
            planted.mkdir()
            (planted / "p4.d.luau").write_text("this is not Luau\n", encoding="utf-8")
            (planted / "p4mod-capabilities.txt").write_text("", encoding="utf-8")
            path_env = dict(env, PATH=str(exe.parent) + os.pathsep + env.get("PATH", ""))
            if os.name == "nt":
                # CreateProcess finds a bare name through the parent's PATH, not the child's env:
                # cmd.exe does the lookup in the child's PATH, as a user's shell would. /s strips
                # the outer quotes; the quoted path is literal for cmd except % and ", so refuse
                # those rather than escape them.
                assert '"' not in str(mod) and "%" not in str(mod) and '"' not in exe.name, (mod, exe)
                shell = os.environ.get("ComSpec", r"C:\Windows\System32\cmd.exe")
                launch = f'"{shell}" /d /v:off /s /c "{exe.name} check "{mod}""'
            else:
                launch = [exe.name, "check", str(mod)]
            result = subprocess.run(launch, capture_output=True, text=True, env=path_env, cwd=planted)
            assert result.returncode == 0 and "0 error(s)" in result.stdout, (result.returncode, result.stdout,
                                                                             result.stderr)
            for name in ("p4.d.luau", "p4mod-capabilities.txt"):
                (planted / name).unlink()
            planted.rmdir()

        # setup names the disc by its SLUS_217.82 and records it in the game's own settings.
        assert "no disc is set up" in p4mod(2, "test", mod)
        disc = root / "disc.iso"
        disc.write_bytes(bytes(0x9000))
        assert "is not an ISO 9660 disc image" in p4mod(2, "setup", "--disc", disc)
        assert "is not the Persona 4 (USA) disc" in p4mod(2, "setup", "--disc", disc_image(disc, b"OTHER.ELF;1"))
        assert "not the retail one this port is built for" in p4mod(2, "setup", "--disc", disc_image(disc, b"SLUS_217.82;1"))
        assert "needs --disc" in p4mod(2, "setup")
        # A disc path the game's settings reader would not read back exactly is refused before
        # anything is written: a comment after a space, a line break (a new key or section).
        commented = root / "disc ;x.iso"
        commented.write_bytes(bytes(0x9000))
        assert "cannot be recorded in p4pc.ini exactly" in p4mod(2, "setup", "--disc", commented)
        commented.unlink()
        if os.name != "nt" and not command[-1].lower().endswith(".exe"):
            injected = root / "disc\n[game]\niso=evil.iso"
            injected.write_bytes(bytes(0x9000))
            assert "cannot be recorded in p4pc.ini exactly" in p4mod(2, "setup", "--disc", injected)
            injected.unlink()
        # Windows: the game's file layer is ANSI, so a non-ASCII argument is refused outright
        # (the CRT would best-fit it onto another ASCII path) instead of opening another file.
        if command[-1].lower().endswith(".exe"):
            assert "non-ASCII" in p4mod(2, "check", root / "mod-\u00e9")
            assert "non-ASCII" in p4mod(2, "new", "x", "--dir", root / "dir-\u00e9")
            assert not (root / "dir-\u00e9").exists() and not (root / "dir-e").exists()
        assert not (root / "p4pc.ini").exists()
        (root / "p4pc.ini").write_bytes(b"[video]\r\nscale=2\r\n[game]\r\niso=old.iso\r\nwidescreen=1\r\n")
        retail = os.environ.get("P4PC_ISO")
        if retail and Path(retail).is_file():
            assert "recorded" in p4mod(0, "setup", "--disc", retail)
            settings = (root / "p4pc.ini").read_bytes().decode()
            match = re.fullmatch(r"\[video\]\r\nscale=2\r\n\[game\]\r\niso=(.+)\r\nwidescreen=1\r\n", settings)
            recorded = match.group(1).replace("\\", "/") if match else ""
            if os.name != "nt" and re.match(r"[A-Za-z]:/", recorded):
                recorded = recorded[2:]  # Wine's Z: drive is the Linux root
            assert match and Path(recorded).resolve() == Path(retail).resolve(), settings
        (root / "p4pc.ini").write_text(f"[game]\niso={disc}\n", encoding="utf-8")
        assert "no game at" in p4mod(2, "test", mod, "--game", root / "missing.exe")
        assert "--scenario is boot, newgame or battle" in p4mod(2, "test", mod, "--scenario", "fight")
        assert "--save is only for --scenario battle" in p4mod(2, "test", mod, "--save", root / "nothing")
        assert "needs --save" in p4mod(2, "test", mod, "--scenario", "battle", "--encounter", "1")
        assert "--encounter is a number from 0 to 65535" in p4mod(2, "test", mod, "--scenario", "battle",
                                                                  "--save", root / "nothing", "--encounter", "65536")
        # A battle starts only from a complete native save image. A damaged or
        # truncated file is named before any work folder or game exists.
        card = root / "card" / "BASLUS-21782Save0"
        card.mkdir(parents=True)
        data = card / "BASLUS-21782Save0"
        image = save_image()
        damaged = bytearray(image)
        damaged[200] ^= 1
        battle_args = ("test", mod, "--scenario", "battle", "--save", card, "--encounter", "0x10", "--game", game_fixture)
        before = sorted(scratch.iterdir())
        data.write_bytes(bytes(damaged))
        output = p4mod(2, *battle_args)
        assert "is not a Persona 4 save image the battle can start from: its record checksum does not match" in output, output
        data.write_bytes(image[:-1])
        assert "its end record is missing" in p4mod(2, *battle_args)
        data.write_bytes(image)
        # A valid save still needs the retail executable from the recorded disc.
        assert "not the retail one this port is built for" in p4mod(2, *battle_args)
        # The battle sets the mod-witness marker itself; an inherited one, even empty, is refused.
        assert "P4PC_MOD_WITNESS is set" in p4mod(2, *battle_args, environment=dict(env, P4PC_MOD_WITNESS=""))
        # Encounters are formations: the catalog keys them but names none, so a name is refused.
        output = p4mod(2, *battle_args[:7], "Cowardly Maya", "--game", game_fixture)
        assert "formations keys but no names" in output, output
        # Quoted plan names are resolved only where the plan takes a skill, item or enemy ID.
        plan = root / "plan.txt"
        for text, message in (('battle:first=skill:"Agi,cross\n', ':1:20: the quoted name has no closing quote'),
                              ('battleturn:ally:"Yosuke"=cross\n', ':1:17: party members are native character IDs'),
                              ('battle:delay="20"\n', ':1:14: a quoted name is resolved only after'),
                              ('battle:first=cross\nwalk:shadow\n', ':2:1: expected a battle:, battlerule: or battleturn: line')):
            plan.write_text(text, encoding="ascii")
            output = p4mod(2, *battle_args, "--plan", plan)
            assert message in output, (text, output)
        assert sorted(scratch.iterdir()) == before
        if retail and Path(retail).is_file():
            (root / "p4pc.ini").write_text(f"[game]\niso={retail}\n", encoding="utf-8")
            output = p4mod(0, *battle_args, environment=dict(env, P4PC_AUTOPRESS="cross@1:1/1"))
            assert "battle win (native_battle_exit_victory) after 123 native updates; encounter 16, " \
                   "typical-player-v1, seed 0" in output and "active mod: demo.mod 0.1" in output, output
            assert "p4mod test: PASS" in output, output
            match = re.search(r"p4mod test: demo\.mod \(battle\), log (.+)", output)
            work = Path(match.group(1).strip().replace("\\", "/")).parent
            if os.name != "nt" and re.match(r"(?i)^z:/", str(work)):
                work = Path(str(work)[2:])
            assert (work / "p4pc-run-mode.json").is_file() and (work / "entry.bin").read_bytes()[64:] == image
            # A battle that has not ended, a result for another mod set, or no
            # result at all is not a pass; the user's save stays as it was.
            output = p4mod(1, *battle_args, environment=dict(env, P4MOD_FIXTURE_OUTCOME="update_limit"))
            assert "FAIL: the battle did not end within 100000 native updates" in output, output
            output = p4mod(1, *battle_args, environment=dict(env, P4MOD_FIXTURE_IDENTITY="0" * 64))
            assert "FAIL: the battle worker's result names another mod set" in output, output
            output = p4mod(1, *battle_args, environment=dict(env, P4MOD_FIXTURE_OUTCOME="none"))
            assert "FAIL: the battle worker published no result" in output, output
            # Quoted names need the game's catalog: a launch that never answers is not resolved.
            plan.write_text('battle:first=circle,down,cross,skill:"Agilao",cross\n', encoding="ascii")
            output = p4mod(1, *battle_args, "--plan", plan)
            assert "FAIL: the game's catalog did not answer" in output, output
            assert data.read_bytes() == image and sorted(card.iterdir()) == [data]
            (root / "p4pc.ini").write_text(f"[game]\niso={disc}\n", encoding="utf-8")
        # Boot still stops at frame 900, but reaching that frame without the
        # title-ready signal is not success. Both playable title states count.
        output = p4mod(1, "test", mod, "--scenario", "boot", "--game", game_fixture)
        assert "gate=900,-1:-1 pace=1" in output, output
        assert "p4mod test: FAIL: the game did not reach the boot scenario" in output, output
        for frame in ("901", "902"):
            output = p4mod(0, "test", mod, "--scenario", "boot", "--frames", frame, "--game", game_fixture)
            assert f"gate={frame},-1:-1 pace=1" in output and "p4mod test: PASS" in output, output
        # A successful process and frame-only gate PASS cannot prove a New Game
        # entered the field. The requested gate must itself name that state.
        output = p4mod(1, "test", mod, "--scenario", "newgame", "--game", game_fixture)
        assert "gate=30500,7:2" in output, output
        assert "p4mod test: FAIL: the game did not reach the newgame scenario" in output, output
        output = p4mod(0, "test", mod, "--scenario", "newgame", "--frames", "30501", "--game", game_fixture)
        assert "gate=30501,7:2" in output and "p4mod test: PASS" in output, output
        # PASS comes from the engine's gate result file, not the log: every success line a mod
        # could print, with an engine result that says otherwise, is a FAIL.
        forged_env = dict(env, P4MOD_FIXTURE_FORGE="1")
        for args in (("--scenario", "boot", "--frames", "901"), ("--scenario", "newgame", "--frames", "30501")):
            result = subprocess.run([*command, "test", str(mod), *args, "--game", str(game_fixture)], capture_output=True,
                                    text=True, env=forged_env, cwd=root)
            output = result.stdout + result.stderr
            assert result.returncode == 1 and "p4mod test: FAIL: the game did not reach" in output, (args, output)
        for frames in ("-1", "0", "1000001", "1e3", "+5", ""):
            assert "--frames is a whole number" in p4mod(2, "test", mod, "--frames", frames, "--game", game_fixture), frames
        # Neither concurrent invocation may reuse/delete the old predictable
        # directory or the other's work; keep a sentinel at the former path.
        old_work = scratch / "p4mod-test-demo.mod"
        old_work.mkdir()
        sentinel = old_work / "personal.txt"
        sentinel.write_bytes(b"do not delete or overwrite")
        held_env = dict(env, P4MOD_FIXTURE_HOLD="1")
        args = [*command, "test", str(mod), "--scenario", "boot", "--frames", "901", "--game", str(game_fixture)]
        children = [subprocess.Popen(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                     env=held_env, cwd=root) for _ in range(2)]
        try:
            assert all(child.poll() is None for child in children), "fixture runs did not overlap"
            outputs = []
            for child in children:
                stdout, stderr = child.communicate(timeout=20)
                assert child.returncode == 0, (child.returncode, stdout, stderr)
                output = stdout + stderr
                assert "p4mod test: PASS" in output, output
                match = re.search(r"p4mod test: demo\.mod \(boot\), log (.+)", output)
                assert match, output
                path = match.group(1).strip().replace("\\", "/")
                if os.name != "nt" and re.match(r"(?i)^z:/", path):
                    path = path[2:]
                outputs.append(Path(path).resolve())
            assert outputs[0] != outputs[1], outputs
            assert all(path.parent.parent == scratch.resolve() and path.is_file() for path in outputs), outputs
            # Each run's folder is private to this user and kept, with its evidence, after the run.
            if os.name != "nt" and game_fixture.suffix.lower() != ".exe":
                assert all((path.parent.stat().st_mode & 0o777) == 0o700 for path in outputs), outputs
            assert sentinel.read_bytes() == b"do not delete or overwrite"
        finally:
            for child in children:
                if child.poll() is None:
                    child.kill()
                    child.communicate()
    print("p4mod: help, new, check diagnostics, pack and setup passed")


def disc_image(path, name):
    """An ISO 9660 image whose root directory holds one file, `name`, of a few bytes."""
    def record(extent, size, identifier):
        body = bytes([0]) + extent.to_bytes(4, "little") + extent.to_bytes(4, "big") + size.to_bytes(4, "little") + \
            size.to_bytes(4, "big") + bytes(7) + bytes([0, 0, 0]) + (1).to_bytes(2, "little") + (1).to_bytes(2, "big") + \
            bytes([len(identifier)]) + identifier
        body = bytes([len(body) + 1 + (len(body) + 1) % 2]) + body
        return body + bytes(body[0] - len(body))
    volume = bytearray(2048)
    volume[0:6] = b"\x01CD001"
    volume[156:156 + 34] = record(20, 2048, b"\x00")
    directory = record(20, 2048, b"\x00") + record(20, 2048, b"\x01") + record(21, 5, name)
    image = bytearray(22 * 2048)
    image[16 * 2048:17 * 2048] = volume
    image[20 * 2048:20 * 2048 + len(directory)] = directory
    image[21 * 2048:21 * 2048 + 5] = b"\x7fELF\x01"
    path.write_bytes(bytes(image))
    return path


SECTIONS = Path(__file__).resolve().parents[1] / "include" / "p4pc" / "lockstep_save_sections.inc"


def save_image():
    """A checksum-valid native save image with the game's record layout and zero data: a format fixture, not a party."""
    rows = re.findall(r"^P4LS_SAVE_SECTION\((0x[0-9a-fA-F]+),0x[0-9a-fA-F]+,(\d+)\)", SECTIONS.read_text(encoding="ascii"), re.M)
    records = b"".join(int(tag, 16).to_bytes(4, "little") + int(size).to_bytes(4, "little") + bytes(int(size))
                       for tag, size in rows)
    return bytes(56) + records + (0x2000).to_bytes(4, "little") + (1).to_bytes(4, "little") + \
        bytes([sum(records) & 255]) + (0xFFFFFFFF).to_bytes(4, "little")


if __name__ == "__main__":
    main()
