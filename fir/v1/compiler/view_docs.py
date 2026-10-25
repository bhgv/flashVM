#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Browse this FIR component with the workspace Python 2.7.9/3 viewer."""
from __future__ import print_function
import os
import sys


def main():
    """Find the workspace engine and preserve repository-relative guide links."""
    local = os.path.dirname(os.path.abspath(__file__))
    root = local
    while not os.path.isdir(os.path.join(root, "flashVM", "doku_viewer")):
        parent = os.path.dirname(root)
        if parent == root:
            raise SystemExit("Workspace viewer missing; read the Markdown guides directly.")
        root = parent
    sys.path.insert(0, os.path.join(root, "flashVM"))
    try:
        from doku_viewer.server import main as browse

        browse(root=root, component=os.path.relpath(local, root))
    finally:
        sys.path.pop(0)


if __name__ == "__main__":
    main()
