# AS2 syntax frontend

Version 0.2.2. [Русская версия](README.ru.md).

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

`as2_check(bytes, size, diagnostic)` / `as2_read(FILE*, diagnostic)`
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

AS2 additionally recognizes typed variables/parameters/results, import, class,
interface, extends, implements, modifiers, methods and accessor-shaped members.
Modifier/accessor legality and declaration placement still require semantic checks.

## Checks

[tests/check.py](tests/check.py): Linux generation/build, valid/invalid syntax,
UTF-8/BOM, malformed encoding, NUL and missing input; each process is time-bounded.
A copied, renamed standalone frontend also configures, builds and passes its tests
with `-pedantic-errors`.

## Grammar rule index

[letter](as2.atg#L70) | [digit](as2.atg#L72) | [hex](as2.atg#L74) | [dq](as2.atg#L76) | [sq](as2.atg#L78) | [linechar](as2.atg#L80) | [escaped](as2.atg#L82) | [nonstar](as2.atg#L85) | [nonstarslash](as2.atg#L86) | [ident](as2.atg#L89) | [number](as2.atg#L91) | [hexnum](as2.atg#L95) | [text](as2.atg#L97) | [blockcomment](as2.atg#L100) | [linecomment](as2.atg#L102) | [As2](as2.atg#L109) | [Item](as2.atg#L114) | [Block](as2.atg#L122) | [Statement](as2.atg#L129) | [IfStatement](as2.atg#L148) | [WhileStatement](as2.atg#L154) | [ForStatement](as2.atg#L161) | [SwitchStatement](as2.atg#L168) | [TryStatement](as2.atg#L173) | [SwitchPart](as2.atg#L182) | [Variables](as2.atg#L187) | [Variable](as2.atg#L192) | [FunctionTail](as2.atg#L197) | [Parameter](as2.atg#L202) | [Expression](as2.atg#L207) | [Assignment](as2.atg#L213) | [AssignOp](as2.atg#L218) | [Conditional](as2.atg#L235) | [LogicalOr](as2.atg#L240) | [LogicalAnd](as2.atg#L245) | [BitOr](as2.atg#L250) | [BitXor](as2.atg#L255) | [BitAnd](as2.atg#L260) | [Equality](as2.atg#L265) | [Relational](as2.atg#L270) | [Shift](as2.atg#L275) | [Additive](as2.atg#L280) | [Product](as2.atg#L285) | [Unary](as2.atg#L290) | [Postfix](as2.atg#L296) | [Arguments](as2.atg#L301) | [Primary](as2.atg#L306) | [Subscript](as2.atg#L324) | [ArrayLiteral](as2.atg#L327) | [ObjectLiteral](as2.atg#L331) | [GroupedExpression](as2.atg#L335) | [Property](as2.atg#L339) | [TypeName](as2.atg#L344) | [Import](as2.atg#L349) | [Class](as2.atg#L354) | [Interface](as2.atg#L360) | [Modifier](as2.atg#L365) | [Member](as2.atg#L374) | [Signature](as2.atg#L379)

## Language reference

[ActionScript language reference (archived Adobe documentation)](https://open-flash.github.io/mirrors/as2-language-reference/language-elements.html).

`in` is supported in expressions; NoIn applies only to for initializers.
Groups, arguments, subscripts and literals restore ordinary expressions.
