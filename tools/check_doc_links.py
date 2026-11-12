#!/usr/bin/env python3
"""Check local Markdown links in docs/, README.md and CONTRIBUTING.md (no network)."""
import argparse
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit


FENCE = re.compile(r"^\s*(`{3,}|~{3,})")
LINK = re.compile(r"!?\[[^\]\n]*\]\(\s*(<[^>\n]+>|(?:[^()\s]|\([^()]*\))+)(?:\s+[\"'][^\n]*[\"'])?\s*\)")
REFERENCE = re.compile(r"^\s{0,3}\[[^\]]+\]:\s*(<[^>]+>|\S+)", re.M)


def prose(text):
    """Retain line numbers while excluding fenced/inline code examples."""
    lines, fence = [], None
    for line in text.splitlines():
        match = FENCE.match(line)
        if match:
            marker = match[1]
            if fence is None:
                fence = marker
            elif marker[0] == fence[0] and len(marker) >= len(fence):
                fence = None
            lines.append("")
        elif fence:
            lines.append("")
        else:
            # Mask code instead of removing it: link syntax cannot appear inside it.
            lines.append(re.sub(r"(`+).*?\1", lambda m: " " * len(m[0]), line))
    return "\n".join(lines)


def anchors(text):
    counts, found = {}, set()
    # Headings may contain inline code, which contributes its text to GitHub's slug.
    fence = None
    for line in text.splitlines():
        match = FENCE.match(line)
        if match:
            marker = match[1]
            if fence is None:
                fence = marker
            elif marker[0] == fence[0] and len(marker) >= len(fence):
                fence = None
            continue
        if fence:
            continue
        heading = re.match(r"^ {0,3}#{1,6}\s+(.+?)(?:\s+#+)?\s*$", line)
        if heading:
            title = re.sub(r"<[^>]*>", "", heading[1]).lower()
            title = re.sub(r"[^\w\-\s]", "", title, flags=re.UNICODE)
            slug = re.sub(r"\s", "-", title)
            count = counts.get(slug, 0)
            counts[slug] = count + 1
            found.add(f"{slug}-{count}" if count else slug)
        for explicit in re.finditer(r"<(?:a|[a-z][a-z0-9]*)\b[^>]*\b(?:id|name)=[\"']([^\"']+)[\"']", line, re.I):
            found.add(explicit[1])
    return found


def check(root):
    sources = sorted((root / "docs").rglob("*.md"))
    sources += [root / "README.md", root / "CONTRIBUTING.md"]
    cache, failures, checked = {}, [], 0
    for source in sources:
        text = prose(source.read_text(encoding="utf-8"))
        for match in list(LINK.finditer(text)) + list(REFERENCE.finditer(text)):
            target = match[1].strip("<>")
            url = urlsplit(target)
            if url.scheme or url.netloc:
                continue
            path = (root / unquote(url.path).lstrip("/") if url.path.startswith("/")
                    else source.parent / unquote(url.path)) if url.path else source
            path = path.resolve()
            checked += 1
            reason = None
            if not path.exists():
                reason = "missing path"
            elif url.fragment and path.is_file() and path.suffix.lower() == ".md":
                if path not in cache:
                    cache[path] = anchors(path.read_text(encoding="utf-8"))
                if unquote(url.fragment) not in cache[path]:
                    reason = "missing heading/anchor"
            if reason:
                line = text.count("\n", 0, match.start()) + 1
                failures.append(f"{source.relative_to(root).as_posix()}:{line}: {reason}: {target}")
    for failure in failures:
        print(failure, file=sys.stderr)
    print(f"doc links: {checked} local links in {len(sources)} files; {len(failures)} broken")
    return bool(failures)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    return check(args.root.resolve())


if __name__ == "__main__":
    sys.exit(main())
