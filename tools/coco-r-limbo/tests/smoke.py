"""Exercise the bundled generator's valid generation and failed grammar exit."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile


# Private generation directories keep upstream/parser sources immutable.
def main():
    generator, frames = map(lambda p: str(Path(p).resolve()), sys.argv[1:])
    grammar = (
        'COMPILER Probe\nCHARACTERS digit="0123456789".\n'
        "TOKENS number=digit {digit}.\nIGNORE CHR(9)+CHR(10)+CHR(13)\n"
        "PRODUCTIONS Probe=number EOF.\nEND Probe.\n"
    )
    with tempfile.TemporaryDirectory(prefix="cocor-smoke-") as directory:
        root = Path(directory)
        inp = root / "probe.atg"
        inp.write_text(grammar)
        env = dict(os.environ, CRFRAMES=frames)
        result = subprocess.run(
            [generator, str(inp)], cwd=root, env=env, capture_output=True, timeout=20
        )
        if not (result.returncode == 0):
            raise AssertionError((result.stdout, result.stderr))
        if not (
            all(
                (
                    (root / name).is_file()
                    for name in ["probep.c", "probes.c", "probep.h", "probes.h"]
                )
            )
        ):
            raise AssertionError("condition check failed: tools/coco-r-limbo/tests/smoke.py:27")
        inp.write_text("COMPILER Bad PRODUCTIONS Bad=missing. END Bad.\n")
        result = subprocess.run(
            [generator, str(inp)], cwd=root, env=env, capture_output=True, timeout=20
        )
        if not (result.returncode != 0):
            raise AssertionError((result.stdout, result.stderr))
    print("PASS bundled Coco/R generation and rejection")


if __name__ == "__main__":
    main()
