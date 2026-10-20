# AS1 syntax frontend

Version 0.4.0. [Русская версия](README.ru.md).

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
The scanner reports UTF-8 byte columns. Identifier categories, Unicode whitespace
and escape spelling follow the [current lexical profile](../../LEXICAL_PROFILE.md).
String values are not evaluated by this syntax API.

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

Automatic `;` insertion is supported at a line break, before `}` and at EOF.
Return/break/continue and postfix ++/-- respect line breaks, including comments;
after throw a line break is rejected (E1102). For-header semicolons remain mandatory.
Classic on/onClipEvent wrappers, #include and paired #initclip/#endinitclip are
recognized without opening includes or attaching events to a scene. Ordinary
on/onClipEvent calls without a following block retain expression syntax.
Legacy and/or/not operators are supported; event names need profile validation.
AS1/AS2 regexp literals and assignment-target/context checks remain unimplemented.
Unicode 3.2 category tables and identifier escapes are now supported. This syntax profile is
not a complete compiler implementation.

## Checks

[tests/check.py](tests/check.py): Linux generation/build, valid/invalid syntax,
UTF-8/BOM, malformed encoding, NUL and missing input; each process is time-bounded.
A copied, renamed standalone frontend also configures, builds and passes its tests
with `-pedantic-errors`.

## Grammar rule index

[letter](as1.atg#L368) | [digit](as1.atg#L374) | [hex](as1.atg#L379) | [dq](as1.atg#L384) | [sq](as1.atg#L389) | [linechar](as1.atg#L394) | [escaped](as1.atg#L399) | [nonstar](as1.atg#L405) | [nonstarslash](as1.atg#L409) | [ident](as1.atg#L426) | [number](as1.atg#L439) | [hexnum](as1.atg#L473) | [text](as1.atg#L486) | [blockcomment](as1.atg#L519) | [linecomment](as1.atg#L537) | [As1](as1.atg#L550) | [Item](as1.atg#L558) | [Block](as1.atg#L568) | [Statement](as1.atg#L579) | [IfStatement](as1.atg#L621) | [WhileStatement](as1.atg#L629) | [ForStatement](as1.atg#L636) | [SwitchStatement](as1.atg#L659) | [TryStatement](as1.atg#L668) | [SwitchPart](as1.atg#L680) | [Variables](as1.atg#L691) | [Variable](as1.atg#L699) | [FunctionTail](as1.atg#L707) | [Parameter](as1.atg#L721) | [Expression](as1.atg#L726) | [Assignment](as1.atg#L735) | [AssignOp](as1.atg#L752) | [Conditional](as1.atg#L769) | [LogicalOr](as1.atg#L781) | [LogicalAnd](as1.atg#L793) | [BitOr](as1.atg#L805) | [BitXor](as1.atg#L813) | [BitAnd](as1.atg#L821) | [Equality](as1.atg#L829) | [Relational](as1.atg#L845) | [Shift](as1.atg#L866) | [Additive](as1.atg#L879) | [Product](as1.atg#L893) | [Unary](as1.atg#L907) | [Postfix](as1.atg#L930) | [Arguments](as1.atg#L950) | [Primary](as1.atg#L963) | [Subscript](as1.atg#L989) | [ArrayLiteral](as1.atg#L996) | [ObjectLiteral](as1.atg#L1012) | [GroupedExpression](as1.atg#L1025) | [Property](as1.atg#L1032)

## Language reference

[ActionScript language reference (archived Adobe documentation)](https://open-flash.github.io/mirrors/as2-language-reference/language-elements.html).

`in` is supported in expressions; NoIn applies only to for initializers.
Groups, arguments, subscripts and literals restore ordinary expressions.

New rules / [ClipInitializer](as1.atg#L1042) | [EventHandler](as1.atg#L1055) | [Terminator](as1.atg#L1073)

[Lexical profile 0.6.0](../../LEXICAL_PROFILE.md).

0.6.0: [Identifier](as1.atg#L1085) | [IdentifierUnit](as1.atg#L32) | [IdentifierLine](as1.atg#L211) | [as1_identifier_valid](as1.atg#L85) | [as1_identifier_space](as1.atg#L109) | [as1_string_valid](as1.atg#L171).
