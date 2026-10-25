#!/usr/bin/env python3
"""Build/install a real upstream Dis diagnostic executable, preserving its production code."""

import argparse
import os
from pathlib import Path
import runpy
import shlex
import subprocess
from unittest.mock import patch


# Reuse the retained diagnostic's exact code generator; retain its compiled binary.
# CLI supplies source driver, C compiler, output and word model. Native checks execute
# once before publication; this is a diagnostic fixture, not a general Limbo host.
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--driver", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cc", required=True)
    parser.add_argument("--wordbits", choices=("32", "64"), required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + ".building-" + str(os.getpid()))
    original_run = subprocess.run
    compiled = []

    def run(arguments, *positional, **keywords):
        """Redirect only generated diagnostic output; run its original checks before keeping it."""
        arguments = list(map(str, arguments))
        if "-o" in arguments:
            index = arguments.index("-o") + 1
            source = Path(next(value for value in arguments if value.endswith("runtime.c")))
            code = source.read_text()
            if code.count("int main(void)") != 1:
                raise RuntimeError("upstream diagnostic entry changed")
            code = code.replace("int main(void)", "int dvm_probe_suite(void)", 1)
            code += "\n/* Owned diagnostic CLI: reject arbitrary module arguments explicitly. */\n"
            code += "int main(int argc, char **argv)\n{\n"
            code += '    if (argc == 2 && strcmp(argv[1], "--help") == 0)\n    {\n'
            code += (
                '        puts("Native AMD64 Dis/JIT diagnostic; WORD'
                + args.wordbits
                + ' operand model.");\n'
            )
            code += '        puts("Runs interpreter/native-leaf checks; does not load arbitrary Limbo applications.");\n'
            code += "        return 0;\n    }\n"
            code += '    if (argc != 1)\n    {\n        fputs("No module arguments accepted; use --help\\n", stderr);\n        return 2;\n    }\n'
            code += "    return dvm_probe_suite();\n}\n"
            source.write_text(code)
            compiled.append(arguments[index])
            arguments[index] = str(temporary)
        elif compiled and arguments[0] == compiled[-1]:
            arguments[0] = str(temporary)
        return original_run(arguments, *positional, **keywords)

    scratch = output.parent / "probe-tmp"
    scratch.mkdir(exist_ok=True)
    environment = dict(
        os.environ,
        TMPDIR=str(scratch),
        TEMP=str(scratch),
        TMP=str(scratch),
        CC=shlex.quote(args.cc),
        CFLAGS="-O2 -UNDEBUG",
        LEAF_WORD_BITS=args.wordbits,
        LEAF_RUNTIME_BACKEND="amd64",
    )
    try:
        with patch.dict(os.environ, environment, clear=True), patch.object(subprocess, "run", run):
            runpy.run_path(str(args.driver.resolve()), run_name="__main__")
        if len(compiled) != 1 or not temporary.is_file():
            raise RuntimeError("upstream diagnostic generation contract changed")
        temporary.chmod(0o755)
        temporary.replace(output)
    finally:
        if temporary.exists():
            temporary.unlink()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
