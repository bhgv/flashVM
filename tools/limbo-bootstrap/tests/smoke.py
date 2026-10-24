"""Compile debug bytecode and legacy C scaffolds with the owned source bootstrap."""
from pathlib import Path
import subprocess
import sys
import tempfile


# This checks compiler products, not execution inside a complete Dis host.
def main():
    compiler = str(Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="limbo-bootstrap-") as directory:
        root = Path(directory)
        source, output = root / "answer.b", root / "answer.dis"
        source.write_text('implement Answer; Answer: module { value: fn(): int; };\nvalue(): int { return 42; }\n')
        result = subprocess.run([compiler, "-g", "-o", str(output), str(source)],
                                cwd=root, capture_output=True, timeout=30)
        assert result.returncode == 0, result.stderr
        assert output.read_bytes().startswith(bytes.fromhex("c00c8030"))
        symbols = output.with_suffix(".sbl").read_bytes()
        assert symbols.startswith(b"limbo .sbl 2.1\n"), symbols[:80]
        interface = root / "native.m"
        interface.write_text('Native: module { add: fn(a,b:int): int; };\n')
        result = subprocess.run([compiler, "-a", str(interface)], cwd=root,
                                capture_output=True, timeout=30)
        assert result.returncode == 0 and b"Native_add" in result.stdout, result
    print("PASS bundled bootstrap Dis/debug and legacy C scaffold products")


if __name__ == "__main__":
    main()
