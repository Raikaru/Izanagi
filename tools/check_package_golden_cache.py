#!/usr/bin/env python3
"""Refuse user-derived Golden cache entries in a package staging tree or archive."""
import os
from pathlib import Path
import sys
import zipfile


def forbidden(parts):
    return any(part.casefold() == "golden-cache" for part in parts)

def walk_error(error):
    raise error


def check(path):
    path = Path(path)
    if forbidden(path.parts) or forbidden(path.resolve(strict=True).parts):
        raise ValueError(str(path))
    if path.is_dir():
        root = path.resolve(strict=True)
        for directory, names, files in os.walk(path, followlinks=False, onerror=walk_error):
            for name in names + files:
                candidate = Path(directory) / name
                if forbidden(candidate.relative_to(path).parts):
                    raise ValueError(str(candidate))
                if candidate.is_symlink() and not candidate.resolve(strict=True).is_relative_to(root):
                    raise ValueError(f"package link escapes staging tree: {candidate}")
    elif zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            for name in archive.namelist():
                if forbidden(name.replace("\\", "/").split("/")):
                    raise ValueError(name)
    else:
        raise ValueError(f"not a package tree or zip: {path}")


if __name__ == "__main__":
    try:
        for argument in sys.argv[1:]:
            check(argument)
        if len(sys.argv) < 2:
            raise ValueError("no package supplied")
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        sys.exit(f"Golden cache is not permitted in a package: {error}")
