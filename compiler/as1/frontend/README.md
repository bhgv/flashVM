# AS1 syntax frontend

Version 0.1.0. [Русская версия](README.ru.md).

The readable [as1.atg](as1.atg) is the authoritative scanner/parser input.
It targets the C backend of [bhgv/coco-r-C](https://github.com/bhgv/coco-r-C).
The embedded predicates are C99; the executable and generated code are built as
C99 with extensions disabled. This is a syntax recognizer, not yet a complete
compiler: there is no AST, symbol table, type checker, code generator or SWF writer.

## Build and detach

Provide an already-built external C generator and its matching `frames/` directory.
No Coco/R source, frame or documentation is copied into this repository. Run from
this frontend directory:

```sh
cmake -S . -B build \
  -DCOCOR_ROOT=/absolute/path/to/coco-r-C \
  -DCOCOR_EXECUTABLE=/absolute/path/to/coco-r-C/cocor
cmake --build build
ctest --test-dir build --output-on-failure
./build/as1_parse /path/to/source
```

The executable needs no generator at runtime. Python 3 is required only for tests;
use `-DBUILD_TESTING=OFF` for a build without it. CMake 3.16+ and a C99 compiler are
required. Windows multi-configuration builds place the executable in the selected
configuration directory (for example `build/Debug`). Windows was not run here.
A compiler-wide optional build is in [../../CMakeLists.txt](../../CMakeLists.txt).
The same configuration options work from the language project directory.

This directory can be copied or renamed and configured without FlashVM, linker,
other frontends or workspace helpers. Keep the common root MIT LICENSE in a
separated distribution; generated files also carry their upstream frame notices.
Generated C and headers live only in `build/generated/` and are not source commits.
Each frontend also exposes a namespaced C library. Sequential calls reset
state; serialize calls to the same parser. See the language README for C API.

## Driver and diagnostics

[main.c](main.c) opens one source file and delegates to [api.c](api.c),
which validates UTF-8 before parsing. Malformed UTF-8, surrogates, overlong encodings, values
above U+10FFFF, incomplete sequences and embedded NUL are rejected. An initial
UTF-8 BOM is supported by the selected generator. Inputs must be seekable files.
The scanner counts **bytes**, not Unicode columns; identifier category restrictions
and escape-value semantics remain future validation work. Strings preserve bytes.

| Driver function | Contract |
| --- | --- |
| [main](main.c#L97) | One input path; opens/closes the descriptor; returns 0 for syntax acceptance, 1 for syntax/encoding rejection, 2 for usage/I/O failure |
| [validate_source](main.c#L33) | Validates encoding without modifying source bytes; returns 0/1/2 and rewinds the descriptor on success |
| [report_error](main.c#L85) | Synchronous generated-parser callback; prints path, line, byte column, diagnostic text/code and byte offset; owns no memory |

The driver uses generated `Parse`, `Successful` and `Custom_Error`; these are
upstream generated interfaces, not a stable public compiler API. Error recovery
may stop at the first useful diagnostic; process status is authoritative. A syntax
acceptance does not prove valid types, assignable left operands, valid modifiers,
control-flow placement or any bytecode/SWF compatibility.

## Syntax coverage and boundaries

Supported: variables, functions/closures, object/array literals (including array
holes), calls/member/index chains, labels, conditional and loop statements,
for-in, switch, try/catch/finally, with, returns and throws. Expressions have
separate precedence levels; assignment and conditional are right-recursive.
LabelAhead distinguishes labels through whitespace/comments without consuming
scanner state. Explicit predicates resolve block/function statement ambiguity
and bind else to the nearest if. Syntax tests include both valid and invalid input.

This initial grammar requires explicit semicolons; automatic semicolon insertion
and newline-sensitive return/postfix rules are not implemented. Regular-expression
literals, the general `in` expression operator, preprocessing (`#include`), timeline
`on`/`onClipEvent` attachment syntax, escaped identifiers and a complete Unicode
identifier-category check are not implemented. It is a documented initial syntax
profile, not full AS language conformance. The parser does not execute source.

## Checks

[tests/check.py](tests/check.py): Linux generation/build, valid/invalid syntax,
UTF-8/BOM, malformed encoding, NUL and missing input; each process is time-bounded.
A copied, renamed standalone frontend also configures, builds and passes its tests
with `-pedantic-errors`.

## Grammar rule index

[As1](as1.atg#L84) | [Item](as1.atg#L89) | [Block](as1.atg#L94) | [Statement](as1.atg#L101) | [IfStatement](as1.atg#L120) | [WhileStatement](as1.atg#L126) | [ForStatement](as1.atg#L133) | [SwitchStatement](as1.atg#L142) | [TryStatement](as1.atg#L147) | [SwitchPart](as1.atg#L156) | [Variables](as1.atg#L161) | [Variable](as1.atg#L166) | [FunctionTail](as1.atg#L171) | [Parameter](as1.atg#L176) | [Expression](as1.atg#L181) | [Assignment](as1.atg#L187) | [AssignOp](as1.atg#L192) | [Conditional](as1.atg#L209) | [LogicalOr](as1.atg#L214) | [LogicalAnd](as1.atg#L219) | [BitOr](as1.atg#L224) | [BitXor](as1.atg#L229) | [BitAnd](as1.atg#L234) | [Equality](as1.atg#L239) | [Relational](as1.atg#L244) | [Shift](as1.atg#L249) | [Additive](as1.atg#L254) | [Product](as1.atg#L259) | [Unary](as1.atg#L264) | [Postfix](as1.atg#L270) | [Arguments](as1.atg#L275) | [Primary](as1.atg#L280) | [Property](as1.atg#L298)

## Language reference

[ActionScript language reference (archived Adobe documentation)](https://open-flash.github.io/mirrors/as2-language-reference/language-elements.html).
