"""Write mod_luau_api.h from layout_api.luau exactly as cmake/mods.cmake does."""
import pathlib
import sys

source = pathlib.Path(sys.argv[1]).read_bytes()
text = (source.replace(b"\\", b"\\\\").replace(b'"', b'\\"').replace(b"\r", b"")
        .replace(b"\n", b'\\n"\n"'))
pathlib.Path(sys.argv[2]).write_bytes(b'static const char p4_mod_layout_api[] =\n"' + text + b'";\n')
