# AS1 syntax frontend

Version 0.2.6. [Русская версия](README.ru.md).

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

[letter](as1.atg#L74) | [digit](as1.atg#L80) | [hex](as1.atg#L85) | [dq](as1.atg#L90) | [sq](as1.atg#L95) | [linechar](as1.atg#L100) | [escaped](as1.atg#L105) | [nonstar](as1.atg#L111) | [nonstarslash](as1.atg#L115) | [ident](as1.atg#L121) | [number](as1.atg#L130) | [hexnum](as1.atg#L164) | [text](as1.atg#L177) | [blockcomment](as1.atg#L194) | [linecomment](as1.atg#L212) | [As1](as1.atg#L225) | [Item](as1.atg#L233) | [Block](as1.atg#L238) | [Statement](as1.atg#L249) | [IfStatement](as1.atg#L284) | [WhileStatement](as1.atg#L292) | [ForStatement](as1.atg#L299) | [SwitchStatement](as1.atg#L322) | [TryStatement](as1.atg#L331) | [SwitchPart](as1.atg#L343) | [Variables](as1.atg#L354) | [Variable](as1.atg#L362) | [FunctionTail](as1.atg#L370) | [Parameter](as1.atg#L384) | [Expression](as1.atg#L389) | [Assignment](as1.atg#L398) | [AssignOp](as1.atg#L410) | [Conditional](as1.atg#L427) | [LogicalOr](as1.atg#L439) | [LogicalAnd](as1.atg#L447) | [BitOr](as1.atg#L455) | [BitXor](as1.atg#L463) | [BitAnd](as1.atg#L471) | [Equality](as1.atg#L479) | [Relational](as1.atg#L495) | [Shift](as1.atg#L516) | [Additive](as1.atg#L529) | [Product](as1.atg#L542) | [Unary](as1.atg#L555) | [Postfix](as1.atg#L577) | [Arguments](as1.atg#L591) | [Primary](as1.atg#L604) | [Subscript](as1.atg#L630) | [ArrayLiteral](as1.atg#L637) | [ObjectLiteral](as1.atg#L653) | [GroupedExpression](as1.atg#L666) | [Property](as1.atg#L673)

## Language reference

[ActionScript language reference (archived Adobe documentation)](https://open-flash.github.io/mirrors/as2-language-reference/language-elements.html).

`in` is supported in expressions; NoIn applies only to for initializers.
Groups, arguments, subscripts and literals restore ordinary expressions.
