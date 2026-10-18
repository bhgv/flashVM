# AS2 syntax frontend

Version 0.1.0. [Русская версия](README.ru.md).

The readable [as2.atg](as2.atg) is the authoritative scanner/parser input.
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
./build/as2_parse /path/to/source
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
Each frontend executable is separate: generated names/global state are not safe
to combine in one library or call concurrently. Use one file per process for now.

## Driver and diagnostics

[main.c](main.c) opens exactly one source file, validates UTF-8 and rewinds the same
descriptor before parsing. Malformed UTF-8, surrogates, overlong encodings, values
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

AS2 additionally recognizes typed variables/parameters/results, import, class,
interface, extends, implements, modifiers, methods and accessor-shaped members.
Modifier/accessor legality and declaration placement still require semantic checks.

## Checks

[tests/check.py](tests/check.py): Linux generation/build, valid/invalid syntax,
UTF-8/BOM, malformed encoding, NUL and missing input; each process is time-bounded.
A copied, renamed standalone frontend also configures, builds and passes its tests
with `-pedantic-errors`.

## Grammar rule index

[As2](as2.atg#L84) | [Item](as2.atg#L89) | [Block](as2.atg#L97) | [Statement](as2.atg#L104) | [IfStatement](as2.atg#L123) | [WhileStatement](as2.atg#L129) | [ForStatement](as2.atg#L136) | [SwitchStatement](as2.atg#L145) | [TryStatement](as2.atg#L150) | [SwitchPart](as2.atg#L159) | [Variables](as2.atg#L164) | [Variable](as2.atg#L169) | [FunctionTail](as2.atg#L174) | [Parameter](as2.atg#L179) | [Expression](as2.atg#L184) | [Assignment](as2.atg#L190) | [AssignOp](as2.atg#L195) | [Conditional](as2.atg#L212) | [LogicalOr](as2.atg#L217) | [LogicalAnd](as2.atg#L222) | [BitOr](as2.atg#L227) | [BitXor](as2.atg#L232) | [BitAnd](as2.atg#L237) | [Equality](as2.atg#L242) | [Relational](as2.atg#L247) | [Shift](as2.atg#L252) | [Additive](as2.atg#L257) | [Product](as2.atg#L262) | [Unary](as2.atg#L267) | [Postfix](as2.atg#L273) | [Arguments](as2.atg#L278) | [Primary](as2.atg#L283) | [Property](as2.atg#L301) | [TypeName](as2.atg#L306) | [Import](as2.atg#L311) | [Class](as2.atg#L316) | [Interface](as2.atg#L322) | [Modifier](as2.atg#L327) | [Member](as2.atg#L336) | [Signature](as2.atg#L341)

## Language reference

[ActionScript language reference (archived Adobe documentation)](https://open-flash.github.io/mirrors/as2-language-reference/language-elements.html).
