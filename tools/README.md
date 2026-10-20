# Source layout tools

[Русский](README.ru.md). These optional host-side tools are not build/runtime
requirements of detached compilers, FIR or the VM.

[atg_format.py](atg_format.py) formats the six current Coco/R C grammars. It
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
python3 tools/atg_format.py --check compiler/*/frontend/*.atg fir/v1/compiler/*.atg
python3 tools/test_atg_format.py
# Optional first-party C formatting inside the prelude and semantic actions:
python3 tools/atg_format.py --clang-format /host/path/clang-format \
  compiler/*/frontend/*.atg fir/v1/compiler/*.atg
```

Without --check, explicit files are rewritten. --check writes nothing and needs
only Python 3. The formatter verifies token-order preservation before publishing
each file. Optional clang-format is a HOST executable, never a cross-built binary.
Seven regression tests cover action ordering, opaque quoted/comment delimiters,
pragma ordering, indentation, idempotence and malformed groups/actions. All six
project grammar/prelude/action streams were compared with their prior versions;
16 frontend and 3 FIR CTests passed after regeneration.
