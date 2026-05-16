#!/usr/bin/env python3
import os
import sys


def relink(root: str) -> int:
    changed = 0
    for dirpath, dirnames, filenames in os.walk(root):
        entries = list(dirnames) + list(filenames)
        for name in entries:
            path = os.path.join(dirpath, name)
            if not os.path.islink(path):
                continue
            target = os.readlink(path)
            if not target.startswith("/"):
                continue
            absolute_target = os.path.normpath(os.path.join(root, target.lstrip("/")))
            if not os.path.exists(absolute_target):
                continue
            relative_target = os.path.relpath(absolute_target, os.path.dirname(path))
            os.unlink(path)
            os.symlink(relative_target, path)
            changed += 1
    return changed


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} SYSROOT", file=sys.stderr)
        sys.exit(2)
    print(f"rewrote {relink(sys.argv[1])} absolute sysroot symlinks")
