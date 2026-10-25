#!/usr/bin/env python3
"""Compatibility spelling for the canonical build-menu.py entry point."""

from pathlib import Path
import runpy


if __name__ == "__main__":
    runpy.run_path(str(Path(__file__).resolve().with_name("build-menu.py")), run_name="__main__")
