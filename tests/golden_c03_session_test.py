"""Run a real scripted Golden session and require an untainted C03 guard."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

elf = Path(os.environ.get("P4PC_ELF", ""))
iso = Path(os.environ.get("P4PC_ISO", ""))
golden = Path(os.environ.get("P4PC_GOLDEN", ""))
if not elf.is_file() or not iso.is_file() or not (golden / "data_e.cpk").is_file():
    print("C03 scripted session needs P4PC_ELF, P4PC_ISO and P4PC_GOLDEN", file=sys.stderr)
    sys.exit(77)
if len(sys.argv) < 2:
    raise SystemExit("driver required")
with tempfile.TemporaryDirectory(prefix="p4-c03-session-") as directory:
    work = Path(directory)
    config = work / "p4pc.ini"
    config.write_text("[golden]\ninstall=" + str(golden.resolve()) + "\n", encoding="utf-8")
    env = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MODE="golden", P4PC_INPUT="script",
               P4PC_ISO=str(iso.resolve()), P4PC_SAVE_DIR=str(work / "save"),
               P4PC_NEWGAME_GATE="30500,7:2", P4PC_TURBO="2", P4PC_TRAP_CONTINUE="1",
               P4PC_AUTOPRESS_FRAMES="1", P4PC_AUTOPRESS_NEWGAME="1560",
               P4PC_AUTOPRESS="cross@2300:2/150,start@6500:2/300",
               SDL_AUDIODRIVER="dummy", LIBGL_ALWAYS_SOFTWARE="1", GALLIUM_DRIVER="llvmpipe",
               XDG_DATA_HOME=str(work / "xdg"))
    if not any("wine" in Path(part).name.lower() for part in sys.argv[1:-1]):
        env["SDL_VIDEODRIVER"] = "offscreen"
    result = subprocess.run([*sys.argv[1:], str(elf.resolve())], cwd=work, env=env,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, errors="replace", timeout=1500, check=False)
    if result.returncode or "new-game gate PASS" not in result.stdout or \
       "Golden C03 session clean" not in result.stdout or "Golden C03 tainted" in result.stdout:
        print(result.stdout, file=sys.stderr)
        raise SystemExit(1)
    print("Golden C03 scripted new-game session remained clean")
