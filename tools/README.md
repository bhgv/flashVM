# Source layout tools

[Русский](README.ru.md). These optional host-side tools are not build/runtime
requirements of detached compilers, FIR or the VM.

[atg_format.py](atg_format.py) formats the seven current Coco/R C grammars. It
lexes strings, comments, C actions and attributes separately; it preserves the
order of grammar tokens and semantic C tokens. It is a layout tool, not a parser
generator or semantic verifier. Coco/R generation and component tests remain
necessary after changes. The attribute lexer targets the C99 argument syntax
used here, not arbitrary future C++ template attributes.

Rules: four-space group nesting; one standalone final period and a blank line
after it; semantic actions at column 73, always last on a physical line. Parser
instructions after an action move to the next line. Action bodies may span
multiple lines, with their opening/closing markers aligned. The special pragma
action remains AFTER its terminating period because Coco/R requires that order.
Quoted periods and C tokens are not grammar terminators.

From repository root:

```sh
python3 tools/atg_format.py --check compiler/*/frontend/*.atg fir/v1/compiler/*.atg fir/v1/frontend/*.atg
python3 tools/test_atg_format.py
# Optional first-party C formatting inside the prelude and semantic actions:
python3 tools/atg_format.py --clang-format /host/path/clang-format \
  compiler/*/frontend/*.atg fir/v1/compiler/*.atg fir/v1/frontend/*.atg
```

Without --check, explicit files are rewritten. --check writes nothing and needs
only Python 3.9+.9+. The formatter verifies token-order preservation before publishing
each file. Optional clang-format is a HOST executable, never a cross-built binary.
Seven regression tests cover action ordering, opaque quoted/comment delimiters,
pragma ordering, indentation, idempotence and malformed groups/actions. All six
project grammar/prelude/action streams were compared with their prior versions;
16 frontend and 3 FIR CTests passed after regeneration.

[test_portable_headers.py](test_portable_headers.py) checks combined public
C99 headers and diagnostics for unsupported byte/int/double representations.
Run python3 tools/test_portable_headers.py --cc /host/path/cc with GCC/Clang
command syntax; generated target code is never executed by this checker.

[test_toolchain_contract.py](test_toolchain_contract.py) runs actual isolated
CMake configure probes; --cmake, --cc and --cxx select host tools. It checks
that missing target C++ cannot silently fall back to a host compiler.

`generate_parser_unicode.py --check` verifies pinned Unicode 3.2 category data
and reserved keyword tables of all three AS frontends. Regeneration needs
Python; building the checked-in C99 tables does not. See
[lexical profile](../compiler/LEXICAL_PROFILE.md).

[Owned Coco/R/Limbo sources and update policy / Источники](UPSTREAMS.md) · [Русский](UPSTREAMS.ru.md).

[Inferno mk](mk/README.md) · [Inferno iyacc](iyacc/README.md): selected experiment 3e3536578b969cf1a460210d8ec667713869ccfc, 82 / 17 upstream files, explicit dependencies and bootstrap scripts.
