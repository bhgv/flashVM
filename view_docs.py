#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""Delegate documentation browsing to FlashVM on Python 2.7.9 or Python 3."""
from __future__ import print_function
import os
import sys


def main():
    """Launch the FlashVM viewer with unchanged arguments and working directory."""
    component = os.path.join(os.path.dirname(os.path.abspath(__file__)), "flashVM")
    # The delegated launcher imports its own documentation engine from this folder.
    sys.path.insert(0, component)
    try:
        from doku_viewer.server import main as browse

        browse(root=os.path.dirname(component), component="flashVM")
    finally:
        # Leave the caller's import search path unchanged after the viewer stops.
        sys.path.pop(0)


if __name__ == "__main__":
    main()
