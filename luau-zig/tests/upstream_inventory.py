#!/usr/bin/env python3
"""Inventory the preserved upstream doctests without discarding variants."""
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
UPSTREAM = HERE / 'upstream-0.744'


def verify_sources():
    manifests = [json.loads((UPSTREAM / name).read_text(encoding='utf-8'))
                 for name in ('manifest.json', 'target-manifest.json')]
    manifest = manifests[0]
    for entry in manifests:
        if entry['commit'] != '32c28089305f488f901aaf1dbf3fd1cb65fa3d11':
            raise ValueError('unexpected upstream commit')
        for path, expected in entry['git_blobs'].items():
            data = (UPSTREAM / path).read_bytes()
            actual = hashlib.sha1(b'blob ' + str(len(data)).encode() + b'\0' + data).hexdigest()
            if actual != expected:
                raise ValueError(f'upstream blob mismatch: {path}: {actual} != {expected}')
    actual = {p.relative_to(UPSTREAM).as_posix() for p in (UPSTREAM / 'tests/conformance').glob('*.luau')}
    expected = {p for p in manifest['git_blobs'] if p.startswith('tests/conformance/')}
    if actual != expected:
        raise ValueError(f'fixture inventory differs: missing={sorted(expected-actual)}, extra={sorted(actual-expected)}')
    return manifest


def closing_brace(text, start):
    """Find the C++ block end, ignoring comments, quoted strings and raw strings."""
    if text[start] != '{':
        raise ValueError('expected opening brace')
    depth, i = 0, start
    while i < len(text):
        if text.startswith('//', i):
            end = text.find('\n', i + 2)
            i = len(text) if end < 0 else end
        elif text.startswith('/*', i):
            end = text.find('*/', i + 2)
            if end < 0: raise ValueError('unterminated C++ comment')
            i = end + 2
        elif text.startswith('R"', i):
            op = text.find('(', i + 2)
            if op < 0: raise ValueError('invalid C++ raw string')
            marker = ')' + text[i + 2:op] + '"'
            end = text.find(marker, op + 1)
            if end < 0: raise ValueError('unterminated C++ raw string')
            i = end + len(marker)
        elif (text[i] in '0123456789' and
              (i == 0 or not (text[i - 1].isalnum() or text[i - 1] == '_'))):
            # C++14 digit separators (e.g. 1'000, 0xff'ff) are part of a
            # preprocessing number, not the start of a character literal.
            # Only start at a token boundary so a prefix such as u8R"..." is
            # left intact and its raw string still gets recognized.
            i += 1
            while i < len(text):
                ch = text[i]
                if ch.isascii() and (ch.isalnum() or ch in '_.'):
                    i += 1
                elif ch == "'" and i + 1 < len(text) and text[i + 1].isascii() and (text[i + 1].isalnum() or text[i + 1] == '_'):
                    i += 1
                elif ch in '+-' and text[i - 1] in 'eEpP':
                    i += 1
                else:
                    break
        elif text[i] in "\"'":
            quote = text[i]
            i += 1
            while i < len(text):
                if text[i] == '\\': i += 2
                elif text[i] == quote:
                    i += 1
                    break
                else: i += 1
            else: raise ValueError('unterminated C++ literal')
        else:
            if text[i] == '{': depth += 1
            elif text[i] == '}':
                depth -= 1
                if depth == 0: return i + 1
            i += 1
    raise ValueError('unterminated C++ block')


def inventory():
    verify_sources()
    cases = []
    sources = ('Conformance.test.cpp', 'DirectFieldAccess.test.cpp', 'FeedbackVector.test.cpp',
               'IrLowering.test.cpp', 'SharedCodeAllocator.test.cpp')
    pattern = re.compile(r'TEST_CASE(?:_FIXTURE\(\s*[^,]+,\s*|\(\s*)"([^"\n]+)"\s*\)\s*\{')
    for source_name in sources:
        text = (UPSTREAM / 'tests' / source_name).read_text(encoding='utf-8')
        suite_match = re.search(r'TEST_SUITE_BEGIN\("([^"]+)"\)', text)
        if not suite_match:
            raise ValueError(f'no suite in {source_name}')
        suite = suite_match.group(1)
        for match in pattern.finditer(text):
            opening = match.end() - 1
            line = text.count('\n', 0, match.start()) + 1
            try:
                end = closing_brace(text, opening)
            except ValueError as exc:
                raise ValueError(f'{source_name}:{line}: {match.group(1)}: {exc}') from exc
            body = text[opening:end]
            body_line = text.count('\n', 0, opening) + 1
            calls = [{'file': m.group(1), 'line': body_line + body.count('\n', 0, m.start())}
                     for m in re.finditer(r'runConformance\(\s*"([^"\n]+\.luau)"', body)]
            subcases = []
            parents = []
            for sub in re.finditer(r'SUBCASE\("([^"\n]*)"\)\s*\{', body):
                while parents and sub.start() >= parents[-1][0]:
                    parents.pop()
                subline = body_line + body.count('\n', 0, sub.start())
                path = [parent[1] for parent in parents] + [sub.group(1)]
                try:
                    subend = closing_brace(body, sub.end() - 1)
                except ValueError as exc:
                    raise ValueError(f'{source_name}:{subline}: {match.group(1)}/{sub.group(1)}: {exc}') from exc
                parents.append((subend, sub.group(1)))
                subcases.append({'name': sub.group(1), 'line': subline, 'path': path})
            flags = sorted(set(re.findall(r'\b(?:D?FFlag|FInt)::([A-Za-z_0-9]+)', body)))
            cases.append({'name': match.group(1), 'line': line, 'file': source_name, 'suite': suite,
                          'source': text[match.start():end], 'body': body, 'calls': calls,
                          'subcases': subcases, 'flags': flags,
                          'simple': bool(re.fullmatch(r'\{\s*runConformance\("[^"\n]+\.luau"\);\s*\}', body))})
    if not cases:
        raise ValueError('no upstream test cases found')
    return cases


if __name__ == '__main__':
    print(json.dumps([{k: v for k, v in case.items() if k not in ('body', 'source')} for case in inventory()], indent=2))
