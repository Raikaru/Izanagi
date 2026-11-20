"""Build-time fixture generation through the production mod entry generator."""
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / "tools"))
import gen_port
import mod_codegen

out = pathlib.Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)
gen_port.write_if_changed(out / "game.c", (root / "tests/mod_hooks_game.c").read_text(encoding="utf-8"))
count = mod_codegen.instrument(out, ["game.c"], out, gen_port)
if count != 9:
    raise SystemExit(f"expected nine typed fixture entries, generated {count}")
