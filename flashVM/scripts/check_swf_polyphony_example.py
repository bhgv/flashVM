#!/usr/bin/env python3
"""Validate actual C89 polyphony tutorial output, native files and no-side-effect usage.
Usage: python3 scripts/check_swf_polyphony_example.py BUILD/flashvm_examples_multimedia_swf_polyphony_main
"""
import pathlib
import struct
import subprocess
import sys
import tempfile


def main():
    executable = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory() as directory:
        root = pathlib.Path(directory)
        subprocess.run([executable], cwd=directory, check=True, stdout=subprocess.PIPE)
        if not (not list(root.iterdir())):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_polyphony_example.py:17"
            )
        paths = [str(root / name) for name in ("A.pcm", "B.pcm", "OTHER.pcm")]
        invalid = subprocess.run(
            [executable, paths[0], paths[0], paths[2]],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if not (invalid.returncode and (not list(root.iterdir()))):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_polyphony_example.py:24"
            )
        result = subprocess.run(
            [executable] + paths, check=True, stdout=subprocess.PIPE, universal_newlines=True
        )
        if not ("A=1 writes, B=1 writes, unused=0 writes" in result.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_polyphony_example.py:28"
            )
        if not ("A=1 stops, B=1 stops, other active=1" in result.stdout):
            raise AssertionError(
                "condition check failed: flashVM/scripts/check_swf_polyphony_example.py:29"
            )
        for path in paths:
            if not (pathlib.Path(path).read_bytes() == struct.pack("=hh", -32768, 32512)):
                raise AssertionError(
                    "condition check failed: flashVM/scripts/check_swf_polyphony_example.py:31"
                )
    print(
        "PASS real polyphony tutorial: overlap, no-multiple, stop-by-ID, independent native files"
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
