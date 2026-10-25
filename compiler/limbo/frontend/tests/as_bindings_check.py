"""Check generated .m scaffolds with the actual Coco/R AS1, AS2 and AS3 parsers."""

from pathlib import Path
import subprocess
import sys
import tempfile


# AS syntax acceptance does not claim executable lowering or a ready Dis/SWF bridge.
def main():
    generator, *parsers = sys.argv[1:]
    with tempfile.TemporaryDirectory(prefix="limbo-as-stubs-") as directory:
        root = Path(directory)
        inp = root / "native.m"
        inp.write_text(
            'Native:module{add:fn(a,b:int # quote " and separator \u2028\n):int; 求值:fn(s:string);};'
        )
        for language, parser in zip(("as1", "as2", "as3"), parsers):
            for direction in ("module", "proxy"):
                output = root / (language + "-" + direction + ".as")
                subprocess.run(
                    [generator, "--format", language + "-" + direction, str(inp), str(output)],
                    check=True,
                )
                result = subprocess.run([parser, str(output)], capture_output=True, timeout=15)
                if not (result.returncode == 0):
                    raise AssertionError(
                        (language, direction, result.stdout, result.stderr, output.read_text())
                    )
    print("PASS .m scaffolds accepted by all three real Coco/R AS frontends")


if __name__ == "__main__":
    main()
