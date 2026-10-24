"""A successful compiler exit cannot admit missing, stale or mismatched IDE build products."""

import os
from pathlib import Path
import subprocess
import sys
import tempfile


# Check each failed compiler contract in an isolated directory with deliberately stale outputs.
def main():
    cmake, guard = sys.argv[1:]
    scenarios = {
        "failure": "out.write_bytes(b'dis'); sym.write_text('limbo .sbl 2.1\\nIdecore\\n'); sys.exit(7)",
        "missing": "pass",
        "only-module": "out.write_bytes(b'dis')",
        "empty-module": "out.write_bytes(b''); sym.write_text('limbo .sbl 2.1\\nIdecore\\n')",
        "wrong-debug": "out.write_bytes(b'dis'); sym.write_text('limbo .sbl 2.1\\nOther\\n')",
        "success": "out.write_bytes(b'dis'); sym.write_text('limbo .sbl 2.1\\nIdecore\\n')",
    }
    with tempfile.TemporaryDirectory(prefix="ide-dis-guard-") as directory:
        root = Path(directory)
        for name, body in scenarios.items():
            compiler = root / "compiler"
            compiler.write_text(
                "#!" + sys.executable + "\nimport sys\nfrom pathlib import Path\n"
                "out=Path(sys.argv[sys.argv.index('-o')+1]); sym=out.with_suffix('.sbl')\n"
                + body
                + "\n"
            )
            compiler.chmod(0o700)
            output, symbols = root / "idecore.dis", root / "idecore.sbl"
            output.write_bytes(b"stale module")
            symbols.write_text("limbo .sbl 2.1\nIdecore\n")
            result = subprocess.run(
                [
                    cmake,
                    "-DLIMBO=" + str(compiler),
                    "-DSOURCE=" + str(root / "input.b"),
                    "-DINCLUDE_DIR=" + str(root),
                    "-DOUTPUT=" + str(output),
                    "-DSYMBOLS=" + str(symbols),
                    "-P",
                    guard,
                ],
                capture_output=True,
                timeout=15,
            )
            if name == "success":
                assert (
                    result.returncode == 0 and output.read_bytes() == b"dis"
                ), result.stderr
            else:
                assert result.returncode != 0, name
                assert not output.exists() and not symbols.exists(), name
    print("6 compiler product-admission contracts passed; no bytecode validity claim")


if __name__ == "__main__":
    main()
