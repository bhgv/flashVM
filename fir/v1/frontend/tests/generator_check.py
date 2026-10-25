"""A grammar error must fail generation even with old generated files present."""

from pathlib import Path
import subprocess
import sys
import tempfile

cmake, wrapper, coco, root = sys.argv[1:]
with tempfile.TemporaryDirectory() as folder:
    work = Path(folder)
    # CHR is deliberately illegal in a TOKENS expression in this Coco/R dialect.
    (work / "broken.atg").write_text(
        'COMPILER Broken\nCHARACTERS\nletter="a".\nTOKENS\nx=CHR(10).\nPRODUCTIONS\nBroken=EOF.\nEND Broken.\n'
    )
    outputs = [work / ("broken" + suffix) for suffix in ("p.c", "s.c", "p.h", "s.h", "c.h", "e.h")]
    for path in outputs:
        path.write_text("stale generated data")
    run = subprocess.run(
        [
            cmake,
            "-DCOCOR_EXECUTABLE=" + coco,
            "-DCOCOR_ROOT=" + root,
            "-DGRAMMAR=broken",
            "-P",
            wrapper,
        ],
        cwd=work,
        capture_output=True,
        timeout=10,
    )
    if not (run.returncode != 0):
        raise AssertionError((run.stdout, run.stderr))
    if not (b"Coco/R" in run.stderr):
        raise AssertionError(run.stderr)
    if not (
        all((not path.exists() or path.read_text() != "stale generated data" for path in outputs))
    ):
        raise AssertionError("condition check failed: fir/v1/frontend/tests/generator_check.py:33")
print("invalid grammar rejected; stale files cannot be accepted")
