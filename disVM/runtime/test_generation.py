"""Verify source-view preparation is idempotent and never compounds dispatch changes."""

from pathlib import Path
import subprocess
import sys
import tempfile


# Prepare twice using the real compiler and pinned source; require byte-identical output.
# Arguments are native compiler and libinterp source. Returns only after every file matches.
def main():
    compiler, source = sys.argv[1:]
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        command = [
            sys.executable,
            str(Path(__file__).with_name("prepare.py")),
            "--compiler",
            compiler,
            "--source",
            source,
            "--sys",
            str(Path(source).parent / "module/sys.m"),
            "--output",
            temporary,
        ]
        subprocess.run(command, check=True)
        first = {file.name: file.read_bytes() for file in root.iterdir() if file.is_file()}
        subprocess.run(command, check=True)
        second = {file.name: file.read_bytes() for file in root.iterdir() if file.is_file()}
        if first != second:
            raise RuntimeError("native source generation compounded a prior adaptation")
        if second["xec.c"].count(b"dvm_runtime_instructions++;") != 1:
            raise RuntimeError("instruction statistics must count once per actual dispatch")
    print("PASS byte-identical native source regeneration and one dispatch counter")


if __name__ == "__main__":
    main()
