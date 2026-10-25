"""Check actual compiler output for distinct signed 64-bit constant-pool entries."""

from pathlib import Path
import re
import subprocess
import sys
import tempfile


# argv[1] is the source-built compiler. Inspect textual Dis data rather than host formatting alone.
# Returns normally only if every requested constant survives without collision or truncation.
def main():
    compiler = str(Path(sys.argv[1]).resolve())
    values = (-2, -5, -65536, -65537, -(1 << 63), (1 << 63) - 1, 4294967294)
    with tempfile.TemporaryDirectory() as temporary:
        root = Path(temporary)
        source = root / "constants.b"
        assembly = root / "constants.s"
        source.write_text(
            "implement Constants;\n\n"
            "Constants: module\n{\n    values: fn(): (big, big, big, big, big, big, big);\n};\n\n\n"
            "# Return distinct values which used to collide when LP64 names were truncated.\n"
            "values(): (big, big, big, big, big, big, big)\n{\n    return ("
            + ", ".join("big " + str(value) for value in values)
            + ");\n}\n"
        )
        result = subprocess.run(
            [compiler, "-S", "-o", str(assembly), str(source)],
            capture_output=True,
            text=True,
            timeout=15,
        )
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        constants = set(map(int, re.findall(r"\blong\s+@mp\+\d+,(-?\d+)", assembly.read_text())))
        missing = set(values) - constants
        if missing:
            raise RuntimeError("big constants merged or truncated: " + str(sorted(missing)))
    print("PASS distinct signed big constants in actual Dis compiler output")


if __name__ == "__main__":
    main()
