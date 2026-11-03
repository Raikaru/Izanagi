"""Windows-side shell code may only call tools that Git for Windows ships.

The Windows-side scripts run under Git Bash, whose /usr/bin has no grep (nor pgrep, pkill, lsof,
flock, xz or zstd). Text that runs elsewhere is exempt: heredoc bodies and quoted command strings,
and the scripts that only ever run on a Linux host or in a container. The private repository's own
scripts are listed in windows_tool_lint_private.py.
"""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
MISSING_ON_WINDOWS = ("grep", "egrep", "fgrep", "pgrep", "pkill", "lsof", "flock", "xz", "zstd")
HOST_ONLY = {
    "android_ci_run.sh", "appimage_build.sh", "appimage_smoke.sh", "build_cache.sh", "ci_tests.sh", "ios_check.sh",
    "package_linux.sh", "package_macos.sh", "package_runtime_provenance.sh", "package_win.sh",
}
PC = {
    "luau_profiles.sh", "package_smoke.sh", "ref_guard.sh",
}
try:
    import windows_tool_lint_private as private
except ImportError:
    private = None
if private is not None:
    HOST_ONLY |= private.HOST_ONLY
    PC |= private.PC
COMMAND = re.compile(r"(?:^|[|;&(!`{]|\$\(|\bthen\b|\bdo\b|\belse\b|\bexec\b|\btimeout\s+\S+)\s*(%s)\b"
                     % "|".join(map(re.escape, MISSING_ON_WINDOWS)))
HEREDOC = re.compile(r"<<-?\s*(['\"]?)(\w+)\1")


def local_code(text):
    """Yield (line number, code) for text the PC itself runs: no heredoc bodies, quotes or comments.
    A $( ... ) inside double quotes still runs locally, so its contents count as code."""
    heredoc, quote, depth = None, None, 0
    for number, line in enumerate(text.splitlines(), 1):
        if heredoc is not None:
            if line.strip() == heredoc:
                heredoc = None
            continue
        code, index = [], 0
        while index < len(line):
            char = line[index]
            if quote == '"' and depth == 0 and line.startswith("$(", index):
                depth = 1
                code.append("$(")
                index += 2
                continue
            if quote and depth:
                if char == "\\":
                    index += 2
                    continue
                depth += char == "("
                depth -= char == ")"
                code.append(char)
                index += 1
                continue
            if quote:
                if char == "\\" and quote == '"':
                    index += 2
                    continue
                if char == quote:
                    quote = None
                index += 1
                continue
            if char == "\\":
                index += 2
                continue
            if char in "'\"":
                quote = char
            elif char == "#" and (index == 0 or line[index - 1] in " \t;"):
                break
            else:
                code.append(char)
            index += 1
        code = "".join(code)
        opened = HEREDOC.search(line) if "<<" in code else None
        if opened:
            heredoc = opened.group(2)
        yield number, code


class WindowsToolLintTest(unittest.TestCase):
    def test_every_script_is_classified(self):
        scripts = {path.name for path in (ROOT / "tools").glob("*.sh")}
        self.assertEqual(scripts - PC - HOST_ONLY, set(), "classify new scripts as PC or HOST_ONLY")
        self.assertEqual((PC | HOST_ONLY) - scripts, set(), "drop removed scripts from the lists")
        self.assertEqual(PC & HOST_ONLY, set())

    def test_pc_scripts_call_only_tools_git_for_windows_ships(self):
        found = []
        for name in sorted(PC):
            text = (ROOT / "tools" / name).read_text(encoding="utf-8")
            for number, code in local_code(text):
                match = COMMAND.search(code)
                if match:
                    found.append("tools/%s:%d: %s" % (name, number, match.group(1)))
        self.assertEqual(found, [], "use shell builtins or awk/sed instead: these tools are absent from "
                                    "Git for Windows")

    def test_scanner_skips_remote_text_and_catches_local_calls(self):
        sample = ('x=$(grep -c a file)\n'
                  'ssh host "grep -q b /etc/x"\n'
                  "ssh host 'flock -n l true'\n"
                  "cat <<'EOF' | ssh host sh\ngrep local-to-host\nEOF\n"
                  '# grep in a comment\n'
                  'if docker --version | grep -qi podman; then :; fi\n'
                  'until fence_held "$out"; do sleep 5; done\n'
                  'echo "count: $(grep -c x local.txt)"\n'
                  'ssh host "n=\\$(grep -c x remote.txt)"\n')
        flagged = [number for number, code in local_code(sample) if COMMAND.search(code)]
        self.assertEqual(flagged, [1, 8, 10])


if __name__ == "__main__":
    unittest.main()
