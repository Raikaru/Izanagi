#!/usr/bin/env python3
"""Discover Zig's LLD argv, then produce and verify the final mapped link."""
import hashlib
import json
import os
import re
from pathlib import Path
import shlex
import struct
import subprocess
import sys


def digest(path):
    h = hashlib.sha256()
    with open(path, 'rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(block)
    return h.hexdigest()


def split_command(line):
    if os.name != 'nt':
        return shlex.split(line)
    import ctypes
    from ctypes import wintypes
    shell = ctypes.windll.shell32
    shell.CommandLineToArgvW.argtypes = (wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_int))
    shell.CommandLineToArgvW.restype = ctypes.POINTER(wintypes.LPWSTR)
    count = ctypes.c_int()
    argv = shell.CommandLineToArgvW(line, ctypes.byref(count))
    if not argv:
        raise ctypes.WinError()
    try:
        return [argv[i] for i in range(count.value)]
    finally:
        ctypes.windll.kernel32.LocalFree(ctypes.cast(argv, ctypes.c_void_p))


def codeview_guids(path):
    """Read RSDS identities via the PE debug directory, not fixed file offsets."""
    data = Path(path).read_bytes()
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    if data[:2] != b'MZ' or data[pe:pe + 4] != b'PE\0\0':
        raise ValueError('invalid PE signature')
    sections, optional_size = struct.unpack_from('<H12xH', data, pe + 6)
    optional = pe + 24
    magic = struct.unpack_from('<H', data, optional)[0]
    directory = {0x10b: 96, 0x20b: 112}[magic]
    rva, size = struct.unpack_from('<II', data, optional + directory + 6 * 8)
    table = optional + optional_size
    for index in range(sections):
        virtual_size, virtual, raw_size, raw = struct.unpack_from('<IIII', data, table + index * 40 + 8)
        if virtual <= rva and rva + size <= virtual + max(virtual_size, raw_size):
            offset = raw + rva - virtual
            break
    else:
        raise ValueError('PE debug directory is not backed by a section')
    if size % 28 or offset + size > len(data):
        raise ValueError('invalid PE debug directory size')
    guids = []
    for entry in range(offset, offset + size, 28):
        kind, count, _, pointer = struct.unpack_from('<IIII', data, entry + 12)
        if kind == 2:
            if count < 24 or pointer + count > len(data) or data[pointer:pointer + 4] != b'RSDS':
                raise ValueError('invalid CodeView RSDS record')
            guids.append({'offset': pointer + 4, 'guid': data[pointer + 4:pointer + 20].hex()})
    if not guids:
        raise ValueError('PE link did not emit a CodeView identity')
    return guids


def coff_gc_policy(command, raw_argv):
    """Closed translation of the caller's declared policy, not a linker default."""
    forwarded = []
    index = 1
    while index < len(command):
        argument = command[index]
        if argument.startswith('-Wl,'):
            forwarded.extend((index, token) for token in argument[4:].split(','))
        elif argument == '-Xlinker':
            index += 1
            if index >= len(command):
                sys.exit('zig_link_map: -Xlinker needs a value')
            forwarded.append((index, command[index]))
        index += 1
    table = {'--gc-sections': '/OPT:REF'}
    requests = []
    for position, token in forwarded:
        policy = token.lower().startswith(('--gc-', '--no-gc-', '--icf', '--no-icf', '/opt:', '-opt:'))
        if not policy:
            continue
        if token not in table:
            sys.exit(f'zig_link_map: unsupported COFF linker policy: {token}')
        requests.append({'command_index': position, 'caller_token': token, 'lld_token': table[token]})
    # An argv that already asks for it (the image clang's MinGW translation: -opt:ref) needs no second spelling.
    has_ref = any(token.lower() in ('-opt:ref', '/opt:ref') for token in raw_argv)
    added = ['/OPT:REF'] if requests and not has_ref else []
    return [*raw_argv, *added], {'translation_table': table, 'caller_requests': requests, 'argv_added': added}


def main():
    if len(sys.argv) < 6 or sys.argv[3] != '--':
        sys.exit('usage: zig_link_map.py <zig> <map> -- <compiler> <args...>')
    zig, map_path = sys.argv[1:3]
    command = sys.argv[4:]
    try:
        index = command.index('-o')
        output = Path(command[index + 1])
    except (ValueError, IndexError):
        sys.exit('zig_link_map: link must have an explicit output')
    env = dict(os.environ, ZIG_VERBOSE_LINK='1')
    result = subprocess.run(command, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    sys.stdout.buffer.write(result.stdout)
    sys.stderr.buffer.write(result.stderr)
    if result.returncode:
        return result.returncode
    # Zig's driver prints `ld.lld ...`/`lld-link ...`; the image runtime mode (zigbuild/zig_cc.zig) prints the executable path
    # of the linker clang named for an ELF link when that is not LLD: the image's GNU ld, as in the CMake build.
    gnu_line = re.compile(r'^/\S*(?:/|-)ld(?:\.bfd)?\s')
    lines = [line for line in result.stderr.decode('utf-8', errors='strict').splitlines()
             if line.startswith(('ld.lld ', 'lld-link ')) or gnu_line.match(line)]
    if len(lines) != 1:
        sys.exit(f'zig_link_map: expected one linker invocation, got {len(lines)}')
    args = split_command(lines[0])
    linker_exe = args[0]
    linker = linker_exe if linker_exe in ('ld.lld', 'lld-link') else 'ld'
    if linker == 'ld' and not gnu_line.match(lines[0]):
        sys.exit('zig_link_map: unsupported linker')

    def lld(argv):
        # Zig's LLD is `zig ld.lld`/`zig lld-link`; the GNU ld is its own executable (argv[0]).
        return [zig, *argv] if linker != 'ld' else list(argv)
    raw_args = args
    # CodeView (a PDB) or DWARF: the argv decides. The Zig driver's COFF links name a PDB; the image clang's MinGW link (the
    # CMake build's) asks for -debug:dwarf and has none.
    pdb_link = linker == 'lld-link' and any(arg.upper().startswith(('/PDB:', '-PDB:')) for arg in args)
    raw_discovery_hash = digest(output)
    raw_discovery_guids = codeview_guids(output) if pdb_link else []
    policy = None
    if linker == 'lld-link':
        args, policy = coff_gc_policy(command, raw_args)
        if args != raw_args:
            corrected = subprocess.run(lld(args))
            if corrected.returncode:
                return corrected.returncode
    discovery_hash = digest(output)
    discovery_guids = codeview_guids(output) if pdb_link else []
    map_arg = '--Map=' + map_path if linker == 'ld.lld' else '/MAP:' + map_path
    added = ['-Map', map_path] if linker == 'ld' else [map_arg]
    if linker == 'lld-link':
        added.append('/lldmap:' + map_path + '.origins')
    final_args = [*args, *added]
    # No flag rewriting, filtering, or hidden defaults between discovery and final.
    if final_args[:len(args)] != args or final_args[len(args):] != added:
        sys.exit('zig_link_map: final argv differs from discovery beyond the map options')
    products = {'executable': output, 'map': Path(map_path)}
    single_map_hashes = None
    single_map_guids = []
    if pdb_link:
        pdbs = [arg.split(':', 1)[1] for arg in args if arg.upper().startswith(('/PDB:', '-PDB:'))]
        outputs = [arg.split(':', 1)[1] for arg in args if arg.upper().startswith(('/OUT:', '-OUT:'))]
        if len(pdbs) != 1 or len(outputs) != 1 or Path(outputs[0]).resolve() != output.resolve():
            sys.exit('zig_link_map: expected explicit matching COFF output and PDB paths')
        products['pdb'] = Path(pdbs[0])
        single = subprocess.run(lld([*args, map_arg]))
        if single.returncode:
            return single.returncode
        single_map_hashes = {name: digest(path) for name, path in products.items()}
        single_map_guids = codeview_guids(output)
        products['origins_map'] = Path(map_path + '.origins')
    elif linker == 'lld-link':
        outputs = [arg.split(':', 1)[1] for arg in args if arg.upper().startswith(('/OUT:', '-OUT:'))]
        if len(outputs) != 1 or Path(outputs[0]).resolve() != output.resolve():
            sys.exit('zig_link_map: expected an explicit COFF output path matching the caller -o')
        products['origins_map'] = Path(map_path + '.origins')
    hashes = []
    for _ in range(2):
        replay = subprocess.run(lld(final_args))
        if replay.returncode:
            return replay.returncode
        for name, path in products.items():
            if not path.is_file() or not path.stat().st_size:
                sys.exit(f'zig_link_map: linker did not produce {name}: {path}')
        hashes.append({name: digest(path) for name, path in products.items()})
    if hashes[0] != hashes[1]:
        sys.exit(f'zig_link_map: final links are not byte-identical: {hashes}')
    after = hashes[1]['executable']
    if linker != 'lld-link' and discovery_hash != after:
        sys.exit(f'zig_link_map: ELF map changed linked output: {discovery_hash} != {after}')
    final_guids = codeview_guids(output) if pdb_link else []
    version = subprocess.run(lld([linker_exe, '--version']) if linker != 'ld' else [linker_exe, '--version'], check=True,
                             stdout=subprocess.PIPE, text=True).stdout.strip().splitlines()[0]
    Path(map_path + '.linker.json').write_text(json.dumps({
        'linker': os.path.abspath(zig) if linker != 'ld' else linker_exe, 'command': linker, 'version': version,
        'sha256': digest(zig) if linker != 'ld' else digest(linker_exe), 'output_sha256': after, 'product_sha256': hashes[1],
        'compiler_command': command, 'raw_compiler_discovery_argv': raw_args,
        'raw_compiler_discovery_output_sha256': raw_discovery_hash,
        'raw_compiler_discovery_codeview': raw_discovery_guids,
        'declared_coff_gc_policy': policy,
        'discovery_argv': args, 'final_argv': final_args, 'argv_added': added,
        'discovery_output_sha256': discovery_hash, 'discovery_byte_equal': discovery_hash == after,
        'discovery_codeview': discovery_guids, 'final_codeview': final_guids,
        'single_map_product_sha256': single_map_hashes, 'single_map_codeview': single_map_guids,
        'single_map_exe_pdb_byte_equal': all(single_map_hashes[name] == hashes[1][name]
                                           for name in ('executable', 'pdb')) if single_map_hashes else None,
        'debug_info': 'codeview-pdb' if pdb_link else ('dwarf' if linker == 'lld-link' else None),
        'codeview_explanation': 'The PDB embeds linker argv; adding /MAP and /lldmap changes its identity. '
                                'Discovery is not the product; both final links must be byte-identical.'
                                if pdb_link else None,
        'final_links_byte_equal': True,
    }, sort_keys=True) + '\n', encoding='utf-8')
    return 0


if __name__ == '__main__':
    sys.exit(main())
