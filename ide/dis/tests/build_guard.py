"""Reject stale or malformed product signatures without claiming bytecode verification."""

from pathlib import Path
import subprocess
import sys
import tempfile


# Compile fixtures with arbitrary trailing bytes: only product signatures are tested.
def compiler_body(module, symbols, status=0):
    statements = []
    if module is not None:
        statements.append("out.write_bytes(" + repr(module) + ")")
    if symbols is not None:
        statements.append("sym.write_bytes(" + repr(symbols) + ")")
    statements.append("sys.exit(" + str(status) + ")")
    return "\n".join(statements)


# Supply malformed physical headers that text-line extraction used to normalize away.
def scenarios():
    magic = bytes.fromhex("c00c8030")
    header = b"limbo .sbl 2.1\nIdecore\n"
    cases = [
        ("failure", magic, header, 7, False),
        ("missing", None, None, 0, False),
        ("only-module", magic, None, 0, False),
        ("only-symbols", None, header, 0, False),
        ("empty-module", b"", header, 0, False),
        ("empty-symbols", magic, b"", 0, False),
        ("garbage-module", b"dis", header, 0, False),
        ("signed-module", bytes.fromhex("c00e1722"), header, 0, False),
        ("wrong-debug", magic, b"limbo .sbl 2.1\nOther\n", 0, False),
        ("leading-blank", magic, b"\n" + header, 0, False),
        ("embedded-blank", magic, header.replace(b"\n", b"\n\n", 1), 0, False),
        ("leading-nul", magic, b"\0" + header, 0, False),
        ("embedded-nul", magic, header.replace(b"\n", b"\0\n", 1), 0, False),
        ("leading-space", magic, b" " + header, 0, False),
        ("module-suffix", magic, header.replace(b"Idecore", b"IdecoreExtra"), 0, False),
        ("bare-cr", magic, header.replace(b"\n", b"\r"), 0, False),
    ]
    # Every truncated signature/header must fail, even after a zero compiler exit.
    for length in range(1, len(magic)):
        cases.append(("magic-prefix-" + str(length), magic[:length], header, 0, False))
    for length in range(1, len(header)):
        cases.append(("symbol-prefix-" + str(length), magic, header[:length], 0, False))
    for first in (b"\n", b"\r\n"):
        for second in (b"\n", b"\r\n"):
            symbols = b"limbo .sbl 2.1" + first + b"Idecore" + second
            cases.append(("line-endings-" + str(len(cases)), magic, symbols, 0, True))
    cases.append(("opaque-tail", magic + b"\0\xff", header + b"opaque\n", 0, True))
    return cases


# Run each contract in isolation with stale outputs; failures must remove both products.
def main():
    cmake, guard = sys.argv[1:]
    cases = scenarios()
    with tempfile.TemporaryDirectory(prefix="ide-dis-guard-") as directory:
        root = Path(directory)
        for name, module, debug, status, accepted in cases:
            compiler = root / "compiler"
            compiler.write_text(
                "#!" + sys.executable + "\nimport sys\nfrom pathlib import Path\n"
                "out = Path(sys.argv[sys.argv.index('-o') + 1])\n"
                "sym = out.with_suffix('.sbl')\n" + compiler_body(module, debug, status) + "\n"
            )
            compiler.chmod(0o700)
            output, symbols = root / "idecore.dis", root / "idecore.sbl"
            output.write_bytes(b"stale module")
            symbols.write_bytes(b"limbo .sbl 2.1\nIdecore\n")
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
            if accepted:
                if not (result.returncode == 0):
                    raise AssertionError((name, result.stderr))
                if not (output.read_bytes() == module):
                    raise AssertionError(name)
                if not (symbols.read_bytes() == debug):
                    raise AssertionError(name)
            else:
                if not (result.returncode != 0):
                    raise AssertionError(name)
                if not (not output.exists() and (not symbols.exists())):
                    raise AssertionError(name)
    print(
        str(len(cases)) + " compiler product-admission contracts passed; no bytecode validity claim"
    )


if __name__ == "__main__":
    main()
