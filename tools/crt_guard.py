#!/usr/bin/env python3
"""Audit the game/host CRT boundary, including compile-only build profiles.

The object list is supplied by CMake, never inferred from directory spelling.
Unknown externals fail closed; known retail symbols are not an escape for CRT
names. JSON and TSV reports retain the referencing object and function (including
address-taken relocations), so the inventory describes the actual generated code.
"""
import argparse
import csv
import io
import json
import os
from pathlib import Path
import re
import subprocess
import sys

import gen_port
import crt_flow

ROOT = Path(__file__).resolve().parents[1]
# ISO C library entry points, including optional C11 Annex K spellings. Target
# library exports supplement this catalog; unknown non-retail externals also fail.
C_FUNCTIONS = set("""
abort abs acos acosh asin asinh atan atan2 atanh atexit atof atoi atol atoll
at_quick_exit bsearch btowc c16rtomb c32rtomb calloc cbrt ceil clearerr clock
copysign cos cosh ctime difftime div erf erfc exit exp exp2 expm1 fabs fclose
fdim feclearexcept fegetenv fegetexceptflag fegetround feholdexcept feof ferror
fesetenv fesetexceptflag fesetround fetestexcept feupdateenv fflush fgetc fgetpos
fgets fgetwc fgetws floor fma fmax fmin fmod fopen fprintf fputc fputs fputwc
fputws fread free freopen frexp fscanf fseek fsetpos ftell fwrite fwprintf
fwscanf getc getchar getenv gets getwc getwchar gmtime hypot ilogb imaxabs
imaxdiv isalnum isalpha isblank iscntrl isdigit isgraph islower isprint ispunct
isspace isupper iswalnum iswalpha iswblank iswcntrl iswctype iswdigit iswgraph
iswlower iswprint iswpunct iswspace iswupper iswxdigit isxdigit labs ldexp ldiv
lgamma llabs lldiv llrint llround localeconv localtime log log10 log1p log2
logb longjmp lrint lround malloc mblen mbrlen mbrtoc16 mbrtoc32 mbrtowc mbsinit
mbsrtowcs mbstowcs mbtowc memchr memcmp memcpy memmove memset mktime modf
nan nearbyint nextafter nexttoward perror pow printf putc putchar puts putwc
putwchar qsort quick_exit raise rand realloc remainder remquo remove rename
rewind rint round scalbln scalbn scanf setbuf setjmp setlocale setvbuf signal
sin sinh snprintf sprintf sqrt srand sscanf strcat strchr strcmp strcoll
strcpy strcspn strerror strftime strlen strncat strncmp strncpy strpbrk strrchr
strspn strstr strtod strtof strtoimax strtok strtol strtold strtoll strtoul
strtoull strtoumax strxfrm swprintf swscanf system tan tanh tgamma time tmpfile
tmpnam tolower toupper towctrans towlower towupper trunc ungetc ungetwc
vfprintf vfscanf vfwprintf vfwscanf vprintf vscanf vsnprintf vsprintf vsscanf
vswprintf vswscanf vwprintf vwscanf wcrtomb wcscat wcschr wcscmp wcscoll wcscpy
wcscspn wcsftime wcslen wcsncat wcsncmp wcsncpy wcspbrk wcsrchr wcsrtombs wcsspn
wcsstr wcstod wcstof wcstoimax wcstok wcstol wcstold wcstoll wcstombs wcstoul
wcstoull wcstoumax wcsxfrm wctob wctomb wctrans wctype wmemchr wmemcmp wmemcpy
wmemmove wmemset wprintf wscanf _Exit aligned_alloc strerror_s strtok_s
""".split())
for _name in "acos acosh asin asinh atan atan2 atanh cbrt ceil copysign cos cosh erf erfc exp exp2 expm1 fabs fdim floor fma fmax fmin fmod frexp hypot ilogb ldexp lgamma llrint llround log log10 log1p log2 logb lrint lround modf nan nearbyint nextafter nexttoward pow remainder remquo rint round scalbln scalbn sin sinh sqrt tan tanh tgamma trunc".split():
    C_FUNCTIONS.update((_name + "f", _name + "l"))
C_FUNCTIONS.update("stderr stdin stdout errno __errno_location __errno _errno".split())
C_FUNCTIONS.update("__error __stderrp __stdinp __stdoutp".split())   # Darwin's errno and standard streams

# Mach-O objects (--macho): every C name carries a leading underscore, as on i686 COFF.
MACHO = False
RELOCATION_RE = re.compile(r"\b(?:R_[A-Z0-9_]+|IMAGE_REL_[A-Z0-9_]+|ARM64_RELOC_[A-Z0-9_]+|X86_64_RELOC_[A-Z0-9_]+)\s+(\S+)")
# `objdump --macho -r` rows, every relocation entry as stored (`address pcrel length extern type scattered symbol`; arm64 prints a pointer's
# length as `?( 3)`): the generic `-r` listing hides some x86_64 X86_64_RELOC_UNSIGNED entries of a data section (gen_port.MACHO_RELOC_RE).
MACHO_RELOCATION_RE = re.compile(r"^[0-9a-fA-F]+\s+(?:True|False)\s+(?:\?\(\s*\d+\)|\S+)\s+True\s+\S+\s+(?:True|False)\s+(\S+)")


def normalize(raw, coff32=False):
    name = raw.split("@", 1)[0]
    if name.startswith("__imp_"):
        name = name[6:]
    if (coff32 or MACHO) and name.startswith("_"):
        name = name[1:]
    return name


def object_symbols(objects, nm, coff32=False):
    """Use POSIX nm output, with explicit headers to retain each object's identity."""
    defined, references = set(), {str(obj): set() for obj in objects}
    for start in range(0, len(objects), 100):
        chunk = objects[start:start + 100]
        out = subprocess.run([nm, "-A", "-P", "-g", *map(str, chunk)],
                             text=True, capture_output=True, check=True).stdout
        # -A -P is '<path>: <symbol> <type> ...' on GNU and LLVM nm.
        for line in out.splitlines():
            match = re.match(r"^(.*?):\s+(\S+)\s+([A-Za-z?])(?:\s|$)", line)
            if not match:
                if line.strip():
                    raise ValueError(f"unrecognized nm record: {line}")
                continue
            obj, raw, kind = match.groups()
            name = normalize(raw, coff32)
            if kind in ("U", "w", "v"):
                references.setdefault(obj, set()).add(name)
            else:
                defined.add(name)
    return defined, references


def relocation_callers(obj, wanted, objdump, coff32=False):
    # Mach-O: the assembler's section-start label (ltmp0) can sit at a function's address; --show-all-symbols prints the
    # function's own label too, and local labels never name a caller.
    out = subprocess.run([objdump, "-dr", *(["--show-all-symbols"] if MACHO else []), str(obj)], text=True,
                         capture_output=True, check=True).stdout
    found = {name: set() for name in wanted}
    caller = "<section relocation>"
    for line in out.splitlines():
        label = re.match(r"^[0-9a-fA-F]+ <(.+)>:$", line.strip())
        if label:
            if not (MACHO and label[1].startswith(("l", "L"))):
                caller = normalize(label[1], coff32)
            continue
        relocation = RELOCATION_RE.search(line)
        if relocation:
            name = re.sub(r"[+-](?:0x)?[0-9a-fA-F]+$", "", relocation[1])
            name = normalize(name, coff32)
            if name in found:
                found[name].add(caller)
    # -dr omits non-code sections (function-pointer tables, for example).
    out = subprocess.run([objdump, *(["--macho"] if MACHO else []), "-r", str(obj)], text=True,
                         capture_output=True, check=True).stdout
    for line in out.splitlines():
        relocation = MACHO_RELOCATION_RE.match(line.strip()) if MACHO else RELOCATION_RE.search(line)
        if relocation:
            name = normalize(re.sub(r"[+-](?:0x)?[0-9a-fA-F]+$", "", relocation[1]), coff32)
            if name in found and not found[name]:
                found[name].add("<data/address relocation>")
    for name, callers in found.items():
        if not callers:
            raise ValueError(f"{obj}: no relocation found for undefined CRT symbol {name}")
    return found


def native_source(obj):
    """CMake's native source suffix; rewritten mirror objects never match."""
    match = re.search(r"(?:^|/)CMakeFiles/[^/]+\.dir/(src/.+\.(?:c|cc|cpp))\.(?:o|obj)$",
                      str(obj).replace("\\", "/"), re.IGNORECASE)
    return match[1] if match else None

def report_source(obj, commands):
    source = native_source(obj)
    if source:
        return source
    match = re.search(r"/(?:units|decomp)/(src/.+\.c)\.(?:64\.)?(?:o|obj)$",
                      str(obj).replace("\\", "/"))
    if match:
        return match[1]
    entry = (commands or {}).get(os.path.normcase(str(Path(obj).resolve())))
    if entry:
        path = str(entry["file"]).replace("\\", "/")
        if "/gen/" in path:
            return "gen/" + path.rsplit("/gen/", 1)[1]
    return ""




def compilation_database(path):
    commands = {}
    for entry in json.loads(Path(path).read_text(encoding="utf-8")):
        output = entry.get("output")
        if output is None:
            match = re.search(r'(?:^|\s)-o\s+("[^"]+"|\S+)', entry["command"])
            if not match:
                raise ValueError(f"compile command has no object output: {entry['file']}")
            output = match[1].strip('"')
        obj = Path(output)
        if not obj.is_absolute():
            obj = Path(entry["directory"]) / obj
        commands[os.path.normcase(str(obj.resolve()))] = entry
    return commands


def compile_target_objects(commands, targets, source_root, include_host=False):
    """Audit entry objects too, including generated address/alias-table code."""
    objects = []
    root = Path(source_root).resolve()
    for obj, entry in commands.items():
        source = Path(entry["file"])
        if not source.is_absolute():
            source = Path(entry["directory"]) / source
        source = source.resolve()
        if not include_host and (source.is_relative_to(root / "src/platform") or source.is_relative_to(root / "third_party")):
            continue
        if any(os.path.normcase(f"/CMakeFiles/{target}.dir/").replace("\\", "/") in os.path.normcase(str(obj)).replace("\\", "/") for target in targets):
            objects.append(Path(obj))
    return objects


def audit(objects, definitions, nm, objdump, coff32, host, known, renames, allow,
          adapters=None, retail_names=None, commands=None):
    defined, references = object_symbols(objects, nm, coff32)
    definition_references = dict(references)
    if definitions:
        more, more_references = object_symbols(definitions, nm, coff32)
        defined.update(more)
        definition_references.update(more_references)
    # Generated unresolved-address traps are definitions, but not CRT
    # implementations. Final closure must find a real body, never such a trap.
    trap_definitions = set()
    for obj, names in definition_references.items():
        if "p4pc_trap_args" in names:
            trap_definitions.update(relocation_callers(
                obj, {"p4pc_trap_args"}, objdump, coff32)["p4pc_trap_args"])
    generated_definitions = set()
    for obj in dict.fromkeys(map(str, [*objects, *definitions])):
        if (re.search(r"(?:^|/)p4pc_tables(?:64w?)?\.c\.(?:o|obj)$", obj.replace("\\", "/")) or
                report_source(obj, commands).startswith("gen/p4pc_tables")):
            names, _ = object_symbols([obj], nm, coff32)
            generated_definitions.update(names)
    implemented = defined - trap_definitions - generated_definitions
    # Include both native-register adapters and their raw retail entry points;
    # replacing one map by the other would silently omit the adapters.
    bindings = list(renames.items())
    bindings.extend({
        "printf": "func_00440b68", "scePrintf": "func_004244c8",
        "vprintf": "func_00446e88", "strncpy": "func_00442de8",
        "memcpy": "func_0043f810", "memmove": "func_0043f8c0", "memset": "func_0043f9c8",
        "strtoll": "func_00444210", "strtoull": "func_00444458",
        "_strtoll_r": "func_00443fd0", "_strtoull_r": "func_00444240",
        "log": "func_004476b0", "log10": "func_00447b88",
        "acos": "func_004470f0", "pow": "func_00447d00",
        "sqrt": "func_00448a00",
        "calloc": "p4pc_heap_calloc", "realloc": "p4pc_heap_realloc",
        "malloc": "p4pc_heap_malloc", "free": "p4pc_heap_free",
        "snprintf": "p4pc_ee_snprintf",
    }.items())
    targets = {}
    for name, target in bindings:
        targets.setdefault(target, set()).add(name)
    for name in ("sprintf", "snprintf", "printf", "fprintf"):
        targets.setdefault("p4pf_" + name, set()).add(name)
    aliases = {}
    alias_addresses = {}
    for name, address in (retail_names or {}).items():
        if 0x0043c6a0 <= address <= 0x0044f510:
            aliases.setdefault(address, set()).add(name)
    for address, names in aliases.items():
        target = f"func_{address:08x}"
        labels = {name for name in names if not name.startswith(("func_", "FUN_"))}
        if target not in targets:
            targets[target] = labels or {f"retail-runtime@{address:08x}"}
        # Record non-host retail compiler aliases before the generated thunk
        # exists. Standard CRT metadata still cannot authorize a host call.
        for name in names.difference(renames):
            if (name not in C_FUNCTIONS and name not in host) or name in implemented:
                targets.setdefault(name, set()).update(labels or {f"retail-runtime@{address:08x}"})
                alias_addresses[name] = address
        alias_addresses[target] = address
    rows, failures = [], []
    for obj, names in sorted(references.items()):
        source = native_source(obj)
        adapter = (adapters or {}).get(source)
        if adapter:
            entry = (commands or {}).get(os.path.normcase(str(Path(obj).resolve())))
            if entry is None:
                raise ValueError(f"{obj}: adapter flow check needs its actual compile command")
            host_names = C_FUNCTIONS.difference(allow) | {
                name for name in adapter["calls"]
                if name not in allow and not name.startswith(("SDL_", "av_", "avcodec_", "av_parser_", "sws_", "pthread_"))}
            failures.extend(crt_flow.check(entry, host_names))
        audited = {}
        for name in sorted(names):
            is_crt = (name in C_FUNCTIONS or name in host or name in renames or
                      name.startswith(("__mingw_", "__isoc99_", "__isoc23_", "__stdio_")))
            unknown = (name not in defined and name not in known and
                       not name.startswith(("p4pc_", "p4_", "p4pf_")) and
                       not gen_port.DATA_NAME_RE.fullmatch(name) and not gen_port.GP_NAME_RE.fullmatch(name) and
                       gen_port.FUNC_NAME_RE.fullmatch(name) is None)
            if name in targets:
                address = alias_addresses.get(name)
                candidates = aliases.get(address, set())
                if address is not None:
                    candidates = candidates | {f"func_{address:08x}"}
                available = bool(implemented.intersection(candidates | {name}))
                if definitions and not available:
                    audited[name] = (",".join(sorted(targets[name])), "FORBIDDEN", "retail CRT binding has no non-trap implementation in supplied game/port objects")
                else:
                    reason = "retail body/native-register adapter; see CRT contract and differential tests"
                    if name in {"func_0043ece8", "func_0043ed08", "func_0043c710", "func_00441558",
                                "func_0044f510", "p4pc_heap_malloc", "p4pc_heap_free",
                                "p4pc_heap_calloc", "p4pc_heap_realloc", "chkmemAllocAligned",
                                "chkmemAllocSiteTag", "chkmemGetBytes", "chkmemGetClasses", "chkmemSetLock"}:
                        reason = "retail EE newlib guest-word heap / matched checked RW boundary; no process CRT heap; see allocator differential and lifetime witness"
                    elif name in {"func_0043f810", "func_0043f8c0", "func_0043f9c8", "func_00442948",
                                  "func_00442de8", "func_00442a80", "func_00443010"}:
                        reason = "EE-width byte/string entry over reviewed host semantics on valid bounded inputs; see strict host allowlist"
                    audited[name] = (",".join(sorted(targets[name])), "retail binding", reason)
            elif is_crt or unknown:
                # EE-width aliases also have reviewed host byte equivalents:
                # the compiler may emit memcpy/memset for aggregate operations.
                # Their explicit allowlist reason remains authoritative; a
                # host function-pointer ABI still needs the indirect-call audit.
                if name in allow:
                    disposition, reason = "reviewed host", allow[name]
                elif name in renames:
                    disposition, reason = "FORBIDDEN", f"bypasses rename to {renames[name]}"
                else:
                    disposition, reason = "FORBIDDEN", "host/unknown external absent from reviewed allowlist"
                audited[name] = (name, disposition, reason)
        callers = relocation_callers(obj, audited, objdump, coff32) if audited else {}
        for target, (name, disposition, reason) in audited.items():
            for caller in sorted(callers[target]):
                row_disposition, row_reason = disposition, reason
                if disposition == "reviewed host" and target in C_FUNCTIONS and adapter and caller not in adapter["calls"].get(target, []):
                    row_disposition, row_reason = "FORBIDDEN", "new host symbol/caller pair in adapter requires review"
                if disposition == "FORBIDDEN" and target not in targets and adapter and caller in adapter["calls"].get(target, []):
                    row_disposition, row_reason = "reviewed host adapter", adapter["reason"]
                if row_disposition == "FORBIDDEN":
                    failures.append(f"{obj}: {caller} -> {target}: {row_reason}")
                rows.append(dict(symbol=name, binding=target, disposition=row_disposition,
                                 object=obj, source=report_source(obj, commands), caller=caller, reason=row_reason))
    return rows, failures


def host_exports(paths, nm, coff32):
    names = set()
    for path in paths:
        dynamic = not str(path).endswith(".a")
        # Mach-O dylibs list their exports without -D; libSystem's .tbd stub carries its re-exported libraries inline
        extra = (["--add-inlinedinfo"] if str(path).endswith(".tbd") else []) if MACHO else (["-D"] if dynamic else [])
        out = subprocess.run([nm, "-P", "-g", "--defined-only", *extra, str(path)],
                             text=True, capture_output=True, check=True).stdout
        for line in out.splitlines():
            parts = line.split()
            if len(parts) >= 2 and len(parts[1]) == 1:
                names.add(normalize(parts[0], coff32))
    return names


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--objects-file", type=Path)
    parser.add_argument("--definitions-file", type=Path)
    parser.add_argument("--project-lib", action="append", type=Path, default=[],
                        help="compiled project archives whose definitions satisfy game references")
    parser.add_argument("--decomp", required=True, type=Path)
    parser.add_argument("--build", required=True, type=Path, help="generated mirror directory")
    parser.add_argument("--nm", default="llvm-nm")
    parser.add_argument("--objdump", default="llvm-objdump")
    parser.add_argument("--coff32", action="store_true")
    parser.add_argument("--macho", action="store_true", help="Mach-O objects and libraries (macOS)")
    parser.add_argument("--host-lib", action="append", default=[])
    parser.add_argument("--host-name", action="append", default=[])
    parser.add_argument("--pic", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--report", required=True, type=Path)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--adapter-policy", type=Path, default=ROOT / "config/crt_host_adapters.json")
    parser.add_argument("--compile-commands", type=Path)
    parser.add_argument("--compile-target", action="append", default=[])
    args = parser.parse_args(argv)
    global MACHO
    MACHO = args.macho
    read_objects = lambda path: [Path(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]
    objects = read_objects(args.objects_file) if args.objects_file else []
    definitions = read_objects(args.definitions_file) if args.definitions_file else []
    commands = compilation_database(args.compile_commands) if args.compile_commands else {}
    if args.compile_target:
        if not args.compile_commands:
            parser.error("--compile-target requires --compile-commands")
        objects.extend(compile_target_objects(commands, args.compile_target, args.source_root))
        definitions.extend(compile_target_objects(commands, args.compile_target, args.source_root, include_host=True))
    if not objects:
        parser.error("game object list is empty")
    retail_names = gen_port.retail_function_names(args.decomp, args.build)
    known = set(gen_port.read_symbols(args.decomp)) | set(gen_port.read_port_symbols()) | set(retail_names)
    # Existing addressless SDK seams are project names, not host-CRT permission.
    # The catalog/host-export checks still apply even to these known names.
    known.update(line.split("#", 1)[0].strip()
                 for line in gen_port.read_list(ROOT / "config/call_traps.txt"))
    # Compile-only mirrors precede native object compilation, so their exact
    # CMake-listed definition ABIs supply native names. Final/native audits
    # already prove those names from supplied nm-defined objects; they must
    # not depend on optional 64-bit ABI files in an i386-only/cold build.
    native_manifest = args.build / "crt-native-abi-files.txt"
    if not definitions and native_manifest.is_file():
        for path in native_manifest.read_text(encoding="utf-8").splitlines():
            if path:
                known.update(json.loads(Path(path).read_text(encoding="utf-8")))
    project_definitions, _ = object_symbols(args.project_lib, args.nm, args.coff32)
    known.update(project_definitions)
    # Project archives can also occur in linker host-export inputs. Actual
    # project definitions are not host calls, but cannot authorize CRT bypass.
    renames = gen_port.read_libc_renames()
    host = host_exports(args.host_lib, args.nm, args.coff32) | set(args.host_name)
    host.difference_update(project_definitions - C_FUNCTIONS - set(renames))
    rows, failures = audit(objects, definitions,
                           args.nm, args.objdump, args.coff32, host, known,
                           renames, gen_port.read_host_allow(),
                           json.loads(args.adapter_policy.read_text(encoding="utf-8"))["sources"],
                           retail_names, commands)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps({"objects": len(objects), "references": rows, "failures": failures}, indent=2) + "\n", encoding="utf-8")
    table = io.StringIO(newline="")
    writer = csv.DictWriter(table, fieldnames=["symbol", "binding", "disposition", "object", "source", "caller", "reason"], delimiter="\t")
    writer.writeheader()
    writer.writerows(rows)
    args.report.with_suffix(".tsv").write_text(table.getvalue(), encoding="utf-8")
    if failures:
        print("CRT guard rejected game references:\n  " + "\n  ".join(failures), file=sys.stderr)
        return 1
    print(f"CRT guard: {len(objects)} game objects, {len(rows)} caller/binding rows; inventory {args.report}")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (ValueError, OSError, subprocess.CalledProcessError) as error:
        print(f"CRT guard: {error}", file=sys.stderr)
        sys.exit(1)
