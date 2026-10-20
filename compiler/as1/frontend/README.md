# AS1 syntax frontend

Version 0.2.2. [Русская версия](README.ru.md).

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

Public interface: [frontend.h](frontend.h), [api.c](api.c).

`as1_check(bytes, size, diagnostic)` / `as1_read(FILE*, diagnostic)`
return 1 accepted / 0 rejected; diagnostic may be NULL. Input storage and
streams belong to the caller. Serialize calls to one parser; calls after
failure are supported. Input is limited to 2 MiB UTF-8. Codes -3/-4 indicate
nesting/work exhaustion; they are implementation limits, not language errors.
Coordinates may be zero on resource failure. Syntax recognition does not
replace type, symbol or return/break context analysis.

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
literals, preprocessing (`#include`), timeline
`on`/`onClipEvent` attachment syntax, escaped identifiers and a complete Unicode
identifier-category check are not implemented. It is a documented initial syntax
profile, not full AS language conformance. The parser does not execute source.

## Checks

[tests/check.py](tests/check.py): Linux generation/build, valid/invalid syntax,
UTF-8/BOM, malformed encoding, NUL and missing input; each process is time-bounded.
A copied, renamed standalone frontend also configures, builds and passes its tests
with `-pedantic-errors`.

## Grammar rule index

[letter](as1.atg#L70) | [digit](as1.atg#L72) | [hex](as1.atg#L74) | [dq](as1.atg#L76) | [sq](as1.atg#L78) | [linechar](as1.atg#L80) | [escaped](as1.atg#L82) | [nonstar](as1.atg#L85) | [nonstarslash](as1.atg#L86) | [ident](as1.atg#L89) | [number](as1.atg#L91) | [hexnum](as1.atg#L95) | [text](as1.atg#L97) | [blockcomment](as1.atg#L100) | [linecomment](as1.atg#L102) | [As1](as1.atg#L109) | [Item](as1.atg#L114) | [Block](as1.atg#L119) | [Statement](as1.atg#L126) | [IfStatement](as1.atg#L145) | [WhileStatement](as1.atg#L151) | [ForStatement](as1.atg#L158) | [SwitchStatement](as1.atg#L165) | [TryStatement](as1.atg#L170) | [SwitchPart](as1.atg#L179) | [Variables](as1.atg#L184) | [Variable](as1.atg#L189) | [FunctionTail](as1.atg#L194) | [Parameter](as1.atg#L199) | [Expression](as1.atg#L204) | [Assignment](as1.atg#L210) | [AssignOp](as1.atg#L215) | [Conditional](as1.atg#L232) | [LogicalOr](as1.atg#L237) | [LogicalAnd](as1.atg#L242) | [BitOr](as1.atg#L247) | [BitXor](as1.atg#L252) | [BitAnd](as1.atg#L257) | [Equality](as1.atg#L262) | [Relational](as1.atg#L267) | [Shift](as1.atg#L272) | [Additive](as1.atg#L277) | [Product](as1.atg#L282) | [Unary](as1.atg#L287) | [Postfix](as1.atg#L293) | [Arguments](as1.atg#L298) | [Primary](as1.atg#L303) | [Subscript](as1.atg#L321) | [ArrayLiteral](as1.atg#L324) | [ObjectLiteral](as1.atg#L328) | [GroupedExpression](as1.atg#L332) | [Property](as1.atg#L336)

## Language reference

[ActionScript language reference (archived Adobe documentation)](https://open-flash.github.io/mirrors/as2-language-reference/language-elements.html).

`in` is supported in expressions; NoIn applies only to for initializers.
Groups, arguments, subscripts and literals restore ordinary expressions.
