#!/usr/bin/env python3
"""Check long frame/source paths and fail-closed limits on a real Coco/R executable."""
import argparse
from pathlib import Path
import os
import shutil
import subprocess
import tempfile


# Generate a real grammar under long paths, then reject impossible directory/option extents.
# CLI names executable/frames explicitly; returns through process status and concrete diagnostics.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("generator", type=Path)
    parser.add_argument("frames", type=Path)
    args = parser.parse_args()
    grammar = ('COMPILER Probe\nCHARACTERS digit="0123456789".\n'
               'TOKENS number=digit {digit}.\nIGNORE CHR(9)+CHR(10)+CHR(13)\n'
               'PRODUCTIONS Probe=number EOF.\nEND Probe.\n')
    with tempfile.TemporaryDirectory(prefix="cocor frame paths ") as directory:
        root = Path(directory)
        frames = root / ("long frame directory " * 8) / "frames"
        frames.parent.mkdir()
        shutil.copytree(args.frames.resolve(), frames)
        source = root / ("long source directory " * 8)
        source.mkdir()
        inp = source / "probe.atg"
        inp.write_text(grammar)
        env = dict(os.environ, CRFRAMES=str(frames))
        for extra in ([], ["-DCRFRAMES=" + str(frames)]):
            result = subprocess.run([str(args.generator.resolve()), *extra, str(inp)],
                                    cwd=source, env=env, capture_output=True, timeout=20)
            if result.returncode or not (source / "probep.c").is_file() or not (source / "probes.c").is_file():
                raise RuntimeError("long-path generation failed: " + repr(result.stdout + result.stderr))
            for generated in source.glob("*.c"):
                generated.unlink()
        cases = [([], dict(env, CRFRAMES="x" * 5000)),
                 (["-DCRFRAMES=" + "x" * 5000], env),
                 (["-DCRHEXT=" + "x" * 100], env),
                 (["-D" + "n" * 250 + "=v"], env),
                 (["s" * 5000], env)]
        for extra, environment in cases:
            result = subprocess.run([str(args.generator.resolve()), *extra, str(inp)],
                                    cwd=source, env=environment, capture_output=True, timeout=20)
            if result.returncode <= 0 or b"Coco/R:" not in result.stderr:
                raise RuntimeError("oversized path/option did not fail cleanly: " + repr(result.stderr))
    print("PASS long environment/CLI frame and source paths; oversized inputs fail before generation")


if __name__ == "__main__":
    main()
