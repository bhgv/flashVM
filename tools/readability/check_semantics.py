#!/usr/bin/env python3
"""Compile real C89/C++98 control-flow fixtures before and after native formatting.

Parameters (CLI): --formatter selects clang-format 18; CC/CXX choose compilers.
Returns: zero after both modes preserve independently expected behavior.
Failures propagate as a nonzero exit; temporary binaries never enter the tree.
"""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile

from format_native import declaration_spacing, directive_tokens, native_tokens


ROOT = Path(__file__).resolve().parents[2]  # Repository owning the checked profile.
# Raw oracle text deliberately exercises compressed and ambiguous-looking layout.
SOURCE = r"""
#include <stdio.h>
#define BUMP(value) do { (value)++; } while (0)
static int choose(int a,int b){ if(a) if(b) return 10; else return 20; return 30; }
static int exercise(void){ int x=0,i=0; const char *text="if(x) { ; } else // literal";
 for(i=0;i<5;i++) if(i&1) BUMP(x); else x+=2;
 while(x<10) x++; do x--; while(x>8);
 if(text[0]!='i') return -1; return x; }
#define WRAPPER(name, op) \
 static int name(int value) { return value op 1; }
WRAPPER(next_value, +)
int main(int argc,char **argv){ (void)argv;
 if(argc>1){ fputs("fixture rejected\n",stderr); return 9; }
 if(choose(0,0)!=30) return 1; if(choose(1,0)!=20) return 2;
 if(choose(1,1)!=10) return 3; if(exercise()!=8) return 4;
 if(next_value(8)!=9) return 5; puts("control flow preserved"); return 0; }
"""


# Compile positive and negative fixtures with checks enabled even under NDEBUG.
# Parameters: none in Python; CLI selects the formatter and CC/CXX select compilers.
# Returns normally on success; compilation, output or control-flow errors raise.
def main():
    """Check C and C++ builds; CLI arguments select only the formatter executable."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--formatter", default="clang-format")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="native-readability-") as directory:
        root = Path(directory)
        for suffix, compiler, standard in (
            (".c", os.environ.get("CC", "cc"), "c89"),
            (".cpp", os.environ.get("CXX", "c++"), "c++98"),
        ):
            source = root / ("control" + suffix)
            formatted = subprocess.check_output(
                [
                    args.formatter,
                    "--style=file:" + str(ROOT / ".clang-format"),
                    "--assume-filename=" + str(source),
                ],
                input=SOURCE,
                text=True,
            )
            formatted = declaration_spacing(formatted)
            if directive_tokens(SOURCE) != directive_tokens(formatted):
                raise RuntimeError("preprocessor boundaries changed")
            if native_tokens(SOURCE) != native_tokens(formatted):
                raise RuntimeError("a non-layout fixture token changed")
            for name, text in (("before", SOURCE), ("after", formatted)):
                source.write_text(text)
                binary = root / (name + suffix + ".bin")
                subprocess.run(
                    [
                        compiler,
                        "-std=" + standard,
                        "-DNDEBUG",
                        "-pedantic-errors",
                        str(source),
                        "-o",
                        str(binary),
                    ],
                    check=True,
                )
                result = subprocess.check_output([str(binary)], text=True)
                if result != "control flow preserved\n":
                    raise RuntimeError("unexpected fixture output")
                rejected = subprocess.run([str(binary), "reject"], capture_output=True, text=True)
                if rejected.returncode != 9 or rejected.stderr != "fixture rejected\n":
                    raise RuntimeError("explicit negative check was disabled or changed")
    print("C89/C++98: dangling else, loops, do/while macro and literals preserved")


if __name__ == "__main__":
    main()
