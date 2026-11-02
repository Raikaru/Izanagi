"""Real configured discovery and atomic selector persistence, without game data."""
import configparser
import os
from pathlib import Path
import subprocess
import sys
import tempfile

end = sys.argv.index("--driver-end")
driver = sys.argv[1:end]
with tempfile.TemporaryDirectory(prefix="p4-mod-selector-") as temp:
    root = Path(temp)
    mods = root / "mods"
    for name, priority in (("low", -5), ("high", 5)):
        folder = mods / name
        folder.mkdir(parents=True)
        (folder / "mod.ini").write_text(
            f"[mod]\nid=mod.{name}\nname={name}\nversion=1\napi=1.0\npriority={priority}\norder=0\n",
            encoding="utf-8")
    for name in ("duplicate-one", "duplicate-two"):
        folder = mods / name
        folder.mkdir(parents=True)
        (folder / "mod.ini").write_text(
            "[mod]\nid=mod.duplicate\nname=duplicate\nversion=1\napi=1.0\npriority=0\norder=0\n",
            encoding="utf-8")
    packs = root / "packs"
    for folder_name, pack_id in (("single", "pack.single"),
                                 ("duplicate-one", "pack.duplicate"),
                                 ("duplicate-two", "pack.duplicate")):
        folder = packs / folder_name
        folder.mkdir(parents=True)
        (folder / "pack.ini").write_text(
            f"[pack]\nid={pack_id}\nname={folder_name}\nversion=1\napi=1.0\npriority=0\norder=0\n",
            encoding="utf-8")
    config = root / "settings.ini"
    config.write_text("[mods]\nenabled = mod.low,mod.high\nload_order =\n[packs]\nenabled =\n", encoding="utf-8")
    env = dict(os.environ, P4PC_CONFIG=str(config), P4PC_MOD_DIR=str(mods),
               P4PC_PACK_DIR=str(packs), P4PC_MODS="0", P4PC_PACKS="0", P4PC_INPUT="script")
    result = subprocess.run(driver, env=env, text=True, capture_output=True)
    if result.returncode:
        raise SystemExit(result.stdout + result.stderr)
    settings = configparser.ConfigParser()
    settings.read(config, encoding="utf-8")
    assert settings["mods"]["enabled"] == "mod.low,mod.high"
    assert settings["mods"]["load_order"] == "mod.low,mod.high"
    assert settings["packs"]["enabled"] == "pack.single"
    print(result.stdout, end="")
