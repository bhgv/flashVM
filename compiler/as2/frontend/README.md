# AS2 syntax frontend

Version 0.2.5. [Русская версия](README.ru.md).

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

[letter](as2.atg#L74) | [digit](as2.atg#L80) | [hex](as2.atg#L85) | [dq](as2.atg#L90) | [sq](as2.atg#L95) | [linechar](as2.atg#L100) | [escaped](as2.atg#L105) | [nonstar](as2.atg#L111) | [nonstarslash](as2.atg#L115) | [ident](as2.atg#L121) | [number](as2.atg#L130) | [hexnum](as2.atg#L164) | [text](as2.atg#L177) | [blockcomment](as2.atg#L194) | [linecomment](as2.atg#L212) | [As2](as2.atg#L225) | [Item](as2.atg#L233) | [Block](as2.atg#L241) | [Statement](as2.atg#L252) | [IfStatement](as2.atg#L287) | [WhileStatement](as2.atg#L295) | [ForStatement](as2.atg#L302) | [SwitchStatement](as2.atg#L325) | [TryStatement](as2.atg#L334) | [SwitchPart](as2.atg#L350) | [Variables](as2.atg#L361) | [Variable](as2.atg#L369) | [FunctionTail](as2.atg#L380) | [Parameter](as2.atg#L398) | [Expression](as2.atg#L406) | [Assignment](as2.atg#L415) | [AssignOp](as2.atg#L427) | [Conditional](as2.atg#L444) | [LogicalOr](as2.atg#L456) | [LogicalAnd](as2.atg#L464) | [BitOr](as2.atg#L472) | [BitXor](as2.atg#L480) | [BitAnd](as2.atg#L488) | [Equality](as2.atg#L496) | [Relational](as2.atg#L512) | [Shift](as2.atg#L533) | [Additive](as2.atg#L546) | [Product](as2.atg#L559) | [Unary](as2.atg#L572) | [Postfix](as2.atg#L594) | [Arguments](as2.atg#L608) | [Primary](as2.atg#L621) | [Subscript](as2.atg#L647) | [ArrayLiteral](as2.atg#L654) | [ObjectLiteral](as2.atg#L670) | [GroupedExpression](as2.atg#L683) | [Property](as2.atg#L690) | [TypeName](as2.atg#L700) | [Import](as2.atg#L708) | [Class](as2.atg#L721) | [Interface](as2.atg#L743) | [Modifier](as2.atg#L759) | [Member](as2.atg#L768) | [Signature](as2.atg#L784)

## Language reference

[ActionScript language reference (archived Adobe documentation)](https://open-flash.github.io/mirrors/as2-language-reference/language-elements.html).

`in` is supported in expressions; NoIn applies only to for initializers.
Groups, arguments, subscripts and literals restore ordinary expressions.
