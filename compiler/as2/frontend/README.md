# AS2 syntax frontend

Version 0.4.0. [Русская версия](README.ru.md).

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
The scanner reports UTF-8 byte columns. Identifier categories, Unicode whitespace
and escape spelling follow the [current lexical profile](../../LEXICAL_PROFILE.md).
String values are not evaluated by this syntax API.

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

[letter](as2.atg#L371) | [digit](as2.atg#L377) | [hex](as2.atg#L382) | [dq](as2.atg#L387) | [sq](as2.atg#L392) | [linechar](as2.atg#L397) | [escaped](as2.atg#L402) | [nonstar](as2.atg#L408) | [nonstarslash](as2.atg#L412) | [ident](as2.atg#L429) | [number](as2.atg#L442) | [hexnum](as2.atg#L476) | [text](as2.atg#L489) | [blockcomment](as2.atg#L522) | [linecomment](as2.atg#L540) | [As2](as2.atg#L553) | [Item](as2.atg#L561) | [Block](as2.atg#L574) | [Statement](as2.atg#L585) | [IfStatement](as2.atg#L627) | [WhileStatement](as2.atg#L635) | [ForStatement](as2.atg#L642) | [SwitchStatement](as2.atg#L665) | [TryStatement](as2.atg#L674) | [SwitchPart](as2.atg#L690) | [Variables](as2.atg#L701) | [Variable](as2.atg#L709) | [FunctionTail](as2.atg#L720) | [Parameter](as2.atg#L738) | [Expression](as2.atg#L746) | [Assignment](as2.atg#L755) | [AssignOp](as2.atg#L772) | [Conditional](as2.atg#L789) | [LogicalOr](as2.atg#L801) | [LogicalAnd](as2.atg#L813) | [BitOr](as2.atg#L825) | [BitXor](as2.atg#L833) | [BitAnd](as2.atg#L841) | [Equality](as2.atg#L849) | [Relational](as2.atg#L865) | [Shift](as2.atg#L886) | [Additive](as2.atg#L899) | [Product](as2.atg#L913) | [Unary](as2.atg#L927) | [Postfix](as2.atg#L950) | [Arguments](as2.atg#L970) | [Primary](as2.atg#L983) | [Subscript](as2.atg#L1009) | [ArrayLiteral](as2.atg#L1016) | [ObjectLiteral](as2.atg#L1032) | [GroupedExpression](as2.atg#L1045) | [Property](as2.atg#L1052) | [TypeName](as2.atg#L1062) | [Import](as2.atg#L1070) | [Class](as2.atg#L1083) | [Interface](as2.atg#L1105) | [Modifier](as2.atg#L1121) | [Member](as2.atg#L1130) | [Signature](as2.atg#L1146)

## Language reference

[ActionScript language reference (archived Adobe documentation)](https://open-flash.github.io/mirrors/as2-language-reference/language-elements.html).

`in` is supported in expressions; NoIn applies only to for initializers.
Groups, arguments, subscripts and literals restore ordinary expressions.

New rules / [ClipInitializer](as2.atg#L1162) | [EventHandler](as2.atg#L1175) | [Terminator](as2.atg#L1193)

[Lexical profile 0.6.0](../../LEXICAL_PROFILE.md).

0.6.0: [Identifier](as2.atg#L1205) | [IdentifierUnit](as2.atg#L32) | [IdentifierLine](as2.atg#L214) | [as2_identifier_valid](as2.atg#L85) | [as2_identifier_space](as2.atg#L109) | [as2_string_valid](as2.atg#L174).
